#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <openssl/sha.h>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>
#include <filesystem>

#include "dott.h"

namespace fs = std::filesystem;
namespace {
  constexpr const char* DOTT_DIR_NAME = ".dott";
  constexpr const char* OBJECTS_DIR_NAME = "objects";
  constexpr const char* INDEX_FILE_NAME = "index";
  constexpr const char* HEAD_FILE_NAME = "HEAD";
  constexpr const char* DEFAULT_TRACKED_REPO_LIST = "/.dottracker/dottfiles";
  constexpr const char* DEFAULT_HEAD_REF = "ref: refs/heads/master";
  constexpr const size_t PREDEFINED_HASH_LENGTH = 2;
}

class DottException : public std::runtime_error
{
public:
  explicit DottException(const std::string& message)
  : std::runtime_error("\033[31mDOTT ERROR:\033[0m " + message) {}
};

bool handle_init(const fs::path& dir_path)
{
  fs::path dott_dir = fs::absolute(dir_path / DOTT_DIR_NAME).lexically_normal();

  if (fs::exists(dott_dir))
  {
    std::cout << "Reinitialized existing Dott repository in "
              << dott_dir.string() << std::endl;

    return true;
  }

  std::error_code ec;
  if (! fs::create_directory(dott_dir, ec))
  {
    if (ec)
      throw DottException("Failed to create `" + dott_dir.string() + "` : " + ec.message());
  }

  if (! fs::create_directory(dott_dir / OBJECTS_DIR_NAME, ec))
  {
    if (ec)
      throw DottException("Failed to create `" + (dott_dir / OBJECTS_DIR_NAME).string() + "` : " + ec.message());
  }

  std::string home_dir = std::getenv("HOME") ? std::getenv("HOME") : "";
  if (home_dir.empty())
    throw DottException("HOME environment variable not set");

  const fs::path config_file = home_dir + DEFAULT_TRACKED_REPO_LIST;

  std::set<std::string> tracked_paths;

  // load existing
  if (fs::exists(config_file)) {
    std::ifstream config_stream(config_file);

    if (! config_stream.is_open())
      throw DottException("Cannot read `" + config_file.string() + "` ");

    std::string line;
    while (std::getline(config_stream, line)){
      if (! line.empty())
        tracked_paths.insert(line);
    }
  }
  else
  {
      fs::create_directory(config_file.parent_path(), ec);
      if (ec)
        throw DottException("Cannot create `" + config_file.parent_path().string() + "`\n" + ec.message());
  }

  if (tracked_paths.insert(dott_dir.string()).second) {
    // load existing
    std::ofstream out(config_file, std::ios::app);
    if (! out.is_open())
        throw DottException("Cannot open `" + config_file.string() + "` ");

    out << dott_dir.string() << std::endl;
  }

  {
    std::string head_file_path = dott_dir / HEAD_FILE_NAME;
    std::ofstream head_file(head_file_path);
    if (! head_file.is_open())
      throw DottException("Cannot open `" + head_file_path + "`");

    head_file << DEFAULT_HEAD_REF << std::endl;
    head_file.close();
  }

  {
    std::string index_file_path = dott_dir / INDEX_FILE_NAME;
    std::ofstream index_file(index_file_path);
    if (! index_file.is_open())
      throw DottException("Cannot open `" + index_file_path + "`");

    index_file.close();
  }

  std::cout << "Initialized empty Dott repository in "
            << dott_dir.string() << std::endl;

  return true;
}

PathObj _initialize_paths(const fs::path& file_path)
{
  fs::path absolute_file_path = fs::absolute(file_path);
  while (absolute_file_path != absolute_file_path.parent_path())
  {
    if (fs::exists(absolute_file_path / DOTT_DIR_NAME)) break;

    absolute_file_path = absolute_file_path.parent_path();
  }

  if (! fs::exists(absolute_file_path / DOTT_DIR_NAME))
    throw DottException("Not a dott repository or any of the parent directory");

  PathObj _path;
  _path.repo_root = absolute_file_path;
  _path.objects_dir = _path.repo_root / DOTT_DIR_NAME / OBJECTS_DIR_NAME;
  _path.index_file = _path.repo_root / DOTT_DIR_NAME / INDEX_FILE_NAME;
 
  return _path;
}

