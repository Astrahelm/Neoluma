#include "Lexer.hpp"
#include "../Token.hpp"

#include <iostream>
#include <unordered_map>
#include <vector>
#include <print>

#include "HelperFunctions.hpp"

// ==== Main ====
std::vector<Token> Lexer::tokenize(const std::string& filePath) {
    tokens.clear();
    this->source = readFile(filePath);
    this->filePath = filePath;
    pos = 0; line = 1; column = 1;

    while (!isAtEnd()) {
        char c = curChar();

        if (c=='\n') {
            const int sl = line; const int sc = column;
            move();
            tokens.push_back(Token{TokenType::Delimeter, "\\n", filePath, sl, sc});
        }
        else if (isspace(c)) move();
        else if (isalpha(c) || c == '_') parseIK();
        else if (isdigit(c)) parseNumber();
        else if (c == '"') parseString();
        else if (c == '/' && pos + 1 < this->source.size() && (this->source[pos+1] == '/' || this->source[pos+1] == '*')) skipComment();
        else if (std::string("+-*/%^=<>!&|?~").find(c) != std::string::npos) parseOperator();
        else if (std::string("(){};:,.[]").find(c) != std::string::npos) parseDelimeter();
        else if (c == '#') parsePreprocessor();
        else if (c == '@') parseDecorator();
        else {
            std::string unknown(1, move());
            auto tok = Token{TokenType::Unknown, unknown, filePath, line, column};
            unsigned char uc = (unsigned char)c;
            if (uc > 127) {
                while (!isAtEnd() && (unsigned char)curChar() >= 128 && (unsigned char)curChar() < 192) move();
            }
            errorManager->addError(
                        ErrorType::Syntax,
                        SyntaxErrors::UnexpectedToken,
                        ErrorSpan{filePath, tok.value, tok.line, tok.column},
                        "ErrorManager.Syntax.UnexpectedToken.message", {tok.value},
                        "ErrorManager.Syntax.UnexpectedToken.hint");
            tokens.push_back(tok);
        }
    }

    tokens.push_back(Token{TokenType::EndOfFile, "", filePath, line, column});
    return tokens;
}

// ==== Helpers ====

char Lexer::curChar() const { return isAtEnd() ? '\0' : source[pos]; }
char Lexer::move() {
    char c = source[pos++];

    if (c=='\n') {
        line++;
        column = 1;
    } else column++;

    return c;
}

bool Lexer::isAtEnd() const { return pos >= source.size(); }

