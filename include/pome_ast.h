#ifndef POME_AST_H
#define POME_AST_H

#include <vector>
#include <memory> // For std::unique_ptr
#include <string>

namespace Pome
{

    /**
     * Forward declaration of the Visitor interface
     */
    class ASTVisitor;

    /**
     * Base class for all AST nodes
     */
    class ASTNode
    {
    public:
        enum NodeType
        {
            EXPRESSION,
            STATEMENT,
            PROGRAM,
            /**
             * Specific expressions
             */
            NUMBER_EXPR,
            STRING_EXPR,
            BOOLEAN_EXPR,
            NIL_EXPR,
            IDENTIFIER_EXPR,
            BINARY_EXPR,
            UNARY_EXPR,
            CALL_EXPR,
            MEMBER_ACCESS_EXPR, // New node type for member access (dot operator)
            LIST_EXPR,
            TABLE_EXPR,
            INDEX_EXPR,   // New node types for lists and tables
            SLICE_EXPR,   // New node type for slicing
            TERNARY_EXPR, // New node type for ternary operator
            THIS_EXPR,    // New node type for 'this'
            FUNCTION_EXPR, // New node type for function expressions
            /**
             * Specific statements
             */
            VAR_DECL_STMT,
            ASSIGN_STMT,
            IF_STMT,
            WHILE_STMT,
            FOR_STMT,
            FOR_EACH_STMT,
            RETURN_STMT,
            EXPRESSION_STMT,
            FUNCTION_DECL_STMT, // Node type for function declarations
            CLASS_DECL_STMT,    // New node type for class declarations
            IMPORT_STMT,
            FROM_IMPORT_STMT,
            EXPORT_STMT // New node types for module system
        };

        ASTNode(NodeType type, int line, int col) : type_(type), line_(line), col_(col) {}
        virtual ~ASTNode() = default;

        NodeType getType() const { return type_; }
        int getLine() const { return line_; }
        int getColumn() const { return col_; }

        /**
         * Visitor pattern for AST traversal
         */
        virtual void accept(ASTVisitor &visitor) = 0;

    private:
        NodeType type_;
        int line_;
        int col_;
    };

    /**
     * Base class for Expression nodes
     */
    class Expression : public ASTNode
    {
    public:
        Expression(NodeType type, int line, int col) : ASTNode(type, line, col) {}
    };

    /**
     * Base class for Statement nodes
     */
    class Statement : public ASTNode
    {
    public:
        Statement(NodeType type, int line, int col) : ASTNode(type, line, col) {}
    };

    /**
     * --- Concrete Expression Nodes ---
     */

    class NumberExpr : public Expression
    {
    public:
        explicit NumberExpr(double value, int line, int col) : Expression(NUMBER_EXPR, line, col), value_(value) {}
        double getValue() const { return value_; }
        void accept(ASTVisitor &visitor) override;

    private:
        double value_;
    };

    class StringExpr : public Expression
    {
    public:
        explicit StringExpr(const std::string &value, int line, int col) : Expression(STRING_EXPR, line, col), value_(value) {}
        const std::string &getValue() const { return value_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::string value_;
    };

    class BooleanExpr : public Expression
    {
    public:
        explicit BooleanExpr(bool value, int line, int col) : Expression(BOOLEAN_EXPR, line, col), value_(value) {}
        bool getValue() const { return value_; }
        void accept(ASTVisitor &visitor) override;

    private:
        bool value_;
    };

    class NilExpr : public Expression
    {
    public:
        explicit NilExpr(int line, int col) : Expression(NIL_EXPR, line, col) {}
        void accept(ASTVisitor &visitor) override;
    };

    class IdentifierExpr : public Expression
    {
    public:
        explicit IdentifierExpr(const std::string &name, int line, int col) : Expression(IDENTIFIER_EXPR, line, col), name_(name) {}
        const std::string &getName() const { return name_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::string name_;
    };

