using System.Text;

namespace Neoluma.Libraries.Utils;

public class FileEmbed {
    public static string readEmbeddedFile(string name) {
        var assembly = typeof(Program).Assembly;

        using Stream stream = assembly.GetManifestResourceStream(name) 
                              ?? throw new InvalidOperationException($"Embedded resource not found: {name}");
    
        using var reader = new StreamReader(stream, Encoding.UTF8);
        return reader.ReadToEnd();
    }
}