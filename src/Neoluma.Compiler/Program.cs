using System.Reflection;
using Neoluma.CLI;
using Neoluma.Core;
using Neoluma.Core.Extras;
using Neoluma.Libraries;

class Program {
    public static void Main(string[] args) {
        Assembly assembly = typeof(Program).Assembly;
        CLIArgs arguments = CLIArgs.parseArgs(args);
        Localization.init();

        switch (arguments.command) {
            case "new": {
                if (arguments.options.Count != 0) {
                    ProjectConfig config = new();
                    if (arguments.options.TryGetValue("name", out var name)) config.name = name;
                    if (arguments.options.TryGetValue("author", out var author)) config.author = author.Split(',').ToList();
                    if (arguments.options.TryGetValue("version", out var version)) config.version = version;
                    if (arguments.options.TryGetValue("license", out var license)) config.license = license;
                    CLI.createProject(config);
                } // If no arguments provided, aka "neoluma new", it will run an integrated assistant to set up a project.
                else CLI.createProject();
                break;
            }
            case "build": break;
            case "run": break;
            case "check": {
                string projectFilePath = String.Empty;

                if (arguments.options.TryGetValue("project", out var project)) {
                    string input = project;

                    if (Directory.Exists(input)) projectFilePath = GeneralUtils.findProjectFile(input);
                    else if (File.Exists(input)) projectFilePath = input;
                }
                else if (arguments.positional.Count > 0) Console.WriteLine("Test");
                else {
                    projectFilePath = GeneralUtils.findProjectFile(Directory.GetCurrentDirectory());

                    if (projectFilePath.Length == 0) 
                        throw new FileNotFoundException($"{Color.TextHex("#ff5050")}[Neoluma/Check] Project file was not found!{Color.Reset}");
                }

                CLI.check(projectFilePath, arguments.options.TryGetValue("json", out var option) && Convert.ToBoolean(option));
                break;
            }
            case "version": 
                Console.WriteLine($"{Color.TextHex("#ff28e6")}{assembly.GetCustomAttribute<AssemblyProductAttribute>()?.Product} ({assembly.GetName().Name}) {assembly.GetCustomAttributes<AssemblyMetadataAttribute>().FirstOrDefault(x => x.Key == "ReleaseType")?.Value} Release v{assembly.GetName().Version?.Major}.{assembly.GetName().Version?.Minor}");
                break;
            default: CLI.printHelp(); break;
        }
    }
}

