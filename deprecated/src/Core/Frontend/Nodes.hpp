#pragma once
#include <vector>
#include <string>
#include <variant>

#include "../../HelperFunctions.hpp"

enum struct ASTNodeType {
    Literal, Variable, MemberAccess, Declaration, Assignment, BinaryOperation, UnaryOperation, CallExpression,
    Block, IfStatement, ForLoop, WhileLoop, TryCatch, ReturnStatement, 
    Function, Class, Namespace,
    Parameter, Modifier,
    Switch, Case, SCDefault,
    Module,
    Import, Decorator, Preprocessor, 
    BreakStatement, ContinueStatement, ThrowStatement,
    Array, Set, Dict, Tuple, Void, Result, Enum, Interface, Lambda,
    EnumMember, InterfaceField,
    RawType, GenericParameter,
};

enum struct ASTModifierType {
    Public, Private, Protected, Static, Const, Override, Async, Debug,
    Intrinsic
};

enum struct ASTPreprocessorDirectiveType {
    Import, Unsafe, Macro, None
};

enum struct ASTImportType {
    /*
    Native - from dependencies
    Relative - relative to path
    Foreign - imported from other language via langpacks.
    ForeignRelative - imported from other language via langpacks, but with relative path.
    */
    Native, Relative, Foreign, ForeignRelative,
};

struct GenericParameter {
    std::string name;
    int line = 0;
    int column = 0;
    std::string filePath = "";
};

struct ASTNode {
    ASTNodeType type;
    std::string value; // for basic values like literals, etc.

    // Tracking the node for the ErrorManager purposes
    int line = 0;
    int column = 0;
    std::string filePath = "";

    virtual ~ASTNode();
    virtual std::string toString(int indent) const = 0;
};

// All nodes available in Neoluma

// This node only represents the existence of a variable (its name). Type info and initialized value are in DeclarationNode.
struct VariableNode : ASTNode {
    std::string varName;

    VariableNode(const std::string& varName): varName(varName) {
        this->type = ASTNodeType::Variable;
    }

    std::string toString(int indent) const override;
};

struct LiteralNode : ASTNode {
    LiteralNode(const std::string& val = "") {
        this->type = ASTNodeType::Literal;
        value = val;
    }

    // TODO: add string statements to support inline data in strings
    //std::optional<std::vector<MemoryPtr<VariableNode>>> stringStatements = std::nullopt;
    std::string toString(int indent) const override;
};

// Assignment node assigns a value to an existing variable.
struct AssignmentNode : ASTNode {
    MemoryPtr<ASTNode> variable;
    std::string op; // Assignment operator
    MemoryPtr<ASTNode> value;

    AssignmentNode(MemoryPtr<ASTNode> variable, const std::string& op, MemoryPtr<ASTNode> value)
        : variable(std::move(variable)), op(op), value(std::move(value)) {
        this->type = ASTNodeType::Assignment;
    }

    std::string toString(int indent) const override;
};

struct MemberAccessNode : ASTNode {
    MemoryPtr<ASTNode> parent;
    MemoryPtr<ASTNode> val;

    MemberAccessNode(MemoryPtr<ASTNode> parent, MemoryPtr<ASTNode> val)
        : parent(std::move(parent)), val(std::move(val)) {
        this->type = ASTNodeType::MemberAccess;
    }

    std::string toString(int indent) const override;
};

struct BinaryOperationNode : ASTNode {
    MemoryPtr<ASTNode> leftOperand;
    MemoryPtr<ASTNode> rightOperand;

    BinaryOperationNode(MemoryPtr<ASTNode> leftOp, const std::string& op, MemoryPtr<ASTNode> rightOp)
        : leftOperand(std::move(leftOp)), rightOperand(std::move(rightOp)) {
        this->type = ASTNodeType::BinaryOperation;
        value = op;
    }

    std::string toString(int indent) const override;
};



struct RawTypeNode : ASTNode {
    MemoryPtr<VariableNode> varType;
    // ASTNode is used only for nullptr. Be aware!
    MemoryPtr<ASTNode> varSize;

    // array<str>, dict<str, int>, Box<T>
    std::vector<MemoryPtr<RawTypeNode>> genericArguments;

