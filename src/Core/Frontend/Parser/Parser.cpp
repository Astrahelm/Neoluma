#include <iostream>
#include <print>
#include <algorithm>

#include "Core/Compiler.hpp"
#include "Libraries/Color/Color.hpp"
#include "Libraries/Localization/Localization.hpp"

#include "../Nodes.hpp"
#include "Parser.hpp"

#include "../Token.hpp"
#include "../../../HelperFunctions.hpp"
#include "ASTBuilder.hpp"

// ==== Print the parser output ====
void Parser::printModule(int indentation) {
    if (!moduleSource) {
        std::println(std::cerr, "[Neoluma/Parser][\"{}\"] No module to print.", __func__);
        return;
    }

    std::println("{}", moduleSource->toString(indentation));
}

// ==== Main parsing ====
void Parser::parseModule(const std::vector<Token>& tok, const std::string& name) {
    // initialization
    this->moduleSource = nullptr; this->tokens = tok; this->moduleName = name; this->pos = 0;

    auto moduleNode = ASTBuilder::createModule(moduleName);
    moduleNode->line = 0; moduleNode->column = 0; moduleNode->filePath = curToken().filePath;

    // FIXME: Reevaluate why the fuck this duct tape method exists. Remake safety guards, since i don't remember it's purpose
    while (!isAtEnd()) {
        if (match(Delimeters::Semicolon)) { next(); continue; }

        MemoryPtr<ASTNode> stmt = parseStatement();
        if (!stmt && match(Delimeters::RightBraces)) { next(); continue; }
        if (!stmt) {
            while (!isAtEnd() && !isNextLine()) next();

            size_t startPos = pos;
            int guard = 0;
            while (!isAtEnd() && guard++ < 100){
                if (isNextLine()){
                    next();
                    break;
                }
                if (match(Delimeters::RightBraces)){
                    break;
                }
                next();
            }

            if (pos == startPos && !isAtEnd()) next();

            if (guard >= 100) {
                std::println(std::cerr, "{}{}{}", Color::TextHex("#ff5050"), formatStr(Localization::translate("Compiler.Core.ErrorManager.safetyGuard"), __func__, curToken().filePath, curToken().line, curToken().column), Color::Reset);
                break;
            }

            continue;
        }
        moduleNode->body.push_back(std::move(stmt));
    }

    moduleSource = std::move(moduleNode);
}

// ==== Statement parsing ====
MemoryPtr<ASTNode> Parser::parseStatement() {
    while (isNextLine()) next(); // Skips newlines in case they ever appear
    if (isAtEnd()) return nullptr;

    Token token = curToken();
    std::vector<MemoryPtr<CallExpressionNode>> decorators = {};

    if (match(TokenType::Preprocessor)) return parsePreprocessor();
    if (match(TokenType::Decorator)) decorators = parseDecoratorCalls();
    while (isNextLine()) next();

    auto modifiers = parseModifiers();

    if (match(TokenType::Identifier) && (match(lookupNext(), Operators::Nullable) || match(lookupNext(), Delimeters::Colon)))
        return parseDeclaration(std::move(decorators), std::move(modifiers));

    token = curToken();
    // === Control Flow Keywords ===
    if (match(TokenType::Keyword)) {
        if (match(token, Keywords::If)) return parseIf();
        if (match(token, Keywords::Switch)) return parseSwitch();
        if (match(token, Keywords::Try)) return parseTryCatch();
        if (match(token, Keywords::For)) return parseFor();
        if (match(token, Keywords::While)) return parseWhile();
        if (match(token, Keywords::Return)) {
            next();

            // allow "return;" or "return }"
            if (isNextLine() || match(Delimeters::RightBraces)) {
                if (isNextLine()) next();
                auto node = ASTBuilder::createReturnStatement(nullptr);
                node->line = token.line; node->column = token.column; node->filePath = token.filePath;
                return node;
            }

            auto expr = parseExpression();
            if (!expr) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{lookBack().filePath, lookBack().value, lookBack().line, lookBack().column},
                    "ErrorManager.Syntax.MissingToken.noVariableAfter.message", {"return"},
                    "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", {"return"});
                return nullptr;
            }

            if (isNextLine()) next();

            auto node = ASTBuilder::createReturnStatement(std::move(expr));
            node->line = token.line; node->column = token.column; node->filePath = token.filePath;
            return node;
        }
        if ((match(token, Keywords::Throw))) {
            next();
            if (isNextLine() || match(Delimeters::RightBraces)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{token.filePath, token.value, token.line, token.column},
                    "ErrorManager.Syntax.MissingToken.noVariableAfter.message", {"throw"},
                    "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", {"throw"});
                if (isNextLine()) next();
                return nullptr;
            }

            auto expr = parseExpression();
            if (!expr) return nullptr;
            if (isNextLine()) next();

            auto node = ASTBuilder::createThrowStatement(std::move(expr));
            node->line = token.line; node->column = token.column; node->filePath = token.filePath;
            return node;
        }
        if (match(token, Keywords::Break)) {
            next();
            auto node = ASTBuilder::createBreakStatement();
            node->line = token.line; node->column = token.column; node->filePath = token.filePath;
            return node;
        }
        if (match(token, Keywords::Continue)) {
            next();
            auto node = ASTBuilder::createContinueStatement();
            node->line = token.line; node->column = token.column; node->filePath = token.filePath;
            return node;
        }

        // for modifier affected structures
        if (match(token, Keywords::Function)) return parseFunction(std::move(decorators), std::move(modifiers));
        if (match(token, Keywords::Class)) return parseClass(std::move(decorators), std::move(modifiers));
        if (match(token, Keywords::Enum)) return parseEnum(std::move(decorators), std::move(modifiers));
        if (match(token,  Keywords::Interface)) return parseInterface(std::move(decorators), std::move(modifiers));
        if (match(token, Keywords::Decorator)) return parseDecorator(std::move(decorators), std::move(modifiers));
        // FIXME: why is this here?
        if (!modifiers.empty()) {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::UnexpectedToken,
                ErrorSpan{token.filePath, token.value, token.line, token.column},
                "ErrorManager.Syntax.UnexpectedToken.message", {token.value},
                "ErrorManager.Syntax.UnexpectedToken.hint");
            return nullptr;
        }

        if (match(token, Keywords::Namespace)) return parseNamespace();
    }

    // === Block ===
    if (match(Delimeters::LeftBraces)) {
        return parseBlock();
    }

    // === Fallback ===
    return parseExpression(); // Default to expression statement
}

// ==== Expression parsing ====
// Note: does not require next(); after it
MemoryPtr<ASTNode> Parser::parseExpression() {
    Token token = curToken();

    // Unary operations
    if (om.find(token.value) != om.end()) {
        if (match(token, Operators::LogicalNot) || match(token, Operators::Subtract)) {
            next();
            return parseUnary(token.value);
        }
    }

    // Assignment
    if (match(TokenType::Identifier) && isAssignableAhead(0)) {
        return parseAssignment();
    }

    // Fallback to binary
    return parseBinary(0);
}

MemoryPtr<ASTNode> Parser::parseBinary(int prevPredecence) {
    MemoryPtr<ASTNode> left = parsePrimary();
    if (!left) return nullptr;

    while (true) {
        Token token = curToken();
        int predecence = getOperatorPrecedence(token.value);
        if (token.type != TokenType::Operator || predecence < prevPredecence) break;
        std::string op = token.value;
        next();

        MemoryPtr<ASTNode> right = parseBinary(predecence+1);
        if (!right) return nullptr;

        auto node = ASTBuilder::createBinaryOperation(std::move(left), op, std::move(right));
        node->line = token.line; node->column = token.column; node->filePath = token.filePath;
        left = std::move(node);
    }

    return left;
}

MemoryPtr<UnaryOperationNode> Parser::parseUnary(const std::string& op) {
    MemoryPtr<ASTNode> operand = parsePrimary();
    if (!operand) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{lookBack().filePath, lookBack().value, lookBack().line, lookBack().column},
            "ErrorManager.Syntax.MissingToken.missingOperandUnary.message", {op},
            "ErrorManager.Syntax.MissingToken.missingOperandUnary.hint", {op});
        return nullptr;
    }
    return ASTBuilder::createUnaryOperation(op, std::move(operand));
}

