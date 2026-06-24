/*
 * Json is an internal Neoluma library that allows to read or write JSON data (with comments)

Quick usage

1) Parse JSON / JSONC
--------------------
JSON.Value root = JSON.parse(text);
// or
JSON.Value root = JSON.parseFile("config.jsonc");

Throws JSON.ParseError on invalid input (never loops or hangs).

2) Read values (safe pattern)
-----------------------------
if (root.isObject()) {
    string name = root["name"].isString()
        ? root["name"].asString()
        : "default";

    int w = (root["window"].isObject() && root["window"]["w"].isInt())
        ? (int)root["window"]["w"].asInt()
        : 800;
}

3) Modify / create values
-------------------------
root["enabled"] = true;
root["count"] = Console.ToInt64(42);
root["list"] = JSON.Array{ 1, 2, 3 };

JSON.Object obj;
obj.add("a", 1);
obj.add("b", "text");
root["sub"] = obj;

4) Comments
-----------
root.commentsBefore.add("Root comment");
root["enabled"].commentsBefore.add("Toggle feature");
root["enabled"].commentsAfter = "do not touch";

5) Write back
-------------
string out = JSON.stringify(root, {
    pretty = true,
    emit_comments = true
});

// or
JSON.writeFile("out.jsonc", root);

Notes:
- Supports // and /.* *./ comments (without dots)
- Supports trailing commas
- Supports single-quoted strings
- Duplicate keys: last one wins
- Always throws on invalid JSON (no infinite loops)
*/
namespace Neoluma.Libraries;

public static class JSON {
    // ====== Error ======
    class ParseError : Exception {
        public int line { get; }
        public int col { get; }

        public ParseError(string message, int line, int col)
        : base($"{message} at line {line}, col {col}") { this.line = line; this.col = col; }
    }
    
    // a pair of key and value
    public class Property { public string Key; public Value Value; }
    
    // Custom JSON types
    public class Array : List<Value> {}
    public class Object : List<Property> {}
    
    // ====== JSON Value ======
    public class Value {
        // Comment strings attached to this node
        List<string> commentsBefore = new();
        List<string> commentsAfter = new();

        // int (long) | string | bool | double | Array | Object
        private object? storage;
        
        // Constructors
        public Value() { storage = null; }
        public Value(long i) { storage = i; }
        public Value(string s) { storage = s; }
        public Value(bool b)  { storage = b; }
        public Value(double d) { storage = d; }
        public Value(Array array) { storage = array; }
        public Value(Object obj) { storage = obj; }
        
        // Type checks
        bool isNull() { return storage == null; }
        bool isInt() { return storage is long; }
        bool isBool() { return storage is bool; }
        bool isDouble() { return storage is double; }
        bool isString() { return storage is string; }
        bool isArray() { return storage is Array; }
        bool isObject() { return storage is Object; }
    }
    
    // Convenience indexing
    //public Value this[string key] { get; set; } // creates if missing
    //public Value this[int index] { get; set; } // returns null Value if missing 
    
    // ====== Parsing Options ======
    public class ParseOptions {
        public bool allowComments = true;
        public bool allowTrailingCommas = true;   // [1,2,] { "a":1, }
        public bool allowBom = true;               // UTF-8 BOM
        public bool allowSingleQuotes = true;
        public bool duplicateKeysLastWins = true;
    }
    
    // ====== Stringifying Options ======
    public class StringifyOptions {
        public bool pretty = true;
        public int indent = 2;
        public bool emitComments = true;
        public bool escapeNonASCII = false; // if true, escape real Unicode as \uXXXX / surrogate pairs
        public bool sortKeys = false; // optional stable output
    }
    
    // ====== Main API ======
    public static Value parse(string text, ParseOptions? options = null) {}
    public static Value parseFile(string filePath, ParseOptions? options = null) {}
    public static string stringify(Value value, StringifyOptions? options = null) {}
    public static void writeFile(string filePath, Value value, StringifyOptions? options = null) {}
    
