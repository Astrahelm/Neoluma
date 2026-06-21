#include "SemanticAnalysis.hpp"

#include <iostream>

#include "Core/Compiler.hpp"

// ==== Main function ====
void SemanticAnalysis::analyzeProgram(Program& program){
    pushScope();

    // Defines all built-in decorators.
    auto& dm = getDecoratorMap();
    for (const auto& [name, _] : dm) {
        declareName(name, Symbol{Symbol::Kind::Decorator, false, "", 0, 0}, nullptr);
    }

    for (ModuleId id : program.order) {
        if (id < 0 || id >= static_cast<ModuleId>(program.moduleInfos.size()))
            continue;

        ModuleNode* module = program.moduleInfos[id].module;
        if (!module)
            continue;

        if (!program.moduleInfos[id].module)
            std::println(std::cerr, "NULL MODULE INFO: {}", id);
        analyzeModule(module);
    }

    popScope();
}

/* Module analysis usually breaks down into two passes
 * 1. Declaration - we get all the symbols used in code, before checking their work logic
 * 2. Analysis - we pass through the whole module body, calling each analyze* for each part
 */
void SemanticAnalysis::analyzeModule(ModuleNode* module) {
    // Declaration pass
    for (const auto& statement : module->body){
        if (match(statement.get(), ASTNodeType::Function)) {
            auto* node = static_cast<FunctionNode*>(statement.get());
            bool isConst = false;
            for (auto& modifier : node->modifiers) if (modifier.get()->modifier == ASTModifierType::Const) isConst = true;
            declareName(node->name, Symbol{Symbol::Kind::Function, isConst, node->filePath, node->line, node->column}, node);
        }
        else if (match(statement.get(), ASTNodeType::Class)) {
            auto* node = static_cast<ClassNode*>(statement.get());
            bool isConst = false;
            for (auto& modifier : node->modifiers) if (modifier.get()->modifier == ASTModifierType::Const) isConst = true;
            declareName(node->name, Symbol{Symbol::Kind::Class, isConst, node->filePath, node->line, node->column}, node);
        }
        else if (match(statement.get(), ASTNodeType::Enum)) {
            auto* node = static_cast<EnumNode*>(statement.get());
            declareName(node->name, Symbol{Symbol::Kind::Enum, true, node->filePath, node->line, node->column}, node);
        }
        else if (match(statement.get(), ASTNodeType::Interface)) {
            auto* node = static_cast<InterfaceNode*>(statement.get());
            declareName(node->name, Symbol{Symbol::Kind::Interface, true, node->filePath, node->line, node->column}, node);
        }
        else if (match(statement.get(), ASTNodeType::Decorator)) {
            auto* node = static_cast<DecoratorNode*>(statement.get());
            bool isConst = false;
            for (auto& modifier : node->modifiers) if (modifier.get()->modifier == ASTModifierType::Const) isConst = true;
            declareName(node->name, Symbol{Symbol::Kind::Decorator, isConst, node->filePath, node->line, node->column}, node);
        }
    }

    // Analysis pass
    for (const auto& statement : module->body)
        analyzeStatement(statement.get());
}

void SemanticAnalysis::analyzeExpression(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case ASTNodeType::Variable: {
            auto* var = static_cast<VariableNode*>(node);
            if (!findName(var->varName))
                errorManager->addError(ErrorType::Analysis, AnalysisErrors::UndefinedVariable,
                    ErrorSpan{node->filePath, var->varName, node->line, node->column},
                    "ErrorManager.Analysis.UndefinedVariable.message", {var->varName},
                    "ErrorManager.Analysis.UndefinedVariable.hint");
            break;
        }
        case ASTNodeType::CallExpression:
            analyzeCallExpression(static_cast<CallExpressionNode*>(node)); break;
        case ASTNodeType::BinaryOperation: {
            auto* bin = static_cast<BinaryOperationNode*>(node);
            analyzeExpression(bin->leftOperand.get());
            analyzeExpression(bin->rightOperand.get());
            break;
        }
        case ASTNodeType::UnaryOperation:
            analyzeExpression(static_cast<UnaryOperationNode*>(node)->operand.get()); break;
        case ASTNodeType::MemberAccess: {
            auto* ma = static_cast<MemberAccessNode*>(node);
            analyzeExpression(ma->parent.get());
            break;
        }
        case ASTNodeType::Array:
            for (const auto& el : static_cast<ArrayNode*>(node)->elements) analyzeExpression(el.get());
            break;
        case ASTNodeType::Set:
            for (const auto& el : static_cast<SetNode*>(node)->elements) analyzeExpression(el.get());
            break;
        case ASTNodeType::Dict:
            for (const auto& [k, v] : static_cast<DictNode*>(node)->elements) {
                analyzeExpression(k.get());
                analyzeExpression(v.get());
            }
            break;
        case ASTNodeType::Lambda: {
            analyzeLambda(static_cast<LambdaNode*>(node)); break;
        }
        default:
            break;
    }
}