MemoryPtr<ASTNode> Parser::parsePrimary() {
    Token token = curToken();

    // Parenthesis, lambdas, arrays, sets, dicts
    if (match(TokenType::Delimeter)) {
        // Parenthesis / lambdas
        if (match(Delimeters::LeftParen)) {
            next();
            std::vector<MemoryPtr<ASTNode>> exprs;
            while (!match(Delimeters::RightParen)) {
                MemoryPtr<ASTNode> expr = parseExpression();
                if (!expr) {
                    errorManager->addError(
                        ErrorType::Syntax, SyntaxErrors::MissingToken,
                        ErrorSpan{token.filePath, token.value, token.line, token.column},
                        "ErrorManager.Syntax.MissingToken.missingExpressionLambda.message", {"("},
                        "ErrorManager.Syntax.MissingToken.missingExpressionLambda.hint");
                    return nullptr;
                }
                exprs.push_back(std::move(expr));
                if (curToken().type == TokenType::Delimeter && isNextLine()) next();
                if (match(Delimeters::Comma)) next();
                else break;
            }
            if (!match(Delimeters::RightParen)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{token.filePath, token.value, token.line, token.column},
                    "ErrorManager.Syntax.MissingToken.closingParen.message", {"("},
                    "ErrorManager.Syntax.MissingToken.closingParen.hint", {"("});
                return nullptr;
            }
            next();
            // Check for lambda expressions here
            if (match(Operators::AssignmentArrow)) {
                next();
                auto block = parseBlock();
                if (!block) {
                    errorManager->addError(
                        ErrorType::Syntax, SyntaxErrors::InvalidStatement,
                        ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                        "ErrorManager.Syntax.InvalidStatement.missedBlock.message", {"=>"},
                        "ErrorManager.Syntax.InvalidStatement.missedBlock.hint", {"=>", "=>"});
                    return nullptr;
                }
                return ASTBuilder::createLambda(std::move(exprs), std::move(block));
            }

            if (exprs.size() == 1) return std::move(exprs.front());
            return ASTBuilder::createTuple(std::move(exprs));
        }
        // Arrays
        if (match(Delimeters::LeftBracket)) {
            if (match(lookBack(), TokenType::Identifier) && (match(lookupNext(), TokenType::Delimeter) && match(lookupNext(), Delimeters::RightBracket))) next();
            else {
                // TODO: implement the strict types for arrays, sets, dicts?
                next();
                std::vector<MemoryPtr<ASTNode>> e;

                while (!match(Delimeters::RightBracket)) {
                    if (isAtEnd()) {
                        errorManager->addError(
                            ErrorType::Syntax, SyntaxErrors::MissingToken,
                            ErrorSpan{token.filePath, token.value, token.line, token.column},
                            "ErrorManager.Syntax.MissingToken.closingBracket.message", {},
                            "ErrorManager.Syntax.MissingToken.closingBracket.hint");
                        return nullptr;
                    }

                    auto element = parseExpression();
                    if (!element) return nullptr;

                    e.push_back(std::move(element));
                    if (curToken().type == TokenType::Delimeter && isNextLine()) next(); // To allow multiline expressions of arrays. I think this would be absolutely neat sugar for everybody.
                    if (match(Delimeters::Comma)) next();
                    else if (!match(Delimeters::RightBracket)) {
                        errorManager->addError(
                            ErrorType::Syntax, SyntaxErrors::MissingToken,
                            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                            "ErrorManager.Syntax.MissingToken.closingBracket.message", {},
                            "ErrorManager.Syntax.MissingToken.closingBracket.hint");
                        return nullptr;
                    }
                }
                next();
                return ASTBuilder::createArray(std::move(e));
            }
        }
        // Sets / dicts
        if (match(Delimeters::LeftBraces)) {
            // TODO next time: Allow dicts and sets to dictate their explicit type (when you come back)
            next();
            bool isDict = (match(lookupNext(), TokenType::Delimeter) && match(lookupNext(), Delimeters::Colon));

            if (isDict) {
                std::vector<std::pair<MemoryPtr<ASTNode>, MemoryPtr<ASTNode>>> e;
                while (!match(Delimeters::RightBraces)) {
                    if (isAtEnd()) {
                        errorManager->addError(
                            ErrorType::Syntax, SyntaxErrors::MissingToken,
                            ErrorSpan{token.filePath, token.value, token.line, token.column},
                            "ErrorManager.Syntax.MissingToken.closingBrace.message", {"dict"},
                            "ErrorManager.Syntax.MissingToken.closingBrace.hint");
                        return nullptr;
                    }

                    auto key = parseExpression();
                    if (!key) return nullptr;
                    if (!match(Delimeters::Colon)) {
                        errorManager->addError(
                            ErrorType::Syntax, SyntaxErrors::MissingToken,
                            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                            "ErrorManager.Syntax.MissingToken.dictColonAfterKey.message", {key->value},
                            "ErrorManager.Syntax.MissingToken.dictColonAfterKey.hint");
                        return nullptr;
                    }
                    next();
                    auto val = parseExpression();
                    if (!val) return nullptr;
                    e.push_back({std::move(key), std::move(val)});
                    if (match(Delimeters::Comma)) next();
                    else if (!match(Delimeters::RightBraces)) {
                        errorManager->addError(
                            ErrorType::Syntax, SyntaxErrors::MissingToken,
                            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                            "ErrorManager.Syntax.MissingToken.closingBrace.message", {"dict"},
                            "ErrorManager.Syntax.MissingToken.closingBrace.hint");
                        return nullptr;
                    }
                }
                next();
                return ASTBuilder::createDict(std::move(e));
            }

            std::vector<MemoryPtr<ASTNode>> e;
            while (!match(Delimeters::RightBraces)) {
                if (isAtEnd()) {
                    errorManager->addError(
                        ErrorType::Syntax, SyntaxErrors::MissingToken,
                        ErrorSpan{token.filePath, token.value, token.line, token.column},
                        "ErrorManager.Syntax.MissingToken.closingBrace.message", {"set"},
                        "ErrorManager.Syntax.MissingToken.closingBrace.hint");
                    return nullptr;
                }

                auto element = parseExpression();
                if (!element) return nullptr;

                e.push_back(std::move(element));
                if (match(Delimeters::Comma)) next();
                else if (!match(Delimeters::RightBraces)) {
                    errorManager->addError(
                        ErrorType::Syntax, SyntaxErrors::MissingToken,
                        ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                        "ErrorManager.Syntax.MissingToken.closingBrace.message", {"set"},
                        "ErrorManager.Syntax.MissingToken.closingBrace.hint");
                    return nullptr;
                }
            }
            next();
            return ASTBuilder::createSet(std::move(e));
        }
    }
    // Data type + Booleans
    else if ((match(TokenType::Number) || match(TokenType::String))
    || (match(TokenType::Identifier) && (token.value == "true" || token.value == "false"))) {
        next();
        return ASTBuilder::createLiteral(token.value);
    }
    // Null
    else if (match(TokenType::Null)) {
        next();
        return ASTBuilder::createLiteral("null");
    }

    // Identifier/variable or function call
    else if (match(TokenType::Identifier)) {
        Token id = next();
        MemoryPtr<ASTNode> node;

        // If function call
        if (match(Delimeters::LeftParen)) {
            next();
            std::vector<MemoryPtr<ASTNode>> args;

            while (!match(Delimeters::RightParen)) {
                auto arg = parseExpression();
                if (arg) args.push_back(std::move(arg));

                if (match(Delimeters::Comma)) next();
                else break;
            }
            if (!match(Delimeters::RightParen)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{token.filePath, token.value, token.line, token.column},
                    "ErrorManager.Syntax.MissingToken.closingParen.message", {token.value},
                    "ErrorManager.Syntax.MissingToken.closingParen.hint", {token.value});
                return nullptr;
            }
            next();

            auto callee = ASTBuilder::createVariable(id.value);
            node = ASTBuilder::createCallExpression(std::move(callee), std::move(args));
        } else {
            // else identifier/variable
            node = ASTBuilder::createVariable(id.value);
        }

        while (match(Delimeters::Dot)) {
            next();
            MemoryPtr<ASTNode> parent = std::move(node);

            if (!match(TokenType::Identifier)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::UnexpectedToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.UnexpectedToken.message", {curToken().value},
                    "ErrorManager.Syntax.UnexpectedToken.hint");
                return nullptr;
            }
            MemoryPtr<ASTNode> member = parsePrimary();
            if (!member) return nullptr;
            if (member->type != ASTNodeType::Variable && member->type != ASTNodeType::CallExpression) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::InvalidStatement,
                    ErrorSpan{member->filePath, member->value, member->line, member->column},
                    "ErrorManager.Syntax.InvalidStatement.message", {},
                    "ErrorManager.Syntax.InvalidStatement.hint");
                return nullptr;
            }
            node = ASTBuilder::createMemberAccess(std::move(parent), std::move(member));
        }
        node->line = id.line; node->column = id.column; node->filePath = id.filePath;
        return node;
    }

    errorManager->addError(
        ErrorType::Syntax, SyntaxErrors::UnexpectedToken,
        ErrorSpan{token.filePath, token.value, token.line, token.column},
        "ErrorManager.Syntax.UnexpectedToken.message", {token.value},
        "ErrorManager.Syntax.UnexpectedToken.hint");
    return nullptr;
}

