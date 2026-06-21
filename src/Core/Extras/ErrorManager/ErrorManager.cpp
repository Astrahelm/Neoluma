#include "ErrorManager.hpp"
#include "../../../Libraries/Color/Color.hpp"
#include <print>
#include <sstream>
#include <format>

#include "HelperFunctions.hpp"
#include "Libraries/Localization/Localization.hpp"

//void printSpanContext(const ErrorSpan& span, const )

void ErrorManager::printErrors() {
    if (errors.empty()) return;

    // stores file sources as keys for easier context access
    std::unordered_map<String, String> fileCache;

    int errorCount = 0;
    int warningCount = 0;

    for (const auto& e : errors) {
        if (e.severity == ErrorSeverity::Error) errorCount++;
        if (e.severity == ErrorSeverity::Warning) warningCount++;

        // Copies a new path, normalizes \\ into / in paths. yea we're fixing windows bad designs now.
        String filePath = normalizePath(e.span.filePath);

        // read a file and cache it if it's not in fileCache
        if (!filePath.empty() && !fileCache.contains(filePath))
            fileCache[filePath] = readFile(filePath);

        // Gets the line context
        // From what i've checked it's guaranteed here for a file to be in vector.
        std::istringstream srcStream(fileCache[filePath]);
        String prevLine, line, errorLine, nextLine;

        int lineNumber = std::max(1, e.span.line);
        int columnNumber  = std::max(1, e.span.column);
        int previousLine = lineNumber - 1;
        int caretOffset = columnNumber - 1; // ^^^^^^^^^^

        int lineNum = 1;
        while (std::getline(srcStream, line)) {
            if (lineNum == lineNumber) {
                errorLine = line;
                if (!std::getline(srcStream, nextLine)) nextLine.clear();
                break;
            }

            prevLine = line;
            lineNum++;
        }

        // Error message (localized)
        String msg = Localization::translatef(e.messageKey, e.messageArgs);

        // Header message
        std::println("{}[{}]  {}  {}{}", std::move(formatColor(e.code)), formatCode(e.code), std::move(formatIcon(e.severity)), msg, Color::Reset);
        // path:line:column
        std::println("➡️  {}:{}:{}", e.span.filePath, lineNumber, columnNumber);
        // Context
        if (previousLine > 0) std::println("{:>3} | {}", previousLine, prevLine);
        std::println("{:>3} | {}{}{}", lineNumber, std::move(Color::TextHex("#ff5050")), errorLine, Color::Reset);
        std::println("{}| {}{} {}", std::string(lineNumber + 2, ' '), std::string(caretOffset, ' '), std::string(e.span.len + 2, '^'), msg);
        if (!nextLine.empty()) std::println("{:>3} | {}", lineNumber + 1, nextLine);
        // Notes
        for (const auto& note : e.notes) {
            String noteMsg = Localization::translatef(note.messageKey, note.messageArgs);
            std::println("{}ℹ️  {}: {}", Color::TextHex("#46b0ba"), Localization::translate("ErrorManager.note"), noteMsg);

            String notePrevLine, noteLine, noteErrorLine, noteNextLine;
            int noteLineNumber = std::max(1, note.span.line);
            int noteColumnNumber  = std::max(1, note.span.column);
            int notePreviousLine = noteLineNumber - 1;
            int noteCaretOffset = noteColumnNumber - 1;

            int noteLineNum = 1;
            while (std::getline(srcStream, noteLine)) {
                if (noteLineNum == noteLineNumber) {
                    noteErrorLine = noteLine;
                    if (!std::getline(srcStream, noteNextLine)) noteNextLine.clear();
                    break;
                }

                notePrevLine = noteLine;
                noteLineNum++;
            }

            if (notePreviousLine > 0) std::println("{:>3} | {}", notePreviousLine, notePrevLine);
            std::println("{:>3} | {}{}{}", noteLineNumber, std::move(Color::TextHex("#ff5050")), noteErrorLine, Color::Reset);
            std::println("{}| {}{} {}", std::string(noteLineNumber + 2, ' '), std::string(noteCaretOffset, ' '), std::string(note.span.len + 2, '^'), noteMsg);
            if (!nextLine.empty()) std::println("{:>3} | {}", noteLineNumber + 1, noteNextLine);
        }
        // Hint
        if (!e.hintKey.empty()) std::println(std::cout, "{}{}{}\n", std::move(Color::TextHex("#f6ff75")), formatStr(Localization::translate("ErrorManager.hint"), Localization::translatef(e.hintKey, e.hintArgs)), Color::Reset);
    }

    if (errorCount > 0) std::println(std::cout, "{}{}{}", Color::TextHex("#ff5050"), Localization::translatef("ErrorManager.errorsFound", {std::to_string(errors.size())}), Color::Reset);
    if (warningCount > 0) std::println(std::cout, "{}{}{}", Color::TextHex("#f6ff75"), Localization::translatef("ErrorManager.warningsFound", {std::to_string(errors.size())}), Color::Reset);
}

