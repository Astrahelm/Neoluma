#pragma once

#include <optional>
#include <string>
#include <unordered_map>

enum class TokenType { 
    Keyword, Identifier, Number, Operator, String, Delimeter, Unknown, Decorator, Preprocessor, EndOfFile, Null,
};

struct Token {
    TokenType type;
    std::string value;
    std::string toStr() const;
    std::string filePath;
    int line, column;
};

// Enums
enum class Keywords {
    Function, Class, Enum, Interface, Namespace,
    If, Else,
    For, While, Break, Continue,
    Switch, Case, Default,
    Try, Catch, Throw,
    Async, Await,
    Yield, Return,
    Static, Decorator, Const,
    As, With, In, Lambda,
    Debug, Public, Protected, Private, Override,
    Intrinsic
};
enum class Operators {
    Add, Subtract, Multiply, Divide, Modulo, Power,
    Equal, NotEqual, LessThan, GreaterThan, LessThanOrEqual, GreaterThanOrEqual,
    LogicalAnd, LogicalOr, LogicalNot,
    Assign, Nullable,
    AddAssign, SubAssign, MulAssign, DivAssign, ModAssign, PowerAssign,
    AssignmentArrow, InheritanceArrow, TypeArrow,
    BitwiseAnd, BitwiseOr, BitwiseXOr, BitwiseNot, BitwiseLeftShift, BitwiseRightShift,
    //BitwiseUnsignedRightShift (from JS, not included for now because argued about)
};
enum class Decorators {
    Entry, Unsafe, Comptime
};
enum class Preprocessors {
    Import, Unsafe, Macro
};
enum class Delimeters {
    LeftParen, RightParen, LeftBracket, RightBracket, Semicolon, Newline, Comma, Dot,
    LeftBraces, RightBraces, Colon,
};

// ResolvedType is an enum of types Neoluma compiler internally supports by default.
enum class ResolvedType {
    Int8, Int16, Int, Int64, Int128,
    UInt8, UInt16, UInt, UInt64, UInt128,
    Float, Float64,
    Number, Bool, Str,
    Array, Dict, Set, Result,
    Void, UserDefined, Unknown
};

// Mapping entries
struct EKeyword { std::string name; Keywords token; };
struct EOperator { std::string name; Operators token; };
struct EDecorator { std::string name; Decorators token; };
struct EPreprocessor { std::string name; Preprocessors token; };
struct EDelimeter { std::string name; Delimeters token; };
struct EResolvedType { ResolvedType type; std::string name; };

const std::unordered_map<std::string, Keywords>& getKeywordMap();
const std::unordered_map<std::string, Operators>& getOperatorMap();
const std::unordered_map<std::string, Decorators>& getDecoratorMap();
const std::unordered_map<std::string, Preprocessors>& getPreprocessorMap();
const std::unordered_map<std::string, Delimeters>& getDelimeterMap();
const std::unordered_map<std::string, ResolvedType>& getTypeMap();