std::optional<std::vector<GenericParameter>> Parser::parseGenericParameters(const std::string& ownerName) {
    std::vector<GenericParameter> parameters;

    if (!match(Operators::LessThan)) return parameters;

    Token openToken = curToken();
    next();

    if (match(Operators::GreaterThan)) {
        errorManager->addError(ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{openToken.filePath, openToken.value, openToken.line, openToken.column},
            "ErrorManager.Syntax.MissingToken.genericParameter.message",
            {ownerName},
            "ErrorManager.Syntax.MissingToken.genericParameter.hint");
        return std::nullopt;
    }

    while (!isAtEnd()) {
        Token nameToken = curToken();

        if (!match(TokenType::Identifier)) {
            errorManager->addError(ErrorType::Syntax, SyntaxErrors::MissingToken,
                ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
                "ErrorManager.Syntax.MissingToken.genericParameter.message",
                {ownerName},
                "ErrorManager.Syntax.MissingToken.genericParameter.hint");
            return std::nullopt;
        }

        for (const auto& parameter : parameters) {
            if (parameter.name == nameToken.value) {
                errorManager->addError(ErrorType::Syntax, SyntaxErrors::InvalidStatement,
                    ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
                    "ErrorManager.Syntax.InvalidStatement.duplicateGenericParameter.message",
                    {nameToken.value, ownerName},
                    "ErrorManager.Syntax.InvalidStatement.duplicateGenericParameter.hint");
                return std::nullopt;
            }
        }

        parameters.push_back(GenericParameter{
            .name = nameToken.value,
            .line = nameToken.line,
            .column = nameToken.column,
            .filePath = nameToken.filePath
        });

        next();

        if (match(Delimeters::Comma)) {
            next();

            if (match(Operators::GreaterThan)) {
                errorManager->addError(ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.genericParameter.message",
                    {ownerName},
                    "ErrorManager.Syntax.MissingToken.genericParameter.hint");
                return std::nullopt;
            }
            continue;
        }
        break;
    }

    if (!match(Operators::GreaterThan)) {
        errorManager->addError(ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.genericClosingAngle.message",
            {ownerName},
            "ErrorManager.Syntax.MissingToken.genericClosingAngle.hint");
        return std::nullopt;
    }
    next();

    return parameters;
}

std::optional<std::vector<MemoryPtr<RawTypeNode>>> Parser::parseGenericArguments(const std::string& typeName) {
    std::vector<MemoryPtr<RawTypeNode>> arguments;

    if (!match(Operators::LessThan)) return arguments;

    Token openToken = curToken();
    next();

    if (match(Operators::GreaterThan)) {
        errorManager->addError(ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{openToken.filePath, openToken.value, openToken.line, openToken.column},
            "ErrorManager.Syntax.MissingToken.genericTypeArgument.message",
            {typeName},
            "ErrorManager.Syntax.MissingToken.genericTypeArgument.hint"
        );
        return std::nullopt;
    }

    while (!isAtEnd()) {
        auto argument = parseType();

        if (!argument) {
            errorManager->addError(ErrorType::Syntax, SyntaxErrors::MissingToken,
                ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                "ErrorManager.Syntax.MissingToken.genericTypeArgument.message",
                {typeName},
                "ErrorManager.Syntax.MissingToken.genericTypeArgument.hint"
            );
            return std::nullopt;
        }

        arguments.push_back(std::move(argument));

        if (match(Delimeters::Comma)) {
            next();
            if (match(Operators::GreaterThan)) {
                errorManager->addError(ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.genericTypeArgument.message",
                    {typeName},
                    "ErrorManager.Syntax.MissingToken.genericTypeArgument.hint"
                );
                return std::nullopt;
            }
            continue;
        }
        break;
    }

    if (!match(Operators::GreaterThan)) {
        errorManager->addError(ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.genericClosingAngle.message",
            {typeName},
            "ErrorManager.Syntax.MissingToken.genericClosingAngle.hint");
        return std::nullopt;
    }
    next();

    return arguments;
}

MemoryPtr<RawTypeNode> Parser::parseType() {
    Token typeToken = curToken();

    if (!match(TokenType::Identifier)) {
        errorManager->addError(ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{typeToken.filePath, typeToken.value, typeToken.line, typeToken.column},
            "ErrorManager.Syntax.InvalidStatement.noType.message",
            {},
            "ErrorManager.Syntax.InvalidStatement.noType.hint");
        return nullptr;
    }

    std::string typeName = typeToken.value;
    MemoryPtr<VariableNode> varType = ASTBuilder::createVariable(typeName);
    next();

    auto result = parseGenericArguments(typeName);
    if (!result) return nullptr;

    std::vector<MemoryPtr<RawTypeNode>> genericArguments = std::move(*result);

    MemoryPtr<ASTNode> varSize = nullptr;

    if (match(Delimeters::LeftBracket)) {
        next();
        if (!match(Delimeters::RightBracket)) {
            varSize = parseExpression();
            if (!varSize) {
                errorManager->addError(ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.arraySize.message",
                    {typeName},
                    "ErrorManager.Syntax.MissingToken.arraySize.hint");
                return nullptr;
            }
        }

        if (!match(Delimeters::RightBracket)) {
            errorManager->addError(ErrorType::Syntax, SyntaxErrors::MissingToken,
                ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                "ErrorManager.Syntax.MissingToken.closingBracket.message",
                {},
                "ErrorManager.Syntax.MissingToken.closingBracket.hint");
            return nullptr;
        }
        next();
    }

    auto node = ASTBuilder::createRawType(std::move(varType), std::move(varSize), std::move(genericArguments));
    node->line = typeToken.line; node->column = typeToken.column; node->filePath = typeToken.filePath;
    return node;
}

MemoryPtr<DeclarationNode> Parser::parseDeclaration(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers){
    Token token = curToken();
    MemoryPtr<VariableNode> var = ASTBuilder::createVariable(token.value);
    next();

    bool isNullable = false;
    if (match(Operators::Nullable)) { isNullable = true; next(); }

    MemoryPtr<RawTypeNode> rawType = nullptr;
    if (!match(Delimeters::Colon)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{token.filePath, token.value, token.line, token.column},
            "ErrorManager.Syntax.MissingToken.colonAfterVar.message", {token.value},
            "ErrorManager.Syntax.MissingToken.colonAfterVar.hint");
        return nullptr;
    }
    next();

    bool isTypeInference = false;
    MemoryPtr<ASTNode> value = nullptr;
    if (match(Operators::Assign)) {
        isTypeInference = true;
        next();
        value = parseExpression();
        if (!value) {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::MissingToken,
                ErrorSpan{token.filePath, token.value, token.line, token.column},
                "ErrorManager.Syntax.MissingToken.noVariableAfter.message", {"="},
                "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", {"="});
            return nullptr;
        }
    } else {
        rawType = parseType();
        if (match(Operators::Assign)){
            next();
            value = parseExpression();
            if (!value) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{token.filePath, token.value, token.line, token.column},
                    "ErrorManager.Syntax.MissingToken.noVariableAfter.message", {"="},
                    "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", {"="});
                return nullptr;
            }
        }
    }

    auto node = ASTBuilder::createDeclaration(std::move(var), std::move(rawType), std::move(value), isNullable, isTypeInference, std::move(decorators), std::move(modifiers));
    node->line = token.line; node->column = token.column; node->filePath = token.filePath;
    return node;
}

