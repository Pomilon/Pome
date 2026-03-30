#include "../include/pome_vm.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <dlfcn.h> 

namespace Pome {

    const std::string RED = "\033[31m";
    const std::string RESET = "\033[0m";

    VM::VM(GarbageCollector& gc, ModuleLoader loader) : gc(gc), moduleLoader(loader), frameCount(0), frameBase(0), ip(nullptr) {
        stack.resize(32768);
        frames.resize(1024);  
        stackTop = 0;
        gc.setVM(this); 
        pendingException = PomeValue();
    }

    VM::~VM() {
    }

    void VM::throwException(PomeValue value) {
        throw VMException(value);
    }

    PomeValue VM::loadNativeModule(const std::string& libraryPath, PomeModule* moduleObj) {
        std::string fullPath = libraryPath;
        if (fullPath.find('/') == std::string::npos) {
            fullPath = "./" + fullPath;
        }
        void* handle = dlopen(fullPath.c_str(), RTLD_LAZY | RTLD_GLOBAL);
        if (!handle) {
            runtimeError("Failed to load native library: " + std::string(dlerror()));
            return PomeValue();
        }

        typedef void (*InitFunc)(VM*, PomeModule*);
        InitFunc init = (InitFunc)dlsym(handle, "pome_init");
        
        if (!init) {
            runtimeError("Failed to find pome_init in native library.");
            dlclose(handle);
            return PomeValue();
        }

        init(this, moduleObj);
        return PomeValue(moduleObj);
    }

