#ifndef POME_COMPILER_H
#define POME_COMPILER_H

#include "pome_ast.h"
#include "pome_chunk.h"
#include "pome_gc.h" 
#include <vector>
#include <map>
#include <string>

namespace Pome {

    class Compiler : public ASTVisitor {
    public:
        Compiler(GarbageCollector& gc);
        std::unique_ptr<Chunk> compile(Program& program);
        
        // Visitor implementation
        void visit(NumberExpr &expr) override;
        void visit(StringExpr &expr) override;
        void visit(BooleanExpr &expr) override;
        void visit(NilExpr &expr) override;
        void visit(IdentifierExpr &expr) override;
        void visit(BinaryExpr &expr) override;
        void visit(CallExpr &expr) override;
        void visit(MemberAccessExpr& expr) override;
        void visit(VarDeclStmt &stmt) override;
        void visit(ExpressionStmt &stmt) override;
        void visit(AssignStmt& stmt) override;
        void visit(IfStmt& stmt) override;
        void visit(WhileStmt& stmt) override;
        void visit(Program &program) override;
        void visit(FunctionDeclStmt& stmt) override;
        void visit(ClassDeclStmt& stmt) override;
        void visit(ReturnStmt& stmt) override;
        void visit(ThisExpr& expr) override;
        void visit(ForStmt& stmt) override;
        void visit(UnaryExpr& expr) override;
        void visit(ListExpr& expr) override;
        void visit(TableExpr& expr) override;
        void visit(IndexExpr& expr) override;
        void visit(ImportStmt& stmt) override;
        void visit(FromImportStmt& stmt) override;
        void visit(ExportStmt& stmt) override;
        void visit(ExportExpressionStmt& stmt) override;
        void visit(ForEachStmt& stmt) override;
        void visit(BlockStmt& stmt) override; // Added
        void visit(SliceExpr& expr) override;
        void visit(TernaryExpr& expr) override;
        void visit(FunctionExpr& expr) override;

    private:
        Chunk* currentChunk; 
        int freeReg = 0;
        
        struct Local {
            std::string name;
            int depth;
            int reg;
        };
        
        std::vector<Local> locals;
        int scopeDepth = 0;
        
        int resolveLocal(const std::string& name);
        int addConstant(PomeValue value);
        void emit(Instruction instruction, int line);
        
        int emitJump(OpCode op);
        void patchJump(int instructionIndex);
        void resetFreeReg();
        
        int lastResultReg = -1; 
        bool strictMode = false;
        
        int allocReg() { return freeReg++; }
        void freeRegs(int n) { freeReg -= n; }
        
        GarbageCollector& gc;
    };

}

#endif // POME_COMPILER_H
