/*
 * Color is an internal Neoluma library used for handling terminal colors
 */
namespace Neoluma.Libraries;

public static class Color {
    public static string TextRGB(int r, int g, int b) => $"\u001b[38;2;{r};{g};{b}m";
    public static string BackgroundRGB(int r, int g, int b) => $"\u001b[48;2;{r};{g};{b}m";

    public static string TextHex(string hex) {
        if (hex.StartsWith("#")) {
            int r = Convert.ToInt32(hex.Substring(1, 2), 16);
            int g = Convert.ToInt32(hex.Substring(3, 2), 16);
            int b = Convert.ToInt32(hex.Substring(5, 2), 16);
            return TextRGB(r, g, b);
        }
        return "";
    }
    
    public static string BackgroundHex(string hex) {
        if (hex.StartsWith("#")) {
            int r = Convert.ToInt32(hex.Substring(1, 2), 16);
            int g = Convert.ToInt32(hex.Substring(3, 2), 16);
            int b = Convert.ToInt32(hex.Substring(5, 2), 16);
            return BackgroundRGB(r, g, b);
        }
        return "";
    }

    public static class Effect {
        public static string BoldOn = "\u001b[1m";
        public static string DimOn = "\u001b[2m";
        public static string UnderlineOn = "\u001b[4m";
        public static string BlinkOn = "\u001b[5m";
        public static string ReverseOn = "\u001b[7m";
        public static string HideOn = "\u001b[8m";

        public static string BoldOff = "\u001b[21m";
        public static string DimOff = "\u001b[22m";
        public static string UnderlineOff = "\u001b[24m";
        public static string BlinkOff  = "\u001b[25m";
        public static string ReverseOff = "\u001b[27m";
        public static string HideOff = "\u001b[28m";
    }

    public static string Reset = "\u001b[0m";
}
