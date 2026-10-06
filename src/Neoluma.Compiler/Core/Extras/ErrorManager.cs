using Neoluma.Libraries;

namespace Neoluma.Core.Extras;



// NSyE{x}
enum SyntaxErrors {
    UnexpectedToken,
    MissingToken,
    InvalidStatement,
    UnterminatedString,
    UnterminatedComment,
    InvalidNumberFormat,
    UnexpectedEndOfFile,
    MismatchedBrackets,
}

// NAnE{x}
enum AnalysisErrors {
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
    ListElementTypeMismatch,
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
}

// NPrE{x}
enum PreprocessorErrors {
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
}

//NCoE{x}
enum CodegenErrors {
    LLVMGenerationError,
    UnsupportedFeature,
    OptimizationFailure,
    LinkageError,
    TargetNotSupported,
}

//NRuE{x}
enum RuntimeErrors {
    DivisionByZero,
    NullReference,
    IndexOutOfBounds,
    FloatingPointError,
    IntegerOverflow,
    UninitializedVariableAccess,
    KeyNotFound,
    StackOverflow,
    UnhandledError,
}

public enum ErrorSeverity {
    Error,
    Warning,
}

public readonly struct ErrorSpan {
    public readonly SourceSpan sourceSpan;
    public readonly int len = 0;

    public ErrorSpan(string filePath, string value, int line, int column) {
        sourceSpan = new SourceSpan(filePath, line, column);
        len = value.Length;
    }

    public ErrorSpan(string value, SourceSpan sourceSpan) {
        this.sourceSpan = sourceSpan;
        len = value.Length;
    }
}

// Additional context related to an error or warning.
public struct ErrorNote {
    public ErrorSpan span;
    public string messageKey;
    public List<string> messageArgs;

    public ErrorNote(ErrorSpan span, string messageKey, List<string> messageArgs) {
        this.span = span;
        this.messageKey = messageKey;
        this.messageArgs = messageArgs;
    }
}

public struct Error {
    public ErrorSeverity severity;
    public Enum code;
    public ErrorSpan span;

    public string messageKey; // Message explaining what's wrong, takes localization key
    public List<string> messageArgs; // Arguments to make message more precise
    public string hintKey; // Hint to fix the error, takes localization key
    public List<string> hintArgs;// Arguments to make hints more precise

    public List<ErrorNote> notes;

    public Error(ErrorSeverity severity, Enum code, ErrorSpan span, string messageKey, List<string>? messageArgs, string? hintKey, List<string>? hintArgs, List<ErrorNote>? notes) {
        this.severity = severity;
        this.code = code;
        this.span = span;
        this.messageKey = messageKey;
        this.messageArgs = messageArgs ?? new();
        this.hintKey = hintKey ?? "";
        this.hintArgs = hintArgs ?? new();
        this.notes = notes ?? new();
    }
}

public class ErrorManager { 
    List<Error> errors = new();
    
    public void addError(Enum code, ErrorSpan span, string messageKey, List<string>? messageArgs = null, string? hintKey = null, List<string>? hintArgs = null, List<ErrorNote>? notes = null) {
        if (!validateErrorCode(code)) throw new ArgumentException("[Neoluma/ErrorManager] The error code enum is used incorrectly.");
        errors.Add(new Error(ErrorSeverity.Error, code, span, messageKey, messageArgs, hintKey, hintArgs, notes));
    }

    public void addWarning(Enum code, ErrorSpan span, string messageKey, List<string>? messageArgs = null, string? hintKey = null, List<string>? hintArgs = null, List<ErrorNote>? notes = null) {
        if (!validateErrorCode(code)) throw new ArgumentException("[Neoluma/ErrorManager] The error code enum is used incorrectly.");
        errors.Add(new Error(ErrorSeverity.Warning, code, span, messageKey, messageArgs, hintKey, hintArgs, notes));
    }

    public bool hasErrors() {
        foreach (var error in errors) if (error.severity == ErrorSeverity.Error) return true;
        return false;
    }
    
    public bool hasWarnings() {
        foreach (var error in errors) if (error.severity == ErrorSeverity.Warning) return true;
        return false;
    }