void SemanticAnalysis::analyzeFunction(FunctionNode* node) {
    pushScope();
    functionDepth++;

    for (const auto& parameter : node->parameters) {
        if (scopes.back().contains((parameter->parameterName))) {
            errorManager->addError(ErrorType::Analysis, AnalysisErrors::DuplicateParameterName,
                ErrorSpan{parameter->filePath, parameter->parameterName, parameter->line, parameter->column},
                "ErrorManager.Analysis.DuplicateParameterName.message", {parameter->parameterName, node->name},
                "ErrorManager.Analysis.DuplicateParameterName.hint");
            popScope();
            functionDepth--;
            return;
        }

        //FIXME: Right now parameters do not support const. this must be addressed.
        declareName(parameter->parameterName, Symbol{Symbol::Kind::Parameter, false, node->filePath, node->line, node->column}, parameter.get());

        if (parameter->defaultValue) analyzeExpression(parameter->defaultValue.get());
    }

    analyzeBlock(node->body.get());
    functionDepth--;
    popScope();
}

void SemanticAnalysis::analyzeBlock(BlockNode* node) {
    pushScope();

    for (const auto& statement : node->statements)
        analyzeStatement(statement.get());

    popScope();
}

void SemanticAnalysis::analyzeStatement(ASTNode* statement) {
    switch (statement->type) {
        case ASTNodeType::Block: analyzeBlock(static_cast<BlockNode*>(statement)); break;
        case ASTNodeType::Declaration: analyzeDeclaration(static_cast<DeclarationNode*>(statement)); break;
        case ASTNodeType::Assignment: analyzeAssignment(static_cast<AssignmentNode*>(statement)); break;
        case ASTNodeType::Function: analyzeFunction(static_cast<FunctionNode*>(statement)); break;
        case ASTNodeType::Class: analyzeClass(static_cast<ClassNode*>(statement)); break;
        case ASTNodeType::Enum: analyzeEnum(static_cast<EnumNode*>(statement)); break;
        case ASTNodeType::Interface: analyzeInterface(static_cast<InterfaceNode*>(statement)); break;
        case ASTNodeType::CallExpression: analyzeCallExpression(static_cast<CallExpressionNode*>(statement)); break;
        case ASTNodeType::Decorator: analyzeDecorator(static_cast<DecoratorNode*>(statement)); break;
        case ASTNodeType::IfStatement: analyzeIf(static_cast<IfNode*>(statement)); break;
        case ASTNodeType::Switch: analyzeSwitch(static_cast<SwitchNode*>(statement)); break;
        case ASTNodeType::WhileLoop: analyzeWhile(static_cast<WhileLoopNode*>(statement)); break;
        case ASTNodeType::ForLoop: analyzeFor(static_cast<ForLoopNode*>(statement)); break;
        case ASTNodeType::TryCatch: analyzeTryCatch(static_cast<TryCatchNode*>(statement)); break;
        case ASTNodeType::ReturnStatement: analyzeReturn(static_cast<ReturnStatementNode*>(statement)); break;
        case ASTNodeType::ThrowStatement: analyzeThrow(static_cast<ThrowStatementNode*>(statement)); break;
        case ASTNodeType::BreakStatement: analyzeBreak(static_cast<BreakStatementNode*>(statement)); break;
        case ASTNodeType::ContinueStatement: analyzeContinue(static_cast<ContinueStatementNode*>(statement)); break;
        case ASTNodeType::Lambda: analyzeLambda(static_cast<LambdaNode*>(statement)); break;
        case ASTNodeType::Import: break; // Imports are already resolved by the Orchestrator
        default: analyzeExpression(statement); break;
    }
}

