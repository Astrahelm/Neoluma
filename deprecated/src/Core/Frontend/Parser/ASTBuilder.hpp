#pragma once
#include "../../../HelperFunctions.hpp"
#include "../Nodes.hpp"

struct ASTBuilder {
    // Creates a LiteralNode
    static MemoryPtr<LiteralNode> createLiteral(const std::string& val = "") {
        return makeMemoryPtr<LiteralNode>(val);
    }

    // Creates a VariableNode
    static MemoryPtr<VariableNode> createVariable(const std::string& varName) {
        return makeMemoryPtr<VariableNode>(varName);
    }

    // Creates a DeclarationNode
    static MemoryPtr<DeclarationNode> createDeclaration(MemoryPtr<VariableNode> variable, MemoryPtr<RawTypeNode> rawType = nullptr, MemoryPtr<ASTNode> value = nullptr, bool isNullable = false, bool isTypeInference = false, std::vector<MemoryPtr<CallExpressionNode>> decorators = {}, std::vector<MemoryPtr<ModifierNode>> modifiers = {}) {
        return makeMemoryPtr<DeclarationNode>(std::move(variable), std::move(rawType), std::move(value), isNullable, isTypeInference, std::move(decorators), std::move(modifiers));
    }

    // Creates an AssignmentNode
    static MemoryPtr<AssignmentNode> createAssignment(MemoryPtr<ASTNode> variable, const std::string& op, MemoryPtr<ASTNode> value) {
        return makeMemoryPtr<AssignmentNode>(std::move(variable), op, std::move(value));
    }

    // Creates a MemberAccessNode
    static MemoryPtr<MemberAccessNode> createMemberAccess(MemoryPtr<ASTNode> parent, MemoryPtr<ASTNode> val) {
        return makeMemoryPtr<MemberAccessNode>(std::move(parent), std::move(val));
    }

    // Creates a BinaryOperationNode
    static MemoryPtr<BinaryOperationNode> createBinaryOperation(MemoryPtr<ASTNode> leftOp, const std::string& op, MemoryPtr<ASTNode> rightOp) {
        return makeMemoryPtr<BinaryOperationNode>(std::move(leftOp), op, std::move(rightOp));
    }

    // Creates a UnaryOperationNode
    static MemoryPtr<UnaryOperationNode> createUnaryOperation(const std::string& op, MemoryPtr<ASTNode> operand) {
        return makeMemoryPtr<UnaryOperationNode>(op, std::move(operand));
    }

    // Creates a BlockNode
    static MemoryPtr<BlockNode> createBlock(std::vector<MemoryPtr<ASTNode>> statements) {
        auto block = makeMemoryPtr<BlockNode>();
        block->statements = std::move(statements);
        return block;
    }

    // Creates an IfNode
    static MemoryPtr<IfNode> createIf(MemoryPtr<ASTNode> condition, MemoryPtr<ASTNode> thenBlock, MemoryPtr<ASTNode> elseBlock = nullptr) {
        return makeMemoryPtr<IfNode>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
    }

    // Creates a CaseNode
    static MemoryPtr<CaseNode> createCase(MemoryPtr<ASTNode> condition, MemoryPtr<ASTNode> body) {
        return makeMemoryPtr<CaseNode>(std::move(condition), std::move(body));
    }

    // Creates an SCDefaultNode
    static MemoryPtr<SCDefaultNode> createDefaultCase(MemoryPtr<ASTNode> body) {
        return makeMemoryPtr<SCDefaultNode>(std::move(body));
    }

    // Creates a SwitchNode
    static MemoryPtr<SwitchNode> createSwitch(MemoryPtr<ASTNode> expression, std::vector<MemoryPtr<CaseNode>> cases, MemoryPtr<SCDefaultNode> defaultCase = nullptr) {
        return makeMemoryPtr<SwitchNode>(std::move(expression), std::move(cases), std::move(defaultCase));
    }

    // Creates a ForLoopNode
    static MemoryPtr<ForLoopNode> createForLoop(MemoryPtr<VariableNode> variable, MemoryPtr<ASTNode> iterable, MemoryPtr<BlockNode> body) {
        return makeMemoryPtr<ForLoopNode>(std::move(variable), std::move(iterable), std::move(body));
    }

    // Creates a WhileLoopNode
    static MemoryPtr<WhileLoopNode> createWhileLoop(MemoryPtr<ASTNode> condition, MemoryPtr<BlockNode> body) {
        return makeMemoryPtr<WhileLoopNode>(std::move(condition), std::move(body));
    }

