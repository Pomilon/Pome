#ifndef POME_PARSER_H
#define POME_PARSER_H

#include "pome_lexer.h"
#include "pome_ast.h"
#include <memory>
#include <vector>
#include <stdexcept>

namespace Pome
{

    class Parser
    {
    public:
        explicit Parser(Lexer &lexer);
        std::unique_ptr<Program> parseProgram();

        /**
         * Operator precedence - moved to public
         */
        enum Precedence
        {
            LOWEST,
            ASSIGN,       // =
            TERNARY,      // ? :
            EQUALS,       // == !=
            LESSGREATER,  // < > <= >=
            SUM,          // + -
            PRODUCT,      // * / %
            PREFIX,       // - ! (unary)
            CALL,         // ()
            MEMBER_ACCESS // . (dot operator)
        };

    private:
        Lexer &lexer_;
        Token currentToken_;
        Token peekToken_;

        void nextToken();
        bool expect(TokenType type);
        bool match(TokenType type);
        void error(const std::string &message);

        /**
         * Parsing expressions
         */
        std::unique_ptr<Expression> parseExpression();                      // Overload without precedence argument
        std::unique_ptr<Expression> parseExpression(Precedence precedence); // Overload with precedence argument
        std::unique_ptr<Expression> parsePrimaryExpression();
        std::unique_ptr<Expression> parseCallExpression(std::unique_ptr<Expression> callee);
        std::unique_ptr<Expression> parsePrefixExpression();                                         // For unary operators
        std::unique_ptr<Expression> parseInfixExpression(std::unique_ptr<Expression> left);          // For binary operators
        std::unique_ptr<Expression> parseMemberAccessExpression(std::unique_ptr<Expression> object); // New method for member access
        std::unique_ptr<Expression> parseIndexExpression(std::unique_ptr<Expression> object);        // New method for index access
        std::unique_ptr<Expression> parseTernaryExpression(std::unique_ptr<Expression> condition);   // Added for ternary
        std::unique_ptr<Expression> parseListLiteral();                                              // New method for list literals
        std::unique_ptr<Expression> parseTableLiteral();                                             // New method for table literals

        Precedence getPrecedence(TokenType type);

        /**
         * Parsing statements
         */
        std::unique_ptr<Statement> parseStatement();
        std::unique_ptr<Statement> parseVarDeclarationStatement();
        std::unique_ptr<Statement> parseAssignmentStatement(std::unique_ptr<Expression> target);
        std::unique_ptr<Statement> parseIfStatement();
        std::unique_ptr<Statement> parseWhileStatement();
        std::unique_ptr<Statement> parseForStatement();
        std::unique_ptr<Statement> parseReturnStatement();
        std::unique_ptr<Statement> parseExpressionStatement();
        std::unique_ptr<Statement> parseFunctionDeclaration();
        std::unique_ptr<Statement> parseClassDeclaration(); // Added
        std::unique_ptr<Statement> parseImportStatement();
        std::unique_ptr<Statement> parseFromImportStatement();
        std::unique_ptr<Statement> parseExportStatement();

        std::vector<std::unique_ptr<Statement>> parseBlockStatement();
    };

} // namespace Pome

#endif // POME_PARSER_H