MemoryPtr<AssignmentNode> Parser::parseAssignment() {

    MemoryPtr<ASTNode> var = parsePrimary();
    if (!var) return nullptr;
    if (var->type != ASTNodeType::Variable && var->type != ASTNodeType::MemberAccess) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::InvalidStatement,
            ErrorSpan{var->filePath, var->value, var->line, var->column},
            "ErrorManager.Syntax.InvalidStatement.message", {},
            "ErrorManager.Syntax.InvalidStatement.hint");
        return nullptr;
    }

    Token token = curToken();
    std::string op = token.value;
    next();

    MemoryPtr<ASTNode> value = parseExpression();
    if (!value) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{token.filePath, token.value, token.line, token.column},
            "ErrorManager.Syntax.MissingToken.noVariableAfter.message", {op},
            "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", {op});
        return nullptr;
    }

    auto node = ASTBuilder::createAssignment(std::move(var), op, std::move(value));
    node->line = token.line; node->column = token.column; node->filePath = token.filePath;
    return node;
}

// ==== Control flow ====
MemoryPtr<IfNode> Parser::parseIf() {
    auto token = curToken();
    next();

    if (!match(Delimeters::LeftParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{token.filePath, token.value, token.line, token.column},
            "ErrorManager.Syntax.MissingToken.openingParen.message", {"if"},
            "ErrorManager.Syntax.MissingToken.openingParen.hint", {"if"});
        return nullptr;
    }
    next(); // consume '('
    if (match(Delimeters::RightParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::InvalidStatement,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.InvalidStatement.emptyCondition.message", {"if"},
            "ErrorManager.Syntax.InvalidStatement.emptyCondition.hint", {"if"});
        next(); // consume ')'
        return nullptr;
    }

    MemoryPtr<ASTNode> condition = parseExpression();
    if (!condition) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::InvalidStatement,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.InvalidStatement.expectedCondition.message", {"if"},
            "ErrorManager.Syntax.InvalidStatement.expectedCondition.hint", {"if"});
        return nullptr;
    }
    if (!match(Delimeters::RightParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.closingParen.message", {"if"},
            "ErrorManager.Syntax.MissingToken.closingParen.hint", {"if"});
        return nullptr;
    }
    next();

    MemoryPtr<ASTNode> ifBlock = parseBlockorStatement();
    MemoryPtr<ASTNode> elseBlock = nullptr;

    if (match(Keywords::Else)) {
        next();
        elseBlock = parseBlockorStatement();
    }

    auto node = ASTBuilder::createIf(std::move(condition), std::move(ifBlock), std::move(elseBlock));
    node->line = token.line; node->column = token.column; node->filePath = token.filePath;
    return node;
}

MemoryPtr<SwitchNode> Parser::parseSwitch() {
    auto token = curToken();
    next();

    if (!match(Delimeters::LeftParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{token.filePath, token.value, token.line, token.column},
            "ErrorManager.Syntax.MissingToken.openingParen.message", {"switch"},
            "ErrorManager.Syntax.MissingToken.openingParen.hint", {"switch"});
        return nullptr;
    }
    next();

    MemoryPtr<ASTNode> expr = parseExpression();
    if (!expr) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::InvalidStatement,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.InvalidStatement.expectedCondition.message", {"switch"},
            "ErrorManager.Syntax.InvalidStatement.expectedCondition.hint", {"switch"});
        return nullptr;
    }

    if (!match(Delimeters::RightParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.closingParen.message", {"switch"},
            "ErrorManager.Syntax.MissingToken.closingParen.hint", {"switch"});
        return nullptr;
    }
    next();

    if (!match(Delimeters::LeftBraces)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.openingBrace.message", {"switch"},
            "ErrorManager.Syntax.MissingToken.openingBrace.hint");
        return nullptr;
    }
    next();
    while (match(Delimeters::Semicolon)) next();

    std::vector<MemoryPtr<CaseNode>> cases;
    MemoryPtr<SCDefaultNode> defaultCase = nullptr;

    while (!match(Delimeters::RightBraces)) {
        Token tok = curToken();

        if (match(Keywords::Case)) {
            next(); // consume 'case'
            MemoryPtr<ASTNode> condition = parseExpression();
            if (!match(Delimeters::Colon)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.colonInCase.message", {},
                    "ErrorManager.Syntax.MissingToken.colonInCase.hint");
                return nullptr;
            }
            next(); // consume ':'
            auto body = parseBlockorStatement();
            if (!body) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::InvalidStatement,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.InvalidStatement.expectedBody.message", {"case"},
                    "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", {"case"});
                return nullptr;
            }
            while (match(Delimeters::Semicolon)) next();
            cases.push_back(ASTBuilder::createCase(std::move(condition), std::move(body)));
        }
        else if (match(Keywords::Default)) {
            next();
            if (!match(Delimeters::Colon)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.colonInDefault.message", {},
                    "ErrorManager.Syntax.MissingToken.colonInDefault.hint");
                return nullptr;
            }
            next();
            auto body = parseBlockorStatement();
            if (!body) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::InvalidStatement,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.InvalidStatement.expectedBody.message", {"default"},
                    "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", {"default"});
                return nullptr;
            }
            while (match(Delimeters::Semicolon)) next();
            defaultCase = ASTBuilder::createDefaultCase(std::move(body));
        }
        else if (isNextLine()) next();
        else {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::UnexpectedToken,
                ErrorSpan{tok.filePath, tok.value, tok.line, tok.column},
                "ErrorManager.Syntax.InvalidStatement.unexpectedInSwitch.message", {tok.value},
                "ErrorManager.Syntax.InvalidStatement.unexpectedInSwitch.hint");
            return nullptr;
        }
    }
    if (!match(Delimeters::RightBraces)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.closingBrace.message", {"switch"},
            "ErrorManager.Syntax.MissingToken.closingBrace.hint");
        return nullptr;
    }
    next();
    auto node = ASTBuilder::createSwitch(std::move(expr), std::move(cases), std::move(defaultCase));
    node->line = token.line; node->column = token.column; node->filePath = token.filePath;
    return node;
};

MemoryPtr<TryCatchNode> Parser::parseTryCatch() {
    auto token = curToken();
    next();

    auto tryBlock = parseBlock();
    if (!tryBlock) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::InvalidStatement,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.InvalidStatement.expectedBody.message", {"try"},
            "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", {"try"});
        return nullptr;
    }

    if (!match(Keywords::Catch)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.catchAfterTry.message", {},
            "ErrorManager.Syntax.MissingToken.catchAfterTry.hint");
        return nullptr;
    }
    next();

    if (!match(Delimeters::LeftParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.openingParen.message", {"catch"},
            "ErrorManager.Syntax.MissingToken.openingParen.hint", {"catch"});
        return nullptr;
    }
    next();

    if (!match(TokenType::Identifier)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.exceptionVar.message", {},
            "ErrorManager.Syntax.MissingToken.exceptionVar.hint");
        return nullptr;
    }
    auto varName = curToken().value;
    auto exception = ASTBuilder::createVariable(varName);
    next();

    if (!match(Delimeters::RightParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.closingParen.message", {"catch"},
            "ErrorManager.Syntax.MissingToken.closingParen.hint", {"catch"});
        return nullptr;
    }
    next();

    auto catchBlock = parseBlock();
    if (!catchBlock) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::InvalidStatement,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.InvalidStatement.expectedBody.message", {"catch"},
            "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", {"catch"});
        return nullptr;
    }

    auto node = ASTBuilder::createTryCatch(std::move(tryBlock), std::move(exception), std::move(catchBlock));
    node->line = token.line; node->column = token.column; node->filePath = token.filePath;
    return node;
}

MemoryPtr<ForLoopNode> Parser::parseFor() {
    auto token = curToken();
    next();

    if (!match(Delimeters::LeftParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{token.filePath, token.value, token.line, token.column},
            "ErrorManager.Syntax.MissingToken.openingParen.message", {"for"},
            "ErrorManager.Syntax.MissingToken.openingParen.hint", {"for"});
        return nullptr;
    }
    next();

    if (curToken().type != TokenType::Identifier) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.noVariableAfter.message", {"for ("},
            "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", {"for ("});
        return nullptr;
    }

    std::string varName = curToken().value;
    auto varNode = ASTBuilder::createVariable(varName);
    next();

    if (!match(Delimeters::Colon)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.colonInFor.message", {varName},
            "ErrorManager.Syntax.MissingToken.colonInFor.hint");
        return nullptr;
    }
    next();

    MemoryPtr<ASTNode> iterable = parseExpression();
    if (!iterable) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::InvalidStatement,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.InvalidStatement.expectedIterable.message", {},
            "ErrorManager.Syntax.InvalidStatement.expectedIterable.hint");
        return nullptr;
    }

    if (!match(Delimeters::RightParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.closingParen.message", {"for"},
            "ErrorManager.Syntax.MissingToken.closingParen.hint", {"for"});
        return nullptr;
    }
    next();

    auto body = parseBlock();
    if (!body) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::InvalidStatement,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.InvalidStatement.expectedBody.message", {"for"},
            "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", {"for"});
        return nullptr;
    }

    auto node = ASTBuilder::createForLoop(std::move(varNode), std::move(iterable), std::move(body));
    node->line = token.line; node->column = token.column; node->filePath = token.filePath;
    return node;
}

