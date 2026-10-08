/*
 * Path is an internal Neoluma library for handling paths near Neoluma executable installation.
 * Really, i made it just for that.
 */

namespace Neoluma.Libraries;

public static class Paths {
    public static string executableDir()
        => AppContext.BaseDirectory;
    
    public static string userDataDir() 
        => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Astrahelm/Neoluma");
}