#pragma once
#include "../Nodes.hpp"
#include "Core/Extras/ErrorManager/ErrorManager.hpp"
#include <unordered_map>

// helper declarations
using ModuleId = int;
struct Compiler;
struct Program;

struct EntryPoint {
    ModuleNode* module = nullptr;
    FunctionNode* function = nullptr;
};

struct DependencyEdge {
    ModuleId moduleId;
    ErrorSpan span; // where did the error happen
};

// main data declarations

struct ModuleInfo {
    ModuleId id;
    ModuleNode* module;
    std::vector<DependencyEdge> dependencies;
    std::unordered_map<std::string, ModuleId> aliasMap;
    std::vector<std::string> nativeImports; // for packages that import from dependencies or std, like "math", "std", "tazer"

    std::vector<std::string> namespaceImports;
    std::unordered_map<std::string, std::string> namespaceAliasMap;
};

struct NamespaceInfo {
    std::string name;
    std::vector<NamespaceNode*> declarations;
};

// Orchestrator is a struct that allows us to stitch the project together into a working program, that can be fed to the Semantic Analysis and lower parts of the Compiler.
struct Orchestrator {
    Compiler* compiler = nullptr;
    void setCompiler(Compiler* comp) { compiler = comp; }

    // main function
    void stitchProgram(Program& program);
    EntryPoint findEntryPoint(const std::vector<MemoryPtr<ModuleNode>>& modules);
    std::vector<ModuleInfo> resolveImports(Program& program);
    std::unordered_map<std::string, NamespaceInfo> collectNamespaces(const std::vector<MemoryPtr<ModuleNode>>& modules);

    // helper functions
    static bool hasEntryDecorator(const FunctionNode* function);
    void dfsVisit(ModuleId id, const std::vector<ModuleInfo>& infos, std::vector<uint8_t>& state, std::vector<ModuleId>& order, const ErrorSpan* fromSpan);

    static std::vector<std::string> splitPath(const std::string& path);
    static std::string joinPath(const std::vector<std::string>& parts);
    static std::string dirOfKey(const std::string& key);
    static std::string resolveRelativeKey(const std::string& currentKey, const std::string& importName);
};