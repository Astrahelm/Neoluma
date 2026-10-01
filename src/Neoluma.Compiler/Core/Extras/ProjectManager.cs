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
    public required List<PlatformTarget> targets;
    public string license = "mit";
    public string output = "exe";
    public string sourceFolder = "src/";
    public string buildFolder = "build/";
    public Dictionary<string, string>? dependencies;
    public Dictionary<string, string>? tasks;
    public Dictionary<string, string>? tests;
    public Dictionary<string, string>? languagePacks;
    
    public CompilerSettings? settings;
    public required List<string> filesList; // List of files inside the project to feed to compiler.
    public required string sourcePath; // Absolute path to locate the project

    
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
};