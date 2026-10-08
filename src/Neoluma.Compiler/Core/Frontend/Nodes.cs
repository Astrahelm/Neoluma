using System.Text;

namespace Neoluma.Core.Frontend;

public enum ASTNodeType : byte {
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
}

public enum ASTModifierType : byte {
    Public, Private, Protected, Static, Const, Override, Async, Debug,
    Intrinsic
}

public enum ASTPreprocessorDirectiveType : byte {
    Import, Unsafe, Macro, None
}

public enum ASTImportType : byte {
    /*
    Native - from dependencies
    Relative - relative to path
    Foreign - imported from other language via langpacks.
    ForeignRelative - imported from other language via langpacks, but with relative path.
    */
    Native, Relative, Foreign, ForeignRelative,
}

public readonly struct GenericParameter(string name, SourceSpan sourceSpan) {
    public readonly string name = name;
    public readonly SourceSpan sourceSpan = sourceSpan;
}

public abstract class ASTNode(ASTNodeType type) {
    public readonly ASTNodeType type = type;
    // Tracking the node for the ErrorManager purposes
    public SourceSpan sourceSpan;
}

// All nodes available in Neoluma

// This node only represents the existence of a variable (its name). Type info and initialized value are in DeclarationNode.
public sealed class VariableNode(string varName) : ASTNode(ASTNodeType.Variable) {
    public string varName = varName;
}

public sealed class LiteralNode(string value) : ASTNode(ASTNodeType.Literal) {
    public string value = value;

    // TODO: add string statements to support inline data in strings
    //optional<public List<VariableNode>> stringStatements = nullopt
}

// Assignment node assigns a value to an existing variable.
public sealed class AssignmentNode(ASTNode variable, string op, ASTNode value) : ASTNode(ASTNodeType.Assignment) {
    public ASTNode variable = variable;
    public string op = op; // Assignment operator
    public ASTNode value = value;
}

public sealed class MemberAccessNode(ASTNode parent, ASTNode val) : ASTNode(ASTNodeType.MemberAccess) {
    public ASTNode parent = parent;
    public ASTNode val = val;
}

public sealed class BinaryOperationNode(ASTNode leftOperand, string op, ASTNode rightOperand)
    : ASTNode(ASTNodeType.BinaryOperation) {
    public ASTNode leftOperand = leftOperand;
    public string op = op;
    public ASTNode rightOperand = rightOperand;
}

public sealed class RawTypeNode : ASTNode {
    public  VariableNode varType;
    // ASTNode is used only for null. Be aware!
    public  ASTNode? varSize;
    
    // array<str>, dict<str, int>, Box<T>
    public List<RawTypeNode>? genericArguments = null;
    
    public RawTypeNode(VariableNode varType, ASTNode? varSize = null, List<RawTypeNode>? genericArguments = null) : base(ASTNodeType.RawType) {
        this.varType = varType;
        this.varSize = varSize;
        this.genericArguments = genericArguments;
    }
}

public sealed class UnaryOperationNode(string op, ASTNode operand) : ASTNode(ASTNodeType.UnaryOperation) {
    public string op = op;
    public ASTNode operand = operand;
}

// statements 
public sealed class BlockNode(List<ASTNode>? statements = null) : ASTNode(ASTNodeType.Block) {
    public List<ASTNode> statements = statements ?? [];
}

public sealed class IfNode(ASTNode condition, ASTNode thenBlock, ASTNode? elseBlock = null)
    : ASTNode(ASTNodeType.IfStatement) {
    public ASTNode condition = condition;
    public ASTNode thenBlock = thenBlock;
    public ASTNode? elseBlock = elseBlock;
}

public sealed class SCDefaultNode(ASTNode body) : ASTNode(ASTNodeType.SCDefault) {
    public ASTNode body = body;
}

public sealed class CaseNode(ASTNode condition, ASTNode body) : ASTNode(ASTNodeType.Case) {
    public ASTNode condition = condition;
    public ASTNode body = body;
}

public sealed class SwitchNode(ASTNode expression, List<CaseNode> cases, SCDefaultNode? defaultCase = null)
    : ASTNode(ASTNodeType.Switch) {
    public ASTNode expression = expression;
    public List<CaseNode> cases = cases;
    public SCDefaultNode? defaultCase = defaultCase;
}

