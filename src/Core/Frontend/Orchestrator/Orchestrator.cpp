#include "Orchestrator.hpp"
#include "Core/Compiler.hpp"

// Helper Functions

bool Orchestrator::hasEntryDecorator(const FunctionNode* function){
    if (!function) return false;
    for (const auto& decorator : function->decorators){
        if (!decorator || !decorator->callee) return false;
        if (decorator->callee->type != ASTNodeType::Variable) return false;
        auto* var = static_cast<VariableNode*>(decorator->callee.get());
        if (var->varName == "entry") return true;
    }
    return false;
}

// https://www.geeksforgeeks.org/dsa/depth-first-search-or-dfs-for-a-graph/
void Orchestrator::dfsVisit( ModuleId id, const std::vector<ModuleInfo>& infos, std::vector<uint8_t>& state,
                            std::vector<ModuleId>& order, const ErrorSpan* fromSpan)
{
    if (state[id] == 2) return; //done
    if (state[id] == 1) {
        if (fromSpan) {
            compiler->errorManager.addError(
                ErrorType::Preprocessor,
                PreprocessorErrors::CircularImport,
                *fromSpan,
                "ErrorManager.Preprocessor.CircularImport.message", {},
                "ErrorManager.Preprocessor.CircularImport.hint");
        } else if (compiler){
            auto* m = infos[id].module;
            compiler->errorManager.addError(
                ErrorType::Preprocessor,
                PreprocessorErrors::CircularImport,
                ErrorSpan{m->filePath, "import", m->line, m->column},
                "ErrorManager.Preprocessor.CircularImport.message", {},
                "ErrorManager.Preprocessor.CircularImport.hint");
        }
        return;
    }

    state[id] = 1; // visiting

    for (const auto& edge : infos[id].dependencies){
        dfsVisit(edge.moduleId, infos, state, order, &edge.span);
    }

    state[id] = 2; // done
    order.push_back(id);
}

std::vector<std::string> Orchestrator::splitPath(const std::string& path){
    std::vector<std::string> result;
    std::string cur;
    for (char c : path){
        char x = (c == '\\') ? '/' : c;
        if (x == '/') {
            if (!cur.empty()) result.push_back(cur); cur.clear();
        } else cur.push_back(x);
    }
    if (!cur.empty()) result.push_back(cur);
    return result;
}

std::string Orchestrator::joinPath(const std::vector<std::string>& parts){
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i){
        if (i) result += '/';
        result += parts[i];
    }
    return result;
}

std::string Orchestrator::dirOfKey(const std::string& key){
    auto norm = key;
    for (auto& ch : norm) if (ch == '\\') ch = '/';
    auto pos = norm.rfind('/');
    if (pos == std::string::npos) return ""; // root of sourceFolder
    return norm.substr(0, pos); // "more"
}

std::string Orchestrator::resolveRelativeKey(const std::string& currentKey, const std::string& importName){
    auto baseDir = dirOfKey(currentKey);
    std::vector<std::string> stack;

    if (!baseDir.empty()) stack = splitPath(baseDir);

    for (auto& part : splitPath(importName)){
        if (part == "." || part.empty()) continue;
        if (part == "..") {
            if (!stack.empty()) stack.pop_back(); // if empty, stay in the root (todo: make an error later for going out of sourceFolder)
        } else { stack.push_back(part); }
    }

    return joinPath(stack);
}

// Main implementations
void Orchestrator::stitchProgram(Program& program) {
    ModuleId entryId = -1;

    for (const auto& info : program.moduleInfos) {
        if (info.module == program.entryPoint.module) {
            entryId = info.id;
            break;
        }
    }

    if (entryId < 0) return; // i mean this should not ever ever happen but...

    program.order.clear(); // clean up previous order just in case

    std::vector<uint8_t> state(program.moduleInfos.size(), 0);
    dfsVisit(entryId, program.moduleInfos, state, program.order, nullptr);
}

EntryPoint Orchestrator::findEntryPoint(const std::vector<MemoryPtr<ModuleNode>>& modules) {
    EntryPoint entryPoint{};
    EntryPoint mainFallback{};
    ModuleNode* firstModule = nullptr;

    bool foundExplicit = false;

    for (const auto& module : modules) {
        if (!module) continue;
        if (!firstModule) firstModule = module.get();
        for (const auto& statement : module->body) {
            if (!statement || statement->type != ASTNodeType::Function) continue;

            auto* func = static_cast<FunctionNode*>(statement.get());

            if (!mainFallback.function && func->name == "main") {
                mainFallback.module = module.get();
                mainFallback.function = func;
            }

            if (hasEntryDecorator(func)) {
                if (!foundExplicit) {
                    foundExplicit = true;
                    entryPoint.module = module.get();
                    entryPoint.function = func;
                } else {
                    auto* firstFn = entryPoint.function;
                    compiler->errorManager.addError(
               ErrorType::Analysis,
               AnalysisErrors::MultipleEntryPoints,
               ErrorSpan{func->filePath, func->name, func->line, func->column},
               "ErrorManager.Analysis.MultipleEntryPoints.message", {},
               "ErrorManager.Analysis.MultipleEntryPoints.hint", {firstFn->filePath, anyToStr(firstFn->line), anyToStr(firstFn->column)});
                }
            }
        }
    }
    if (foundExplicit) return entryPoint;

    if (mainFallback.function) return mainFallback;

    compiler->errorManager.addError(
            ErrorType::Analysis,
            AnalysisErrors::NoEntryPoints,
            ErrorSpan{firstModule ? firstModule->filePath : "", "", 1, 1},
            "ErrorManager.Analysis.NoEntryPoints.message", {},
               "ErrorManager.Analysis.NoEntryPoints.hint");

    return {};
}