// ==== Parsing functions ====
void Lexer::parseIK() {
    const int sl = line; const int sc = column;
    std::string word;

    while (!isAtEnd() && (isalnum(curChar()) || curChar() == '_')) word += move();

    if (km.find(word) != km.end()) tokens.push_back(Token{TokenType::Keyword, word, filePath, sl, sc});
    else if (word == "null") tokens.push_back(Token{TokenType::Null, word, filePath, sl, sc});
    else if (om.find(word) != om.end()) tokens.push_back(Token{TokenType::Operator, word, filePath, sl, sc});
    else tokens.push_back(Token{TokenType::Identifier, word, filePath, sl, sc});
}
void Lexer::parseNumber()
{
    const int sl = line; const int sc = column;
    std::string number;

    while (!isAtEnd() && isdigit(curChar())) number += move();

    if (!isAtEnd() && curChar() == '.')
    {
        number += move();

        if (isAtEnd() || !isdigit(curChar())) {
            errorManager->addError(ErrorType::Syntax, SyntaxErrors::InvalidNumberFormat,
                ErrorSpan{filePath, number, sl, sc},
                "ErrorManager.Syntax.InvalidNumberFormat.message", {number},
                "ErrorManager.Syntax.InvalidNumberFormat.hint");
            tokens.push_back(Token{TokenType::Number, number, filePath, sl, sc});
            return;
        }
        while (!isAtEnd() && isdigit(curChar())) number += move();
    }

    if (!isAtEnd() && (curChar() == 'e' || curChar() == 'E')) {
        number += move();

        if (!isAtEnd() && (curChar() == '+' || curChar() == '-')) number += move();

        if (isAtEnd() || !isdigit(curChar())) {
            errorManager->addError(ErrorType::Syntax, SyntaxErrors::InvalidNumberFormat,
                ErrorSpan{filePath, number, sl, sc},
                "ErrorManager.Syntax.InvalidNumberFormat.message", {number},
                "ErrorManager.Syntax.InvalidNumberFormat.hint");
            tokens.push_back(Token{TokenType::Number, number, filePath, sl, sc});
            return;
        }
        while (!isAtEnd() && isdigit(curChar())) number += move();
    }

    tokens.push_back(Token{TokenType::Number, number, filePath, sl, sc});
}
void Lexer::parseString() {
    const int sl = line; const int sc = column;
    /* hardest thing to make. strings in neoluma can be multiline,
       have f-strings (variables inside ${}) inside them and support \n \t or anything i forgor.
    */

    move();
    std::string value;
    bool esc = false; // multipurpose \n stuff checker..
    bool closedStr = false;

    while (!isAtEnd()) {
        char c = curChar();
        if (esc) {
            switch (c) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                default:
                    errorManager->addError(ErrorType::Syntax, SyntaxErrors::UnexpectedToken,
                        ErrorSpan{ filePath, formatStr("\\{}",c), line, column},
                        "ErrorManager.Syntax.UnexpectedToken.message", {formatStr("\\{}",c)},
                        "ErrorManager.Syntax.UnexpectedToken.hint");
                    value += c;
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
        else value += c;

        move();
    }
    if (!closedStr){
        errorManager->addError(ErrorType::Syntax, SyntaxErrors::UnterminatedString,
            ErrorSpan{filePath, "\"", line, column},
            "ErrorManager.Syntax.UnterminatedString.message");
        return;
    }
    tokens.push_back(Token{TokenType::String, value, filePath, sl, sc});
}
void Lexer::parseOperator() {
    const int sl = line; const int sc = column;
    std::string op;
    op += move();

    if (!isAtEnd()) {
        std::string twoChar = op + curChar();
        if (om.find(twoChar) != om.end())
            op += move();
    }
    tokens.push_back(Token{TokenType::Operator, op, filePath, sl, sc});
}
void Lexer::parseDelimeter() {
    const int sl = line; const int sc = column;
    std::string delimeter;
    delimeter += move();

    if (dm.find(delimeter) != dm.end()) tokens.push_back(Token{TokenType::Delimeter, delimeter, filePath, sl, sc});
    else {
        errorManager->addError(
            ErrorType::Syntax,
            SyntaxErrors::UnexpectedToken,
            ErrorSpan{filePath, delimeter, sl, sc},
            "ErrorManager.Syntax.UnexpectedToken.message", {delimeter},
            "ErrorManager.Syntax.UnexpectedToken.hint");
        tokens.push_back(Token{TokenType::Unknown, delimeter, filePath, sl, sc});
    }
}
void Lexer::parsePreprocessor() {
    const int sl = line; const int sc = column;
    move();
    std::string word;

    while (!isAtEnd() && (isalpha(curChar()) || curChar() == '_')) word += move();

    if (pm.find(word) != pm.end()) tokens.push_back(Token{TokenType::Preprocessor, word, filePath, sl, sc});
    else {
        errorManager->addError(
            ErrorType::Preprocessor,
            PreprocessorErrors::InvalidDirective,
            ErrorSpan{filePath, '#'+word, sl, sc},
        "ErrorManager.Preprocessor.InvalidDirective.message", {"#"+word},
        "ErrorManager.Preprocessor.InvalidDirective.hint");
        tokens.push_back(Token{TokenType::Unknown, "#" + word, filePath, sl, sc});
    }
}
void Lexer::parseDecorator() {
    const int sl = line; const int sc = column;
    move();
    std::string word;

    while (!isAtEnd() && (isalpha(curChar()) || curChar() == '_')) word += move();

    tokens.push_back(Token{TokenType::Decorator, word, filePath, sl, sc});
}
void Lexer::skipComment() {
    const int sl = line; const int sc = column;

    // apparently if i don't do this check im gonna regret it
    if (isAtEnd()) {
        move();
        // single '/' at EOF - treat as operator
        tokens.push_back(Token{TokenType::Operator, "/", filePath, sl, sc});
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
            if (curChar() == '*' && (pos + 1 < source.size()) && source[pos + 1] == '/') {
                move(); // '*'
                move(); // '/'
                return;
            }
            move(); // \n_terminator3000
        }
        // Unterminated block comment
        errorManager->addError(ErrorType::Syntax, SyntaxErrors::UnterminatedComment,
            ErrorSpan{filePath, formatStr("{}{}", source[pos - 2], source[pos - 1]), sl, sc},
            "ErrorManager.Syntax.UnterminatedComment.message", {},
            "Close with */");
        return;
    }

    // Not actually a comment sequence; treat as operator
    move();
    tokens.push_back(Token{TokenType::Operator, "/", filePath, sl, sc});
}

void Lexer::printTokens() const {
    std::println("=== Lexer Output ({}) ===", filePath);
    for (const auto& token : tokens) std::cout << token.toStr();
    std::println("====================");
}