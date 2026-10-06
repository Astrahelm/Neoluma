using Neoluma.Core.Extras;

namespace Neoluma.Core.Frontend;

// helper declarations
using ModuleId = int;

public struct EntryPoint {
    public ModuleNode? module;
    public FunctionNode? function;
}

public readonly struct DependencyEdge(ModuleId moduleId, ErrorSpan span) {
    public readonly ModuleId moduleId = moduleId;
    public readonly ErrorSpan span = span; // where did the error happen
}

// main data declarations

public sealed class ModuleInfo {
    public ModuleId id;
    public ModuleNode? module;
    public List<DependencyEdge> dependencies = [];
    public Dictionary<string, ModuleId> aliasMap = [];
    public List<string> nativeImports = []; // for packages that import from dependencies or std, like "math", "std", "tazer"

    public List<string> namespaceImports = [];
    public Dictionary<string, string> namespaceAliasMap = [];
}

public sealed class NamespaceInfo(string name) {
    public string name = name;
    public List<NamespaceNode> declarations = [];
}

// Orchestrator is a class that allows us to stitch the project together into a working program, that can be fed to the Semantic Analysis and lower parts of the Compiler.
public sealed class Orchestrator {
    public Compiler compiler = null!;
    public void setCompiler(Compiler comp) { compiler = comp; }

    // ==== main function ====
    public void stitchProgram(ProgramInterface program) {
        ModuleId entryId = -1;

        foreach (var info in program.moduleInfos) {
            if (info.module == program.entryPoint.module) {
                entryId = info.id;
                break;
            }
        }

        if (entryId < 0) return; // i mean this should not ever ever happen but...

        program.order = new(); // clean up previous order just in case

        byte[] state = new byte[program.moduleInfos.Count];
        dfsVisit(entryId, program.moduleInfos, state, program.order, null);
    }
    public EntryPoint findEntryPoint(List<ModuleNode> modules) {
        EntryPoint entryPoint = new();
        EntryPoint mainFallback = new();
        ModuleNode? firstModule = null;

        bool foundExplicit = false;

        foreach (var module in modules) {
            if (module == null) continue;
            if (firstModule == null) firstModule = module;
            foreach (var statement in module.body) {
                if (statement == null || statement is not FunctionNode func) continue;

                if (mainFallback.function == null && func.name == "main") {
                    mainFallback.module = module;
                    mainFallback.function = func;
                }

                if (hasEntryDecorator(func)) {
                    if (!foundExplicit) {
                        foundExplicit = true;
                        entryPoint.module = module;
                        entryPoint.function = func;
                    } else {
                        var firstFn = entryPoint.function!;
                        compiler.errorManager.addError(AnalysisErrors.MultipleEntryPoints,
                            new ErrorSpan(func.name, func.sourceSpan),
                        "ErrorManager.Analysis.MultipleEntryPoints.message", [],
                        "ErrorManager.Analysis.MultipleEntryPoints.hint", [firstFn.sourceSpan.filePath, $"{firstFn.sourceSpan.line}", $"{firstFn.sourceSpan.column}"]);
                    }
                }
            }
        }
        if (foundExplicit) return entryPoint;

        if (mainFallback.function != null) return mainFallback;

        compiler.errorManager.addError(AnalysisErrors.NoEntryPoints,
            new ErrorSpan(firstModule != null ? firstModule.sourceSpan.filePath : "", "", 1, 1),
        "ErrorManager.Analysis.NoEntryPoints.message",  [],
        "ErrorManager.Analysis.NoEntryPoints.hint");

        return new();
    }
    public List<ModuleInfo> resolveImports(ProgramInterface program) {
        var modules = program.modules;
        List<ModuleInfo> infos = new(modules.Count);
        for (int i = 0; i < modules.Count; ++i) infos.Add(new ModuleInfo());

        // key to module id
        Dictionary<string, ModuleId> keyToId = new();
        keyToId.EnsureCapacity(modules.Count * 2);
        string[] idToKey = new string[modules.Count];

        for (ModuleId i = 0; i < modules.Count; ++i) {
            ModuleNode m = modules[i];
            if (m == null) continue;

            string key = normalizeKey(m.sourceSpan.filePath);
            idToKey[i] = key;
            keyToId[key] = i;

            infos[i].id = i;
            infos[i].module = m;
        }

        // go through imports and fill out dependencies and aliasMap
        for (ModuleId i = 0; i < infos.Count; ++i)
        {
            ModuleInfo mi = infos[i];
            ModuleNode? m = mi.module;
            if (m == null) continue;

            foreach (var st in m.body) {
                if (st == null || st is not ImportNode imp) continue;
                
                string name = imp.moduleName;

                if (program.namespaces.ContainsKey(name)) {
                    mi.namespaceImports.Add(name);
                    if (imp.alias.Length != 0) mi.namespaceAliasMap.TryAdd(imp.alias, name);
                    continue;
                }

                if (imp.importType == ASTImportType.Relative){
                    string resolvedKey = resolveRelativeKey(idToKey[mi.id], imp.moduleName);
                    if (!keyToId.TryGetValue(resolvedKey, out var resolvedValue)){
                        compiler.errorManager.addError(PreprocessorErrors.ImportNotFound,
                            new ErrorSpan(imp.moduleName, imp.sourceSpan),
                            "ErrorManager.Preprocessor.ImportNotFound.message", [name],
                            "ErrorManager.Preprocessor.ImportNotFound.hint");
                        continue;
                    }

                    ModuleId depId = resolvedValue;
                    mi.dependencies.Add(new DependencyEdge(depId, new ErrorSpan(imp.moduleName, imp.sourceSpan)));
                    registerAlias(mi, imp, depId);
                }
                else if (imp.importType == ASTImportType.Native){
                    // At first we're gonna assume the file is in the same folder, if not, it's really a native import
                    // As of now i have no idea how to detect Native modules properly, but if i find out a better solution,
                    // i should check the parsePreprocessor() in Parser and fix the detection.
                    string resolvedKey = resolveRelativeKey(idToKey[mi.id], imp.moduleName);
                    
                    if (keyToId.TryGetValue(resolvedKey, out var resolvedValue)){
                        // is a relative import
                        ModuleId depId = resolvedValue;
                        mi.dependencies.Add(new DependencyEdge(depId, new ErrorSpan(imp.moduleName, imp.sourceSpan)));
                        registerAlias(mi, imp, depId);
                    } else {
                        // is a native import
                        if (imp.moduleName != "std" && !compiler.program.input.dependencies.ContainsKey(imp.moduleName)){
                            compiler.errorManager.addError(PreprocessorErrors.ImportNotFound,
                            new ErrorSpan(imp.moduleName, imp.sourceSpan),
                            "ErrorManager.Preprocessor.ImportNotFound.nativePackageNotInstalled.message", [imp.moduleName],
                            "ErrorManager.Preprocessor.ImportNotFound.nativePackageNotInstalled.hint");
                        }
                        else mi.nativeImports.Add(imp.moduleName);
                    }

                    // Foreign and Foreign Relative will come out with language packs update.
                }
            }
        }

        return infos;
    }
    public Dictionary<string, NamespaceInfo> collectNamespaces(List<ModuleNode> modules) {
        Dictionary<string, NamespaceInfo> namespaces = new();

        foreach (var module in modules) {
            if (module == null) continue;

            foreach (var statement in module.body) {
                if (statement == null || statement is not NamespaceNode node) continue;
                string name = node.namespaceName;
                if (!namespaces.ContainsKey(name)) namespaces.Add(name, new NamespaceInfo(name));

                namespaces[name].declarations.Add(node);
            }
        }

        return namespaces;
    }