String ErrorManager::formatCode(ErrorCode code){
    if (optionIs<SyntaxErrors>(code)) return formatStr("NSyE{}", (int)getOption<SyntaxErrors>(code)+1);
    if (optionIs<AnalysisErrors>(code)) return formatStr("NAnE{}", (int)getOption<AnalysisErrors>(code)+1);
    if (optionIs<PreprocessorErrors>(code)) return formatStr("NPrE{}", (int)getOption<PreprocessorErrors>(code)+1);
    if (optionIs<CodegenErrors>(code)) return formatStr("NCoE{}", (int)getOption<CodegenErrors>(code)+1);
    if (optionIs<RuntimeErrors>(code)) return formatStr("NRuE{}", (int)getOption<RuntimeErrors>(code)+1);
    return "N??E?";
}

String ErrorManager::formatColor(ErrorCode code) {
    if (optionIs<SyntaxErrors>(code)) return Color::TextHex("#ff5050");
    if (optionIs<AnalysisErrors>(code)) return Color::TextHex("#ff9f40");
    if (optionIs<PreprocessorErrors>(code)) return Color::TextHex("#00bfff");
    if (optionIs<CodegenErrors>(code)) return Color::TextHex("#ff75d7");
    if (optionIs<RuntimeErrors>(code)) return Color::TextHex("#ffa500");
    return Color::TextHex("#4A2BD6");
}

String ErrorManager::formatIcon(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::Error: return "❌";
        case ErrorSeverity::Warning: return "⚠️";
    }
    return "❔";
}

String ErrorManager::formatSeverity(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::Error: return "error";
        case ErrorSeverity::Warning: return "warning";
    }
    return "???";
}

// TODO: this clanker made solution should be begone
json::Value ErrorManager::toJson() const {
    json::Object root;
    root.emplace_back("status", hasErrors() ? json::Value("ok") : json::Value("error"));

    json::Array errorArray;
    errorArray.reserve(errors.size());

    for (const auto& error : errors) {
        json::Object item;
        item.emplace_back("severity", formatSeverity(error.severity));
        item.emplace_back("error_code", formatCode(error.code));
        item.emplace_back("file", error.span.filePath);
        item.emplace_back("line", (std::int64_t)error.span.line);
        item.emplace_back("column", (std::int64_t)error.span.column);
        item.emplace_back("length", (std::int64_t)error.span.len);
        item.emplace_back("message_key", error.messageKey);
        item.emplace_back("message", Localization::translatef(error.messageKey, error.messageArgs));
        item.emplace_back("hint_key", error.hintKey);
        item.emplace_back("hint", error.hintKey.empty() ? json::Value("") : json::Value(Localization::translatef(error.hintKey, error.hintArgs)));

        json::Array messageArgs;
        for (const auto& arg : error.messageArgs) messageArgs.emplace_back(arg);
        item.emplace_back("message_args", std::move(messageArgs));

        json::Array hintArgs;
        for (const auto& arg : error.hintArgs) hintArgs.emplace_back(arg);
        item.emplace_back("hint_args", std::move(hintArgs));

        errorArray.emplace_back(std::move(item));
    }

    root.emplace_back("Errors", std::move(errorArray));
    return json::Value(std::move(root));
}