    // ====== Lexer (needed for parser) ======
    enum TokenType {Integer, String, Boolean, Double, Identifier, Null, Comment, Delimiter, Newline, Unknown, EndOfFile}
    
    struct Token {
        public readonly TokenType type;
        public readonly string token;
        public readonly int line;
        public readonly int col;

        public Token(TokenType type, string token, int line, int col) {
            this.type = type;
            this.token = token;
            this.line = line;
            this.col = col;
        }
    }
    
    class Lexer {
        private string src;
        private int line = 1;
        private int col = 1;
        private int pos;
        
        // Helper
        char curChar() { return peek(); }
        char move() {
            char c = src[pos++];
            if (c=='\n') { line++; col = 1; } 
            else col++;
            return c;
        }
        char peek(int offset = 0) {
            int index = pos + offset;
            return index < src.Length ? src[index] : '\0';
        }
        bool isAtEnd() { return pos >= src.Length; }
        // meant for watching strings from ahead
        bool match(string text) {
            if (pos + text.Length > src.Length) return false;
            return src.Substring(pos, text.Length) == text;
        }

        // Main function
        public List<Token> tokenize(string source) {
            src = source;
            List<Token> tokens = new();

            char[] delimiters = ['{', '}', '[', ']', ':', ','];

            while (!isAtEnd()) {
                char c = curChar();

                if (c == '\n') {
                    int sl = line;
                    int sc = col;
                    move();
                    tokens.Add(new Token(TokenType.Newline, "\\n", sl, sc));
                }
                else if (char.IsWhiteSpace(c) && c == ' ') move();
                else if (char.IsDigit(c) || c == '.' || c == '+' || c == '-'
                         || match("Infinity") || match("NaN")) lexNumber(tokens);
                else if (c == '"' || c == '\'') lexString(tokens);
                else if (delimiters.Contains(c)) {
                    int sl = line; int sc = col;
                    tokens.Add(new Token(TokenType.Delimiter, move().ToString(), sl, sc));
                }
                else if (c == '/') lexComment(tokens);
                else if (char.IsLetter(c) || c == '_' || c == '$') lexIdentifier(tokens);
                else tokens.Add(new Token(TokenType.Unknown, move().ToString(), line, col));
            }
            
            tokens.Add(new Token(TokenType.EndOfFile, "", line, col));
            return tokens;
        }

        void lexNumber(List<Token> tokens) {
            int sl = line; int sc = col;
            string number = "";
            bool isFloat = false;
            
            // Optional sign
            if (!isAtEnd() && (curChar() == '+' || curChar() == '-')) number += move();
            
            // Infinity | -Infinity
            if (match("Infinity")) {
                number += move();
                for (int i = 0; i < 7; i++) number += move();
                tokens.Add(new Token(TokenType.Double, number, sl, sc));
                return;
            }
            
            // NaN
            if (match("NaN")) {
                number +=  move();
                for (int i = 0; i < 2; i++) number += move();
                tokens.Add(new Token(TokenType.Double, number, sl, sc));
                return;
            }
            
            // Hexadecimal numbers
            if (pos + 1 < src.Length && curChar() == '0' && src[pos + 1] == 'x') {
                number += move(); number += move(); // 0x
                if (!char.IsAsciiHexDigit(curChar())) throw new ParseError("Haven't found a hexadecimal digit after 'x'", sl, sc);
                while (!isAtEnd() && char.IsAsciiHexDigit(curChar())) number += move();
                tokens.Add(new Token(TokenType.Integer, number, sl, sc));
                return;
            }

            // Digits
            while (!isAtEnd() && char.IsDigit(curChar())) number += move();
            
            // Floating point numbers
            if (!isAtEnd() && curChar() == '.') {
                isFloat = true;
                
                bool digitBeforeDot = false;
                if (number.Length != 0 && char.IsBetween(number.Last(), '0', '9')) digitBeforeDot = true;
                number += move(); // '.'
                
                if (!digitBeforeDot  && (isAtEnd() || !char.IsDigit(curChar()))) 
                    throw new ParseError("A number value has only a dot", sl, sc);
                while (!isAtEnd() && char.IsDigit(curChar())) number += move();
            }

            // Exponents
            if (!isAtEnd() && (curChar() == 'e' || curChar() == 'E')) {
                isFloat = true;
                number += move();
                
                if (!isAtEnd() && (curChar() == '+' || curChar() == '-')) number += move();
                if (isAtEnd() || !char.IsDigit(curChar()))
                    throw new ParseError("Number's exponent has no degree", sl, sc);
                
                while (!isAtEnd() && char.IsDigit(curChar())) number += move();
            }
            
            // If number is nothing but a sign
            if (number == "+" || number == "-") throw new ParseError("Number doesn't have any digit but a positive or a negative sign", sl, sc);
            
            tokens.Add(new Token(isFloat ? TokenType.Double : TokenType.Integer, number, sl, sc));
        }

