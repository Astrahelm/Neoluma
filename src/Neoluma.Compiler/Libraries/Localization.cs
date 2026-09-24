using System.ComponentModel.DataAnnotations;
using System.Globalization;
using Neoluma.Libraries.Utils;

namespace Neoluma.Libraries;

public static class Localization {
    private static Dictionary<string, string> localeMap = new();
    
    // ==== Internal helpers ====
    private static string detectSystemLanguage() 
        => CultureInfo.CurrentUICulture.Name.Replace("-", "_"); // runs only once if configs don't exist
    
    static void pancakeJson(JSON.Value value, string prefix, Dictionary<string, string> output) {
        if (value.isObject()) {
            foreach (JSON.Property property in value.asObject()) {
                string key = prefix.Length == 0 ? property.key.asString() : $"{prefix}.{property.key.asString()}";
                pancakeJson(property.value, key, output);
            }

            return;
        }
        
        if (!value.isString()) {
            // FIXME: Replace later with some internal layer later maybe.
            Console.Error.WriteLine($"[Localization] Non-string value at key: {prefix}");
            return;
        }
        
        output[prefix] = value.asString();
    }

    static Dictionary<string, string> loadLocale(string locale) {
        Dictionary<string, string> output = new();

        var assembly = typeof(Program).Assembly;
        
        foreach (string resource in assembly.GetManifestResourceNames()) {
            string prefix = "Neoluma.Localization.";
            string suffix = $".{locale}.jsonc";

            if (!resource.StartsWith(prefix) || !resource.EndsWith(suffix)) continue;

            string category = resource[prefix.Length.. ^suffix.Length];
            
            string json = FileEmbed.readEmbeddedFile(resource);
            JSON.Value value = JSON.parse(json);
            
            pancakeJson(value, category, output);
        }
        
        return output;
    }
    
    // ==== Main functions ====
    public static void init() {
        localeMap = loadLocale("en_US");
        string configPath = Path.Combine(Paths.userDataDir(), "config.jsonc");
        
        if (!File.Exists(configPath)) {
            Directory.CreateDirectory(Paths.userDataDir());

            JSON.Value config = JSON.parse(
                $$"""
                  {
                      // Locale is used to determine your language Neoluma will use.
                      // Please do not modify this option unless you're familiar with languages Neoluma supports.
                      "language": "{{detectSystemLanguage()}}",
                  }
                  """
            );
            
            JSON.writeFile(configPath, config);
        }
        
        JSON.Value userConfig = JSON.parseFile(configPath);
        string language = userConfig["language"].asString();
        Dictionary<string, string> userLocale = loadLocale(language);
        
        foreach (var (key, value) in userLocale)
            localeMap[key] = value;
    }

    public static string translate(string key, params string[] args) {
        string result = localeMap.TryGetValue(key, out string? value) ? value : key;
        return args.Length == 0 ? result : string.Format(result, args);
    }
}