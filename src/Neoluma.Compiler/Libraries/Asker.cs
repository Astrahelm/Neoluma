/*
 * Asker is an internal Neoluma library for handling CLI input
 */

namespace Neoluma.Libraries;

public static class Asker {
    private static void clearBlock(int lines) {
        for (int i = 0; i < lines; i++) Console.Write("\u001b[1A\u001b[2K\r");
    }
    
    public static string input(string question, bool required = false) {
        Console.Write($"{Color.TextHex("#75b5ff")}>> {question} {Color.TextHex("#ff28e6")}");
        while (true) {
            string? response = Console.ReadLine();
            if (!required) { Console.Write(Color.Reset);
                return response ?? "";
            }
            if (!string.IsNullOrEmpty(response)) { Console.Write(Color.Reset);
                return response;
            }
        } 
    }

    public static bool confirm(string question) {
        bool result = true;
        string response = input($"{question} {Color.TextHex("#e84b85")} (y/n) {Color.TextHex("#ff28e6")}");
        if (response != "y" && response != "Y") result = false;
        Console.Write(Color.Reset);
        return result;
    }

    public static string selectList(string question, List<string> options) {
        int pos = 0;
        int lines = options.Count + 2;
        
        while (true) {
            clearBlock(lines);
            
            Console.WriteLine($"{Color.TextHex("#75b5ff")}>> {question}");

            for (int i = 0; i < options.Count; i++) {
                if (i == pos) Console.WriteLine($"{Color.TextHex("#00ff48")}> {Color.TextHex("#ff28e6")}{options[i]}");
                else Console.WriteLine($"{Color.TextHex("#ff28e6")} {options[i]}");
            }
            
            ConsoleKey key = Console.ReadKey(true).Key;
            if ((key == ConsoleKey.UpArrow || key == ConsoleKey.NumPad8 || key == ConsoleKey.W) && pos > 0) pos--;
            else if ((key == ConsoleKey.DownArrow || key == ConsoleKey.NumPad2 || key == ConsoleKey.S) && pos < options.Count - 1) pos++;
            else if (key == ConsoleKey.Enter) break;
        }
        
        Console.WriteLine(Color.Reset);
        return options[pos];
    }
}