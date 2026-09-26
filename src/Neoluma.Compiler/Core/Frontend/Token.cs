namespace Neoluma.Core.Frontend;

public enum TokenType {
    Keyword, Identifier, Number, Operator, String, Delimeter, Unknown, Decorator, Preprocessor, EndOfFile, Null,
}

public struct Token {
    public TokenType type;
    public string value;
    
    public string filePath;
    public int line, column;

    public Token(TokenType type, string value, string filePath, int line, int column) {
        this.type = type;
        this.value = value;
        this.filePath = filePath;
        this.line = line;
        this.column = column;
    }

    public string toStr() {
        string typeStr;

        switch (type) {
            case TokenType.Keyword:       typeStr = "Keyword"; break;
            case TokenType.Identifier:    typeStr = "Identifier"; break;
            case TokenType.Number:        typeStr = "Number"; break;
            case TokenType.Operator:      typeStr = "Operator"; break;
            case TokenType.String:        typeStr = "String"; break;
            case TokenType.Delimeter:     typeStr = "Delimeter"; break;
            case TokenType.Unknown:       typeStr = "Unknown"; break;
            case TokenType.Decorator:     typeStr = "Decorator"; break;
            case TokenType.Preprocessor:  typeStr = "Preprocessor"; break;
            case TokenType.EndOfFile:     typeStr = "EndOfFile"; break;
            case TokenType.Null:          typeStr = "Null"; break;
            default:                      typeStr = "<UNK>"; break;
        }

        return $"[{typeStr}] -> \"{value}\", (L{line}:{column})\n";
    }
};

// Enums
public enum Keywords {
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
public enum Operators {
    Add, Subtract, Multiply, Divide, Modulo, Power,
    Equal, NotEqual, LessThan, GreaterThan, LessThanOrEqual, GreaterThanOrEqual,
    LogicalAnd, LogicalOr, LogicalNot,
    Assign, Nullable,
    AddAssign, SubAssign, MulAssign, DivAssign, ModAssign, PowerAssign,
    AssignmentArrow, InheritanceArrow, TypeArrow,
    BitwiseAnd, BitwiseOr, BitwiseXOr, BitwiseNot, BitwiseLeftShift, BitwiseRightShift,
    //BitwiseUnsignedRightShift (from JS, not included for now because argued about)
};
public enum Decorators {
    Entry, Unsafe, Comptime
};
public enum Preprocessors {
    Import, Unsafe, Macro
};
public enum Delimeters {
    LeftParen, RightParen, LeftBracket, RightBracket, Semicolon, Newline, Comma, Dot,
    LeftBraces, RightBraces, Colon,
};

// ResolvedType is an enum of types Neoluma compiler internally supports by default.
public enum ResolvedType {
    Int8, Int16, Int, Int64, Int128,
    UInt8, UInt16, UInt, UInt64, UInt128,
    Float, Float64,
    Number, Bool, Str,
    Array, Dict, Set, Result,
    Void, UserDefined, Unknown
};

// Maps for every entry
public static class TokenMaps {
    public static readonly Dictionary<string, Keywords> keywords = new() {
        ["function"] = Keywords.Function, ["fn"] = Keywords.Function, ["class"] = Keywords.Class, ["enum"] = Keywords.Enum, ["interface"] = Keywords.Interface, ["namespace"] = Keywords.Namespace,
        ["if"] = Keywords.If, ["else"] = Keywords.Else, 
        ["for"] = Keywords.For, ["while"] = Keywords.While, ["break"] = Keywords.Break, ["continue"] = Keywords.Continue,
        ["switch"] = Keywords.Switch, ["case"] = Keywords.Case, ["default"] = Keywords.Default,
        ["try"] = Keywords.Try, ["catch"] = Keywords.Catch, ["throw"] = Keywords.Throw,
        ["yield"] = Keywords.Yield, ["return"] = Keywords.Return,
        ["as"] = Keywords.As, ["with"] = Keywords.With, ["in"] = Keywords.In, [":"] = Keywords.In, ["lambda"] = Keywords.Lambda,
        ["decorator"] = Keywords.Decorator,
        ["async"] = Keywords.Async, ["await"] = Keywords.Await, ["const"] = Keywords.Const, ["static"] = Keywords.Static,
        ["debug"] = Keywords.Debug, ["public"] = Keywords.Public, ["protected"] = Keywords.Protected, ["private"] = Keywords.Private,
        ["override"] = Keywords.Override,
        // Internal std library keyword that allows passing through LLVM calls
        ["intrinsic"] = Keywords.Intrinsic,
    };
    
