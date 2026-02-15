#include "../include/pome_compiler.h"
#include <iostream>

namespace Pome {

    Compiler::Compiler(GarbageCollector& gc, Compiler* parent) : gc(gc), currentChunk(nullptr), parent(parent) {}

    std::unique_ptr<Chunk> Compiler::compile(Program& program) {
        auto scriptChunk = std::make_unique<Chunk>();
        currentChunk = scriptChunk.get();
        freeReg = 0;
        locals.clear();
        upvalues.clear();
        scopeDepth = 0;
        
        program.accept(*this);
        
        emit(Chunk::makeABC(OpCode::RETURN, 0, 1, 0), 0); 
        return scriptChunk;
    }

    void Compiler::emit(Instruction instruction, int line) {
        currentChunk->write(instruction, line);
    }

    int Compiler::addConstant(PomeValue value) {
        return currentChunk->addConstant(value);
    }

    int Compiler::emitJump(OpCode op) {
        emit(Chunk::makeAsBx(op, 0, 0), 0);
        return currentChunk->code.size() - 1;
    }

    void Compiler::patchJump(int instructionIndex) {
        int offset = currentChunk->code.size() - instructionIndex - 1;
        Instruction& instr = currentChunk->code[instructionIndex];
        OpCode op = Chunk::getOpCode(instr);
        int a = Chunk::getA(instr);
        instr = Chunk::makeAsBx(op, a, offset);
    }

    int Compiler::resolveUpvalue(const std::string& name) {
        if (parent == nullptr) return -1;

        for (int i = (int)parent->locals.size() - 1; i >= 0; i--) {
            if (parent->locals[i].name == name) {
                parent->locals[i].isCaptured = true;
                return addUpvalue((uint8_t)parent->locals[i].reg, true);
            }
        }

        int upvalue = parent->resolveUpvalue(name);
        if (upvalue != -1) {
            return addUpvalue((uint8_t)upvalue, false);
        }

        return -1;
    }

    int Compiler::addUpvalue(uint8_t index, bool isLocal) {
        int count = upvalues.size();
        for (int i = 0; i < count; i++) {
            Upvalue* upvalue = &upvalues[i];
            if (upvalue->index == index && upvalue->isLocal == isLocal) {
                return i;
            }
        }

        upvalues.push_back({index, isLocal});
        return count;
    }

    // ... visitors ...

    void Compiler::visit(FunctionDeclStmt &stmt) {
        // 1. Create function object
        PomeFunction* func = gc.allocate<PomeFunction>();
        func->name = stmt.getName();
        func->parameters = stmt.getParams();
        
        // 2. Compile function body into a new chunk using a separate compiler
        Compiler innerCompiler(gc, this);
        innerCompiler.currentChunk = func->chunk.get();
        innerCompiler.strictMode = strictMode;
        
        // R0 is reserved for the function itself
        innerCompiler.allocReg(); 
        
        // Bind parameters to registers R1, R2...
        for (const auto& param : func->parameters) {
            innerCompiler.locals.push_back({param, 0, innerCompiler.allocReg()});
        }
        
        for (const auto& s : stmt.getBody()) {
            s->accept(innerCompiler);
            innerCompiler.resetFreeReg();
        }
        
        innerCompiler.emit(Chunk::makeABC(OpCode::RETURN, 0, 1, 0), stmt.getLine());
        
        func->upvalueCount = (uint16_t)innerCompiler.upvalues.size();

        // 3. Emit CLOSURE and SETGLOBAL in CURRENT chunk
        int reg = allocReg();
        int constIdx = addConstant(PomeValue(func));
        emit(Chunk::makeABx(OpCode::CLOSURE, reg, constIdx), stmt.getLine());
        for (const auto& uv : innerCompiler.upvalues) {
            emit(Chunk::makeABC(uv.isLocal ? OpCode::MOVE : OpCode::GETUPVAL, 0, uv.index, 0), stmt.getLine());
        }
        
        PomeString* nameStr = gc.allocate<PomeString>(stmt.getName());
        int nameIdx = addConstant(PomeValue(nameStr));
        emit(Chunk::makeABx(OpCode::SETGLOBAL, reg, nameIdx), stmt.getLine());
        lastResultReg = reg;
    }

    int Compiler::resolveLocal(const std::string& name) {
        for (int i = (int)locals.size() - 1; i >= 0; i--) {
            if (locals[i].name == name) {
                return locals[i].reg;
            }
        }
        return -1;
    }

