using Neoluma.Core.Extras;
using Neoluma.Libraries.Utils;

namespace Neoluma.CLI;

public static class CLI {
    /// Compiles Neoluma program into target output
    public static void build(string nlpFile) {}
    /// Runs the code interpreted way. Useful for testing.
    public static void run(string nlpFile) {}
    /// Checks code on errors. Doesn't generate any binaries
    public static void check(string nlpFile, bool jsonOutput = false) {}
    /// Creates a project
    public static void createProject(ProjectConfig config) {}
    /// Creates a project (Without ProjectConfig)
    public static void createProject() {}
    
    /// Help function that just tells details about compiler and it's CLI.
    public static void printHelp() {}
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
    public static string getLicenseText(ProjectConfig config, string license) {
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

