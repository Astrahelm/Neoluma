#include "CLI.hpp"
#include "Core/Compiler.hpp"

#include "Libraries/Localization/Localization.hpp"
#include "Libraries/Color/Color.hpp"
#include "Libraries/Paths/Paths.hpp"

#include <fstream>

// ==== Main functions ====

void build(const std::string& nlpFile) {
    ProjectConfig config = parseProjectFile(nlpFile);
    std::println("{} {}", Localization::translate("CLI.build.initialization"), config.name);
    // todo: compiler call, executable generation
}

void run(const std::string& nlpFile) {
    build(nlpFile);
    std::println("{}\n", Localization::translate("CLI.run.initialization"));
    // todo: launch the file using std::system or CreateProcess
}

void check(const std::string& nlpFile, bool jsonOutput) {
    ProjectConfig config = parseProjectFile(nlpFile);
    CompilationInput input;

    input.targetOutput = config.output;
    input.settings = parseCompilerSettings(config.compilerSettings);
    for (const auto& file : std:: filesystem::recursive_directory_iterator(std::filesystem::path(config.sourcePath) / config.sourceFolder, std::filesystem::directory_options::skip_permission_denied)) {
        if (file.is_regular_file() && file.path().extension() == ".nm") input.files.push_back(file.path());
    }

    DependencyInput stdDependency = {
        .rootPath = std::filesystem::path(Paths::dataDir()) / "modules" / "std",
        .sourceFolder = "src"
    };
    input.dependencies["std"] = stdDependency; // todo: doesn't support external for now

    Compiler compiler = Compiler(input);
    if (!jsonOutput) std::println("{}{}{}", Color::TextHex("#75ff87"), formatStr(Localization::translate("CLI.check.initialization"), config.name), Color::Reset);
    compiler.check(jsonOutput);
}

void createProject() {
    ProjectConfig config;
    int steps = 5; int step = 0;
    std::string title = formatStr("{} ", Localization::translate("CLI.createProject.initialization"));

    clearScreen();
    showProgressBar(title, step++, steps);

    config.name = asker::input(Localization::translate("CLI.createProject.projectName"), true);
    clearScreen();
    showProgressBar(title, step++, steps);
    config.version = asker::input(Localization::translate("CLI.createProject.projectVersion"));
    clearScreen();
    showProgressBar(title, step++, steps);
    std::vector<std::string> authors = split(asker::input(Localization::translate("CLI.createProject.projectAuthors"), true), ',');
    std::string authorList = listAuthors(authors);
    config.author = authors;
    clearScreen();
    showProgressBar(title, step++, steps);
    std::string licenses[14] = { "MIT", "Apache 2.0", "GNU GPL v3", "BSD 2-Clause \"Simplified\"", "BSD 3-Clause \"New\" or \"Revised\"", "Boost Software 1.0", "CC0 v1 Universal", "Eclipse", "GNU AGPL v3", "GNU GPL v2", "GNU LGPL v2.1", "Mozilla 2.0", "The Unlicense", "Custom"};
    std::string license = asker::selectList(Localization::translate("CLI.createProject.projectLicense"), licenses);

    if (license == "MIT") config.license = License::MIT;
    else if (license == "Apache 2.0") config.license = License::Apache;
    else if (license == "GNU GPL v3") config.license = License::GPL3;
    else if (license == "BSD 2-Clause \"Simplified\"") config.license = License::BSD2;
    else if (license == "BSD 3-Clause \"New\" or \"Revised\"") config.license = License::BSD3;
    else if (license == "Boost Software 1.0") config.license = License::Boost;
    else if (license == "CC0 v1 Universal") config.license = License::CC0;
    else if (license == "Eclipse") config.license = License::Eclipse;
    else if (license == "GNU AGPL v3") config.license = License::AGPL;
    else if (license == "GNU GPL v2") config.license = License::GPL2;
    else if (license == "GNU LGPL v2.1") config.license = License::LGPL;
    else if (license == "Mozilla 2.0") config.license = License::Mozilla;
    else if (license == "The Unlicense") config.license = License::Unlicense;
    else config.license = License::Custom;
    
    clearScreen();
    showProgressBar(title, step++, steps);
    bool confirmation = asker::confirm(formatStr("{}{}{}", Color::TextHex("#FF8C75"),
        formatStr(Localization::translate("CLI.createProject.confirmation"), config.name,
        config.version, authorList, license, Color::TextHex("#96fcbd")),
        Color::Reset));
    if (confirmation) {
        createProject(config);
        clearScreen();
        std::println(std::cout, "{}", formatStr(Localization::translate("CLI.createProject.confirmation.yes"), Color::TextHex("#75ff87"), Color::Reset));
    } else {
        std::println(std::cout, "{}", formatStr(Localization::translate("CLI.createProject.confirmation.no"), Color::TextHex("#ff5050"), Color::Reset));
    }
    std::println(Color::Reset);
}

void createProject(ProjectConfig config) {
    std::filesystem::path projectPath = std::filesystem::current_path() / config.name;
    std::filesystem::create_directory(projectPath);
    std::filesystem::create_directory(projectPath / "src");

    std::ofstream mainFile(projectPath / "src/main.nm");
    mainFile << std::format("// {} \n@entry\nfn main() {{\n    print(\"{}\");\n}}", Localization::translate("CLI.createProject.template.main.comment"), Localization::translate("CLI.createProject.template.main.printmsg"));
    mainFile.close();

    Toml::Table table;
    Toml::Table project;
    project["name"] = config.name;
    project["version"] = config.version;

    Toml::TomlArray authors_array;
    for (const auto& author : config.author) authors_array.push_back(Toml::TomlValue(author));
    project["authors"] = Toml::TomlValue(authors_array);
    project["license"] = licenseID(config.license);
    project["output"] = outputID(config.output);
    project["sourceFolder"] = config.sourceFolder;
    project["buildFolder"] = config.buildFolder;
    table["project"] = project.get();

    Toml::Table tasks;
    tasks["dev"] = "neoluma run --debug";
    table["tasks"] = tasks.get();

    std::ofstream cfg(projectPath / std::format("{}.nlp", formatNameToSnakeCase(config.name)));
    if (cfg.is_open()){
        Toml::serializeTable(cfg, table.get());
    }
    cfg.close();

    std::ofstream license(projectPath / "LICENSE");
    if (license.is_open()) {
        Licenses licenses;
        license << licenses.checkLicense(config, config.license);
    }
    license.close();
}

void printHelp() {
    std::println(std::cout, "{}", Localization::translate("CLI.helpMessage"));
}

