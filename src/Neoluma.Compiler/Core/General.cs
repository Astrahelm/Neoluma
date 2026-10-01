// this file is meant for structs, funcs and utilities that don't really relate to a certain file and are used in multiple files

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
}