#ifndef POME_LEXER_H
#define POME_LEXER_H

#include <string>
#include <vector>
#include <map>

namespace Pome
{

    enum class TokenType
    {
        // Keywords
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
        THIS,
        STRICT, 

        // Operators
        PLUS,
        MINUS,
        MULTIPLY,
        DIVIDE,
        MODULO,
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
        QUESTION,
        CARET, // Added

        // Delimiters
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

        // Literals
        IDENTIFIER,
        NUMBER,
        STRING,

        // End of File
        END_OF_FILE,
        UNKNOWN
    };

    struct Token
    {
        TokenType type;
        std::string value;
        int line;
        int column;

        static std::string toString(TokenType type);
        std::string debugString() const;
    };

    class Lexer
    {
    public:
        Lexer(const std::string &source);
        Token getNextToken();
        char peek(); 

    private:
        std::string source;
        size_t currentPos;
        int currentLine;
        int currentCol;

        Token readIdentifier();
        Token readNumber();
        Token readString();
        Token makeToken(TokenType type, const std::string &value);
        void skipWhitespace();
        char advance();
    };

} // namespace Pome

#endif // POME_LEXER_H