    // ==== helper functions ====
    static bool hasEntryDecorator(FunctionNode? function) {
        if (function == null) return false;
        foreach (var decorator in function.decorators ?? []){
            if (decorator == null || decorator.callee == null) return false;
            if (decorator.callee is not VariableNode variable) return false;
            if (variable.varName == "entry") return true;
        }
        return false;
    }
    // https://www.geeksforgeeks.org/dsa/depth-first-search-or-dfs-for-a-graph/
    void dfsVisit(ModuleId id, List<ModuleInfo> infos, byte[] state, List<ModuleId> order, ErrorSpan? fromSpan) {
        if (state[id] == 2) return; //done
        if (state[id] == 1) {
            if (fromSpan != null) {
                compiler.errorManager.addError(PreprocessorErrors.CircularImport, fromSpan.Value,
                    "ErrorManager.Preprocessor.CircularImport.message", [],
                "ErrorManager.Preprocessor.CircularImport.hint");
            } else if (compiler != null){
                var m = infos[id].module;
                compiler.errorManager.addError(PreprocessorErrors.CircularImport,
                   new ErrorSpan("import", m!.sourceSpan),
                "ErrorManager.Preprocessor.CircularImport.message", [],
                "ErrorManager.Preprocessor.CircularImport.hint");
            }
            return;
        }

        state[id] = 1; // visiting

        foreach (var edge in infos[id].dependencies)
            dfsVisit(edge.moduleId, infos, state, order, edge.span);

        state[id] = 2; // done
        order.Add(id);
    }
    static string dirOfKey(string key) {
        string norm = key.Replace('\\', '/');
        int pos = norm.LastIndexOf('/');
        return pos < 0 ? "" : norm[..pos];
    }
    static string resolveRelativeKey(string currentKey, string importName) {
        List<string> stack = dirOfKey(currentKey).Split('/', StringSplitOptions.RemoveEmptyEntries).ToList();

        foreach (string part in importName.Replace('\\', '/').Split('/', StringSplitOptions.RemoveEmptyEntries)) {
            if (part == ".") continue;

            if (part == "..") {
                if (stack.Count > 0) stack.RemoveAt(stack.Count - 1);
            } else stack.Add(part);
        }

        return string.Join('/', stack);
    }
    static string normalizeKey(string path) {
        List<string> parts = [];

        foreach (string part in Path.ChangeExtension(path, null).Replace('\\', '/').Split('/', StringSplitOptions.RemoveEmptyEntries)) {
            if (part == ".") continue;

            if (part == "..") {
                if (parts.Count > 0 && parts[^1] != "..") parts.RemoveAt(parts.Count - 1);
                else parts.Add("..");
            } else parts.Add(part);
        }

        return string.Join('/', parts);
    }
    void registerAlias(ModuleInfo mi, ImportNode imp, ModuleId depId) {
        if (imp.alias.Length == 0) return;

        if (mi.aliasMap.ContainsKey(imp.alias)) {
            compiler.errorManager.addError(
                PreprocessorErrors.ImportAliasConflict,
                new ErrorSpan(imp.alias, imp.sourceSpan),
                "ErrorManager.Preprocessor.ImportAliasConflict.message", [imp.alias],
                "ErrorManager.Preprocessor.ImportAliasConflict.hint");
            return;
        }

        mi.aliasMap.Add(imp.alias, depId);
    }
}