void SemanticAnalysis::analyzeDeclaration(DeclarationNode* node) {
    // at first you make sure what the value is to not screw up with x := x being undefined
    if (node->value) analyzeExpression(node->value.get());

    if (!node->isTypeInference && node->rawType) {
        auto type = resolveType(node->rawType.get());
        if (type == ResolvedType::Unknown)
            errorManager->addError(ErrorType::Analysis, AnalysisErrors::UnknownType,
        ErrorSpan{node->rawType->filePath, node->rawType->varType->varName, node->rawType->line, node->rawType->column},
        "ErrorManager.Analysis.UnknownType.message", {node->rawType->varType->varName, node->variable->varName},
        "ErrorManager.Analysis.UnknownType.hint");
    }

    bool isConst = false;
    for (auto& modifier : node->modifiers) if (modifier.get()->modifier == ASTModifierType::Const) isConst = true;
    declareName(node->variable->varName, Symbol{Symbol::Kind::Variable, isConst, node->filePath, node->line, node->column}, node);
}

void SemanticAnalysis::analyzeAssignment(AssignmentNode* node) {
    analyzeExpression(node->value.get());

    auto* variable = getRootVariable(node->variable.get());
    if (variable && match(variable, ASTNodeType::Variable)) {
        auto* var = static_cast<VariableNode*>(variable);
        auto* symbol = findName(var->varName);
        if (!symbol) errorManager->addError(ErrorType::Analysis, AnalysisErrors::UndefinedVariable,
            ErrorSpan{var->filePath, var->varName, var->line, var->column},
            "ErrorManager.Analysis.UndefinedVariable.message", {var->varName},
            "ErrorManager.Analysis.UndefinedVariable.hint"
            );
        else if (symbol->isConst) errorManager->addError(ErrorType::Analysis, AnalysisErrors::ConstantReassignment,
            ErrorSpan{var->filePath, var->varName, var->line, var->column},
            "ErrorManager.Analysis.ConstantReassignment.message", {var->varName},
            "ErrorManager.Analysis.ConstantReassignment.hint"
            );
    }
}

void SemanticAnalysis::analyzeCallExpression(CallExpressionNode* node) {
    for (const auto& arg : node->arguments) analyzeExpression(arg.get());

    if (match(node->callee.get(), ASTNodeType::Variable)) {
        auto varName = static_cast<VariableNode*>(node->callee.get())->varName;
        auto* sym = findName(varName);

        if (!sym && node->isDecoratorCall)
            errorManager->addError(ErrorType::Analysis, AnalysisErrors::UndefinedDecorator,
                ErrorSpan{node->filePath, varName, node->line, node->column},
                "ErrorManager.Analysis.UndefinedDecorator.message", {varName},
                "ErrorManager.Analysis.UndefinedDecorator.hint");
        else if (!sym && !node->isDecoratorCall)
            errorManager->addError(ErrorType::Analysis, AnalysisErrors::UndefinedFunction,
                ErrorSpan{node->filePath, varName, node->line, node->column},
                "ErrorManager.Analysis.UndefinedFunction.message", {varName},
                "ErrorManager.Analysis.UndefinedFunction.hint");
        else if (sym && node->isDecoratorCall && sym->kind != Symbol::Kind::Decorator)
            errorManager->addError(ErrorType::Analysis, AnalysisErrors::DecoratorMisuse,
                ErrorSpan{node->filePath, varName, node->line, node->column},
                "ErrorManager.Analysis.DecoratorMisuse.message", {varName},
                "ErrorManager.Analysis.DecoratorMisuse.hint");
        else if (sym && !node->isDecoratorCall && sym->kind != Symbol::Kind::Function && sym->kind != Symbol::Kind::Class)
            errorManager->addError(ErrorType::Analysis, AnalysisErrors::FunctionMismatch,
                ErrorSpan{node->filePath, varName, node->line, node->column},
                "ErrorManager.Analysis.FunctionMismatch.message", {varName},
                "ErrorManager.Analysis.FunctionMismatch.hint");
    }
    else if (match(node->callee.get(), ASTNodeType::MemberAccess)) {
        auto* root = getRootVariable(node->callee.get());
        if (root && match(root, ASTNodeType::Variable)) {
            auto varName = static_cast<VariableNode*>(root)->varName;
            auto* symbol = findName(varName);
            if (!symbol)
                errorManager->addError(ErrorType::Analysis, AnalysisErrors::UndefinedVariable,
                    ErrorSpan{node->filePath, varName, node->line, node->column},
                    "ErrorManager.Analysis.UndefinedVariable.message", {varName},
                    "ErrorManager.Analysis.UndefinedVariable.hint");
        }
    }
}