    void Compiler::resetFreeReg() {
        int maxReg = -1;
        for (const auto& local : locals) {
            if (local.reg > maxReg) maxReg = local.reg;
        }
        freeReg = maxReg + 1;
    }

    // --- Visitors ---
    
    void Compiler::visit(Program &program) {
        strictMode = program.isStrict;
        for (const auto &stmt : program.getStatements()) {
            stmt->accept(*this);
            resetFreeReg();
        }
    }
    
    void Compiler::visit(IfStmt &stmt) {
        stmt.getCondition()->accept(*this);
        int condReg = lastResultReg;
        
        // Skip next JMP if truthy
        emit(Chunk::makeABC(OpCode::TEST, condReg, 0, 1), stmt.getLine());
        
        int jumpToElse = emitJump(OpCode::JMP);
        
        for (const auto& s : stmt.getThenBranch()) {
            s->accept(*this);
            resetFreeReg();
        }
        
        int jumpToEnd = emitJump(OpCode::JMP);
        
        patchJump(jumpToElse);
        for (const auto& s : stmt.getElseBranch()) {
            s->accept(*this);
            resetFreeReg();
        }
        
        patchJump(jumpToEnd);
    }
    
    void Compiler::visit(WhileStmt &stmt) {
        int loopStart = currentChunk->code.size();
        
        stmt.getCondition()->accept(*this);
        int condReg = lastResultReg;
        
        // Skip next JMP if truthy
        emit(Chunk::makeABC(OpCode::TEST, condReg, 0, 1), stmt.getLine());
        int jumpToEnd = emitJump(OpCode::JMP);
        
        for (const auto& s : stmt.getBody()) {
            s->accept(*this);
            resetFreeReg();
        }
        
        int offset = loopStart - (int)currentChunk->code.size() - 1;
        emit(Chunk::makeAsBx(OpCode::JMP, 0, offset), stmt.getLine());
        
        patchJump(jumpToEnd);
    }

    void Compiler::visit(NumberExpr &expr) {
        int reg = allocReg();
        int constIdx = addConstant(PomeValue(expr.getValue()));
        emit(Chunk::makeABx(OpCode::LOADK, reg, constIdx), expr.getLine());
        lastResultReg = reg;
    }
    
    void Compiler::visit(StringExpr &expr) {
        int reg = allocReg();
        PomeString* s = gc.allocate<PomeString>(expr.getValue());
        int constIdx = addConstant(PomeValue(s));
        emit(Chunk::makeABx(OpCode::LOADK, reg, constIdx), expr.getLine());
        lastResultReg = reg;
    }

    void Compiler::visit(BooleanExpr &expr) {
        int reg = allocReg();
        emit(Chunk::makeABC(OpCode::LOADBOOL, reg, expr.getValue() ? 1 : 0, 0), expr.getLine());
        lastResultReg = reg;
    }

    void Compiler::visit(NilExpr &expr) {
        int reg = allocReg();
        emit(Chunk::makeABC(OpCode::LOADNIL, reg, 0, 0), expr.getLine());
        lastResultReg = reg;
    }

    void Compiler::visit(IdentifierExpr &expr) {
        int reg = resolveLocal(expr.getName());
        if (reg != -1) {
            int dest = allocReg();
            emit(Chunk::makeABC(OpCode::MOVE, dest, reg, 0), expr.getLine());
            lastResultReg = dest;
        } else if ((reg = resolveUpvalue(expr.getName())) != -1) {
            int dest = allocReg();
            emit(Chunk::makeABC(OpCode::GETUPVAL, dest, reg, 0), expr.getLine());
            lastResultReg = dest;
        } else {
            // Global Read
            int dest = allocReg();
            
            PomeString* nameStr = gc.allocate<PomeString>(expr.getName());
            int nameIdx = addConstant(PomeValue(nameStr));
            
            emit(Chunk::makeABx(OpCode::GETGLOBAL, dest, nameIdx), expr.getLine());
            lastResultReg = dest;
        }
    }