    void VM::runtimeError(const std::string& message) {
        if (handlers.empty()) {
            std::cerr << RED << "Runtime Error: " << RESET << message << std::endl;
            
            for (int i = frameCount - 1; i >= 0; i--) {
                CallFrame* frame = &frames[i];
                size_t offset = frame->ip - frame->chunk->code.data();
                if (offset > 0) offset--; 
                
                int line = (offset < (int)frame->chunk->lines.size()) ? frame->chunk->lines[offset] : -1;
                
                std::string loc = "script";
                if (frame->function) {
                    loc = "function " + frame->function->name;
                    if (frame->function->module && !frame->function->module->scriptPath.empty()) {
                        loc += " in " + frame->function->module->scriptPath;
                    }
                } else if (currentModule && !currentModule->scriptPath.empty()) {
                    loc = currentModule->scriptPath;
                }

                std::cerr << "  at line " << line << " in " << loc << std::endl;
            }
            hasError = true;
        }

        throwException(PomeValue(gc.allocate<PomeString>(message)));
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

        for (auto task : taskQueue) {
            gc.markObject(task);
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

    PomeValue VM::interpret(Chunk* chunk, PomeModule* module) {
        if (!chunk) return PomeValue();
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
        
        std::vector<PomeValue> args;
        std::string moduleNameStr;
        
        uint32_t instruction;
        int a, b, c, bx, sbx;

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

    LABEL_EXCEPTION_LOOP:
        try {
#ifdef COMPUTED_GOTO
        static void* dispatchTable[] = {
            &&LABEL_MOVE, &&LABEL_LOADK, &&LABEL_LOADBOOL, &&LABEL_LOADNIL,
            &&LABEL_ADD, &&LABEL_SUB, &&LABEL_MUL, &&LABEL_DIV, &&LABEL_MOD, &&LABEL_POW, &&LABEL_UNM,
            &&LABEL_NOT, &&LABEL_LEN, &&LABEL_CONCAT, &&LABEL_JMP, &&LABEL_EQ, &&LABEL_LT, &&LABEL_LE,
            &&LABEL_TEST, &&LABEL_TESTSET,
            &&LABEL_CALL, &&LABEL_TAILCALL, &&LABEL_RETURN,
            &&LABEL_GETGLOBAL, &&LABEL_SETGLOBAL, &&LABEL_GETUPVAL, &&LABEL_SETUPVAL, &&LABEL_CLOSURE,
            &&LABEL_NEWLIST, &&LABEL_NEWTABLE, &&LABEL_GETTABLE, &&LABEL_SETTABLE, &&LABEL_SELF,
            &&LABEL_FORLOOP, &&LABEL_FORPREP,
            &&LABEL_TFORCALL, &&LABEL_TFORLOOP,
            &&LABEL_IMPORT, &&LABEL_EXPORT, &&LABEL_INHERIT, &&LABEL_GETSUPER, &&LABEL_GETITER,
            &&LABEL_AND, &&LABEL_OR, &&LABEL_SLICE, &&LABEL_PRINT,
            &&LABEL_TRY, &&LABEL_THROW, &&LABEL_CATCH,
            &&LABEL_ASYNC, &&LABEL_AWAIT
            };        
        #define DISPATCH() \
            do { \
                instruction = *ip++; \
                OpCode op = Chunk::getOpCode(instruction); \
                a = Chunk::getA(instruction); \
                b = Chunk::getB(instruction); \
                c = Chunk::getC(instruction); \
                bx = Chunk::getBx(instruction); \
                sbx = Chunk::getSBx(instruction); \
                goto *dispatchTable[static_cast<uint8_t>(op)]; \
            } while (false)

        DISPATCH();
        #else
        #define DISPATCH() break
        while (true) {
            instruction = *ip++;
            a = Chunk::getA(instruction);
            b = Chunk::getB(instruction);
            c = Chunk::getC(instruction);
            bx = Chunk::getBx(instruction);
            sbx = Chunk::getSBx(instruction);
            OpCode op = Chunk::getOpCode(instruction);
            switch (op) {
        #endif

        LABEL_MOVE: {
            #ifndef COMPUTED_GOTO
            case OpCode::MOVE:
            #endif
            stack[frameBase + a] = stack[frameBase + b];
            DISPATCH();
        }

        LABEL_LOADK: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADK:
            #endif
            stack[frameBase + a] = constants[bx];
            DISPATCH();
        }

        LABEL_LOADBOOL: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADBOOL:
            #endif
            stack[frameBase + a] = PomeValue(b != 0);
            if (c != 0) ip++;
            DISPATCH();
        }

        LABEL_LOADNIL: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADNIL:
            #endif
            for (int i = 0; i <= b; ++i) stack[frameBase + a + i] = PomeValue();
            DISPATCH();
        }

        LABEL_GETUPVAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETUPVAL:
            #endif
            stack[frameBase + a] = currentFrame->function->upvalues[b];
            DISPATCH();
        }

        LABEL_SETUPVAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETUPVAL:
            #endif
            currentFrame->function->upvalues[b] = stack[frameBase + a];
            DISPATCH();
        }

        LABEL_GETGLOBAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETGLOBAL:
            #endif
            PomeValue key = constants[bx];
            if (globals.count(key)) {
                stack[frameBase + a] = globals[key];
            } else {
                stack[frameBase + a] = PomeValue();
            }
            DISPATCH();
        }