void SemanticAnalysis::analyzeIf(IfNode* node) {
    analyzeExpression(node->condition.get());
    if (node->thenBlock) analyzeStatement(node->thenBlock.get());
    if (node->elseBlock) analyzeStatement(node->elseBlock.get());
}

void SemanticAnalysis::analyzeWhile(WhileLoopNode* node) {
    analyzeExpression(node->condition.get());
    loopDepth++;
    analyzeBlock(node->body.get());
    loopDepth--;
}

void SemanticAnalysis::analyzeFor(ForLoopNode* node) {
    analyzeExpression(node->iterable.get());

    loopDepth++;
    pushScope();

    // FIXME: Find out how to get if it's the constant.
    declareName(node->variable.get()->varName, Symbol{Symbol::Kind::Variable, false, node->variable->filePath, node->variable->line, node->variable->column}, node->variable.get());
    for (const auto& stmt : node->body->statements)
        analyzeStatement(stmt.get());

    popScope();
    loopDepth--;
}

void SemanticAnalysis::analyzeReturn(ReturnStatementNode* node) {
    if (functionDepth <= 0) {
        errorManager->addError(ErrorType::Analysis, AnalysisErrors::ReturnOutsideFunction,
            ErrorSpan{node->filePath, "return", node->line, node->column},
            "ErrorManager.Analysis.ReturnOutsideFunction.message", {},
            "ErrorManager.Analysis.ReturnOutsideFunction.hint");
    }

    if (node->expression) analyzeExpression(node->expression.get());
}

void SemanticAnalysis::analyzeBreak(BreakStatementNode* node) {
    if (loopDepth <= 0)
        errorManager->addError(ErrorType::Analysis, AnalysisErrors::BreakOutsideLoop,
            ErrorSpan{node->filePath, "break", node->line, node->column},
            "ErrorManager.Analysis.BreakOutsideLoop.message", {},
            "ErrorManager.Analysis.BreakOutsideLoop.hint");

}

void SemanticAnalysis::analyzeContinue(ContinueStatementNode* node) {
    if (loopDepth <= 0)
        errorManager->addError(ErrorType::Analysis, AnalysisErrors::ContinueOutsideLoop,
            ErrorSpan{node->filePath, "continue", node->line, node->column},
            "ErrorManager.Analysis.ContinueOutsideLoop.message", {},
            "ErrorManager.Analysis.ContinueOutsideLoop.hint");
}

void SemanticAnalysis::analyzeThrow(ThrowStatementNode* node) {
    if (node->expression) analyzeExpression(node->expression.get());
}

void SemanticAnalysis::analyzeClass(ClassNode* node) {
    pushScope();

    // self и super are available inside the whole class
    declareName("self", Symbol{Symbol::Kind::Variable, false, node->filePath, node->line, node->column}, node);
    if (node->super) declareName("super", Symbol{Symbol::Kind::Function, false, node->filePath, node->line, node->column}, node);


    // declaration of methods
    for (const auto& method : node->methods) {
        bool isConst = false;
        for (auto& modifier : method->modifiers) if (modifier.get()->modifier == ASTModifierType::Const) isConst = true;
        declareName(method->name, Symbol{Symbol::Kind::Function, isConst, node->filePath, node->line, node->column}, node);
    }

    for (const auto& field : node->fields) analyzeDeclaration(field.get());
    if (node->constructor) analyzeFunction(node->constructor.get());
    // implementation of methods
    for (const auto& method : node->methods) analyzeFunction(method.get());

    popScope();
}

void SemanticAnalysis::analyzeTryCatch(TryCatchNode* node) {
    analyzeBlock(node->tryBlock.get());

    pushScope(); // catch has it's own scope
    declareName(node->exception->varName, Symbol{Symbol::Kind::Variable, false, node->exception->filePath, node->exception->line, node->exception->column}, node);
    analyzeBlock(node->catchBlock.get());
    popScope();
}