    class ThisExpr : public Expression
    {
    public:
        ThisExpr(int line, int col) : Expression(THIS_EXPR, line, col) {}
        void accept(ASTVisitor &visitor) override;
    };

    class BinaryExpr : public Expression
    {
    public:
        BinaryExpr(std::unique_ptr<Expression> left, const std::string &op, std::unique_ptr<Expression> right, int line, int col)
            : Expression(BINARY_EXPR, line, col), left_(std::move(left)), op_(op), right_(std::move(right)) {}
        Expression *getLeft() const { return left_.get(); }
        const std::string &getOperator() const { return op_; }
        Expression *getRight() const { return right_.get(); }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Expression> left_;
        std::string op_;
        std::unique_ptr<Expression> right_;
    };

    class UnaryExpr : public Expression
    {
    public:
        UnaryExpr(const std::string &op, std::unique_ptr<Expression> operand, int line, int col)
            : Expression(UNARY_EXPR, line, col), op_(op), operand_(std::move(operand)) {}
        const std::string &getOperator() const { return op_; }
        Expression *getOperand() const { return operand_.get(); }
        void accept(ASTVisitor &visitor) override;

    private:
        std::string op_;
        std::unique_ptr<Expression> operand_;
    };

    class CallExpr : public Expression
    {
    public:
        CallExpr(std::unique_ptr<Expression> callee, std::vector<std::unique_ptr<Expression>> args, int line, int col)
            : Expression(CALL_EXPR, line, col), callee_(std::move(callee)), args_(std::move(args)) {}
        Expression *getCallee() const { return callee_.get(); }
        const std::vector<std::unique_ptr<Expression>> &getArgs() const { return args_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Expression> callee_;
        std::vector<std::unique_ptr<Expression>> args_;
    };

    class MemberAccessExpr : public Expression
    {
    public:
        MemberAccessExpr(std::unique_ptr<Expression> object, const std::string &member, int line, int col)
            : Expression(MEMBER_ACCESS_EXPR, line, col), object_(std::move(object)), member_(member) {}
        Expression *getObject() const { return object_.get(); }
        const std::string &getMember() const { return member_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Expression> object_;
        std::string member_;
    };

    class ListExpr : public Expression
    {
    public:
        explicit ListExpr(std::vector<std::unique_ptr<Expression>> elements, int line, int col)
            : Expression(LIST_EXPR, line, col), elements_(std::move(elements)) {}
        const std::vector<std::unique_ptr<Expression>> &getElements() const { return elements_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::vector<std::unique_ptr<Expression>> elements_;
    };

    class TableExpr : public Expression
    {
    public:
        using Entry = std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>;
        explicit TableExpr(std::vector<Entry> entries, int line, int col)
            : Expression(TABLE_EXPR, line, col), entries_(std::move(entries)) {}
        const std::vector<Entry> &getEntries() const { return entries_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::vector<Entry> entries_;
    };

    class IndexExpr : public Expression
    {
    public:
        IndexExpr(std::unique_ptr<Expression> object, std::unique_ptr<Expression> index, int line, int col)
            : Expression(INDEX_EXPR, line, col), object_(std::move(object)), index_(std::move(index)) {}
        Expression *getObject() const { return object_.get(); }
        Expression *getIndex() const { return index_.get(); }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Expression> object_;
        std::unique_ptr<Expression> index_;
    };

    class SliceExpr : public Expression
    {
    public:
        SliceExpr(std::unique_ptr<Expression> object, std::unique_ptr<Expression> start, std::unique_ptr<Expression> end, int line, int col)
            : Expression(SLICE_EXPR, line, col), object_(std::move(object)), start_(std::move(start)), end_(std::move(end)) {}
        Expression *getObject() const { return object_.get(); }
        Expression *getStart() const { return start_.get(); }
        Expression *getEnd() const { return end_.get(); }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Expression> object_;
        std::unique_ptr<Expression> start_;
        std::unique_ptr<Expression> end_;
    };

