using System.Reflection;
using Neoluma.CLI;
using Neoluma.Libraries;

class Program {
    public static void Main(string[] args) {
        Assembly assembly = typeof(Program).Assembly;
        CLIArgs arguments = CLIArgs.parseArgs(args);
        Localization.init();

        switch (arguments.command) {
            case "new": break;
            case "build": break;
            case "run": break;
            case "check": break;
            case "version": 
                Console.WriteLine($"{assembly.GetCustomAttribute<AssemblyProductAttribute>()?.Product} ({assembly.GetName().Name}) {assembly.GetCustomAttributes<AssemblyMetadataAttribute>().FirstOrDefault(x => x.Key == "ReleaseType")?.Value} Release v{assembly.GetName().Version?.Major}.{assembly.GetName().Version?.Minor}");
                break;
            default: CLI.printHelp(); break;
        }
    }
}