void SemanticAnalysis::analyzeSwitch(SwitchNode* node) {
    analyzeExpression(node->expression.get());

    for (const auto& sCase : node->cases) {
        analyzeExpression(sCase->condition.get());
        analyzeStatement(sCase->body.get());
    }

    if (node->defaultCase) analyzeStatement(node->defaultCase.get()->body.get());
}

void SemanticAnalysis::analyzeEnum(EnumNode* node) {
    pushScope();

    for (const auto& element : node->elements) {
        if (scopes.back().contains(element->name)) errorManager->addError(ErrorType::Analysis, AnalysisErrors::DuplicateEnumMember,
            ErrorSpan{node->filePath, element->name, node->line, node->column},
            "ErrorManager.Analysis.DuplicateEnumMember.message", {element->name, node->name},
            "ErrorManager.Analysis.DuplicateEnumMember.hint"
        );
        else declareName(element->name, Symbol{Symbol::Kind::Variable, false, node->filePath, node->line, node->column}, node);
    }

    popScope();
}

void SemanticAnalysis::analyzeInterface(InterfaceNode* node) {
    for (const auto& field : node->elements)
        if (!field->isFunction && field->rawType)
            resolveType(field->rawType.get());
}

void SemanticAnalysis::analyzeDecorator(DecoratorNode* node) {
    pushScope();
    functionDepth++;

    for (const auto& parameter : node->parameters) {
        if (!parameter) continue;
        declareName(parameter->parameterName, Symbol{Symbol::Kind::Parameter, false, node->filePath, node->line, node->column}, node);
        if (parameter->defaultValue) analyzeExpression(parameter->defaultValue.get());
    }
    analyzeBlock(node->body.get());

    functionDepth--;
    popScope();
}

void SemanticAnalysis::analyzeLambda(LambdaNode* node) {
    pushScope();
    functionDepth++;

    for (const auto& param : node->params) {
        if (match(param.get(), ASTNodeType::Variable)) {
            auto* v = static_cast<VariableNode*>(param.get());
            declareName(v->varName, Symbol{Symbol::Kind::Parameter, false, v->filePath, v->line, v->column}, param.get());
        }
    }

    analyzeStatement(node->body.get());

    functionDepth--;
    popScope();
}

ResolvedType SemanticAnalysis::resolveType(RawTypeNode* type) {
    auto varType = type->varType.get()->varName;
    auto tm = getTypeMap();

    // first check built-ins
    if (tm.contains(varType)) return tm.find(varType)->second;

    // well, perhaps it's user-defined?
    auto* userDefined = findName(varType);
    if (userDefined && (userDefined->kind == Symbol::Kind::Class || userDefined->kind == Symbol::Kind::Enum || userDefined->kind == Symbol::Kind::Interface)) return ResolvedType::UserDefined;

    // The type is unknown. Error message context is added at parent call.
    return ResolvedType::Unknown;
}

// ==== Helpers ====

void SemanticAnalysis::pushScope() { scopes.emplace_back(); }

void SemanticAnalysis::popScope() { if (!scopes.empty()) scopes.pop_back(); }

bool SemanticAnalysis::declareName(const std::string& name, Symbol symbol, ASTNode* node)
{
    if (scopes.empty()) pushScope();
    auto& parent = scopes.back();

    if (parent.contains(name)) {
        errorManager->addError(ErrorType::Analysis, AnalysisErrors::RedefinedVariable,
            ErrorSpan{node ? node->filePath : "", name, node ? node->line : 0, node ? node->column : 0},
            "ErrorManager.Analysis.RedefinedVariable.message", {name},
            "ErrorManager.Analysis.RedefinedVariable.hint");
        return false;
    }

    if (node) {
        symbol.filePath = node->filePath;
        symbol.line = node->line;
        symbol.column = node->column;
    }

    parent.emplace(name, symbol);
    return true;
}

SemanticAnalysis::Symbol* SemanticAnalysis::findName(const std::string& name) {
    for (int i = (int)scopes.size()-1; i >= 0; i--){
        if (scopes[i].contains(name))
            return &scopes[i].find(name)->second;
    }
    return nullptr;
}
