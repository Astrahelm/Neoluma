using Neoluma.CLI;
using Neoluma.Libraries;

class Program {
    public static void Main(string[] args) {
        CLIArgs arguments = CLIArgs.parseArgs(args);

        switch (arguments.command) {
            case "new": break;
            case "build": break;
            case "run": break;
            case "check": break;
            case "version": break;
            default: CLI.printHelp(); break;
        }
        
        Console.WriteLine($"{Color.TextHex("#34FF25")} Hello! {Color.Reset}");
    }
}

