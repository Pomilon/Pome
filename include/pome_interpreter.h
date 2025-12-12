#ifndef POME_INTERPRETER_H
#define POME_INTERPRETER_H

#include "pome_ast.h"
#include "pome_value.h"
#include "pome_environment.h"
#include "pome_gc.h"
#include "pome_importer.h"
#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

namespace Pome
{

    class Interpreter : public ASTVisitor
    {
    public:
        Interpreter();
        void interpret(Program &program);

        /**
         * Visitor methods for expressions - return PomeValue
         */
        void visit(NumberExpr &expr) override;
        void visit(StringExpr &expr) override;
        void visit(BooleanExpr &expr) override;
        void visit(NilExpr &expr) override;
        void visit(IdentifierExpr &expr) override;
        void visit(ThisExpr &expr) override; // Added
        void visit(BinaryExpr &expr) override;
        void visit(UnaryExpr &expr) override;
        void visit(CallExpr &expr) override;
        void visit(MemberAccessExpr &expr) override;
        void visit(ListExpr &expr) override;
        void visit(TableExpr &expr) override;
        void visit(IndexExpr &expr) override;
        void visit(SliceExpr &expr) override; // Added
        void visit(TernaryExpr &expr) override;
        void visit(FunctionExpr &expr) override; // Added

        /**
         * Statements
         */
        void visit(VarDeclStmt &stmt) override;
        void visit(AssignStmt &stmt) override;
        void visit(IfStmt &stmt) override;
        void visit(WhileStmt &stmt) override;
        void visit(ForStmt &stmt) override;
        void visit(ForEachStmt &stmt) override;
        void visit(ReturnStmt &stmt) override;
        void visit(ExpressionStmt &stmt) override;
        void visit(FunctionDeclStmt &stmt) override;
        void visit(ClassDeclStmt &stmt) override;
        void visit(ImportStmt &stmt) override;
        void visit(FromImportStmt &stmt) override;
        void visit(ExportStmt &stmt) override;

        /**
         * Program visitor
         */
        void visit(Program &program) override;

        PomeValue getLastEvaluatedValue() const { return lastEvaluatedValue_; }

        /**
         * GC Access
         */
        GarbageCollector &getGC() { return gc_; }

        /**
         * Root marking for GC
         */
        void markRoots();

    private:
        PomeValue lastEvaluatedValue_;
        Environment *currentEnvironment_;
        Environment *globalEnvironment_; // Keep track of global scope

        GarbageCollector gc_;
        Importer importer_;

        std::vector<PomeModule *> exportStack_;
        std::map<std::string, PomeModule *> executedModules_; // Cache for executed modules

        PomeValue evaluateExpression(Expression &expr);
        void executeStatement(Statement &stmt);

        /**
         * Binary operation helpers
         */
        PomeValue applyBinaryOp(const PomeValue &left, const std::string &op, const PomeValue &right);
        PomeValue applyUnaryOp(const std::string &op, const PomeValue &operand);

        void setupGlobalEnvironment();

        /**
         * Helper to load or retrieve cached module
         */
        PomeModule *loadModule(const std::string &moduleName);

        PomeValue callPomeFunction(PomeFunction *func, const std::vector<PomeValue> &args, PomeInstance *thisInstance = nullptr);
    };

} // namespace Pome

#endif // POME_INTERPRETER_H
