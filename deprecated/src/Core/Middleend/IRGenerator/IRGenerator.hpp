#pragma once
#include <vector>
#include <unordered_map>

#include "Core/Frontend/Nodes.hpp"
#include "Core/Frontend/Orchestrator/Orchestrator.hpp"
#include "HelperFunctions.hpp"
/* Deprecated structure
// LLVM Pritimives
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

struct Compiler;

class IRGenerator {
public:
    Compiler* compiler = nullptr;
    void setCompiler(Compiler* compiler) { this->compiler = compiler; }

    // Main function
    void generate(const ProgramUnit& programUnit, const std::vector<ModuleInfo>& infos);

private:
    llvm::LLVMContext context;
    MemoryPtr<llvm::Module> program;
    MemoryPtr<llvm::IRBuilder<>> builder;

    std::vector<std::unordered_map<std::string, llvm::Value*>> scopes;

    // Helper functions
    bool match(ASTNode* node, ASTNodeType type) {
        if (node->type == type) return true;
        return false;
    }
    void pushScope();
    void popScope();
    void declareVariable(const std::string& name, llvm::Value* value, llvm::Type* type);
    llvm::Value* findVariable(const std::string& name);
    std::string getRootVarName(ASTNode* node); // for member access
    std::unordered_map<Operators, std::string> on = getOperatorNames();

    // Generators
    void generateModule(ModuleNode* module);
    llvm::Function* generateFunctionSignature(FunctionNode* node);
    void generateFunctionBody(FunctionNode* node);

    llvm::Value* generateStatement(ASTNode* node);
    llvm::Value* generateExpression(ASTNode* node);
    llvm::Value* generateLiteral(LiteralNode* node);
    llvm::Value* generateVariable(VariableNode* node);
    llvm::Value* generateDeclaration(DeclarationNode* node);
    llvm::Value* generateAssignment(AssignmentNode* node);
    llvm::Value* generateBinaryOp(BinaryOperationNode* node);
    llvm::Value* generateUnaryOp(UnaryOperationNode* node);
    llvm::Value* generateCall(CallExpressionNode* node);
    llvm::Value* generateIntrinsicCall(const std::string& name, std::vector<llvm::Value*>& args);
    llvm::Value* generateIf(IfNode* node);
    llvm::Value* generateWhile(WhileLoopNode* node);
    llvm::Value* generateFor(ForLoopNode* node);
    llvm::Value* generateReturn(ReturnStatementNode* node);
    llvm::Value* generateBlock(BlockNode* block);
    llvm::Value* generateBreak();
    llvm::Value* generateContinue();
    llvm::Value* generateThrow(ThrowStatementNode* node);
    llvm::Value* generateSwitch(SwitchNode* node);
    llvm::Value* generateTryCatch(TryCatchNode* node);
    llvm::Value* generateLambda(LambdaNode* node);
    llvm::Value* generateDecorator(DecoratorNode* node);
    llvm::StructType* generateClassType(ClassNode* node);
    void generateClassMethods(ClassNode* node);
    llvm::Value* generateEnum(EnumNode* node);
    void generateInterface(InterfaceNode* node);

    llvm::Type* resolveType(const std::string& typeName);

    // Private fields for certain generate functions are stored here
    llvm::BasicBlock* currentBreakBB = nullptr; // break block
    llvm::BasicBlock* currentContinueBB = nullptr;// continue block
    llvm::Function* currentFunction = nullptr; // needed for IfNode and others to access
    std::unordered_map<std::string, llvm::Type*> variableTypes; // llvm 17+ everything is opaque ptrs so need to store type somewhere
    std::unordered_map<std::string, llvm::Function*> functions; // function signatures
    std::unordered_map<std::string, llvm::StructType*> classTypes; // types of classes
    std::unordered_map<std::string, bool> intrinsicFunctions; // for intrinsic calls
    int lambdaCounter = 0;
};
*/