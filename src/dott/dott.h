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
bool _update_index(const std::string& hash, const fs::path& file_path);

bool handle_init(const fs::path& dir_path);
bool handle_stage(const fs::path& file_path);
bool show_staged_files(const fs::path& path);
