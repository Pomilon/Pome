#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "pome_lexer.h"

using namespace Pome;

class PomeFormatter {
public:
    std::string format(const std::string& source) {
        Lexer lexer(source);
        std::stringstream out;
        int indentLevel = 0;
        bool startOfLine = true;

        Token token = lexer.getNextToken();
        while (token.type != TokenType::END_OF_FILE) {
            if (token.type == TokenType::RBRACE) indentLevel--;

            if (startOfLine) {
                for (int i = 0; i < indentLevel; ++i) out << "    ";
                startOfLine = false;
            }

            if (token.type == TokenType::STRING) out << "\"" << token.value << "\"";
            else out << token.value;

            if (token.type == TokenType::LBRACE) {
                indentLevel++;
                out << "\n";
                startOfLine = true;
            } else if (token.type == TokenType::SEMICOLON) {
                out << "\n";
                startOfLine = true;
            } else if (token.type == TokenType::COMMA) {
                out << " ";
            } else {
                // Peek next to see if we need a space
                // This is a simple formatter, so we just add spaces between most tokens
                out << " ";
            }

            token = lexer.getNextToken();
        }

        return out.str();
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: pome-fmt <file.pome>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    PomeFormatter fmt;
    std::cout << fmt.format(buffer.str());
    
    return 0;
}