    void Compiler::visit(BinaryExpr &expr) {
        std::string oper = expr.getOperator();
        
        if (oper == "and") {
            expr.getLeft()->accept(*this);
            int leftReg = lastResultReg;
            
            int resReg = allocReg();
            emit(Chunk::makeABC(OpCode::MOVE, resReg, leftReg, 0), expr.getLine());
            
            // Skip next JMP if truthy
            emit(Chunk::makeABC(OpCode::TEST, resReg, 0, 1), expr.getLine());
            int jumpToEnd = emitJump(OpCode::JMP);
            
            expr.getRight()->accept(*this);
            emit(Chunk::makeABC(OpCode::MOVE, resReg, lastResultReg, 0), expr.getLine());
            
            patchJump(jumpToEnd);
            lastResultReg = resReg;
            return;
        }

        if (oper == "or") {
            expr.getLeft()->accept(*this);
            int leftReg = lastResultReg;
            
            int resReg = allocReg();
            emit(Chunk::makeABC(OpCode::MOVE, resReg, leftReg, 0), expr.getLine());
            
            // Skip next JMP if falsy
            emit(Chunk::makeABC(OpCode::TEST, resReg, 0, 0), expr.getLine());
            int jumpToEnd = emitJump(OpCode::JMP);
            
            expr.getRight()->accept(*this);
            emit(Chunk::makeABC(OpCode::MOVE, resReg, lastResultReg, 0), expr.getLine());
            
            patchJump(jumpToEnd);
            lastResultReg = resReg;
            return;
        }

        if (oper == "=") {
            // Assignment Expression
            if (auto ident = dynamic_cast<IdentifierExpr*>(expr.getLeft())) {
                // Compile RHS
                expr.getRight()->accept(*this);
                int valReg = lastResultReg;
                
                int localReg = resolveLocal(ident->getName());
                if (localReg != -1) {
                    emit(Chunk::makeABC(OpCode::MOVE, localReg, valReg, 0), expr.getLine());
                    lastResultReg = localReg;
                } else if ((localReg = resolveUpvalue(ident->getName())) != -1) {
                    emit(Chunk::makeABC(OpCode::SETUPVAL, valReg, localReg, 0), expr.getLine());
                    lastResultReg = valReg;
                } else {
                    if (strictMode) {
                        std::cerr << "Compiler Error: Undefined variable '" << ident->getName() << "' in strict mode." << std::endl;
                        exit(1);
                    }
                    // Global assignment
                    PomeString* nameStr = gc.allocate<PomeString>(ident->getName());
                    int nameIdx = addConstant(PomeValue(nameStr));
                    emit(Chunk::makeABx(OpCode::SETGLOBAL, valReg, nameIdx), expr.getLine());
                }
            } else if (auto member = dynamic_cast<MemberAccessExpr*>(expr.getLeft())) {
                // obj.member = value
                member->getObject()->accept(*this);
                int objReg = lastResultReg;
                
                expr.getRight()->accept(*this);
                int valReg = lastResultReg;
                
                PomeString* keyStr = gc.allocate<PomeString>(member->getMember());
                int keyIdx = addConstant(PomeValue(keyStr));
                int keyReg = allocReg();
                emit(Chunk::makeABx(OpCode::LOADK, keyReg, keyIdx), expr.getLine());
                
                emit(Chunk::makeABC(OpCode::SETTABLE, objReg, keyReg, valReg), expr.getLine());
                lastResultReg = valReg;
            } else {
                std::cerr << "Compiler Error: Invalid assignment target." << std::endl;
                exit(1);
            }
            return;
        }

        expr.getLeft()->accept(*this);
        int leftReg = lastResultReg;
        
        expr.getRight()->accept(*this);
        int rightReg = lastResultReg;
        
        freeRegs(2); 
        int resReg = allocReg();
        
        OpCode op = OpCode::ADD;
        bool swap = false;
        bool invert = false;

        if (oper == "+") op = OpCode::ADD;
        else if (oper == "-") op = OpCode::SUB;
        else if (oper == "*") op = OpCode::MUL;
        else if (oper == "/") op = OpCode::DIV;
        else if (oper == "%") op = OpCode::MOD;
        else if (oper == "^") op = OpCode::POW;
        else if (oper == "<") op = OpCode::LT;
        else if (oper == "<=") op = OpCode::LE;
        else if (oper == ">") { op = OpCode::LT; swap = true; }
        else if (oper == ">=") { op = OpCode::LE; swap = true; }
        else if (oper == "==") op = OpCode::EQ;
        else if (oper == "!=") { op = OpCode::EQ; invert = true; }
        
        if (swap) {
            emit(Chunk::makeABC(op, resReg, rightReg, leftReg), expr.getLine());
        } else {
            emit(Chunk::makeABC(op, resReg, leftReg, rightReg), expr.getLine());
        }
        
        if (invert) {
            emit(Chunk::makeABC(OpCode::NOT, resReg, resReg, 0), expr.getLine());
        }

        lastResultReg = resReg;
    }

