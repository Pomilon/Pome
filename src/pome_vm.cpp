#include "../include/pome_vm.h"
#include <iostream>
#include <cmath>
#include <cstring>

namespace Pome {

    VM::VM(GarbageCollector& gc, ModuleLoader loader) : gc(gc), moduleLoader(loader), frameCount(0), frameBase(0), ip(nullptr) {
        stack.resize(32768);
        frames.resize(1024);  
        stackTop = 0;
        gc.setVM(this); 
    }

    VM::~VM() {
    }

    void VM::runtimeError(const std::string& message) {
        std::cerr << "Runtime Error: " << message << std::endl;
        // Print stack trace
        for (int i = frameCount - 1; i >= 0; i--) {
            CallFrame* frame = &frames[i];
            if (frame->function) {
                std::cerr << "  in function " << frame->function->name << std::endl;
            } else {
                std::cerr << "  in script" << std::endl;
            }
        }
        hasError = true;
    }
    
    void VM::markRoots() {
        for (int i = 0; i < stackTop; ++i) {
            stack[i].mark(gc);
        }
        for (auto const& [key, val] : globals) {
            key.mark(gc);
            val.mark(gc);
        }
        for (auto const& [key, val] : moduleCache) {
            val.mark(gc);
        }
        if (currentModule) gc.markObject(currentModule);

        for (int i = 0; i < frameCount; ++i) {
            if (frames[i].function) gc.markObject(frames[i].function);
            if (frames[i].chunk) {
                for (auto& val : frames[i].chunk->constants) {
                    val.mark(gc);
                }
            }
        }
    }

    void VM::registerNative(const std::string& name, NativeFn fn) {
        NativeFunction* nativeObj = gc.allocate<NativeFunction>(name, fn);
        PomeString* nameStr = gc.allocate<PomeString>(name);
        globals[PomeValue(nameStr)] = PomeValue(nativeObj);
    }

    void VM::registerGlobal(const std::string& name, PomeValue value) {
        PomeString* nameStr = gc.allocate<PomeString>(name);
        globals[PomeValue(nameStr)] = value;
    }

#ifdef __GNUC__
    #define COMPUTED_GOTO 1
#endif

    void VM::interpret(Chunk* chunk, PomeModule* module) {
        if (!chunk) return;
        hasError = false;
        
        PomeModule* savedModule = currentModule;
        if (module) currentModule = module;
        
        int initialFrameIdx = frameCount;
        if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
        
        CallFrame* frame = &frames[frameCount++];
        frame->function = nullptr;
        frame->chunk = chunk;
        frame->ip = chunk->code.data();
        frame->base = stackTop;
        frame->destReg = 0;
        
        CallFrame* currentFrame;
        PomeValue* constants;
        uint32_t* ip;
        int frameBase;
        
        // Reusable buffers to avoid destructors in dispatch loop
        std::vector<PomeValue> args;
        std::string moduleName;

        #define REFRESH_FRAME() \
            currentFrame = &frames[frameCount - 1]; \
            constants = currentFrame->chunk->constants.data(); \
            ip = currentFrame->ip; \
            frameBase = currentFrame->base

        #define SAVE_FRAME() \
            currentFrame->ip = ip

        REFRESH_FRAME();
        stackTop += 256; 
        if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);

#ifdef COMPUTED_GOTO
        static void* dispatchTable[] = {
            &&LABEL_MOVE, &&LABEL_LOADK, &&LABEL_LOADBOOL, &&LABEL_LOADNIL,
            &&LABEL_ADD, &&LABEL_SUB, &&LABEL_MUL, &&LABEL_DIV, &&LABEL_MOD, &&LABEL_POW, &&LABEL_UNM,
            &&LABEL_NOT, &&LABEL_LEN, &&LABEL_CONCAT, &&LABEL_JMP, &&LABEL_EQ, &&LABEL_LT, &&LABEL_LE,
            &&LABEL_TEST, &&LABEL_TESTSET,
            &&LABEL_CALL, &&LABEL_TAILCALL, &&LABEL_RETURN,
            &&LABEL_GETGLOBAL, &&LABEL_SETGLOBAL,
            &&LABEL_GETUPVAL, &&LABEL_SETUPVAL, &&LABEL_CLOSURE,
            &&LABEL_NEWLIST, &&LABEL_NEWTABLE, &&LABEL_GETTABLE, &&LABEL_SETTABLE, &&LABEL_SELF,
            &&LABEL_FORLOOP, &&LABEL_FORPREP,
            &&LABEL_TFORCALL, &&LABEL_TFORLOOP,
            &&LABEL_IMPORT, &&LABEL_EXPORT, &&LABEL_GETITER,
            &&LABEL_AND, &&LABEL_OR,
            &&LABEL_SLICE,
            &&LABEL_PRINT
        };
        
