#include "../include/pome_lexer.h"
#include <map>

namespace Pome
{

    // Static helper to convert TokenType to string
    std::string Token::toString(TokenType type)
    {
        static const std::map<TokenType, std::string> tokenTypeNames = {
            {TokenType::FUNCTION, "FUNCTION"}, {TokenType::IF, "IF"}, {TokenType::ELSE, "ELSE"}, {TokenType::WHILE, "WHILE"}, {TokenType::FOR, "FOR"}, {TokenType::RETURN, "RETURN"}, {TokenType::TRUE, "TRUE"}, {TokenType::FALSE, "FALSE"}, {TokenType::NIL, "NIL"}, {TokenType::IMPORT, "IMPORT"}, {TokenType::FROM, "FROM"}, {TokenType::EXPORT, "EXPORT"}, {TokenType::VAR, "VAR"}, {TokenType::CLASS, "CLASS"}, {TokenType::THIS, "THIS"}, // Added for OOP

            {TokenType::PLUS, "PLUS"},
            {TokenType::MINUS, "MINUS"},
            {TokenType::MULTIPLY, "MULTIPLY"},
            {TokenType::DIVIDE, "DIVIDE"},
            {TokenType::MODULO, "MODULO"},
            {TokenType::ASSIGN, "ASSIGN"},
            {TokenType::EQ, "EQ"},
            {TokenType::NE, "NE"},
            {TokenType::LT, "LT"},
            {TokenType::LE, "LE"},
            {TokenType::GT, "GT"},
            {TokenType::GE, "GE"},
            {TokenType::AND, "AND"},
            {TokenType::OR, "OR"},
            {TokenType::NOT, "NOT"},
            {TokenType::QUESTION, "QUESTION"}, // Added for ternary operator

            {TokenType::LPAREN, "LPAREN"},
            {TokenType::RPAREN, "RPAREN"},
            {TokenType::LBRACE, "LBRACE"},
            {TokenType::RBRACE, "RBRACE"},
            {TokenType::LBRACKET, "LBRACKET"},
            {TokenType::RBRACKET, "RBRACKET"},
            {TokenType::COMMA, "COMMA"},
            {TokenType::DOT, "DOT"},
            {TokenType::COLON, "COLON"},
            {TokenType::SEMICOLON, "SEMICOLON"},

            {TokenType::IDENTIFIER, "IDENTIFIER"},
            {TokenType::NUMBER, "NUMBER"},
            {TokenType::STRING, "STRING"},

            {TokenType::END_OF_FILE, "END_OF_FILE"},
            {TokenType::UNKNOWN, "UNKNOWN"}};

        auto it = tokenTypeNames.find(type);
        if (it != tokenTypeNames.end())
        {
            return it->second;
        }
        return "UNKNOWN";
    }

    // Member function to get a detailed string representation of the token
    std::string Token::debugString() const
    {
        return "Type: " + Token::toString(type) + ", Value: '" + value + "', Line: " + std::to_string(line) + ", Col: " + std::to_string(column);
    }

    Lexer::Lexer(const std::string &source)
        : source(source), currentPos(0), currentLine(1), currentCol(1) {}

    char Lexer::peek()
    {
        if (currentPos >= source.length())
        {
            return '\0';
        }
        return source[currentPos];
    }

    char Lexer::advance()
    {
        if (currentPos >= source.length())
        {
            return '\0';
        }
        char c = source[currentPos];
        currentPos++;
        currentCol++;
        return c;
    }

    void Lexer::skipWhitespace()
    {
        for (;;)
        { // Loop to skip multiple comments/whitespace
            while (currentPos < source.length() && (isspace(peek()) || peek() == '\r'))
            {
                if (peek() == '\n')
                {
                    currentLine++;
                    currentCol = 1;
                }
                advance();
            }

            // Handle single-line comments: //
            if (peek() == '/' && currentPos + 1 < source.length() && source[currentPos + 1] == '/')
            {
                while (currentPos < source.length() && peek() != '\n' && peek() != '\r')
                {
                    advance(); // Consume until newline or EOF
                }
                // Don't advance past newline here, let the outer loop handle it as whitespace
                continue; // Go back to check for more whitespace/comments
            }

            // Handle multi-line comments: /* ... */
            if (peek() == '/' && currentPos + 1 < source.length() && source[currentPos + 1] == '*')
            {
                advance(); // Consume '/'
                advance(); // Consume '*'
                bool commentEnded = false;
                while (currentPos < source.length())
                {
                    if (peek() == '*' && currentPos + 1 < source.length() && source[currentPos + 1] == '/')
                    {
                        advance(); // Consume '*'
                        advance(); // Consume '/'
                        commentEnded = true;
                        break;
                    }
                    if (peek() == '\n')
                    {
                        currentLine++;
                        currentCol = 1;
                    }
                    advance();
                }
                if (!commentEnded)
                {
                    // Handle error: unterminated multi-line comment. For now, just exit loop.
                    // In a real lexer, this would throw an error.
                    break;
                }
                continue; // Go back to check for more whitespace/comments
            }

            break; // No more whitespace or comments found, exit loop
        }
    }