    void Compiler::visit(VarDeclStmt &stmt) {
        if (stmt.getInitializer()) {
            stmt.getInitializer()->accept(*this);
        } else {
            int reg = allocReg();
            emit(Chunk::makeABC(OpCode::LOADNIL, reg, 0, 0), stmt.getLine());
            lastResultReg = reg;
        }
        
        locals.push_back({stmt.getName(), scopeDepth, lastResultReg});
    }
    
    void Compiler::visit(AssignStmt &stmt) {
        stmt.getValue()->accept(*this);
        int valReg = lastResultReg;
        
        // Save valReg if we are going to compile something else
        int savedValReg = allocReg();
        emit(Chunk::makeABC(OpCode::MOVE, savedValReg, valReg, 0), stmt.getLine());

        if (auto ident = dynamic_cast<IdentifierExpr*>(stmt.getTarget())) {
            int localReg = resolveLocal(ident->getName());
            if (localReg != -1) {
                emit(Chunk::makeABC(OpCode::MOVE, localReg, savedValReg, 0), stmt.getLine());
                lastResultReg = localReg; 
            } else if ((localReg = resolveUpvalue(ident->getName())) != -1) {
                emit(Chunk::makeABC(OpCode::SETUPVAL, savedValReg, localReg, 0), stmt.getLine());
                lastResultReg = savedValReg;
            } else {
                if (strictMode) {
                    std::cerr << "Compiler Error: Undefined variable '" << ident->getName() << "' in strict mode." << std::endl;
                    exit(1);
                }
                PomeString* nameStr = gc.allocate<PomeString>(ident->getName());
                int nameIdx = addConstant(PomeValue(nameStr));
                emit(Chunk::makeABx(OpCode::SETGLOBAL, savedValReg, nameIdx), stmt.getLine());
            }
        } else if (auto member = dynamic_cast<MemberAccessExpr*>(stmt.getTarget())) {
            member->getObject()->accept(*this);
            int objReg = lastResultReg;
            
            PomeString* keyStr = gc.allocate<PomeString>(member->getMember());
            int keyIdx = addConstant(PomeValue(keyStr));
            int keyReg = allocReg();
            emit(Chunk::makeABx(OpCode::LOADK, keyReg, keyIdx), stmt.getLine());
            
            emit(Chunk::makeABC(OpCode::SETTABLE, objReg, keyReg, savedValReg), stmt.getLine());
        } else if (auto index = dynamic_cast<IndexExpr*>(stmt.getTarget())) {
            index->getObject()->accept(*this);
            int objReg = lastResultReg;
            
            index->getIndex()->accept(*this);
            int keyReg = lastResultReg;
            
            emit(Chunk::makeABC(OpCode::SETTABLE, objReg, keyReg, savedValReg), stmt.getLine());
        } else {
             std::cerr << "Compiler Error: Unsupported assignment target." << std::endl;
        }
        freeRegs(1); // Free savedValReg
    }

    void Compiler::visit(ExpressionStmt &stmt) {
        stmt.getExpression()->accept(*this);
    }

    void Compiler::visit(MemberAccessExpr &expr) {
        expr.getObject()->accept(*this);
        int objReg = lastResultReg;
        
        PomeString* memberStr = gc.allocate<PomeString>(expr.getMember());
        int memberIdx = addConstant(PomeValue(memberStr));
        
        int dest = allocReg();
        // Since RK(C) logic is not yet in Compiler, we LOADK the key first.
        int keyReg = allocReg();
        emit(Chunk::makeABx(OpCode::LOADK, keyReg, memberIdx), expr.getLine());
        
        emit(Chunk::makeABC(OpCode::GETTABLE, dest, objReg, keyReg), expr.getLine());
        lastResultReg = dest;
    }

