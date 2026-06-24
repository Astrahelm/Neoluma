#include <string>
#include <iostream>
#include <print>
#include <filesystem>

#include "CLI/CLI.hpp"
#include "Libraries/Color/Color.hpp"
#include "Core/Frontend/Lexer/Lexer.hpp"
#include "HelperFunctions.hpp"
#include "Libraries/Localization/Localization.hpp"
#include "Libraries/Paths/Paths.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

std::filesystem::path findProjectFile(const std::filesystem::path& folder) {
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".nlp") {
            return entry.path();
        }
    }
    return {};
}

int main(int argc, char** argv) {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif
    Localization::init();
    CLIArgs args = parseArgs(argc, argv);

    if (args.command == "new") {
        if (!args.options.empty()) {
            ProjectConfig config;
            config.name = args.options.count("name") ? args.options.at("name") : config.name;
            if (args.options.count("author"))
                config.author = split(args.options.at("author"), ',');
            config.version = args.options.count("version") ? args.options.at("version") : config.version;
            if (args.options.count("license"))
                config.license = parseLicense(args.options.at("license"));
            createProject(config);
        } // If no arguments provided, aka "neoluma new", it will run an integrated assistant to set up a project.
        else createProject();

    } else if (args.command == "build") {
        // build
    } else if (args.command == "run") {
        // run
    } else if (args.command == "check") {
        std::filesystem::path projectFilePath;

        if (args.options.count("project")) {
            std::filesystem::path input = args.options.at("project");

            if (std::filesystem::exists(input)) {
                if (std::filesystem::is_directory(input)) projectFilePath = findProjectFile(input);
                else projectFilePath = input;
            }
        }
        else if (args.positional.size() > 0) {
            std::println("Test");
        }
        else {
            projectFilePath = findProjectFile(std::filesystem::current_path());

            if (projectFilePath.empty()) {
                std::println("{}[Neoluma/Check] Project file was not found!{}", Color::TextHex("#ff5050"), Color::Reset);
                return 2;
            }
        }

        check(projectFilePath.string(), args.options.count("json"));
    } else if (args.command == "version") std::println("{}Neoluma Alpha Release v0.1{}", Color::TextHex("#ff28e6"), Color::Reset);
    else printHelp();

    return 0;
}