    Token Lexer::readIdentifier()
    {
        size_t start = currentPos;
        while (currentPos < source.length() && (isalnum(peek()) || peek() == '_'))
        {
            advance();
        }
        std::string value = source.substr(start, currentPos - start);

        // Check for keywords
        static const std::map<std::string, TokenType> keywords = {
            {"fun", TokenType::FUNCTION}, {"if", TokenType::IF}, {"else", TokenType::ELSE}, {"while", TokenType::WHILE}, {"for", TokenType::FOR}, {"return", TokenType::RETURN}, {"true", TokenType::TRUE}, {"false", TokenType::FALSE}, {"nil", TokenType::NIL}, {"import", TokenType::IMPORT}, {"from", TokenType::FROM}, {"export", TokenType::EXPORT}, {"var", TokenType::VAR}, {"class", TokenType::CLASS}, {"this", TokenType::THIS}, // Added for OOP
            {"and", TokenType::AND},
            {"or", TokenType::OR},
            {"not", TokenType::NOT}};

        TokenType type = TokenType::IDENTIFIER;
        auto it = keywords.find(value);
        if (it != keywords.end())
        {
            type = it->second;
        }
        return makeToken(type, value);
    }

    Token Lexer::readNumber()
    {
        size_t start = currentPos;
        bool hasDecimal = false;
        while (currentPos < source.length() && isdigit(peek()))
        {
            advance();
        }
        if (peek() == '.' && (currentPos + 1 < source.length() && isdigit(source[currentPos + 1])))
        {
            hasDecimal = true;
            advance(); // Consume the dot
            while (currentPos < source.length() && isdigit(peek()))
            {
                advance();
            }
        }
        std::string value = source.substr(start, currentPos - start);
        return makeToken(TokenType::NUMBER, value);
    }

    Token Lexer::readString()
    {
        // Assume opening quote has already been consumed
        size_t start = currentPos;
        while (peek() != '"' && peek() != '\0')
        {
            if (peek() == '\n')
            { // Strings cannot span multiple lines in Pome without explicit escape (not implemented yet)
                throw std::runtime_error("Unterminated string literal (newline encountered)");
            }
            advance();
        }

        if (peek() == '\0')
        {
            throw std::runtime_error("Unterminated string literal (EOF)");
        }

        std::string value = source.substr(start, currentPos - start);
        advance(); // Consume the closing quote
        return makeToken(TokenType::STRING, value);
    }

    Token Lexer::makeToken(TokenType type, const std::string &value)
    {
        // Adjust currentCol for the length of the token value for accurate reporting.
        // The current currentCol points to the character AFTER the token has been read.
        // So, we subtract the length of the value to get the start column.
        return {type, value, currentLine, currentCol - static_cast<int>(value.length())};
    }

    Token Lexer::getNextToken()
    {
        skipWhitespace();

        if (currentPos >= source.length())
        {
            return makeToken(TokenType::END_OF_FILE, "");
        }

        char c = peek();
        int tokenStartCol = currentCol; // Store starting column for this token

        if (isalpha(c) || c == '_')
        {
            return readIdentifier();
        }

        if (isdigit(c))
        {
            return readNumber();
        }

        if (c == '"')
        {
            advance(); // Consume opening quote
            try
            {
                return readString();
            }
            catch (const std::runtime_error &e)
            {
                // For now, re-throw or return an error token
                return makeToken(TokenType::UNKNOWN, e.what()); // Or handle more gracefully
            }
        }

        // Handle operators and delimiters
        switch (c)
        {
        case '+':
            advance();
            return makeToken(TokenType::PLUS, "+");
        case '-':
            advance();
            return makeToken(TokenType::MINUS, "-");
        case '*':
            advance();
            return makeToken(TokenType::MULTIPLY, "*");
        case '%':
            advance();
            return makeToken(TokenType::MODULO, "% ");
        case '=':
        {
            advance();
            if (peek() == '=')
            {
                advance();
                return makeToken(TokenType::EQ, "==");
            }
            return makeToken(TokenType::ASSIGN, "=");
        }
        case '!':
        {
            advance();
            if (peek() == '=')
            {
                advance();
                return makeToken(TokenType::NE, "!=");
            }
            // If '!' is not followed by '=', it might be a unary NOT.
            // For now, treat it as unknown, parser will decide its context.
            return makeToken(TokenType::NOT, "!"); // Changed to NOT for unary operator
        }
        case '<':
        {
            advance();
            if (peek() == '=')
            {
                advance();
                return makeToken(TokenType::LE, "<=");
            }
            return makeToken(TokenType::LT, "<");
        }
        case '>':
        {
            advance();
            if (peek() == '=')
            {
                advance();
                return makeToken(TokenType::GE, ">=");
            }
            return makeToken(TokenType::GT, ">");
        }
        case '(':
            advance();
            return makeToken(TokenType::LPAREN, "(");
        case ')':
            advance();
            return makeToken(TokenType::RPAREN, ")");
        case '{':
            advance();
            return makeToken(TokenType::LBRACE, "{");
        case '}':
            advance();
            return makeToken(TokenType::RBRACE, "}");
        case '[':
            advance();
            return makeToken(TokenType::LBRACKET, "[");
        case ']':
            advance();
            return makeToken(TokenType::RBRACKET, "]");
        case ',':
            advance();
            return makeToken(TokenType::COMMA, ",");
        case '.':
            advance();
            return makeToken(TokenType::DOT, ".");
        case ':':
            advance();
            return makeToken(TokenType::COLON, ":");
        case ';':
            advance();
            return makeToken(TokenType::SEMICOLON, ";");
        case '?':
            advance();
            return makeToken(TokenType::QUESTION, "?"); // Added for ternary operator
        case '/':
        {
            advance();
            if (peek() == '/' || peek() == '*')
            {   // This should be handled by skipWhitespace now
                // This block should ideally not be reached if skipWhitespace is correct
                // For now, just return DIVIDE, but it implies a problem in comment skipping logic
                return makeToken(TokenType::DIVIDE, "/");
            }
            return makeToken(TokenType::DIVIDE, "/");
        }
        default:
            std::string unknownChar(1, advance());
            return makeToken(TokenType::UNKNOWN, unknownChar);
        }
    }

} // namespace Pome