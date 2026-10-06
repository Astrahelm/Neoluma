using Neoluma.Core.Extras;
using Neoluma.Libraries;

namespace Neoluma.Core.Frontend;

public class Parser {
    private List<Token> tokens = new();
    private int pos = 0;
    string moduleName = string.Empty;

    // ErrorManager is used to report errors
    public ErrorManager errorManager = null!;
    public ModuleNode? moduleSource;

    // ==== Main functions ====
    public void parseModule(List<Token> tok, string moduleName) { // main parsing
        moduleSource = null; tokens = tok; this.moduleName = moduleName; pos = 0;

        var moduleNode = new ModuleNode(moduleName);
        moduleNode.sourceSpan = new SourceSpan(curToken().sourceSpan.filePath, 0, 0);

        // FIXME: Reevaluate why the fuck this duct tape method exists. Remake safety guards, since i don't remember it's purpose
        while (!isAtEnd()) {
            if (match(Delimeters.Semicolon)) { next(); continue; }

            ASTNode? stmt = parseStatement();
            if (stmt != null && match(Delimeters.RightBraces)) { next(); continue; }
            if (stmt == null) {
                while (!isAtEnd() && !isNextLine()) next();

                int startPos = pos;
                int guard = 0;
                while (!isAtEnd() && guard++ < 100){
                    if (isNextLine()){
                        next();
                        break;
                    }
                    if (match(Delimeters.RightBraces)){
                        break;
                    }
                    next();
                }

                if (pos == startPos && !isAtEnd()) next();

                if (guard >= 100) {
                    Console.WriteLine($"{Color.TextHex("#ff5050")}{String.Format(Localization.translate("Compiler.Core.ErrorManager.safetyGuard"), "parseModule", curToken().sourceSpan.filePath, curToken().sourceSpan.line, curToken().sourceSpan.column)}{Color.Reset}");
                    break;
                }

                continue;
            }
            moduleNode.body.Add(stmt);
        }

        moduleSource = moduleNode;
    }
    public void printModule(int indentation = 0) {
        if (moduleSource == null) {
            Console.WriteLine($"[Neoluma/Parser][{nameof(printModule)}] No module to print.");
            return;
        }
        
        Console.WriteLine(ASTPrinter.toString(moduleSource, indentation));
    }

    // ==== Parser helpers ====
    Token curToken() {
        if (pos >= tokens.Count) return new Token(TokenType.EndOfFile, "", default);
        return tokens[pos];
    }
    Token lookBack() {
        if (pos >= tokens.Count || pos == 0) return new Token(TokenType.EndOfFile, "", default);
        return tokens[pos - 1];
    }
    Token next() {
        if (pos >= tokens.Count) return new Token(TokenType.EndOfFile, "", default);
        return tokens[pos++];
    }
    Token lookupNext() {
        if (pos + 1 >= tokens.Count) return new Token(TokenType.EndOfFile, "", default);
        return tokens[pos + 1];
    }
    bool isAtEnd() {
        return pos >= tokens.Count || curToken().type == TokenType.EndOfFile;
    }

    bool match(TokenType type, string value) {
        if (curToken().type != type) return false;
        if (curToken().value != value) return false;
        return true;
    }
    bool match(TokenType type) {
        if (curToken().type != type) return false;
        return true;
    }
    bool match(Token token, TokenType type) {
        if (token.type != type) return false;
        return true;
    }
    bool match(Keywords expected) => match(curToken(), expected);
    bool match(Operators expected) => match(curToken(), expected);
    bool match(Delimeters expected) => match(curToken(), expected);
    bool match(Preprocessors expected) => match(curToken(), expected);
    bool match(Decorators expected) => match(curToken(), expected);
    bool match(Token token, Keywords expected) {
        if (token.type != TokenType.Keyword) return false;
        return TokenMaps.keywords.TryGetValue(token.value, out Keywords actual) && actual == expected;
    }
    bool match(Token token, Operators expected) {
        if (token.type != TokenType.Operator) return false;
        return TokenMaps.operators.TryGetValue(token.value, out Operators actual) && actual == expected;
    }
    bool match(Token token, Delimeters expected) {
        if (token.type != TokenType.Delimeter) return false;
        return TokenMaps.delimeters.TryGetValue(token.value, out Delimeters actual) && actual == expected;
    }
    bool match(Token token, Preprocessors expected) {
        if (token.type != TokenType.Preprocessor) return false;
        return TokenMaps.preprocessors.TryGetValue(token.value, out Preprocessors actual) && actual == expected;
    }
    bool match(Token token, Decorators expected) {
        if (token.type != TokenType.Decorator) return false;
        return TokenMaps.decorators.TryGetValue(token.value, out Decorators actual) && actual == expected;
    }

    // to make math order
    static int getOperatorPrecedence(string op) {
        if (!TokenMaps.operators.TryGetValue(op, out Operators operation)) return -3;

        switch (operation) {
            case Operators.Power:
                return 7;
            case Operators.Multiply:
            case Operators.Divide:
            case Operators.Modulo:
                return 6;
            case Operators.Add:
            case Operators.Subtract:
                return 5;
            case Operators.BitwiseLeftShift:
            case Operators.BitwiseRightShift:
                return 4;
            case Operators.BitwiseAnd:
                return 3;
            case Operators.BitwiseXOr:
                return 2;
            case Operators.BitwiseOr:
                return 1;
            case Operators.Equal:
            case Operators.NotEqual:
            case Operators.LessThan:
            case Operators.GreaterThan:
            case Operators.LessThanOrEqual:
            case Operators.GreaterThanOrEqual:
                return 0;
            case Operators.LogicalAnd:
                return -1;
            case Operators.LogicalOr:
                return -2;
            default:
                return -3;
        }
    }
    static bool isAssignmentOperator(string op) {
        if (!TokenMaps.operators.TryGetValue(op, out Operators operation)) return false;

        switch (operation) {
            case Operators.Assign:
            case Operators.AddAssign:
            case Operators.SubAssign:
            case Operators.MulAssign:
            case Operators.DivAssign:
            case Operators.ModAssign:
            case Operators.PowerAssign:
                return true;
            default:
                return false;
        }
    }
    
    // ==== Helper functions ====
    // Parses either a block or a single statement after an 'if' condition.
    ASTNode? parseBlockorStatement() {
        if (match(Delimeters.LeftBraces)) {
            BlockNode? block = parseBlock();
            return block;
        }

        ASTNode? statement = parseStatement();
        if(statement == null) return null;
        return statement;
    }
    FunctionNode? parseConstructor(List<CallExpressionNode> decorators, List<ModifierNode> modifiers) {
        Token nameToken = curToken();
        if (!match(TokenType.Identifier)) {
            errorManager.addError(SyntaxErrors.MissingToken,
                new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                "ErrorManager.Syntax.MissingToken.functionName.message", [],
                "ErrorManager.Syntax.MissingToken.functionName.hint");
            return null;
        }
        string funcName = nameToken.value;
        next();

        if (!match(Delimeters.LeftParen)) {
            errorManager.addError(SyntaxErrors.MissingToken,
                new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                "ErrorManager.Syntax.MissingToken.functionparameters.message", [funcName],
                "ErrorManager.Syntax.MissingToken.functionparameters.hint", [funcName]);
            return null;
        }
        next();

        List<ParameterNode> parameters = new();
        while (!match(Delimeters.RightParen)) {
            Token paramName = curToken();
            if (paramName.type != TokenType.Identifier) {
                errorManager.addError(SyntaxErrors.MissingToken,
                    new ErrorSpan(paramName.value, paramName.sourceSpan),
                    "ErrorManager.Syntax.MissingToken.functionParamName.message", [funcName],
                    "ErrorManager.Syntax.MissingToken.functionParamName.hint");
                return null;
            }
            next();
            RawTypeNode? type = null;
            if (match(Delimeters.Colon)) {
                next();
                type = parseType();
            }

            ASTNode? defaultValue = null;
            if (match(Operators.Assign)) {
                next();
                defaultValue = parseExpression();
                if (defaultValue == null) {
                    errorManager.addError(SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.functionParamDefault.message", [],
                        "ErrorManager.Syntax.MissingToken.functionParamDefault.hint");
                    return null;
                }
            }
            ParameterNode param = new(paramName.value, type, defaultValue);
            param.sourceSpan = paramName.sourceSpan;
            parameters.Add(param);
            if (match(Delimeters.Comma)) next();
            else break;
        }

        if (!match(Delimeters.RightParen)) {
            errorManager.addError(SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.functionClosingParen.message", [funcName],
                "ErrorManager.Syntax.MissingToken.functionClosingParen.hint");
            return null;
        }
        next();

        RawTypeNode? returnType = null;
        if (match(Operators.TypeArrow)) {
            next();
            returnType = parseType();
        }

        BlockNode? body = parseBlock();
        if (body == null) {
            errorManager.addError(SyntaxErrors.InvalidStatement,
                new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                "ErrorManager.Syntax.MissingToken.functionBody.message", [funcName],
                "ErrorManager.Syntax.MissingToken.functionBody.hint", [funcName]);
            return null;
        }

        FunctionNode node = new(funcName, parameters, returnType, body, decorators, modifiers);
        foreach (var modifier in modifiers) {
            if (modifier.modifier == ASTModifierType.Intrinsic) {
                node.isIntrinsic = true;
                node.body = null;
            }
        }
        node.sourceSpan = nameToken.sourceSpan;
        return node;
    }
    // Detects nextline expression
    bool isNextLine() {
        if (match(Delimeters.Semicolon) || match(Delimeters.Newline)) return true;
        return false;
    }
    // Detects whether upcoming tokens are identifier or member access followed by an assignment operator
    // This code is pure unreadable garbage so lemme explain
    bool isAssignableAhead(int offset = 0) {
        // Take up new offset
        int p = pos + offset;

        // First token must be identifier for sure
        if (p >= tokens.Count) return false;
        if (tokens[p].type != TokenType.Identifier) return false;
        p++;

        // Then we create a chain of member accesses and identifiers/function calls inside them
        while (p < tokens.Count) {

            // If function call
            if (match(tokens[p], Delimeters.LeftParen)) {
                int depth = 1;
                p++; // consume left parenthesis

                // to be fair, i could not care less what's inside the () while we look ahead, so just skip until we find the matching right parenthesis
                while (p < tokens.Count && depth > 0) {
                    if (match(tokens[p], Delimeters.LeftParen)) depth++;
                    else if (match(tokens[p], Delimeters.RightParen)) depth--;
                    p++;
                }

                continue;
            }

            // If member access however, we parse it too.
            if (match(tokens[p], Delimeters.Dot)) {
                p++; // consume dot
                if (p >= tokens.Count) return false;
                if (tokens[p].type != TokenType.Identifier) return false;
                p++;
                continue;
            }
            break;
        }

        // After all of that we check if there's assignment operator.
        if (p < tokens.Count && tokens[p].type == TokenType.Operator && isAssignmentOperator(tokens[p].value)) return true;
        // whoops, not an assignment
        return false;
    }
    string namespaceNameToString(ASTNode? node) {
        if (node == null) return "<unknown>";

        if (node is VariableNode variable) return variable.varName;

        if (node is MemberAccessNode ma) 
            return namespaceNameToString(ma.parent) + "." + namespaceNameToString(ma.val);

        return "<invalid namespace>";
    }
    bool consumeTypeCloseAngle() {
        if (match(Operators.GreaterThan)) {
            next();
            return true;
        }

        if (match(Operators.BitwiseRightShift)) {
            Token token = tokens[pos];
            tokens[pos] = new Token(token.type, ">", new SourceSpan(token.sourceSpan.filePath, token.sourceSpan.line, token.sourceSpan.column + 1));
            return true;
        }

        return false;
    }
    List<RawTypeNode>? parseGenericArguments(string typeName) {
        List<RawTypeNode> arguments = new();
        
        if (!match(Operators.LessThan)) return arguments;

        Token openToken = curToken();
        next();

        if (match(Operators.GreaterThan) || match(Operators.BitwiseRightShift)) {
            errorManager.addError(SyntaxErrors.MissingToken, new ErrorSpan(openToken.value, openToken.sourceSpan),
                "ErrorManager.Syntax.MissingToken.genericTypeArgument.message", [typeName],
                "ErrorManager.Syntax.MissingToken.genericTypeArgument.hint");
            return null;
        }

        while (!isAtEnd()) {
            if (match(Delimeters.Comma) || match(Operators.GreaterThan) || match(Operators.BitwiseRightShift)) {
                errorManager.addError(SyntaxErrors.MissingToken,
                    new ErrorSpan(curToken().value, curToken().sourceSpan),
                    "ErrorManager.Syntax.MissingToken.genericTypeArgument.message", [typeName],
                    "ErrorManager.Syntax.MissingToken.genericTypeArgument.hint");
                return null;
            }

            RawTypeNode? argument = parseType();
            if (argument == null) return null;
            arguments.Add(argument);

            if (match(Delimeters.Comma)) {
                next();
                if (match(Operators.GreaterThan) || match(Operators.BitwiseRightShift)) {
                    errorManager.addError(SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.genericTypeArgument.message", [typeName],
                        "ErrorManager.Syntax.MissingToken.genericTypeArgument.hint");
                    return null;
                }
                continue;
            }
            break;
        }

        if (!match(Operators.GreaterThan)) {
            errorManager.addError(SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.genericClosingAngle.message", [typeName],
                "ErrorManager.Syntax.MissingToken.genericClosingAngle.hint");
            return null;
        }
        next();

        return arguments;
    }
    
    // ==== Parser functions ====
    // Expression parsing
    ASTNode? parsePrimary() {
        Token token = curToken();
        
        // Parenthesis, lambdas, arrays, sets, dicts
        if (match(TokenType.Delimeter)) {
            // Parenthesis / lambdas
            if (match(Delimeters.LeftParen)) {
                next();
                List<ASTNode> exprs = new();
                while (!match(Delimeters.RightParen)) {
                    ASTNode? expr = parseExpression();
                    if (expr == null) {
                        errorManager.addError(
                            SyntaxErrors.MissingToken,
                            new ErrorSpan(token.value, token.sourceSpan),
                            "ErrorManager.Syntax.MissingToken.missingExpressionLambda.message", ["("],
                            "ErrorManager.Syntax.MissingToken.missingExpressionLambda.hint");
                        return null;
                    }
                    exprs.Add(expr);
                    if (curToken().type == TokenType.Delimeter && isNextLine()) next();
                    if (match(Delimeters.Comma)) next();
                    else break;
                }
                if (!match(Delimeters.RightParen)) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(token.value, token.sourceSpan),
                        "ErrorManager.Syntax.MissingToken.closingParen.message", ["("],
                        "ErrorManager.Syntax.MissingToken.closingParen.hint", ["("]);
                    return null;
                }
                next();
                // Check for lambda expressions here
                if (match(Operators.AssignmentArrow)) {
                    next();
                    var block = parseBlock();
                    if (block == null) {
                        errorManager.addError(
                            SyntaxErrors.InvalidStatement,
                            new ErrorSpan(curToken().value, curToken().sourceSpan),
                            "ErrorManager.Syntax.InvalidStatement.missedBlock.message", ["=>"],
                            "ErrorManager.Syntax.InvalidStatement.missedBlock.hint", ["=>", "=>"]);
                        return null;
                    }
                    return new LambdaNode(exprs, block);
                }

                if (exprs.Count == 1) return exprs.Last();
                return new TupleNode(exprs);
            }
            // Arrays
            if (match(Delimeters.LeftBracket)) {
                if (match(lookBack(), TokenType.Identifier) && (match(lookupNext(), TokenType.Delimeter) && match(lookupNext(), Delimeters.RightBracket))) next();
                else {
                    // TODO: implement the strict types for arrays, sets, dicts?
                    next();
                    List<ASTNode> e = new();

                    while (!match(Delimeters.RightBracket)) {
                        if (isAtEnd()) {
                            errorManager.addError(
                                SyntaxErrors.MissingToken,
                                new ErrorSpan(token.value, token.sourceSpan),
                                "ErrorManager.Syntax.MissingToken.closingBracket.message", [],
                                "ErrorManager.Syntax.MissingToken.closingBracket.hint");
                            return null;
                        }

                        var element = parseExpression();
                        if (element == null) return null;

                        e.Add(element);
                        if (curToken().type == TokenType.Delimeter && isNextLine()) next(); // To allow multiline expressions of arrays. I think this would be absolutely neat sugar for everybody.
                        if (match(Delimeters.Comma)) next();
                        else if (!match(Delimeters.RightBracket)) {
                            errorManager.addError(
                                SyntaxErrors.MissingToken,
                                new ErrorSpan(curToken().value, curToken().sourceSpan),
                                "ErrorManager.Syntax.MissingToken.closingBracket.message", [],
                                "ErrorManager.Syntax.MissingToken.closingBracket.hint");
                            return null;
                        }
                    }
                    next();
                    return new ArrayNode(e);
                }
            }
            // Sets / dicts
            if (match(Delimeters.LeftBraces)) {
                // TODO next time: Allow dicts and sets to dictate their explicit type (when you come back)
                next();
                bool isDict = (match(lookupNext(), TokenType.Delimeter) && match(lookupNext(), Delimeters.Colon));

                if (isDict) {
                    List<(ASTNode, ASTNode)> elements = new();
                    while (!match(Delimeters.RightBraces)) {
                        if (isAtEnd()) {
                            errorManager.addError(
                                SyntaxErrors.MissingToken,
                                new ErrorSpan(token.value, token.sourceSpan),
                                "ErrorManager.Syntax.MissingToken.closingBrace.message", ["dict"],
                                "ErrorManager.Syntax.MissingToken.closingBrace.hint");
                            return null;
                        }

                        Token keyToken = curToken();
                        var key = parseExpression();
                        if (key == null) return null;
                        if (!match(Delimeters.Colon)) {
                            errorManager.addError(
                                SyntaxErrors.MissingToken,
                                new ErrorSpan(curToken().value, curToken().sourceSpan),
                                "ErrorManager.Syntax.MissingToken.dictColonAfterKey.message", [keyToken.value],
                                "ErrorManager.Syntax.MissingToken.dictColonAfterKey.hint");
                            return null;
                        }
                        next();
                        var val = parseExpression();
                        if (val == null) return null;
                        elements.Add((key, val));
                        if (match(Delimeters.Comma)) next();
                        else if (!match(Delimeters.RightBraces)) {
                            errorManager.addError(
                                SyntaxErrors.MissingToken,
                                new ErrorSpan(curToken().value, curToken().sourceSpan),
                                "ErrorManager.Syntax.MissingToken.closingBrace.message", ["dict"],
                                "ErrorManager.Syntax.MissingToken.closingBrace.hint");
                            return null;
                        }
                    }
                    next();
                    return new DictNode(elements);
                }

                List<ASTNode> e = new();
                while (!match(Delimeters.RightBraces)) {
                    if (isAtEnd()) {
                        errorManager.addError(
                            SyntaxErrors.MissingToken,
                            new ErrorSpan(token.value, token.sourceSpan),
                            "ErrorManager.Syntax.MissingToken.closingBrace.message", ["set"],
                            "ErrorManager.Syntax.MissingToken.closingBrace.hint");
                        return null;
                    }

                    var element = parseExpression();
                    if (element == null) return null;

                    e.Add(element);
                    if (match(Delimeters.Comma)) next();
                    else if (!match(Delimeters.RightBraces)) {
                        errorManager.addError(
                            SyntaxErrors.MissingToken,
                            new ErrorSpan(curToken().value, curToken().sourceSpan),
                            "ErrorManager.Syntax.MissingToken.closingBrace.message", ["set"],
                            "ErrorManager.Syntax.MissingToken.closingBrace.hint");
                        return null;
                    }
                }
                next();
                return new SetNode(e);
            }
        }
        // Data type + Booleans
        else if ((match(TokenType.Number) || match(TokenType.String))
        || (match(TokenType.Identifier) && (token.value == "true" || token.value == "false"))) {
            next();
            return new LiteralNode(token.value);
        }
        // Null
        else if (match(TokenType.Null)) {
            next();
            return new LiteralNode("null");
        }

