#include "dottracker.h"
#include <cstddef>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

void dotTracker::run(int argc, char *argv[]) {
  if (argc == 1)
  {
    std::cerr << "No commands provided";
    return;
  }

  for (int i = 1; i < argc; i++)
  {
    argv_.emplace_back(argv[i]);
  }

  argc_ = argc--;
  initialize_paths();

  for (auto& cm : argv_)
  {
    if (cm == "init") handle_init();
    else if (cm == "add") handle_stage();
    else if (cm == "diff") show_staged_files();
    // else std::cout << "run 'dott --help' for more details";
  }
}

int dotTracker::handle_init()
{

  fs::path current_dir = fs::current_path();
  fs::path dottracker_dir = current_dir / ".dott";

  if (fs::exists(dottracker_dir))
  {
    std::cout << "Reinitialized existing dotTracker repository in "
      << fs::absolute(dottracker_dir).string() << std::endl;

    return 0;
  }

  fs::create_directory(dottracker_dir);
  fs::create_directory(dottracker_dir / "objects");

  std::ofstream head_file(dottracker_dir / "HEAD");
  head_file << "ref: refs/heads/master" << std::endl;
  head_file.close();

  std::ofstream index_file_stream(dottracker_dir / "index");
  index_file_stream.close();

  std::cout << "Initialized empty dotTracker repository in "
    << fs::absolute(dottracker_dir).string() << std::endl;

  return 0;
}

void dotTracker::initialize_paths()
{
  repo_root = fs::current_path();
  while (repo_root != repo_root.parent_path())
  {
    if (fs::exists(repo_root / ".dott"))
    {
      break;
    }
    repo_root = repo_root.parent_path();
  }

  objects_dir = repo_root / ".dott" / "objects";
  index_file = repo_root / ".dott" / "index";

  if (fs::exists(repo_root / ".dott"))
  {
    fs::create_directories(objects_dir);
  }
}

std::string dotTracker::calculate_file_hash(const fs::path& file_path)
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

bool dotTracker::store_object(const fs::path& file_path, const std::string& hash)
{
  std::string obj_dir = hash.substr(0, 2);
  std::string obj_file = hash.substr(2);

  fs::path obj_path = objects_dir / obj_dir;
  fs::create_directories(obj_path);

  fs::path full_obj_path = obj_path / obj_file;

  if (fs::exists(full_obj_path)) return true;

  fs::copy_file(file_path, full_obj_path);
  return true;
}

bool dotTracker::update_index(const std::vector<std::pair<std::string, fs::path>>& staged_files)
{
  std::vector<std::string> index_entries;

  if (fs::exists(index_file))
  {
    std::ifstream existing_index(index_file);
    std::string line;
    std::unordered_set<std::string> new_files;

    for (const auto& [hash, path] : staged_files)
    {
      new_files.insert(fs::relative(path, repo_root).string());
    }
    while (std::getline(existing_index, line))
    {
      if (! line.empty())
      {
        std::size_t space_pos = line.find(' ');
        if (space_pos != std::string::npos)
        {
          std::string file_path = line.substr(space_pos + 1);
          if (new_files.find(file_path) == new_files.end())
          {
            index_entries.push_back(line);
          }
        }
      }
    }
  }

  for (const auto& [hash, path] : staged_files)
  {
    fs::path relative_path = fs::relative(path, repo_root);
    index_entries.push_back(hash + " " + relative_path.string());
  }

  std::ofstream index_out(index_file);
  for (const auto& entry : index_entries)
  {
    index_out << entry << std::endl;
  }

  return true;
}

int dotTracker::handle_stage()
{

  if (argc_ == 1)
  {
    std::cerr << "Nothing specified, Nothing added" << std::endl;
    return 1;
  }

  for (auto it = argv_.begin() + 1; it != argv_.end(); it++)
  {
    fs::path target_path = fs::absolute(*it);
    if (! fs::exists(target_path))
    {
      std::cerr << "pathspec '" << *it << "' did not match any files" << std::endl;
      continue;
    }

    if (fs::is_regular_file(target_path)) paths.emplace_back(target_path);
    
    else if(fs::is_directory(target_path))
    {
      for (const auto& entry : fs::recursive_directory_iterator(target_path))
      {
        if (fs::is_regular_file(entry) &&
            entry.path().filename() != ".dott" &&
            entry.path().string().find("/.dott/") == std::string::npos)
        paths.emplace_back(entry);
      }
    }
  }

  if (paths.empty()) {
    std::cerr << "No files to add." << std::endl;
    return 1;
  }

  std::vector<std::pair<std::string, fs::path>> staged_files;

  for (const auto& file_path : paths)
  {
    std::string hash = calculate_file_hash(file_path);

    if (! store_object(file_path, hash))
    {
       std::cerr << "failed to store :" << file_path << std::endl;
      continue;
    }

    staged_files.emplace_back(hash, file_path);
    std::cout << " add " << file_path << std::endl;

  }

  if (! staged_files.empty())
  {
    if (! update_index(staged_files))
    {
      std::cerr << "failed to update index " << staged_files.size() << std::endl;
      return 1;
    }

    std::cout << "successfully staged " << staged_files.size() << " file(s)" << std::endl;
  }

  return 0;
}

int dotTracker::show_staged_files() {
  if (!fs::exists(index_file)) {
    std::cout << "No files staged for commit." << std::endl;
    return 0;
  }

  std::ifstream index(index_file);
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
