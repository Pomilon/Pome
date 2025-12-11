#ifndef POME_INTERPRETER_H
#define POME_INTERPRETER_H

#include "pome_ast.h"
#include "pome_value.h"
#include "pome_environment.h"
#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream> // For temporary print
#include <fstream>  // For reading module files
#include <sstream>  // For stringstream
#include <map>      // For loadedModules_

namespace Pome
{

    class Interpreter : public ASTVisitor
    {
    public:
        Interpreter();
        void interpret(Program &program);

        // Visitor methods for expressions - return PomeValue
        void visit(NumberExpr &expr) override;
        void visit(StringExpr &expr) override;
        void visit(IdentifierExpr &expr) override;
        void visit(BinaryExpr &expr) override;
        void visit(UnaryExpr &expr) override;
        void visit(CallExpr &expr) override;
        void visit(MemberAccessExpr &expr) override; // New visitor method
        void visit(ListExpr &expr) override;
        void visit(TableExpr &expr) override;
        void visit(IndexExpr &expr) override;
        void visit(TernaryExpr &expr) override;
        void visit(ThisExpr &expr) override; // Added

        // Statements
        void visit(VarDeclStmt &stmt) override;
        void visit(AssignStmt &stmt) override;
        void visit(IfStmt &stmt) override;
        void visit(WhileStmt &stmt) override;
        void visit(ForStmt &stmt) override;
        void visit(ForEachStmt &stmt) override;
        void visit(ReturnStmt &stmt) override;
        void visit(ExpressionStmt &stmt) override;
        void visit(FunctionDeclStmt &stmt) override;
        void visit(ClassDeclStmt &stmt) override; // Added
        void visit(ImportStmt &stmt) override;    // New visitor method for import statements
        void visit(FromImportStmt &stmt) override;
        void visit(ExportStmt &stmt) override;

        // Program visitor
        void visit(Program &program) override;

        // Helper to get the last evaluated value (for expressions)
        PomeValue getLastEvaluatedValue() const { return lastEvaluatedValue_; }

    private:
        PomeValue lastEvaluatedValue_; // Stores the result of the last expression evaluation
        std::shared_ptr<Environment> currentEnvironment_;
        // Changed to cache both Program AST, its Environment, and its Export Table
        struct ModuleCacheEntry
        {
            std::shared_ptr<Program> program;
            std::shared_ptr<Environment> env;
            std::shared_ptr<std::map<PomeValue, PomeValue>> exports;
        };
        std::map<std::string, ModuleCacheEntry> loadedModules_;

        PomeValue evaluateExpression(Expression &expr);
        void executeStatement(Statement &stmt);

        // Binary operation helpers
        PomeValue applyBinaryOp(const PomeValue &left, const std::string &op, const PomeValue &right);
        PomeValue applyUnaryOp(const std::string &op, const PomeValue &operand);

        void setupGlobalEnvironment();
        std::string resolveModulePath(const std::string &moduleName);

        // Module system state
        std::vector<std::string> searchPaths_;
        // Stack of export maps for currently executing modules.
        // The top (back) is the current module's exports.
        // We use shared_ptr because tables are shared_ptr.
        std::vector<std::shared_ptr<std::map<PomeValue, PomeValue>>> exportStack_;

        // Helper to load a module and return its export table
        std::shared_ptr<std::map<PomeValue, PomeValue>> loadModule(const std::string &moduleName);

        // Helper to call a Pome function (user-defined)
        PomeValue callPomeFunction(std::shared_ptr<PomeFunction> func, const std::vector<PomeValue> &args, std::shared_ptr<PomeInstance> thisInstance = nullptr);
    };

} // namespace Pome

#endif // POME_INTERPRETER_H