public sealed class ForLoopNode(VariableNode variable, ASTNode iterable, BlockNode body)
    : ASTNode(ASTNodeType.ForLoop) {
    public VariableNode variable = variable;
    public ASTNode iterable = iterable;
    public BlockNode body = body;  // TODO: Implement one-liner statement for for.
}

public sealed class WhileLoopNode(ASTNode condition, BlockNode body) : ASTNode(ASTNodeType.WhileLoop) {
    public ASTNode condition = condition;
    public BlockNode body = body; // TODO: Implement one-liner statement for while.
}

public sealed class BreakStatementNode() : ASTNode(ASTNodeType.BreakStatement);

public sealed class ContinueStatementNode() : ASTNode(ASTNodeType.ContinueStatement);

public sealed class ReturnStatementNode(ASTNode? expression = null) : ASTNode(ASTNodeType.ReturnStatement) {
    public ASTNode? expression = expression;
}

public sealed class ThrowStatementNode(ASTNode expression) : ASTNode(ASTNodeType.ThrowStatement) {
    public ASTNode expression = expression;
}

public sealed class TryCatchNode(BlockNode tryBlock, VariableNode exception, BlockNode catchBlock)
    : ASTNode(ASTNodeType.TryCatch) {
    public BlockNode tryBlock = tryBlock;
    public VariableNode exception = exception;
    public BlockNode catchBlock = catchBlock;
}

// composite data
public sealed class ArrayNode(List<ASTNode> elements) : ASTNode(ASTNodeType.Array) {
    public List<ASTNode> elements = elements;
    /*ASTNode> typeHint;*/

    /*, ASTNode> typeHint=nullptr*/
}

public sealed class SetNode(List<ASTNode> elements) : ASTNode(ASTNodeType.Set) {
    public List<ASTNode> elements = elements;
    //RawTypeNode> typeHint;

    /*, ASTNode> typeHint=nullptr*/
}

public sealed class DictNode(List<(ASTNode key, ASTNode value)> elements) : ASTNode(ASTNodeType.Dict) {
    public List<(ASTNode key, ASTNode value)> elements = elements;
    //array<RawTypeNode>, 2> types;

    /*, array<RawTypeNode>, 2> types*/
}

// For now it's used only in lambda conditions, it must be fixed later
public sealed class TupleNode(List<ASTNode> elements) : ASTNode(ASTNodeType.Tuple) {
    public List<ASTNode> elements = elements;
}

public sealed class ResultNode : ASTNode {
    public ASTNode t;
    public ASTNode? e;
    public bool isError = false;

    public ResultNode(ASTNode t, ASTNode? e = null, bool isError = false) : base(ASTNodeType.Result) {
        this.t = t;
        this.e = e;
        this.isError = isError;
    }
}

// higher structures
public sealed class ParameterNode : ASTNode {
    public string parameterName;
    public RawTypeNode? parameterRawType = null;
    public ASTNode? defaultValue = null; // optional

    public ParameterNode(string parameterName, RawTypeNode? parameterRawType = null, ASTNode? defaultValue = null) : base(ASTNodeType.Parameter) {
        this.parameterName = parameterName;
        this.parameterRawType = parameterRawType;
        this.defaultValue = defaultValue;
    }
}

public sealed class ModifierNode(ASTModifierType modifier) : ASTNode(ASTNodeType.Modifier) {
    public ASTModifierType modifier = modifier;
}

public sealed class CallExpressionNode : ASTNode {
    public ASTNode callee;
    public List<ASTNode> arguments;
    public bool isDecoratorCall = false;

    public CallExpressionNode(ASTNode callee, List<ASTNode> arguments, bool isDecoratorCall = false) : base(ASTNodeType.CallExpression) {
        this.callee = callee;
        this.arguments = arguments;
        this.isDecoratorCall = isDecoratorCall;
    }
}

public sealed class EnumMemberNode(string name, LiteralNode? value = null) : ASTNode(ASTNodeType.EnumMember) {
    public string name = name;
    public LiteralNode? value = value; // TODO: Maybe allow expressions in the future that would return a number.
};

public sealed class EnumNode(
    string name,
    List<EnumMemberNode> elements,
    List<CallExpressionNode>? decorators = null,
    List<ModifierNode>? modifiers = null)
    : ASTNode(ASTNodeType.Enum) {
    public string name = name;
    public List<CallExpressionNode>? decorators = decorators;
    public List<ModifierNode>? modifiers = modifiers;
    public List<EnumMemberNode> elements = elements;
}