    RawTypeNode(MemoryPtr<VariableNode> varType, MemoryPtr<ASTNode> varSize = nullptr,
        std::vector<MemoryPtr<RawTypeNode>> genericArguments = {})
    : varType(std::move(varType)), varSize(std::move(varSize)), genericArguments(std::move(genericArguments)) {
        this->type = ASTNodeType::RawType;
    }

    std::string toString(int indent) const override;
};

struct UnaryOperationNode : ASTNode {
    MemoryPtr<ASTNode> operand;

    UnaryOperationNode(const std::string& op, MemoryPtr<ASTNode> operand)
        : operand(std::move(operand)) {
        this->type = ASTNodeType::UnaryOperation;
        value = op;
    }

    std::string toString(int indent) const override;
};

// statements
struct BlockNode : ASTNode {
    std::vector<MemoryPtr<ASTNode>> statements;
    BlockNode() { this->type = ASTNodeType::Block; }

    std::string toString(int indent) const override;
};

struct IfNode : ASTNode {
    MemoryPtr<ASTNode> condition;
    MemoryPtr<ASTNode> thenBlock;
    MemoryPtr<ASTNode> elseBlock;

    IfNode(MemoryPtr<ASTNode> condition, MemoryPtr<ASTNode> thenBlock, MemoryPtr<ASTNode> elseBlock = nullptr)
        : condition(std::move(condition)), thenBlock(std::move(thenBlock)), elseBlock(std::move(elseBlock)) {
        this->type = ASTNodeType::IfStatement;
    }

    std::string toString(int indent) const override;
};

