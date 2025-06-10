#pragma once

#include <utility>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

struct PathObj{
  fs::path repo_root;
  fs::path objects_dir;
  fs::path index_file;
};

std::string _calculate_file_hash(const fs::path& file_path);
bool _store_object(const fs::path& file_path, const std::string& hash);
bool _update_index(const std::vector<std::pair<std::string, fs::path>>& staged_files);


void run(int argc, char *argv[]);

bool handle_init(fs::path& dir_path);
bool handle_stage(fs::path& file_path);
bool show_staged_files(fs::path& path);