MemoryPtr<WhileLoopNode> Parser::parseWhile() {
    auto token = curToken();
    next();

    if (!match(Delimeters::LeftParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{token.filePath, token.value, token.line, token.column},
            "ErrorManager.Syntax.MissingToken.openingParen.message", {"while"},
            "ErrorManager.Syntax.MissingToken.openingParen.hint", {"while"});
        return nullptr;
    }
    next();
    MemoryPtr<ASTNode> condition = parseExpression();
    if (!condition) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::InvalidStatement,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.InvalidStatement.expectedCondition.message", {"while"},
            "ErrorManager.Syntax.InvalidStatement.expectedCondition.hint", {"while"});
        return nullptr;
    }
    if (!match(Delimeters::RightParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.closingParen.message", {"while"},
            "ErrorManager.Syntax.MissingToken.closingParen.hint", {"while"});
        return nullptr;
    }
    next();

    auto body = parseBlock();
    if (!body) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::InvalidStatement,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.InvalidStatement.expectedBody.message", {"while"},
            "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", {"while"});
        return nullptr;
    }

    auto node = ASTBuilder::createWhileLoop(std::move(condition), std::move(body));
    node->line = token.line; node->column = token.column; node->filePath = token.filePath;
    return node;
}


// ==== Declarations ====
MemoryPtr<FunctionNode> Parser::parseFunction(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers) {
    next();

    Token nameToken = curToken();
    if (!match(TokenType::Identifier)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
            "ErrorManager.Syntax.MissingToken.functionName.message", {},
            "ErrorManager.Syntax.MissingToken.functionName.hint");
        return nullptr;
    }
    std::string funcName = nameToken.value;
    next();

    if (!match(Delimeters::LeftParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
            "ErrorManager.Syntax.MissingToken.functionParams.message", {funcName},
            "ErrorManager.Syntax.MissingToken.functionParams.hint", {funcName});
        return nullptr;
    }
    next();

    std::vector<MemoryPtr<ParameterNode>> params;
    while (!match(Delimeters::RightParen)) {
        Token paramName = curToken();
        if (paramName.type != TokenType::Identifier) {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::MissingToken,
                ErrorSpan{paramName.filePath, paramName.value, paramName.line, paramName.column},
                "ErrorManager.Syntax.MissingToken.functionParamName.message", {funcName},
                "ErrorManager.Syntax.MissingToken.functionParamName.hint");
            return nullptr;
        }
        next();
        MemoryPtr<RawTypeNode> type = nullptr;
        if (match(Delimeters::Colon)) {
            next();
            type = parseType();
        }

        MemoryPtr<ASTNode> defaultValue = nullptr;
        if (match(Operators::Assign)) {
            next();
            defaultValue = parseExpression();
            if (!defaultValue) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.functionParamDefault.message", {},
                    "ErrorManager.Syntax.MissingToken.functionParamDefault.hint");
                return nullptr;
            }
        }
        auto param = ASTBuilder::createParameter(paramName.value, std::move(type), std::move(defaultValue));
        param->line = paramName.line; param->column = paramName.column; param->filePath = paramName.filePath;
        params.push_back(std::move(param));
        if (match(Delimeters::Comma)) next();
        else break;
    }

    if (!match(Delimeters::RightParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.functionClosingParen.message", {funcName},
            "ErrorManager.Syntax.MissingToken.functionClosingParen.hint");
        return nullptr;
    }
    next();

    MemoryPtr<RawTypeNode> returnType = nullptr;
    if (match(Operators::TypeArrow)) {
        next();
        returnType = parseType();
    }

    bool isIntrinsic = false;
    for (const auto& modifier : modifiers) {
        if (modifier && modifier->modifier == ASTModifierType::Intrinsic) {
            isIntrinsic = true;
            break;
        }
    }

    MemoryPtr<BlockNode> body = nullptr;

    if (isIntrinsic) {
        while (isNextLine()) next();

        if (match(Delimeters::RightBraces) || isAtEnd()) {} // не понял че тут
        else if (match(Delimeters::LeftBraces)) {
            errorManager->addError(ErrorType::Syntax, SyntaxErrors::InvalidStatement,
                ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                "ErrorManager.Syntax.InvalidStatement.intrinsicBody.message",
                {funcName},
                "ErrorManager.Syntax.InvalidStatement.intrinsicBody.hint");
            return nullptr;
        }
    }
    else {
        body = parseBlock();
        if (!body) {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::InvalidStatement,
                ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
                "ErrorManager.Syntax.MissingToken.functionBody.message", {funcName},
                "ErrorManager.Syntax.MissingToken.functionBody.hint", {funcName});
            return nullptr;
        }
    }

    auto node = ASTBuilder::createFunction(funcName, std::move(params), std::move(returnType), std::move(body), std::move(decorators), std::move(modifiers));
    node->line = nameToken.line; node->column = nameToken.column; node->filePath = nameToken.filePath;
    node->isIntrinsic = isIntrinsic;
    return node;
}

MemoryPtr<ClassNode> Parser::parseClass(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers) {
    next();

    Token nameToken = curToken();
    if (!match(TokenType::Identifier)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
            "ErrorManager.Syntax.MissingToken.className.message", {},
            "ErrorManager.Syntax.MissingToken.className.hint");
        return nullptr;
    }
    std::string className = nameToken.value;
    next();

    // Checking if class is inherited
    MemoryPtr<VariableNode> super = nullptr;
    if (match(Operators::InheritanceArrow)) {
        next();
        super = ASTBuilder::createVariable(curToken().value);
        next();
    }

    if (!match(Delimeters::LeftBraces)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
            "ErrorManager.Syntax.MissingToken.classBody.message", {className},
            "ErrorManager.Syntax.MissingToken.classBody.hint");
        return nullptr;
    }
    next();
    while (match(Delimeters::Semicolon)) next();

    std::vector<MemoryPtr<DeclarationNode>> fields;
    std::vector<MemoryPtr<FunctionNode>> methods;
    MemoryPtr<FunctionNode> constructor = nullptr;

    while (!match(Delimeters::RightBraces)) {
        Token token = curToken();

        std::vector<MemoryPtr<CallExpressionNode>> decs = {};
        if (match(TokenType::Decorator)) decs = parseDecoratorCalls();
        auto modifs = parseModifiers();

        if (match(TokenType::Identifier, className)) {
            auto cfilePath = curToken().filePath; auto cLine = curToken().line; auto cColumn = curToken().column;
            constructor = parseConstructor(std::move(decs), std::move(modifs));
            if (!constructor) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::InvalidStatement,
                    ErrorSpan{cfilePath, curToken().value, cLine, cColumn},
                    "ErrorManager.Syntax.InvalidStatement.constructorFailed.message", {className},
                    "ErrorManager.Syntax.InvalidStatement.constructorFailed.hint");
                return nullptr;
            }
        }
        else if (match(Keywords::Function)) {
            auto method = parseFunction(std::move(decs), std::move(modifs));
            if (method) methods.push_back(std::move(method));
        }
        else if (match(token, TokenType::Identifier) && (match(lookupNext(), TokenType::Delimeter) && match(lookupNext(), Delimeters::Colon))) {
            MemoryPtr<DeclarationNode> decl = parseDeclaration(std::move(decs), std::move(modifs));
            fields.push_back(std::move(decl));
            if (isNextLine()) next();
        }
        else if (isNextLine()) next();
        else {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::UnexpectedToken,
                ErrorSpan{token.filePath, token.value, token.line, token.column},
                "ErrorManager.Syntax.InvalidStatement.unexpectedInClass.message", {token.value},
                "ErrorManager.Syntax.InvalidStatement.unexpectedInClass.hint");
            break;
        }
    }

    next();
    auto node = ASTBuilder::createClass(className, std::move(constructor), std::move(super), std::move(fields), std::move(methods), std::move(decorators), std::move(modifiers));
    node->line = nameToken.line; node->column = nameToken.column; node->filePath = nameToken.filePath;
    return node;
}

