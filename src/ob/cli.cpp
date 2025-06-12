#include "cli.h"


// Method to add a new command
CLI& CLI::add_command(Command&& cmd) {
    auto [it, success] = commands_.emplace(cmd.name, std::move(cmd));
    if (success) {
        for (const auto& alias : it->second.aliases) {
            aliases_[alias] = it->second.name;
        }
    }
    return *this;
}

// Run the CLI with given arguments
int CLI::run(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    std::string command_name = argv[1];
    if (command_name == "help" || command_name == "--help" || command_name == "-h") {
        if (argc > 2) {
            auto it = commands_.find(argv[2]);
            if (it != commands_.end()) {
                print_command_help(it->second);
            } else {
                auto alias_it = aliases_.find(argv[2]);
                if (alias_it != aliases_.end()) {
                    it = commands_.find(alias_it->second);
                    if (it != commands_.end()) {
                        print_command_help(it->second);
                    }
                } else {
                    std::cerr << "Error: Unknown command '" << argv[2] << "'" << std::endl;
                    return 1;
                }
            }
        } else {
            print_help();
        }
        return 0;
    }

    auto it = commands_.find(command_name);
    if (it == commands_.end()) {
        auto alias_it = aliases_.find(command_name);
        if (alias_it != aliases_.end()) {
            it = commands_.find(alias_it->second);
        }
    }

    if (it != commands_.end()) {
        try {
            it->second.action(argc - 2, argv + 2);
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << "\n\n";
            print_command_help(it->second);
        }
    } else {
        std::cerr << "Error: Unknown command '" << command_name << "'" << std::endl;
        print_help();
        return 1;
    }

    return 0;
}

// Print the main help message
void CLI::print_help() const {
    std::cout << app_name_ << " - " << app_description_ << std::endl << std::endl;
    std::cout << "Usage: " << app_name_ << " <command> [options]" << std::endl << std::endl;
    std::cout << "Available commands:" << std::endl;

    size_t max_len = commands_.size();

    for (const auto& [name, cmd] : commands_) {
        std::cout << "  " << std::left << std::setw(max_len + 2) << name << cmd.description << std::endl;
    }
}

// Helper to print help for a specific command
void CLI::print_command_help(const Command& cmd) const {
    std::cout << "Usage: " << app_name_ << " " << cmd.name;
    if (!cmd.usage.empty()) {
        std::cout << " " << cmd.usage;
    }
    std::cout << std::endl << std::endl;
    std::cout << cmd.description << std::endl;

    if (!cmd.aliases.empty()) {
        std::cout << std::endl << "Aliases: ";
        for (size_t i = 0; i < cmd.aliases.size(); ++i) {
            std::cout << cmd.aliases[i] << (i == cmd.aliases.size() - 1 ? "" : ", ");
        }
        std::cout << std::endl;
    }
}