public sealed class InterfaceFieldNode : ASTNode {
    public string name;
    public RawTypeNode? rawType;
    public bool isNullable;

    public bool isFunction = false;
    public List<ParameterNode>? parameters = null;
    public VariableNode? returnType = null;

    public InterfaceFieldNode(string name, bool isNullable = false, bool isFunction = false, RawTypeNode? type = null, List<ParameterNode>? parameters = null, VariableNode? returnType = null) : base(ASTNodeType.InterfaceField) {
        this.name = name;
        rawType = type;
        this.isNullable = isNullable;
        this.isFunction = isFunction;
        this.parameters = parameters;
        this.returnType = returnType;
    }
};

public sealed class InterfaceNode(
    string name,
    List<InterfaceFieldNode>? elements = null,
    List<CallExpressionNode>? decorators = null,
    List<ModifierNode>? modifiers = null,
    List<GenericParameter>? genericParameters = null)
    : ASTNode(ASTNodeType.Interface) {
    public string name = name;
    public List<CallExpressionNode>? decorators = decorators;
    public List<ModifierNode>? modifiers = modifiers;
    public List<GenericParameter>? genericParameters = genericParameters;
    public List<InterfaceFieldNode>? elements = elements;
}

public sealed class LambdaNode(List<ASTNode> parameters, ASTNode body) : ASTNode(ASTNodeType.Lambda) {
    public List<ASTNode> parameters = parameters;
    public ASTNode body = body;
}

public sealed class FunctionNode : ASTNode {
    public List<CallExpressionNode>? decorators;
    public List<ModifierNode>? modifiers;
    public List<GenericParameter>? genericParameters;

    public string name;
    public List<ParameterNode> parameters;
    public RawTypeNode? returnType = null;
    public BlockNode? body = null;
    public bool isIntrinsic = false; // Is this a function that passes through an LLVM call?

    public FunctionNode(string name, List<ParameterNode> parameters, RawTypeNode? returnType = null, BlockNode? body = null, List<CallExpressionNode>? decorators = null, List<ModifierNode>? modifiers = null, List<GenericParameter>? genericParameters = null) : base(ASTNodeType.Function) {
        this.name = name;
        this.parameters = parameters;
        this.returnType = returnType;
        this.body = body; 
        this.decorators = decorators;
        this.modifiers = modifiers;
        this.genericParameters = genericParameters;
    }
}

// Declaration node holds type info, initialization value, and other metadata about a variable.
public sealed class DeclarationNode : ASTNode {
    public List<CallExpressionNode>? decorators;
    public List<ModifierNode>? modifiers;

    public VariableNode variable;
    public bool isNullable = false;
    public RawTypeNode? rawType;
    public bool isTypeInference = false;
    public ASTNode? value = null;

    public DeclarationNode(VariableNode variable, RawTypeNode? rawType = null, ASTNode? value = null, bool isNullable = false, bool isTypeInference = false, List<CallExpressionNode>? decorators = null, List<ModifierNode>? modifiers = null) : base(ASTNodeType.Declaration) {
        this.variable = variable;
        this.rawType = rawType;
        this.value = value;
        this.isNullable = isNullable;
        this.isTypeInference = isTypeInference;
        this.decorators = decorators;
        this.modifiers = modifiers;
    }
}

public sealed class ClassNode : ASTNode {
    public string name;
    public FunctionNode? constructor = null;
    public VariableNode? super = null; // Name of class or interface being inherited from, if any
    public List<CallExpressionNode>? decorators;
    public List<ModifierNode>? modifiers;
    public List<GenericParameter>? genericParameters;
    public List<DeclarationNode> fields;
    public List<FunctionNode> methods;

    public ClassNode(string name, List<DeclarationNode> fields,  List<FunctionNode> methods, FunctionNode? constructor = null, VariableNode? super = null,
              List<CallExpressionNode>? decorators = null, List<ModifierNode>? modifiers = null, List<GenericParameter>? genericParameters = null) : base(ASTNodeType.Class) {
        this.name = name;
        this.constructor = constructor;
        this.super = super;
        this.fields = fields;
        this.methods = methods;
        this.decorators = decorators;
        this.modifiers = modifiers;
        this.genericParameters = genericParameters;
    }
}