        #define DISPATCH() \
            do { \
                uint32_t instruction = *ip++; \
                goto *dispatchTable[static_cast<uint8_t>(Chunk::getOpCode(instruction))]; \
            } while (false)
            
        DISPATCH();
#else
        #define DISPATCH() break
        while (true) {
            uint32_t instruction = *ip++;
            OpCode op = Chunk::getOpCode(instruction);
            switch (op) {
#endif

        LABEL_MOVE: {
            #ifndef COMPUTED_GOTO
            case OpCode::MOVE:
            #endif
            uint32_t instruction = ip[-1]; 
            stack[frameBase + Chunk::getA(instruction)] = stack[frameBase + Chunk::getB(instruction)];
            DISPATCH();
        }
        
        LABEL_LOADK: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADK:
            #endif
            uint32_t instruction = ip[-1];
            stack[frameBase + Chunk::getA(instruction)] = constants[Chunk::getBx(instruction)];
            DISPATCH();
        }
        
        LABEL_LOADBOOL: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADBOOL:
            #endif
            uint32_t instruction = ip[-1];
            stack[frameBase + Chunk::getA(instruction)] = PomeValue((bool)Chunk::getB(instruction));
            if (Chunk::getC(instruction)) ip++; 
            DISPATCH();
        }
        