    void Compiler::visit(CallExpr &expr) {
        // Native PRINT hack for speed
        if (auto ident = dynamic_cast<IdentifierExpr*>(expr.getCallee())) {
            if (ident->getName() == "print") {
                int baseReg = allocReg();
                int argCount = expr.getArgs().size();
                // Ensure we have enough registers for all args
                for (int i = 1; i < argCount; ++i) allocReg();

                for (int i = 0; i < argCount; ++i) {
                    expr.getArgs()[i]->accept(*this);
                    emit(Chunk::makeABC(OpCode::MOVE, baseReg + i, lastResultReg, 0), expr.getLine());
                }
                emit(Chunk::makeABC(OpCode::PRINT, baseReg, argCount, 0), expr.getLine());
                
                // Free all registers used for print args
                freeRegs(argCount);

                int reg = allocReg();
                emit(Chunk::makeABC(OpCode::LOADNIL, reg, 0, 0), expr.getLine());
                lastResultReg = reg;
                return;
            }
        }

        if (auto member = dynamic_cast<MemberAccessExpr*>(expr.getCallee())) {
            // Method Call: obj.method(args)
            member->getObject()->accept(*this);
            int objReg = lastResultReg;
            
            int calleeReg = allocReg();
            
            PomeString* keyStr = gc.allocate<PomeString>(member->getMember());
            int keyIdx = addConstant(PomeValue(keyStr));
            int keyReg = allocReg();
            emit(Chunk::makeABx(OpCode::LOADK, keyReg, keyIdx), expr.getLine());
            
            emit(Chunk::makeABC(OpCode::GETTABLE, calleeReg, objReg, keyReg), expr.getLine());
            
            // Push 'this' (objReg) as first argument (R1)
            emit(Chunk::makeABC(OpCode::MOVE, calleeReg + 1, objReg, 0), expr.getLine());
            
            // Push other args
            int argCount = expr.getArgs().size();
            for (int i = 0; i < argCount; ++i) {
                expr.getArgs()[i]->accept(*this);
                emit(Chunk::makeABC(OpCode::MOVE, calleeReg + 2 + i, lastResultReg, 0), expr.getLine());
            }
            
            emit(Chunk::makeABC(OpCode::CALL, calleeReg, argCount + 2, 1), expr.getLine());
            lastResultReg = calleeReg;
            return;
        }

        // General Call
        expr.getCallee()->accept(*this);
        int calleeReg = lastResultReg;
        
        int argCount = expr.getArgs().size();
        for (int i = 0; i < argCount; ++i) {
            expr.getArgs()[i]->accept(*this);
            // Move arg to calleeReg + 1 + i
            emit(Chunk::makeABC(OpCode::MOVE, calleeReg + 1 + i, lastResultReg, 0), expr.getLine());
        }
        
        emit(Chunk::makeABC(OpCode::CALL, calleeReg, argCount + 1, 1), expr.getLine());
        lastResultReg = calleeReg;
    }

    void Compiler::visit(ClassDeclStmt &stmt) {
        PomeClass* klass = gc.allocate<PomeClass>(stmt.getName());
        
        for (const auto& method : stmt.getMethods()) {
            PomeFunction* func = gc.allocate<PomeFunction>();
            func->name = method->getName();
            func->parameters = method->getParams();
            
            Chunk* prevChunk = currentChunk;
            int prevFreeReg = freeReg;
            auto prevLocals = locals;
            
            currentChunk = func->chunk.get();
            freeReg = 0;
            locals.clear();
            
            // Methods: R0 = Function, R1 = 'this', R2... = Params
            allocReg(); // Skip R0
            locals.push_back({"this", 0, allocReg()}); // R1
            for (const auto& param : func->parameters) {
                locals.push_back({param, 0, allocReg()});
            }
            
            for (const auto& s : method->getBody()) {
                s->accept(*this);
                resetFreeReg();
            }
            if (func->name == "init") {
                // Constructors return 'this' (R1)
                emit(Chunk::makeABC(OpCode::RETURN, 1, 2, 0), method->getLine());
            } else {
                emit(Chunk::makeABC(OpCode::RETURN, 0, 1, 0), method->getLine());
            }
            
            currentChunk = prevChunk;
            freeReg = prevFreeReg;
            locals = prevLocals;
            
            klass->methods[func->name] = func;
        }
        
        int reg = allocReg();
        int constIdx = addConstant(PomeValue(klass));
        emit(Chunk::makeABx(OpCode::LOADK, reg, constIdx), stmt.getLine());
        
        PomeString* nameStr = gc.allocate<PomeString>(stmt.getName());
        int nameIdx = addConstant(PomeValue(nameStr));
        emit(Chunk::makeABx(OpCode::SETGLOBAL, reg, nameIdx), stmt.getLine());
        lastResultReg = reg;
    }

    void Compiler::visit(ThisExpr &expr) {
        int reg = resolveLocal("this");
        if (reg != -1) {
            int dest = allocReg();
            emit(Chunk::makeABC(OpCode::MOVE, dest, reg, 0), expr.getLine());
            lastResultReg = dest;
        } else {
            std::cerr << "Compiler Error: Cannot use 'this' outside of a class method." << std::endl;
            exit(1);
        }
    }

