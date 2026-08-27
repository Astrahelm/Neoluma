/*
 * Toml is an internal Neoluma library responsible for working with TOML format, used specifically for project files.
 */

namespace Neoluma.Libraries;

public static class Toml {
    public class Property {
        public string key;
        public Value value;
    }

    public class Value {
        private object? storage;
    }
    
    public class Array : List<Value> { }
    public class Table : List<Property> { }

    class Lexer { }

    class Parser { }
    
    class Stringifier { }
}