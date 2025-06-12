#include "ob/cli.h"
#include "dott/dott.h"
#include <filesystem>
#include <iostream>
#include <ostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    CLI dottracker("dott", "Dott — A minimal dotfile tracker with version controlling and managing. \
\nTrack, version, and manage dotfiles efficiently from the CLI.");

    dottracker.add_command({
        "init",
        "Create an empty Dott repository or reinitialize an existing one",
        "<directory>...",
        {},
        [](int argc, char** argv){
            if (! argc) {
                std::cerr << "No directory provided" << std::endl;
                throw std::invalid_argument("Missing arguments");
            }
            for (int i = 0; i < argc; i++)
            {
                if (fs::is_directory(argv[i]) || *argv[i] == '.')
                {
                    fs::path dir_path {argv[i]};
                    handle_init(dir_path);
                }
                else {
                    std::cout << "Found this argv from init " << argv[i] << std::endl;
                }
            }
        }
    });

    dottracker.add_command({
        "add",
        "Add file contents to the index",
        "<pathspec>...",
        {},
        [](int argc, char** argv){
            if (! argc) {
                std::cerr << "No pathspec provided" << std::endl;
                throw std::invalid_argument("Missing arguments");
            }
            for (int i = 0; i < argc; i++)
            {
                if (fs::is_regular_file(argv[i]))
                {
                    fs::path file_path {argv[i]};
                    if (! handle_stage(file_path)) {
                        throw std::runtime_error("Failed staging");
                    }
                }
                else {
                    std::cout << "Found this argv " << argv[i] << std::endl;
                    throw std::runtime_error("Found this argv from add " + std::string(argv[i]));
                }
            }
            std::cout << "successfully staged  file" << std::endl;
        }
    });

    dottracker.add_command({
        "status",
        "Shows staged files awaiting commit.",
        "<directory>...",
        {},
        [](int argc, char** argv){
            if (! argc) {
                std::cerr << "No directory provided" << std::endl;
                throw std::invalid_argument("Missing arguments");
            }
            for (int i = 0; i < argc; i++)
            {
                if (fs::exists(fs::path(argv[i]) / ".dott"))
                {
                    fs::path dir_path {argv[i]};
                    show_staged_files(dir_path);
                }
                else {
                    std::cout << "Found this argv from init " << argv[i] << std::endl;
                }
            }
        }

    });

    return dottracker.run(argc, argv);
}