    void Compiler::visit(ReturnStmt &stmt) {
        if (stmt.getValue()) {
            stmt.getValue()->accept(*this);
            int valReg = lastResultReg;
            emit(Chunk::makeABC(OpCode::RETURN, valReg, 2, 0), stmt.getLine());
        } else {
            emit(Chunk::makeABC(OpCode::RETURN, 0, 1, 0), stmt.getLine());
        }
    }

    void Compiler::visit(ForStmt &stmt) {
        scopeDepth++;
        
        if (stmt.getInitializer()) {
            stmt.getInitializer()->accept(*this);
        }
        
        int loopStart = currentChunk->code.size();
        
        int jumpToEnd = -1;
        if (stmt.getCondition()) {
            stmt.getCondition()->accept(*this);
            int condReg = lastResultReg;
            // Skip next JMP if truthy
            emit(Chunk::makeABC(OpCode::TEST, condReg, 0, 1), stmt.getLine());
            jumpToEnd = emitJump(OpCode::JMP);
        }
        
        for (const auto& s : stmt.getBody()) {
            s->accept(*this);
            resetFreeReg();
        }
        
        if (stmt.getIncrement()) {
            stmt.getIncrement()->accept(*this);
        }
        
        int offset = loopStart - (int)currentChunk->code.size() - 1;
        emit(Chunk::makeAsBx(OpCode::JMP, 0, offset), stmt.getLine());
        
        if (jumpToEnd != -1) {
            patchJump(jumpToEnd);
        }
        
        // Pop locals from this scope
        while (!locals.empty() && locals.back().depth == scopeDepth) {
            locals.pop_back();
        }
        scopeDepth--;
        resetFreeReg();
    }

    void Compiler::visit(UnaryExpr &expr) {
        expr.getOperand()->accept(*this);
        int operandReg = lastResultReg;
        
        int dest = allocReg();
        OpCode op = OpCode::UNM;
        if (expr.getOperator() == "!") op = OpCode::NOT;
        
        emit(Chunk::makeABC(op, dest, operandReg, 0), expr.getLine());
        lastResultReg = dest;
    }

    void Compiler::visit(ListExpr &expr) {
        int tableReg = allocReg();
        emit(Chunk::makeABC(OpCode::NEWLIST, tableReg, 0, 0), 0); 
        
        const auto& elements = expr.getElements();
        for (size_t i = 0; i < elements.size(); ++i) {
            int keyReg = allocReg();
            int constIdx = addConstant(PomeValue((double)i));
            emit(Chunk::makeABx(OpCode::LOADK, keyReg, constIdx), 0);
            
            elements[i]->accept(*this);
            int valReg = lastResultReg;
            
            emit(Chunk::makeABC(OpCode::SETTABLE, tableReg, keyReg, valReg), 0);
        }
        lastResultReg = tableReg;
    }

    void Compiler::visit(TableExpr &expr) {
        int tableReg = allocReg();
        emit(Chunk::makeABC(OpCode::NEWTABLE, tableReg, 0, 0), 0);
        
        for (const auto& pair : expr.getEntries()) {
            pair.first->accept(*this);
            int keyReg = lastResultReg;
            
            pair.second->accept(*this);
            int valReg = lastResultReg;
            
            emit(Chunk::makeABC(OpCode::SETTABLE, tableReg, keyReg, valReg), 0);
            // resetFreeReg(); // REMOVED
        }
        lastResultReg = tableReg;
    }

    void Compiler::visit(IndexExpr &expr) {
        expr.getObject()->accept(*this);
        int objReg = lastResultReg;
        
        expr.getIndex()->accept(*this);
        int keyReg = lastResultReg;
        
        int dest = allocReg();
        emit(Chunk::makeABC(OpCode::GETTABLE, dest, objReg, keyReg), 0);
        lastResultReg = dest;
    }

    void Compiler::visit(SliceExpr &expr) {
        expr.getObject()->accept(*this);
        int objReg = lastResultReg;
        
        // We need start and end in consecutive registers
        int base = allocReg(); // base for start
        allocReg(); // space for end
        
        if (expr.getStart()) {
            expr.getStart()->accept(*this);
            emit(Chunk::makeABC(OpCode::MOVE, base, lastResultReg, 0), expr.getLine());
        } else {
            int c = addConstant(PomeValue(0.0));
            emit(Chunk::makeABx(OpCode::LOADK, base, c), expr.getLine());
        }
        
        if (expr.getEnd()) {
            expr.getEnd()->accept(*this);
            emit(Chunk::makeABC(OpCode::MOVE, base + 1, lastResultReg, 0), expr.getLine());
        } else {
            emit(Chunk::makeABC(OpCode::LEN, base + 1, objReg, 0), expr.getLine());
        }
        
        int dest = allocReg();
        emit(Chunk::makeABC(OpCode::SLICE, dest, objReg, base), expr.getLine());
        lastResultReg = dest;
    }

