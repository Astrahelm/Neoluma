using System.Text;
using Neoluma.Core.Extras;

namespace Neoluma.Core.Frontend;

public class Lexer {
    // Data
    private List<Token> tokens = new();
    private string source = "";
    private string filePath = ""; private int pos; private int line = 1; private int column = 1;
    public ErrorManager errorManager = null!;
    
    // Helpers
    char curChar() => isAtEnd() ? '\0' : source[pos];
    char move() {
        char c = source[pos++];
        if (c=='\n') { line++; column = 1; } 
        else column++;
        return c;
    }
    bool isAtEnd() => pos >= source.Length;

    // Main functions
    public List<Token> tokenize(string filePath) {
        tokens.Clear();
        source = File.ReadAllText(filePath);
        this.filePath = filePath;
        pos = 0; line = 1; column = 1;

        while (!isAtEnd()) {
            char c = curChar();

            if (c=='\n') {
                int sl = line; int sc = column;
                move();
                tokens.Add(new Token(TokenType.Delimeter, "\\n", filePath, sl, sc));
            }
            else if (char.IsWhiteSpace(c)) move();
            else if (char.IsLetter(c) || c == '_') lexIK();
            else if (char.IsDigit(c)) lexNumber();
            else if (c == '"') lexString();
            else if (c == '/' && pos + 1 < source.Length && (source[pos+1] == '/' || source[pos+1] == '*')) skipComment();
            else if ("+-*/%^=<>!&|?~".Contains(c)) lexOperator();
            else if ("(){};:,.[]".Contains(c)) lexDelimeter();
            else if (c == '#') lexPreprocessor();
            else if (c == '@') lexDecorator();
            else {
                string unknown = move().ToString();
                Token tok = new(TokenType.Unknown, unknown, filePath, line, column);
                
                errorManager.addError(
                    SyntaxErrors.UnexpectedToken,
                    new ErrorSpan(filePath, tok.value, tok.line, tok.column),
                    "ErrorManager.Syntax.UnexpectedToken.message", new() { tok.value },
                    "ErrorManager.Syntax.UnexpectedToken.hint");
                tokens.Add(tok);
            }
        }

        tokens.Add(new Token(TokenType.EndOfFile, "", filePath, line, column));
        return tokens;
    }

    public void printTokens() {  // Debug command to check tokens correctness
        Console.WriteLine($"=== Lexer Output ({filePath}) ===");
        foreach (var token in tokens) Console.Write(token.toStr());
        Console.WriteLine("====================");
    }
    
    // Lexs
    void lexIK() { // Lex identifier or keyword
        int sl = line; int sc = column;
        int start = pos;

        while (!isAtEnd() && (char.IsLetterOrDigit(curChar()) || curChar() == '_')) move();

        string word = source[start..pos];

        if (TokenMaps.keywords.ContainsKey(word)) tokens.Add(new Token(TokenType.Keyword, word, filePath, sl, sc));
        else if (word == "null") tokens.Add(new Token(TokenType.Null, word, filePath, sl, sc));
        else if (TokenMaps.operators.ContainsKey(word)) tokens.Add(new Token(TokenType.Operator, word, filePath, sl, sc));
        else tokens.Add(new Token(TokenType.Identifier, word, filePath, sl, sc));
    } 

    void lexNumber() {
        int sl = line; int sc = column;
        int start = pos;

        while (!isAtEnd() && char.IsDigit(curChar())) move();

        if (!isAtEnd() && curChar() == '.') {
            move();

            if (isAtEnd() || !char.IsDigit(curChar())) {
                string number = source[start..pos];
                errorManager.addError(
                    SyntaxErrors.InvalidNumberFormat,
                    new ErrorSpan(filePath, number, sl, sc),
                    "ErrorManager.Syntax.InvalidNumberFormat.message", new() { number },
                    "ErrorManager.Syntax.InvalidNumberFormat.hint");
                tokens.Add(new Token(TokenType.Number, number, filePath, sl, sc));
                return;
            }
            while (!isAtEnd() && char.IsDigit(curChar())) move();
        }

        if (!isAtEnd() && (curChar() == 'e' || curChar() == 'E')) {
            move();

            if (!isAtEnd() && (curChar() == '+' || curChar() == '-')) move();

            if (isAtEnd() || !char.IsDigit(curChar())) {
                string number = source[start..pos];
                errorManager.addError(
                    SyntaxErrors.InvalidNumberFormat,
                    new ErrorSpan(filePath, number, sl, sc),
                    "ErrorManager.Syntax.InvalidNumberFormat.message", new() { number },
                    "ErrorManager.Syntax.InvalidNumberFormat.hint");
                tokens.Add(new Token(TokenType.Number, number, filePath, sl, sc));
                return;
            }
            while (!isAtEnd() && char.IsDigit(curChar())) move();
        }
        
        string result = source[start..pos];
        tokens.Add(new Token(TokenType.Number, result, filePath, sl, sc));
    }

