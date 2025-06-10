#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <iomanip>

// Represents a single command in the CLI
struct Command {
    std::string name;
    std::string description;
    std::string usage;
    std::vector<std::string> aliases;
    std::function<void(int, char**)> action;
};

class CLI {
private:
    std::string app_name_;
    std::string app_description_;
    std::unordered_map<std::string, Command> commands_;
    std::unordered_map<std::string, std::string> aliases_;

public:
    // Constructor
    CLI(std::string app_name, std::string app_description)
        : app_name_(std::move(app_name)), app_description_(std::move(app_description)) {}

    // Method to add a new command
    CLI& add_command(Command&& cmd);

    // Run the CLI with given arguments
    int run(int argc, char* argv[]);

    // Print the main help message
    void print_help() const;

private:
    // Helper to print help for a specific command
    void print_command_help(const Command& cmd) const;
};