public sealed class DecoratorNode(
    string name,
    List<ParameterNode> parameters,
    BlockNode body,
    List<CallExpressionNode>? decorators = null,
    List<ModifierNode>? modifiers = null)
    : ASTNode(ASTNodeType.Decorator) {
    public string name = name;
    public List<CallExpressionNode>? decorators = decorators;
    public List<ModifierNode>? modifiers = modifiers;
    public List<ParameterNode> parameters = parameters;
    public BlockNode body = body;
}

public sealed class NamespaceNode(ASTNode name, string namespaceName, List<ASTNode> body) : ASTNode(ASTNodeType.Namespace) {
    public ASTNode name = name;
    public string namespaceName = namespaceName; // for magic imports
    public List<ASTNode> body = body;
}

// imports and program structure
public sealed class ImportNode(string moduleName, string alias, ASTImportType importType)
    : ASTNode(ASTNodeType.Import) {
    public string moduleName = moduleName;
    public string alias = alias;
    public ASTImportType importType = importType;
}

public sealed class PreprocessorDirectiveNode(ASTPreprocessorDirectiveType directive, string value = "")
    : ASTNode(ASTNodeType.Preprocessor) {
    public ASTPreprocessorDirectiveType directive = directive;
    public string value = value;
}

public sealed class ModuleNode(string name, List<ASTNode>? body = null) : ASTNode(ASTNodeType.Module) {
    public string moduleName = name;
    public List<ASTNode> body = body ?? [];
}

// because toString() in each node is heavier than the universe dayo
public static class ASTPrinter {
    public static string toString(ASTNode node, int indent = 0) {
        StringBuilder output = new();
        appendNode(output, node, indent);
        return output.ToString();
    }

