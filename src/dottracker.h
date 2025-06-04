#pragma once

#include <utility>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class dotTracker
{
private:
  std::vector<std::string> argv_;
  int argc_;
  std::vector<fs::path> paths;
  fs::path repo_root;
  fs::path objects_dir;
  fs::path index_file;

  void initialize_paths();
  std::string calculate_file_hash(const fs::path& file_path);
  bool store_object(const fs::path& file_path, const std::string& hash);
  bool update_index(const std::vector<std::pair<std::string, fs::path>>& staged_files);

public:

  dotTracker() = default;
  void run(int argc, char *argv[]);

  int handle_init();
  int handle_stage();
  int show_staged_files();
};