        LABEL_LOADNIL: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADNIL:
            #endif
            uint32_t instruction = ip[-1];
            int a = Chunk::getA(instruction);
            int b = Chunk::getB(instruction);
            for (int i = a; i <= a + b; ++i) {
                stack[frameBase + i] = PomeValue();
            }
            DISPATCH();
        }

        LABEL_ADD: {
            #ifndef COMPUTED_GOTO
            case OpCode::ADD:
            #endif
            uint32_t instruction = ip[-1];
            int a = Chunk::getA(instruction);
            PomeValue v1 = stack[frameBase + Chunk::getB(instruction)];
            PomeValue v2 = stack[frameBase + Chunk::getC(instruction)];
            if (v1.isNumber() && v2.isNumber()) {
                stack[frameBase + a] = PomeValue(v1.asNumber() + v2.asNumber());
            } else if (v1.isInstance()) {
                PomeInstance* instance = v1.asInstance();
                PomeFunction* method = instance->klass->findMethod("__add__");
                if (method) {
                    int callBase = stackTop;
                    stack[callBase] = PomeValue(method);
                    stack[callBase + 1] = v1;
                    stack[callBase + 2] = v2;
                    SAVE_FRAME();
                    if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                    CallFrame* nextFrame = &frames[frameCount++];
                    nextFrame->function = method;
                    nextFrame->chunk = method->chunk.get();
                    nextFrame->ip = method->chunk->code.data();
                    nextFrame->base = callBase;
                    nextFrame->destReg = a;
                    REFRESH_FRAME();
                    if (frameBase + 256 > stackTop) stackTop = frameBase + 256;
                    if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);
                    DISPATCH();
                } else {
                    stack[frameBase + a] = PomeValue(gc.allocate<PomeString>(v1.toString() + v2.toString()));
                }
            } else {
                stack[frameBase + a] = PomeValue(gc.allocate<PomeString>(v1.toString() + v2.toString()));
            }
            DISPATCH();
        }
        
        LABEL_SUB: {
            #ifndef COMPUTED_GOTO
            case OpCode::SUB:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue v1 = stack[frameBase + Chunk::getB(instruction)];
            PomeValue v2 = stack[frameBase + Chunk::getC(instruction)];
            if (!v1.isNumber() || !v2.isNumber()) {
                runtimeError("Arithmetic on non-number.");
                currentModule = savedModule; // Restore
                return;
            }
            stack[frameBase + Chunk::getA(instruction)] = PomeValue(v1.asNumber() - v2.asNumber());
            DISPATCH();
        }
        
        LABEL_MUL: {
            #ifndef COMPUTED_GOTO
            case OpCode::MUL:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue v1 = stack[frameBase + Chunk::getB(instruction)];
            PomeValue v2 = stack[frameBase + Chunk::getC(instruction)];
            if (!v1.isNumber() || !v2.isNumber()) {
                runtimeError("Arithmetic on non-number.");
                currentModule = savedModule; // Restore
                return;
            }
            stack[frameBase + Chunk::getA(instruction)] = PomeValue(v1.asNumber() * v2.asNumber());
            DISPATCH();
        }
        
        LABEL_DIV: {
            #ifndef COMPUTED_GOTO
            case OpCode::DIV:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue v1 = stack[frameBase + Chunk::getB(instruction)];
            PomeValue v2 = stack[frameBase + Chunk::getC(instruction)];
            if (!v1.isNumber() || !v2.isNumber()) {
                runtimeError("Arithmetic on non-number.");
                currentModule = savedModule; // Restore
                return;
            }
            if (v2.asNumber() == 0.0) {
                runtimeError("Division by zero.");
                currentModule = savedModule; // Restore
                return;
            }
            stack[frameBase + Chunk::getA(instruction)] = PomeValue(v1.asNumber() / v2.asNumber());
            DISPATCH();
        }

        LABEL_MOD: {
            #ifndef COMPUTED_GOTO
            case OpCode::MOD:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue v1 = stack[frameBase + Chunk::getB(instruction)];
            PomeValue v2 = stack[frameBase + Chunk::getC(instruction)];
            if (!v1.isNumber() || !v2.isNumber()) {
                runtimeError("Arithmetic on non-number.");
                currentModule = savedModule; // Restore
                return;
            }
            stack[frameBase + Chunk::getA(instruction)] = PomeValue(std::fmod(v1.asNumber(), v2.asNumber()));
            DISPATCH();
        }

        LABEL_POW: {
            #ifndef COMPUTED_GOTO
            case OpCode::POW:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue v1 = stack[frameBase + Chunk::getB(instruction)];
            PomeValue v2 = stack[frameBase + Chunk::getC(instruction)];
            if (!v1.isNumber() || !v2.isNumber()) {
                runtimeError("Arithmetic on non-number.");
                currentModule = savedModule; // Restore
                return;
            }
            stack[frameBase + Chunk::getA(instruction)] = PomeValue(std::pow(v1.asNumber(), v2.asNumber()));
            DISPATCH();
        }

        LABEL_UNM: {
            #ifndef COMPUTED_GOTO
            case OpCode::UNM:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue v1 = stack[frameBase + Chunk::getB(instruction)];
            if (v1.isNumber()) {
                stack[frameBase + Chunk::getA(instruction)] = PomeValue(-v1.asNumber());
            } else if (v1.isInstance()) {
                PomeInstance* instance = v1.asInstance();
                PomeFunction* method = instance->klass->findMethod("__neg__");
                if (method) {
                    int callBase = stackTop;
                    stack[callBase] = PomeValue(method);
                    stack[callBase + 1] = v1;
                    SAVE_FRAME();
                    if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                    CallFrame* nextFrame = &frames[frameCount++];
                    nextFrame->function = method;
                    nextFrame->chunk = method->chunk.get();
                    nextFrame->ip = method->chunk->code.data();
                    nextFrame->base = callBase;
                    nextFrame->destReg = Chunk::getA(instruction);
                    REFRESH_FRAME();
                    if (frameBase + 256 > stackTop) stackTop = frameBase + 256;
                    if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);
                    DISPATCH();
                } else {
                    runtimeError("Unary negation not implemented for this instance.");
                    currentModule = savedModule; // Restore
                    return;
                }
            } else {
                runtimeError("Unary negation on non-number.");
                currentModule = savedModule; // Restore
                return;
            }
            DISPATCH();
        }

        LABEL_LEN: {
            #ifndef COMPUTED_GOTO
            case OpCode::LEN:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue val = stack[frameBase + Chunk::getB(instruction)];
            double length = 0;
            if (val.isString()) length = (double)val.asString().length();
            else if (val.isList()) length = (double)val.asList()->elements.size();
            else if (val.isTable()) length = (double)val.asTable()->elements.size();
            stack[frameBase + Chunk::getA(instruction)] = PomeValue(length);
            DISPATCH();
        }

        LABEL_CONCAT: {
            #ifndef COMPUTED_GOTO
            case OpCode::CONCAT:
            #endif
            uint32_t instruction = ip[-1];
            PomeString* s = gc.allocate<PomeString>(stack[frameBase + Chunk::getB(instruction)].toString() + stack[frameBase + Chunk::getC(instruction)].toString());
            stack[frameBase + Chunk::getA(instruction)] = PomeValue(s);
            DISPATCH();
        }
        
        LABEL_LT: {
            #ifndef COMPUTED_GOTO
            case OpCode::LT:
            #endif
            uint32_t instruction = ip[-1];
            bool res = stack[frameBase + Chunk::getB(instruction)].asNumber() < stack[frameBase + Chunk::getC(instruction)].asNumber();
            stack[frameBase + Chunk::getA(instruction)] = PomeValue(res);
            DISPATCH();
        }
        
        LABEL_LE: {
            #ifndef COMPUTED_GOTO
            case OpCode::LE:
            #endif
            uint32_t instruction = ip[-1];
            bool res = stack[frameBase + Chunk::getB(instruction)].asNumber() <= stack[frameBase + Chunk::getC(instruction)].asNumber();
            stack[frameBase + Chunk::getA(instruction)] = PomeValue(res);
            DISPATCH();
        }
        
        LABEL_EQ: {
            #ifndef COMPUTED_GOTO
            case OpCode::EQ:
            #endif
            uint32_t instruction = ip[-1];
            bool res = (stack[frameBase + Chunk::getB(instruction)] == stack[frameBase + Chunk::getC(instruction)]);
            stack[frameBase + Chunk::getA(instruction)] = PomeValue(res);
            DISPATCH();
        }
        
        LABEL_NOT: {
            #ifndef COMPUTED_GOTO
            case OpCode::NOT:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue v1 = stack[frameBase + Chunk::getB(instruction)];
            if (v1.isInstance()) {
                PomeInstance* instance = v1.asInstance();
                PomeFunction* method = instance->klass->findMethod("__not__");
                if (method) {
                    int callBase = stackTop;
                    stack[callBase] = PomeValue(method);
                    stack[callBase + 1] = v1;
                    SAVE_FRAME();
                    if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                    CallFrame* nextFrame = &frames[frameCount++];
                    nextFrame->function = method;
                    nextFrame->chunk = method->chunk.get();
                    nextFrame->ip = method->chunk->code.data();
                    nextFrame->base = callBase;
                    nextFrame->destReg = Chunk::getA(instruction);
                    REFRESH_FRAME();
                    if (frameBase + 256 > stackTop) stackTop = frameBase + 256;
                    if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);
                    DISPATCH();
                } else {
                     stack[frameBase + Chunk::getA(instruction)] = PomeValue(!v1.asBool());
                }
            } else {
                stack[frameBase + Chunk::getA(instruction)] = PomeValue(!v1.asBool());
            }
            DISPATCH();
        }
        
        LABEL_JMP: {
            #ifndef COMPUTED_GOTO
            case OpCode::JMP:
            #endif
            ip += Chunk::getSBx(ip[-1]);
            DISPATCH();
        }
        
        LABEL_TEST: {
            #ifndef COMPUTED_GOTO
            case OpCode::TEST:
            #endif
            uint32_t instruction = ip[-1];
            if (stack[frameBase + Chunk::getA(instruction)].asBool() == (Chunk::getC(instruction) != 0)) {
                ip++; 
            }
            DISPATCH();
        }
        
        LABEL_IMPORT: {
            #ifndef COMPUTED_GOTO
            case OpCode::IMPORT:
            #endif
            uint32_t instruction = ip[-1];
            int a = Chunk::getA(instruction);
            moduleName = constants[Chunk::getBx(instruction)].asString();

            if (moduleCache.count(moduleName)) {
                stack[frameBase + a] = moduleCache[moduleName];
            } else if (moduleLoader) {
                stack[frameBase + a] = moduleLoader(moduleName);
            } else {
                stack[frameBase + a] = PomeValue();
            }
            DISPATCH();
        }

        LABEL_EXPORT: {
            #ifndef COMPUTED_GOTO
            case OpCode::EXPORT:
            #endif
            uint32_t instruction = ip[-1];
            if (currentModule) {
                PomeValue key = constants[Chunk::getBx(instruction)];
                PomeValue val = stack[frameBase + Chunk::getA(instruction)];
                currentModule->exports[key] = val;
                gc.writeBarrier(currentModule, val);
            }
            DISPATCH();
        }

        LABEL_GETITER: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETITER:
            #endif
            uint32_t instruction = ip[-1];
            int a = Chunk::getA(instruction);
            int b = Chunk::getB(instruction);
            PomeValue obj = stack[frameBase + b];
            PomeValue existing = stack[frameBase + a];

            if (existing.isInstance() && existing.asInstance()->klass->findMethod("next")) {
                DISPATCH();
            }

            if (obj.isInstance()) {
                PomeInstance* instance = obj.asInstance();
                PomeFunction* iterMethod = instance->klass->findMethod("iterator");
                if (iterMethod) {
                    int callBase = stackTop;
                    stack[callBase] = PomeValue(iterMethod);
                    stack[callBase + 1] = obj;
                    SAVE_FRAME();
                    currentFrame->ip = ip - 1; // RE-EXECUTE GETITER
                    if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                    CallFrame* nextFrame = &frames[frameCount++];
                    nextFrame->function = iterMethod;
                    nextFrame->chunk = iterMethod->chunk.get();
                    nextFrame->ip = iterMethod->chunk->code.data();
                    nextFrame->base = callBase;
                    nextFrame->destReg = a;
                    REFRESH_FRAME();
                    stackTop += 256;
                    if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);
                    DISPATCH();
                } else {
                    stack[frameBase + a] = obj;
                }
            } else {
                stack[frameBase + a] = obj;
            }
            DISPATCH();
        }

        LABEL_AND: {
            #ifndef COMPUTED_GOTO
            case OpCode::AND:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue v1 = stack[frameBase + Chunk::getB(instruction)];
            stack[frameBase + Chunk::getA(instruction)] = !v1.asBool() ? v1 : stack[frameBase + Chunk::getC(instruction)];
            DISPATCH();
        }

        LABEL_OR: {
            #ifndef COMPUTED_GOTO
            case OpCode::OR:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue v1 = stack[frameBase + Chunk::getB(instruction)];
            stack[frameBase + Chunk::getA(instruction)] = v1.asBool() ? v1 : stack[frameBase + Chunk::getC(instruction)];
            DISPATCH();
        }

        LABEL_SLICE: {
            #ifndef COMPUTED_GOTO
            case OpCode::SLICE:
            #endif
            uint32_t instruction = ip[-1];
            int a = Chunk::getA(instruction);
            int b = Chunk::getB(instruction);
            int c = Chunk::getC(instruction);
            PomeValue obj = stack[frameBase + b];
            int start = (int)stack[frameBase + c].asNumber();
            int end = (int)stack[frameBase + c + 1].asNumber();
            
            if (obj.isList()) {
                auto& elements = obj.asList()->elements;
                int len = (int)elements.size();
                if (start < 0) start += len;
                if (end < 0) end += len;
                if (start < 0) start = 0;
                if (end > len) end = len;
                PomeList* newList = gc.allocate<PomeList>();
                for (int i = start; i < end; ++i) newList->elements.push_back(elements[i]);
                stack[frameBase + a] = PomeValue(newList);
            } else if (obj.isString()) {
                std::string s = obj.asString();
                int len = (int)s.length();
                if (start < 0) start += len;
                if (end < 0) end += len;
                if (start < 0) start = 0;
                if (end > len) end = len;
                stack[frameBase + a] = (start < end) ? PomeValue(gc.allocate<PomeString>(s.substr(start, end - start))) : PomeValue(gc.allocate<PomeString>(""));
            } else {
                stack[frameBase + a] = PomeValue();
            }
            DISPATCH();
        }

        LABEL_PRINT: {
            #ifndef COMPUTED_GOTO
            case OpCode::PRINT:
            #endif
            uint32_t instruction = ip[-1];
            int a = Chunk::getA(instruction);
            int b = Chunk::getB(instruction);
            for (int i = 0; i < b; ++i) {
                std::cout << stack[frameBase + a + i].toString();
                if (i < b - 1) std::cout << " ";
            }
            std::cout << std::endl;
            DISPATCH();
        }
        
        LABEL_RETURN: {
            #ifndef COMPUTED_GOTO
            case OpCode::RETURN:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue result = (Chunk::getB(instruction) > 1) ? stack[frameBase + Chunk::getA(instruction)] : PomeValue();
            int oldDestReg = currentFrame->destReg;
            stackTop = frameBase; 
            frameCount--;
            if (frameCount == initialFrameIdx) {
                currentModule = savedModule; // Restore
                return;
            }
            REFRESH_FRAME();
            stack[frameBase + oldDestReg] = result; 
            DISPATCH();
        }
        
        LABEL_CALL: {
            #ifndef COMPUTED_GOTO
            case OpCode::CALL:
            #endif
            uint32_t instruction = ip[-1];
            int a = Chunk::getA(instruction);
            int b = Chunk::getB(instruction); 
            PomeValue callee = stack[frameBase + a];
            if (callee.isNativeFunction()) {
                args.clear();
                for (int i = 1; i < b; ++i) args.push_back(stack[frameBase + a + i]);
                stack[frameBase + a] = callee.asNativeFunction()->call(args);
                DISPATCH();
            } else if (callee.isPomeFunction()) {
                PomeFunction* func = callee.asPomeFunction();
                
                // Logic to strip implicit 'self' if it's a Module and function doesn't expect it
                int argCount = b - 1;
                int paramCount = (int)func->parameters.size();
                if (argCount > paramCount && argCount > 0) {
                    PomeValue firstArg = stack[frameBase + a + 1];
                    if (firstArg.isModule()) {
                        // Shift arguments down to overwrite module self
                        for (int i = 1; i < argCount; ++i) {
                            stack[frameBase + a + i] = stack[frameBase + a + i + 1];
                        }
                        // We effectively removed one argument
                        // b--; // Not strictly needed as frame setup relies on params
                    }
                }

                SAVE_FRAME();
                if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                CallFrame* nextFrame = &frames[frameCount++];
                nextFrame->function = func;
                nextFrame->chunk = func->chunk.get();
                nextFrame->ip = func->chunk->code.data();
                nextFrame->base = frameBase + a;
                nextFrame->destReg = a; 
                REFRESH_FRAME();
                if (frameBase + 256 > stackTop) stackTop = frameBase + 256;
                if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);
                DISPATCH();
            } else if (callee.isClass()) {
                PomeClass* klass = callee.asClass();
                PomeInstance* instance = gc.allocate<PomeInstance>(klass);
                PomeFunction* init = klass->findMethod("init");
                if (init) {
                    for (int i = b - 1; i >= 1; --i) stack[frameBase + a + i + 1] = stack[frameBase + a + i];
                    stack[frameBase + a + 1] = PomeValue(instance);
                    SAVE_FRAME();
                    if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                    CallFrame* nextFrame = &frames[frameCount++];
                    nextFrame->function = init;
                    nextFrame->chunk = init->chunk.get();
                    nextFrame->ip = init->chunk->code.data();
                    nextFrame->base = frameBase + a;
                    nextFrame->destReg = a;
                    REFRESH_FRAME();
                    if (frameBase + 256 > stackTop) stackTop = frameBase + 256;
                    if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);
                } else {
                    stack[frameBase + a] = PomeValue(instance);
                }
                DISPATCH();
            } else {
                stack[frameBase + a] = PomeValue();
                DISPATCH();
            }
        }
        
        LABEL_GETGLOBAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETGLOBAL:
            #endif
            PomeValue key = constants[Chunk::getBx(ip[-1])];
            stack[frameBase + Chunk::getA(ip[-1])] = globals.count(key) ? globals[key] : PomeValue();
            DISPATCH();
        }
        
        LABEL_SETGLOBAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETGLOBAL:
            #endif
            globals[constants[Chunk::getBx(ip[-1])]] = stack[frameBase + Chunk::getA(ip[-1])];
            DISPATCH();
        }

        LABEL_GETUPVAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETUPVAL:
            #endif
            uint32_t instruction = ip[-1];
            int a = Chunk::getA(instruction);
            int b = Chunk::getB(instruction);
            if (currentFrame->function && b < (int)currentFrame->function->upvalues.size()) {
                stack[frameBase + a] = currentFrame->function->upvalues[b];
            } else {
                stack[frameBase + a] = PomeValue();
            }
            DISPATCH();
        }

        LABEL_SETUPVAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETUPVAL:
            #endif
            uint32_t instruction = ip[-1];
            int a = Chunk::getA(instruction);
            int b = Chunk::getB(instruction);
            if (currentFrame->function && b < (int)currentFrame->function->upvalues.size()) {
                currentFrame->function->upvalues[b] = stack[frameBase + a];
            }
            DISPATCH();
        }

        LABEL_CLOSURE: {
            #ifndef COMPUTED_GOTO
            case OpCode::CLOSURE:
            #endif
            uint32_t instruction = ip[-1];
            int a = Chunk::getA(instruction);
            int bx = Chunk::getBx(instruction);
            PomeFunction* funcTemplate = constants[bx].asPomeFunction();
            
            // Create a new function object (the closure) from the template
            PomeFunction* closure = gc.allocate<PomeFunction>();
            closure->name = funcTemplate->name;
            closure->parameters = funcTemplate->parameters;
            closure->chunk = std::make_unique<Chunk>();
            closure->chunk->code = funcTemplate->chunk->code;
            closure->chunk->constants = funcTemplate->chunk->constants;
            closure->chunk->lines = funcTemplate->chunk->lines;
            closure->upvalueCount = funcTemplate->upvalueCount;
            
            // Capture upvalues
            for (int i = 0; i < closure->upvalueCount; ++i) {
                uint32_t uvMeta = *ip++; // Read metadata instruction
                OpCode metaOp = Chunk::getOpCode(uvMeta);
                int index = Chunk::getB(uvMeta);
                
                if (metaOp == OpCode::MOVE) {
                    // Local from parent frame
                    PomeValue val = stack[frameBase + index];
                    closure->upvalues.push_back(val);
                } else {
                    // Upvalue from parent function
                    if (currentFrame->function && index < (int)currentFrame->function->upvalues.size()) {
                        closure->upvalues.push_back(currentFrame->function->upvalues[index]);
                    } else {
                        closure->upvalues.push_back(PomeValue());
                    }
                }
            }
            
            stack[frameBase + a] = PomeValue(closure);
            DISPATCH();
        }

        LABEL_NEWLIST: {
            #ifndef COMPUTED_GOTO
            case OpCode::NEWLIST:
            #endif
            stack[frameBase + Chunk::getA(ip[-1])] = PomeValue(gc.allocate<PomeList>());
            DISPATCH();
        }

        LABEL_NEWTABLE: {
            #ifndef COMPUTED_GOTO
            case OpCode::NEWTABLE:
            #endif
            stack[frameBase + Chunk::getA(ip[-1])] = PomeValue(gc.allocate<PomeTable>(std::map<PomeValue, PomeValue>{}));
            DISPATCH();
        }

        LABEL_GETTABLE: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETTABLE:
            #endif
            uint32_t instruction = ip[-1];
            int a = Chunk::getA(instruction);
            PomeValue obj = stack[frameBase + Chunk::getB(instruction)];
            PomeValue key = stack[frameBase + Chunk::getC(instruction)];
            if (obj.isTable()) {
                auto it = obj.asTable()->elements.find(key);
                stack[frameBase + a] = (it != obj.asTable()->elements.end()) ? it->second : PomeValue();
            } else if (obj.isList() && key.isNumber()) {
                int idx = (int)key.asNumber();
                auto& elements = obj.asList()->elements;
                stack[frameBase + a] = (idx >= 0 && idx < (int)elements.size()) ? elements[idx] : PomeValue();
            } else if (obj.isInstance()) {
                if (!key.isString()) {
                    runtimeError("Instance member key must be a string.");
                    return;
                }
                PomeInstance* instance = obj.asInstance();
                PomeValue field = instance->get(key.asString());
                if (!field.isNil()) stack[frameBase + a] = field;
                else {
                    PomeFunction* method = instance->klass->findMethod(key.asString());
                    stack[frameBase + a] = method ? PomeValue(method) : PomeValue();
                }
            } else if (obj.isModule()) {
                if (!key.isString()) {
                    runtimeError("Module export key must be a string.");
                    return;
                }
                auto it = obj.asModule()->exports.find(key);
                stack[frameBase + a] = (it != obj.asModule()->exports.end()) ? it->second : PomeValue();
            } else {
                runtimeError("Attempt to index " + obj.toString());
                return;
            }
            DISPATCH();
        }
        
        LABEL_SETTABLE: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETTABLE:
            #endif
            uint32_t instruction = ip[-1];
            PomeValue obj = stack[frameBase + Chunk::getA(instruction)];
            PomeValue key = stack[frameBase + Chunk::getB(instruction)];
            PomeValue val = stack[frameBase + Chunk::getC(instruction)];
            if (obj.isTable()) {
                obj.asTable()->elements[key] = val;
                gc.writeBarrier(obj.asObject(), val); 
            } else if (obj.isList() && key.isNumber()) {
                int idx = (int)key.asNumber();
                auto& elements = obj.asList()->elements;
                if (idx >= 0 && idx < (int)elements.size()) elements[idx] = val;
                else if (idx == (int)elements.size()) elements.push_back(val);
                gc.writeBarrier(obj.asObject(), val);
            } else if (obj.isInstance()) {
                obj.asInstance()->set(key.asString(), val);
                gc.writeBarrier(obj.asObject(), val);
            }
            DISPATCH();
        }
        
        LABEL_TFORCALL: {
            #ifndef COMPUTED_GOTO
            case OpCode::TFORCALL:
            #endif
            uint32_t instruction = ip[-1];
            int dest = Chunk::getA(instruction);
            int base = Chunk::getB(instruction);
            PomeValue iterObj = stack[frameBase + base + 4];

            if (iterObj.isInstance() && iterObj.asInstance()->klass->findMethod("next")) {
                PomeInstance* instance = iterObj.asInstance();
                PomeFunction* nextMethod = instance->klass->findMethod("next");
                int callBase = stackTop;
                stack[callBase] = PomeValue(nextMethod);
                stack[callBase + 1] = iterObj;
                SAVE_FRAME();
                if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                CallFrame* nextFrame = &frames[frameCount++];
                nextFrame->function = nextMethod;
                nextFrame->chunk = nextMethod->chunk.get();
                nextFrame->ip = nextMethod->chunk->code.data();
                nextFrame->base = callBase;
                nextFrame->destReg = dest;
                REFRESH_FRAME();
                stackTop += 256;
                if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);
                DISPATCH();
            } else if (iterObj.isTable()) {
                auto& elements = iterObj.asTable()->elements;
                PomeValue lastKey = stack[frameBase + base + 1];
                auto it = (lastKey.isNil()) ? elements.begin() : elements.upper_bound(lastKey);
                if (it != elements.end()) {
                    stack[frameBase + dest] = it->first;  
                    stack[frameBase + dest + 1] = it->second; 
                } else stack[frameBase + dest] = PomeValue(); 
            } else if (iterObj.isList()) {
                auto& elements = iterObj.asList()->elements;
                PomeValue lastKey = stack[frameBase + base + 1];
                int nextIdx = lastKey.isNil() ? 0 : (int)lastKey.asNumber() + 1;
                if (nextIdx >= 0 && nextIdx < (int)elements.size()) {
                    stack[frameBase + dest] = PomeValue((double)nextIdx); 
                    stack[frameBase + dest + 1] = elements[nextIdx];         
                } else stack[frameBase + dest] = PomeValue();
            } else stack[frameBase + dest] = PomeValue();
            DISPATCH();
        }

        LABEL_TFORLOOP: {
            #ifndef COMPUTED_GOTO
            case OpCode::TFORLOOP:
            #endif
            uint32_t instruction = ip[-1];
            if (!stack[frameBase + Chunk::getA(instruction) + 2].isNil()) {
                stack[frameBase + Chunk::getA(instruction) + 1] = stack[frameBase + Chunk::getA(instruction) + 2]; 
                ip += Chunk::getSBx(instruction); 
            }
            DISPATCH();
        }
        
        LABEL_TAILCALL:
        LABEL_SELF:
        LABEL_FORLOOP:
        LABEL_FORPREP:
        LABEL_TESTSET:
        {
            #ifndef COMPUTED_GOTO
            default:
            #endif
            return;
        }

#ifndef COMPUTED_GOTO
            } 
        } 
#endif
    }

}