    public void printErrors() {
        if (errors.Count == 0) return;

        // stores file sources as keys for easier context access
        Dictionary<string, string> fileCache = new();
        
        int errorCount = 0; 
        int warningCount = 0;

        foreach (var error in errors) {
            if (error.severity == ErrorSeverity.Error) errorCount++;
            if (error.severity == ErrorSeverity.Warning) warningCount++;
            
            string msg = Localization.translate(error.messageKey, error.messageArgs.ToArray());

            // Header message
            Console.WriteLine($"{formatColor(error.code)}[{formatCode(error.code)}]  {formatIcon(error.severity)}  {msg}{Color.Reset}");
            
            // Message
            printSpan(error.span, msg, fileCache);

            // Notes
            foreach (var note in error.notes) {
                string noteMsg = Localization.translate(note.messageKey, note.messageArgs.ToArray());
                Console.WriteLine($"{Color.TextHex("#46b0ba")}ℹ️  {Localization.translate("ErrorManager.note")}: {noteMsg}{Color.Reset}");
                printSpan(note.span, noteMsg, fileCache);
            }

            // Hints
            if (error.hintKey.Length != 0) {
                string hint = Localization.translate(error.hintKey, error.hintArgs.ToArray());
                Console.WriteLine($"{Color.TextHex("#f6ff75")}{Localization.translate("ErrorManager.hint", hint)}{Color.Reset}\n");
            }
        }
        
        if (errorCount > 0) Console.WriteLine($"{Color.TextHex("#ff5050")}{Localization.translate("ErrorManager.errorsFound", Convert.ToString(errorCount))}{Color.Reset}");
        if (warningCount > 0) Console.WriteLine($"{Color.TextHex("#f6ff75")}{Localization.translate("ErrorManager.warningsFound", Convert.ToString(warningCount))}{Color.Reset}");
    }
    
    void printSpan(ErrorSpan span, string msg, Dictionary<string, string> fileCache) {
        // Copies a new path, normalizes \\ into / in paths. yea we're fixing windows bad designs now.
        string filePath = span.sourceSpan.filePath.Replace('\\', '/');
        if (filePath.Length == 0) return;
        
        // read a file and cache it if it's not in fileCache
        if (filePath.Length != 0 && !fileCache.ContainsKey(filePath))
            fileCache[filePath] = File.ReadAllText(filePath);

        // Gets the line context
        // From what i've checked it's guaranteed here for a file to be in vector.
        string source = fileCache[filePath];
        string[] lines = source.Replace("\r\n", "\n").Split('\n');

        int lineNumber = Math.Max(1, span.sourceSpan.line);
        int columnNumber = Math.Max(1, span.sourceSpan.column);
        int index = lineNumber - 1;

        string prevLine = index > 0 && index - 1 < lines.Length ? lines[index - 1] : "";
        string errorLine = index < lines.Length ? lines[index] : "";
        string nextLine = index + 1 < lines.Length ? lines[index + 1] : "";

        int caretOffset = columnNumber - 1;

        Console.WriteLine($"➡️  {span.sourceSpan.filePath}:{lineNumber}:{columnNumber}");
        if (index > 0) Console.WriteLine($"{index,3} | {prevLine}");
        Console.WriteLine($"{lineNumber,3} | {Color.TextHex("#ff5050")}{errorLine}{Color.Reset}");
        Console.WriteLine($"{new string(' ', lineNumber + 2)}| {new string(' ', caretOffset)}{new string('^', span.len + 2)} {msg}");
        if (nextLine.Length != 0) Console.WriteLine($"{lineNumber + 1,3} | {nextLine}");
    }

    string formatCode(Enum code) {
        if (!validateErrorCode(code)) throw new ArgumentException("[Neoluma/ErrorManager] The error code enum is used incorrectly.");
        int value = Convert.ToInt32(code) + 1;
        
        return code switch {
            SyntaxErrors => $"NSyE{value}",
            AnalysisErrors => $"NAnE{value}",
            PreprocessorErrors => $"NPrE{value}",
            CodegenErrors => $"NCoE{value}",
            RuntimeErrors => $"NRuE{value}",
            _ => "N??E?"
        };
    }

    string formatColor(Enum code) {
        if (!validateErrorCode(code)) throw new ArgumentException("[Neoluma/ErrorManager] The error code enum is used incorrectly.");
        
        return code switch {
            SyntaxErrors => Color.TextHex("#ff5050"),
            AnalysisErrors => Color.TextHex("#ff9f40"),
            PreprocessorErrors => Color.TextHex("#00bfff"),
            CodegenErrors => Color.TextHex("#ff75d7"),
            RuntimeErrors => Color.TextHex("#ffa500"),
            _ => Color.TextHex("#4A2BD6")
        };
    }

    string formatIcon(ErrorSeverity severity) {
        return severity switch {
            ErrorSeverity.Error => "❌",
            ErrorSeverity.Warning => "⚠️",
            _ => "❔"
        };
    }

    string formatSeverity(ErrorSeverity severity) {
        return severity switch {
            ErrorSeverity.Error => "error",
            ErrorSeverity.Warning => "warning",
            _ => "???"
        };
    }

    public JSON.Value toJson() {
        JSON.Object root = new();
        root.Add("status", hasErrors() ? "error" : "ok");

        JSON.Array errorArray = new();

        foreach (var error in errors) {
            JSON.Object item = new();

            item.Add("severity", formatSeverity(error.severity));
            item.Add("error_code", formatCode(error.code));
            item.Add("file", error.span.sourceSpan.filePath);
            item.Add("line", error.span.sourceSpan.line);
            item.Add("column", error.span.sourceSpan.column);
            item.Add("length", error.span.len);
            item.Add("message_key", error.messageKey);
            item.Add("message", Localization.translate(error.messageKey, error.messageArgs.ToArray()));
            item.Add("hint_key", error.hintKey);
            item.Add("hint", error.hintKey.Length == 0 ? "" : Localization.translate(error.hintKey, error.hintArgs.ToArray()));

            JSON.Array messageArgs = new();
            foreach (string arg in error.messageArgs) messageArgs.Add(arg);
            item.Add("message_args", messageArgs);

            JSON.Array hintArgs = new();
            foreach (string arg in error.hintArgs) hintArgs.Add(arg);
            item.Add("hint_args", hintArgs);

            errorArray.Add(item);
        }

        root.Add("Errors", errorArray);
        return root;
    }
    
    // helper
    bool validateErrorCode(Enum code) {
        return code is SyntaxErrors or AnalysisErrors or PreprocessorErrors or CodegenErrors or RuntimeErrors;
    }
};
