#include "dott.h"
#include <cstddef>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <openssl/sha.h>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;


bool handle_init(fs::path& dir_path)
{

  fs::path dott_dir = dir_path / ".dott";
  dott_dir = fs::absolute(dott_dir).lexically_normal();

  if (fs::exists(dott_dir))
  {
    std::cout << "Reinitialized existing Dott repository in "
      << dott_dir.string() << std::endl;

    return true;
  }

  fs::create_directory(dott_dir);
  fs::create_directory(dott_dir / "objects");

  std::string line;
  std::set<std::string> paths;
  const std::string file = std::string(std::getenv("HOME")) + "/.dottracker/dottfiles";

  // load existing
  if (fs::exists(file)) {
    std::ifstream in(file);
    while (std::getline(in, line)) paths.insert(line);
  }
  else {
    throw std::runtime_error("File not found: " + file);
  }
  if (paths.insert(dott_dir.string()).second) {
    // load existing
    std::ofstream out(file, std::ios::app);
    out << dott_dir.string() << std::endl;
  }

  std::ofstream head_file(dott_dir / "HEAD");
  head_file << "ref: refs/heads/master" << std::endl;
  head_file.close();

  std::ofstream index_file_stream(dott_dir / "index");
  index_file_stream.close();

  std::cout << "Initialized empty Dott repository in "
    << dott_dir.string() << std::endl;

  return true;
}

PathObj _initialize_paths(fs::path file_path)
{

  while (file_path != file_path.parent_path())
  {
    if (fs::exists(file_path / ".dott")) break;

    file_path = file_path.parent_path();
  }
  PathObj _path;
  _path.repo_root = file_path;

  _path.objects_dir = _path.repo_root / ".dott" / "objects";
  _path.index_file = _path.repo_root / ".dott" / "index";

  // if (fs::exists(repo_root / ".dott"))
  // {
  //   fs::create_directories(_path.objects_dir);
  // }
 
  return _path;
}

std::string calculate_file_hash(const fs::path& file_path)
{
  std::ifstream file(file_path, std::ios::binary);
  if (! file.is_open())
  {
    throw std::runtime_error("Cannot open: " + file_path.string());
  }

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

bool store_object(fs::path& file_path, const std::string& hash)
{
  std::string obj_dir = hash.substr(0, 2);
  std::string obj_file = hash.substr(2);

  PathObj _path = _initialize_paths(file_path);
  fs::path obj_path = _path.objects_dir / obj_dir;
  fs::create_directories(obj_path);

  fs::path full_obj_path = obj_path / obj_file;

  if (fs::exists(full_obj_path)) return true;

  fs::create_directories(full_obj_path.parent_path());
  fs::copy_file(file_path, full_obj_path);
  return true;
}

bool update_index(std::string hash, fs::path file_path)
{
  std::vector<std::string> index_entries;
  file_path = fs::absolute(file_path);
  PathObj _path = _initialize_paths(file_path);

  if (fs::exists(_path.index_file))
  {
    std::ifstream existing_index(_path.index_file);
    if (! existing_index.is_open()) return false;

    std::string line;
    std::unordered_set<std::string> new_files;

    new_files.insert(fs::relative(file_path, _path.repo_root).string());
    while (std::getline(existing_index, line))
    {
      if (! line.empty())
      {
        std::size_t space_pos = line.find(' ');
        if (space_pos != std::string::npos)
        {
          std::string indexed_rel_path = line.substr(space_pos + 1);
          if (new_files.find(indexed_rel_path) == new_files.end())
          {
            index_entries.push_back(line);
          }
        }
      }
    }
  }

  fs::path relative_path = fs::relative(file_path, _path.repo_root);
  index_entries.push_back(hash + " " + relative_path.string());

  std::ofstream index_out(_path.index_file);
  if (! index_out.is_open()) return false;
  for (const auto& entry : index_entries)
  {
    index_out << entry << std::endl;
  }
  index_out.close();

  return true;
}

bool handle_stage(fs::path& file_path)
{
  std::string hash = calculate_file_hash(file_path);

  if (! store_object(file_path, hash))
  {
    std::cerr << "failed to store :" << file_path << std::endl;
    return false;
  }

  std::cout << " add " << file_path << std::endl;

  if (! update_index(hash, file_path))
  {
    std::cerr << "failed to update index " << std::endl;
    return false;
  }

  return true;
}

bool show_staged_files(fs::path& path) {
  PathObj path_ = _initialize_paths(path); // path: I have no idea somekind of path where it's parent has .dott file
  if (!fs::exists(path_.index_file)) {
    std::cout << "No files staged for commit." << std::endl;
    return 0;
  }

  std::ifstream index(path_.index_file);
  std::string line;
  std::vector<std::string> staged_files;

  while (std::getline(index, line)) {
    if (!line.empty()) {
      size_t space_pos = line.find(' ');
      if (space_pos != std::string::npos) {
        staged_files.push_back(line.substr(space_pos + 1));
      }
    }
  }

  if (staged_files.empty()) {
    std::cout << "No files staged for commit." << std::endl;
  } else {
    std::cout << "Changes to be committed:" << std::endl;
    for (const auto& file : staged_files) {
      std::cout << "\tnew file:   " << file << std::endl;
    }
  }

  return 0;
}