    class TernaryExpr : public Expression
    {
    public:
        TernaryExpr(std::unique_ptr<Expression> condition, std::unique_ptr<Expression> thenExpr, std::unique_ptr<Expression> elseExpr, int line, int col)
            : Expression(TERNARY_EXPR, line, col), condition_(std::move(condition)), thenExpr_(std::move(thenExpr)), elseExpr_(std::move(elseExpr)) {}
        Expression *getCondition() const { return condition_.get(); }
        Expression *getThenExpr() const { return thenExpr_.get(); }
        Expression *getElseExpr() const { return elseExpr_.get(); }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Expression> condition_;
        std::unique_ptr<Expression> thenExpr_;
        std::unique_ptr<Expression> elseExpr_;
    };

    class FunctionExpr : public Expression
    {
    public:
        FunctionExpr(const std::string &name, std::vector<std::string> params,
                     std::vector<std::unique_ptr<Statement>> body, int line, int col)
            : Expression(FUNCTION_EXPR, line, col), name_(name), params_(std::move(params)), body_(std::move(body)) {}
        const std::string &getName() const { return name_; }
        const std::vector<std::string> &getParams() const { return params_; }
        const std::vector<std::unique_ptr<Statement>> &getBody() const { return body_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::string name_;
        std::vector<std::string> params_;
        std::vector<std::unique_ptr<Statement>> body_;
    };

    /**
     * --- Concrete Statement Nodes ---
     */

    class VarDeclStmt : public Statement
    {
    public:
        VarDeclStmt(const std::string &name, std::unique_ptr<Expression> initializer, int line, int col)
            : Statement(VAR_DECL_STMT, line, col), name_(name), initializer_(std::move(initializer)) {}
        const std::string &getName() const { return name_; }
        Expression *getInitializer() const { return initializer_.get(); }
        void accept(ASTVisitor &visitor) override;

    private:
        std::string name_;
        std::unique_ptr<Expression> initializer_;
    };

    class AssignStmt : public Statement
    {
    public:
        AssignStmt(std::unique_ptr<Expression> target, std::unique_ptr<Expression> value, int line, int col)
            : Statement(ASSIGN_STMT, line, col), target_(std::move(target)), value_(std::move(value)) {}
        Expression *getTarget() const { return target_.get(); } // Can be IdentifierExpr or other l-values
        Expression *getValue() const { return value_.get(); }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Expression> target_;
        std::unique_ptr<Expression> value_;
    };

    class IfStmt : public Statement
    {
    public:
        IfStmt(std::unique_ptr<Expression> condition, std::vector<std::unique_ptr<Statement>> thenBranch,
               std::vector<std::unique_ptr<Statement>> elseBranch, int line, int col)
            : Statement(IF_STMT, line, col), condition_(std::move(condition)),
              thenBranch_(std::move(thenBranch)), elseBranch_(std::move(elseBranch)) {}
        Expression *getCondition() const { return condition_.get(); }
        const std::vector<std::unique_ptr<Statement>> &getThenBranch() const { return thenBranch_; }
        const std::vector<std::unique_ptr<Statement>> &getElseBranch() const { return elseBranch_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Expression> condition_;
        std::vector<std::unique_ptr<Statement>> thenBranch_;
        std::vector<std::unique_ptr<Statement>> elseBranch_;
    };

    class WhileStmt : public Statement
    {
    public:
        WhileStmt(std::unique_ptr<Expression> condition, std::vector<std::unique_ptr<Statement>> body, int line, int col)
            : Statement(WHILE_STMT, line, col), condition_(std::move(condition)), body_(std::move(body)) {}
        Expression *getCondition() const { return condition_.get(); }
        const std::vector<std::unique_ptr<Statement>> &getBody() const { return body_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Expression> condition_;
        std::vector<std::unique_ptr<Statement>> body_;
    };

