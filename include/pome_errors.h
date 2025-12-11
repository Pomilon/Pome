#ifndef POME_ERRORS_H
#define POME_ERRORS_H

#include <stdexcept>
#include <string>

namespace Pome
{

    class PomeException : public std::exception
    {
    public:
        PomeException(const std::string &message, int line, int column)
            : message_(message), line_(line), column_(column)
        {
            fullMessage_ = message_ + " at line " + std::to_string(line_) + ", column " + std::to_string(column_);
        }

        const char *what() const noexcept override
        {
            return fullMessage_.c_str();
        }

        int getLine() const { return line_; }
        int getColumn() const { return column_; }

    private:
        std::string message_;
        int line_;
        int column_;
        std::string fullMessage_;
    };

    class SyntaxError : public PomeException
    {
    public:
        SyntaxError(const std::string &message, int line, int column)
            : PomeException("Syntax Error: " + message, line, column) {}
    };

    class RuntimeError : public PomeException
    {
    public:
        RuntimeError(const std::string &message, int line, int column)
            : PomeException("Runtime Error: " + message, line, column) {}
    };

} // namespace Pome

#endif // POME_ERRORS_H