MemoryPtr<BlockNode> Parser::parseBlock() {
    if (!match(Delimeters::LeftBraces)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.openingBrace.message", {"block"},
            "ErrorManager.Syntax.MissingToken.openingBrace.hint");
        return nullptr;
    }
    next();
    while (isNextLine()) next();

    std::vector<MemoryPtr<ASTNode>> block = {};

    while (!isAtEnd()) {
        while (isNextLine()) next();

        if (match(Delimeters::RightBraces)) {
            break;
        }

        MemoryPtr<ASTNode> stmt = parseStatement();
        if (!stmt && match(Delimeters::RightBraces)) {
            break;
        }
        if (!stmt) {

            while (!isAtEnd() && !isNextLine()) next();

            size_t startPos = pos;
            int guard = 0;
            while (!isAtEnd() && guard++ < 100){
                if (isNextLine()){ next(); break; }
                if (match(Delimeters::RightBraces)){ break; }
                next();
            }

            if (pos == startPos && !isAtEnd()) next();

            if (guard >= 100) {
                break;
            }

            if (isAtEnd()) break;
            continue;
        }
        block.push_back(std::move(stmt));
        while (isNextLine()) next();
    }
    if (!match(Delimeters::RightBraces)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.closingBrace.message", {"block"},
            "ErrorManager.Syntax.MissingToken.closingBrace.hint");
        return nullptr;
    }
    next();
    return ASTBuilder::createBlock(std::move(block));
}

MemoryPtr<ASTNode> Parser::parseDecorator(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers, bool isCall) {
    Token nameToken = curToken();
    MemoryPtr<ASTNode> node;

    if (!isCall) {
        next();
        if (!match(TokenType::Identifier)) {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::MissingToken,
                ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
                "ErrorManager.Syntax.MissingToken.decoratorName.message", {},
                "ErrorManager.Syntax.MissingToken.decoratorName.hint");
            return nullptr;
        }
        std::string name = curToken().value;
        next();

        std::vector<MemoryPtr<ParameterNode>> params;
        if (match(Delimeters::LeftParen)) {
            next();
            while (!match(Delimeters::RightParen)) {
                if (!match(TokenType::Identifier)) {
                    errorManager->addError(
                        ErrorType::Syntax, SyntaxErrors::MissingToken,
                        ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                        "ErrorManager.Syntax.MissingToken.decoratorParamName.message", {name},
                        "ErrorManager.Syntax.MissingToken.decoratorParamName.hint");
                    return nullptr;
                }

                Token tok = curToken();
                std::string paramName = tok.value;
                next();

                MemoryPtr<RawTypeNode> type = nullptr;
                if (match(Delimeters::Colon)) {
                    next();
                    type = parseType();
                }

                MemoryPtr<ASTNode> defaultValue = nullptr;
                if (match(Operators::Assign)) {
                    next();
                    defaultValue = parseExpression();
                    if (!defaultValue) {
                        errorManager->addError(
                            ErrorType::Syntax, SyntaxErrors::MissingToken,
                            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                            "ErrorManager.Syntax.MissingToken.decoratorParamDefault.message", {},
                            "ErrorManager.Syntax.MissingToken.decoratorParamDefault.hint");
                        return nullptr;
                    }
                }

                params.push_back(ASTBuilder::createParameter(paramName, std::move(type), std::move(defaultValue)));
                if (match(Delimeters::Comma)) next();
                else break;
            }

            if (!match(Delimeters::RightParen)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.decoratorClosingParen.message", {name},
                    "ErrorManager.Syntax.MissingToken.decoratorClosingParen.hint");
                return nullptr;
            }
            next();
        }

        auto block = parseBlock();
        if (!block) {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::InvalidStatement,
                ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
                "ErrorManager.Syntax.MissingToken.decoratorBody.message", {name},
                "ErrorManager.Syntax.MissingToken.decoratorBody.hint");
            return nullptr;
        }
        node = ASTBuilder::createDecorator(name, std::move(params), std::move(block), std::move(decorators), std::move(modifiers));
        node->line = nameToken.line; node->column = nameToken.column; node->filePath = nameToken.filePath;
    } else {
        std::string name = nameToken.value;
        next();

        std::vector<MemoryPtr<ASTNode>> args;
        if (match(Delimeters::LeftParen)) {
            next();
            while (!match(Delimeters::RightParen)) {
                auto arg = parseExpression();
                if (arg) args.push_back(std::move(arg));

                if (match(Delimeters::Comma)) next();
                else break;
            }
            if (!match(Delimeters::RightParen)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
                    "ErrorManager.Syntax.MissingToken.decoratorClosingParen.message", {name},
                    "ErrorManager.Syntax.MissingToken.decoratorClosingParen.hint");
                return nullptr;
            }
            next();
        }
        node = ASTBuilder::createCallExpression(ASTBuilder::createVariable(name), std::move(args), true);
        node->line = nameToken.line; node->column = nameToken.column; node->filePath = nameToken.filePath;
    }
    return node;
}

std::vector<MemoryPtr<CallExpressionNode>> Parser::parseDecoratorCalls() {
    std::vector<MemoryPtr<CallExpressionNode>> calls;
    while (match(TokenType::Decorator)) {
        auto node = parseDecorator({}, {}, true);
        if (!node) break; // error already reported by Lexer::parseDecorator
        auto call = as<CallExpressionNode>(std::move(node));
        if (!call) {
            // [internal]
            break;
        }
        calls.push_back(std::move(call));
    }

    return calls;
}

std::vector<MemoryPtr<ModifierNode>> Parser::parseModifiers() {
    std::vector<MemoryPtr<ModifierNode>> modifiers;

    while (curToken().type == TokenType::Keyword) {
        if (match(Keywords::Static))
            modifiers.push_back(ASTBuilder::createModifier(ASTModifierType::Static));
        else if (match(Keywords::Const))
            modifiers.push_back(ASTBuilder::createModifier(ASTModifierType::Const));
        else if (match(Keywords::Public))
            modifiers.push_back(ASTBuilder::createModifier(ASTModifierType::Public));
        else if (match(Keywords::Protected))
            modifiers.push_back(ASTBuilder::createModifier(ASTModifierType::Protected));
        else if (match(Keywords::Private))
            modifiers.push_back(ASTBuilder::createModifier(ASTModifierType::Private));
        else if (match(Keywords::Override))
            modifiers.push_back(ASTBuilder::createModifier(ASTModifierType::Override));
        else if (match(Keywords::Async))
            modifiers.push_back(ASTBuilder::createModifier(ASTModifierType::Async));
        else if (match(Keywords::Debug))
            modifiers.push_back(ASTBuilder::createModifier(ASTModifierType::Debug));
        else if (match(Keywords::Intrinsic))
            modifiers.push_back(ASTBuilder::createModifier(ASTModifierType::Intrinsic));
        else break;
        next();
    }

    return modifiers;
}

MemoryPtr<ASTNode> Parser::parsePreprocessor() {
    Token token = curToken();
    MemoryPtr<ASTNode> node;

    if (match(Preprocessors::Import)) {
        next();
        if (!match(TokenType::String)) {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::MissingToken,
                ErrorSpan{token.filePath, token.value, token.line, token.column},
                "ErrorManager.Syntax.MissingToken.importTarget.message", {},
                "ErrorManager.Syntax.MissingToken.importTarget.hint");
            return nullptr;
        }
        auto moduleStr = curToken().value;
        ASTImportType importType = ASTImportType::Native;
        if ((moduleStr.contains("/") || moduleStr.contains(".")) && moduleStr.contains(":")) importType = ASTImportType::ForeignRelative;
        else if (moduleStr.contains("/") || moduleStr.contains(".")) importType = ASTImportType::Relative;
        else if (moduleStr.contains(":")) importType = ASTImportType::Foreign;

        std::string alias;
        next();
        if (match(Keywords::As)) {
            next();
            if (!match(TokenType::Identifier)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.importAlias.message", {},
                    "ErrorManager.Syntax.MissingToken.importAlias.hint");
                return nullptr;
            }
            alias = curToken().value;
            next();
        }
        node = ASTBuilder::createImport(moduleStr, alias, importType);
    }
    else if (match(Preprocessors::Macro)) {
        next();
        if (!match(TokenType::Identifier)) {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::MissingToken,
                ErrorSpan{token.filePath, token.value, token.line, token.column},
                "ErrorManager.Syntax.MissingToken.macroIdentifier.message", {},
                "ErrorManager.Syntax.MissingToken.macroIdentifier.hint");
            return nullptr;
        }
        next();
        std::string value;
        while (!isNextLine()) {
            value += curToken().value;
            next();
        }
        node = ASTBuilder::createPreprocessor(ASTPreprocessorDirectiveType::Macro, value);
    }
    else if (match(Preprocessors::Unsafe)) {
        next();
        node = ASTBuilder::createPreprocessor(ASTPreprocessorDirectiveType::Unsafe);
    }
    else node = ASTBuilder::createPreprocessor(ASTPreprocessorDirectiveType::None);
    node->line = token.line; node->column = token.column; node->filePath = token.filePath;
    return node;
}