    public static readonly Dictionary<string, Operators> operators = new() {
        ["+"] = Operators.Add, ["-"] = Operators.Subtract, ["*"] = Operators.Multiply, ["/"] = Operators.Divide, ["%"] = Operators.Modulo, ["^"] = Operators.Power,
        ["=="] = Operators.Equal, ["!="] = Operators.NotEqual, ["<"] = Operators.LessThan, [">"] = Operators.GreaterThan, ["<="] = Operators.LessThanOrEqual, [">="] = Operators.GreaterThanOrEqual,
        ["&&"] = Operators.LogicalAnd, ["||"] = Operators.LogicalOr, ["!"] = Operators.LogicalNot,
        ["and"] = Operators.LogicalAnd, ["or"] = Operators.LogicalOr, ["not"] = Operators.LogicalNot,
        ["="] = Operators.Assign, ["?"] = Operators.Nullable, ["=>"] = Operators.AssignmentArrow, ["<-"] = Operators.InheritanceArrow, ["->"] = Operators.TypeArrow,
        ["+="] = Operators.AddAssign, ["-="] = Operators.SubAssign, ["*="] = Operators.MulAssign, ["/="] = Operators.DivAssign, ["%="] = Operators.ModAssign, ["^="] = Operators.PowerAssign,
        ["~"] = Operators.BitwiseNot, ["&"] = Operators.BitwiseAnd, ["|"] = Operators.BitwiseOr, ["^^"] = Operators.BitwiseXOr, ["<<"] = Operators.BitwiseLeftShift, [">>"] = Operators.BitwiseRightShift,
    };

    public static readonly Dictionary<string, Decorators> decorators = new() {
        ["entry"] = Decorators.Entry, ["unsafe"] = Decorators.Unsafe, ["comptime"] = Decorators.Comptime,
    };

    public static readonly Dictionary<string, Preprocessors> preprocessors = new() {
        ["import"] = Preprocessors.Import, ["unsafe"] = Preprocessors.Unsafe, ["macro"] = Preprocessors.Macro,
    };

    public static readonly Dictionary<string, Delimeters> delimeters = new() {
        ["("] = Delimeters.LeftParen, [")"] = Delimeters.RightParen,
        ["{"] = Delimeters.LeftBraces, ["}"] = Delimeters.RightBraces,
        [";"] = Delimeters.Semicolon, [":"] = Delimeters.Colon, ["\\n"] = Delimeters.Newline, [","] = Delimeters.Comma,
        ["."] = Delimeters.Dot, ["["] = Delimeters.LeftBracket,
        ["]"] = Delimeters.RightBracket,
    };

    public static readonly Dictionary<ResolvedType, string> types = new() {
        [ResolvedType.Int8] = "int8", [ResolvedType.Int16] = "int16", [ResolvedType.Int] = "int", [ResolvedType.Int64] = "int64", [ResolvedType.Int128] = "int128", 
        [ResolvedType.UInt8] = "uint8", [ResolvedType.UInt16] = "uint16", [ResolvedType.UInt] = "uint", [ResolvedType.UInt64] = "uint64", [ResolvedType.UInt128] = "uint128", 
        [ResolvedType.Float] = "float", [ResolvedType.Float64] = "float64", [ResolvedType.Number] = "number", 
        [ResolvedType.Bool] = "bool", [ResolvedType.Str] = "str", [ResolvedType.Array] = "array", [ResolvedType.Dict] = "dict",
        [ResolvedType.Set] = "set", [ResolvedType.Result] = "result", [ResolvedType.Void] = "void",
    };
}