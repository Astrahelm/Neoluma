#include "IRGenerator.hpp"
/* Deprecated structure.
#include "Core/Compiler.hpp"

// LLVM Primitives
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/TypedPointerType.h"

void IRGenerator::generate(const ProgramUnit& programUnit, const std::vector<ModuleInfo>& infos) {
    program = makeMemoryPtr<llvm::Module>("program", context);
    builder = makeMemoryPtr<llvm::IRBuilder<>>(context);

    for (ModuleId id : programUnit.order) generateModule(infos[id].module);

    if (llvm::verifyModule(*program, &llvm::errs())) {
        // later make a handler
        return;
    };
    program->print(llvm::outs(), nullptr);
}

void IRGenerator::generateModule(ModuleNode* module) {
    // Declaration pass
    for (const auto& statement : module->body) {
        switch (statement->type) {
            case ASTNodeType::Function:
            case ASTNodeType::Decorator:
                generateFunctionSignature(static_cast<FunctionNode*>(statement.get())); break;
            case ASTNodeType::Class: generateClassType(static_cast<ClassNode*>(statement.get())); break;
            case ASTNodeType::Enum: generateEnum(static_cast<EnumNode*>(statement.get())); break;
            default: break;
        }
    }

    // Generation pass
    for (const auto& statement : module->body) {
        switch (statement->type) {
            case ASTNodeType::Function:
            case ASTNodeType::Decorator:
                generateFunctionBody(static_cast<FunctionNode*>(statement.get())); break;
            case ASTNodeType::Class: generateClassMethods(static_cast<ClassNode*>(statement.get())); break;
            default: break;
        }
    }
}

llvm::Value* IRGenerator::generateStatement(ASTNode* node) {
    switch(node->type) {
        case ASTNodeType::Declaration: return generateDeclaration(static_cast<DeclarationNode*>(node));
        case ASTNodeType::Assignment: return generateAssignment(static_cast<AssignmentNode*>(node));
        case ASTNodeType::Function:
            generateFunctionSignature(static_cast<FunctionNode*>(node));
            if (!static_cast<FunctionNode*>(node)->isIntrinsic) generateFunctionBody(static_cast<FunctionNode*>(node));
            return nullptr;
        case ASTNodeType::ReturnStatement: return generateReturn(static_cast<ReturnStatementNode*>(node));
        case ASTNodeType::IfStatement: return generateIf(static_cast<IfNode*>(node));
        case ASTNodeType::WhileLoop: return generateWhile(static_cast<WhileLoopNode*>(node));
        case ASTNodeType::ForLoop: return generateFor(static_cast<ForLoopNode*>(node));
        case ASTNodeType::CallExpression: return generateCall(static_cast<CallExpressionNode*>(node));
        case ASTNodeType::BreakStatement: return generateBreak();
        case ASTNodeType::ContinueStatement: return generateContinue();
        case ASTNodeType::ThrowStatement: return generateThrow(static_cast<ThrowStatementNode*>(node));
        case ASTNodeType::Switch: return generateSwitch(static_cast<SwitchNode*>(node));
        case ASTNodeType::TryCatch: return generateTryCatch(static_cast<TryCatchNode*>(node));
        case ASTNodeType::Lambda: return generateLambda(static_cast<LambdaNode*>(node));
        default: return generateExpression(node);
    }
}

llvm::Value* IRGenerator::generateExpression(ASTNode* node) {
    switch (node->type) {
        case ASTNodeType::Literal: return generateLiteral(static_cast<LiteralNode*>(node));
        case ASTNodeType::Variable: return generateVariable(static_cast<VariableNode*>(node));
        case ASTNodeType::CallExpression: return generateCall(static_cast<CallExpressionNode*>(node));
        case ASTNodeType::BinaryOperation: return generateBinaryOp(static_cast<BinaryOperationNode*>(node));
        case ASTNodeType::UnaryOperation: return generateUnaryOp(static_cast<UnaryOperationNode*>(node));
        case ASTNodeType::MemberAccess: return nullptr; // fixme: requires classes
        case ASTNodeType::Array:
        case ASTNodeType::Set:
        case ASTNodeType::Dict:
            return nullptr; //fixme: requires std
        case ASTNodeType::Lambda: return generateLambda(static_cast<LambdaNode*>(node));
        default: return nullptr;
    }
}

llvm::Value* IRGenerator::generateLiteral(LiteralNode* node) {
    if (node->value == "true") return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), 1);
    if (node->value == "false") return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), 0);
    if (node->value == "null") return llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(context));

    if (node->value.contains('.'))  // float
        return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), stod(node->value));

    try {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), stoi(node->value));
    } catch (const std::exception&) {}

    return llvm::cast<llvm::Value>(builder->CreateGlobalString(node->value));
}

llvm::Value* IRGenerator::generateVariable(VariableNode* node) {
    auto* alloc = findVariable(node->varName);
    return builder->CreateLoad(variableTypes[node->varName], alloc, node->varName);
}

llvm::Value* IRGenerator::generateDeclaration(DeclarationNode* node) {
    // initializating type and value
    llvm::Type* type = nullptr;
    llvm::Value* value = nullptr;

    if (!node->isTypeInference && node->rawType)
        type = resolveType(node->rawType->varType->varName);
    else if (node->value) {
        value = generateExpression(node->value.get());
        type = value->getType();
    }

    if (type == nullptr) type = llvm::Type::getInt32Ty(context); // fallback

    auto* alloc = builder->CreateAlloca(type, nullptr, node->variable->varName);
    declareVariable(node->variable->varName, alloc, type);

    if (node->value && value == nullptr) value = generateExpression(node->value.get());
    if (value) builder->CreateStore(value, alloc);

    return alloc;
}

llvm::Value* IRGenerator::generateAssignment(AssignmentNode* node) {
    auto* alloc = findVariable(getRootVarName(node->variable.get()));
    if (!alloc) return nullptr;

    auto* value = generateExpression(node->value.get());

    builder->CreateStore(value, alloc);
    return value;
}

llvm::Value* IRGenerator::generateBinaryOp(BinaryOperationNode* node) {
    auto* left  = generateExpression(node->leftOperand.get());
    auto* right = generateExpression(node->rightOperand.get());

    if (node->value == on[Operators::Add]) return builder->CreateAdd(left, right);
    if (node->value == on[Operators::Subtract]) return builder->CreateSub(left, right);
    if (node->value == on[Operators::Multiply]) return builder->CreateMul(left, right);
    if (node->value == on[Operators::Divide]) return builder->CreateSDiv(left, right);
    if (node->value == on[Operators::Modulo]) return builder->CreateSRem(left, right);
    if (node->value == on[Operators::Power]) return nullptr; //requires pow from libm
    if (node->value == on[Operators::Equal]) return builder->CreateICmpEQ(left, right);
    if (node->value == on[Operators::NotEqual]) return builder->CreateICmpNE(left, right);
    if (node->value == on[Operators::LessThan]) return builder->CreateICmpSLT(left, right);
    if (node->value == on[Operators::GreaterThan]) return builder->CreateICmpSGT(left, right);
    if (node->value == on[Operators::LessThanOrEqual]) return builder->CreateICmpSLE(left, right);
    if (node->value == on[Operators::GreaterThanOrEqual]) return builder->CreateICmpSGE(left, right);
    if (node->value == on[Operators::LogicalAnd]) return builder->CreateAnd(left, right);
    if (node->value == on[Operators::LogicalOr]) return builder->CreateOr(left, right);
    return nullptr;
}

llvm::Value* IRGenerator::generateUnaryOp(UnaryOperationNode* node) {
    auto* operand = generateExpression(node->operand.get());

    if (node->value == on[Operators::Subtract]) return builder->CreateNeg(operand);
    if (node->value == on[Operators::LogicalNot]) return builder->CreateNot(operand);

    return nullptr;
}

llvm::Value* IRGenerator::generateIf(IfNode* node) {
    auto* thenBlock = llvm::BasicBlock::Create(context, "if.thenBlock", currentFunction);
    auto* elseBlock = llvm::BasicBlock::Create(context, "if.elseBlock", currentFunction);
    // A block following after If. LLVM has to fallback to something after meeting condition somehow
    auto* afterBlock = llvm::BasicBlock::Create(context, "if.afterBlock", currentFunction);

    // condition
    auto* condition = generateExpression(node->condition.get());
    builder->CreateCondBr(condition, thenBlock, elseBlock);

    // then
    builder->SetInsertPoint(thenBlock);
    generateStatement(node->thenBlock.get());
    builder->CreateBr(afterBlock);

    // else
    builder->SetInsertPoint(elseBlock);
    if (node->elseBlock) generateStatement(node->elseBlock.get());
    builder->CreateBr(afterBlock);

    // merge back from condition
    builder->SetInsertPoint(afterBlock);
    return nullptr;
}

llvm::Value* IRGenerator::generateFor(ForLoopNode* node) {
    auto* iterable = generateExpression(node->iterable.get());

    auto* conditionBlock = llvm::BasicBlock::Create(context, "for.condition", currentFunction);
    auto* body = llvm::BasicBlock::Create(context, "for.body", currentFunction);
    auto* afterBlock = llvm::BasicBlock::Create(context, "for.afterBlock", currentFunction);

    // save old breaks and continues for nested loops
    auto* prevBreak = currentBreakBB;
    auto* prevContinue = currentContinueBB;
    currentBreakBB = afterBlock;
    currentContinueBB = conditionBlock;

    // initialization of iterable variable
    pushScope();
    auto* iterableAlloc = builder->CreateAlloca(llvm::Type::getInt32Ty(context), nullptr, node->variable->varName);
    declareVariable(node->variable->varName, iterableAlloc, llvm::Type::getInt32Ty(context));
    builder->CreateBr(conditionBlock);

    // fixme: conditionBlock is temporarily mute, we don't have range() in std for now
    builder->SetInsertPoint(conditionBlock);
    builder->CreateCondBr(llvm::ConstantInt::getTrue(context), body, afterBlock);

    // body
    builder->SetInsertPoint(body);
    generateBlock(node->body.get());
    builder->CreateBr(conditionBlock);

    builder->SetInsertPoint(afterBlock);

    popScope();
    currentBreakBB = prevBreak;
    currentContinueBB = prevContinue;
    return nullptr;
}

llvm::Value* IRGenerator::generateWhile(WhileLoopNode* node) {
    auto* conditionBlock = llvm::BasicBlock::Create(context, "while.condition", currentFunction);
    auto* body = llvm::BasicBlock::Create(context, "while.body", currentFunction);
    auto* afterBlock = llvm::BasicBlock::Create(context, "while.afterBlock", currentFunction);

    // for nested loops we save the old breaks and continues
    auto* prevBreak = currentBreakBB;
    auto* prevContinue = currentContinueBB;

    currentBreakBB = afterBlock;
    currentContinueBB = conditionBlock;

    // condition
    builder->CreateBr(conditionBlock);
    builder->SetInsertPoint(conditionBlock);
    auto* condition = generateExpression(node->condition.get());
    builder->CreateCondBr(condition, body, afterBlock);

    // body
    builder->SetInsertPoint(body);
    generateBlock(node->body.get());
    builder->CreateBr(conditionBlock);

    // after the loop
    builder->SetInsertPoint(afterBlock);

    // recover the nested breaks and continues
    currentBreakBB = prevBreak;
    currentContinueBB = prevContinue;

    return nullptr;
}

llvm::Value* IRGenerator::generateSwitch(SwitchNode* node) {
    auto* expression = generateExpression(node->expression.get());

    auto* defaultBlock = llvm::BasicBlock::Create(context, "switch.default", currentFunction);
    auto* afterBlock = llvm::BasicBlock::Create(context, "switch.afterBlock", currentFunction);

    // preservation
    auto* prevBreak = currentBreakBB;
    currentBreakBB = afterBlock;

    auto* switchInstance = builder->CreateSwitch(expression, defaultBlock, node->cases.size());

    // cases
    for (const auto& sCase : node->cases) {
        auto* caseBlock = llvm::BasicBlock::Create(context, "switch.case", currentFunction);
        auto* caseValue = generateExpression(sCase->condition.get()); // should be ConstantInt
        switchInstance->addCase(llvm::cast<llvm::ConstantInt>(caseValue), caseBlock);
        builder->SetInsertPoint(caseBlock);
        generateStatement(sCase->body.get());
        builder->CreateBr(afterBlock);
    }

    // default
    builder->SetInsertPoint(defaultBlock);
    if (node->defaultCase) generateStatement(node->defaultCase->body.get());
    builder->CreateBr(afterBlock);

    builder->SetInsertPoint(afterBlock);
    currentBreakBB = prevBreak;
    return nullptr;
}

llvm::Function* IRGenerator::generateFunctionSignature(FunctionNode* node) {
    if (node->isIntrinsic) intrinsicFunctions[node->name] = true;

    std::vector<llvm::Type*> parameterTypes;
    for (const auto& parameter : node->parameters)
        parameterTypes.push_back(resolveType(parameter->parameterRawType->varType->varName));

    llvm::Type* returnType = node->returnType ? resolveType(node->returnType->varType->varName)
                        : llvm::Type::getVoidTy(context);

    llvm::FunctionType* functionType = llvm::FunctionType::get(returnType, parameterTypes, false);
    llvm::Function* function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, node->name, program.get());
    functions.emplace(node->name, function);
    return function;
}

void IRGenerator::generateFunctionBody(FunctionNode* node) {
    llvm::Function* function = functions.at(node->name);
    currentFunction = function;
    llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(context, "entry", function);
    builder->SetInsertPoint(entryBlock);

    pushScope();

    auto* llvmArg = function->arg_begin();
    for (const auto& parameter : node->parameters) {
        llvmArg->setName(parameter->parameterName);

        // fixme: for now, array sizes are syntactically supported, but IR doesn't address it. i will fix that after the working alpha
        auto* paramAlloc = builder->CreateAlloca(llvmArg->getType(), nullptr, parameter->parameterName);
        builder->CreateStore(&*llvmArg, paramAlloc);
        llvmArg++;
    }

    generateBlock(node->body.get());
    popScope();

    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateRetVoid();
}

llvm::Value* IRGenerator::generateLambda(LambdaNode* node) {
    std::string name = "__lambda_" + std::to_string(lambdaCounter++);

    std::vector<llvm::Type*> parameterTypes;
    for (const auto& parameter : node->params)
        if (parameter->type == ASTNodeType::Variable)
            parameterTypes.push_back(llvm::Type::getInt32Ty(context));

    // create function
    auto* fnType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), parameterTypes, false);
    auto* fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, name, program.get());

    // generate body
    auto* prevFunction = currentFunction;
    currentFunction = fn;

    auto* entryBlock = llvm::BasicBlock::Create(context, "entry", fn);
    builder->SetInsertPoint(entryBlock);

    pushScope();
    auto arg = fn->arg_begin();
    for (const auto& parameter : node->params)
        if (parameter->type == ASTNodeType::Variable) {
            auto* var = static_cast<VariableNode*>(parameter.get());
            arg->setName(var->varName);
            auto* alloc = builder->CreateAlloca(arg->getType());
            builder->CreateStore(&*arg, alloc);
            declareVariable(var->varName, alloc, arg->getType());
            arg++;
        }

    generateStatement(node->body.get());
    popScope();

    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateRetVoid();

    currentFunction = prevFunction;
    builder->SetInsertPoint(&currentFunction->back());

    return fn;
}

llvm::Value* IRGenerator::generateDecorator(DecoratorNode* node) {
    // decorator is a function that wraps behaviors of functions and does external stuff

    std::vector<llvm::Type*> parameterTypes;
    for (const auto& parameter : node->parameters) {
        if (parameter->parameterRawType)
            parameterTypes.push_back(resolveType(parameter->parameterRawType->varType->varName));
        else
            parameterTypes.push_back(llvm::Type::getInt32Ty(context));
    }

    // create function
    auto* fnType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), parameterTypes, false);
    auto* fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, node->name, program.get());
    functions[node->name] = fn;

    // generate body
    auto* entryBlock = llvm::BasicBlock::Create(context, "entry", fn);
    builder->SetInsertPoint(entryBlock);

    pushScope();
    auto arg = fn->arg_begin();
    for (const auto& parameter : node->parameters)
    {
        arg->setName(parameter->parameterName);
        auto* alloc = builder->CreateAlloca(arg->getType(), nullptr, parameter->parameterName);
        builder->CreateStore(&*arg, alloc);
        declareVariable(parameter->parameterName, alloc, arg->getType());
        arg++;
    }
    generateStatement(node->body.get());
    popScope();

    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateRetVoid();

    return fn;
}

llvm::Value* IRGenerator::generateBlock(BlockNode* node) {
    // In LLVM, the block returns only the last statement, other statements
    // before that are already in IR via builder.
    llvm::Value* lastStmt = nullptr;
    for (const auto& stmt : node->statements)
        lastStmt = generateStatement(stmt.get());
    return lastStmt;
}

llvm::Value* IRGenerator::generateCall(CallExpressionNode* node) {
    auto varName = static_cast<VariableNode*>(node->callee.get())->varName;
    auto* function = program->getFunction(varName);

    // generate arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : node->arguments)
        args.push_back(generateExpression(arg.get()));

    // if intrinsic - handle differently
    if (intrinsicFunctions.count(varName))
        return generateIntrinsicCall(varName, args);

    // usual call
    return builder->CreateCall(function, args, "calltmp");
}

// all std magic happens here
llvm::Value* IRGenerator::generateIntrinsicCall(const std::string& name, std::vector<llvm::Value*>& args) {

    Intrinsic system calls list:
    Neoluma | glibc
    __read -> read
    __write -> write
    __malloc -> malloc
    __free -> free
    __memcpy -> memcpy
    __memset -> memset
    __exit -> exit
    __panic -> not in libc, made via __panic -> fprintf(stderr) + exit(1)

    auto* ptr = llvm::PointerType::getUnqual(context);
    auto* i8  = llvm::Type::getInt8Ty(context);
    auto* i32 = llvm::Type::getInt32Ty(context);
    auto* i64 = llvm::Type::getInt64Ty(context);
    auto* voidTy = llvm::Type::getVoidTy(context);

    if (name == "__read") {
        auto* fnType = llvm::FunctionType::get(i64, {i32, ptr, i64}, false);
        auto fn = program->getOrInsertFunction("read", fnType);
        return builder->CreateCall(fn, args);
    }
    if (name == "__write") {
        auto* fnType = llvm::FunctionType::get(i64, {i32, ptr, i64}, false);
        auto fn = program->getOrInsertFunction("write", fnType);
        return builder->CreateCall(fn, args);
    }
    if (name == "__malloc") {
        auto* fnType = llvm::FunctionType::get(ptr, {i64}, false);
        auto fn = program->getOrInsertFunction("malloc", fnType);
        return builder->CreateCall(fn, args);
    }
    if (name == "__free") {
        auto* fnType = llvm::FunctionType::get(voidTy, {ptr}, false);
        auto fn = program->getOrInsertFunction("free", fnType);
        return builder->CreateCall(fn, args);
    }
    if (name == "__memcpy") {
        auto* fnType = llvm::FunctionType::get(ptr, {ptr, ptr, i64}, false);
        auto fn = program->getOrInsertFunction("memcpy", fnType);
        return builder->CreateCall(fn, args);
    }
    if (name == "__memset") {
        auto* fnType = llvm::FunctionType::get(ptr, {ptr, i8, i64}, false);
        auto fn = program->getOrInsertFunction("memset", fnType);
        return builder->CreateCall(fn, args);
    }
    if (name == "__exit") {
        auto* fnType = llvm::FunctionType::get(voidTy, {i32}, false);
        auto fn = program->getOrInsertFunction("exit", fnType);
        builder->CreateCall(fn, args);
        builder->CreateUnreachable();
        return nullptr;
    }
    if (name == "__panic") {
        // fprintf(stderr, msg)
        auto* fprintfType = llvm::FunctionType::get(i32, {ptr, ptr}, true);
        auto fprintfFn = program->getOrInsertFunction("fprintf", fprintfType);

        // stderr is FILE*
        auto* stderrVar = program->getOrInsertGlobal("stderr", ptr);
        auto* stderrVal = builder->CreateLoad(ptr, stderrVar);

        builder->CreateCall(fprintfFn, {stderrVal, args[0]});

        // exit(1)
        auto* exitType = llvm::FunctionType::get(voidTy, {i32}, false);
        auto exitFn = program->getOrInsertFunction("exit", exitType);
        builder->CreateCall(exitFn, {llvm::ConstantInt::get(i32, 1)});

        builder->CreateUnreachable();
        return nullptr;
    }

    return nullptr;
}

llvm::Value* IRGenerator::generateBreak() {
    builder->CreateBr(currentBreakBB);
    return nullptr;
}

llvm::Value* IRGenerator::generateContinue() {
    builder->CreateBr(currentContinueBB);
    return nullptr;
}

llvm::Value* IRGenerator::generateReturn(ReturnStatementNode* node) {
    if (node->expression) {
        auto val = generateExpression(node->expression.get());
        builder->CreateRet(val);
    }
    else builder->CreateRetVoid();
    return nullptr;
}

llvm::Value* IRGenerator::generateThrow(ThrowStatementNode* node) {
    generateExpression(node->expression.get());
    builder->CreateUnreachable(); // this is pain in the ass to make im skipping ts
    return nullptr;
}

llvm::Type* IRGenerator::resolveType(const std::string& typeName) {
    auto tm = getTypeNames();

    if (typeName == tm[ResolvedType::Int8]) return llvm::Type::getInt8Ty(context);
    if (typeName == tm[ResolvedType::Int16]) return llvm::Type::getInt16Ty(context);
    if (typeName == tm[ResolvedType::Int]) return llvm::Type::getInt32Ty(context);
    if (typeName == tm[ResolvedType::Int64]) return llvm::Type::getInt64Ty(context);
    if (typeName == tm[ResolvedType::Int128]) return llvm::Type::getInt128Ty(context);
    if (typeName == tm[ResolvedType::Bool]) return llvm::Type::getInt1Ty(context);
    if (typeName == tm[ResolvedType::Float]) return llvm::Type::getFloatTy(context);
    if (typeName == tm[ResolvedType::Float64]) return llvm::Type::getDoubleTy(context);
    if (typeName == tm[ResolvedType::Void]) return llvm::Type::getVoidTy(context);
    if (typeName == tm[ResolvedType::Str]) return llvm::Type::getInt8Ty(context)->getPointerTo(0);
    if (typeName == tm[ResolvedType::Bool]) return llvm::Type::getInt1Ty(context);

    // fixme: address the pointer of str and class
    for (const auto& classType : classTypes)
        if (typeName == classType.first) return llvm::TypedPointerType::get(classType.second, 0);

    return llvm::Type::getInt8Ty(context); // default
}

void IRGenerator::pushScope() { scopes.emplace_back(); }

void IRGenerator::popScope() { if (!scopes.empty()) scopes.pop_back(); }

void IRGenerator::declareVariable(const std::string& name, llvm::Value* value, llvm::Type* type) {
    if (scopes.empty()) pushScope();
    scopes.back()[name] = value;
    variableTypes[name] = type;
}

llvm::Value* IRGenerator::findVariable(const std::string& name) {
    for (int i = (int)scopes.size()-1; i >= 0; i--) if (scopes[i].count(name)) return scopes[i][name];
    return nullptr;
}

std::string IRGenerator::getRootVarName(ASTNode* node) {
    if (node->type == ASTNodeType::Variable)
        return static_cast<VariableNode*>(node)->varName;
    if (node->type == ASTNodeType::MemberAccess)
        return getRootVarName(static_cast<MemberAccessNode*>(node)->parent.get());
    return "";
}
*/