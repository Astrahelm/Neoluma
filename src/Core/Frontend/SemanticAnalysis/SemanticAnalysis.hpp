#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "Core/Extras/ErrorManager/ErrorManager.hpp"
#include "Core/Frontend/Nodes.hpp"
#include "Core/Frontend/Token.hpp"
#include "Core/Frontend/Orchestrator/Orchestrator.hpp"

struct Program;

// Semantic Analysis is a part of Frontend in Compiler responsible for logical part of the code.
struct SemanticAnalysis {
    // ErrorManager is used to report errors
    ErrorManager* errorManager = nullptr;

    // Main entry
    void analyzeProgram(Program& program);

    // Per-node analyzers
    void analyzeModule(ModuleNode* module);
    void analyzeBlock(BlockNode* block);
    void analyzeDeclaration(DeclarationNode* node);
    void analyzeAssignment(AssignmentNode* node);
    void analyzeFunction(FunctionNode* node);
    void analyzeClass(ClassNode* node);
    void analyzeEnum(EnumNode* node);
    void analyzeInterface(InterfaceNode* node);
    void analyzeCallExpression(CallExpressionNode* node);
    void analyzeDecorator(DecoratorNode* node);
    void analyzeIf(IfNode* node);
    void analyzeSwitch(SwitchNode* node);
    void analyzeWhile(WhileLoopNode* node);
    void analyzeFor(ForLoopNode* node);
    void analyzeTryCatch(TryCatchNode* node);
    void analyzeReturn(ReturnStatementNode* node);
    void analyzeThrow(ThrowStatementNode* node);
    void analyzeBreak(BreakStatementNode* node);
    void analyzeContinue(ContinueStatementNode* node);
    void analyzeLambda(LambdaNode* node);
    void analyzeExpression(ASTNode* node); // dispatcher for expressions only

    void analyzeStatement(ASTNode* node);
    ResolvedType resolveType(RawTypeNode* type);

private:
    struct Symbol {
        enum class Kind { Variable, Function, Parameter, Class, Enum, Interface, Decorator };
        Kind kind;
        bool isConst = false;
        std::string filePath;
        int line = 0, column = 0;
    };

    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    int loopDepth = 0;
    int functionDepth = 0;

    // Scope helpers
    void pushScope();
    void popScope();
    bool declareName(const std::string& name, Symbol symbol, ASTNode* node);
    Symbol* findName(const std::string& name);

    // just helpers
    bool match(ASTNode* node, ASTNodeType type) {
        if (node->type == type) return true;
        return false;
    }

    // Recursively goes through MemberAccessNode until it finds the first ever parent VariableNode.
    // if VariableNode is passed it returns it; used for analyzeAssignment().
    ASTNode* getRootVariable(ASTNode* node) {
        if (match(node, ASTNodeType::MemberAccess)) {
            auto* ma = static_cast<MemberAccessNode*>(node);
            return getRootVariable(ma->parent.get());
        }
        return node; // parent found
    }
};