    // Creates a BreakStatementNode
    static MemoryPtr<BreakStatementNode> createBreakStatement() {
        return makeMemoryPtr<BreakStatementNode>();
    }

    // Creates a ContinueStatementNode
    static MemoryPtr<ContinueStatementNode> createContinueStatement() {
        return makeMemoryPtr<ContinueStatementNode>();
    }

    // Creates a ReturnStatementNode
    static MemoryPtr<ReturnStatementNode> createReturnStatement(MemoryPtr<ASTNode> expression) {
        return makeMemoryPtr<ReturnStatementNode>(std::move(expression));
    }

    // Creates a ThrowStatementNode
    static MemoryPtr<ThrowStatementNode> createThrowStatement(MemoryPtr<ASTNode> expression) {
        return makeMemoryPtr<ThrowStatementNode>(std::move(expression));
    }

    // Creates a TryCatchNode
    static MemoryPtr<TryCatchNode> createTryCatch(MemoryPtr<BlockNode> tryBlock, MemoryPtr<VariableNode> exception, MemoryPtr<BlockNode> catchBlock) {
        return makeMemoryPtr<TryCatchNode>(std::move(tryBlock), std::move(exception), std::move(catchBlock));
    }

    // Creates an ArrayNode
    static MemoryPtr<ArrayNode> createArray(std::vector<MemoryPtr<ASTNode>> elements/*, MemoryPtr<ASTNode> typeHint = nullptr*/) {
        return makeMemoryPtr<ArrayNode>(std::move(elements)/*, std::move(typeHint)*/);
    }

    // Creates a SetNode
    static MemoryPtr<SetNode> createSet(std::vector<MemoryPtr<ASTNode>> elements/*, MemoryPtr<ASTNode> typeHint = nullptr*/) {
        return makeMemoryPtr<SetNode>(std::move(elements)/*, std::move(typeHint)*/);
    }

    // Creates a DictNode
    static MemoryPtr<DictNode> createDict(std::vector<std::pair<MemoryPtr<ASTNode>, MemoryPtr<ASTNode>>> elements/*, std::array<std::string, 2> types={"none", "none"}*/) {
        return makeMemoryPtr<DictNode>(std::move(elements)/*, std::move(types)*/);
    }

    // Creates a TupleNode
    static MemoryPtr<TupleNode> createTuple(std::vector<MemoryPtr<ASTNode>> elements) {
        return makeMemoryPtr<TupleNode>(std::move(elements));
    }

    // Creates a ResultNode
    static MemoryPtr<ResultNode> createResult(MemoryPtr<ASTNode> t, MemoryPtr<ASTNode> e = nullptr, bool isError = false) {
        return makeMemoryPtr<ResultNode>(std::move(t), std::move(e), isError);
    }

    // Creates a ParameterNode
    static MemoryPtr<ParameterNode> createParameter(const std::string& parameterName, MemoryPtr<RawTypeNode> parameterRawType = nullptr, MemoryPtr<ASTNode> defaultValue = nullptr) {
        return makeMemoryPtr<ParameterNode>(parameterName, std::move(parameterRawType), std::move(defaultValue));
    }

    // Creates a ModifierNode
    static MemoryPtr<ModifierNode> createModifier(ASTModifierType modifier) {
        return makeMemoryPtr<ModifierNode>(modifier);
    }

    // Creates a CallExpressionNode
    static MemoryPtr<CallExpressionNode> createCallExpression(MemoryPtr<ASTNode> callee, std::vector<MemoryPtr<ASTNode>> arguments, bool isDecoratorCall = false) {
        return makeMemoryPtr<CallExpressionNode>(std::move(callee), std::move(arguments), isDecoratorCall);
    }

    // Creates an EnumMemberNode
    static MemoryPtr<EnumMemberNode> createEnumMember(const std::string& name, MemoryPtr<LiteralNode> value = nullptr) {
        return makeMemoryPtr<EnumMemberNode>(name, std::move(value));
    }

    // Creates an EnumNode
    static MemoryPtr<EnumNode> createEnum(const std::string& name, std::vector<MemoryPtr<EnumMemberNode>> elements, std::vector<MemoryPtr<CallExpressionNode>> decorators = {}, std::vector<MemoryPtr<ModifierNode>> modifiers = {}) {
        return makeMemoryPtr<EnumNode>(name, std::move(elements), std::move(decorators), std::move(modifiers));
    }

