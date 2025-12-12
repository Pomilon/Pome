#ifndef POME_LEXER_H
#define POME_LEXER_H

#include <string>
#include <vector>
#include <stdexcept>
#include <map> // For static map in toString

namespace Pome
{

    enum class TokenType
    {
        /**
         * Keywords
         */
        FUNCTION,
        IF,
        ELSE,
        WHILE,
        FOR,
        RETURN,
        TRUE,
        FALSE,
        NIL,
        IMPORT,
        FROM,
        EXPORT,
        VAR,
        CLASS,
        THIS, // Added for OOP

        /**
         * Operators
         */
        PLUS,
        MINUS,
        MULTIPLY,
        DIVIDE,
        MODULO,
        CARET, // ^ (Exponentiation)
        ASSIGN,
        EQ,
        NE,
        LT,
        LE,
        GT,
        GE,
        AND,
        OR,
        NOT,
        QUESTION, // Added for ternary operator

        /**
         * Delimiters
         */
        LPAREN,
        RPAREN,
        LBRACE,
        RBRACE,
        LBRACKET,
        RBRACKET,
        COMMA,
        DOT,
        COLON,
        SEMICOLON,

        /**
         * Literals
         */
        IDENTIFIER,
        NUMBER,
        STRING,

        /**
         * Special
         */
        END_OF_FILE,
        UNKNOWN
    };

    struct Token
    {
        TokenType type;
        std::string value;
        int line;
        int column;

        /**
         * Static helper to convert TokenType to string for debugging/error reporting
         */
        static std::string toString(TokenType type);

        /**
         * Member function to get a detailed string representation of the token
         */
        std::string debugString() const;
    };

    class Lexer
    {
    public:
        explicit Lexer(const std::string &source);
        Token getNextToken();

    private:
        const std::string &source;
        size_t currentPos;
        int currentLine;
        int currentCol;

        char peek();
        char advance();
        void skipWhitespace();
        Token readIdentifier();
        Token readNumber();
        Token readString();
        Token makeToken(TokenType type, const std::string &value);
    };

} // namespace Pome

#endif // POME_LEXER_H