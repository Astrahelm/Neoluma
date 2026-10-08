using Neoluma.Core;
using Neoluma.Core.Extras;
using Neoluma.Libraries;
using Neoluma.Libraries.Utils;

namespace Neoluma.CLI;

public static class CLI {
    // ==== Main functions ====

    /// Compiles Neoluma program into target output
    public static void build(string nlpFile) {
        ProjectConfig config = CLIParser.parseProjectFile(nlpFile);
        Console.WriteLine($"{Localization.translate("CLI.build.initialization")} {config.name}");
        // todo: compiler call, executable generation
    }

    /// Runs the code interpreted way. Useful for testing.
    public static void run(string nlpFile) {
        build(nlpFile);
        Console.WriteLine($"{Localization.translate("CLI.run.initialization")}\n");
        // todo: launch the file using std::system or CreateProcess
    }

    /// Checks code on errors. Doesn't generate any binaries
    public static void check(string nlpFile, bool jsonOutput = false) {
        ProjectConfig config = CLIParser.parseProjectFile(nlpFile);
        CompilationInput input = new();

        input.targetOutput = ProjectConfig.strToOutputType(config.output);
        input.settings = config.settings ?? new();
        foreach (string file in Directory.EnumerateFiles(Path.Combine(config.sourcePath, config.sourceFolder), "*", new EnumerationOptions{RecurseSubdirectories = true, IgnoreInaccessible = true})) {
            if (Path.GetExtension(file) == ".nm") input.files!.Add(file);
        }

        DependencyInput stdDependency = new(){
            rootPath = Path.Combine(AppContext.BaseDirectory, "modules", "std"),
            sourceFolder = "src"
        };
        input.dependencies["std"] = stdDependency; // todo: doesn't support external for now

        Compiler compiler = new(input);
        if (!jsonOutput) Console.WriteLine($"{Color.TextHex("#75ff87")}{String.Format(Localization.translate("CLI.check.initialization"), config.name)}{Color.Reset}");
        compiler.check(jsonOutput);
    }

    /// Creates a project
    public static void createProject(ProjectConfig config) {
        string projectPath = Path.Combine(Directory.GetCurrentDirectory(), config.name);
        Directory.CreateDirectory(Path.Combine(projectPath, "src"));
        
        File.WriteAllText(Path.Combine(projectPath, "src/main.nm"), String.Format("// {} \n@entry\nfn main() {{\n    print(\"{}\");\n}}", Localization.translate("CLI.createProject.template.main.comment"), Localization.translate("CLI.createProject.template.main.printmsg")));
        
        /*
        Toml::Table table;
        Toml::Table project;
        project["name"] = config.name;
        project["version"] = config.version;

        Toml::TomlArray authors_array;
        for (const auto& author : config.author) authors_array.push_back(Toml::TomlValue(author));
        project["authors"] = Toml::TomlValue(authors_array);
        project["license"] = licenseID(config.license);
        project["output"] = outputID(config.output);
        project["sourceFolder"] = config.sourceFolder;
        project["buildFolder"] = config.buildFolder;
        table["project"] = project.get();

        Toml::Table tasks;
        tasks["dev"] = "neoluma run --debug";
        table["tasks"] = tasks.get();

        std::ofstream cfg(projectPath / std::format("{}.nlp", formatNameToSnakeCase(config.name)));
        if (cfg.is_open()){
            Toml::serializeTable(cfg, table.get());
        }
        cfg.close();
        */
        File.WriteAllText(Path.Combine(projectPath, "LICENSE"), Licenses.getLicenseText(config));
    }

    /// Creates a project (Without ProjectConfig)
    public static void createProject() {
        ProjectConfig config = new();
        int steps = 5; int step = 0;
        string title = $"{Localization.translate("CLI.createProject.initialization")} ";

        clearScreen();
        showProgressBar(title, step++, steps);

        config.name = Asker.input(Localization.translate("CLI.createProject.projectName"), true);
        clearScreen();
        showProgressBar(title, step++, steps);
        config.version = Asker.input(Localization.translate("CLI.createProject.projectVersion"));
        clearScreen();
        showProgressBar(title, step++, steps);
        List<string> authors = Asker.input(Localization.translate("CLI.createProject.projectAuthors"), true).Split(',').ToList();
        string authorList = listAuthors(authors);
        config.author = authors;
        clearScreen();
        showProgressBar(title, step++, steps);
        List<string> licenses = new(){ "MIT", "Apache 2.0", "GNU GPL v3", "BSD 2-Clause \"Simplified\"", "BSD 3-Clause \"New\" or \"Revised\"", "Boost Software 1.0", "CC0 v1 Universal", "Eclipse", "GNU AGPL v3", "GNU GPL v2", "GNU LGPL v2.1", "Mozilla 2.0", "The Unlicense", "Custom" };
        string license = Asker.selectList(Localization.translate("CLI.createProject.projectLicense"), licenses);

        //gpl2, gpl3, bsd2, bsd3, boost, cc0, eclipse, agpl, lgpl, mozilla, unlicense
        if (license == "MIT") config.license = "mit";
        else if (license == "Apache 2.0") config.license = "apache";
        else if (license == "GNU GPL v3") config.license = "gpl3";
        else if (license == "BSD 2-Clause \"Simplified\"") config.license = "bsd2";
        else if (license == "BSD 3-Clause \"New\" or \"Revised\"") config.license = "bsd3";
        else if (license == "Boost Software 1.0") config.license = "boost";
        else if (license == "CC0 v1 Universal") config.license = "cc0";
        else if (license == "Eclipse") config.license = "eclipse";
        else if (license == "GNU AGPL v3") config.license = "agpl";
        else if (license == "GNU GPL v2") config.license = "gpl2";
        else if (license == "GNU LGPL v2.1") config.license = "lgpl";
        else if (license == "Mozilla 2.0") config.license = "mozilla";
        else if (license == "The Unlicense") config.license = "unlicense";
        else config.license = "custom";
        
        clearScreen();
        showProgressBar(title, step++, steps);
        bool confirmation = Asker.confirm($"{Color.TextHex("#FF8C75")}{String.Format(Localization.translate("CLI.createProject.confirmation"), config.name, config.version, authorList, license, Color.TextHex("#96fcbd"))}{Color.Reset}");
        if (confirmation) {
            createProject(config);
            clearScreen();
            Console.WriteLine(Localization.translate("CLI.createProject.confirmation.yes"), Color.TextHex("#75ff87"), Color.Reset);
        } else Console.WriteLine(Localization.translate("CLI.createProject.confirmation.no"), Color.TextHex("#ff5050"), Color.Reset);
        Console.WriteLine(Color.Reset);
    }

    /// Help function that just tells details about compiler and it's CLI.
    public static void printHelp() => Console.WriteLine(Localization.translate("CLI.helpMessage"));
    
    // ==== Helping functions ====

    // Lists authors by comma. If author is only mentioned once, just author name is inputted
    static string listAuthors(List<string> authors) {
        string authorList = String.Empty;
        bool first = true;

        foreach (string raw in authors) {
            string name = raw.Trim();
            if (name.Length == 0) continue;

            if (!first) authorList += ", ";
            authorList += name;
            first = false;
        }

        return authorList;
    }

    // Formats the input string to letters and underscores.
    static string formatNameToSnakeCase(string input) {
        return input.ToLower().Replace(' ', '_');
    }

    // Returns progress bar for CLI
    static void showProgressBar(string stepName, int step, int total) {
        int percentage = (step * 100) / total;
        int hashes = percentage / 5;
        if (percentage != 100) Console.WriteLine($"{Color.TextHex("#f6ff75")} [ {stepName} ({'#'*hashes}{'_'*(20-hashes)}) {percentage}% ] {Color.TextHex("#01e0d4")}");
        else Console.WriteLine($"{Color.TextHex("#75ff87")} [ {stepName} ({'#'*hashes}{'_'*(20-hashes)}) {percentage}% ] {Color.TextHex("#01e0d4")}");
    }

    // Clears terminal screen
    static void clearScreen() {
        Console.Write("\u001b[0m\u001b[2J\u001b[H");
    }
}

class Licenses {
    private static string Apache = FileEmbed.readEmbeddedFile("LicenseTemplates.Apache");
    private static string BoostV1 = FileEmbed.readEmbeddedFile("LicenseTemplates.BoostV1");
    private static string BSDv2Simplified = FileEmbed.readEmbeddedFile("LicenseTemplates.BSDv2Simplified");
    private static string BSDv3NewRevised = FileEmbed.readEmbeddedFile("LicenseTemplates.BSDv3NewRevised");
    private static string CC0v1 = FileEmbed.readEmbeddedFile("LicenseTemplates.CC0v1");
    private static string EclipseV2 = FileEmbed.readEmbeddedFile("LicenseTemplates.EclipseV2");
    private static string GNUAGPLv3 = FileEmbed.readEmbeddedFile("LicenseTemplates.GNUAGPLv3");
    private static string GNUGPLv2 = FileEmbed.readEmbeddedFile("LicenseTemplates.GNUGPLv2");
    private static string GNUGPLv3 = FileEmbed.readEmbeddedFile("LicenseTemplates.GNUGPLv3");
    private static string GNULGPLv2_1 = FileEmbed.readEmbeddedFile("LicenseTemplates.GNULGPLv2_1");
    private static string MIT = FileEmbed.readEmbeddedFile("LicenseTemplates.MIT");
    private static string MozillaV2 = FileEmbed.readEmbeddedFile("LicenseTemplates.MozillaV2");
    private static string Unlicense = FileEmbed.readEmbeddedFile("LicenseTemplates.Unlicense");
    
    /// Available identifiers: mit, apache, gpl2, gpl3, bsd2, bsd3, boost, cc0, eclipse, agpl, lgpl, mozilla, unlicense. Otherwise returns specific message
    public static string getLicenseText(ProjectConfig config) {
        switch (config.license) {
            case "mit": return String.Format(MIT, DateTime.Today.Year, ProjectConfig.listAuthors(config.author));
            case "apache": return Apache;
            case "gpl2": return GNUGPLv2;
            case "gpl3": return GNUGPLv3;
            case "bsd2": return String.Format(BSDv2Simplified, DateTime.Today.Year, ProjectConfig.listAuthors(config.author));
            case "bsd3": return String.Format(BSDv3NewRevised, DateTime.Today.Year, ProjectConfig.listAuthors(config.author));
            case "boost": return BoostV1;
            case "cc0": return CC0v1;
            case "eclipse": return EclipseV2;
            case "agpl": return GNUAGPLv3;
            case "lgpl": return GNULGPLv2_1;
            case "mozilla": return MozillaV2;
            case "unlicense": return Unlicense;
            default: return "We haven't found licenses for your case. Please delete this text and insert your license here, or delete the license file completely.";
        }
    }
}