std::vector<ModuleInfo> Orchestrator::resolveImports(Program& program){
    const auto& modules = program.modules;
    std::vector<ModuleInfo> infos(modules.size());

    // key to module id
    std::unordered_map<std::string, ModuleId> keyToId;
    keyToId.reserve(modules.size() * 2);
    std::vector<std::string> idToKey(modules.size());

    for (ModuleId i = 0; i < (ModuleId)modules.size(); ++i) {
        ModuleNode* m = modules[i].get();
        if (!m) continue;

        std::string key = std::filesystem::path(m->filePath).replace_extension().lexically_normal().generic_string();
        idToKey[i] = key;
        keyToId[key] = i;

        infos[i].id = i;
        infos[i].module = m;
    }

    auto registerAlias = [&](ModuleInfo& mi, ImportNode* imp, ModuleId depId) {
        if (imp->alias.empty()) return;

        if (mi.aliasMap.count(imp->alias)) {
            compiler->errorManager.addError(
                ErrorType::Preprocessor,
                PreprocessorErrors::ImportAliasConflict,
                ErrorSpan{imp->filePath, imp->alias, imp->line, imp->column},
                "ErrorManager.Preprocessor.ImportAliasConflict.message", {imp->alias},
                "ErrorManager.Preprocessor.ImportAliasConflict.hint");
            return;
        }

        mi.aliasMap.emplace(imp->alias, depId);
    };

    // go through imports and fill out dependencies and aliasMap
    for (ModuleId i = 0; i < (ModuleId)infos.size(); ++i)
    {
        ModuleInfo& mi = infos[i];
        ModuleNode* m = mi.module;
        if (!m) continue;

        for (const auto& st : m->body)
        {
            if (!st || st->type != ASTNodeType::Import) continue;

            auto* imp = static_cast<ImportNode*>(st.get());
            const std::string& name = imp->moduleName;

            if (program.namespaces.contains(name)) {
                mi.namespaceImports.push_back(name);
                if (!imp->alias.empty()) mi.namespaceAliasMap.emplace(imp->alias, name);
                continue;
            }

            if (imp->importType == ASTImportType::Relative){
                std::string resolvedKey = resolveRelativeKey(idToKey[mi.id], imp->moduleName);
                auto it = keyToId.find(resolvedKey);
                if (it == keyToId.end()){
                    compiler->errorManager.addError(
                        ErrorType::Preprocessor,
                        PreprocessorErrors::ImportNotFound,
                        ErrorSpan{imp->filePath, imp->moduleName, imp->line, imp->column},
                        "ErrorManager.Preprocessor.ImportNotFound.message", {name},
                        "ErrorManager.Preprocessor.ImportNotFound.hint");
                    continue;
                }

                ModuleId depId = it->second;
                mi.dependencies.push_back(DependencyEdge{depId, ErrorSpan{imp->filePath, imp->moduleName, imp->line, imp->column}});
                registerAlias(mi, imp, depId);
            }
            else if (imp->importType == ASTImportType::Native){
                // At first we're gonna assume the file is in the same folder, if not, it's really a native import
                // As of now i have no idea how to detect Native modules properly, but if i find out a better solution,
                // i should check the parsePreprocessor() in Parser and fix the detection.
                std::string resolvedKey = resolveRelativeKey(idToKey[mi.id], imp->moduleName);

                auto it = keyToId.find(resolvedKey);
                if (it != keyToId.end()){
                    // is a relative import
                    ModuleId depId = it->second;
                    mi.dependencies.push_back(DependencyEdge{depId, ErrorSpan{imp->filePath, imp->moduleName, imp->line, imp->column}});
                    registerAlias(mi, imp, depId);
                } else {
                    // is a native import
                    if (imp->moduleName != "std" && !compiler->program.input.dependencies.contains(imp->moduleName)){
                        compiler->errorManager.addError(
                        ErrorType::Preprocessor,
                        PreprocessorErrors::ImportNotFound,
                        ErrorSpan{imp->filePath, imp->moduleName, imp->line, imp->column},
                        "ErrorManager.Preprocessor.ImportNotFound.nativePackageNotInstalled.message", {imp->moduleName},
                        "ErrorManager.Preprocessor.ImportNotFound.nativePackageNotInstalled.hint");
                    }
                    else mi.nativeImports.push_back(imp->moduleName);
                }

                // Foreign and Foreign Relative will come out with language packs update.
            }
        }
    }

    return infos;
}

std::unordered_map<std::string, NamespaceInfo> Orchestrator::collectNamespaces(const std::vector<MemoryPtr<ModuleNode>>& modules) {
    std::unordered_map<std::string, NamespaceInfo> namespaces;

    for (const auto& module : modules) {
        if (!module) continue;

        for (const auto& statement : module->body) {
            if (!statement || statement->type != ASTNodeType::Namespace) continue;

            auto* node = static_cast<NamespaceNode*>(statement.get());
            const std::string& name = node->value;

            if (!namespaces.contains(name)) namespaces.emplace(name, NamespaceInfo{name, {}});

            namespaces[name].declarations.push_back(node);
        }
    }

    return namespaces;
}