// nodes.cpp
// note from tsuki: this is tf2 coconut of this entire project. if you delete it
// the entire project will blow tf up. don't delete it.
// xD /jk
//
/*
 * Note: this code is AI-generated. I didn't want to repetitively sit for hours trying to make a
 * nice debug output tree. Nobody's gonna use this anyway unless for dev purposes like me, who cares?!
*/
#include "Nodes.hpp"

#include <format>
#include <string>
#include <utility>
#include <vector>

ASTNode::~ASTNode() = default;

// Pure-virtual can still have a body.
// If you get this output it means you screwed up -tsuki
std::string ASTNode::toString(int indent) const { return std::string(indent, ' ') + "<ASTNode>"; }

namespace {

// ---------- tiny formatting helpers ----------
inline std::string ind(int n) { return std::string(n, ' '); }
inline const char* b2s(bool v) { return v ? "true" : "false"; }

static std::string modifierToString(ASTModifierType modifier) {
    switch (modifier) {
        case ASTModifierType::Public: return "Public";
        case ASTModifierType::Private: return "Private";
        case ASTModifierType::Protected: return "Protected";
        case ASTModifierType::Static: return "Static";
        case ASTModifierType::Const: return "Const";
        case ASTModifierType::Override: return "Override";
        case ASTModifierType::Async: return "Async";
        case ASTModifierType::Debug: return "Debug";
        default: return "Unknown";
    }
}

static std::string importTypeToString(ASTImportType importType) {
    switch (importType) {
        case ASTImportType::Native: return "Native";
        case ASTImportType::Relative: return "Relative";
        case ASTImportType::Foreign: return "Foreign";
        case ASTImportType::ForeignRelative: return "ForeignRelative";
        default: return "Unknown";
    }
}

static std::string directiveToString(ASTPreprocessorDirectiveType directive) {
    switch (directive) {
        case ASTPreprocessorDirectiveType::Import: return "Import";
        case ASTPreprocessorDirectiveType::Unsafe: return "Unsafe";
        case ASTPreprocessorDirectiveType::Macro: return "Macro";
        case ASTPreprocessorDirectiveType::None: return "None";
        default: return "Unknown";
    }
}

// ---- Header building ----
// Keep base fields minimal: line/column only if not 0:0, filePath only if non-empty, value only if non-empty.
static void appendBaseHeaderFields(std::vector<std::pair<std::string, std::string>>& fields, const ASTNode& n) {
    if (!(n.line == 0 && n.column == 0)) {
        fields.emplace_back("line", std::format("{}", n.line));
        fields.emplace_back("column", std::format("{}", n.column));
    }
    if (!n.filePath.empty()) {
        fields.emplace_back("filePath", n.filePath);
    }
    if (!n.value.empty()) {
        fields.emplace_back("value", n.value);
    }
}

static std::string makeHeader(const std::string& nodeName,
                              const std::vector<std::pair<std::string, std::string>>& fields) {
    if (fields.empty()) return nodeName;

    std::string out = nodeName;
    out += "(";
    for (size_t i = 0; i < fields.size(); ++i) {
        out += std::format("{}: {}", fields[i].first, fields[i].second);
        if (i + 1 < fields.size()) out += ", ";
    }
    out += ")";
    return out;
}

// ---- printing fields in body ----
template <class TPtr>
static void appendPtrField(std::string& out, const char* name, const TPtr& p, int indent) {
    if (!p) {
        out += std::format("{}{}: null\n", ind(indent), name);
        return;
    }
    out += std::format("{}{}:\n", ind(indent), name);
    out += p->toString(indent + 2);
    out += "\n";
}

template <class TPtrVec>
static void appendPtrVec(std::string& out, const char* name, const TPtrVec& vec, int indent) {
    if (vec.empty()) {
        out += std::format("{}{}: []\n", ind(indent), name);
        return;
    }

    out += std::format("{}{}: [\n", ind(indent), name);
    for (const auto& p : vec) {
        if (p) out += p->toString(indent + 2);
        else   out += ind(indent + 2) + "null";
        out += "\n";
    }
    out += std::format("{}]\n", ind(indent));
}

} // namespace

// -------------------- leaf / basic --------------------

std::string LiteralNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this); // includes value if not empty
    return std::format("{}{}", ind(indent), makeHeader("Literal", hdr));
}

std::string VariableNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("varName", varName);
    return std::format("{}{}", ind(indent), makeHeader("Variable", hdr));
}

// -------------------- expressions --------------------