        // Identifier/variable or function call
        else if (match(TokenType.Identifier)) {
            Token id = next();
            ASTNode node;

            // If function call
            if (match(Delimeters.LeftParen)) {
                next();
                List<ASTNode> args = new();

                while (!match(Delimeters.RightParen)) {
                    var arg = parseExpression();
                    if (arg != null) args.Add(arg);

                    if (match(Delimeters.Comma)) next();
                    else break;
                }
                if (!match(Delimeters.RightParen)) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(token.value, token.sourceSpan),
                        "ErrorManager.Syntax.MissingToken.closingParen.message", [token.value],
                        "ErrorManager.Syntax.MissingToken.closingParen.hint", [token.value]);
                    return null;
                }
                next();

                var callee = new VariableNode(id.value);
                node = new CallExpressionNode(callee, args);
            } else {
                // else identifier/variable
                node = new VariableNode(id.value);
            }

            while (match(Delimeters.Dot)) {
                next();
                ASTNode parent = node;

                if (!match(TokenType.Identifier)) {
                    errorManager.addError(
                        SyntaxErrors.UnexpectedToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.UnexpectedToken.message", [curToken().value],
                        "ErrorManager.Syntax.UnexpectedToken.hint");
                    return null;
                }

                Token memberToken = curToken();
                ASTNode? member = parsePrimary();
                if (member == null) return null;
                if (member.type != ASTNodeType.Variable && member.type != ASTNodeType.CallExpression) {
                    errorManager.addError(
                        SyntaxErrors.InvalidStatement,
                        new ErrorSpan(memberToken.value, memberToken.sourceSpan),
                        "ErrorManager.Syntax.InvalidStatement.message", [],
                        "ErrorManager.Syntax.InvalidStatement.hint");
                    return null;
                }
                node = new MemberAccessNode(parent, member);
            }
            node.sourceSpan = id.sourceSpan;
            return node;
        }

        errorManager.addError(
            SyntaxErrors.UnexpectedToken,
            new ErrorSpan(token.value, token.sourceSpan),
            "ErrorManager.Syntax.UnexpectedToken.message", [token.value],
            "ErrorManager.Syntax.UnexpectedToken.hint");
        return null;
    }
    // Note: does not require next(); after it
    ASTNode? parseExpression() {
        Token token = curToken();

        // Unary operations
        if (TokenMaps.operators.ContainsKey(token.value)) {
            if (match(token, Operators.LogicalNot) || match(token, Operators.Subtract)) {
                next();
                return parseUnary(token.value);
            }
        }

        // Assignment
        if (match(TokenType.Identifier) && isAssignableAhead(0)) {
            return parseAssignment();
        }

        // Fallback to binary
        return parseBinary(0);
    }
    ASTNode? parseBinary(int prevPredecence = 0) {
        ASTNode? left = parsePrimary();
        if (left == null) return null;

        while (true) {
            Token token = curToken();
            int predecence = getOperatorPrecedence(token.value);
            if (token.type != TokenType.Operator || predecence < prevPredecence) break;
            string op = token.value;
            next();

            ASTNode? right = parseBinary(predecence+1);
            if (right == null) return null;

            var node = new BinaryOperationNode(left, op, right);
            node.sourceSpan = token.sourceSpan;
            left = node;
        }

        return left;
    }
    UnaryOperationNode? parseUnary(string op) {
        ASTNode? operand = parsePrimary();
        if (operand == null) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(lookBack().value, lookBack().sourceSpan),
            "ErrorManager.Syntax.MissingToken.missingOperandUnary.message", [op],
            "ErrorManager.Syntax.MissingToken.missingOperandUnary.hint", [op]);
            return null;
        }
        return new UnaryOperationNode(op, operand);
    }

    List<GenericParameter>? parseGenericParameters(string ownerName) {
        List<GenericParameter> parameters = new();

        if (!match(Operators.LessThan)) return parameters;

        Token openToken = curToken();
        next();

        if (match(Operators.GreaterThan)) {
            errorManager.addError(SyntaxErrors.MissingToken,
                new ErrorSpan(openToken.value, openToken.sourceSpan),
                "ErrorManager.Syntax.MissingToken.genericParameter.message", [ownerName],
                "ErrorManager.Syntax.MissingToken.genericParameter.hint");
            return null;
        }

        while (!isAtEnd()) {
            Token nameToken = curToken();

            if (!match(TokenType.Identifier)) {
                errorManager.addError(SyntaxErrors.MissingToken,
                    new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                    "ErrorManager.Syntax.MissingToken.genericParameter.message", [ownerName],
                    "ErrorManager.Syntax.MissingToken.genericParameter.hint");
                return null;
            }

            foreach (var parameter in parameters) {
                if (parameter.name == nameToken.value) {
                    errorManager.addError(SyntaxErrors.InvalidStatement,
                        new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                        "ErrorManager.Syntax.InvalidStatement.duplicateGenericParameter.message", [nameToken.value, ownerName],
                        "ErrorManager.Syntax.InvalidStatement.duplicateGenericParameter.hint");
                    return null;
                }
            }

            parameters.Add(new GenericParameter(nameToken.value, new SourceSpan(nameToken.sourceSpan)));

            next();

            if (match(Delimeters.Comma)) {
                next();

                if (match(Operators.GreaterThan)) {
                    errorManager.addError(SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.genericParameter.message",
                        [ownerName],
                        "ErrorManager.Syntax.MissingToken.genericParameter.hint");
                    return null;
                }
                continue;
            }
            break;
        }

        if (!match(Operators.GreaterThan)) {
            errorManager.addError(SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.genericClosingAngle.message", [ownerName],
                "ErrorManager.Syntax.MissingToken.genericClosingAngle.hint");
            return null;
        }
        next();

        return parameters;
    }
    RawTypeNode? parseType() {
        Token typeToken = curToken();
        if (!match(TokenType.Identifier)) {
            errorManager.addError(SyntaxErrors.MissingToken,
                new ErrorSpan(typeToken.value, typeToken.sourceSpan),
                "ErrorManager.Syntax.InvalidStatement.noType.message", [],
                "ErrorManager.Syntax.InvalidStatement.noType.hint");
            return null;
        }

        string typeName = typeToken.value;
        VariableNode varType = new VariableNode(typeName);
        next();

        var result = parseGenericArguments(typeName);
        if (result == null) return null;

        List<RawTypeNode> genericArguments = result;

        ASTNode? varSize = null;
        if (match(Delimeters.LeftBracket)) {
            next();

            if (!match(Delimeters.RightBracket)) {
                varSize = parseExpression();
                if (varSize == null) {
                    errorManager.addError(SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.arraySize.message", [typeName],
                        "ErrorManager.Syntax.MissingToken.arraySize.hint");
                    return null;
                }
            }

            if (!match(Delimeters.RightBracket)) {
                errorManager.addError(SyntaxErrors.MissingToken,
                    new ErrorSpan(curToken().value, curToken().sourceSpan),
                    "ErrorManager.Syntax.MissingToken.closingBracket.message", [],
                    "ErrorManager.Syntax.MissingToken.closingBracket.hint");
                return null;
            }

            next();
        }

        var node = new RawTypeNode(varType, varSize, genericArguments);
        node.sourceSpan = typeToken.sourceSpan;
        return node;
    }

    // Statement parsing
    ASTNode? parseStatement() {
        while (isNextLine()) next(); // Skips newlines in case they ever appear
        if (isAtEnd()) return null;

        Token token = curToken();
        List<CallExpressionNode> decorators = new();

        if (match(TokenType.Preprocessor)) return parsePreprocessor();
        if (match(TokenType.Decorator)) decorators = parseDecoratorCalls();
        while (isNextLine()) next();

        var modifiers = parseModifiers();

        if (match(TokenType.Identifier) && (match(lookupNext(), Operators.Nullable) || match(lookupNext(), Delimeters.Colon)))
            return parseDeclaration(decorators, modifiers);

        token = curToken();
        // === Control Flow Keywords ===
        if (match(TokenType.Keyword)) {
            if (match(token, Keywords.If)) return parseIf();
            if (match(token, Keywords.Switch)) return parseSwitch();
            if (match(token, Keywords.Try)) return parseTryCatch();
            if (match(token, Keywords.For)) return parseFor();
            if (match(token, Keywords.While)) return parseWhile();
            if (match(token, Keywords.Return)) {
                next();

                // allow "return;" or "return }"
                if (isNextLine() || match(Delimeters.RightBraces)) {
                    if (isNextLine()) next();
                    var rsnode = new ReturnStatementNode(null);
                    rsnode.sourceSpan = token.sourceSpan;
                    return rsnode;
                }

                var expr = parseExpression();
                if (expr == null) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(lookBack().value, lookBack().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.noVariableAfter.message", ["return"],
                        "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", ["return"]);
                    return null;
                }

                if (isNextLine()) next();

                var node = new ReturnStatementNode(expr);
                node.sourceSpan = token.sourceSpan;
                return node;
            }
            if ((match(token, Keywords.Throw))) {
                next();
                if (isNextLine() || match(Delimeters.RightBraces)) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(token.value, token.sourceSpan),
                        "ErrorManager.Syntax.MissingToken.noVariableAfter.message", ["throw"],
                        "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", ["throw"]);
                    if (isNextLine()) next();
                    return null;
                }

                var expr = parseExpression();
                if (expr == null) return null;
                if (isNextLine()) next();

                var node = new ThrowStatementNode(expr);
                node.sourceSpan = token.sourceSpan;
                return node;
            }
            if (match(token, Keywords.Break)) {
                next();
                var node = new BreakStatementNode();
                node.sourceSpan = token.sourceSpan;
                return node;
            }
            if (match(token, Keywords.Continue)) {
                next();
                var node = new ContinueStatementNode();
                node.sourceSpan = token.sourceSpan;
                return node;
            }

            // for modifier affected structures
            if (match(token, Keywords.Function)) return parseFunction(decorators, modifiers);
            if (match(token, Keywords.Class)) return parseClass(decorators, modifiers);
            if (match(token, Keywords.Enum)) return parseEnum(decorators, modifiers);
            if (match(token,  Keywords.Interface)) return parseInterface(decorators, modifiers);
            if (match(token, Keywords.Decorator)) return parseDecorator(decorators, modifiers);
            // FIXME: why is this here?
            if (modifiers.Count == 0) {
                errorManager.addError(
                    SyntaxErrors.UnexpectedToken,
                    new ErrorSpan(token.value, token.sourceSpan),
                    "ErrorManager.Syntax.UnexpectedToken.message", [token.value],
                    "ErrorManager.Syntax.UnexpectedToken.hint");
                return null;
            }

            if (match(token, Keywords.Namespace)) return parseNamespace();
        }

        // === Block ===
        if (match(Delimeters.LeftBraces)) {
            return parseBlock();
        }

        // === Fallback ===
        return parseExpression(); // Default to expression statement
    }
    DeclarationNode? parseDeclaration(List<CallExpressionNode> decorators, List<ModifierNode> modifiers) {
        Token token = curToken();
        VariableNode var = new VariableNode(token.value);
        next();

        bool isNullable = false;
        if (match(Operators.Nullable)) { isNullable = true; next(); }

        RawTypeNode? rawType = null;
        if (!match(Delimeters.Colon)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(token.value, token.sourceSpan),
                "ErrorManager.Syntax.MissingToken.colonAfterVar.message", [token.value],
                "ErrorManager.Syntax.MissingToken.colonAfterVar.hint");
            return null;
        }
        next();

        bool isTypeInference = false;
        ASTNode? value = null;
        if (match(Operators.Assign)) {
            isTypeInference = true;
            next();
            value = parseExpression();
            if (value == null) {
                errorManager.addError(
                    SyntaxErrors.MissingToken,
                    new ErrorSpan(token.value, token.sourceSpan),
                    "ErrorManager.Syntax.MissingToken.noVariableAfter.message", ["="],
                    "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", ["="]);
                return null;
            }
        } else {
            rawType = parseType();
            if (match(Operators.Assign)){
                next();
                value = parseExpression();
                if (value == null) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(token.value, token.sourceSpan),
                        "ErrorManager.Syntax.MissingToken.noVariableAfter.message", ["="],
                        "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", ["="]);
                    return null;
                }
            }
        }

        var node = new DeclarationNode(var, rawType, value, isNullable, isTypeInference, decorators, modifiers);
        node.sourceSpan = token.sourceSpan;
        return node;
    }
    AssignmentNode? parseAssignment() {
        Token varToken = curToken();
        ASTNode? var = parsePrimary();
        if (var == null) return null;
        if (var.type != ASTNodeType.Variable && var.type != ASTNodeType.MemberAccess) {
            errorManager.addError(
                SyntaxErrors.InvalidStatement,
                new ErrorSpan(varToken.value, varToken.sourceSpan),
            "ErrorManager.Syntax.InvalidStatement.message", [],
            "ErrorManager.Syntax.InvalidStatement.hint");
            return null;
        }

        Token token = curToken();
        string op = token.value;
        next();

        ASTNode? value = parseExpression();
        if (value == null) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(token.value, token.sourceSpan),
            "ErrorManager.Syntax.MissingToken.noVariableAfter.message", [op],
            "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", [op]);
            return null;
        }

        var node = new AssignmentNode(var, op, value);
        node.sourceSpan = token.sourceSpan;
        return node;
    }

    // Control flow
    IfNode? parseIf() {
        var token = curToken();
        next();

        if (!match(Delimeters.LeftParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(token.value, token.sourceSpan),
                "ErrorManager.Syntax.MissingToken.openingParen.message", ["if"],
                "ErrorManager.Syntax.MissingToken.openingParen.hint", ["if"]);
            return null;
        }
        next(); // consume '('
        if (match(Delimeters.RightParen)) {
            errorManager.addError(
                SyntaxErrors.InvalidStatement,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.InvalidStatement.emptyCondition.message", ["if"],
                "ErrorManager.Syntax.InvalidStatement.emptyCondition.hint", ["if"]);
            next(); // consume ')'
            return null;
        }

        ASTNode? condition = parseExpression();
        if (condition == null) {
            errorManager.addError(
                SyntaxErrors.InvalidStatement,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.InvalidStatement.expectedCondition.message", ["if"],
                "ErrorManager.Syntax.InvalidStatement.expectedCondition.hint", ["if"]);
            return null;
        }
        if (!match(Delimeters.RightParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.closingParen.message", ["if"],
                "ErrorManager.Syntax.MissingToken.closingParen.hint", ["if"]);
            return null;
        }
        next();

        ASTNode? ifBlock = parseBlockorStatement();
        ASTNode? elseBlock = null;

        if (match(Keywords.Else)) {
            next();
            elseBlock = parseBlockorStatement();
        }

        var node = new IfNode(condition, ifBlock, elseBlock);
        node.sourceSpan = token.sourceSpan;
        return node;
    }
    SwitchNode? parseSwitch() {
        var token = curToken();
        next();

        if (!match(Delimeters.LeftParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(token.value, token.sourceSpan),
                "ErrorManager.Syntax.MissingToken.openingParen.message", ["switch"],
                "ErrorManager.Syntax.MissingToken.openingParen.hint", ["switch"]);
            return null;
        }
        next();

        ASTNode? expr = parseExpression();
        if (expr == null) {
            errorManager.addError(
                SyntaxErrors.InvalidStatement,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.InvalidStatement.expectedCondition.message", ["switch"],
                "ErrorManager.Syntax.InvalidStatement.expectedCondition.hint", ["switch"]);
            return null;
        }

        if (!match(Delimeters.RightParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.closingParen.message", ["switch"],
                "ErrorManager.Syntax.MissingToken.closingParen.hint", ["switch"]);
            return null;
        }
        next();

        if (!match(Delimeters.LeftBraces)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.openingBrace.message", ["switch"],
                "ErrorManager.Syntax.MissingToken.openingBrace.hint");
            return null;
        }
        next();
        while (match(Delimeters.Semicolon)) next();

        List<CaseNode> cases = new();
        SCDefaultNode? defaultCase = null;

        while (!match(Delimeters.RightBraces)) {
            Token tok = curToken();

            if (match(Keywords.Case)) {
                next(); // consume 'case'
                ASTNode? condition = parseExpression();
                if (!match(Delimeters.Colon)) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.colonInCase.message", [],
                        "ErrorManager.Syntax.MissingToken.colonInCase.hint");
                    return null;
                }
                next(); // consume ':'
                var body = parseBlockorStatement();
                if (body == null) {
                    errorManager.addError(
                        SyntaxErrors.InvalidStatement,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.InvalidStatement.expectedBody.message", ["case"],
                        "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", ["case"]);
                    return null;
                }
                while (match(Delimeters.Semicolon)) next();
                cases.Add(new CaseNode(condition, body));
            }
            else if (match(Keywords.Default)) {
                next();
                if (!match(Delimeters.Colon)) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.colonInDefault.message", [],
                        "ErrorManager.Syntax.MissingToken.colonInDefault.hint");
                    return null;
                }
                next();
                var body = parseBlockorStatement();
                if (body == null) {
                    errorManager.addError(
                        SyntaxErrors.InvalidStatement,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.InvalidStatement.expectedBody.message", ["default"],
                        "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", ["default"]);
                    return null;
                }
                while (match(Delimeters.Semicolon)) next();
                defaultCase = new SCDefaultNode(body);
            }
            else if (isNextLine()) next();
            else {
                errorManager.addError(
                    SyntaxErrors.UnexpectedToken,
                    new ErrorSpan(tok.value, tok.sourceSpan),
                    "ErrorManager.Syntax.InvalidStatement.unexpectedInSwitch.message", [tok.value],
                    "ErrorManager.Syntax.InvalidStatement.unexpectedInSwitch.hint");
                return null;
            }
        }
        if (!match(Delimeters.RightBraces)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.closingBrace.message", ["switch"],
                "ErrorManager.Syntax.MissingToken.closingBrace.hint");
            return null;
        }
        next();
        var node = new SwitchNode(expr, cases, defaultCase);
        node.sourceSpan = token.sourceSpan;
        return node;
    }
    TryCatchNode? parseTryCatch() {
        var token = curToken();
        next();

        var tryBlock = parseBlock();
        if (tryBlock == null) {
            errorManager.addError(
                SyntaxErrors.InvalidStatement,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.InvalidStatement.expectedBody.message", ["try"],
                "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", ["try"]);
            return null;
        }

        if (!match(Keywords.Catch)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.catchAfterTry.message", [],
                "ErrorManager.Syntax.MissingToken.catchAfterTry.hint");
            return null;
        }
        next();

        if (!match(Delimeters.LeftParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.openingParen.message", ["catch"],
                "ErrorManager.Syntax.MissingToken.openingParen.hint", ["catch"]);
            return null;
        }
        next();

        if (!match(TokenType.Identifier)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.exceptionVar.message", [],
                "ErrorManager.Syntax.MissingToken.exceptionVar.hint");
            return null;
        }
        var varName = curToken().value;
        var exception = new VariableNode(varName);
        next();

        if (!match(Delimeters.RightParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.closingParen.message", ["catch"],
                "ErrorManager.Syntax.MissingToken.closingParen.hint", ["catch"]);
            return null;
        }
        next();

        var catchBlock = parseBlock();
        if (catchBlock == null) {
            errorManager.addError(
                SyntaxErrors.InvalidStatement,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.InvalidStatement.expectedBody.message", ["catch"],
                "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", ["catch"]);
            return null;
        }

        var node = new TryCatchNode(tryBlock, exception, catchBlock);
        node.sourceSpan = token.sourceSpan;
        return node;
    }
    ForLoopNode? parseFor() {
        var token = curToken();
        next();

        if (!match(Delimeters.LeftParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(token.value, token.sourceSpan),
                "ErrorManager.Syntax.MissingToken.openingParen.message", ["for"],
                "ErrorManager.Syntax.MissingToken.openingParen.hint", ["for"]);
            return null;
        }
        next();

        if (curToken().type != TokenType.Identifier) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.noVariableAfter.message", ["for ("],
                "ErrorManager.Syntax.MissingToken.noVariableAfter.hint", ["for ("]);
            return null;
        }

        string varName = curToken().value;
        var varNode = new VariableNode(varName);
        next();

        if (!match(Delimeters.Colon)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.colonInFor.message", [varName],
                "ErrorManager.Syntax.MissingToken.colonInFor.hint");
            return null;
        }
        next();

        ASTNode? iterable = parseExpression();
        if (iterable == null) {
            errorManager.addError(
                SyntaxErrors.InvalidStatement,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.InvalidStatement.expectedIterable.message", [],
                "ErrorManager.Syntax.InvalidStatement.expectedIterable.hint");
            return null;
        }

        if (!match(Delimeters.RightParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.closingParen.message", ["for"],
                "ErrorManager.Syntax.MissingToken.closingParen.hint", ["for"]);
            return null;
        }
        next();

        var body = parseBlock();
        if (body == null) {
            errorManager.addError(
                SyntaxErrors.InvalidStatement,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.InvalidStatement.expectedBody.message", ["for"],
                "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", ["for"]);
            return null;
        }

        var node = new ForLoopNode(varNode, iterable, body);
        node.sourceSpan = token.sourceSpan;
        return node;
    }
    WhileLoopNode? parseWhile() {
        var token = curToken();
        next();

        if (!match(Delimeters.LeftParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(token.value, token.sourceSpan),
            "ErrorManager.Syntax.MissingToken.openingParen.message", ["while"],
            "ErrorManager.Syntax.MissingToken.openingParen.hint", ["while"]);
            return null;
        }
        next();
        ASTNode? condition = parseExpression();
        if (condition == null) {
            errorManager.addError(
                SyntaxErrors.InvalidStatement,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
            "ErrorManager.Syntax.InvalidStatement.expectedCondition.message", ["while"],
            "ErrorManager.Syntax.InvalidStatement.expectedCondition.hint", ["while"]);
            return null;
        }
        if (!match(Delimeters.RightParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
            "ErrorManager.Syntax.MissingToken.closingParen.message", ["while"],
            "ErrorManager.Syntax.MissingToken.closingParen.hint", ["while"]);
            return null;
        }
        next();

        var body = parseBlock();
        if (body == null) {
            errorManager.addError(
                SyntaxErrors.InvalidStatement,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
            "ErrorManager.Syntax.InvalidStatement.expectedBody.message", ["while"],
            "ErrorManager.Syntax.InvalidStatement.expectedBody.hint", ["while"]);
            return null;
        }

        var node = new WhileLoopNode(condition, body);
        node.sourceSpan = token.sourceSpan;
        return node;
    }

    // Declarations
    FunctionNode? parseFunction(List<CallExpressionNode> decorators, List<ModifierNode> modifiers) {
        next();
        Token nameToken = curToken();
        if (!match(TokenType.Identifier)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                "ErrorManager.Syntax.MissingToken.functionName.message", [],
                "ErrorManager.Syntax.MissingToken.functionName.hint");
            return null;
        }
        string funcName = nameToken.value;
        next();

        if (!match(Delimeters.LeftParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                "ErrorManager.Syntax.MissingToken.functionparameters.message", [funcName],
                "ErrorManager.Syntax.MissingToken.functionparameters.hint", [funcName]);
            return null;
        }
        next();

        List<ParameterNode> parameters = new();
        while (!match(Delimeters.RightParen)) {
            Token paramName = curToken();
            if (paramName.type != TokenType.Identifier) {
                errorManager.addError(
                    SyntaxErrors.MissingToken,
                    new ErrorSpan(paramName.value, paramName.sourceSpan),
                    "ErrorManager.Syntax.MissingToken.functionParamName.message", [funcName],
                    "ErrorManager.Syntax.MissingToken.functionParamName.hint");
                return null;
            }
            next();
            RawTypeNode? type = null;
            if (match(Delimeters.Colon)) {
                next();
                type = parseType();
            }

            ASTNode? defaultValue = null;
            if (match(Operators.Assign)) {
                next();
                defaultValue = parseExpression();
                if (defaultValue == null) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.functionParamDefault.message", [],
                        "ErrorManager.Syntax.MissingToken.functionParamDefault.hint");
                    return null;
                }
            }
            var param = new ParameterNode(paramName.value, type, defaultValue);
            param.sourceSpan = paramName.sourceSpan;
            parameters.Add(param);
            if (match(Delimeters.Comma)) next();
            else break;
        }

        if (!match(Delimeters.RightParen)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.functionClosingParen.message", [funcName],
                "ErrorManager.Syntax.MissingToken.functionClosingParen.hint");
            return null;
        }
        next();

        RawTypeNode? returnType = null;
        if (match(Operators.TypeArrow)) {
            next();
            returnType = parseType();
        }

        bool isIntrinsic = false;
        foreach (var modifier in modifiers) {
            if (modifier != null && modifier.modifier == ASTModifierType.Intrinsic) {
                isIntrinsic = true;
                break;
            }
        }

        BlockNode? body = null;

        if (isIntrinsic) {
            while (isNextLine()) next();

            if (match(Delimeters.RightBraces) || isAtEnd()) {} // не понял че тут
            else if (match(Delimeters.LeftBraces)) {
                errorManager.addError(SyntaxErrors.InvalidStatement,
                    new ErrorSpan(curToken().value, curToken().sourceSpan),
                    "ErrorManager.Syntax.InvalidStatement.intrinsicBody.message", [funcName],
                    "ErrorManager.Syntax.InvalidStatement.intrinsicBody.hint");
                return null;
            }
        }
        else {
            body = parseBlock();
            if (body == null) {
                errorManager.addError(
                    SyntaxErrors.InvalidStatement,
                    new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                    "ErrorManager.Syntax.MissingToken.functionBody.message", [funcName],
                    "ErrorManager.Syntax.MissingToken.functionBody.hint", [funcName]);
                return null;
            }
        }

        var node = new FunctionNode(funcName, parameters, returnType, body, decorators, modifiers);
        node.sourceSpan = nameToken.sourceSpan;
        node.isIntrinsic = isIntrinsic;
        return node;
    }
    ClassNode? parseClass(List<CallExpressionNode> decorators, List<ModifierNode> modifiers) {
        next();
        Token nameToken = curToken();
        if (!match(TokenType.Identifier)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                "ErrorManager.Syntax.MissingToken.className.message", [],
                "ErrorManager.Syntax.MissingToken.className.hint");
            return null;
        }
        string className = nameToken.value;
        next();

        // Checking if class is inherited
        VariableNode? super = null;
        if (match(Operators.InheritanceArrow)) {
            next();
            super = new VariableNode(curToken().value);
            next();
        }

        if (!match(Delimeters.LeftBraces)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                "ErrorManager.Syntax.MissingToken.classBody.message", [className],
                "ErrorManager.Syntax.MissingToken.classBody.hint");
            return null;
        }
        next();
        while (match(Delimeters.Semicolon)) next();

        List<DeclarationNode> fields = new();
        List<FunctionNode> methods = new();
        FunctionNode? constructor = null;

        while (!match(Delimeters.RightBraces)) {
            Token token = curToken();

            List<CallExpressionNode> decs = new();
            if (match(TokenType.Decorator)) decs = parseDecoratorCalls();
            var modifs = parseModifiers();

            if (match(TokenType.Identifier, className)) {
                var cSourceSpan = new SourceSpan(curToken().sourceSpan);
                constructor = parseConstructor(decs, modifs);
                if (constructor == null) {
                    errorManager.addError(SyntaxErrors.InvalidStatement,
                        new ErrorSpan(curToken().value, cSourceSpan),
                        "ErrorManager.Syntax.InvalidStatement.constructorFailed.message", [className],
                        "ErrorManager.Syntax.InvalidStatement.constructorFailed.hint");
                    return null;
                }
            }
            else if (match(Keywords.Function)) {
                var method = parseFunction(decs, modifs);
                if (method != null) methods.Add(method);
            }
            else if (match(token, TokenType.Identifier) && (match(lookupNext(), TokenType.Delimeter) && match(lookupNext(), Delimeters.Colon))) {
                DeclarationNode? decl = parseDeclaration(decs, modifs);
                fields.Add(decl);
                if (isNextLine()) next();
            }
            else if (isNextLine()) next();
            else {
                errorManager.addError(
                    SyntaxErrors.UnexpectedToken,
                    new ErrorSpan(token.value, token.sourceSpan),
                    "ErrorManager.Syntax.InvalidStatement.unexpectedInClass.message", [token.value],
                    "ErrorManager.Syntax.InvalidStatement.unexpectedInClass.hint");
                break;
            }
        }

        next();
        var node = new ClassNode(className, fields, methods, constructor, super, decorators, modifiers);
        node.sourceSpan = nameToken.sourceSpan;
        return node;
    }
    NamespaceNode? parseNamespace() {
        next();
        var token = curToken();

        var name = parsePrimary();
        if (name == null || (name!.type != ASTNodeType.Variable && name.type != ASTNodeType.MemberAccess)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
            "ErrorManager.Syntax.MissingToken.namespaceName.message", [],
            "ErrorManager.Syntax.MissingToken.namespaceName.hint"
                );
            return null;
        }

        string namespaceName = namespaceNameToString(name);

        var body = parseBlock();
        if (body == null) return null;

        var node = new NamespaceNode(name, namespaceName, body.statements);
        node.sourceSpan = token.sourceSpan;
        return node;
    }
    BlockNode? parseBlock() {
        if (!match(Delimeters.LeftBraces)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
            "ErrorManager.Syntax.MissingToken.openingBrace.message", ["block"],
            "ErrorManager.Syntax.MissingToken.openingBrace.hint");
            return null;
        }
        next();
        while (isNextLine()) next();

        List<ASTNode> block = new();

        while (!isAtEnd()) {
            while (isNextLine()) next();

            if (match(Delimeters.RightBraces)) {
                break;
            }

            ASTNode? stmt = parseStatement();
            if (stmt != null && match(Delimeters.RightBraces)) {
                break;
            }
            if (stmt == null) {

                while (!isAtEnd() && !isNextLine()) next();

                int startPos = pos;
                int guard = 0;
                while (!isAtEnd() && guard++ < 100){
                    if (isNextLine()){ next(); break; }
                    if (match(Delimeters.RightBraces)){ break; }
                    next();
                }

                if (pos == startPos && !isAtEnd()) next();

                if (guard >= 100) {
                    break;
                }

                if (isAtEnd()) break;
                continue;
            }
            block.Add(stmt);
            while (isNextLine()) next();
        }
        if (!match(Delimeters.RightBraces)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
            "ErrorManager.Syntax.MissingToken.closingBrace.message", ["block"],
            "ErrorManager.Syntax.MissingToken.closingBrace.hint");
            return null;
        }
        next();
        return new BlockNode(block);
    }

    EnumNode? parseEnum(List<CallExpressionNode> decorators, List<ModifierNode> modifiers) {
        next();
        if (!match(TokenType.Identifier)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.enumName.message", [],
                "ErrorManager.Syntax.MissingToken.enumName.hint");
            return null;
        }
        var enumToken = curToken();
        next();
        if (!match(Delimeters.LeftBraces)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(enumToken.value, enumToken.sourceSpan),
                "ErrorManager.Syntax.MissingToken.enumBody.message", [enumToken.value],
                "ErrorManager.Syntax.MissingToken.enumBody.hint");
            return null;
        }
        next();
        while (isNextLine()) next();

        List<EnumMemberNode> elements = new();

        while (!match(Delimeters.RightBraces)) {
            while (isNextLine()) next();
            if (match(Delimeters.RightBraces)) break;

            if (!match(TokenType.Identifier)) {
                errorManager.addError(
                    SyntaxErrors.UnexpectedToken,
                    new ErrorSpan(curToken().value, curToken().sourceSpan),
                    "ErrorManager.Syntax.InvalidStatement.unexpectedInEnum.message", [curToken().value],
                    "ErrorManager.Syntax.InvalidStatement.unexpectedInEnum.hint");
                return null;
            }
            var name = curToken().value;
            LiteralNode? value = null;

            next();
            if (match(Operators.Assign)) {
                next();
                var tmp = parsePrimary();
                if (tmp is not LiteralNode literal) {
                    errorManager.addError(
                    SyntaxErrors.InvalidStatement,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.InvalidStatement.enumNonLiteralValue.message", [name],
                        "ErrorManager.Syntax.InvalidStatement.enumNonLiteralValue.hint");
                    return null;
                }
                value = literal;
            }
            elements.Add(new EnumMemberNode(name, value));

            if (match(Delimeters.Comma)) next();
            else if (isNextLine()) next();
            else if (!match(Delimeters.RightBraces)) {
                errorManager.addError(
                    SyntaxErrors.UnexpectedToken,
                    new ErrorSpan(curToken().value, curToken().sourceSpan),
                    "ErrorManager.Syntax.InvalidStatement.enumDelimiter.message", [curToken().value],
                    "ErrorManager.Syntax.InvalidStatement.enumDelimiter.hint");
                return null;
            }
            else break;
        }
        if (!match(Delimeters.RightBraces)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.enumClosingBrace.message", [enumToken.value],
                "ErrorManager.Syntax.MissingToken.enumClosingBrace.hint");
            return null;
        }
        next();

        var node = new EnumNode(enumToken.value, elements, decorators, modifiers);
        node.sourceSpan = enumToken.sourceSpan;
        return node;
    }
    InterfaceNode? parseInterface(List<CallExpressionNode> decorators, List<ModifierNode> modifiers) {
        next();
        if (!match(TokenType.Identifier)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.interfaceName.message", [],
                "ErrorManager.Syntax.MissingToken.interfaceName.hint");
            return null;
        }
        var interfaceToken = curToken();
        next();
        if (!match(Delimeters.LeftBraces)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(interfaceToken.value, interfaceToken.sourceSpan),
                "ErrorManager.Syntax.MissingToken.interfaceBody.message", [interfaceToken.value],
                "ErrorManager.Syntax.MissingToken.interfaceBody.hint");
            return null;
        }
        next();
        while (isNextLine()) next();

        List<InterfaceFieldNode> elements = new();
        List<ParameterNode> parameters = new();

        while (!match(Delimeters.RightBraces)) {
            if (curToken().type == TokenType.Identifier) {
                var name = curToken().value;
                bool isNullable = false;
                next();
                if (match(Operators.Nullable)) {
                    isNullable = true;
                    next();
                }

                if (!match(Delimeters.Colon)) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.colonInInterface.message", [name],
                        "ErrorManager.Syntax.MissingToken.colonInInterface.hint");
                    return null;
                }
                next();

                RawTypeNode? rawType = parseType();
                elements.Add(new InterfaceFieldNode(name, isNullable, false, rawType));

                if (isNextLine()) next();
                else if (!match(Delimeters.RightBraces)) {
                    errorManager.addError(
                        SyntaxErrors.UnexpectedToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.InvalidStatement.interfaceDelimiter.message", [curToken().value],
                        "ErrorManager.Syntax.InvalidStatement.interfaceDelimiter.hint");
                    return null;
                }
                else break;
            } else if (match(Keywords.Function)) {
                next();
                if (curToken().type != TokenType.Identifier) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.interfaceMethodName.message", [interfaceToken.value],
                        "ErrorManager.Syntax.MissingToken.interfaceMethodName.hint");
                    return null;
                }
                string methodName = curToken().value;
                next();
                if (!match(Delimeters.LeftParen)) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.interfaceMethodparameters.message", [methodName],
                        "ErrorManager.Syntax.MissingToken.interfaceMethodparameters.hint");
                    return null;
                }
                next();
                while (!match(Delimeters.RightParen)) {
                    Token token = curToken();
                    if (!match(TokenType.Identifier)) {
                        errorManager.addError(
                            SyntaxErrors.MissingToken,
                            new ErrorSpan(token.value, token.sourceSpan),
                            "ErrorManager.Syntax.MissingToken.interfaceMethodParamName.message", [methodName],
                            "ErrorManager.Syntax.MissingToken.interfaceMethodParamName.hint");
                        return null;
                    }
                    string paramName = token.value;
                    next();
                    RawTypeNode? type = null;
                    if (match(Delimeters.Colon)) {
                        next();
                        type = parseType();
                    }
                    parameters.Add(new ParameterNode(paramName, type, null));
                    if (match(Delimeters.Comma)) next();
                    else break;
                }
                if (!match(Delimeters.RightParen)) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.interfaceMethodClosingParen.message", [methodName],
                        "ErrorManager.Syntax.MissingToken.interfaceMethodClosingParen.hint");
                    return null;
                }
                next();
                VariableNode? returnType = null;
                if (match(Operators.TypeArrow)) {
                    next();
                    if (curToken().type != TokenType.Identifier) {
                        errorManager.addError(
                            SyntaxErrors.MissingToken,
                            new ErrorSpan(curToken().value, curToken().sourceSpan),
                            "ErrorManager.Syntax.MissingToken.interfaceReturnType.message", [methodName],
                            "ErrorManager.Syntax.MissingToken.interfaceReturnType.hint");
                        return null;
                    }
                    returnType = new VariableNode(curToken().value);
                    next();
                }
                elements.Add(new InterfaceFieldNode(methodName, false, true, null, parameters, returnType));
                parameters.Clear();
                if (isNextLine()) next();
            } else {
                errorManager.addError(
                    SyntaxErrors.UnexpectedToken,
                    new ErrorSpan(curToken().value, curToken().sourceSpan),
                    "ErrorManager.Syntax.InvalidStatement.unexpectedInInterface.message", [curToken().value],
                    "ErrorManager.Syntax.InvalidStatement.unexpectedInInterface.hint");
                return null;
            }
        }
        if (!match(Delimeters.RightBraces)) {
            errorManager.addError(
                SyntaxErrors.MissingToken,
                new ErrorSpan(curToken().value, curToken().sourceSpan),
                "ErrorManager.Syntax.MissingToken.interfaceClosingBrace.message", [interfaceToken.value],
                "ErrorManager.Syntax.MissingToken.interfaceClosingBrace.hint");
            return null;
        }
        next();

        var node = new InterfaceNode(interfaceToken.value, elements, decorators, modifiers);
        node.sourceSpan = interfaceToken.sourceSpan;
        return node;
    }

    // Imports, decorators, modifiers, preprocessor
    ASTNode? parseDecorator(List<CallExpressionNode>? decorators = null, List<ModifierNode>? modifiers = null, bool isCall = false) {
        decorators ??= []; modifiers ??= [];
        Token nameToken = curToken();
        ASTNode node;

        if (!isCall) {
            next();
            if (!match(TokenType.Identifier)) {
                errorManager.addError(
                    SyntaxErrors.MissingToken,
                    new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                    "ErrorManager.Syntax.MissingToken.decoratorName.message", [],
                    "ErrorManager.Syntax.MissingToken.decoratorName.hint");
                return null;
            }
            string name = curToken().value;
            next();

            List<ParameterNode> parameters = new();
            if (match(Delimeters.LeftParen)) {
                next();
                while (!match(Delimeters.RightParen)) {
                    if (!match(TokenType.Identifier)) {
                        errorManager.addError(
                            SyntaxErrors.MissingToken,
                            new ErrorSpan(curToken().value, curToken().sourceSpan),
                            "ErrorManager.Syntax.MissingToken.decoratorParamName.message", [name],
                            "ErrorManager.Syntax.MissingToken.decoratorParamName.hint");
                        return null;
                    }

                    Token tok = curToken();
                    string paramName = tok.value;
                    next();

                    RawTypeNode? type = null;
                    if (match(Delimeters.Colon)) {
                        next();
                        type = parseType();
                    }

                    ASTNode? defaultValue = null;
                    if (match(Operators.Assign)) {
                        next();
                        defaultValue = parseExpression();
                        if (defaultValue == null) {
                            errorManager.addError(
                                SyntaxErrors.MissingToken,
                                new ErrorSpan(curToken().value, curToken().sourceSpan),
                                "ErrorManager.Syntax.MissingToken.decoratorParamDefault.message", [],
                                "ErrorManager.Syntax.MissingToken.decoratorParamDefault.hint");
                            return null;
                        }
                    }

                    parameters.Add(new ParameterNode(paramName, type, defaultValue));
                    if (match(Delimeters.Comma)) next();
                    else break;
                }

                if (!match(Delimeters.RightParen)) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.decoratorClosingParen.message", [name],
                        "ErrorManager.Syntax.MissingToken.decoratorClosingParen.hint");
                    return null;
                }
                next();
            }

            var block = parseBlock();
            if (block == null) {
                errorManager.addError(
                    SyntaxErrors.InvalidStatement,
                    new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                    "ErrorManager.Syntax.MissingToken.decoratorBody.message", [name],
                    "ErrorManager.Syntax.MissingToken.decoratorBody.hint");
                return null;
            }
            node = new DecoratorNode(name, parameters, block, decorators, modifiers);
            node.sourceSpan = nameToken.sourceSpan;
        } else {
            string name = nameToken.value;
            next();

            List<ASTNode> args = new();
            if (match(Delimeters.LeftParen)) {
                next();
                while (!match(Delimeters.RightParen)) {
                    var arg = parseExpression();
                    if (arg != null) args.Add(arg);

                    if (match(Delimeters.Comma)) next();
                    else break;
                }
                if (!match(Delimeters.RightParen)) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(nameToken.value, nameToken.sourceSpan),
                        "ErrorManager.Syntax.MissingToken.decoratorClosingParen.message", [name],
                        "ErrorManager.Syntax.MissingToken.decoratorClosingParen.hint");
                    return null;
                }
                next();
            }
            node = new CallExpressionNode(new VariableNode(name), args, true);
            node.sourceSpan = nameToken.sourceSpan;
        }
        return node;
    }
    List<CallExpressionNode> parseDecoratorCalls() {
        List<CallExpressionNode> calls = new();
        while (match(TokenType.Decorator)) {
            var node = parseDecorator([], [], true);
            if (node == null) break; // error already reported by Lexer.parseDecorator
            if (node is not CallExpressionNode call) {
                // [internal]
                break;
            }
            calls.Add(call);
        }

        return calls;
    }
    List<ModifierNode> parseModifiers() {
        List<ModifierNode> modifiers = new();

        while (curToken().type == TokenType.Keyword) {
            if (match(Keywords.Static))
                modifiers.Add(new ModifierNode(ASTModifierType.Static));
            else if (match(Keywords.Const))
                modifiers.Add(new ModifierNode(ASTModifierType.Const));
            else if (match(Keywords.Public))
                modifiers.Add(new ModifierNode(ASTModifierType.Public));
            else if (match(Keywords.Protected))
                modifiers.Add(new ModifierNode(ASTModifierType.Protected));
            else if (match(Keywords.Private))
                modifiers.Add(new ModifierNode(ASTModifierType.Private));
            else if (match(Keywords.Override))
                modifiers.Add(new ModifierNode(ASTModifierType.Override));
            else if (match(Keywords.Async))
                modifiers.Add(new ModifierNode(ASTModifierType.Async));
            else if (match(Keywords.Debug))
                modifiers.Add(new ModifierNode(ASTModifierType.Debug));
            else if (match(Keywords.Intrinsic))
                modifiers.Add(new ModifierNode(ASTModifierType.Intrinsic));
            else break;
            next();
        }

        return modifiers;
    }
    ASTNode? parsePreprocessor() {
        Token token = curToken();
        ASTNode node;

        if (match(Preprocessors.Import)) {
            next();
            if (!match(TokenType.String)) {
                errorManager.addError(
                    SyntaxErrors.MissingToken,
                    new ErrorSpan(token.value, token.sourceSpan),
                    "ErrorManager.Syntax.MissingToken.importTarget.message", [],
                    "ErrorManager.Syntax.MissingToken.importTarget.hint");
                return null;
            }
            var moduleStr = curToken().value;
            ASTImportType importType = ASTImportType.Native;
            if ((moduleStr.Contains("/") || moduleStr.Contains(".")) && moduleStr.Contains(":")) importType = ASTImportType.ForeignRelative;
            else if (moduleStr.Contains("/") || moduleStr.Contains(".")) importType = ASTImportType.Relative;
            else if (moduleStr.Contains(":")) importType = ASTImportType.Foreign;

            string alias = String.Empty;
            next();
            if (match(Keywords.As)) {
                next();
                if (!match(TokenType.Identifier)) {
                    errorManager.addError(
                        SyntaxErrors.MissingToken,
                        new ErrorSpan(curToken().value, curToken().sourceSpan),
                        "ErrorManager.Syntax.MissingToken.importAlias.message", [],
                        "ErrorManager.Syntax.MissingToken.importAlias.hint");
                    return null;
                }
                alias = curToken().value;
                next();
            }
            node = new ImportNode(moduleStr, alias, importType);
        }
        else if (match(Preprocessors.Macro)) {
            next();
            if (!match(TokenType.Identifier)) {
                errorManager.addError(
                    SyntaxErrors.MissingToken,
                    new ErrorSpan(token.value, token.sourceSpan),
                    "ErrorManager.Syntax.MissingToken.macroIdentifier.message", [],
                    "ErrorManager.Syntax.MissingToken.macroIdentifier.hint");
                return null;
            }
            next();
            string value = String.Empty;
            while (!isNextLine()) {
                value += curToken().value;
                next();
            }
            node = new PreprocessorDirectiveNode(ASTPreprocessorDirectiveType.Macro, value);
        }
        else if (match(Preprocessors.Unsafe)) {
            next();
            node = new PreprocessorDirectiveNode(ASTPreprocessorDirectiveType.Unsafe);
        }
        else node = new PreprocessorDirectiveNode(ASTPreprocessorDirectiveType.None);
        node.sourceSpan = token.sourceSpan;
        return node;
    }
}