    class ForStmt : public Statement
    {
    public:
        ForStmt(std::unique_ptr<Statement> initializer, std::unique_ptr<Expression> condition,
                std::unique_ptr<Statement> increment, std::vector<std::unique_ptr<Statement>> body, int line, int col)
            : Statement(FOR_STMT, line, col), initializer_(std::move(initializer)),
              condition_(std::move(condition)), increment_(std::move(increment)),
              body_(std::move(body)) {}
        Statement *getInitializer() const { return initializer_.get(); }
        Expression *getCondition() const { return condition_.get(); }
        Statement *getIncrement() const { return increment_.get(); }
        const std::vector<std::unique_ptr<Statement>> &getBody() const { return body_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Statement> initializer_;
        std::unique_ptr<Expression> condition_;
        std::unique_ptr<Statement> increment_;
        std::vector<std::unique_ptr<Statement>> body_;
    };

    class ForEachStmt : public Statement
    {
    public:
        ForEachStmt(const std::string &varName, std::unique_ptr<Expression> iterable, std::vector<std::unique_ptr<Statement>> body, int line, int col)
            : Statement(FOR_EACH_STMT, line, col), varName_(varName), iterable_(std::move(iterable)), body_(std::move(body)) {}
        const std::string &getVarName() const { return varName_; }
        Expression *getIterable() const { return iterable_.get(); }
        const std::vector<std::unique_ptr<Statement>> &getBody() const { return body_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::string varName_;
        std::unique_ptr<Expression> iterable_;
        std::vector<std::unique_ptr<Statement>> body_;
    };

    class ReturnStmt : public Statement
    {
    public:
        explicit ReturnStmt(std::unique_ptr<Expression> value, int line, int col)
            : Statement(RETURN_STMT, line, col), value_(std::move(value)) {}
        Expression *getValue() const { return value_.get(); }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Expression> value_;
    };

    class ExpressionStmt : public Statement
    {
    public:
        explicit ExpressionStmt(std::unique_ptr<Expression> expr, int line, int col)
            : Statement(EXPRESSION_STMT, line, col), expr_(std::move(expr)) {}
        Expression *getExpression() const { return expr_.get(); }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Expression> expr_;
    };

    class FunctionDeclStmt : public Statement
    {
    public:
        FunctionDeclStmt(const std::string &name, std::vector<std::string> params,
                         std::vector<std::unique_ptr<Statement>> body, int line, int col)
            : Statement(FUNCTION_DECL_STMT, line, col), name_(name), params_(std::move(params)), body_(std::move(body)) {}
        const std::string &getName() const { return name_; }
        const std::vector<std::string> &getParams() const { return params_; }
        const std::vector<std::unique_ptr<Statement>> &getBody() const { return body_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::string name_;
        std::vector<std::string> params_;
        std::vector<std::unique_ptr<Statement>> body_;
    };

    class ClassDeclStmt : public Statement
    {
    public:
        ClassDeclStmt(const std::string &name, std::vector<std::unique_ptr<FunctionDeclStmt>> methods, int line, int col)
            : Statement(CLASS_DECL_STMT, line, col), name_(name), methods_(std::move(methods)) {}
        const std::string &getName() const { return name_; }
        const std::vector<std::unique_ptr<FunctionDeclStmt>> &getMethods() const { return methods_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::string name_;
        std::vector<std::unique_ptr<FunctionDeclStmt>> methods_;
    };

    class ImportStmt : public Statement
    {
    public:
        explicit ImportStmt(const std::string &moduleName, int line, int col) : Statement(IMPORT_STMT, line, col), moduleName_(moduleName) {}
        const std::string &getModuleName() const { return moduleName_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::string moduleName_;
    };

    class FromImportStmt : public Statement
    {
    public:
        FromImportStmt(const std::string &moduleName, const std::vector<std::string> &symbols, int line, int col)
            : Statement(FROM_IMPORT_STMT, line, col), moduleName_(moduleName), symbols_(symbols) {}
        const std::string &getModuleName() const { return moduleName_; }
        const std::vector<std::string> &getSymbols() const { return symbols_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::string moduleName_;
        std::vector<std::string> symbols_;
    };

