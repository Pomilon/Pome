#include "../include/pome_lexer.h"
#include <iostream>
#include <map>

namespace Pome
{

    /**
     * Static helper to convert TokenType to string
     */
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

    /**
     * Member function to get a detailed string representation of the token
     */
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

            /**
             * Handle single-line comments: //
             */
            if (peek() == '/' && currentPos + 1 < source.length() && source[currentPos + 1] == '/')
            {
                while (currentPos < source.length() && peek() != '\n' && peek() != '\r')
                {
                    advance(); // Consume until newline or EOF
                }
                /**
                 * Don't advance past newline here, let the outer loop handle it as whitespace
                 */
                continue; // Go back to check for more whitespace/comments
            }

            /**
             * Handle multi-line comments: / * ... * /
             */
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
                    /**
                     * Handle error: unterminated multi-line comment. For now, just exit loop.
                     */
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

        /**
         * Check for keywords
         */
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
            advance(); // Consume the dot
            while (currentPos < source.length() && isdigit(peek()))
            {
                advance();
            }
        }
        // Handle scientific notation (e.g., 1e5, 1.5e-3, 1E+2)
        if (currentPos < source.length() && (peek() == 'e' || peek() == 'E'))
        {
            advance(); // Consume 'e' or 'E'
            if (currentPos < source.length() && (peek() == '+' || peek() == '-'))
            {
                advance(); // Consume '+' or '-'
            }
            // Must have at least one digit after 'e'/'E' (and optional sign)
            if (currentPos < source.length() && isdigit(peek()))
            {
                while (currentPos < source.length() && isdigit(peek()))
                {
                    advance();
                }
            }
            else
            {
                // If 'e'/'E' is not followed by digits (or sign then digits),
                // it's not a valid scientific number.
                // Leave it to std::stod to handle invalid format, but consume the 'e'/'E' and optional sign.
            }
        }
        std::string value = source.substr(start, currentPos - start);
        return makeToken(TokenType::NUMBER, value);
    }

    Token Lexer::readString()
    {
        /**
         * Assume opening quote has already been consumed
         */
        size_t start = currentPos;
        std::string raw_value; // To build the string with processed escapes

        while (peek() != '"' && peek() != '\0')
        {
            if (peek() == '\n')
            { // Strings cannot span multiple lines in Pome without explicit escape (not implemented yet)
                throw std::runtime_error("Unterminated string literal (newline encountered)");
            }

            if (peek() == '\\')
            {              // Handle escape sequences
                advance(); // Consume '\\'
                char escapedChar = peek();
                switch (escapedChar)
                {
                case '"':
                    raw_value += '"';
                    break;
                case '\\':
                    raw_value += '\\';
                    break;
                case 'n':
                    raw_value += '\n';
                    break;
                case 't':
                    raw_value += '\t';
                    break;
                case 'r':
                    raw_value += '\r';
                    break;
                // Add more escape sequences as needed
                default:
                    // For unknown escape sequences, include the backslash and the character literally
                    raw_value += '\\';
                    raw_value += escapedChar;
                    break;
                }
                advance(); // Consume the escaped character
            }
            else
            {
                raw_value += peek();
                advance(); // Consume regular character
            }
        }

        if (peek() == '\0')
        {
            throw std::runtime_error("Unterminated string literal (EOF)");
        }

        // Consume the closing quote
        advance();
        return makeToken(TokenType::STRING, raw_value);
    }

    Token Lexer::makeToken(TokenType type, const std::string &value)
    {
        /**
         * Adjust currentCol for the length of the token value for accurate reporting.
         * The current currentCol points to the character AFTER the token has been read.
         * So, we subtract the length of the value to get the start column.
         */
        return {type, value, currentLine, currentCol - static_cast<int>(value.length())};
    }

    Token Lexer::getNextToken()
    {
        skipWhitespace();

        if (currentPos >= source.length())
        {
            auto token = makeToken(TokenType::END_OF_FILE, "");
            return token;
        }

        char c = peek();
        // int tokenStartCol = currentCol; // Store starting column for this token - not used

        Token token; // Declare token here
        if (isalpha(c) || c == '_')
        {
            token = readIdentifier();
        }

        else if (isdigit(c))
        {
            token = readNumber();
        }

        else if (c == '"')
        {
            advance(); // Consume opening quote
            try
            {
                token = readString();
            }
            catch (const std::runtime_error &e)
            {
                /**
                 * For now, re-throw or return an error token
                 */
                token = makeToken(TokenType::UNKNOWN, e.what()); // Or handle more gracefully
            }
        }

        /**
         * Handle operators and delimiters
         */
        else
            switch (c) // Changed to else if to correctly chain with previous conditions
            {
            case '+':
                advance();
                token = makeToken(TokenType::PLUS, "+");
                break;
            case '-':
                advance();
                token = makeToken(TokenType::MINUS, "-");
                break;
            case '*':
                advance();
                token = makeToken(TokenType::MULTIPLY, "*");
                break;
            case '%':
                advance();
                token = makeToken(TokenType::MODULO, "%");
                break;
            case '^':
                advance();
                token = makeToken(TokenType::CARET, "^");
                break;
            case '=':
            {
                advance();
                if (peek() == '=')
                {
                    advance();
                    token = makeToken(TokenType::EQ, "==");
                }
                else
                {
                    token = makeToken(TokenType::ASSIGN, "=");
                }
                break;
            }
            case '!':
            {
                advance();
                if (peek() == '=')
                {
                    advance();
                    token = makeToken(TokenType::NE, "!=");
                }
                else
                {
                    token = makeToken(TokenType::NOT, "!"); // Changed to NOT for unary operator
                }
                break;
            }
            case '<':
            {
                advance();
                if (peek() == '=')
                {
                    advance();
                    token = makeToken(TokenType::LE, "<=");
                }
                else
                {
                    token = makeToken(TokenType::LT, "<");
                }
                break;
            }
            case '>':
            {
                advance();
                if (peek() == '=')
                {
                    advance();
                    token = makeToken(TokenType::GE, ">=");
                }
                else
                {
                    token = makeToken(TokenType::GT, ">");
                }
                break;
            }
            case '(':
                advance();
                token = makeToken(TokenType::LPAREN, "(");
                break;
            case ')':
                advance();
                token = makeToken(TokenType::RPAREN, ")");
                break;
            case '{':
                advance();
                token = makeToken(TokenType::LBRACE, "{");
                break;
            case '}':
                advance();
                token = makeToken(TokenType::RBRACE, "}");
                break;
            case '[':
                advance();
                token = makeToken(TokenType::LBRACKET, "[");
                break;
                break;
            case ']':
                advance();
                token = makeToken(TokenType::RBRACKET, "]");
                break;
            case ',':
                advance();
                token = makeToken(TokenType::COMMA, ",");
                break;
            case '.':
                advance();
                token = makeToken(TokenType::DOT, ".");
                break;
            case ':':
                advance();
                token = makeToken(TokenType::COLON, ":");
                break;
            case ';':
                advance();
                token = makeToken(TokenType::SEMICOLON, ";");
                break;
            case '?':
                advance();
                token = makeToken(TokenType::QUESTION, "?"); // Added for ternary operator
                break;
            case '/':
            {
                advance();
                if (peek() == '/' || peek() == '*')
                { // This should be handled by skipWhitespace now
                    /**
                     * This block should ideally not be reached if skipWhitespace is correct
                     * For now, just return DIVIDE, but it implies a problem in comment skipping logic
                     */
                    token = makeToken(TokenType::DIVIDE, "/");
                }
                else
                {
                    token = makeToken(TokenType::DIVIDE, "/");
                }
                break;
            }
            default:
                std::string unknownChar(1, advance());
                token = makeToken(TokenType::UNKNOWN, unknownChar);
                break;
            }
        return token;
    }

} // namespace Pome