std::string _calculate_file_hash(const fs::path& file_path)
{
  if (! fs::exists(file_path))
    throw DottException("File does not exists: " + file_path.string());

  std::ifstream file(file_path, std::ios::binary);

  if (! file.is_open())
    throw DottException("Cannot open: " + file_path.string());
  

  std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA1(reinterpret_cast<const unsigned char*>(content.c_str()),
       content.length(), hash);

  std::stringstream ss;
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
  {
    ss << std::hex
       << std::setw(2)
       << std::setfill('0')
       << static_cast<int>(hash[i]);
  }

  return ss.str();
}

bool _store_object(const fs::path& file_path, const std::string& hash)
{
  if (hash.length() < PREDEFINED_HASH_LENGTH)
    throw DottException("Invalid hash: " + hash);

  std::string obj_dir = hash.substr(0, PREDEFINED_HASH_LENGTH);
  std::string obj_file = hash.substr(PREDEFINED_HASH_LENGTH);

  PathObj _path = _initialize_paths(file_path);
  fs::path obj_path = _path.objects_dir / obj_dir;
  fs::create_directories(obj_path);

  fs::path full_obj_path = obj_path / obj_file;

  if (fs::exists(full_obj_path)) return true;

  std::error_code ec;
  fs::create_directories(full_obj_path.parent_path(), ec);
  if (ec)
    throw DottException("Failed to create to create object directory: " + ec.message());

  fs::copy_file(file_path, full_obj_path, ec);
  if (ec)
    throw DottException("Failed to store object: " + ec.message());

  return true;
}

bool _update_index(const std::string& hash, const fs::path& file_path)
{
  try
  {
    fs::path absolute_file_path = fs::absolute(file_path);
    PathObj _path = _initialize_paths(absolute_file_path);

    std::vector<std::string> index_entries;
    std::string relative_path = fs::relative(absolute_file_path, _path.repo_root).string();

    if (fs::exists(_path.index_file))
    {
      std::ifstream existing_index(_path.index_file);
      if (! existing_index.is_open())
        throw DottException("Cannot read index file");

      std::string line;
      std::unordered_set<std::string> new_files;

      new_files.insert(relative_path);

      while (std::getline(existing_index, line))
      {
        if (line.empty()) continue;

        std::size_t space_pos = line.find(' ');
        if (space_pos == std::string::npos) continue;

        std::string indexed_rel_path = line.substr(space_pos + 1);
        if (indexed_rel_path != relative_path)
          index_entries.push_back(std::move(line));
      }
    }

    index_entries.push_back(hash + " " + relative_path);

    std::ofstream index_out(_path.index_file);
    if (! index_out.is_open())
      throw DottException("Cannot read index file");

    for (const auto& entry : index_entries)
    {
      index_out << entry << std::endl;
    }
    index_out.close();

    return true;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error updating index: " << e.what();
    return false;
  }
}

bool handle_stage(const fs::path& file_path)
{
  try
  {
    const fs::path absolute_file_path = fs::absolute(file_path);
    const std::string hash = _calculate_file_hash(absolute_file_path);

    if (! _store_object(absolute_file_path, hash))
    {
      std::cerr << "Failed to store :" << absolute_file_path << std::endl;
      return false;
    }

    std::cout << " add " << file_path << std::endl;

    if (! _update_index(hash, absolute_file_path))
    {
      std::cerr << "failed to update index " << std::endl;
      return false;
    }

    return true;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error while staging file: " << e.what() << std::endl;
    return false;
  }
}

bool show_staged_files(const fs::path& path) {
  try
  {
    PathObj path_ = _initialize_paths(path);

    if (! fs::exists(path_.index_file)) {
      std::cout << "No files staged for commit." << std::endl;
      return true;
    }

    std::ifstream index(path_.index_file);
    if (! index.is_open())
      throw DottException("Cannot read index file");

    std::string line;
    std::vector<std::string> staged_files;
    staged_files.reserve(50);

    while (std::getline(index, line)) {
      if (line.empty()) continue;

      size_t space_pos = line.find(' ');
      if (space_pos != std::string::npos) {
        staged_files.push_back(line.substr(space_pos + 1));
      }
    }

    if (staged_files.empty()) {
      std::cout << "No files staged for commit." << std::endl;
    }
    else
  {
      std::cout << "Changes to be committed:" << std::endl;
      for (const auto& file : staged_files) {
        std::cout << "\tnew file:   " << file << std::endl;
      }
    }

    return true;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error while showing staged files: " << e.what() << std::endl;
    return false;
  }
}
