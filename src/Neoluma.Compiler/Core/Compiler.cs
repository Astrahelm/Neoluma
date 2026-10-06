using Neoluma.Core.Extras;
using Neoluma.Core.Frontend;
using Neoluma.Libraries;

namespace Neoluma.Core;

public enum OutputType { Executable, StaticLibrary, SharedLibrary, Object, IR, LLVM_IR, None };

/// Compiler settings for the project tell the compiler what to set up before building
public class CompilerSettings {
    ///<c>Verbose</c> is an option that allows extra keywords, syntactic sugar. Not recommended for people who loves keeping shape of a language. Recommended for people who don't give a heck and love copypasting answers from StackOverflow.
    ///<br/> <c>true</c> - enables this option
    ///<br/> <c>false</c> - disables this option
    public bool verbose = false; 
        
    ///<c>Baremetal</c> is an option that toggles the ability to develop software for bare metal hardware. It disables platform-based ABI (including std) and compiles into binary.
    ///<br/> <c>true</c> - enables this option
    ///<br/> <c>false</c> - disables this option
    public bool baremetal = false;
    
    public class Memory {
        public enum MemoryOptions { None, Rusty, ARC, Default };
        
        ///<c>level</c> is an option of <c>Memory</c> class that goes through types of Memory options and, depending on chosen option, will manage the memory in the application the preferred way.
        ///<br/> <c>Default</c> - enables default Garbage Collector (Java, C#, others)
        ///<br/> <c>ARC</c> - enables Automatic Reference Counter (Python, JS, others)
        ///<br/> <c>Rusty</c> - enables Borrow Checker (Rust)
        ///<br/> <c>None</c> - No memory management tools (C, C++, maybe others)
        public MemoryOptions level = MemoryOptions.Default;
    }
}

public struct DependencyInput(string rootPath, string sourceFolder = "src") {
    public string rootPath = rootPath;
    public string sourceFolder = sourceFolder;
}

public struct CompilationInput(OutputType targetOutput, List<string> files, Dictionary<string, DependencyInput> dependencies, CompilerSettings settings) {
    public OutputType targetOutput = targetOutput;
    public List<string> files = files;
    public Dictionary<string, DependencyInput> dependencies = dependencies;
    public CompilerSettings settings = settings;
}

// Program is a class that stores results of compilation here for easy access to all information
public struct ProgramInterface {
    public CompilationInput input; // Compilation data for the compiler

    // Parser result
    public List<ModuleNode> modules; // all files of the project

    // Orchestrator result
    public List<ModuleInfo> moduleInfos;
    public EntryPoint entryPoint;
    public List<int> order;

    // global namespaces for the entire program
    public Dictionary<string, NamespaceInfo> namespaces;
}

// Compiler is a general class that allows Neoluma compiler to turn source code into machine code.
public sealed class Compiler {
    // Constructor
    public Compiler(CompilationInput input) {
        program.input = input;

        lexer.errorManager = errorManager;
        parser.errorManager = errorManager;
        orchestrator.setCompiler(this); // it requires for internal project checks
        //semanticAnalysis.errorManager = &errorManager;
    }
    
    // Functions
    void compile() {  // compiled way
    }
    void check(bool jsonOutput = false) {
        // TODO: Tolerate sourceFolder choice
        // Parsing dependencies before getting started
        List<string> files = program.input.files;

        foreach (var (name, dependency) in program.input.dependencies) {
            string sourcePath = Path.Combine(dependency.rootPath, dependency.sourceFolder);
            foreach (string file in Directory.EnumerateFiles(sourcePath, "*", new EnumerationOptions{RecurseSubdirectories = true, IgnoreInaccessible = true})) {
                if (Path.GetExtension(file) == ".nm") files.Add(file);
            }
        }

        // Parsing the project itself
        foreach (var file in files){
            // Lexer: breaks code down into tokens.
            string source = File.ReadAllText(file);
            List<Token> tokens = lexer.tokenize(file);
            lexer.printTokens();

            // Parser: builds a module tree out of tokens
            parser.parseModule(tokens, file);
            parser.printModule();
            ModuleNode tree = parser.moduleSource;

            // Adding modules to program's tree
            if (tree == null) Console.WriteLine($"[Neoluma/check] An empty tree was found for: {file}");
            if (tree != null) program.modules.Add(tree);
        }

        // Orchestrator: stitches files together into a full program, used for Semantic Analysis and more.
        program.namespaces = orchestrator.collectNamespaces(program.modules);
        program.entryPoint = orchestrator.findEntryPoint(program.modules);
        program.moduleInfos = orchestrator.resolveImports(program);
        Console.WriteLine("=== ModuleId map ===");
        foreach (var info in program.moduleInfos){
            Console.WriteLine($"[{info.id}] file={info.module?.sourceSpan.filePath ?? "<null>"}");
            Console.WriteLine($"     deps={info.dependencies.Count}");
            foreach (var d in info.dependencies) Console.WriteLine($"        -> {d.moduleId}");
        }
        orchestrator.stitchProgram(program);
        Console.WriteLine($"Entry module id: {program.moduleInfos.First(info => info.module == program.entryPoint.module).id}"); 
        Console.WriteLine("Order:");
        foreach(var id in program.order) {
            Console.WriteLine($"    {id}");
        }

        // Semantic Analysis: Make sure the program runs logically correct, before turned into a machine code
        //semanticAnalysis.analyzeProgram(program);

        if (errorManager.hasErrors()) {
            if (jsonOutput) Console.WriteLine($"{JSON.stringify(errorManager.toJson(), new JSON.StringifyOptions{pretty=true, emitComments=false})}");
            else {
                errorManager.printErrors();
                Console.WriteLine($"{Color.TextHex("#ff5050")}{Localization.translate("CLI.check.failed")}{Color.Reset}");
            }
        } 
        else if (jsonOutput) Console.WriteLine($"{JSON.stringify(errorManager.toJson(), new JSON.StringifyOptions{pretty=true, emitComments=false})}");
        else Console.WriteLine($"{Color.TextHex("#75ff87")}{Localization.translate("CLI.check.complete")}{Color.Reset}");
    }
    void run() { // interpreted way
    }
    
    // All parts of compiler
    Lexer lexer = new();
    Parser parser = new();
    Orchestrator orchestrator = new();
    //SemanticAnalysis semanticAnalysis;
    
    public ErrorManager errorManager = new();
    
    // Data
    public ProgramInterface program;
}