    void Compiler::visit(TernaryExpr &expr) {
        expr.getCondition()->accept(*this);
        int condReg = lastResultReg;
        
        emit(Chunk::makeABC(OpCode::TEST, condReg, 0, 1), expr.getLine());
        int jumpToFalse = emitJump(OpCode::JMP);
        
        expr.getThenExpr()->accept(*this);
        int resReg = allocReg();
        emit(Chunk::makeABC(OpCode::MOVE, resReg, lastResultReg, 0), expr.getLine());
        int jumpToEnd = emitJump(OpCode::JMP);
        
        patchJump(jumpToFalse);
        expr.getElseExpr()->accept(*this);
        emit(Chunk::makeABC(OpCode::MOVE, resReg, lastResultReg, 0), expr.getLine());
        
        patchJump(jumpToEnd);
        lastResultReg = resReg;
    }

    void Compiler::visit(FunctionExpr &expr) {
        PomeFunction* func = gc.allocate<PomeFunction>();
        func->name = expr.getName().empty() ? "anonymous" : expr.getName();
        func->parameters = expr.getParams();
        
        // Setup new compiler state for inner function
        Compiler innerCompiler(gc, this);
        innerCompiler.currentChunk = func->chunk.get();
        innerCompiler.strictMode = strictMode;

        // R0
        innerCompiler.allocReg(); 
        for (const auto& param : func->parameters) {
            innerCompiler.locals.push_back({param, 0, innerCompiler.allocReg()});
        }
        
        for (const auto& s : expr.getBody()) {
            s->accept(innerCompiler);
            innerCompiler.resetFreeReg();
        }
        
        innerCompiler.emit(Chunk::makeABC(OpCode::RETURN, 0, 1, 0), expr.getLine());
        
        func->upvalueCount = (uint16_t)innerCompiler.upvalues.size();

        int reg = allocReg();
        int constIdx = addConstant(PomeValue(func));
        
        // Emit CLOSURE R(A) K(Bx)
        emit(Chunk::makeABx(OpCode::CLOSURE, reg, constIdx), expr.getLine());
        
        // Emit upvalue details immediately after CLOSURE
        for (const auto& uv : innerCompiler.upvalues) {
            emit(Chunk::makeABC(uv.isLocal ? OpCode::MOVE : OpCode::GETUPVAL, 0, uv.index, 0), expr.getLine());
        }

        lastResultReg = reg;
    }

    void Compiler::visit(ImportStmt &stmt) {
        PomeString* nameStr = gc.allocate<PomeString>(stmt.getModuleName());
        int nameIdx = addConstant(PomeValue(nameStr));
        int reg = allocReg();
        emit(Chunk::makeABx(OpCode::IMPORT, reg, nameIdx), stmt.getLine());
        
        locals.push_back({stmt.getModuleName(), scopeDepth, reg});
        lastResultReg = reg;
    }

    void Compiler::visit(FromImportStmt &stmt) {
        PomeString* nameStr = gc.allocate<PomeString>(stmt.getModuleName());
        int nameIdx = addConstant(PomeValue(nameStr));
        int modReg = allocReg();
        emit(Chunk::makeABx(OpCode::IMPORT, modReg, nameIdx), stmt.getLine());
        
        for (const auto& symbol : stmt.getSymbols()) {
            PomeString* symStr = gc.allocate<PomeString>(symbol);
            int symIdx = addConstant(PomeValue(symStr));
            int symKeyReg = allocReg();
            emit(Chunk::makeABx(OpCode::LOADK, symKeyReg, symIdx), stmt.getLine());
            
            int valReg = allocReg();
            emit(Chunk::makeABC(OpCode::GETTABLE, valReg, modReg, symKeyReg), stmt.getLine());
            
            locals.push_back({symbol, scopeDepth, valReg});
        }
        lastResultReg = modReg;
    }