    static void appendNode(StringBuilder output, ASTNode node, int indent) {
        switch (node.type) {
            case ASTNodeType.Literal: {
                LiteralNode n = (LiteralNode)node;

                if (n.value.Length == 0) appendHeader(output, n, "Literal", indent);
                else appendHeader(output, n, "Literal", indent, ("value", n.value));
                return;
            }

            case ASTNodeType.Variable: {
                VariableNode n = (VariableNode)node;
                appendHeader(output, n, "Variable", indent, ("varName", n.varName));
                return;
            }

            case ASTNodeType.Assignment: {
                AssignmentNode n = (AssignmentNode)node;
                appendHeader(output, n, "Assignment", indent, ("op", n.op));
                output.Append(" {\n");
                appendNodeField(output, "variable", n.variable, indent + 2);
                appendNodeField(output, "value", n.value, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.MemberAccess: {
                MemberAccessNode n = (MemberAccessNode)node;
                appendHeader(output, n, "MemberAccess", indent);
                output.Append(" {\n");
                appendNodeField(output, "parent", n.parent, indent + 2);
                appendNodeField(output, "val", n.val, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.BinaryOperation: {
                BinaryOperationNode n = (BinaryOperationNode)node;
                appendHeader(output, n, "BinaryOperation", indent, ("op", n.op));
                output.Append(" {\n");
                appendNodeField(output, "leftOperand", n.leftOperand, indent + 2);
                appendNodeField(output, "rightOperand", n.rightOperand, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.RawType: {
                RawTypeNode n = (RawTypeNode)node;
                appendHeader(output, n, "RawType", indent);
                output.Append(" {\n");
                appendNodeField(output, "varType", n.varType, indent + 2);
                appendNodeField(output, "varSize", n.varSize, indent + 2);
                appendNodeList(output, "genericArguments", n.genericArguments, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.UnaryOperation: {
                UnaryOperationNode n = (UnaryOperationNode)node;
                appendHeader(output, n, "UnaryOperation", indent, ("op", n.op));
                output.Append(" {\n");
                appendNodeField(output, "operand", n.operand, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.CallExpression: {
                CallExpressionNode n = (CallExpressionNode)node;
                appendHeader(output, n, "CallExpression", indent,
                    ("isDecoratorCall", n.isDecoratorCall ? "true" : "false"));
                output.Append(" {\n");
                appendNodeField(output, "callee", n.callee, indent + 2);
                appendNodeList(output, "arguments", n.arguments, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Lambda: {
                LambdaNode n = (LambdaNode)node;
                appendHeader(output, n, "Lambda", indent);
                output.Append(" {\n");
                appendNodeList(output, "parameters", n.parameters, indent + 2);
                appendNodeField(output, "body", n.body, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Block: {
                BlockNode n = (BlockNode)node;
                appendHeader(output, n, "Block", indent);
                output.Append(" {\n");
                appendNodeList(output, "statements", n.statements, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.IfStatement: {
                IfNode n = (IfNode)node;
                appendHeader(output, n, "If", indent);
                output.Append(" {\n");
                appendNodeField(output, "condition", n.condition, indent + 2);
                appendNodeField(output, "thenBlock", n.thenBlock, indent + 2);
                appendNodeField(output, "elseBlock", n.elseBlock, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.SCDefault: {
                SCDefaultNode n = (SCDefaultNode)node;
                appendHeader(output, n, "SCDefault", indent);
                output.Append(" {\n");
                appendNodeField(output, "body", n.body, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Case: {
                CaseNode n = (CaseNode)node;
                appendHeader(output, n, "Case", indent);
                output.Append(" {\n");
                appendNodeField(output, "condition", n.condition, indent + 2);
                appendNodeField(output, "body", n.body, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Switch: {
                SwitchNode n = (SwitchNode)node;
                appendHeader(output, n, "Switch", indent);
                output.Append(" {\n");
                appendNodeField(output, "expression", n.expression, indent + 2);
                appendNodeList(output, "cases", n.cases, indent + 2);
                appendNodeField(output, "defaultCase", n.defaultCase, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.ForLoop: {
                ForLoopNode n = (ForLoopNode)node;
                appendHeader(output, n, "ForLoop", indent);
                output.Append(" {\n");
                appendNodeField(output, "variable", n.variable, indent + 2);
                appendNodeField(output, "iterable", n.iterable, indent + 2);
                appendNodeField(output, "body", n.body, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.WhileLoop: {
                WhileLoopNode n = (WhileLoopNode)node;
                appendHeader(output, n, "WhileLoop", indent);
                output.Append(" {\n");
                appendNodeField(output, "condition", n.condition, indent + 2);
                appendNodeField(output, "body", n.body, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.BreakStatement:
                appendHeader(output, node, "BreakStatement", indent);
                return;

            case ASTNodeType.ContinueStatement:
                appendHeader(output, node, "ContinueStatement", indent);
                return;

            case ASTNodeType.ReturnStatement: {
                ReturnStatementNode n = (ReturnStatementNode)node;
                appendHeader(output, n, "ReturnStatement", indent);
                output.Append(" {\n");
                appendNodeField(output, "expression", n.expression, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.ThrowStatement: {
                ThrowStatementNode n = (ThrowStatementNode)node;
                appendHeader(output, n, "ThrowStatement", indent);
                output.Append(" {\n");
                appendNodeField(output, "expression", n.expression, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.TryCatch: {
                TryCatchNode n = (TryCatchNode)node;
                appendHeader(output, n, "TryCatch", indent);
                output.Append(" {\n");
                appendNodeField(output, "tryBlock", n.tryBlock, indent + 2);
                appendNodeField(output, "exception", n.exception, indent + 2);
                appendNodeField(output, "catchBlock", n.catchBlock, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Array: {
                ArrayNode n = (ArrayNode)node;
                appendHeader(output, n, "Array", indent);
                output.Append(" {\n");
                appendNodeList(output, "elements", n.elements, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Set: {
                SetNode n = (SetNode)node;
                appendHeader(output, n, "Set", indent);
                output.Append(" {\n");
                appendNodeList(output, "elements", n.elements, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Dict: {
                DictNode n = (DictNode)node;
                appendHeader(output, n, "Dict", indent);
                output.Append(" {\n");

                if (n.elements.Count == 0) {
                    appendIndent(output, indent + 2);
                    output.Append("elements: []\n");
                }
                else {
                    appendIndent(output, indent + 2);
                    output.Append("elements: [\n");

                    foreach ((ASTNode key, ASTNode value) in n.elements) {
                        appendIndent(output, indent + 4);
                        output.Append("{\n");

                        appendNodeField(output, "key", key, indent + 6);
                        appendNodeField(output, "value", value, indent + 6);

                        appendIndent(output, indent + 4);
                        output.Append("}\n");
                    }

                    appendIndent(output, indent + 2);
                    output.Append("]\n");
                }

                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Tuple: {
                TupleNode n = (TupleNode)node;
                appendHeader(output, n, "Tuple", indent);
                output.Append(" {\n");
                appendNodeList(output, "elements", n.elements, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Result: {
                ResultNode n = (ResultNode)node;
                appendHeader(output, n, "Result", indent,
                    ("isError", n.isError ? "true" : "false"));
                output.Append(" {\n");
                appendNodeField(output, "t", n.t, indent + 2);
                appendNodeField(output, "e", n.e, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Parameter: {
                ParameterNode n = (ParameterNode)node;
                appendHeader(output, n, "Parameter", indent,
                    ("parameterName", n.parameterName));
                output.Append(" {\n");
                appendNodeField(output, "parameterRawType", n.parameterRawType, indent + 2);
                appendNodeField(output, "defaultValue", n.defaultValue, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Modifier: {
                ModifierNode n = (ModifierNode)node;
                appendHeader(output, n, "Modifier", indent,
                    ("modifier", n.modifier.ToString()));
                return;
            }

            case ASTNodeType.EnumMember: {
                EnumMemberNode n = (EnumMemberNode)node;
                appendHeader(output, n, "EnumMember", indent, ("name", n.name));
                output.Append(" {\n");
                appendNodeField(output, "value", n.value, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Enum: {
                EnumNode n = (EnumNode)node;
                appendHeader(output, n, "Enum", indent, ("name", n.name));
                output.Append(" {\n");
                appendNodeList(output, "decorators", n.decorators, indent + 2);
                appendNodeList(output, "modifiers", n.modifiers, indent + 2);
                appendNodeList(output, "elements", n.elements, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.InterfaceField: {
                InterfaceFieldNode n = (InterfaceFieldNode)node;
                appendHeader(output, n, "InterfaceField", indent,
                    ("name", n.name),
                    ("isNullable", n.isNullable ? "true" : "false"),
                    ("isFunction", n.isFunction ? "true" : "false"));
                output.Append(" {\n");
                appendNodeField(output, "rawType", n.rawType, indent + 2);
                appendNodeList(output, "parameters", n.parameters, indent + 2);
                appendNodeField(output, "returnType", n.returnType, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Interface: {
                InterfaceNode n = (InterfaceNode)node;
                appendHeader(output, n, "Interface", indent, ("name", n.name));
                output.Append(" {\n");
                appendNodeList(output, "decorators", n.decorators, indent + 2);
                appendNodeList(output, "modifiers", n.modifiers, indent + 2);
                appendGenericParameters(output, n.genericParameters, indent + 2);
                appendNodeList(output, "elements", n.elements, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Function: {
                FunctionNode n = (FunctionNode)node;
                appendHeader(output, n, "Function", indent,
                    ("name", n.name),
                    ("isIntrinsic", n.isIntrinsic ? "true" : "false"));
                output.Append(" {\n");
                appendNodeList(output, "decorators", n.decorators, indent + 2);
                appendNodeList(output, "modifiers", n.modifiers, indent + 2);
                appendGenericParameters(output, n.genericParameters, indent + 2);
                appendNodeList(output, "parameters", n.parameters, indent + 2);
                appendNodeField(output, "returnType", n.returnType, indent + 2);
                appendNodeField(output, "body", n.body, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Declaration: {
                DeclarationNode n = (DeclarationNode)node;
                appendHeader(output, n, "Declaration", indent,
                    ("isNullable", n.isNullable ? "true" : "false"),
                    ("isTypeInference", n.isTypeInference ? "true" : "false"));
                output.Append(" {\n");
                appendNodeList(output, "decorators", n.decorators, indent + 2);
                appendNodeList(output, "modifiers", n.modifiers, indent + 2);
                appendNodeField(output, "variable", n.variable, indent + 2);
                appendNodeField(output, "rawType", n.rawType, indent + 2);
                appendNodeField(output, "value", n.value, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Class: {
                ClassNode n = (ClassNode)node;
                appendHeader(output, n, "Class", indent, ("name", n.name));
                output.Append(" {\n");
                appendNodeField(output, "constructor", n.constructor, indent + 2);
                appendNodeField(output, "super", n.super, indent + 2);
                appendNodeList(output, "decorators", n.decorators, indent + 2);
                appendNodeList(output, "modifiers", n.modifiers, indent + 2);
                appendGenericParameters(output, n.genericParameters, indent + 2);
                appendNodeList(output, "fields", n.fields, indent + 2);
                appendNodeList(output, "methods", n.methods, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Decorator: {
                DecoratorNode n = (DecoratorNode)node;
                appendHeader(output, n, "Decorator", indent, ("name", n.name));
                output.Append(" {\n");
                appendNodeList(output, "decorators", n.decorators, indent + 2);
                appendNodeList(output, "modifiers", n.modifiers, indent + 2);
                appendNodeList(output, "parameters", n.parameters, indent + 2);
                appendNodeField(output, "body", n.body, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Namespace: {
                NamespaceNode n = (NamespaceNode)node;
                appendHeader(output, n, "Namespace", indent);
                output.Append(" {\n");
                appendNodeField(output, "name", n.name, indent + 2);
                appendNodeList(output, "body", n.body, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            case ASTNodeType.Import: {
                ImportNode n = (ImportNode)node;
                appendHeader(output, n, "Import", indent,
                    ("moduleName", n.moduleName),
                    ("alias", n.alias),
                    ("importType", n.importType.ToString()));
                return;
            }

            case ASTNodeType.Preprocessor: {
                PreprocessorDirectiveNode n = (PreprocessorDirectiveNode)node;

                if (n.value.Length == 0)
                    appendHeader(output, n, "Preprocessor", indent,
                        ("directive", n.directive.ToString()));
                else
                    appendHeader(output, n, "Preprocessor", indent,
                        ("directive", n.directive.ToString()),
                        ("value", n.value));

                return;
            }

            case ASTNodeType.Module: {
                ModuleNode n = (ModuleNode)node;
                appendHeader(output, n, "Module", indent, ("moduleName", n.moduleName));
                output.Append(" {\n");
                appendNodeList(output, "body", n.body, indent + 2);
                appendIndent(output, indent);
                output.Append('}');
                return;
            }

            default:
                appendHeader(output, node, node.type.ToString(), indent);
                return;
        }
    }

    static void appendHeader(StringBuilder output, ASTNode node, string nodeName, int indent,
        params (string name, string value)[] fields) {
        appendIndent(output, indent);
        output.Append(nodeName);

        SourceSpan span = node.sourceSpan;
        bool hasSource = span.line != 0 || span.column != 0 || !string.IsNullOrEmpty(span.filePath);
        if (!hasSource && fields.Length == 0) return;

        output.Append('(');
        bool first = true;

        if (span.line != 0 || span.column != 0) {
            appendHeaderField(output, ref first, "line", span.line.ToString());
            appendHeaderField(output, ref first, "column", span.column.ToString());
        }

        if (!string.IsNullOrEmpty(span.filePath))
            appendHeaderField(output, ref first, "filePath", span.filePath);

        foreach ((string name, string value) in fields)
            appendHeaderField(output, ref first, name, value);

        output.Append(')');
    }

    static void appendHeaderField(StringBuilder output, ref bool first, string name, string value) {
        if (!first) output.Append(", ");
        output.Append(name).Append(": ").Append(value);
        first = false;
    }

    static void appendNodeField(StringBuilder output, string name, ASTNode? node, int indent) {
        appendIndent(output, indent);
        output.Append(name).Append(':');

        if (node == null) {
            output.Append(" null\n");
            return;
        }

        output.Append('\n');
        appendNode(output, node, indent + 2);
        output.Append('\n');
    }

    static void appendNodeList<T>(StringBuilder output, string name, List<T>? nodes, int indent) where T : ASTNode {
        appendIndent(output, indent);
        output.Append(name).Append(": ");

        if (nodes == null || nodes.Count == 0) {
            output.Append("[]\n");
            return;
        }

        output.Append("[\n");

        foreach (T node in nodes) {
            appendNode(output, node, indent + 2);
            output.Append('\n');
        }

        appendIndent(output, indent);
        output.Append("]\n");
    }

    static void appendGenericParameters(StringBuilder output, List<GenericParameter>? parameters, int indent) {
        appendIndent(output, indent);
        output.Append("genericParameters: ");

        if (parameters == null || parameters.Count == 0) {
            output.Append("[]\n");
            return;
        }

        output.Append("[\n");

        foreach (GenericParameter parameter in parameters) {
            appendIndent(output, indent + 2);
            output.Append("GenericParameter(name: ").Append(parameter.name);

            SourceSpan span = parameter.sourceSpan;

            if (span.line != 0 || span.column != 0) {
                output.Append(", line: ").Append(span.line);
                output.Append(", column: ").Append(span.column);
            }

            if (!string.IsNullOrEmpty(span.filePath))
                output.Append(", filePath: ").Append(span.filePath);

            output.Append(")\n");
        }

        appendIndent(output, indent);
        output.Append("]\n");
    }

    static void appendIndent(StringBuilder output, int indent) => output.Append(' ', indent);
}