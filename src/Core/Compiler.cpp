#include "Compiler.hpp"

#include "Libraries/Asker/Asker.hpp"
#include "Libraries/Color/Color.hpp"
#include "Libraries/Json/Json.hpp"
#include "Libraries/Localization/Localization.hpp"

Compiler::Compiler(const CompilationInput& input) {
    program.input = input;

    lexer.errorManager = &errorManager;
    parser.errorManager = &errorManager;
    orchestrator.setCompiler(this); // it requires for internal project checks
    semanticAnalysis.errorManager = &errorManager;
}

void Compiler::check(bool jsonOutput) {
    // TODO: Tolerate sourceFolder choice
    // Parsing dependencies before getting started
    std::vector<std::filesystem::path> files = program.input.files;

    for (const auto& [name, dependency] : program.input.dependencies) {
        std::filesystem::path sourcePath = dependency.rootPath / dependency.sourceFolder;
        for (const auto& file : std::filesystem::recursive_directory_iterator( sourcePath, std::filesystem::directory_options::skip_permission_denied)) {
            if (file.is_regular_file() && file.path().extension() == ".nm") files.push_back(file.path());
        }
    }

    // Parsing the project itself
    for (const auto& file : files){
        // Lexer: breaks code down into tokens.
        std::string source = readFile(file.string());
        std::vector<Token> tokens = lexer.tokenize(file.string());
        //lexer.printTokens();

        // Parser: builds a module tree out of tokens
        parser.parseModule(tokens, file.string());
        //parser.printModule();
        MemoryPtr<ModuleNode> tree = std::move(parser.moduleSource);

        // Adding modules to program's tree
        if (!tree) std::println(std::cerr, "NULL TREE: {}", file.string());
        if (tree) program.modules.push_back(std::move(tree));
    }

    // Orchestrator: stitches files together into a full program, used for Semantic Analysis and more.
    program.namespaces = orchestrator.collectNamespaces(program.modules);
    program.entryPoint = orchestrator.findEntryPoint(program.modules);
    program.moduleInfos = orchestrator.resolveImports(program);
    // /*std::println(std::cout, "=== ModuleId map ===");
    // for (const auto& info : infos){
    //     std::println(std::cout, "[{}] file={}", info.id, info.module ? info.module->filePath : "<null>");
    //     std::println(std::cout, "     deps={}", info.dependencies.size());
    //     for (auto d : info.dependencies) std::println(std::cout, "        -> {}", d.moduleId);
    // }*/
    orchestrator.stitchProgram(program);
    /*std::println(std::cout, "Entry module id: {}", program.entryModule);
    std::println(std::cout, "Order:");
    for (auto id : program.order) {
        std::println(std::cout, "    {}", id);
    }*/

    // Semantic Analysis: Make sure the program runs logically correct, before turned into a machine code
    semanticAnalysis.analyzeProgram(program);

    if (errorManager.hasErrors()) {
        if (jsonOutput) {
            std::println(std::cout, "{}", json::stringify(errorManager.toJson(), {.pretty = true, .emit_comments = false}));
        } else {
            errorManager.printErrors();
            std::println(std::cout, "{}{}{}", Color::TextHex("#ff5050"), Localization::translate("CLI.check.failed"), Color::Reset);
        }
    } else if (jsonOutput) {
        std::println(std::cout, "{}", json::stringify(errorManager.toJson(), {.pretty = true, .emit_comments = false}));
    } else {
        std::println(std::cout, "{}{}{}", Color::TextHex("#75ff87"), Localization::translate("CLI.check.complete"), Color::Reset);
    }
}