    void lexString() {
        int sl = line; int sc = column;
        /* hardest thing to make. strings in neoluma can be multiline,
           have f-strings (variables inside ${}) inside them and support \n \t or anything i forgor.
        */

        move();
        StringBuilder value = new();
        bool esc = false; // multipurpose \n stuff checker..
        bool closedStr = false;

        while (!isAtEnd()) {
            char c = curChar();
            if (esc) {
                switch (c) {
                    case 'n': value.Append('\n'); break;
                    case 't': value.Append('\t'); break;
                    case '\\': value.Append('\\'); break;
                    case '"': value.Append('"'); break;
                    default:
                        errorManager.addError(
                            SyntaxErrors.UnexpectedToken,
                            new ErrorSpan(filePath, $"\\{c}", line, column),
                            "ErrorManager.Syntax.UnexpectedToken.message", new() { $"\\{c}" },
                            "ErrorManager.Syntax.UnexpectedToken.hint");
                        value.Append(c);
                        break;
                }
                esc = false;
            }
            else if (c == '\\') esc = true;
            else if (c == '"') {
                move();
                closedStr = true;
                break;
            }
            else value.Append(c);

            move();
        }
        if (!closedStr){
            errorManager.addError(
                SyntaxErrors.UnterminatedString,
                new ErrorSpan(filePath, "\"", line, column),
                "ErrorManager.Syntax.UnterminatedString.message");
            return;
        }
        tokens.Add(new Token(TokenType.String, value.ToString(), filePath, sl, sc));
    }

    void lexOperator() {
        int sl = line; int sc = column;
        string op = "";
        op += move();

        if (!isAtEnd()) {
            string twoChar = op + curChar();
            if (TokenMaps.operators.ContainsKey(twoChar))
                op += move();
        }
        tokens.Add(new Token(TokenType.Operator, op, filePath, sl, sc));
    }

    void lexDelimeter() {
        int sl = line; int sc = column;
        string delimeter = "";
        delimeter += move();

        if (TokenMaps.delimeters.ContainsKey(delimeter)) tokens.Add(new Token(TokenType.Delimeter, delimeter, filePath, sl, sc));
        else {
            errorManager.addError(
                SyntaxErrors.UnexpectedToken,
                new ErrorSpan(filePath, delimeter, sl, sc),
                "ErrorManager.Syntax.UnexpectedToken.message", new() { delimeter },
                "ErrorManager.Syntax.UnexpectedToken.hint");
            tokens.Add(new Token(TokenType.Unknown, delimeter, filePath, sl, sc));
        }
    }

    void lexPreprocessor() {
        int sl = line; int sc = column;
        move();
        int start = pos;

        while (!isAtEnd() && (char.IsLetter(curChar()) || curChar() == '_')) move();
        
        string word = source[start..pos];

        if (TokenMaps.preprocessors.ContainsKey(word)) tokens.Add(new Token(TokenType.Preprocessor, word, filePath, sl, sc));
        else {
            errorManager.addError(
                PreprocessorErrors.InvalidDirective,
                new ErrorSpan(filePath, "#" + word, sl, sc),
                "ErrorManager.Preprocessor.InvalidDirective.message", new() { "#" + word },
                "ErrorManager.Preprocessor.InvalidDirective.hint");
            tokens.Add(new Token(TokenType.Unknown, "#" + word, filePath, sl, sc));
        }
    }

    void lexDecorator() {
        int sl = line; int sc = column;
        move();
        int start = pos;

        while (!isAtEnd() && (char.IsLetter(curChar()) || curChar() == '_')) move();
        
        string word = source[start..pos];

        tokens.Add(new Token(TokenType.Decorator, word, filePath, sl, sc));
    }

    void skipComment() {
        int sl = line; int sc = column;

        // apparently if i don't do this check im gonna regret it
        if (isAtEnd()) {
            move();
            // single '/' at EOF - treat as operator
            tokens.Add(new Token(TokenType.Operator, "/", filePath, sl, sc));
            return;
        }

        // Single-line comment
        if (source[pos] == '/' && source[pos+1] == '/') {
            move(); // '/'
            move(); // '/'
            while (!isAtEnd() && curChar() != '\n') move();
            if (curChar() == '\n') move();
            return;
        }

        // Block comment '/* ... */'
        if (source[pos] == '/' && source[pos+1] == '*') {
            move(); // '/'
            move(); // '*'
            while (!isAtEnd()) {
                if (curChar() == '*' && (pos + 1 < source.Length) && source[pos + 1] == '/') {
                    move(); // '*'
                    move(); // '/'
                    return;
                }
                move(); // \n_terminator3000
            }
            // Unterminated block comment
            errorManager.addError(
                SyntaxErrors.UnterminatedComment, 
                new ErrorSpan(filePath, $"{source[pos - 2]}{source[pos - 1]}", sl, sc), 
                "ErrorManager.Syntax.UnterminatedComment.message", null,
                "ErrorManager.Syntax.UnterminatedComment.hint");
            return;
        }

        // Not actually a comment sequence; treat as operator
        move();
        tokens.Add(new Token(TokenType.Operator, "/", filePath, sl, sc));
    }
}
