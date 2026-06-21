#pragma once
#include <vector>

#include "Libraries/Json/Json.hpp"
#include "HelperFunctions.hpp"

// NSyE{x}
enum struct SyntaxErrors {
    UnexpectedToken,
    MissingToken,
    InvalidStatement,
    UnterminatedString,
    UnterminatedComment,
    InvalidNumberFormat,
    UnexpectedEndOfFile,
    MismatchedBrackets,
};

// NAnE{x}
enum struct AnalysisErrors {
    // Variables & Scope
    UndefinedVariable,
    RedefinedVariable,
    UninitializedVariable,
    ConstantReassignment,
    VariableOutOfScope,
    ShadowedVariable,

    // Functions
    FunctionMismatch,
    UndefinedFunction,
    WrongArgumentCount,
    MissingRequiredParameter,
    DuplicateParameterName,
    InvalidParameterOrder,
    MissingReturnStatement,
    ReturnOutsideFunction,

    // Classes & OOP
    UndefinedMember,
    CircularInheritance,
    InvalidConstructor,
    MissingSuperCall,
    InvalidSuperCall,
    AccessViolation,
    InvalidOverride,
    OverrideSignatureMismatch,

    // Modifiers & Decorators
    InvalidModifierUsage,
    ConflictingModifiers,
    DecoratorMisuse,
    UndefinedDecorator,
    DecoratorOnInvalidTarget,
    DecoratorArgumentMismatch,
    MultipleEntryPoints,
    NoEntryPoints,

    // Control Flow
    BreakOutsideLoop,
    ContinueOutsideLoop,
    UnreachableCode,
    DuplicateCaseValue,
    CaseTypeMismatch,

    // Interfaces & Enums
    InterfaceNotImplemented,
    InterfaceSignatureMismatch,
    DuplicateEnumMember,

    // Lambdas & Closures
    InvalidCapture,
    ModifyingCapturedConst,

    // Special Features
    AwaitOutsideAsync,
    YieldOutsideGenerator,

    // Assignment
    AssignmentToNonLValue,

    // Core Type Errors
    TypeMismatch,
    UnknownType,
    InvalidCast,
    LiteralOverflow,

    // Operations
    AssignmentTypeMismatch,
    BinaryOperationTypeMismatch,
    UnaryOperationTypeMismatch,
    ReturnTypeMismatch,
    ArgumentTypeMismatch,

    // Nullable
    NullAssignmentToNonNullable,
    NullableAccessWithoutCheck,

    // Collections
    ArrayElementTypeMismatch,
    SetDuplicateValue,
    DictKeyTypeMismatch,
    DictValueTypeMismatch,
    InvalidIndexType,
    IndexingNonIndexable,

    // Result Type
    ResultUnwrapWithoutCheck,

    // Member Access
    MemberAccessOnNonObject,
    MemberAccessOnNullable,

    // Inference
    TypeInferenceFailed,
    AmbiguousType,
};

// NPrE{x}
enum struct PreprocessorErrors {
    // Import
    ImportNotFound,
    CircularImport,
    ImportAliasConflict,
    InvalidImportPath,
    ForeignImportWithoutLangpack,

    // Macro
    MacroError,
    UndefinedMacro,
    MacroExpansionError,

    // Directive
    InvalidDirective,
    UnsafeWithoutDirective,
    BaremetalWithoutDirective,
    ConflictingDirectives,
    DirectiveInWrongContext,

    // ???
    InvalidConsoleArgument
};

//NCoE{x}
enum struct CodegenErrors {
    LLVMGenerationError,
    UnsupportedFeature,
    OptimizationFailure,
    LinkageError,
    TargetNotSupported,
};

//NRuE{x}
enum struct RuntimeErrors {
    DivisionByZero,
    NullReference,
    IndexOutOfBounds,
    FloatingPointError,
    IntegerOverflow,
    UninitializedVariableAccess,
    KeyNotFound,
    StackOverflow,
    UnhandledError,
};

enum struct ErrorSeverity {
    Error,
    Warning,
};

using ErrorCode = Option<SyntaxErrors, AnalysisErrors, PreprocessorErrors, CodegenErrors, RuntimeErrors>;

struct ErrorSpan {
    String filePath;
    int len = 0;
    int line = 0;
    int column = 1;

    ErrorSpan(String filePath, String value, int line, int column): filePath(std::move(filePath)), len((int)std::move(value).length()), line(line), column(column) {};
};

// Additional context related to an error or warning.
struct ErrorNote {
    ErrorSpan span;
    String messageKey;
    Array<String> messageArgs;
};

struct Error {
    ErrorSeverity severity;
    ErrorCode code;
    ErrorSpan span;

    String messageKey; // Message explaining what's wrong, takes localization key
    Array<String> messageArgs; // Arguments to make message more precise
    String hintKey; // Hint to fix the error, takes localization key
    Array<String> hintArgs;// Arguments to make hints more precise

    Array<ErrorNote> notes;

    Error(ErrorSeverity severity, ErrorCode code, ErrorSpan span, String messageKey, Array<String> messageArgs = {},
    String hintKey = "", Array<String> hintArgs = {}, Array<ErrorNote> notes = {}) : severity(severity),
    code(std::move(code)), span(std::move(span)), messageKey(std::move(messageKey)), messageArgs(std::move(messageArgs)),
    hintKey(std::move(hintKey)), hintArgs(std::move(hintArgs)), notes(std::move(notes)) {}
};

class ErrorManager {
public:
    void addError(ErrorCode code, ErrorSpan span, String messageKey, Array<String> messageArgs = {}, String hintKey = "",
        Array<String> hintArgs = {}, Array<ErrorNote> notes = {}) { errors.push_back(Error{ErrorSeverity::Error, code, span,
        messageKey, messageArgs, hintKey, hintArgs, notes});}

    void addWarning(ErrorCode code, ErrorSpan span, String messageKey, Array<String> messageArgs = {}, String hintKey = "",
        Array<String> hintArgs = {}, Array<ErrorNote> notes = {}) { errors.push_back(Error{ErrorSeverity::Warning, code, span,
        messageKey, messageArgs, hintKey, hintArgs, notes}); }

    [[nodiscard]] bool hasErrors() const {
        for (const auto& error : errors) if (error.severity == ErrorSeverity::Error) return true;
        return false;
    }
    [[nodiscard]] bool hasWarnings() const {
        for (const auto& error : errors) if (error.severity == ErrorSeverity::Warning) return true;
        return false;
    }
    void printErrors();
private:
    Array<Error> errors;
    static String formatCode(ErrorCode code);
    static String formatColor(ErrorCode code);
    static String formatIcon(ErrorSeverity severity);
    static String formatSeverity(ErrorSeverity severity);
    json::Value toJson() const;
};
