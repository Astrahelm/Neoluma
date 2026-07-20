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
    public class ParseError : Exception {
        public int line { get; }
        public int col { get; }

        public ParseError(string message, int? line = null, int? col = null)
            : base(line.HasValue && col.HasValue ? $"{message} at line {line}, col {col}" : message) {
            this.line = line.GetValueOrDefault(-1);
            this.col = line.GetValueOrDefault(-1);
        }
    }

    // a pair of key and value
    public class Property {
        public Value key;
        public Value value;
        
        public Property(Value key, Value val) { this.key = key; value = val; }
        public Property(string key, Value val) { this.key = new Value(key); value = val; }
    }

    // Custom JSON types
    public class Array : List<Value> {}

    public class Object : List<Property> {
        public void Add(string key, Value value) { Add(new Property(new Value(key), value)); }
    }

    // ====== JSON Value ======
    public class Value {
        // Comment strings attached to this node
        public List<string> commentsBefore = new();
        public string commentsAfter;

        // int (long) | string | bool | double | Array | Object
        private object? storage;
        
        // Convenience indexing
        public Value this[string key] {
            get {
                if (storage is not Object obj) {
                    obj = new Object();
                    storage = obj;
                }
                foreach (Property prop in obj) { if (prop.key.isString() && prop.key.asString() == key) return prop.value; }
                Value val = new();
                obj.Add(new Property(key, val));
                return val;
            }
            set {
                if (storage is not Object obj) {
                    obj = new Object();
                    storage = obj;
                }
                foreach (Property prop in obj) {
                    if (prop.key.isString() && prop.key.asString() == key) {
                        prop.value = value;
                        return;
                    }
                }
                obj.Add(new Property(key, value));
            }
        }

        public Value this[int index] {
            get {
                if (storage is not Array arr) return new Value();
                if (index < 0 || index >= arr.Count) return new Value();
                return arr[index];
            }
            set {
                if (storage is not Array arr) {
                    arr = new Array();
                    storage = arr;
                }
                while (arr.Count <= index) arr.Add(new Value());
                arr[index] = value;
            }
        }

        // Constructors
        public Value() { storage = null; }
        public Value(long i) { storage = i; }
        public Value(string s) { storage = s; }
        public Value(bool b) { storage = b; }
        public Value(double d) { storage = d; }
        public Value(Array array) { storage = array; }
        public Value(Object obj) { storage = obj; }

        // Type checks
        public bool isNull() { return storage == null; }
        public bool isInt() { return storage is long; }
        public bool isBool() { return storage is bool; }
        public bool isDouble() { return storage is double; }
        public bool isString() { return storage is string; }
        public bool isArray() { return storage is Array; }
        public bool isObject() { return storage is Object; }
        
        // Getters
        public long asInt() { return storage is long i ? i : 0; }
        public bool asBool() { return storage is bool b && b; }
        public double asDouble() {
            if (storage is double d) return d;
            if (storage is long i) return i;

            return 0.0;
        }
        public string asString() { return storage is string s ? s : ""; }
        public Array asArray() { return storage is Array arr ? arr : new Array(); }
        public Object asObject() { return storage is Object obj ? obj : new Object(); }

        public object? raw() { return storage; }

        public static implicit operator Value(int v) => new((long)v);
        public static implicit operator Value(long v) => new(v);
        public static implicit operator Value(bool v) => new(v);
        public static implicit operator Value(double v) => new(v);
        public static implicit operator Value(string v) => new(v);
        public static implicit operator Value(Array v) => new(v);
        public static implicit operator Value(Object v) => new(v);
    }

    // ====== Parsing Options ======
    public class ParseOptions {
        public bool allowComments = true;
        public bool allowTrailingCommas = true; // [1,2,] { "a":1, }
        public bool allowBom = true; // UTF-8 BOM
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
    public static Value parse(string text, ParseOptions? options = null) {
        options ??= new ParseOptions();
        
        // bom character
        if (text.StartsWith('\uFEFF')) {
            if (!options.allowBom) throw new ParseError("BOM is not allowed");
            text = text[1..];
        }
        
        Lexer lexer = new(); Parser parser = new();
        return parser.parseRoot(lexer.tokenize(text, options), options);
    }

    public static Value parseFile(string filePath, ParseOptions? options = null) {
        string text = File.ReadAllText(filePath);
        return parse(text, options);
    }
    //public static string stringify(Value value, StringifyOptions? options = null) { }
    public static void writeFile(string filePath, Value value, StringifyOptions? options = null) { }

    // ====== Lexer (needed for parser) ======
    enum TokenType {
        Integer,
        String,
        Boolean,
        Double,
        Identifier,
        Null,
        Comment,
        Delimiter,
        Newline,
        Unknown,
        EndOfFile
    }

    struct Token {
        public readonly TokenType type;
        public readonly string value;
        public readonly int line;
        public readonly int col;

        public Token(TokenType type, string value, int line, int col) {
            this.type = type;
            this.value = value;
            this.line = line;
            this.col = col;
        }

        public static bool operator ==(Token a, Token b) {
            return a.type == b.type && a.value == b.value && a.line == b.line && a.col == b.col;
        }

        public static bool operator !=(Token a, Token b) {
            return !(a == b);
        }
    }

    class Lexer {
        private string src = "";
        private ParseOptions parseOptions = new();
        private int line = 1;
        private int col = 1;
        private int pos;

        // Helper
        char curChar() { return peek(); }
        char move() {
            char c = src[pos++];
            if (c == '\n') {
                line++;
                col = 1;
            }
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
        public List<Token> tokenize(string source, ParseOptions? parseOptions = null) {
            src = source;
            this.parseOptions = parseOptions ?? new();
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
                else if (char.IsWhiteSpace(c) && c != '\n') move();
                else if (char.IsDigit(c) || c == '.' || c == '+' || c == '-'
                         || match("Infinity") || match("NaN")) lexNumber(tokens);
                else if (c == '"' || c == '\'') lexString(tokens);
                else if (delimiters.Contains(c)) {
                    int sl = line;
                    int sc = col;
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
            int sl = line;
            int sc = col;
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
                number += move();
                for (int i = 0; i < 2; i++) number += move();
                tokens.Add(new Token(TokenType.Double, number, sl, sc));
                return;
            }

            // Hexadecimal numbers
            if (pos + 1 < src.Length && curChar() == '0' && (src[pos + 1] == 'x' || src[pos + 1] == 'X')) {
                number += move();
                number += move(); // 0x
                if (!char.IsAsciiHexDigit(curChar()))
                    throw new ParseError("Haven't found a hexadecimal digit after 'x'", sl, sc);
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

                if (!digitBeforeDot && (isAtEnd() || !char.IsDigit(curChar())))
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
            if (number == "+" || number == "-")
                throw new ParseError("Number doesn't have any digit but a positive or a negative sign", sl, sc);

            tokens.Add(new Token(isFloat ? TokenType.Double : TokenType.Integer, number, sl, sc));
        }

        void lexString(List<Token> tokens) {
            int sl = line;
            int sc = col;
            string str = "";
            char openingQuote = curChar();
            if (openingQuote == '\'' && !parseOptions.allowSingleQuotes) 
                throw new ParseError("Single quotes are not allowed", line, col);
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
            int sl = line;
            int sc = col;
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
                        move();
                        move();
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
            int sl = line;
            int sc = col;
            string value = "";

            while (!isAtEnd() && (char.IsLetterOrDigit(curChar()) || curChar() == '_' || curChar() == '$'))
                value += move();

            if (value == "true" || value == "false") tokens.Add(new Token(TokenType.Boolean, value, sl, sc));
            else if (value == "null") tokens.Add(new Token(TokenType.Null, value, sl, sc));
            else tokens.Add(new Token(TokenType.Identifier, value, sl, sc));
        }
    }

    // ====== Parser ======
    class Parser {
        private Value root;
        private ParseOptions parseOptions;
        private List<Token> tokens;
        private int pos;
        private int lastValueLine;

        // Helper
        Token curToken() { return peek(); }
        Token move() { return tokens[pos++]; }
        Token peek(int offset = 0) {
            int index = pos + offset;
            if (index < tokens.Count) return tokens[index];
            throw new ParseError("The token has not been found (peeking index is beyond the end of file)");
        }
        bool isAtEnd() { return pos >= tokens.Count; }
        bool match(string val) {
            if (curToken().value == val) return true;
            return false;
        }
        bool match(TokenType type) {
            if (curToken().type == type) return true;
            return false;
        }
        List<string> readCommentsBefore() {
            List<string> comments = new();

            while (!isAtEnd()) {
                if (match(TokenType.Newline)) { move(); continue; }

                if (match(TokenType.Comment)) {
                    if (!parseOptions.allowComments)
                        throw new ParseError("Comments are not allowed", curToken().line, curToken().col);

                    comments.Add(curToken().value);
                    move();
                    continue;
                }
                break;
            }
            return comments;
        }
        List<string> readCommentsAfter(int line) {
            List<string> comments = new();

            while (!isAtEnd() && match(TokenType.Comment) && curToken().line == line) {
                if (!parseOptions.allowComments)
                    throw new ParseError("Comments are not allowed", curToken().line, curToken().col);

                comments.Add(curToken().value);
                move();
            }
            return comments;
        }
        // long can't parse hex numbers
        long parseInteger(string text) {
            int sign = 1;

            if (text.StartsWith("-")) { sign = -1; text = text[1..]; }
            else if (text.StartsWith("+")) { text = text[1..]; }

            if (text.StartsWith("0x") || text.StartsWith("0X"))
                return sign * Convert.ToInt64(text[2..], 16);

            return sign * long.Parse(text, System.Globalization.CultureInfo.InvariantCulture);
        }
        // add with replacing duplicate object keys if happening
        void addProperty(Object obj, Value key, Value val, Token keyToken) {
            foreach (Property prop in obj) {
                if (prop.key.isString() && key.isString() && prop.key.asString() == key.asString()) {
                    if (!parseOptions.duplicateKeysLastWins)
                        throw new ParseError("Duplicate object key", keyToken.line, keyToken.col);
                    prop.value = val;
                    return;
                }
            }
            obj.Add(new Property(key, val));
        }
        
        // Main function
        public Value parseRoot(List<Token> toks, ParseOptions? options) {
            root = new();
            parseOptions = options ?? new();
            tokens = toks;
            pos = 0;
            
            List<string> before = readCommentsBefore();

            Value result = parseValue();
            result.commentsBefore.AddRange(before);
            result.commentsAfter.AddRange(readCommentsAfter(lastValueLine));

            readCommentsBefore();

            if (!match(TokenType.EndOfFile))
                throw new ParseError("Unexpected token after root value", curToken().line, curToken().col);

            return result;
        }

        // Parse JSON types
        Value parseValue() {
            Token token = curToken();

            if (match("[")) return parseArray();
            if (match("{")) return parseObject();
            
            move();
            lastValueLine = token.line;
            
            if (token.type == TokenType.String || token.type == TokenType.Identifier) return new Value(token.value);
            if (token.type == TokenType.Boolean) return new Value(token.value == "true");
            if (token.type == TokenType.Null) return new Value();
            if (token.type == TokenType.Integer) return new Value(parseInteger(token.value));
            if (token.type == TokenType.Double) return new Value(double.Parse(token.value, System.Globalization.CultureInfo.InvariantCulture));
            
            throw new ParseError("Unexpected value", token.line, token.col);
        }
        Value parseArray() {
            move(); // "["
            Array arr = new();
            
            List<string> before = readCommentsBefore();
            
            while (!isAtEnd() && !match("]")) {
                if (match(TokenType.EndOfFile))
                    throw new ParseError("Array was not closed", curToken().line, curToken().col);

                Value value = parseValue();
                int valueLine = lastValueLine;

                value.commentsBefore.AddRange(before);
                value.commentsAfter.AddRange(readCommentsAfter(valueLine));

                List<string> between = readCommentsBefore();
                value.commentsAfter.AddRange(between);
                
                if (match(",")) {
                    Token comma = move();
                    value.commentsAfter.AddRange(readCommentsAfter(comma.line));

                    arr.Add(value);

                    before = readCommentsBefore();

                    if (match("]")) {
                        if (!parseOptions.allowTrailingCommas) 
                            throw new ParseError("Trailing commas are not allowed", curToken().line, curToken().col);

                        break;
                    }

                    continue;
                }
                
                arr.Add(value);
                before = new();

                if (match(TokenType.EndOfFile))
                    throw new ParseError("Array was not closed", curToken().line, curToken().col);
                
                if (!match("]"))
                    throw new ParseError("Expected ',' or ']' after array value", curToken().line, curToken().col);
            }
            
            if (match("]")) lastValueLine = move().line;
            else throw new ParseError("Array was not closed", curToken().line, curToken().col);
            
            return new Value(arr);
        }

        Value parseObject() {
            move(); // "{"
            Object obj = new();
            
            List<string> before = readCommentsBefore();
            
            while (!isAtEnd() && !match("}")) {
                if (match(TokenType.EndOfFile))
                    throw new ParseError("Object was not closed", curToken().line, curToken().col);
                
                Token keyToken = curToken();
                
                if (!match(TokenType.String) && !match(TokenType.Identifier)) 
                    throw new ParseError("A key is not string or identifier", keyToken.line, keyToken.col);
                
                Value key = new(keyToken.value);
                move();
                
                if (!match(":")) throw new ParseError("Expected ':' after colon key", curToken().line, curToken().col);
                move();
                
                List<string> valueBefore = readCommentsBefore();
                
                Value value = parseValue();
                int valueLine = lastValueLine;

                value.commentsBefore.AddRange(before);
                value.commentsBefore.AddRange(valueBefore);
                value.commentsAfter.AddRange(readCommentsAfter(valueLine));

                List<string> between = readCommentsBefore();
                value.commentsAfter.AddRange(between);

                if (match(",")) {
                    Token comma = move();
                    value.commentsAfter.AddRange(readCommentsAfter(comma.line));
                    addProperty(obj, key, value, keyToken);
                    
                    before = readCommentsBefore();
                    
                    if (match("}")) {
                        if (!parseOptions.allowTrailingCommas)
                            throw new ParseError("Trailing comma is not allowed", curToken().line, curToken().col);

                        break;
                    }

                    continue;
                }
                
                addProperty(obj, key, value, keyToken);
                before = new();

                if (match(TokenType.EndOfFile))
                    throw new ParseError("Object was not closed", curToken().line, curToken().col);
                
                if (!match("}"))
                    throw new ParseError("Expected ',' or '}' after object property", curToken().line, curToken().col);
            }
            
            if (match("}")) lastValueLine = move().line;
            else throw new ParseError("Object was not closed", curToken().line, curToken().col);
            
            return new Value(obj);
        }
    }
}