        LABEL_SETGLOBAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETGLOBAL:
            #endif
            globals[constants[bx]] = stack[frameBase + a];
            DISPATCH();
        }

        LABEL_GETTABLE: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETTABLE:
            #endif
            {
                PomeValue obj = stack[frameBase + b];
                PomeValue key = stack[frameBase + c];
                if (obj.isTable()) {
                    auto it = obj.asTable()->elements.find(key);
                    stack[frameBase + a] = (it != obj.asTable()->elements.end()) ? it->second : PomeValue();
                } else if (obj.isList()) {
                    if (key.isNumber()) {
                        int idx = (int)key.asNumber();
                        auto& elements = obj.asList()->elements;
                        if (idx >= 0 && idx < (int)elements.size()) {
                            stack[frameBase + a] = elements[idx];
                        } else {
                            stack[frameBase + a] = PomeValue();
                        }
                    } else {
                        stack[frameBase + a] = PomeValue();
                    }
                } else if (obj.isInstance()) {
                    PomeInstance* inst = obj.asInstance();
                    if (key.isString()) {
                        std::string k = key.asString();
                        PomeValue field = inst->get(k);
                        if (!field.isNil()) {
                            stack[frameBase + a] = field;
                        } else {
                            PomeFunction* method = inst->klass->findMethod(k);
                            if (method) {
                                stack[frameBase + a] = PomeValue(method);
                            } else {
                                stack[frameBase + a] = PomeValue();
                            }
                        }
                    } else {
                        stack[frameBase + a] = PomeValue();
                    }
                } else if (obj.isModule()) {
                    PomeModule* mod = obj.asModule();
                    auto it = mod->exports.find(key);
                    if (it != mod->exports.end()) {
                        stack[frameBase + a] = it->second;
                    } else {
                        stack[frameBase + a] = PomeValue();
                    }
                } else {
                    SAVE_FRAME();
                    runtimeError("Attempt to index " + obj.toString());
                    return PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_SETTABLE: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETTABLE:
            #endif
            PomeValue obj = stack[frameBase + a];
            PomeValue key = stack[frameBase + b];
            PomeValue val = stack[frameBase + c];
            if (obj.isTable()) {
                obj.asTable()->elements[key] = val;
                gc.writeBarrier(obj.asObject(), val);
            } else if (obj.isList()) {
                if (key.isNumber()) {
                    int idx = (int)key.asNumber();
                    auto& elements = obj.asList()->elements;
                    if (idx >= 0 && idx < (int)elements.size()) {
                        elements[idx] = val;
                        gc.writeBarrier(obj.asObject(), val);
                    } else if (idx == (int)elements.size()) {
                        elements.push_back(val);
                        gc.writeBarrier(obj.asObject(), val);
                    }
                }
            } else if (obj.isInstance()) {
                if (key.isString()) {
                    obj.asInstance()->set(key.asString(), val);
                    gc.writeBarrier(obj.asObject(), val);
                }
            } else if (obj.isModule()) {
                obj.asModule()->exports[key] = val;
                gc.writeBarrier(obj.asObject(), val);
            }
            DISPATCH();
        }

        LABEL_NEWLIST: {
            #ifndef COMPUTED_GOTO
            case OpCode::NEWLIST:
            #endif
            PomeList* list = gc.allocate<PomeList>();
            for (int i = 0; i < b; ++i) list->elements.push_back(stack[frameBase + a + i]);
            stack[frameBase + a] = PomeValue(list);
            DISPATCH();
        }

        LABEL_NEWTABLE: {
            #ifndef COMPUTED_GOTO
            case OpCode::NEWTABLE:
            #endif
            stack[frameBase + a] = PomeValue(gc.allocate<PomeTable>());
            DISPATCH();
        }

        LABEL_ADD: {
            #ifndef COMPUTED_GOTO
            case OpCode::ADD:
            #endif
            PomeValue v1 = stack[frameBase + b];
            PomeValue v2 = stack[frameBase + c];
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
            } else if (v1.isString() || v2.isString()) {
                stack[frameBase + a] = PomeValue(gc.allocate<PomeString>(v1.toString() + v2.toString()));
            } else {
                SAVE_FRAME();
                runtimeError("Arithmetic on non-number.");
                return PomeValue();
            }
            DISPATCH();
        }

        LABEL_SUB: {
            #ifndef COMPUTED_GOTO
            case OpCode::SUB:
            #endif
            PomeValue v1 = stack[frameBase + b];
            PomeValue v2 = stack[frameBase + c];
            if (v1.isNumber() && v2.isNumber()) {
                stack[frameBase + a] = PomeValue(v1.asNumber() - v2.asNumber());
            } else {
                SAVE_FRAME();
                runtimeError("Arithmetic on non-number.");
                return PomeValue();
            }
            DISPATCH();
        }

        LABEL_MUL: {
            #ifndef COMPUTED_GOTO
            case OpCode::MUL:
            #endif
            PomeValue v1 = stack[frameBase + b];
            PomeValue v2 = stack[frameBase + c];
            if (v1.isNumber() && v2.isNumber()) {
                stack[frameBase + a] = PomeValue(v1.asNumber() * v2.asNumber());
            } else {
                SAVE_FRAME();
                runtimeError("Arithmetic on non-number.");
                return PomeValue();
            }
            DISPATCH();
        }

        LABEL_DIV: {
            #ifndef COMPUTED_GOTO
            case OpCode::DIV:
            #endif
            PomeValue v1 = stack[frameBase + b];
            PomeValue v2 = stack[frameBase + c];
            if (v1.isNumber() && v2.isNumber()) {
                if (v2.asNumber() == 0.0) {
                    SAVE_FRAME();
                    runtimeError("Division by zero.");
                    return PomeValue();
                }
                stack[frameBase + a] = PomeValue(v1.asNumber() / v2.asNumber());
            } else {
                SAVE_FRAME();
                runtimeError("Arithmetic on non-number.");
                return PomeValue();
            }
            DISPATCH();
        }

        LABEL_MOD: {
            #ifndef COMPUTED_GOTO
            case OpCode::MOD:
            #endif
            PomeValue v1 = stack[frameBase + b];
            PomeValue v2 = stack[frameBase + c];
            if (v1.isNumber() && v2.isNumber()) {
                stack[frameBase + a] = PomeValue(std::fmod(v1.asNumber(), v2.asNumber()));
            } else {
                SAVE_FRAME();
                runtimeError("Arithmetic on non-number.");
                return PomeValue();
            }
            DISPATCH();
        }

        LABEL_POW: {
            #ifndef COMPUTED_GOTO
            case OpCode::POW:
            #endif
            PomeValue v1 = stack[frameBase + b];
            PomeValue v2 = stack[frameBase + c];
            if (v1.isNumber() && v2.isNumber()) {
                stack[frameBase + a] = PomeValue(std::pow(v1.asNumber(), v2.asNumber()));
            } else {
                SAVE_FRAME();
                runtimeError("Arithmetic on non-number.");
                return PomeValue();
            }
            DISPATCH();
        }

        LABEL_UNM: {
            #ifndef COMPUTED_GOTO
            case OpCode::UNM:
            #endif
            PomeValue v1 = stack[frameBase + b];
            if (v1.isNumber()) {
                stack[frameBase + a] = PomeValue(-v1.asNumber());
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
                    nextFrame->destReg = a;
                    REFRESH_FRAME();
                    if (frameBase + 256 > stackTop) stackTop = frameBase + 256;
                    if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);
                    DISPATCH();
                } else {
                    SAVE_FRAME();
                    runtimeError("Unary negation not implemented for this instance.");
                    return PomeValue();
                }
            } else {
                SAVE_FRAME();
                runtimeError("Arithmetic on non-number.");
                return PomeValue();
            }
            DISPATCH();
        }

        LABEL_NOT: {
            #ifndef COMPUTED_GOTO
            case OpCode::NOT:
            #endif
            stack[frameBase + a] = PomeValue(!stack[frameBase + b].asBool());
            DISPATCH();
        }

        LABEL_LEN: {
            #ifndef COMPUTED_GOTO
            case OpCode::LEN:
            #endif
            PomeValue v = stack[frameBase + b];
            if (v.isString()) stack[frameBase + a] = PomeValue((double)v.asString().length());
            else if (v.isList()) stack[frameBase + a] = PomeValue((double)v.asList()->elements.size());
            else if (v.isTable()) stack[frameBase + a] = PomeValue((double)v.asTable()->elements.size());
            DISPATCH();
        }

        LABEL_CONCAT: {
            #ifndef COMPUTED_GOTO
            case OpCode::CONCAT:
            #endif
            PomeString* s = gc.allocate<PomeString>(stack[frameBase + b].toString() + stack[frameBase + c].toString());
            stack[frameBase + a] = PomeValue(s);
            DISPATCH();
        }

        LABEL_EQ: {
            #ifndef COMPUTED_GOTO
            case OpCode::EQ:
            #endif
            stack[frameBase + a] = PomeValue(stack[frameBase + b] == stack[frameBase + c]);
            DISPATCH();
        }

        LABEL_LT: {
            #ifndef COMPUTED_GOTO
            case OpCode::LT:
            #endif
            stack[frameBase + a] = PomeValue(stack[frameBase + b] < stack[frameBase + c]);
            DISPATCH();
        }

        LABEL_LE: {
            #ifndef COMPUTED_GOTO
            case OpCode::LE:
            #endif
            PomeValue v1 = stack[frameBase + b];
            PomeValue v2 = stack[frameBase + c];
            stack[frameBase + a] = PomeValue(v1 < v2 || v1 == v2);
            DISPATCH();
        }

        LABEL_JMP: {
            #ifndef COMPUTED_GOTO
            case OpCode::JMP:
            #endif
            ip += sbx;
            DISPATCH();
        }

        LABEL_TEST: {
            #ifndef COMPUTED_GOTO
            case OpCode::TEST:
            #endif
            if (stack[frameBase + a].asBool() == (c != 0)) {
                ip++; 
            }
            DISPATCH();
        }

        LABEL_TESTSET: {
            #ifndef COMPUTED_GOTO
            case OpCode::TESTSET:
            #endif
            if (stack[frameBase + b].asBool() == (c != 0)) {
                stack[frameBase + a] = stack[frameBase + b];
            } else {
                ip++;
            }
            DISPATCH();
        }

        LABEL_CALL: {
            #ifndef COMPUTED_GOTO
            case OpCode::CALL:
            #endif
            PomeValue callee = stack[frameBase + a];
            if (callee.isNativeFunction()) {
                args.clear();
                int startIdx = 1;
                if (b > 1 && stack[frameBase + a + 1].isModule()) startIdx = 2;
                for (int i = startIdx; i < b; ++i) args.push_back(stack[frameBase + a + i]);
                
                SAVE_FRAME();
                PomeValue res = callee.asNativeFunction()->call(args);
                REFRESH_FRAME();
                
                stack[frameBase + a] = res;
                DISPATCH();
            } else if (callee.isPomeFunction()) {
                PomeFunction* func = callee.asPomeFunction();

                int argCount = b - 1;
                if (func->isAsync) {
                    PomeTask* task = gc.allocate<PomeTask>(func);
                    for (int i = 0; i < argCount; ++i) {
                        task->args.push_back(stack[frameBase + a + 1 + i]);
                    }
                    taskQueue.push_back(task);
                    stack[frameBase + a] = PomeValue(task);
                    SAVE_FRAME();
                    DISPATCH();
                }

                int paramCount = (int)func->parameters.size();
                if (argCount > paramCount && argCount > 0) {
                    if (stack[frameBase + a + 1].isModule()) {
                        for (int i = 1; i < argCount; ++i) stack[frameBase + a + i] = stack[frameBase + a + i + 1];
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
                    DISPATCH();
                } else {
                    stack[frameBase + a] = PomeValue(instance);
                    DISPATCH();
                }
            } else {
                SAVE_FRAME();
                runtimeError("Object is not callable.");
                return PomeValue();
            }
            DISPATCH();
        }

        LABEL_TAILCALL: {
            #ifndef COMPUTED_GOTO
            case OpCode::TAILCALL:
            #endif
            DISPATCH();
        }

        LABEL_RETURN: {
            #ifndef COMPUTED_GOTO
            case OpCode::RETURN:
            #endif
            PomeValue result = (b > 1) ? stack[frameBase + a] : PomeValue();

            if (frames[frameCount-1].task) {
                frames[frameCount-1].task->result = result;
                frames[frameCount-1].task->isCompleted = true;
            }

            // Pop handlers belonging to this frame

            while (!handlers.empty() && handlers.back().frameIdx >= frameCount - 1) {
                handlers.pop_back();
            }

            frameCount--;
            if (frameCount <= initialFrameIdx) {
                while (!taskQueue.empty()) {
                    PomeTask* task = taskQueue.front();
                    taskQueue.pop_front();
                    if (task->isCompleted) continue;

                    if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                    CallFrame* nextFrame = &frames[frameCount++];
                    nextFrame->function = task->function;
                    nextFrame->chunk = task->function->chunk.get();
                    nextFrame->ip = task->function->chunk->code.data();
                    nextFrame->base = stackTop;
                    nextFrame->destReg = -1;
                    nextFrame->task = task;

                    for (size_t i = 0; i < (int)task->args.size(); ++i) {
                        stack[stackTop + i] = task->args[i];
                    }

                    REFRESH_FRAME();
                    stackTop += 256;
                    if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);
                    DISPATCH();
                }
                currentModule = savedModule;
                return result;
            }

            int dest = frames[frameCount].destReg;

            REFRESH_FRAME();
            if (dest != -1) {
                stack[frameBase + dest] = result;
            }
            DISPATCH();
        }

        LABEL_SELF: {
            #ifndef COMPUTED_GOTO
            case OpCode::SELF:
            #endif
            DISPATCH();
        }

        LABEL_FORLOOP: {
            #ifndef COMPUTED_GOTO
            case OpCode::FORLOOP:
            #endif
            double step = stack[frameBase + a + 2].asNumber();
            double idx = stack[frameBase + a].asNumber() + step;
            double limit = stack[frameBase + a + 1].asNumber();
            stack[frameBase + a] = PomeValue(idx);
            if ((step > 0 && idx <= limit) || (step < 0 && idx >= limit)) {
                ip += sbx;
                stack[frameBase + a + 3] = PomeValue(idx);
            }
            DISPATCH();
        }

        LABEL_FORPREP: {
            #ifndef COMPUTED_GOTO
            case OpCode::FORPREP:
            #endif
            double initVal = stack[frameBase + a].asNumber();
            double step = stack[frameBase + a + 2].asNumber();
            stack[frameBase + a] = PomeValue(initVal - step);
            ip += sbx;
            DISPATCH();
        }

        LABEL_TFORCALL: {
            #ifndef COMPUTED_GOTO
            case OpCode::TFORCALL:
            #endif
            PomeValue iterObj = stack[frameBase + b + 4];
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
                nextFrame->destReg = a;
                REFRESH_FRAME();
                stackTop += 256;
                if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);
                DISPATCH();
            } else if (iterObj.isTable()) {
                auto& data = iterObj.asTable()->elements;
                PomeValue lastKey = stack[frameBase + b + 1];
                auto it = (lastKey.isNil()) ? data.begin() : data.upper_bound(lastKey);
                if (it != data.end()) {
                    stack[frameBase + a] = it->first;  
                    stack[frameBase + a + 1] = it->second; 
                } else stack[frameBase + a] = PomeValue(); 
            } else if (iterObj.isList()) {
                auto& elements = iterObj.asList()->elements;
                PomeValue lastKey = stack[frameBase + b + 1];
                int nextIdx = lastKey.isNil() ? 0 : (int)lastKey.asNumber() + 1;
                if (nextIdx >= 0 && nextIdx < (int)elements.size()) {
                    stack[frameBase + a] = PomeValue((double)nextIdx); 
                    stack[frameBase + a + 1] = elements[nextIdx];         
                } else stack[frameBase + a] = PomeValue();
            } else stack[frameBase + a] = PomeValue();
            DISPATCH();
        }

        LABEL_TFORLOOP: {
            #ifndef COMPUTED_GOTO
            case OpCode::TFORLOOP:
            #endif
            if (!stack[frameBase + a + 2].isNil()) {
                stack[frameBase + a + 1] = stack[frameBase + a + 2]; 
                ip += sbx; 
            }
            DISPATCH();
        }

        LABEL_IMPORT: {
            #ifndef COMPUTED_GOTO
            case OpCode::IMPORT:
            #endif
            {
                std::string name = constants[bx].asString();
                if (moduleCache.count(name)) {
                    stack[frameBase + a] = moduleCache[name];
                } else if (moduleLoader) {
                    SAVE_FRAME();
                    PomeValue mod = moduleLoader(name);
                    REFRESH_FRAME();
                    if (mod.isNil()) {
                        runtimeError("Cannot find module '" + name + "'");
                        return PomeValue();
                    }
                    if (mod.isModule()) moduleCache[name] = mod;
                    stack[frameBase + a] = mod;
                } else {
                    stack[frameBase + a] = PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_EXPORT: {
            #ifndef COMPUTED_GOTO
            case OpCode::EXPORT:
            #endif
            if (currentModule) {
                currentModule->exports[constants[bx]] = stack[frameBase + a];
                gc.writeBarrier(currentModule, stack[frameBase + a]);
            }
            DISPATCH();
        }

        LABEL_INHERIT: {
            #ifndef COMPUTED_GOTO
            case OpCode::INHERIT:
            #endif
            PomeValue classVal = stack[frameBase + a];
            PomeValue superVal = stack[frameBase + b];
            if (!classVal.isClass()) {
                SAVE_FRAME();
                runtimeError("Inheritance target must be a class.");
                return PomeValue();
            }
            if (!superVal.isNil()) {
                if (!superVal.isClass()) {
                    SAVE_FRAME();
                    runtimeError("Superclass must be a class.");
                    return PomeValue();
                }
                classVal.asClass()->superclass = superVal.asClass();
            }
            DISPATCH();
        }

        LABEL_GETSUPER: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETSUPER:
            #endif
            PomeValue methodName = constants[c];
            if (currentFrame->function && currentFrame->function->klass && currentFrame->function->klass->superclass) {
                PomeClass* super = currentFrame->function->klass->superclass;
                PomeFunction* method = super->findMethod(methodName.asString());
                if (method) {
                    stack[frameBase + a] = PomeValue(method);
                } else {
                    SAVE_FRAME();
                    runtimeError("Superclass '" + super->name + "' has no method '" + methodName.asString() + "'");
                    return PomeValue();
                }
            } else {
                SAVE_FRAME();
                runtimeError("'super' used in a class with no superclass or outside of a method.");
                return PomeValue();
            }
            DISPATCH();
        }

        LABEL_GETITER: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETITER:
            #endif
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
            PomeValue v1 = stack[frameBase + b];
            stack[frameBase + a] = !v1.asBool() ? v1 : stack[frameBase + c];
            DISPATCH();
        }

        LABEL_OR: {
            #ifndef COMPUTED_GOTO
            case OpCode::OR:
            #endif
            PomeValue v1 = stack[frameBase + b];
            stack[frameBase + a] = v1.asBool() ? v1 : stack[frameBase + c];
            DISPATCH();
        }

        LABEL_SLICE: {
            #ifndef COMPUTED_GOTO
            case OpCode::SLICE:
            #endif
            PomeValue obj = stack[frameBase + b];
            PomeValue startVal = stack[frameBase + c];
            PomeValue endVal = stack[frameBase + a + 1]; 
            if (obj.isList()) {
                PomeList* list = obj.asList();
                int s = startVal.isNil() ? 0 : (int)startVal.asNumber();
                int e = endVal.isNil() ? (int)list->elements.size() : (int)endVal.asNumber();
                PomeList* res = gc.allocate<PomeList>();
                for (int i = s; i < e && i < (int)list->elements.size(); ++i) res->elements.push_back(list->elements[i]);
                stack[frameBase + a] = PomeValue(res);
            }
            DISPATCH();
        }

        LABEL_PRINT: {
            #ifndef COMPUTED_GOTO
            case OpCode::PRINT:
            #endif
            for (int i = 0; i < b; ++i) {
                std::cout << stack[frameBase + a + i].toString() << (i == b - 1 ? "" : " ");
            }
            std::cout << std::endl;
            DISPATCH();
        }

        LABEL_TRY: {
            #ifndef COMPUTED_GOTO
            case OpCode::TRY:
            #endif
            handlers.push_back({frameCount - 1, ip + sbx, stackTop});
            DISPATCH();
        }

        LABEL_THROW: {
            #ifndef COMPUTED_GOTO
            case OpCode::THROW:
            #endif
            SAVE_FRAME();
            throwException(stack[frameBase + a]);
            DISPATCH();
        }

        LABEL_CATCH: {
            #ifndef COMPUTED_GOTO
            case OpCode::CATCH:
            #endif
            stack[frameBase + a] = pendingException;
            pendingException = PomeValue(); // Clear it
            DISPATCH();
        }

        LABEL_ASYNC: {
           #ifndef COMPUTED_GOTO
           case OpCode::ASYNC:
           #endif
           PomeValue callee = stack[frameBase + b];
           if (callee.isPomeFunction()) {
               PomeFunction* func = callee.asPomeFunction();
               PomeTask* task = gc.allocate<PomeTask>(func);
               taskQueue.push_back(task);
               stack[frameBase + a] = PomeValue(task);
           } else {
               runtimeError("ASYNC requires a Pome function.");
               return PomeValue();
           }
           DISPATCH();
        }
        LABEL_AWAIT: {
            #ifndef COMPUTED_GOTO
            case OpCode::AWAIT:
            #endif
            PomeValue val = stack[frameBase + b];
            if (val.isObject() && val.asObject()->type() == ObjectType::TASK) {
                PomeTask* task = (PomeTask*)val.asObject();
                if (!task->isCompleted) {
                    SAVE_FRAME();
                    if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                    CallFrame* nextFrame = &frames[frameCount++];
                    nextFrame->function = task->function;
                    nextFrame->chunk = task->function->chunk.get();
                    nextFrame->ip = task->function->chunk->code.data();
                    nextFrame->base = stackTop;
                    nextFrame->destReg = a;
                    nextFrame->task = task;
                    // Copy arguments to the new frame's stack
                    for (size_t i = 0; i < task->args.size(); ++i) {
                        stack[stackTop + i] = task->args[i];
                    }
                    
                    REFRESH_FRAME();
                    stackTop += 256;
                    if (stackTop + 256 >= (int)stack.size()) stack.resize(stack.size() * 2);
                    DISPATCH();
                } else {
                    stack[frameBase + a] = task->result;
                }
            } else {
                stack[frameBase + a] = val;
            }
            DISPATCH();
        }

        LABEL_CLOSURE: {
            #ifndef COMPUTED_GOTO
            case OpCode::CLOSURE:
            #endif
            PomeFunction* proto = constants[bx].asPomeFunction();
            PomeFunction* closure = gc.allocate<PomeFunction>();
            closure->name = proto->name;
            closure->parameters = proto->parameters;
            closure->chunk = std::make_unique<Chunk>(*proto->chunk);
            closure->upvalueCount = proto->upvalueCount;
            closure->isAsync = proto->isAsync;
            closure->module = currentModule;
            closure->klass = proto->klass;

            for (int i = 0; i < closure->upvalueCount; ++i) {
                uint32_t uv = *ip++;
                int upB = Chunk::getB(uv);
                if (Chunk::getOpCode(uv) == OpCode::MOVE) {
                    closure->upvalues.push_back(stack[frameBase + upB]);
                } else {
                    closure->upvalues.push_back(currentFrame->function->upvalues[upB]);
                }
            }
            stack[frameBase + a] = PomeValue(closure);
            DISPATCH();
        }

#ifndef COMPUTED_GOTO
            default:
                return PomeValue();
            }
        }
#endif
        } catch (const VMException& e) {
            if (handlers.empty()) {
                hasError = true;
                currentModule = savedModule;
                return e.value;
            }

            pendingException = e.value;
            ExceptionHandler handler = handlers.back();
            handlers.pop_back();

            // Unwind call stack
            frameCount = handler.frameIdx + 1;
            REFRESH_FRAME();
            
            // Restore state
            ip = handler.catchIp;
            stackTop = handler.stackTop;
            
            // Ensure frame IP is consistent
            currentFrame->ip = ip;

            goto LABEL_EXCEPTION_LOOP;
        }

        runEventLoop();
        return PomeValue(); // Return result of main chunk if needed, but for now nil is fine
    }

}
