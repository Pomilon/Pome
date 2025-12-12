#include "../include/pome_parser.h"
#include <iostream>
#include <map>

namespace Pome
{

    /**
     * Helper function to map TokenType to its precedence
     */
    static std::map<TokenType, Parser::Precedence> precedences = {
        {TokenType::EQ, Parser::EQUALS},
        {TokenType::NE, Parser::EQUALS},
        {TokenType::LT, Parser::LESSGREATER},
        {TokenType::LE, Parser::LESSGREATER},
        {TokenType::GT, Parser::LESSGREATER},
        {TokenType::GE, Parser::LESSGREATER},
        {TokenType::PLUS, Parser::SUM},
        {TokenType::MINUS, Parser::SUM},
        {TokenType::MULTIPLY, Parser::PRODUCT},
        {TokenType::DIVIDE, Parser::PRODUCT},
        {TokenType::MODULO, Parser::PRODUCT},
        {TokenType::CARET, Parser::EXPONENT},         // Added for exponentiation
        {TokenType::DOT, Parser::MEMBER_ACCESS},      // Added for member access
        {TokenType::LBRACKET, Parser::MEMBER_ACCESS}, // Added for index access (same precedence as dot)
        {TokenType::QUESTION, Parser::TERNARY},       // Added for ternary operator
        {TokenType::AND, Parser::LOGICAL_AND},
        {TokenType::OR, Parser::LOGICAL_OR},
        /**
         * ASSIGN is not here, as assignment will be handled as a statement or specific expression context
         */
        {TokenType::LPAREN, Parser::CALL}, // For function calls
    };

    Parser::Parser(Lexer &lexer) : lexer_(lexer)
    {
        nextToken(); // Initialize currentToken_
        nextToken(); // Initialize peekToken_
    }

    void Parser::nextToken()
    {
        currentToken_ = peekToken_;
        peekToken_ = lexer_.getNextToken();
    }

    bool Parser::expect(TokenType type)
    {
        if (peekToken_.type == type)
        {
            nextToken();
            return true;
        }
        error("Expected " + Token::toString(type) + ", got " + peekToken_.debugString());
        return false;
    }

    bool Parser::match(TokenType type)
    {
        if (currentToken_.type == type)
        {
            nextToken();
            return true;
        }
        return false;
    }

    void Parser::error(const std::string &message)
    {
        throw std::runtime_error("Parsing error at line " + std::to_string(currentToken_.line) +
                                 ", column " + std::to_string(currentToken_.column) + ": " + message);
    }

    Parser::Precedence Parser::getPrecedence(TokenType type)
    {
        auto it = precedences.find(type);
        if (it != precedences.end())
        {
            return it->second;
        }
        return LOWEST;
    }

    /**
     * Top-level parsing function
     */
    std::unique_ptr<Program> Parser::parseProgram()
    {
        auto program = std::make_unique<Program>();
        while (currentToken_.type != TokenType::END_OF_FILE)
        {
            auto stmt = parseStatement();
            if (stmt)
            {
                program->addStatement(std::move(stmt));
            }
            else
            {
                /**
                 * Skip token if statement parsing failed to avoid infinite loop
                 * In a real interpreter, more sophisticated error recovery is needed
                 */
                nextToken();
            }
        }
        return program;
    }

    /**
     * --- Expression Parsing ---
     */

    /**
     * Overload without precedence argument, calls with LOWEST
     */
    std::unique_ptr<Expression> Parser::parseExpression()
    {
        return parseExpression(LOWEST);
    }

    std::unique_ptr<Expression> Parser::parseExpression(Precedence precedence)
    {
        auto leftExpr = parsePrefixExpression();
        if (!leftExpr)
            return nullptr;

        while (getPrecedence(peekToken_.type) > precedence)
        {
            nextToken(); // Advance to the infix operator

            if (currentToken_.type == TokenType::LPAREN)
            { // Function call
                leftExpr = parseCallExpression(std::move(leftExpr));
            }
            else if (currentToken_.type == TokenType::DOT)
            { // Member access
                leftExpr = parseMemberAccessExpression(std::move(leftExpr));
            }
            else if (currentToken_.type == TokenType::LBRACKET)
            { // Index access
                leftExpr = parseIndexExpression(std::move(leftExpr));
            }
            else if (currentToken_.type == TokenType::QUESTION)
            { // Ternary operator
                leftExpr = parseTernaryExpression(std::move(leftExpr));
            }
            else
            { // Binary operator
                leftExpr = parseInfixExpression(std::move(leftExpr));
            }
            if (!leftExpr)
                return nullptr;
        }
        return leftExpr;
    }

    std::unique_ptr<Expression> Parser::parsePrimaryExpression()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;

