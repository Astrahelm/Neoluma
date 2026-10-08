namespace Neoluma.CLI;

/// CLIArgs is a struct that allows me to parse CLI arguments
public struct CLIArgs {
    public string command; // parses commands like new, build, run, check and help
    public Dictionary<string, string> options; // parses --arguments with_values.nlp
    public List<string> positional; // anything that is not an --argument
    
    /// Argument parsing
    public static CLIArgs parseArgs(string[] args) {
        CLIArgs arguments = new CLIArgs();
        if (args.Length > 0) arguments.command = args[0];
        
        arguments.options = new(); arguments.positional = new();
        
        for (int i = 1; i < args.Length; i++) {
            string token = args[i];
            
            if (token.StartsWith("--")) {
                string key = token.Substring(2);
                string value = "";

                if (i + 1 < args.Length && !args[i + 1].StartsWith("--")) {
                    value = args[i + 1];
                    i++;
                }
                
                arguments.options[key] = value;
            }
            else arguments.positional.Add(token);
        }
        return arguments;
    }
};