struct SCDefaultNode : ASTNode {
    MemoryPtr<ASTNode> body;
    SCDefaultNode(MemoryPtr<ASTNode> body) : body(std::move(body)) {
        this->type = ASTNodeType::SCDefault;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct CaseNode : ASTNode {
    MemoryPtr<ASTNode> condition;
    MemoryPtr<ASTNode> body;
    CaseNode(MemoryPtr<ASTNode> condition, MemoryPtr<ASTNode> body)
        : condition(std::move(condition)), body(std::move(body)) {
        this->type = ASTNodeType::Case;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct SwitchNode : ASTNode {
    MemoryPtr<ASTNode> expression;
    std::vector<MemoryPtr<CaseNode>> cases;
    MemoryPtr<SCDefaultNode> defaultCase;

    SwitchNode(MemoryPtr<ASTNode> expression, std::vector<MemoryPtr<CaseNode>> cases, MemoryPtr<SCDefaultNode> defaultCase = nullptr)
        : expression(std::move(expression)), cases(std::move(cases)), defaultCase(std::move(defaultCase)) {
        this->type = ASTNodeType::Switch;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct ForLoopNode : ASTNode {
    MemoryPtr<VariableNode> variable;
    MemoryPtr<ASTNode> iterable;
    MemoryPtr<BlockNode> body;

    ForLoopNode(MemoryPtr<VariableNode> variable, MemoryPtr<ASTNode> iterable, MemoryPtr<BlockNode> body)
        : variable(std::move(variable)), iterable(std::move(iterable)), body(std::move(body)) {
        this->type = ASTNodeType::ForLoop;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct WhileLoopNode : ASTNode {
    MemoryPtr<ASTNode> condition;
    MemoryPtr<BlockNode> body;

    WhileLoopNode(MemoryPtr<ASTNode> condition, MemoryPtr<BlockNode> body)
        : condition(std::move(condition)), body(std::move(body)) {
        this->type = ASTNodeType::WhileLoop;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct BreakStatementNode : ASTNode {
    BreakStatementNode() { this->type = ASTNodeType::BreakStatement; }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct ContinueStatementNode : ASTNode {
    ContinueStatementNode() { this->type = ASTNodeType::ContinueStatement; }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct ReturnStatementNode : ASTNode {
    MemoryPtr<ASTNode> expression;
    ReturnStatementNode(MemoryPtr<ASTNode> expression)
        : expression(std::move(expression)) {
        this->type = ASTNodeType::ReturnStatement;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct ThrowStatementNode : ASTNode {
    MemoryPtr<ASTNode> expression;
    ThrowStatementNode(MemoryPtr<ASTNode> expression)
        : expression(std::move(expression)) {
        this->type = ASTNodeType::ThrowStatement;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct TryCatchNode : ASTNode {
    MemoryPtr<BlockNode> tryBlock;
    MemoryPtr<BlockNode> catchBlock;
    MemoryPtr<VariableNode> exception;

    TryCatchNode(MemoryPtr<BlockNode> tryBlock, MemoryPtr<VariableNode> exception, MemoryPtr<BlockNode> catchBlock)
        : tryBlock(std::move(tryBlock)), exception(std::move(exception)), catchBlock(std::move(catchBlock)) {
        this->type = ASTNodeType::TryCatch;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

// composite data
struct ArrayNode : ASTNode {
    std::vector<MemoryPtr<ASTNode>> elements;
    /*MemoryPtr<ASTNode> typeHint;*/

    ArrayNode(std::vector<MemoryPtr<ASTNode>> elements/*, MemoryPtr<ASTNode> typeHint=nullptr*/)
        : elements(std::move(elements))/*, typeHint(std::move(typeHint))*/ {
        this->type = ASTNodeType::Array;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct SetNode : ASTNode {
    std::vector<MemoryPtr<ASTNode>> elements;
    //MemoryPtr<RawTypeNode> typeHint;

    SetNode(std::vector<MemoryPtr<ASTNode>> elements/*, MemoryPtr<ASTNode> typeHint=nullptr*/)
        : elements(std::move(elements))/*, typeHint(std::move(typeHint))*/ {
        this->type = ASTNodeType::Set;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct DictNode : ASTNode {
    std::vector<std::pair<MemoryPtr<ASTNode>, MemoryPtr<ASTNode>>> elements;
    //std::array<MemoryPtr<RawTypeNode>, 2> types;

    DictNode(std::vector<std::pair<MemoryPtr<ASTNode>, MemoryPtr<ASTNode>>> elements/*, std::array<MemoryPtr<RawTypeNode>, 2> types*/)
        : elements(std::move(elements))/*, types(std::move(types))*/ {
        this->type = ASTNodeType::Dict;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

// For now it's used only in lambda conditions, it must be fixed later
struct TupleNode : ASTNode {
    std::vector<MemoryPtr<ASTNode>> elements;

    TupleNode(std::vector<MemoryPtr<ASTNode>> elements)
        : elements(std::move(elements)) {
        this->type = ASTNodeType::Tuple;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent) const override;
};

struct ResultNode : ASTNode {
    MemoryPtr<ASTNode> t;
    MemoryPtr<ASTNode> e;
    bool isError;

    ResultNode(MemoryPtr<ASTNode> t, MemoryPtr<ASTNode> e = nullptr, bool isError = false)
        : t(std::move(t)), e(std::move(e)), isError(isError) {
        this->type = ASTNodeType::Result;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

// higher structures
struct ParameterNode : ASTNode {
    std::string parameterName;
    MemoryPtr<RawTypeNode> parameterRawType = nullptr;
    MemoryPtr<ASTNode> defaultValue = nullptr; // optional

    ParameterNode(const std::string& parameterName, MemoryPtr<RawTypeNode> parameterRawType, MemoryPtr<ASTNode> defaultValue)
        : parameterName(parameterName), parameterRawType(std::move(parameterRawType)), defaultValue(std::move(defaultValue)) {
        this->type = ASTNodeType::Parameter;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct ModifierNode : ASTNode {
    ASTModifierType modifier;
    ModifierNode(ASTModifierType& modifier) : modifier(modifier) {
        this->type = ASTNodeType::Modifier;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct CallExpressionNode : ASTNode {
    MemoryPtr<ASTNode> callee;
    std::vector<MemoryPtr<ASTNode>> arguments;
    bool isDecoratorCall = false;

    CallExpressionNode(MemoryPtr<ASTNode> callee, std::vector<MemoryPtr<ASTNode>> arguments, bool isDecoratorCall = false)
        : callee(std::move(callee)), arguments(std::move(arguments)), isDecoratorCall(isDecoratorCall) {
        this->type = ASTNodeType::CallExpression;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct EnumMemberNode : ASTNode {
    std::string name;
    MemoryPtr<LiteralNode> value; // TODO: Maybe allow expressions in the future that would return a number.
    EnumMemberNode(const std::string& name, MemoryPtr<LiteralNode> value = nullptr) : name(name), value(std::move(value)) {
        this->type = ASTNodeType::EnumMember;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct EnumNode : ASTNode {
    std::string name;
    std::vector<MemoryPtr<CallExpressionNode>> decorators;
    std::vector<MemoryPtr<ModifierNode>> modifiers;
    std::vector<MemoryPtr<EnumMemberNode>> elements;

    EnumNode(const std::string& name, std::vector<MemoryPtr<EnumMemberNode>> elements, std::vector<MemoryPtr<CallExpressionNode>> decorators = std::vector<MemoryPtr<CallExpressionNode>>{}, std::vector<MemoryPtr<ModifierNode>> modifiers = std::vector<MemoryPtr<ModifierNode>>{})
    : name(name) {
        this->type = ASTNodeType::Enum;
        this->elements = std::move(elements);
        this->decorators = std::move(decorators);
        this->modifiers = std::move(modifiers);
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct InterfaceFieldNode : ASTNode {
    std::string name;
    MemoryPtr<RawTypeNode> rawType;
    bool isNullable;

    bool isFunction = false;
    std::vector<MemoryPtr<ParameterNode>> parameters = {};
    MemoryPtr<VariableNode> returnType = nullptr;

    InterfaceFieldNode(const std::string& name, MemoryPtr<RawTypeNode> type, bool isNullable = false, bool isFunction = false, std::vector<MemoryPtr<ParameterNode>> parameters = {}, MemoryPtr<VariableNode> returnType = nullptr)
        : name(name), rawType(std::move(type)), isNullable(isNullable), isFunction(isFunction), parameters(std::move(parameters)), returnType(std::move(returnType)) {
        this->type = ASTNodeType::InterfaceField;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct InterfaceNode : ASTNode {
    std::string name;
    std::vector<MemoryPtr<CallExpressionNode>> decorators;
    std::vector<MemoryPtr<ModifierNode>> modifiers;
    std::vector<GenericParameter> genericParameters;
    std::vector<MemoryPtr<InterfaceFieldNode>> elements;

    InterfaceNode(const std::string& name, std::vector<MemoryPtr<InterfaceFieldNode>> elements, std::vector<MemoryPtr<CallExpressionNode>> decorators = {}, std::vector<MemoryPtr<ModifierNode>> modifiers = {}, std::vector<GenericParameter> genericParameters = {})
    : name(name), elements(std::move(elements)), decorators(std::move(decorators)), modifiers(std::move(modifiers)), genericParameters(std::move(genericParameters)) {
        this->type = ASTNodeType::Interface;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct LambdaNode : ASTNode {
    std::vector<MemoryPtr<ASTNode>> params;
    MemoryPtr<ASTNode> body;

    LambdaNode(std::vector<MemoryPtr<ASTNode>> params, MemoryPtr<ASTNode> body)
        : params(std::move(params)), body(std::move(body)) {
        this->type = ASTNodeType::Lambda;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct FunctionNode : ASTNode {
    std::vector<MemoryPtr<CallExpressionNode>> decorators;
    std::vector<MemoryPtr<ModifierNode>> modifiers;
    std::vector<GenericParameter> genericParameters;

    std::string name;
    std::vector<MemoryPtr<ParameterNode>> parameters;
    MemoryPtr<RawTypeNode> returnType = nullptr;
    MemoryPtr<BlockNode> body;
    bool isIntrinsic = false; // Is this a function that passes through an LLVM call?

    FunctionNode(const std::string& name, std::vector<MemoryPtr<ParameterNode>> parameters, MemoryPtr<RawTypeNode> returnType, MemoryPtr<BlockNode> body,std::vector<MemoryPtr<CallExpressionNode>> decorators = {}, std::vector<MemoryPtr<ModifierNode>> modifiers = {}, std::vector<GenericParameter> genericParameters = {})
        : name(name), parameters(std::move(parameters)), body(std::move(body)), decorators(std::move(decorators)), modifiers(std::move(modifiers)), genericParameters(std::move(genericParameters)), returnType(std::move(returnType)) {
        this->type = ASTNodeType::Function;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

// Declaration node holds type info, initialization value, and other metadata about a variable.
struct DeclarationNode : ASTNode {
    std::vector<MemoryPtr<CallExpressionNode>> decorators;
    std::vector<MemoryPtr<ModifierNode>> modifiers;

    MemoryPtr<VariableNode> variable;
    bool isNullable = false;
    MemoryPtr<RawTypeNode> rawType;
    bool isTypeInference = false;
    MemoryPtr<ASTNode> value = nullptr;

    DeclarationNode(MemoryPtr<VariableNode> variable, MemoryPtr<RawTypeNode> rawType = nullptr, MemoryPtr<ASTNode> value = nullptr, bool isNullable = false, bool isTypeInference = false, std::vector<MemoryPtr<CallExpressionNode>> decorators = {}, std::vector<MemoryPtr<ModifierNode>> modifiers = {})
    : variable(std::move(variable)), rawType(std::move(rawType)), value(std::move(value)), isNullable(isNullable), isTypeInference(isTypeInference), decorators(std::move(decorators)), modifiers(std::move(modifiers)) {
        this->type = ASTNodeType::Declaration;
    }

    std::string toString(int indent) const override;
};

struct ClassNode : ASTNode {
    std::string name;
    MemoryPtr<FunctionNode> constructor = nullptr;
    MemoryPtr<VariableNode> super = nullptr; // Name of class or interface being inherited from, if any
    std::vector<MemoryPtr<CallExpressionNode>> decorators;
    std::vector<MemoryPtr<ModifierNode>> modifiers;
    std::vector<GenericParameter> genericParameters;
    std::vector<MemoryPtr<DeclarationNode>> fields;
    std::vector<MemoryPtr<FunctionNode>> methods;

    ClassNode(const std::string& name, MemoryPtr<FunctionNode> constructor, MemoryPtr<VariableNode> super, std::vector<MemoryPtr<DeclarationNode>> fields, std::vector<MemoryPtr<FunctionNode>> methods,
              std::vector<MemoryPtr<CallExpressionNode>> decorators = {}, std::vector<MemoryPtr<ModifierNode>> modifiers = {}, std::vector<GenericParameter> genericParameters = {})
        : name(name), constructor(std::move(constructor)), super(std::move(super)), fields(std::move(fields)), methods(std::move(methods)), decorators(std::move(decorators)), modifiers(std::move(modifiers)), genericParameters(std::move(genericParameters)) {
        this->type = ASTNodeType::Class;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct DecoratorNode : ASTNode {
    std::string name;
    std::vector<MemoryPtr<CallExpressionNode>> decorators;
    std::vector<MemoryPtr<ModifierNode>> modifiers;
    std::vector<MemoryPtr<ParameterNode>> parameters;
    MemoryPtr<BlockNode> body;

    DecoratorNode(const std::string& name, std::vector<MemoryPtr<ParameterNode>> parameters, MemoryPtr<BlockNode> body,
                 std::vector<MemoryPtr<CallExpressionNode>> decorators = std::vector<MemoryPtr<CallExpressionNode>>{}, std::vector<MemoryPtr<ModifierNode>> modifiers = std::vector<MemoryPtr<ModifierNode>>{})
        : name(name), parameters(std::move(parameters)), body(std::move(body)), decorators(std::move(decorators)), modifiers(std::move(modifiers)) {
        this->type = ASTNodeType::Decorator;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct NamespaceNode : ASTNode {
    MemoryPtr<ASTNode> name;
    std::vector<MemoryPtr<ASTNode>> body;

    NamespaceNode(MemoryPtr<ASTNode> name, std::vector<MemoryPtr<ASTNode>> body)
        : name(std::move(name)), body(std::move(body)) {
        this->type = ASTNodeType::Namespace;
    }

    std::string toString(int indent = 0) const override;
};

// imports and program structure
struct ImportNode : ASTNode {
    std::string moduleName;
    std::string alias;
    ASTImportType importType;

    ImportNode(const std::string& moduleName, const std::string& alias, ASTImportType importType)
        : moduleName(moduleName), alias(alias), importType(importType) {
        this->type = ASTNodeType::Import;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct PreprocessorDirectiveNode : ASTNode {
    ASTPreprocessorDirectiveType directive;
    PreprocessorDirectiveNode(ASTPreprocessorDirectiveType& directive, const std::string& value = "")
        : directive(directive) {
        this->type = ASTNodeType::Preprocessor;
        this->value = value;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};

struct ModuleNode : ASTNode {
    std::string moduleName;
    std::vector<MemoryPtr<ASTNode>> body;
    ModuleNode(const std::string& name) : moduleName(name) {
        this->type = ASTNodeType::Module;
    }

    // Suggested by AI. If it fails, it's his fault
    std::string toString(int indent = 0) const override;
};