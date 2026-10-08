// this file is meant for structs, funcs and utilities that don't really relate to a certain file and are used in multiple files

using System.Runtime.CompilerServices;

namespace Neoluma.Core;

// Contains general information about the source of Token/Node/Error
public readonly struct SourceSpan {
    public readonly string filePath;
    public readonly int line;
    public readonly int column;

    public SourceSpan(string filePath, int line, int column) {
        this.filePath = filePath;
        this.line = line;
        this.column = column;
    }

    public SourceSpan(SourceSpan sourceSpan) {
        filePath = sourceSpan.filePath;
        line = sourceSpan.line;
        column = sourceSpan.column;
    }
}

public static class GeneralUtils {
    public static string findProjectFile(string folder) {
        foreach (string file in Directory.EnumerateFiles(folder, "*")) {
            if (Path.GetExtension(file) == ".nlp") return file;
        }
        return String.Empty;
    }
}