        switch (currentToken_.type)
        {
        case TokenType::IDENTIFIER:
        {
            auto identifier = std::make_unique<IdentifierExpr>(currentToken_.value, line, col);

            return identifier;
        }
        case TokenType::THIS:
        {
            return std::make_unique<ThisExpr>(line, col);
        }
        case TokenType::NUMBER:
        {
            try
            {
                auto number = std::make_unique<NumberExpr>(std::stod(currentToken_.value), line, col);

                return number;
            }
            catch (const std::exception &e)
            {
                error("Invalid number literal: " + currentToken_.value);
                return nullptr;
            }
        }
        case TokenType::STRING:
        {
            auto str = std::make_unique<StringExpr>(currentToken_.value, line, col);

            return str;
        }
        case TokenType::TRUE:
        {
            return std::make_unique<BooleanExpr>(true, line, col);
        }
        case TokenType::FALSE:
        {
            return std::make_unique<BooleanExpr>(false, line, col);
        }
        case TokenType::NIL:
        {
            return std::make_unique<NilExpr>(line, col);
        }
        case TokenType::LPAREN:
        {
            /**
             * currentToken_ is LPAREN, advance past it
             */
            nextToken();                   // consume '('
            auto expr = parseExpression(); // Calls the overload without precedence, which defaults to LOWEST
            if (!expr)
                return nullptr;
            if (!expect(TokenType::RPAREN))
                return nullptr; // expect consumes RPAREN
            return expr;
        }
        case TokenType::LBRACKET:
        {
            return parseListLiteral();
        }
        case TokenType::LBRACE:
        {
            return parseTableLiteral();
        }
        case TokenType::FUNCTION:
        {
            return parseFunctionExpression();
        }
        default:
            error("Unexpected token in expression: " + currentToken_.debugString());
            return nullptr;
        }
    }

    std::unique_ptr<Expression> Parser::parseCallExpression(std::unique_ptr<Expression> callee)
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        std::vector<std::unique_ptr<Expression>> args;

        if (peekToken_.type != TokenType::RPAREN)
        { // Check if there are arguments
            do
            {
                nextToken();                  // Move past '(' or ','
                auto arg = parseExpression(); // Calls the overload without precedence, which defaults to LOWEST
                if (!arg)
                    return nullptr;
                args.push_back(std::move(arg));
            } while (peekToken_.type == TokenType::COMMA && (nextToken(), true)); // Consume comma
        }
        if (!expect(TokenType::RPAREN))
            return nullptr;
        return std::make_unique<CallExpr>(std::move(callee), std::move(args), line, col);
    }

    std::unique_ptr<Expression> Parser::parsePrefixExpression()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * Check for unary operators
         */
        if (currentToken_.type == TokenType::MINUS || currentToken_.type == TokenType::NOT)
        {
            std::string op = currentToken_.value;
            nextToken();                          // Consume unary operator
            auto right = parseExpression(PREFIX); // Unary operators typically have higher precedence
            if (!right)
                return nullptr;
            return std::make_unique<UnaryExpr>(op, std::move(right), line, col);
        }
        /**
         * If not a prefix operator, parse as a primary expression
         */
        auto primaryExpr = parsePrimaryExpression();
        return primaryExpr;
    }

    std::unique_ptr<Expression> Parser::parseInfixExpression(std::unique_ptr<Expression> left)
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        TokenType operatorType = currentToken_.type;
        std::string op = currentToken_.value;
        Precedence precedence = getPrecedence(operatorType);
        nextToken(); // Consume operator

        auto right = parseExpression(precedence); // Recursive call with current operator's precedence
        if (!right)
            return nullptr;

        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right), line, col);
    }

    std::unique_ptr<Expression> Parser::parseMemberAccessExpression(std::unique_ptr<Expression> object)
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is DOT. Need to advance past it.
         */
        nextToken(); // Consume DOT. currentToken_ is now IDENTIFIER (member name)

        if (currentToken_.type != TokenType::IDENTIFIER)
        {
            error("Expected identifier after '.' operator, got " + currentToken_.debugString());
            return nullptr;
        }
        std::string memberName = currentToken_.value;
        /**
         * currentToken_ is now the member name.
         */

        /**
         * We don't call nextToken here, as the main parseExpression loop will handle it.
         */
        return std::make_unique<MemberAccessExpr>(std::move(object), memberName, line, col);
    }

    std::unique_ptr<Expression> Parser::parseIndexExpression(std::unique_ptr<Expression> object)
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is LBRACKET.
         */
        nextToken(); // Consume LBRACKET. currentToken_ is now start of index expression

        std::unique_ptr<Expression> start = nullptr;
        std::unique_ptr<Expression> end = nullptr;
        bool isSlice = false;

        // Check for [:end]
        if (currentToken_.type == TokenType::COLON)
        {
            isSlice = true;
            // currentToken_ is COLON. peekToken_ is start of end expr OR RBRACKET.
            if (peekToken_.type != TokenType::RBRACKET)
            {
                nextToken(); // Move to start of end expr
                end = parseExpression();
                if (!end)
                    return nullptr;
            }
            else
            {
                // buh
            }
        }
        else
        {
            start = parseExpression();
            if (!start)
                return nullptr;

            // Check if next token is COLON -> [start:end]
            if (peekToken_.type == TokenType::COLON)
            {
                isSlice = true;
                nextToken(); // Move to COLON (currentToken_ was last token of start)
                // Now currentToken_ is COLON. peekToken_ is start of end expr or RBRACKET.

                if (peekToken_.type != TokenType::RBRACKET)
                {
                    nextToken(); // Move to start of end expr
                    end = parseExpression();
                    if (!end)
                        return nullptr;
                }
            }
        }

        if (!expect(TokenType::RBRACKET))
            return nullptr; // expect consumes RBRACKET

        if (isSlice)
        {
            return std::make_unique<SliceExpr>(std::move(object), std::move(start), std::move(end), line, col);
        }
        else
        {
            return std::make_unique<IndexExpr>(std::move(object), std::move(start), line, col);
        }
    }

    std::unique_ptr<Expression> Parser::parseTernaryExpression(std::unique_ptr<Expression> condition)
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is QUESTION.
         */
        nextToken(); // Consume '?'

        /**
         * Parse then expression.
         */
        auto thenExpr = parseExpression(LOWEST);
        if (!thenExpr)
            return nullptr;

        if (!expect(TokenType::COLON))
            return nullptr; // expect consumes COLON
        nextToken();        // Consume COLON so currentToken_ becomes start of elseExpr

        /**
         * Parse else expression.
         * Use LOWEST to allow right-associativity and assignment in the else branch.
         */
        auto elseExpr = parseExpression(LOWEST);
        if (!elseExpr)
            return nullptr;

        return std::make_unique<TernaryExpr>(std::move(condition), std::move(thenExpr), std::move(elseExpr), line, col);
    }

    std::unique_ptr<Expression> Parser::parseFunctionExpression()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;

        // currentToken_ is FUNCTION
        nextToken(); // Consume FUNCTION

        std::string funcName = "";
        if (currentToken_.type == TokenType::IDENTIFIER)
        {
            funcName = currentToken_.value;
            nextToken(); // Consume name
        }

        if (currentToken_.type != TokenType::LPAREN)
        {
            error("Expected '(' after function keyword/name, got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume '('

        std::vector<std::string> params;
        if (currentToken_.type != TokenType::RPAREN)
        {
            do
            {
                if (currentToken_.type != TokenType::IDENTIFIER)
                {
                    error("Expected parameter name, got " + currentToken_.debugString());
                    return nullptr;
                }
                params.push_back(currentToken_.value);
                nextToken(); // Consume parameter name
            } while (currentToken_.type == TokenType::COMMA && (nextToken(), true));
        }

        if (currentToken_.type != TokenType::RPAREN)
        {
            error("Expected ')' after parameters, got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume ')'

        if (currentToken_.type != TokenType::LBRACE)
        {
            error("Expected '{' for function body, got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume '{'

        std::vector<std::unique_ptr<Statement>> body = parseBlockStatement();

        if (currentToken_.type != TokenType::RBRACE)
        {
            error("Expected '}' after function body, got " + currentToken_.debugString());
            return nullptr;
        }

        return std::make_unique<FunctionExpr>(funcName, std::move(params), std::move(body), line, col);
    }

    std::unique_ptr<Expression> Parser::parseListLiteral()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        std::vector<std::unique_ptr<Expression>> elements;
        nextToken(); // Consume LBRACKET. currentToken_ is now first element start or RBRACKET.

        if (currentToken_.type != TokenType::RBRACKET) // If the list is NOT empty
        {
            do
            {
                auto element = parseExpression();
                if (!element)
                    return nullptr;
                elements.push_back(std::move(element));

            } while (peekToken_.type == TokenType::COMMA && (nextToken(), nextToken(), true)); // Consume currentToken (element) and then COMMA
            // After this loop, currentToken_ is the last element, peekToken_ is RBRACKET
            nextToken(); // Consume the last element. Now currentToken_ is RBRACKET.
        }

        // At this point, currentToken_ should be RBRACKET (either because list was empty, or consumed after loop)
        if (currentToken_.type != TokenType::RBRACKET)
        {
            error("Expected RBRACKET after list, got " + currentToken_.debugString());
            return nullptr;
        }
        // DO NOT Consume RBRACKET.

        return std::make_unique<ListExpr>(std::move(elements), line, col);
    }

    std::unique_ptr<Expression> Parser::parseTableLiteral()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        std::vector<TableExpr::Entry> entries;
        nextToken(); // Consume LBRACE. currentToken_ is now token after LBRACE.

        if (currentToken_.type != TokenType::RBRACE) // If the table is NOT empty
        {
            do
            {
                // Parse Key
                std::unique_ptr<Expression> key;
                if (currentToken_.type == TokenType::IDENTIFIER || currentToken_.type == TokenType::STRING || currentToken_.type == TokenType::NUMBER)
                {
                    if (currentToken_.type == TokenType::STRING)
                    {
                        key = std::make_unique<StringExpr>(currentToken_.value, currentToken_.line, currentToken_.column);
                    }
                    else if (currentToken_.type == TokenType::NUMBER)
                    {
                        key = std::make_unique<NumberExpr>(std::stod(currentToken_.value), currentToken_.line, currentToken_.column);
                    }
                    else
                    { // IDENTIFIER
                        key = std::make_unique<StringExpr>(currentToken_.value, currentToken_.line, currentToken_.column);
                    }
                    nextToken(); // Consume the key token. currentToken_ is now COLON, peekToken_ is value.
                }
                else
                {
                    error("Expected identifier, string, or number as table key, got " + currentToken_.debugString());
                    return nullptr;
                }

                if (currentToken_.type != TokenType::COLON)
                { // Manually check for COLON in currentToken_
                    error("Expected COLON after table key, got " + currentToken_.debugString());
                    return nullptr;
                }
                nextToken(); // Consume COLON. currentToken_ is COLON, peekToken_ is value.

                auto value = parseExpression(); // Parses value. currentToken_ becomes value's last token, peekToken_ becomes next token (COMMA or RBRACE).
                if (!value)
                    return nullptr;

                entries.push_back({std::move(key), std::move(value)});

            } while (peekToken_.type == TokenType::COMMA && (nextToken(), nextToken(), true)); // Consume currentToken (value) and then COMMA

            // After this loop, currentToken_ is the last value, peekToken_ is RBRACE.
            nextToken(); // Consume the last value. Now currentToken_ is RBRACE.
        }

        // At this point, currentToken_ should be RBRACE (either because table was empty, or consumed after loop)
        if (currentToken_.type != TokenType::RBRACE)
        {
            error("Expected RBRACE after table, got " + currentToken_.debugString());
            return nullptr;
        }
        // DO NOT Consume RBRACE.

        return std::make_unique<TableExpr>(std::move(entries), line, col);
    }

    /**
     * --- Statement Parsing ---
     */

    std::unique_ptr<Statement> Parser::parseStatement()
    {
        switch (currentToken_.type)
        {
        case TokenType::VAR:
            return parseVarDeclarationStatement();
        case TokenType::IF:
            return parseIfStatement();
        case TokenType::WHILE:
            return parseWhileStatement();
        case TokenType::FOR:
            return parseForStatement();
        case TokenType::RETURN:
            return parseReturnStatement();
        case TokenType::FUNCTION: // Handle function declarations
            return parseFunctionDeclaration();
        case TokenType::CLASS: // Handle class declarations
            return parseClassDeclaration();
        case TokenType::IMPORT: // Handle import statements
            return parseImportStatement();
        case TokenType::FROM: // Handle from ... import statements
            return parseFromImportStatement();
        case TokenType::EXPORT: // Handle export statements
            return parseExportStatement();
        default:
            /**
             * Could be an expression statement or an assignment
             */
            return parseExpressionStatement();
        }
    }

    std::unique_ptr<Statement> Parser::parseVarDeclarationStatement()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;

        nextToken(); // Consume 'VAR', currentToken_ is now IDENTIFIER (e.g., 'x' or 'z')

        if (currentToken_.type != TokenType::IDENTIFIER)
        {
            error("Expected identifier after 'var' keyword, got " + currentToken_.debugString());
            return nullptr;
        }
        std::string varName = currentToken_.value;
        nextToken(); // Consume IDENTIFIER ('x' or 'z'). currentToken_ is now ASSIGN ('=') or SEMICOLON (';')

        std::unique_ptr<Expression> initializer = nullptr;
        if (currentToken_.type == TokenType::ASSIGN)
        {                                    // Check if there's an initializer (e.g., '= 10')
            nextToken();                     // Consume 'ASSIGN', currentToken_ is now start of expression (e.g., NUMBER '10')
            initializer = parseExpression(); // This will parse '10'. After this, currentToken_ is '10'
            if (!initializer)
                return nullptr;
        }

        if (currentToken_.type != TokenType::SEMICOLON)
        {
            if (!expect(TokenType::SEMICOLON))
                return nullptr;
        }
        nextToken(); // Consume SEMICOLON

        return std::make_unique<VarDeclStmt>(varName, std::move(initializer), line, col);
    }

    std::unique_ptr<Statement> Parser::parseAssignmentStatement(std::unique_ptr<Expression> target)
    {
        int line = target->getLine();
        int col = target->getColumn();

        nextToken(); // Consume target. currentToken_ is now ASSIGN.
        nextToken(); // Consume ASSIGN. currentToken_ is now start of RHS.

        auto value = parseExpression(LOWEST);
        if (!value)
            return nullptr;
        if (!expect(TokenType::SEMICOLON))
            return nullptr;
        nextToken(); // Consume SEMICOLON

        return std::make_unique<AssignStmt>(std::move(target), std::move(value), line, col);
    }

    std::unique_ptr<Statement> Parser::parseIfStatement()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is IF
         */
        nextToken(); // Consume 'IF', currentToken_ is now LPAREN ('(')

        if (currentToken_.type != TokenType::LPAREN)
        {
            error("Expected '(' after 'if', got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume 'LPAREN'

        auto condition = parseExpression();
        if (!condition)
            return nullptr;

        if (!expect(TokenType::RPAREN))
            return nullptr;

        std::vector<std::unique_ptr<Statement>> thenBranch;
        if (peekToken_.type == TokenType::LBRACE)
        {
            nextToken(); // Consume LBRACE (peekToken -> currentToken)
            // currentToken_ is LBRACE
            nextToken(); // Move to first token of block
            thenBranch = parseBlockStatement();

            if (currentToken_.type != TokenType::RBRACE)
            {
                error("Expected RBRACE after 'then' block, got " + currentToken_.debugString());
                return nullptr;
            }
            nextToken(); // Consume RBRACE
        }
        else
        {
            nextToken(); // Move past RPAREN to start of statement
            auto stmt = parseStatement();
            if (!stmt)
                return nullptr;
            thenBranch.push_back(std::move(stmt));
        }

        std::vector<std::unique_ptr<Statement>> elseBranch;
        if (currentToken_.type == TokenType::ELSE)
        {
            nextToken(); // Consume ELSE

            if (currentToken_.type == TokenType::IF) // Handle 'else if'
            {
                // If it's 'else if', parse another IfStatement as the else branch
                auto nestedIf = parseIfStatement(); // Recursively parse the 'if' part
                if (!nestedIf)
                    return nullptr;
                elseBranch.push_back(std::move(nestedIf)); // Add the nested if as the else branch
            }
            else if (currentToken_.type == TokenType::LBRACE) // Regular 'else' block
            {
                nextToken(); // Consume LBRACE
                elseBranch = parseBlockStatement();
                if (currentToken_.type != TokenType::RBRACE)
                {
                    error("Expected RBRACE after 'else' block, got " + currentToken_.debugString());
                    return nullptr;
                }
                nextToken(); // Consume RBRACE
            }
            else // Single statement else
            {
                auto stmt = parseStatement();
                if (!stmt)
                    return nullptr;
                elseBranch.push_back(std::move(stmt));
            }
        }

        return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), line, col);
    }

    std::unique_ptr<Statement> Parser::parseWhileStatement()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is WHILE
         */
        nextToken(); // Consume 'WHILE', currentToken_ is now LPAREN ('(')
        if (currentToken_.type != TokenType::LPAREN)
        {
            error("Expected '(' after 'while', got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume 'LPAREN'

        auto condition = parseExpression();
        if (!condition)
            return nullptr;
        if (!expect(TokenType::RPAREN))
            return nullptr;

        std::vector<std::unique_ptr<Statement>> body;
        if (peekToken_.type == TokenType::LBRACE)
        {
            nextToken(); // Consume LBRACE (peek -> current)
            nextToken(); // Start of block
            body = parseBlockStatement();

            if (currentToken_.type != TokenType::RBRACE)
            {
                error("Expected RBRACE after 'while' block, got " + currentToken_.debugString());
                return nullptr;
            }
            nextToken(); // Consume RBRACE
        }
        else
        {
            nextToken(); // Move past RPAREN
            auto stmt = parseStatement();
            if (!stmt)
                return nullptr;
            body.push_back(std::move(stmt));
        }

        return std::make_unique<WhileStmt>(std::move(condition), std::move(body), line, col);
    }

    std::unique_ptr<Statement> Parser::parseForStatement()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is FOR
         */
        nextToken(); // Consume 'FOR', currentToken_ is now LPAREN ('(')

        if (currentToken_.type != TokenType::LPAREN)
        {
            error("Expected '(' after 'for', got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume 'LPAREN'

        if (currentToken_.type == TokenType::VAR)
        {
            nextToken(); // Consume VAR. currentToken_ is IDENTIFIER.
            if (currentToken_.type != TokenType::IDENTIFIER)
            {
                error("Expected identifier after 'var'.");
                return nullptr;
            }
            std::string varName = currentToken_.value;
            nextToken(); // Consume name. currentToken_ is now IN or ASSIGN or SEMICOLON.

            if (currentToken_.type == TokenType::IDENTIFIER && currentToken_.value == "in")
            {                // Check for pseudo-keyword 'in'
                nextToken(); // Consume 'in'. currentToken_ is start of iterable expr.

                auto iterable = parseExpression();
                if (!iterable)
                    return nullptr;

                if (!expect(TokenType::RPAREN))
                    return nullptr;

                std::vector<std::unique_ptr<Statement>> body;
                if (peekToken_.type == TokenType::LBRACE)
                {
                    nextToken(); // Consume LBRACE (peek -> current)
                    nextToken(); // Start of block
                    body = parseBlockStatement();
                    if (currentToken_.type != TokenType::RBRACE)
                    {
                        error("Expected '}' after for loop body.");
                        return nullptr;
                    }
                    nextToken(); // Consume RBRACE
                }
                else
                {
                    nextToken(); // Move past RPAREN
                    auto stmt = parseStatement();
                    if (!stmt)
                        return nullptr;
                    body.push_back(std::move(stmt));
                }

                return std::make_unique<ForEachStmt>(varName, std::move(iterable), std::move(body), line, col);
            }
            else
            {
                /**
                 * It's a standard for loop.
                 */
                std::unique_ptr<Expression> initExpr = nullptr;
                if (currentToken_.type == TokenType::ASSIGN)
                {
                    nextToken(); // Consume '='
                    initExpr = parseExpression();
                }
                if (!expect(TokenType::SEMICOLON))
                    return nullptr;

                auto initializer = std::make_unique<VarDeclStmt>(varName, std::move(initExpr), line, col);
                nextToken(); // Consume SEMICOLON. currentToken_ is now start of condition.

                /**
                 * Parse condition
                 */
                std::unique_ptr<Expression> condition = nullptr;
                if (currentToken_.type != TokenType::SEMICOLON)
                {
                    condition = parseExpression();
                }
                if (!expect(TokenType::SEMICOLON))
                    return nullptr;
                nextToken(); // Consume SEMICOLON

                /**
                 * Parse increment
                 */
                std::unique_ptr<Statement> increment = nullptr;
                if (currentToken_.type != TokenType::RPAREN)
                {
                    auto expr = parseExpression();
                    if (peekToken_.type == TokenType::ASSIGN)
                    {
                        IdentifierExpr *idExpr = dynamic_cast<IdentifierExpr *>(expr.get());
                        if (!idExpr)
                        {
                            error("Invalid left-hand side in assignment.");
                            return nullptr;
                        }
                        nextToken(); // Consume target.
                        nextToken(); // Consume ASSIGN.
                        auto value = parseExpression(LOWEST);
                        increment = std::make_unique<AssignStmt>(std::move(expr), std::move(value), line, col);
                    }
                    else
                    {
                        increment = std::make_unique<ExpressionStmt>(std::move(expr), line, col);
                    }
                }

                if (!expect(TokenType::RPAREN))
                    return nullptr;

                std::vector<std::unique_ptr<Statement>> body;
                if (peekToken_.type == TokenType::LBRACE)
                {
                    nextToken(); // Consume LBRACE
                    nextToken(); // Start of block
                    body = parseBlockStatement();

                    if (currentToken_.type != TokenType::RBRACE)
                    {
                        error("Expected RBRACE after 'for' block, got " + currentToken_.debugString());
                        return nullptr;
                    }
                    nextToken(); // Consume RBRACE
                }
                else
                {
                    nextToken(); // Move past RPAREN
                    auto stmt = parseStatement();
                    if (!stmt)
                        return nullptr;
                    body.push_back(std::move(stmt));
                }

                return std::make_unique<ForStmt>(std::move(initializer), std::move(condition), std::move(increment), std::move(body), line, col);
            }
        }

        std::unique_ptr<Statement> initializer = nullptr;
        if (currentToken_.type == TokenType::SEMICOLON)
        {
            nextToken(); // Empty initializer
        }
        else
        {
            initializer = parseExpressionStatement(); // This parses expr and consumes semicolon
        }

        /**
         * Parse condition
         */
        std::unique_ptr<Expression> condition = nullptr;
        if (currentToken_.type != TokenType::SEMICOLON)
        {
            condition = parseExpression();
        }
        if (!expect(TokenType::SEMICOLON))
            return nullptr;
        nextToken(); // Consume SEMICOLON

        /**
         * Parse increment
         */
        std::unique_ptr<Statement> increment = nullptr;
        if (currentToken_.type != TokenType::RPAREN)
        {
            auto expr = parseExpression();
            if (peekToken_.type == TokenType::ASSIGN)
            {
                IdentifierExpr *idExpr = dynamic_cast<IdentifierExpr *>(expr.get());
                if (!idExpr)
                {
                    error("Invalid left-hand side in assignment.");
                    return nullptr;
                }
                nextToken(); // Consume target.
                nextToken(); // Consume ASSIGN.
                auto value = parseExpression(LOWEST);
                increment = std::make_unique<AssignStmt>(std::move(expr), std::move(value), line, col);
            }
            else
            {
                increment = std::make_unique<ExpressionStmt>(std::move(expr), line, col);
            }
        }

        if (!expect(TokenType::RPAREN))
            return nullptr;

        std::vector<std::unique_ptr<Statement>> body;
        if (peekToken_.type == TokenType::LBRACE)
        {
            nextToken(); // Consume LBRACE
            nextToken(); // Start of block
            body = parseBlockStatement();

            if (currentToken_.type != TokenType::RBRACE)
            {
                error("Expected RBRACE after 'for' block, got " + currentToken_.debugString());
                return nullptr;
            }
            nextToken(); // Consume RBRACE
        }
        else
        {
            nextToken(); // Move past RPAREN
            auto stmt = parseStatement();
            if (!stmt)
                return nullptr;
            body.push_back(std::move(stmt));
        }

        return std::make_unique<ForStmt>(std::move(initializer), std::move(condition), std::move(increment), std::move(body), line, col);
    }

    std::unique_ptr<Statement> Parser::parseReturnStatement()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is RETURN
         */
        nextToken(); // Consume RETURN

        std::unique_ptr<Expression> returnValue = nullptr;
        if (currentToken_.type == TokenType::SEMICOLON)
        {
            nextToken(); // Consume SEMICOLON
        }
        else
        {
            returnValue = parseExpression();
            if (!returnValue)
                return nullptr;

            if (!expect(TokenType::SEMICOLON))
                return nullptr;
            nextToken(); // Consume SEMICOLON (which is now in currentToken_)
        }
        return std::make_unique<ReturnStmt>(std::move(returnValue), line, col);
    }

    std::unique_ptr<Statement> Parser::parseExpressionStatement()
    {
        auto expr = parseExpression();
        if (!expr)
            return nullptr;

        if (peekToken_.type == TokenType::ASSIGN)
        {
            bool isValidLValue = (dynamic_cast<IdentifierExpr *>(expr.get()) != nullptr) ||
                                 (dynamic_cast<IndexExpr *>(expr.get()) != nullptr) ||
                                 (dynamic_cast<MemberAccessExpr *>(expr.get()) != nullptr);

            if (!isValidLValue)
            {
                error("Invalid left-hand side in assignment. Must be an identifier, index, or member access expression.");
                return nullptr;
            }
            return parseAssignmentStatement(std::move(expr));
        }
        else
        {
            // Only expect a semicolon if the next token is not a closing brace or EOF
            if (peekToken_.type != TokenType::RBRACE && peekToken_.type != TokenType::END_OF_FILE)
            {
                if (!expect(TokenType::SEMICOLON))
                    return nullptr;
                nextToken(); // Consume SEMICOLON
            }
            // If peekToken_ IS RBRACE or EOF, we don't expect a semicolon.
            return std::make_unique<ExpressionStmt>(std::move(expr), expr->getLine(), expr->getColumn());
        }
    }

    std::unique_ptr<Statement> Parser::parseFunctionDeclaration()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is FUNCTION
         */
        nextToken(); // Consume FUNCTION

        if (currentToken_.type != TokenType::IDENTIFIER)
        {
            error("Expected function name, got " + currentToken_.debugString());
            return nullptr;
        }
        std::string funcName = currentToken_.value;
        nextToken(); // Consume function name

        if (currentToken_.type != TokenType::LPAREN)
        {
            error("Expected '(' after function name, got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume '('

        std::vector<std::string> params;
        if (currentToken_.type != TokenType::RPAREN)
        {
            do
            {
                if (currentToken_.type != TokenType::IDENTIFIER)
                {
                    error("Expected parameter name, got " + currentToken_.debugString());
                    return nullptr;
                }
                params.push_back(currentToken_.value);
                nextToken(); // Consume parameter name
            } while (currentToken_.type == TokenType::COMMA && (nextToken(), true));
        }

        if (currentToken_.type != TokenType::RPAREN)
        {
            error("Expected ')' after parameters, got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume ')'

        if (currentToken_.type != TokenType::LBRACE)
        {
            error("Expected '{' for function body, got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume '{'

        std::vector<std::unique_ptr<Statement>> body = parseBlockStatement();

        if (currentToken_.type != TokenType::RBRACE)
        {
            error("Expected '}' after function body, got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume '}'

        return std::make_unique<FunctionDeclStmt>(funcName, std::move(params), std::move(body), line, col);
    }

    std::unique_ptr<Statement> Parser::parseClassDeclaration()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is CLASS
         */
        nextToken(); // Consume CLASS. currentToken_ is IDENTIFIER (name)

        if (currentToken_.type != TokenType::IDENTIFIER)
        {
            error("Expected class name.");
            return nullptr;
        }
        std::string className = currentToken_.value;

        if (!expect(TokenType::LBRACE))
            return nullptr; // Expect '{' (peek), consumes Name, currentToken_ becomes '{'
        nextToken();        // Consume '{'. currentToken_ becomes start of body

        std::vector<std::unique_ptr<FunctionDeclStmt>> methods;
        while (currentToken_.type != TokenType::RBRACE && currentToken_.type != TokenType::END_OF_FILE)
        {
            if (currentToken_.type == TokenType::FUNCTION)
            {
                auto stmt = parseFunctionDeclaration(); // Handles 'fun name() {}'
                if (auto funcDecl = dynamic_cast<FunctionDeclStmt *>(stmt.get()))
                {
                    stmt.release(); // Release ownership from stmt
                    methods.push_back(std::unique_ptr<FunctionDeclStmt>(funcDecl));
                }
                else
                {
                    error("Error parsing class method.");
                    return nullptr;
                }
            }
            else
            {
                error("Only methods (fun) are supported in classes for now.");
                return nullptr;
            }
        }

        if (currentToken_.type != TokenType::RBRACE)
        {
            error("Expected '}' after class body, got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume '}'

        return std::make_unique<ClassDeclStmt>(className, std::move(methods), line, col);
    }

    std::unique_ptr<Statement> Parser::parseImportStatement()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is IMPORT
         */
        nextToken(); // Consume IMPORT

        std::string moduleName; // Declare moduleName here

        if (currentToken_.type != TokenType::IDENTIFIER)
        {
            error("Expected module name after 'import', got " + currentToken_.debugString());
            return nullptr;
        }
        moduleName = currentToken_.value;
        nextToken(); // Consume IDENTIFIER

        while (currentToken_.type == TokenType::DOT || currentToken_.type == TokenType::DIVIDE)
        {
            if (currentToken_.type == TokenType::DOT)
            {
                moduleName += ".";
            }
            else
            { // It's DIVIDE
                moduleName += "/";
            }
            nextToken(); // Consume DOT or DIVIDE
            if (currentToken_.type != TokenType::IDENTIFIER)
            {
                error("Expected identifier after '.' or '/' in module name.");
                return nullptr;
            }
            moduleName += currentToken_.value;
            nextToken(); // Consume IDENTIFIER
        }

        if (currentToken_.type != TokenType::SEMICOLON)
        {
            error("Expected SEMICOLON after module name, got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume SEMICOLON

        return std::make_unique<ImportStmt>(moduleName, line, col);
    }

    std::unique_ptr<Statement> Parser::parseFromImportStatement()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is FROM
         */
        nextToken(); // Consume FROM

        std::string moduleName;
        if (currentToken_.type != TokenType::IDENTIFIER)
        {
            error("Expected module name after 'from', got " + currentToken_.debugString());
            return nullptr;
        }
        moduleName = currentToken_.value;
        nextToken(); // Consume module name

        while (currentToken_.type == TokenType::DOT || currentToken_.type == TokenType::DIVIDE)
        {
            if (currentToken_.type == TokenType::DOT)
            {
                moduleName += ".";
            }
            else
            { // It's DIVIDE
                moduleName += "/";
            }
            nextToken(); // Consume DOT or DIVIDE
            if (currentToken_.type != TokenType::IDENTIFIER)
            {
                error("Expected identifier after '.' or '/' in module name.");
                return nullptr;
            }
            moduleName += currentToken_.value;
            nextToken(); // Consume IDENTIFIER
        }

        if (currentToken_.type != TokenType::IMPORT)
        {
            error("Expected 'import' after module name in 'from' statement, got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume IMPORT

        std::vector<std::string> symbols;
        do
        {
            if (currentToken_.type != TokenType::IDENTIFIER)
            {
                error("Expected symbol name in import list, got " + currentToken_.debugString());
                return nullptr;
            }
            symbols.push_back(currentToken_.value);
            nextToken(); // Consume symbol name
        } while (currentToken_.type == TokenType::COMMA && (nextToken(), true));

        if (currentToken_.type != TokenType::SEMICOLON)
        {
            error("Expected SEMICOLON after import list, got " + currentToken_.debugString());
            return nullptr;
        }
        nextToken(); // Consume SEMICOLON

        return std::make_unique<FromImportStmt>(moduleName, symbols, line, col);
    }

    std::unique_ptr<Statement> Parser::parseExportStatement()
    {
        int line = currentToken_.line;
        int col = currentToken_.column;
        /**
         * currentToken_ is EXPORT
         */
        nextToken(); // Consume EXPORT

        std::unique_ptr<Statement> stmt = nullptr;
        if (currentToken_.type == TokenType::VAR)
        {
            stmt = parseVarDeclarationStatement();
        }
        else if (currentToken_.type == TokenType::FUNCTION)
        {
            stmt = parseFunctionDeclaration();
        }
        else if (currentToken_.type == TokenType::CLASS) // ADD THIS
        {
            stmt = parseClassDeclaration();
        }
        else if (currentToken_.type == TokenType::IDENTIFIER || currentToken_.type == TokenType::THIS) // Exporting an identifier or member access or 'this'
        {
            auto expr = parseExpression(LOWEST); // Parse the identifier or member access expression
            if (!expr) {
                error("Expected an expression after 'export', got " + currentToken_.debugString());
                return nullptr;
            }
            if (!expect(TokenType::SEMICOLON)) return nullptr; // Expect a semicolon after exported expression
            nextToken(); // Consume SEMICOLON (it was in peekToken_ for expect, now in currentToken_ after expect)
            return std::make_unique<ExportExpressionStmt>(std::move(expr), line, col);
        }
        else
        {
            error("Expected 'var', 'fun', 'class', or an identifier after 'export', got " + currentToken_.debugString()); // Update error message
            return nullptr;
        }

        return std::make_unique<ExportStmt>(std::move(stmt), line, col);
    }

    std::vector<std::unique_ptr<Statement>> Parser::parseBlockStatement()
    {
        std::vector<std::unique_ptr<Statement>> statements;

        while (currentToken_.type != TokenType::RBRACE && currentToken_.type != TokenType::END_OF_FILE)
        {
            auto stmt = parseStatement();
            if (stmt)
            {
                statements.push_back(std::move(stmt));
            }
            else
            {
                /**
                 * Skip token if statement parsing failed to avoid infinite loop
                 */
                nextToken();
            }
        }
        /**
         * currentToken_ is now RBRACE or END_OF_FILE. It's up to the caller to consume RBRACE.
         */
        return statements;
    }

} // namespace Pome