MemoryPtr<EnumNode> Parser::parseEnum(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers) {
    next();
    if (!match(TokenType::Identifier)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.enumName.message", {},
            "ErrorManager.Syntax.MissingToken.enumName.hint");
        return nullptr;
    }
    auto enumToken = curToken();
    next();
    if (!match(Delimeters::LeftBraces)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{enumToken.filePath, enumToken.value, enumToken.line, enumToken.column},
            "ErrorManager.Syntax.MissingToken.enumBody.message", {enumToken.value},
            "ErrorManager.Syntax.MissingToken.enumBody.hint");
        return nullptr;
    }
    next();
    while (isNextLine()) next();

    std::vector<MemoryPtr<EnumMemberNode>> elements;

    while (!match(Delimeters::RightBraces)) {
        while (isNextLine()) next();
        if (match(Delimeters::RightBraces)) break;

        if (!match(TokenType::Identifier)) {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::UnexpectedToken,
                ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                "ErrorManager.Syntax.InvalidStatement.unexpectedInEnum.message", {curToken().value},
                "ErrorManager.Syntax.InvalidStatement.unexpectedInEnum.hint");
            return nullptr;
        }
        auto name = curToken().value;
        MemoryPtr<LiteralNode> value = nullptr;

        next();
        if (match(Operators::Assign)) {
            next();
            auto tmp = parsePrimary();
            if (!tmp || tmp->type != ASTNodeType::Literal) {
                errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::InvalidStatement,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.InvalidStatement.enumNonLiteralValue.message", {name},
                    "ErrorManager.Syntax.InvalidStatement.enumNonLiteralValue.hint");
                return nullptr;
            }
            value = as<LiteralNode>(std::move(tmp));
        }
        elements.push_back(ASTBuilder::createEnumMember(name, std::move(value)));

        if (match(Delimeters::Comma)) next();
        else if (isNextLine()) next();
        else if (!match(Delimeters::RightBraces)) {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::UnexpectedToken,
                ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                "ErrorManager.Syntax.InvalidStatement.enumDelimiter.message", {curToken().value},
                "ErrorManager.Syntax.InvalidStatement.enumDelimiter.hint");
            return nullptr;
        }
        else break;
    }
    if (!match(Delimeters::RightBraces)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.enumClosingBrace.message", {enumToken.value},
            "ErrorManager.Syntax.MissingToken.enumClosingBrace.hint");
        return nullptr;
    }
    next();

    auto node = ASTBuilder::createEnum(enumToken.value, std::move(elements), std::move(decorators), std::move(modifiers));
    node->line = enumToken.line; node->column = enumToken.column; node->filePath = enumToken.filePath;
    return node;
}

MemoryPtr<InterfaceNode> Parser::parseInterface(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers) {
    next();

    if (!match(TokenType::Identifier)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.interfaceName.message", {},
            "ErrorManager.Syntax.MissingToken.interfaceName.hint");
        return nullptr;
    }
    auto interfaceToken = curToken();
    next();
    if (!match(Delimeters::LeftBraces)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{interfaceToken.filePath, interfaceToken.value, interfaceToken.line, interfaceToken.column},
            "ErrorManager.Syntax.MissingToken.interfaceBody.message", {interfaceToken.value},
            "ErrorManager.Syntax.MissingToken.interfaceBody.hint");
        return nullptr;
    }
    next();
    while (isNextLine()) next();

    std::vector<MemoryPtr<InterfaceFieldNode>> elements;
    std::vector<MemoryPtr<ParameterNode>> params;

    while (!match(Delimeters::RightBraces)) {
        if (curToken().type == TokenType::Identifier) {
            auto name = curToken().value;
            bool isNullable = false;
            next();
            if (match(Operators::Nullable)) {
                isNullable = true;
                next();
            }

            if (!match(Delimeters::Colon)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.colonInInterface.message", {name},
                    "ErrorManager.Syntax.MissingToken.colonInInterface.hint");
                return nullptr;
            }
            next();

            MemoryPtr<RawTypeNode> rawType = parseType();
            elements.push_back(ASTBuilder::createInterfaceField(name, std::move(rawType), isNullable));

            if (isNextLine()) next();
            else if (!match(Delimeters::RightBraces)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::UnexpectedToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.InvalidStatement.interfaceDelimiter.message", {curToken().value},
                    "ErrorManager.Syntax.InvalidStatement.interfaceDelimiter.hint");
                return nullptr;
            }
            else break;
        } else if (match(Keywords::Function)) {
            next();
            if (curToken().type != TokenType::Identifier) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.interfaceMethodName.message", {interfaceToken.value},
                    "ErrorManager.Syntax.MissingToken.interfaceMethodName.hint");
                return nullptr;
            }
            std::string methodName = curToken().value;
            next();
            if (!match(Delimeters::LeftParen)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.interfaceMethodParams.message", {methodName},
                    "ErrorManager.Syntax.MissingToken.interfaceMethodParams.hint");
                return nullptr;
            }
            next();
            while (!match(Delimeters::RightParen)) {
                Token token = curToken();
                if (!match(TokenType::Identifier)) {
                    errorManager->addError(
                        ErrorType::Syntax, SyntaxErrors::MissingToken,
                        ErrorSpan{token.filePath, token.value, token.line, token.column},
                        "ErrorManager.Syntax.MissingToken.interfaceMethodParamName.message", {methodName},
                        "ErrorManager.Syntax.MissingToken.interfaceMethodParamName.hint");
                    return nullptr;
                }
                std::string paramName = token.value;
                next();
                MemoryPtr<RawTypeNode> type = nullptr;
                if (match(Delimeters::Colon)) {
                    next();
                    type = parseType();
                }
                params.push_back(ASTBuilder::createParameter(paramName, std::move(type), nullptr));
                if (match(Delimeters::Comma)) next();
                else break;
            }
            if (!match(Delimeters::RightParen)) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.interfaceMethodClosingParen.message", {methodName},
                    "ErrorManager.Syntax.MissingToken.interfaceMethodClosingParen.hint");
                return nullptr;
            }
            next();
            MemoryPtr<VariableNode> returnType = nullptr;
            if (match(Operators::TypeArrow)) {
                next();
                if (curToken().type != TokenType::Identifier) {
                    errorManager->addError(
                        ErrorType::Syntax, SyntaxErrors::MissingToken,
                        ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                        "ErrorManager.Syntax.MissingToken.interfaceReturnType.message", {methodName},
                        "ErrorManager.Syntax.MissingToken.interfaceReturnType.hint");
                    return nullptr;
                }
                returnType = ASTBuilder::createVariable(curToken().value);
                next();
            }
            elements.push_back(ASTBuilder::createInterfaceField(methodName, nullptr, false, true, std::move(params), std::move(returnType)));
            params.clear();
            if (isNextLine()) next();
        } else {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::UnexpectedToken,
                ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                "ErrorManager.Syntax.InvalidStatement.unexpectedInInterface.message", {curToken().value},
                "ErrorManager.Syntax.InvalidStatement.unexpectedInInterface.hint");
            return nullptr;
        }
    }
    if (!match(Delimeters::RightBraces)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.interfaceClosingBrace.message", {interfaceToken.value},
            "ErrorManager.Syntax.MissingToken.interfaceClosingBrace.hint");
        return nullptr;
    }
    next();

    auto node = ASTBuilder::createInterface(interfaceToken.value, std::move(elements), std::move(decorators), std::move(modifiers));
    node->line = interfaceToken.line; node->column = interfaceToken.column; node->filePath = interfaceToken.filePath;
    return node;
}