    class ExportStmt : public Statement
    {
    public:
        explicit ExportStmt(std::unique_ptr<Statement> stmt, int line, int col)
            : Statement(EXPORT_STMT, line, col), stmt_(std::move(stmt)) {}
        Statement *getStmt() const { return stmt_.get(); }
        void accept(ASTVisitor &visitor) override;

    private:
        std::unique_ptr<Statement> stmt_;
    };

    /**
     * The main program node, a list of statements
     */
    class Program : public ASTNode
    {
    public:
        Program() : ASTNode(PROGRAM, 1, 1) {}
        void addStatement(std::unique_ptr<Statement> stmt) { statements_.push_back(std::move(stmt)); }
        const std::vector<std::unique_ptr<Statement>> &getStatements() const { return statements_; }
        void accept(ASTVisitor &visitor) override;

    private:
        std::vector<std::unique_ptr<Statement>> statements_;
    };

    /**
     * Visitor interface for traversing the AST
     */
    class ASTVisitor
    {
    public:
        virtual ~ASTVisitor() = default;

        /**
         * Expressions
         */
        virtual void visit(NumberExpr &expr) = 0;
        virtual void visit(StringExpr &expr) = 0;
        virtual void visit(IdentifierExpr &expr) = 0;
        virtual void visit(class BooleanExpr &expr) = 0; // Added
        virtual void visit(class NilExpr &expr) = 0;     // Added
        virtual void visit(ThisExpr &expr) = 0; // Added
        virtual void visit(BinaryExpr &expr) = 0;
        virtual void visit(UnaryExpr &expr) = 0;
        virtual void visit(CallExpr &expr) = 0;
        virtual void visit(MemberAccessExpr &expr) = 0; // New visitor method
        virtual void visit(ListExpr &expr) = 0;
        virtual void visit(TableExpr &expr) = 0;
        virtual void visit(IndexExpr &expr) = 0;
        virtual void visit(SliceExpr &expr) = 0; // Added
        virtual void visit(TernaryExpr &expr) = 0;
        virtual void visit(FunctionExpr &expr) = 0;

        /**
         * Statements
         */
        virtual void visit(VarDeclStmt &stmt) = 0;
        virtual void visit(AssignStmt &stmt) = 0;
        virtual void visit(IfStmt &stmt) = 0;
        virtual void visit(WhileStmt &stmt) = 0;
        virtual void visit(ForStmt &stmt) = 0;
        virtual void visit(ForEachStmt &stmt) = 0;
        virtual void visit(ReturnStmt &stmt) = 0;
        virtual void visit(ExpressionStmt &stmt) = 0;
        virtual void visit(FunctionDeclStmt &stmt) = 0;
        virtual void visit(ClassDeclStmt &stmt) = 0; // Added
        virtual void visit(ImportStmt &stmt) = 0;
        virtual void visit(FromImportStmt &stmt) = 0;
        virtual void visit(ExportStmt &stmt) = 0;

        /**
         * Program
         */
        virtual void visit(Program &program) = 0;
    };

    inline void NumberExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void StringExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void BooleanExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void NilExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void IdentifierExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void ThisExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); } // Added
    inline void BinaryExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void UnaryExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void CallExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void MemberAccessExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); } // New accept method
    inline void ListExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void TableExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void IndexExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void SliceExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); } // Added
    inline void TernaryExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void FunctionExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }

    inline void VarDeclStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void AssignStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void IfStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void WhileStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void ForStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void ForEachStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void ReturnStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void ExpressionStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void FunctionDeclStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void ClassDeclStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); } // Added
    inline void ImportStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void FromImportStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
    inline void ExportStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

    inline void Program::accept(ASTVisitor &visitor) { visitor.visit(*this); }

} // namespace Pome

#endif // POME_AST_H