    void Compiler::visit(ExportStmt &stmt) {
        stmt.getStmt()->accept(*this);
        int valReg = lastResultReg;

        std::string name;
        if (auto varDecl = dynamic_cast<VarDeclStmt*>(stmt.getStmt())) {
            name = varDecl->getName();
        } else if (auto funcDecl = dynamic_cast<FunctionDeclStmt*>(stmt.getStmt())) {
            name = funcDecl->getName();
        } else if (auto classDecl = dynamic_cast<ClassDeclStmt*>(stmt.getStmt())) {
            name = classDecl->getName();
        }

        if (!name.empty()) {
            PomeString* nameStr = gc.allocate<PomeString>(name);
            int nameIdx = addConstant(PomeValue(nameStr));
            emit(Chunk::makeABx(OpCode::EXPORT, valReg, nameIdx), stmt.getLine());
        }
    }

    void Compiler::visit(ExportExpressionStmt &stmt) {
        stmt.getExpression()->accept(*this);
        int valReg = lastResultReg;

        std::string name;
        if (auto ident = dynamic_cast<IdentifierExpr*>(stmt.getExpression())) {
            name = ident->getName();
        } else if (auto member = dynamic_cast<MemberAccessExpr*>(stmt.getExpression())) {
            name = member->getMember();
        }

        if (!name.empty()) {
            PomeString* nameStr = gc.allocate<PomeString>(name);
            int nameIdx = addConstant(PomeValue(nameStr));
            emit(Chunk::makeABx(OpCode::EXPORT, valReg, nameIdx), stmt.getLine());
        }
    }

    void Compiler::visit(ForEachStmt &stmt) {
        scopeDepth++;
        
        stmt.getIterable()->accept(*this);
        int iterableReg = lastResultReg;
        
        // State: [base: iterable, base+1: lastKey, base+2: nextKey, base+3: nextValue]
        int baseReg = allocReg(); 
        emit(Chunk::makeABC(OpCode::MOVE, baseReg, iterableReg, 0), stmt.getLine());
        
        int lastKeyReg = allocReg();
        int nextKeyReg = allocReg();
        int nextValueReg = allocReg();
        int internalIterReg = allocReg(); // base+4
        
        // Nil all state registers: base+1, base+2, base+3, base+4
        emit(Chunk::makeABC(OpCode::LOADNIL, lastKeyReg, 3, 0), stmt.getLine());

        // GETITER internalIterReg, baseReg -> calls .iterator() if instance
        emit(Chunk::makeABC(OpCode::GETITER, internalIterReg, baseReg, 0), stmt.getLine());
        
        int userKeyReg = allocReg();
        locals.push_back({stmt.getVarName(), scopeDepth, userKeyReg});
        
        int loopStart = currentChunk->code.size();
        
        // TFORCALL baseReg + 2, baseReg -> fills nextKeyReg (base+2) and nextValueReg (base+3)
        // A = destination register for RETURN, B = base register containing iterable
        emit(Chunk::makeABC(OpCode::TFORCALL, baseReg + 2, baseReg, 0), stmt.getLine());
        
        // If nextKeyReg is nil, exit loop.
        // We check nextKeyReg == nil
        int nilReg = allocReg();
        emit(Chunk::makeABC(OpCode::LOADNIL, nilReg, 0, 0), stmt.getLine());
        
        int isEndReg = allocReg();
        emit(Chunk::makeABC(OpCode::EQ, isEndReg, nextKeyReg, nilReg), stmt.getLine());
        
        // If it IS nil (isEndReg is true), jump to end.
        // TEST R(A), C: if R(A).truthy == (C != 0) ip++
        // Skip next JMP if NOT end (isEndReg is false)
        emit(Chunk::makeABC(OpCode::TEST, isEndReg, 0, 0), stmt.getLine()); 
        int jumpToEnd = emitJump(OpCode::JMP);
        
        // Move nextKey to user variable
        emit(Chunk::makeABC(OpCode::MOVE, userKeyReg, nextKeyReg, 0), stmt.getLine());
        
        for (const auto& s : stmt.getBody()) {
            s->accept(*this);
            resetFreeReg();
        }
        
        // TFORLOOP: lastKey = nextKey; jump to loopStart
        int offset = loopStart - (int)currentChunk->code.size() - 1;
        emit(Chunk::makeAsBx(OpCode::TFORLOOP, baseReg, offset), stmt.getLine());
        
        patchJump(jumpToEnd);
        
        while (!locals.empty() && locals.back().depth == scopeDepth) {
            locals.pop_back();
        }
        scopeDepth--;
        resetFreeReg();
    }

    void Compiler::visit(BlockStmt &stmt) {
        scopeDepth++;
        for (const auto& s : stmt.getStatements()) {
            s->accept(*this);
            resetFreeReg();
        }
        
        while (!locals.empty() && locals.back().depth == scopeDepth) {
            locals.pop_back();
        }
        scopeDepth--;
    }

}