    // Creates an InterfaceFieldNode
    static MemoryPtr<InterfaceFieldNode> createInterfaceField(const std::string& name, MemoryPtr<RawTypeNode> type, bool isNullable = false, bool isFunction = false, std::vector<MemoryPtr<ParameterNode>> parameters = {}, MemoryPtr<VariableNode> returnType = nullptr) {
        return makeMemoryPtr<InterfaceFieldNode>(name, std::move(type), isNullable, isFunction, std::move(parameters), std::move(returnType));
    }

    // Creates an InterfaceNode
    static MemoryPtr<InterfaceNode> createInterface(const std::string& name, std::vector<MemoryPtr<InterfaceFieldNode>> elements, std::vector<MemoryPtr<CallExpressionNode>> decorators = {}, std::vector<MemoryPtr<ModifierNode>> modifiers = {}, std::vector<GenericParameter> genericParameters = {}) {
        return makeMemoryPtr<InterfaceNode>(name, std::move(elements), std::move(decorators), std::move(modifiers), std::move(genericParameters));
    }

    // Creates a LambdaNode
    static MemoryPtr<LambdaNode> createLambda(std::vector<MemoryPtr<ASTNode>> params, MemoryPtr<ASTNode> body) {
        return makeMemoryPtr<LambdaNode>(std::move(params), std::move(body));
    }

    // Creates a FunctionNode
    static MemoryPtr<FunctionNode> createFunction(const std::string& name, std::vector<MemoryPtr<ParameterNode>> parameters, MemoryPtr<RawTypeNode> returnType, MemoryPtr<BlockNode> body, std::vector<MemoryPtr<CallExpressionNode>> decorators = {}, std::vector<MemoryPtr<ModifierNode>> modifiers = {}, std::vector<GenericParameter> genericParameters = {}) {
        return makeMemoryPtr<FunctionNode>(name, std::move(parameters), std::move(returnType), std::move(body), std::move(decorators), std::move(modifiers), std::move(genericParameters));
    }

    // Creates a ClassNode
    static MemoryPtr<ClassNode> createClass(const std::string& name, MemoryPtr<FunctionNode> constructor, MemoryPtr<VariableNode> super, std::vector<MemoryPtr<DeclarationNode>> fields, std::vector<MemoryPtr<FunctionNode>> methods, std::vector<MemoryPtr<CallExpressionNode>> decorators = {}, std::vector<MemoryPtr<ModifierNode>> modifiers = {}, std::vector<GenericParameter> genericParameters = {}) {
        return makeMemoryPtr<ClassNode>(name, std::move(constructor), std::move(super), std::move(fields), std::move(methods), std::move(decorators), std::move(modifiers), std::move(genericParameters));
    }

    // Creates a DecoratorNode
    static MemoryPtr<DecoratorNode> createDecorator(const std::string& name, std::vector<MemoryPtr<ParameterNode>> parameters, MemoryPtr<BlockNode> body, std::vector<MemoryPtr<CallExpressionNode>> decorators = {}, std::vector<MemoryPtr<ModifierNode>> modifiers = {}) {
        return makeMemoryPtr<DecoratorNode>(name, std::move(parameters), std::move(body), std::move(decorators), std::move(modifiers));
    }

    // Creates a NamespaceNode
    static MemoryPtr<NamespaceNode> createNamespace(MemoryPtr<ASTNode> name, std::vector<MemoryPtr<ASTNode>> body) {
        return makeMemoryPtr<NamespaceNode>(std::move(name), std::move(body));
    }

    // Creates an ImportNode
    static MemoryPtr<ImportNode> createImport(const std::string& moduleName, const std::string& alias, ASTImportType importType) {
        return makeMemoryPtr<ImportNode>(moduleName, alias, importType);
    }

    // Creates a PreprocessorDirectiveNode
    static MemoryPtr<PreprocessorDirectiveNode> createPreprocessor(ASTPreprocessorDirectiveType directive, const std::string& value = "") {
        return makeMemoryPtr<PreprocessorDirectiveNode>(directive, value);
    }

    // Creates a RawTypeNode
    static MemoryPtr<RawTypeNode> createRawType(MemoryPtr<VariableNode> varType, MemoryPtr<ASTNode> varSize = nullptr, std::vector<MemoryPtr<RawTypeNode>> genericArguments = {}) {
        return makeMemoryPtr<RawTypeNode>(std::move(varType), std::move(varSize), std::move(genericArguments));
    }

    // Creates a ModuleNode
    static MemoryPtr<ModuleNode> createModule(const std::string& name, std::vector<MemoryPtr<ASTNode>> body = {}) {
        auto mod = makeMemoryPtr<ModuleNode>(name);
        mod->body = std::move(body);
        return mod;
    }
};

