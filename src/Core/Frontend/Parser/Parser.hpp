#pragma once

#include "../Lexer/Lexer.hpp"
#include "../Nodes.hpp"
#include "../Token.hpp"
#include "Core/Extras/ErrorManager/ErrorManager.hpp"


// to make math order
int getOperatorPrecedence(const std::string& op);
bool isAssignmentOperator(const std::string& op);

//Parser
class Parser {
public:
    void parseModule(const std::vector<Token>& tok, const std::string& moduleName); // main parsing
    void printModule(int indentation = 0);

    // ErrorManager is used to report errors
    ErrorManager* errorManager = nullptr;
private:
    MemoryPtr<ModuleNode> moduleSource = nullptr;
    std::vector<Token> tokens;
    size_t pos = 0;
    std::string moduleName = "";

    // Parser helpers
    Token curToken() {
        if (pos >= tokens.size()) { Token token = Token{TokenType::EndOfFile, ""}; return token; }
        return tokens[pos];
    };
    Token lookBack() {
        if (pos >= tokens.size() || pos == 0) return Token{TokenType::EndOfFile, ""};
        return tokens[pos-1];
    }
    Token next() {
        if (pos >= tokens.size()) return Token{TokenType::EndOfFile, ""};
        return tokens[pos++];
    };
    Token lookupNext() {
        if (pos + 1 >= tokens.size()) return Token{TokenType::EndOfFile, ""};
        return tokens[pos+1];
    }
    bool match(TokenType type, const std::string& value) {
        if (curToken().type != type) return false;
        if (curToken().value != value) return false;
        return true;
    }
    bool match(TokenType type) {
        if (curToken().type != type) return false;
        return true;
    }
    bool match(const Token& token, TokenType type) {
        if (token.type != type) return false;
        return true;
    }

    bool matchTokenImpl(const Token& token, std::variant<Keywords, Operators, Delimeters, Preprocessors, Decorators> expected) {
        if (std::holds_alternative<Keywords>(expected)) {
            if (token.type != TokenType::Keyword) return false;
            auto it = km.find(token.value);
            return it != km.end() && it->second == std::get<Keywords>(expected);
        }

        if (std::holds_alternative<Operators>(expected)) {
            if (token.type != TokenType::Operator) return false;
            auto it = om.find(token.value);
            return it != om.end() && it->second == std::get<Operators>(expected);
        }

        if (std::holds_alternative<Delimeters>(expected)) {
            if (token.type != TokenType::Delimeter) return false;
            auto it = dm.find(token.value);
            return it != dm.end() && it->second == std::get<Delimeters>(expected);
        }

        if (std::holds_alternative<Preprocessors>(expected)) {
            if (token.type != TokenType::Preprocessor) return false;
            auto it = pm.find(token.value);
            return it != pm.end() && it->second == std::get<Preprocessors>(expected);
        }

        if (std::holds_alternative<Decorators>(expected)) {
            if (token.type != TokenType::Decorator) return false;
            auto it = decm.find(token.value);
            return it != decm.end() && it->second == std::get<Decorators>(expected);
        }

        return false;
    }
    bool match(std::variant<Keywords, Operators, Delimeters, Preprocessors, Decorators> expected) {
        return matchTokenImpl(curToken(), expected);
    }
    bool match(const Token& token, std::variant<Keywords, Operators, Delimeters, Preprocessors, Decorators> expected) {
        return matchTokenImpl(token, expected);
    }
    bool isAtEnd() { return pos >= tokens.size() || curToken().type == TokenType::EndOfFile; }
    std::unordered_map<std::string, Keywords> km = getKeywordMap();
    std::unordered_map<std::string, Delimeters> dm = getDelimeterMap();
    std::unordered_map<std::string, Operators> om = getOperatorMap();
    std::unordered_map<std::string, Preprocessors> pm = getPreprocessorMap();
    std::unordered_map<std::string, Decorators> decm = getDecoratorMap();

    // Expression parsing
    MemoryPtr<ASTNode> parsePrimary();
    MemoryPtr<ASTNode> parseExpression();
    MemoryPtr<ASTNode> parseBinary(int prevPrecedence = 0);
    MemoryPtr<UnaryOperationNode> parseUnary(const std::string& op);
    MemoryPtr<RawTypeNode> parseType();

    // Statement parsing
    MemoryPtr<ASTNode> parseStatement();
    MemoryPtr<DeclarationNode> parseDeclaration(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers);
    MemoryPtr<AssignmentNode> parseAssignment();

    // Control flow
    MemoryPtr<IfNode> parseIf();
    MemoryPtr<SwitchNode> parseSwitch();
    MemoryPtr<TryCatchNode> parseTryCatch();
    MemoryPtr<ForLoopNode> parseFor();
    MemoryPtr<WhileLoopNode> parseWhile();

    // Declarations
    MemoryPtr<FunctionNode> parseFunction(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers, bool isIntrinsic);
    MemoryPtr<ClassNode> parseClass(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers);
    MemoryPtr<NamespaceNode> parseNamespace();
    MemoryPtr<BlockNode> parseBlock();

    MemoryPtr<EnumNode> parseEnum(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers);
    MemoryPtr<InterfaceNode> parseInterface(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers);

    // Imports, decorators, modifiers, preprocessor
    MemoryPtr<ASTNode> parseDecorator(std::vector<MemoryPtr<CallExpressionNode>> decorators = {}, std::vector<MemoryPtr<ModifierNode>> modifiers = {}, bool isCall=false);
    std::vector<MemoryPtr<CallExpressionNode>> parseDecoratorCalls();
    std::vector<MemoryPtr<ModifierNode>> parseModifiers();
    MemoryPtr<ASTNode> parsePreprocessor();

    // Helper functions
    MemoryPtr<ASTNode> parseBlockorStatement();
    MemoryPtr<FunctionNode> parseConstructor(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers);
    bool isNextLine();
    int getOperatorPrecedence(const std::string& op);
    bool isAssignmentOperator(const std::string& op);
    // Lookahead helper to check if upcoming tokens form an assignable lvalue followed by an assignment operator
    bool isAssignableAhead(size_t offset = 0);
    std::string namespaceNameToString(ASTNode* node);
};