MemoryPtr<NamespaceNode> Parser::parseNamespace() {
    next();
    auto token = curToken();

    auto name = parsePrimary();
    if (!name || (name->type != ASTNodeType::Variable && name->type != ASTNodeType::MemberAccess)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.namespaceName.message", {},
            "ErrorManager.Syntax.MissingToken.namespaceName.hint"
        );
        return nullptr;
    }

    std::string namespaceName = namespaceNameToString(name.get());

    auto body = parseBlock();
    if (!body) return nullptr;

    auto node = ASTBuilder::createNamespace(std::move(name), std::move(body->statements));
    node->value = namespaceName; node->filePath = token.filePath; node->line = token.line; node->column = token.column;
    return node;
}

// ==== Helper functions ====

// Parses either a block or a single statement after an 'if' condition.
MemoryPtr<ASTNode> Parser::parseBlockorStatement() {

    if (match(Delimeters::LeftBraces)) {
        MemoryPtr<BlockNode> block = parseBlock();
        return std::move(block);
    }

    MemoryPtr<ASTNode> block = parseStatement();
    if(!block) return nullptr;
    return std::move(block);
}

MemoryPtr<FunctionNode> Parser::parseConstructor(std::vector<MemoryPtr<CallExpressionNode>> decorators, std::vector<MemoryPtr<ModifierNode>> modifiers) {
    Token nameToken = curToken();
    if (!match(TokenType::Identifier)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
            "ErrorManager.Syntax.MissingToken.functionName.message", {},
            "ErrorManager.Syntax.MissingToken.functionName.hint");
        return nullptr;
    }
    std::string funcName = nameToken.value;
    next();

    if (!match(Delimeters::LeftParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
            "ErrorManager.Syntax.MissingToken.functionParams.message", {funcName},
            "ErrorManager.Syntax.MissingToken.functionParams.hint", {funcName});
        return nullptr;
    }
    next();

    std::vector<MemoryPtr<ParameterNode>> params;
    while (!match(Delimeters::RightParen)) {
        Token paramName = curToken();
        if (paramName.type != TokenType::Identifier) {
            errorManager->addError(
                ErrorType::Syntax, SyntaxErrors::MissingToken,
                ErrorSpan{paramName.filePath, paramName.value, paramName.line, paramName.column},
                "ErrorManager.Syntax.MissingToken.functionParamName.message", {funcName},
                "ErrorManager.Syntax.MissingToken.functionParamName.hint");
            return nullptr;
        }
        next();
        MemoryPtr<RawTypeNode> type = nullptr;
        if (match(Delimeters::Colon)) {
            next();
            type = parseType();
        }

        MemoryPtr<ASTNode> defaultValue = nullptr;
        if (match(Operators::Assign)) {
            next();
            defaultValue = parseExpression();
            if (!defaultValue) {
                errorManager->addError(
                    ErrorType::Syntax, SyntaxErrors::MissingToken,
                    ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
                    "ErrorManager.Syntax.MissingToken.functionParamDefault.message", {},
                    "ErrorManager.Syntax.MissingToken.functionParamDefault.hint");
                return nullptr;
            }
        }
        auto param = ASTBuilder::createParameter(paramName.value, std::move(type), std::move(defaultValue));
        param->line = paramName.line; param->column = paramName.column; param->filePath = paramName.filePath;
        params.push_back(std::move(param));
        if (match(Delimeters::Comma)) next();
        else break;
    }

    if (!match(Delimeters::RightParen)) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::MissingToken,
            ErrorSpan{curToken().filePath, curToken().value, curToken().line, curToken().column},
            "ErrorManager.Syntax.MissingToken.functionClosingParen.message", {funcName},
            "ErrorManager.Syntax.MissingToken.functionClosingParen.hint");
        return nullptr;
    }
    next();

    MemoryPtr<RawTypeNode> returnType = nullptr;
    if (match(Operators::TypeArrow)) {
        next();
        returnType = parseType();
    }

    MemoryPtr<BlockNode> body = parseBlock();
    if (!body) {
        errorManager->addError(
            ErrorType::Syntax, SyntaxErrors::InvalidStatement,
            ErrorSpan{nameToken.filePath, nameToken.value, nameToken.line, nameToken.column},
            "ErrorManager.Syntax.MissingToken.functionBody.message", {funcName},
            "ErrorManager.Syntax.MissingToken.functionBody.hint", {funcName});
        return nullptr;
    }

    auto node = ASTBuilder::createFunction(funcName, std::move(params), std::move(returnType), std::move(body), std::move(decorators), std::move(modifiers));
    for (const auto& modifier : modifiers) {
        if (modifier->modifier == ASTModifierType::Intrinsic) {
            node->isIntrinsic = true;
            node->body = nullptr;
        }
    }
    node->line = nameToken.line; node->column = nameToken.column; node->filePath = nameToken.filePath;
    return node;
}

// Detects nextline expression
bool Parser::isNextLine(){
    if (match(Delimeters::Semicolon) || match(Delimeters::Newline)) return true;
    return false;
}

// Detects whether upcoming tokens are identifier or member access followed by an assignment operator
// This code is pure unreadable garbage so lemme explain
bool Parser::isAssignableAhead(size_t offset) {
    // Take up new offset
    size_t p = pos + offset;

    // First token must be identifier for sure
    if (p >= tokens.size()) return false;
    if (tokens[p].type != TokenType::Identifier) return false;
    p++;

    // Then we create a chain of member accesses and identifiers/function calls inside them
    while (p < tokens.size()) {

        // If function call
        if (match(tokens[p], Delimeters::LeftParen)) {
            int depth = 1;
            p++; // consume left parenthesis

            // to be fair, i could not care less what's inside the () while we look ahead, so just skip until we find the matching right parenthesis
            while (p < tokens.size() && depth > 0) {
                if (match(tokens[p], Delimeters::LeftParen)) depth++;
                else if (match(tokens[p], Delimeters::RightParen)) depth--;
                p++;
            }

            continue;
        }

        // If member access however, we parse it too.
        if (match(tokens[p], Delimeters::Dot)) {
            p++; // consume dot
            if (p >= tokens.size()) return false;
            if (tokens[p].type != TokenType::Identifier) return false;
            p++;
            continue;
        }
        break;
    }

    // After all of that we check if there's assignment operator.
    if (p < tokens.size() && tokens[p].type == TokenType::Operator && isAssignmentOperator(tokens[p].value)) return true;
    // whoops, not an assignment
    return false;
}

// to make math order
int Parser::getOperatorPrecedence(const std::string& op){
    auto it = om.find(op);
    if (it == om.end()) return -3;

    switch (it->second) {
        case Operators::Power:
            return 7;
        case Operators::Multiply:
        case Operators::Divide:
        case Operators::Modulo:
            return 6;
        case Operators::Add:
        case Operators::Subtract:
            return 5;
        case Operators::BitwiseLeftShift:
        case Operators::BitwiseRightShift:
            return 4;
        case Operators::BitwiseAnd: return 3;
        case Operators::BitwiseXOr: return 2;
        case Operators::BitwiseOr: return 1;
        case Operators::Equal:
        case Operators::NotEqual:
        case Operators::LessThan:
        case Operators::GreaterThan:
        case Operators::LessThanOrEqual:
        case Operators::GreaterThanOrEqual:
            return 0;
        case Operators::LogicalAnd: return -1;
        case Operators::LogicalOr: return -2;
        default: return -3;
    }
}

bool Parser::isAssignmentOperator(const std::string& op) {
    auto it = om.find(op);
    if (it == om.end()) return false;

    switch (it->second) {
    case Operators::Assign:
    case Operators::AddAssign:
    case Operators::SubAssign:
    case Operators::MulAssign:
    case Operators::DivAssign:
    case Operators::ModAssign:
    case Operators::PowerAssign:
        return true;
    default:
        return false;
    }
}

std::string Parser::namespaceNameToString(ASTNode* node) {
    if (!node) return "<unknown>";

    if (node->type == ASTNodeType::Variable) return static_cast<VariableNode*>(node)->varName;

    if (node->type == ASTNodeType::MemberAccess) {
        auto* ma = static_cast<MemberAccessNode*>(node);
        return namespaceNameToString(ma->parent.get()) + "." +
            namespaceNameToString(ma->val.get());
    }

    return "<invalid namespace>";
}