        void lexString(List<Token> tokens) {
            int sl = line; int sc = col;
            string str = "";
            char openingQuote = curChar();
            move();
            
            while (!isAtEnd()) {
                if (curChar() == '\\') {
                    str += move();
                    if (!isAtEnd()) str += move();
                    continue;
                }
                if (curChar() == openingQuote) {
                    move();
                    tokens.Add(new Token(TokenType.String, str, sl, sc));
                    return;
                }
                if (curChar() == '\n') throw new ParseError("No backslash character before newline", sl, sc);
                str += move();
            }
            
            throw new ParseError("String doesn't have any closing quote", sl, sc);
        }

        void lexComment(List<Token> tokens) {
            int sl = line; int sc = col;
            string comment = "";
            move(); 
            
            // Single-line comment
            if (!isAtEnd() && curChar() == '/') {
                move();
                while (!isAtEnd() && curChar() != '\n') comment += move();
                
                tokens.Add(new Token(TokenType.Comment, comment, sl, sc));
                return;
            } 
            
            // Multi-line comment
            if (!isAtEnd() && curChar() == '*') {
                move();
                while (!isAtEnd()) {
                    if (curChar() == '*' && peek(1) == '/') {
                        move(); move();
                        tokens.Add(new Token(TokenType.Comment, comment, sl, sc));
                        return;
                    }

                    comment += move();
                }
                throw new ParseError("Unterminated comment", sl, sc);
            }
            
            throw new ParseError("Expected comment after '/'", sl, sc);
        }

        void lexIdentifier(List<Token> tokens) {
            int sl = line; int sc = col;
            string value = "";

            while (!isAtEnd() && (char.IsLetterOrDigit(curChar()) || curChar() == '_' || curChar() == '$'))
                value += move();

            if (value == "true" || value == "false") tokens.Add(new Token(TokenType.Boolean, value, sl, sc));
            else if (value == "null") tokens.Add(new Token(TokenType.Null, value, sl, sc));
            else tokens.Add(new Token(TokenType.Identifier, value, sl, sc));
        }
    }
}
    // ====== Parser ======
    // class JSONParser {
    //     private Object root;
    //     private ParseOptions parseOptions;
    //     private string source;
    //     private int pos = 0;
    //
    //     // Main function
    //     void parseRoot(string text, ParseOptions? options) {
    //         root = new();
    //         parseOptions = options ?? new();
    //         source = text;
    //         
    //         if (source.StartsWith("[")) parseArray();
    //         else if (source.StartsWith("{")) parseObject();
    //     }
    //
    //     // Parse JSON types
    //     void parseInt() {}
    //     void parseBool() {}
    //     void parseDouble() {}
    //     void parseString() {}
    //     void parseArray() {
    //         pos++;
    //         while (pos < source.Length || source[pos] == ']') {
    //             
    //         }
    //         if (source[pos] != ']') throw ParseError("")
    //     }
    //     void parseObject() {}
    // }