std::string AssignmentNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("op", op);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Assignment", hdr));
    appendPtrField(out, "variable", variable, indent + 2);
    appendPtrField(out, "value", value, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string MemberAccessNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("MemberAccess", hdr));
    appendPtrField(out, "parent", parent, indent + 2);
    appendPtrField(out, "val", val, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string BinaryOperationNode::toString(int indent) const {
    // operator stored in base `value`
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("BinaryOperation", hdr));
    appendPtrField(out, "leftOperand", leftOperand, indent + 2);
    appendPtrField(out, "rightOperand", rightOperand, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string RawTypeNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("RawType", hdr));
    appendPtrField(out, "varType", varType, indent + 2);
    appendPtrField(out, "varSize", varSize, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string UnaryOperationNode::toString(int indent) const {
    // operator stored in base `value`
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("UnaryOperation", hdr));
    appendPtrField(out, "operand", operand, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string CallExpressionNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("CallExpression", hdr));
    appendPtrField(out, "callee", callee, indent + 2);
    appendPtrVec(out, "arguments", arguments, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string LambdaNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Lambda", hdr));
    appendPtrVec(out, "params", params, indent + 2);
    appendPtrField(out, "body", body, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

// -------------------- statements / control flow --------------------

std::string BlockNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Block", hdr));
    appendPtrVec(out, "statements", statements, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string IfNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("If", hdr));
    appendPtrField(out, "condition", condition, indent + 2);
    appendPtrField(out, "thenBlock", thenBlock, indent + 2);
    appendPtrField(out, "elseBlock", elseBlock, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string SCDefaultNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("SCDefault", hdr));
    appendPtrField(out, "body", body, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string CaseNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Case", hdr));
    appendPtrField(out, "condition", condition, indent + 2);
    appendPtrField(out, "body", body, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string SwitchNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Switch", hdr));
    appendPtrField(out, "expression", expression, indent + 2);

    if (cases.empty()) {
        out += std::format("{}cases: []\n", ind(indent + 2));
    } else {
        out += std::format("{}cases: [\n", ind(indent + 2));
        for (const auto& c : cases) {
            out += (c ? c->toString(indent + 4) : ind(indent + 4) + "null");
            out += "\n";
        }
        out += std::format("{}]\n", ind(indent + 2));
    }

    if (!defaultCase) out += std::format("{}defaultCase: null\n", ind(indent + 2));
    else {
        out += std::format("{}defaultCase:\n", ind(indent + 2));
        out += defaultCase->toString(indent + 4);
        out += "\n";
    }

    out += std::format("{}}}", ind(indent));
    return out;
}

std::string ForLoopNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("ForLoop", hdr));
    appendPtrField(out, "variable", variable, indent + 2);
    appendPtrField(out, "iterable", iterable, indent + 2);
    appendPtrField(out, "body", body, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string WhileLoopNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("WhileLoop", hdr));
    appendPtrField(out, "condition", condition, indent + 2);
    appendPtrField(out, "body", body, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string BreakStatementNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    return std::format("{}{}", ind(indent), makeHeader("BreakStatement", hdr));
}

std::string ContinueStatementNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    return std::format("{}{}", ind(indent), makeHeader("ContinueStatement", hdr));
}

std::string ReturnStatementNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("ReturnStatement", hdr));
    appendPtrField(out, "expression", expression, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string ThrowStatementNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("ThrowStatement", hdr));
    appendPtrField(out, "expression", expression, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string TryCatchNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("TryCatch", hdr));
    appendPtrField(out, "tryBlock", tryBlock, indent + 2);
    appendPtrField(out, "exception", exception, indent + 2);
    appendPtrField(out, "catchBlock", catchBlock, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

// -------------------- composite data --------------------

std::string ArrayNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Array", hdr));
    appendPtrVec(out, "elements", elements, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string SetNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Set", hdr));
    appendPtrVec(out, "elements", elements, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string DictNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Dict", hdr));

    if (elements.empty()) {
        out += std::format("{}elements: []\n", ind(indent + 2));
    } else {
        out += std::format("{}elements: [\n", ind(indent + 2));
        for (const auto& kv : elements) {
            out += std::format("{}{{\n", ind(indent + 4));

            out += std::format("{}key:\n", ind(indent + 6));
            out += (kv.first ? kv.first->toString(indent + 8) : ind(indent + 8) + "null");
            out += "\n";

            out += std::format("{}value:\n", ind(indent + 6));
            out += (kv.second ? kv.second->toString(indent + 8) : ind(indent + 8) + "null");
            out += "\n";

            out += std::format("{}}}\n", ind(indent + 4));
        }
        out += std::format("{}]\n", ind(indent + 2));
    }

    out += std::format("{}}}", ind(indent));
    return out;
}

std::string TupleNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Tuple", hdr));
    appendPtrVec(out, "elements", elements, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string ResultNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("isError", b2s(isError));

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Result", hdr));
    appendPtrField(out, "t", t, indent + 2);
    appendPtrField(out, "e", e, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

// -------------------- higher structures --------------------

std::string ParameterNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("parameterName", parameterName);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Parameter", hdr));
    appendPtrField(out, "parameterRawType", parameterRawType, indent + 2);
    appendPtrField(out, "defaultValue", defaultValue, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string ModifierNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("modifier", modifierToString(modifier));

    return std::format("{}{}", ind(indent), makeHeader("Modifier", hdr));
}

std::string EnumMemberNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("name", name);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("EnumMember", hdr));
    appendPtrField(out, "value", value, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string EnumNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("name", name);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Enum", hdr));
    appendPtrVec(out, "decorators", decorators, indent + 2);
    appendPtrVec(out, "modifiers", modifiers, indent + 2);
    appendPtrVec(out, "elements", elements, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string InterfaceFieldNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    hdr.emplace_back("name", name);
    hdr.emplace_back("isNullable", b2s(isNullable));
    hdr.emplace_back("isFunction", b2s(isFunction));

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("InterfaceField", hdr));
    appendPtrField(out, "rawType", rawType, indent + 2);
    appendPtrVec(out, "parameters", parameters, indent + 2);
    appendPtrField(out, "returnType", returnType, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string InterfaceNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("name", name);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Interface", hdr));
    appendPtrVec(out, "decorators", decorators, indent + 2);
    appendPtrVec(out, "modifiers", modifiers, indent + 2);
    appendPtrVec(out, "elements", elements, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string FunctionNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("name", name);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Function", hdr));
    appendPtrVec(out, "decorators", decorators, indent + 2);
    appendPtrVec(out, "modifiers", modifiers, indent + 2);
    appendPtrVec(out, "parameters", parameters, indent + 2);
    appendPtrField(out, "returnType", returnType, indent + 2);
    appendPtrField(out, "body", body, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string DeclarationNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    hdr.emplace_back("isNullable", b2s(isNullable));
    hdr.emplace_back("isTypeInference", b2s(isTypeInference));

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Declaration", hdr));
    appendPtrVec(out, "decorators", decorators, indent + 2);
    appendPtrVec(out, "modifiers", modifiers, indent + 2);
    appendPtrField(out, "variable", variable, indent + 2);
    appendPtrField(out, "rawType", rawType, indent + 2);
    appendPtrField(out, "value", value, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string ClassNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("name", name);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Class", hdr));
    appendPtrField(out, "constructor", constructor, indent + 2);
    appendPtrField(out, "super", super, indent + 2);
    appendPtrVec(out, "decorators", decorators, indent + 2);
    appendPtrVec(out, "modifiers", modifiers, indent + 2);
    appendPtrVec(out, "fields", fields, indent + 2);
    appendPtrVec(out, "methods", methods, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string DecoratorNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("name", name);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Decorator", hdr));
    appendPtrVec(out, "decorators", decorators, indent + 2);
    appendPtrVec(out, "modifiers", modifiers, indent + 2);
    appendPtrVec(out, "parameters", parameters, indent + 2);
    appendPtrField(out, "body", body, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

std::string NamespaceNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Namespace", hdr));
    appendPtrField(out, "name", name, indent + 2);
    appendPtrVec(out, "body", body, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}

// -------------------- imports / module / preprocessor --------------------

std::string ImportNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);

    hdr.emplace_back("moduleName", moduleName);
    hdr.emplace_back("alias", alias);
    hdr.emplace_back("importType", importTypeToString(importType));

    return std::format("{}{}", ind(indent), makeHeader("Import", hdr));
}

std::string PreprocessorDirectiveNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("directive", directiveToString(directive));
    // value is base `ASTNode::value` and is included only if not empty via appendBaseHeaderFields()

    return std::format("{}{}", ind(indent), makeHeader("Preprocessor", hdr));
}

std::string ModuleNode::toString(int indent) const {
    std::vector<std::pair<std::string, std::string>> hdr;
    appendBaseHeaderFields(hdr, *this);
    hdr.emplace_back("moduleName", moduleName);

    std::string out = std::format("{}{} {{\n", ind(indent), makeHeader("Module", hdr));
    appendPtrVec(out, "body", body, indent + 2);
    out += std::format("{}}}", ind(indent));
    return out;
}
