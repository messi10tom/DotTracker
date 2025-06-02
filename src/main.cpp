#include <fstream>
#include <filesystem>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <regex>
#include <openssl/sha.h>

namespace fs = std::filesystem;
fs::path PATH;

std::optional<std::vector<std::string>> match(std::string const& str, std::regex rx)
{
  std::smatch m;

  if (std::regex_match(str, m, rx, std::regex_constants::match_not_null))
  {
    std::vector<std::string> v;

    for (auto const& e : m)
    {
      v.emplace_back(std::string(e));
    }

    return v;
  }

  return {};
}

std::string hash_content(const std::string& content) {
  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA1((unsigned char*)content.c_str(), content.size(), hash);

  std::ostringstream oss;
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    oss << std::hex << (int)hash[i];
  }
  return oss.str();
}

void write_object(const std::string& hash, const std::string& content) {
  fs::path dir = PATH / ".dt/objects/" / hash.substr(0, 2);
  fs::create_directories(dir);
  std::string file = dir / hash.substr(2);

  if (! fs::exists(file)) {
    std::ofstream out(file);
    out << content;
    out.close();
  }
}

void stage_file(const fs::path& path) {
  std::ifstream file(path);
  std::stringstream buffer;

  buffer << file.rdbuf();

  std::string content = buffer.str();

  std::cout << content;
  std::string hash = hash_content(content);
  write_object(hash, content);

  std::ofstream index(PATH / ".dt/index/", std::ios::app);
  index << hash << " " <<  path<< "\n";
}

std::string write_tree() {
  std::ifstream index(PATH / ".dt/index");
  std::stringstream buffer;

  buffer << index.rdbuf();

  std::string content = buffer.str();

  std::string tree_hash = hash_content(content);
  write_object(tree_hash, content);
  return tree_hash;
}

std::string get_head_commit(const fs::path& path) {
  std::ifstream head( PATH / ".dt/HEAD");
  std::string ref;
  std::getline(head, ref);

  std::ifstream branch( PATH / ".dt/refs/" / ref);
  std::string commit;
  std::getline(branch, commit);
  return commit;
}

void commit(const std::string& message, const fs::path& path) {
  std::string tree = write_tree();
  std::string parent;
  if (fs::exists(PATH / ".dt/HEAD")) parent = get_head_commit(path);

  std::ostringstream commit_data;
  commit_data << "tree " << tree << "\n";
  if (! parent.empty()) commit_data << "parent " << parent << "\n";
  commit_data << "message " << message << "\n";

  std::string commit_harsh = hash_content(commit_data.str());
  write_object(commit_harsh, commit_data.str());

  std::ofstream head(PATH / ".dt/refs/main");
  head << commit_harsh;

  std::ofstream head_ref(PATH / ".dt/HEAD");
  head_ref << "main";

  fs::remove(PATH / ".dt/index");
}

void init_repo() {
  fs::create_directories(PATH / ".dt/objects");
  fs::create_directories(PATH / ".dt/refs");
  std::ofstream(PATH / "/.dt/HEAD") << "main";
  std::ofstream(PATH / "/.dt/index");
}

int main(int argc, char *argv[])
{
 
  for (int i = 0; i < argc; i++) {

    if (auto match_option = match(argv[i], std::regex("--add=([^\\s]*)"))) {
      PATH = match_option.value().at(1);
      init_repo();
    }

    else if (auto match_option = match(argv[i], std::regex("--stage=([^\\s]*)"))) {
      const fs::path& path = match_option.value().at(1);
      stage_file(path);
    }
  }
  
  return 0;
}
