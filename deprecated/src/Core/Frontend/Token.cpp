#include "Token.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <format>
#include <unordered_map>
#include <format>

// Maps for every entry
const EKeyword keywordMap[] = {
    {"function", Keywords::Function}, {"fn", Keywords::Function}, {"class", Keywords::Class}, {"enum", Keywords::Enum}, {"interface", Keywords::Interface}, {"namespace", Keywords::Namespace},
    {"if", Keywords::If}, {"else", Keywords::Else},
    {"for", Keywords::For}, {"while", Keywords::While}, {"break", Keywords::Break}, {"continue", Keywords::Continue},
    {"switch", Keywords::Switch}, {"case", Keywords::Case}, {"default", Keywords::Default},
    {"try", Keywords::Try}, {"catch", Keywords::Catch}, {"throw", Keywords::Throw},
    {"yield", Keywords::Yield}, {"return", Keywords::Return},
    {"as", Keywords::As}, {"with", Keywords::With}, {"in", Keywords::In}, {":", Keywords::In}, {"lambda", Keywords::Lambda},
    {"decorator", Keywords::Decorator}, 
    {"async", Keywords::Async}, {"await", Keywords::Await}, {"const", Keywords::Const}, {"static", Keywords::Static}, 
    {"debug", Keywords::Debug}, {"public", Keywords::Public}, {"protected", Keywords::Protected}, {"private", Keywords::Private},
    {"override", Keywords::Override},
    // Internal std library keyword that allows passing through LLVM calls
    {"intrinsic", Keywords::Intrinsic},
};

const EOperator operatorMap[] {
    {"+", Operators::Add}, {"-", Operators::Subtract}, {"*", Operators::Multiply}, {"/", Operators::Divide}, {"%", Operators::Modulo}, {"^", Operators::Power},
    {"==", Operators::Equal}, {"!=", Operators::NotEqual}, {"<", Operators::LessThan}, {">", Operators::GreaterThan}, {"<=", Operators::LessThanOrEqual}, {">=", Operators::GreaterThanOrEqual},
    {"&&", Operators::LogicalAnd}, {"||", Operators::LogicalOr}, {"!", Operators::LogicalNot},
    {"and", Operators::LogicalAnd}, {"or", Operators::LogicalOr}, {"not", Operators::LogicalNot},
    {"=", Operators::Assign}, {"?", Operators::Nullable}, {"=>", Operators::AssignmentArrow}, {"<-", Operators::InheritanceArrow}, {"->", Operators::TypeArrow},
    {"+=", Operators::AddAssign}, {"-=", Operators::SubAssign}, {"*=", Operators::MulAssign}, {"/=", Operators::DivAssign}, {"%=", Operators::ModAssign}, {"^=", Operators::PowerAssign},
    {"~", Operators::BitwiseNot}, {"&", Operators::BitwiseAnd}, {"|", Operators::BitwiseOr}, {"^^", Operators::BitwiseXOr}, {"<<", Operators::BitwiseLeftShift}, {">>", Operators::BitwiseRightShift}, 
};

const EDecorator decoratorMap[] {
    {"entry", Decorators::Entry}, {"unsafe", Decorators::Unsafe}, {"comptime", Decorators::Comptime},
};

const EPreprocessor preprocessorMap[] {
    {"import", Preprocessors::Import},  {"unsafe", Preprocessors::Unsafe}, {"macro", Preprocessors::Macro},
};

const EDelimeter delimeterMap[] {
    {"(", Delimeters::LeftParen}, {")", Delimeters::RightParen},
    {"{", Delimeters::LeftBraces}, {"}", Delimeters::RightBraces},
    {";", Delimeters::Semicolon}, {":", Delimeters::Colon}, {"\\n", Delimeters::Newline}, {",", Delimeters::Comma},
    {".", Delimeters::Dot}, {"[", Delimeters::LeftBracket},
    {"]", Delimeters::RightBracket},
};

const EResolvedType typesMap[] = {
    { ResolvedType::Int8, "int8" }, { ResolvedType::Int16, "int16" }, { ResolvedType::Int, "int" }, { ResolvedType::Int64, "int64" },
    { ResolvedType::Int128, "int128" }, { ResolvedType::UInt8, "uint8" }, { ResolvedType::UInt16, "uint16" }, { ResolvedType::UInt, "uint" },
    { ResolvedType::UInt64, "uint64" }, { ResolvedType::UInt128, "uint128" }, { ResolvedType::Float, "float" }, { ResolvedType::Float64, "float64" },
    { ResolvedType::Number, "number" }, { ResolvedType::Bool, "bool" }, { ResolvedType::Str, "str" }, { ResolvedType::Array, "array" },
    { ResolvedType::Dict, "dict" }, { ResolvedType::Set, "set" }, { ResolvedType::Result, "result" }, { ResolvedType::Void, "void" },
};

std::string Token::toStr() const {
    std::string typeStr;

    switch (type) {
        case TokenType::Keyword:         typeStr = "Keyword"; break;
        case TokenType::Identifier:      typeStr = "Identifier"; break;
        case TokenType::Number:          typeStr = "Number"; break;
        case TokenType::Operator:        typeStr = "Operator"; break;
        case TokenType::String:          typeStr = "String"; break;
        case TokenType::Delimeter:       typeStr = "Delimeter"; break;
        case TokenType::Unknown:         typeStr = "Unknown"; break;
        case TokenType::Decorator:       typeStr = "Decorator"; break;
        case TokenType::Preprocessor:    typeStr = "Preprocessor"; break;
        case TokenType::EndOfFile:       typeStr = "EndOfFile"; break;
        case TokenType::Null:            typeStr = "Null"; break;
        default:                         typeStr = "<UNK>"; break;
    }

    return std::format("[{}] -> \"{}\", (L{}:{})\n", typeStr, value, line, column);
}

const std::unordered_map<std::string, Keywords>& getKeywordMap() {
    static std::unordered_map<std::string, Keywords> map;
    if (map.empty()) {
        for (auto& k : keywordMap) map[k.name] = k.token;
    }
    return map;
}
const std::unordered_map<std::string, Operators>& getOperatorMap() {
    static std::unordered_map<std::string, Operators> map;
    if (map.empty()) {
        for (auto& k : operatorMap) map[k.name] = k.token;
    }
    return map;
}
const std::unordered_map<std::string, Decorators>& getDecoratorMap() {
    static std::unordered_map<std::string, Decorators> map;
    if (map.empty()) {
        for (auto& k : decoratorMap) map[k.name] = k.token;
    }
    return map;
}
const std::unordered_map<std::string, Preprocessors>& getPreprocessorMap() {
    static std::unordered_map<std::string, Preprocessors> map;
    if (map.empty()) {
        for (auto& k : preprocessorMap) map[k.name] = k.token;
    }
    return map;
}
const std::unordered_map<std::string, Delimeters>& getDelimeterMap() {
    static std::unordered_map<std::string, Delimeters> map;
    if (map.empty()) {
        for (auto& k : delimeterMap) map[k.name] = k.token;
    }
    return map;
}

const std::unordered_map<std::string, ResolvedType>& getTypeMap() {
    static std::unordered_map<std::string, ResolvedType> map;
    if (map.empty()) for (auto& k : typesMap) map[k.name] = k.type;
    return map;
}