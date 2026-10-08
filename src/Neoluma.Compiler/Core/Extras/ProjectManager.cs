using Neoluma.Libraries;

namespace Neoluma.Core.Extras;

public sealed class PlatformTarget {
    private enum Platform { Windows, Linux, MacOS, iOS, Android, Other };
    private Platform platform;
    private string arch; // x86_64, arm64, wasm32

    public string toString() {
        string s = arch + "-";

        switch (platform) {
            case Platform.Windows: s += "windows"; break;
            case Platform.Linux: s += "linux"; break;
            case Platform.MacOS: s += "macos"; break;
            case Platform.iOS: s += "ios"; break;
            case Platform.Android: s += "android"; break;
            default: s += "unknown"; break;
        }
        
        return s;
    }
};

/// ProjectConfig is a class that allows me to determine project structure.
public class ProjectConfig {
    public string name = "Untitled Project";
    public string version = "1.0.0";
    public List<string> author = new(){ "Untitled Author" };
    public List<PlatformTarget> targets;
    public string license = "mit";
    public string output = "exe";
    public string sourceFolder = "src/";
    public string buildFolder = "build/";
    public Dictionary<string, string>? dependencies;
    public Dictionary<string, string>? tasks;
    public Dictionary<string, string>? tests;
    public Dictionary<string, string>? languagePacks;
    
    public CompilerSettings? settings;
    public List<string> filesList; // List of files inside the project to feed to compiler.
    public string sourcePath; // Absolute path to locate the project

    
    // Lists authors by comma. If author is only mentioned once, just author name is inputted
    public static string listAuthors(List<string> authors) {
        string authorList = "";
        bool first = true;

        foreach (string author in authors) {
            string name = author.Trim();
            if (name.Length == 0) continue;

            if (!first) authorList += ", ";
            authorList += name;
            first = false;
        }

        return authorList;
    }

    public static OutputType strToOutputType(string input) {
        switch (input.ToLower()) {
            case "exe" or "executable" or "app" or "application": return OutputType.Executable;
            case "ir" or "neoluma_ir": return OutputType.IR;
            case "llvm" or "llvm_ir": return OutputType.LLVM_IR;
            case "obj" or "object": return OutputType.Object;
            case "shared" or "sharedlib": return OutputType.SharedLibrary;
            case "static" or "staticlib" or "dll" or "dynamiclib": return OutputType.SharedLibrary;
        }
        Console.WriteLine($"{Color.TextHex("#ff5050")}[NeolumaCLI/IDtoOutput] {Localization.translate("CLI.parseProjectFile.parseOutputError")}");
        return OutputType.None;
    }
};