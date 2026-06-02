#pragma once
#include <map>
#include <filesystem>

#include "Frontend/Lexer/Lexer.hpp"
#include "Frontend/Parser/Parser.hpp"
#include "Extras/ErrorManager/ErrorManager.hpp"
#include "Frontend/SemanticAnalysis/SemanticAnalysis.hpp"
#include "Frontend/Orchestrator/Orchestrator.hpp"

enum class OutputType { Executable, StaticLibrary, SharedLibrary, Object, IR, LLVM_IR, None };

// Compiler settings for the project tell the compiler what to set up before building
struct CompilerSettings {
    /**
     * @brief `Verbose` is an option that allows extra keywords, syntactic sugar. Not recommended for people who loves keeping shape of a language. Recommended for people who don't give a heck and love copypasting answers from StackOverflow.
     * @param true - enables this option
     * @param false - disables this option
    */
    bool verbose = false;
    /**
     * @brief `Baremetal` is an option that toggles the ability to develop software for bare metal hardware. It disables platform-based ABI (including std) and compiles into binary.
     * @param true - enables this option
     * @param false - disables this option
     */
    bool baremetal = false;

    struct Memory {
        enum class MemoryOptions { None, Rusty, ARC, Default };

        /**
         * @brief `level` is an option of `Memory` struct that goes through types of Memory options and, depending on chosen option, will manage the memory in the application the preferred way.
         * @param Default - enables default Garbage Collector (Java, C#, others)
         * @param ARC - enables Automatic Reference Counter (Python, JS, others)
         * @param Rusty - enables Borrow Checker (Rust)
         * @param None - No memory management tools (C, C++, maybe others)
         */
        MemoryOptions level = MemoryOptions::Default;
    };
};

struct DependencyInput {
    std::filesystem::path rootPath;
    std::filesystem::path sourceFolder = "src";
};

struct CompilationInput {
    OutputType targetOutput;
    std::vector<std::filesystem::path> files;
    std::map<std::string, DependencyInput> dependencies;
    CompilerSettings settings;
};

// Program is a class that stores results of compilation here for easy access to all information
struct Program {
    CompilationInput input; // Compilation data for the compiler

    // Parser result
    std::vector<MemoryPtr<ModuleNode>> modules; // all files of the project

    // Orchestrator result
    std::vector<ModuleInfo> moduleInfos;
    EntryPoint entryPoint;
    std::vector<ModuleId> order;

    // global namespaces for the entire program
    std::unordered_map<std::string, NamespaceInfo> namespaces;
};

/**
 * @brief Compiler is a general class that allows Neoluma compiler to turn source code into machine code.
 */
class Compiler {
public:
    // Constructor
    Compiler(const CompilationInput& input);

    // Functions
    void compile(); // compiled way
    void check(bool jsonOutput = false);
    void run(); // interpreted way

    ErrorManager errorManager;

    // Data
    Program program;
private:
    // All parts of compiler
    Lexer lexer;
    Parser parser;
    Orchestrator orchestrator;
    SemanticAnalysis semanticAnalysis;
};