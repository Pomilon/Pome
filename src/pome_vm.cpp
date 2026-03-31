#include "pome_vm.h"
#include "pome_chunk.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <dlfcn.h>
#include "pome_stdlib.h"

namespace Pome {

    VM::VM(GarbageCollector& gc, ModuleLoader loader) 
        : gc(gc), moduleLoader(loader), frameCount(0), stack(2048) {
        stackTop = 0;
        frames.resize(64);
    }

    VM::~VM() {}

    void VM::runtimeError(const std::string& message) {
        if (handlers.empty()) {
            std::cerr << "Runtime Error: " << message << std::endl;
            for (int i = frameCount - 1; i >= 0; --i) {
                CallFrame* frame = &frames[i];
                int line = frame->chunk->lines[frame->ip - frame->chunk->code.data() - 1];
                std::cerr << "  at [line " << line << "] in ";
                if (frame->function) std::cerr << frame->function->name;
                else std::cerr << "script";
                std::cerr << std::endl;
            }
        }
        // Construct exception object
        PomeString* msgStr = gc.allocate<PomeString>(message);
        throw VMException{PomeValue(msgStr)};
    }

    void VM::throwException(PomeValue value) {
        throw VMException{value};
    }

    void VM::registerNative(const std::string& name, NativeFn fn) {
        PomeString* nameStr = gc.allocate<PomeString>(name);
        RootGuard guard(gc, nameStr);
        NativeFunction* native = gc.allocate<NativeFunction>(name, fn);
        globals[PomeValue(nameStr)] = PomeValue(native);
    }

    void VM::registerGlobal(const std::string& name, PomeValue value) {
        PomeString* nameStr = gc.allocate<PomeString>(name);
        globals[PomeValue(nameStr)] = value;
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
        pendingException.mark(gc);

        for (int i = 0; i < frameCount; ++i) {
            if (frames[i].function) gc.markObject(frames[i].function);
            if (frames[i].task) gc.markObject(frames[i].task);
            if (frames[i].chunk) {
                for (auto& val : frames[i].chunk->constants) {
                    val.mark(gc);
                }
            }
        }

        for (auto* task : taskQueue) {
            gc.markObject(task);
        }
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
        PomeValue* R; 
        
        std::vector<PomeValue> args;
        uint32_t instruction;
        int a;

        #define REFRESH_FRAME() \
            currentFrame = &frames[frameCount - 1]; \
            constants = currentFrame->chunk->constants.data(); \
            ip = currentFrame->ip; \
            frameBase = currentFrame->base; \
            if (frameBase + 512 >= (int)stack.size()) { \
                stack.resize(stack.size() * 2); \
            } \
            R = stack.data() + frameBase; \
            stackTop = frameBase + 256

        #define SAVE_FRAME() \
            currentFrame->ip = ip

        REFRESH_FRAME();

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
                    a = (instruction >> 6) & 0xFF; \
                    goto *dispatchTable[static_cast<uint8_t>(instruction & 0x3F)]; \
                } while (false)

        DISPATCH();
        #else
        #define DISPATCH() break
        while (true) {
            instruction = *ip++;
            a = (instruction >> 6) & 0xFF;
            OpCode op = static_cast<OpCode>(instruction & 0x3F);
            switch (op) {
        #endif

        LABEL_MOVE: {
            #ifndef COMPUTED_GOTO
            case OpCode::MOVE:
            #endif
            R[a] = R[(instruction >> 23) & 0x1FF];
            DISPATCH();
        }

        LABEL_LOADK: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADK:
            #endif
            R[a] = constants[(instruction >> 14) & 0x3FFFF];
            DISPATCH();
        }

        LABEL_LOADBOOL: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADBOOL:
            #endif
            R[a] = PomeValue(((instruction >> 23) & 0x1FF) != 0);
            if (((instruction >> 14) & 0x1FF) != 0) ip++;
            DISPATCH();
        }

        LABEL_LOADNIL: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADNIL:
            #endif
            {
                int rb = (instruction >> 23) & 0x1FF;
                for (int i = 0; i <= rb; ++i) R[a + i] = PomeValue();
            }
            DISPATCH();
        }

        LABEL_GETUPVAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETUPVAL:
            #endif
            R[a] = currentFrame->function->upvalues[(instruction >> 23) & 0x1FF];
            DISPATCH();
        }

        LABEL_SETUPVAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETUPVAL:
            #endif
            currentFrame->function->upvalues[(instruction >> 23) & 0x1FF] = R[a];
            DISPATCH();
        }

        LABEL_GETGLOBAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETGLOBAL:
            #endif
            {
                PomeValue key = constants[(instruction >> 14) & 0x3FFFF];
                auto it = globals.find(key);
                if (it != globals.end()) {
                    R[a] = it->second;
                } else {
                    R[a] = PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_SETGLOBAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETGLOBAL:
            #endif
            globals[constants[(instruction >> 14) & 0x3FFFF]] = R[a];
            DISPATCH();
        }

        LABEL_GETTABLE: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETTABLE:
            #endif
            {
                PomeValue obj = R[(instruction >> 23) & 0x1FF];
                PomeValue key = R[(instruction >> 14) & 0x1FF];
                if (obj.isTable()) {
                    auto it = obj.asTable()->elements.find(key);
                    R[a] = (it != obj.asTable()->elements.end()) ? it->second : PomeValue();
                } else if (obj.isList()) {
                    if (key.isNumber()) {
                        int idx = (int)key.asNumber();
                        auto& elements = obj.asList()->elements;
                        if (idx >= 0 && idx < (int)elements.size()) {
                            R[a] = elements[idx];
                        } else {
                            R[a] = PomeValue();
                        }
                    } else {
                        R[a] = PomeValue();
                    }
                } else if (obj.isInstance()) {
                    PomeInstance* inst = obj.asInstance();
                    if (key.isString()) {
                        std::string k = key.asString();
                        PomeValue field = inst->get(k);
                        if (!field.isNil()) {
                            R[a] = field;
                        } else {
                            PomeFunction* method = inst->klass->findMethod(k);
                            if (method) {
                                R[a] = PomeValue(method);
                            } else {
                                R[a] = PomeValue();
                            }
                        }
                    } else {
                        R[a] = PomeValue();
                    }
                } else if (obj.isModule()) {
                    PomeModule* mod = obj.asModule();
                    auto it = mod->exports.find(key);
                    if (it != mod->exports.end()) {
                        R[a] = it->second;
                    } else {
                        R[a] = PomeValue();
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
            {
                PomeValue obj = R[a];
                PomeValue key = R[(instruction >> 23) & 0x1FF];
                PomeValue val = R[(instruction >> 14) & 0x1FF];
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
            }
            DISPATCH();
        }

        LABEL_NEWLIST: {
            #ifndef COMPUTED_GOTO
            case OpCode::NEWLIST:
            #endif
            {
                int rb = (instruction >> 23) & 0x1FF;
                PomeList* list = gc.allocate<PomeList>();
                R[a] = PomeValue(list); 
                for (int i = 0; i < rb; ++i) list->elements.push_back(R[a + i]);
            }
            DISPATCH();
        }

        LABEL_NEWTABLE: {
            #ifndef COMPUTED_GOTO
            case OpCode::NEWTABLE:
            #endif
            R[a] = PomeValue(gc.allocate<PomeTable>());
            DISPATCH();
        }

        LABEL_ADD: {
            #ifndef COMPUTED_GOTO
            case OpCode::ADD:
            #endif
            {
                PomeValue v1 = R[(instruction >> 23) & 0x1FF];
                PomeValue v2 = R[(instruction >> 14) & 0x1FF];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(v1.asNumber() + v2.asNumber());
                } else if (v1.isInstance()) {
                    PomeInstance* instance = v1.asInstance();
                    PomeFunction* method = instance->klass->findMethod("__add__");
                    if (method) {
                        SAVE_FRAME();
                        if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = method;
                        nextFrame->chunk = method->chunk.get();
                        nextFrame->ip = method->chunk->code.data();
                        nextFrame->base = frameBase + a + 3; // R(a) is target, v1, v2 are after
                        nextFrame->destReg = a;
                        
                        stack[nextFrame->base] = PomeValue(method);
                        stack[nextFrame->base + 1] = v1;
                        stack[nextFrame->base + 2] = v2;
                        
                        REFRESH_FRAME();
                        DISPATCH();
                    } else {
                        R[a] = PomeValue(gc.allocate<PomeString>(v1.toString() + v2.toString()));
                    }
                } else if (v1.isString() || v2.isString()) {
                    R[a] = PomeValue(gc.allocate<PomeString>(v1.toString() + v2.toString()));
                } else {
                    SAVE_FRAME();
                    runtimeError("Arithmetic on non-number.");
                    return PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_SUB: {
            #ifndef COMPUTED_GOTO
            case OpCode::SUB:
            #endif
            {
                PomeValue v1 = R[(instruction >> 23) & 0x1FF];
                PomeValue v2 = R[(instruction >> 14) & 0x1FF];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(v1.asNumber() - v2.asNumber());
                } else {
                    SAVE_FRAME();
                    runtimeError("Arithmetic on non-number.");
                    return PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_MUL: {
            #ifndef COMPUTED_GOTO
            case OpCode::MUL:
            #endif
            {
                PomeValue v1 = R[(instruction >> 23) & 0x1FF];
                PomeValue v2 = R[(instruction >> 14) & 0x1FF];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(v1.asNumber() * v2.asNumber());
                } else {
                    SAVE_FRAME();
                    runtimeError("Arithmetic on non-number.");
                    return PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_DIV: {
            #ifndef COMPUTED_GOTO
            case OpCode::DIV:
            #endif
            {
                PomeValue v1 = R[(instruction >> 23) & 0x1FF];
                PomeValue v2 = R[(instruction >> 14) & 0x1FF];
                if (v1.isNumber() && v2.isNumber()) {
                    if (v2.asNumber() == 0.0) {
                        SAVE_FRAME();
                        runtimeError("Division by zero.");
                        return PomeValue();
                    }
                    R[a] = PomeValue(v1.asNumber() / v2.asNumber());
                } else {
                    SAVE_FRAME();
                    runtimeError("Arithmetic on non-number.");
                    return PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_MOD: {
            #ifndef COMPUTED_GOTO
            case OpCode::MOD:
            #endif
            {
                PomeValue v1 = R[(instruction >> 23) & 0x1FF];
                PomeValue v2 = R[(instruction >> 14) & 0x1FF];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(std::fmod(v1.asNumber(), v2.asNumber()));
                } else {
                    SAVE_FRAME();
                    runtimeError("Arithmetic on non-number.");
                    return PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_POW: {
            #ifndef COMPUTED_GOTO
            case OpCode::POW:
            #endif
            {
                PomeValue v1 = R[(instruction >> 23) & 0x1FF];
                PomeValue v2 = R[(instruction >> 14) & 0x1FF];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(std::pow(v1.asNumber(), v2.asNumber()));
                } else {
                    SAVE_FRAME();
                    runtimeError("Arithmetic on non-number.");
                    return PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_UNM: {
            #ifndef COMPUTED_GOTO
            case OpCode::UNM:
            #endif
            {
                PomeValue v1 = R[(instruction >> 23) & 0x1FF];
                if (v1.isNumber()) {
                    R[a] = PomeValue(-v1.asNumber());
                } else if (v1.isInstance()) {
                    PomeInstance* instance = v1.asInstance();
                    PomeFunction* method = instance->klass->findMethod("__neg__");
                    if (method) {
                        SAVE_FRAME();
                        if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = method;
                        nextFrame->chunk = method->chunk.get();
                        nextFrame->ip = method->chunk->code.data();
                        nextFrame->base = frameBase + a + 2;
                        nextFrame->destReg = a;
                        
                        stack[nextFrame->base] = PomeValue(method);
                        stack[nextFrame->base + 1] = v1;
                        
                        REFRESH_FRAME();
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
            }
            DISPATCH();
        }

        LABEL_NOT: {
            #ifndef COMPUTED_GOTO
            case OpCode::NOT:
            #endif
            R[a] = PomeValue(!R[(instruction >> 23) & 0x1FF].asBool());
            DISPATCH();
        }

        LABEL_LEN: {
            #ifndef COMPUTED_GOTO
            case OpCode::LEN:
            #endif
            {
                PomeValue v = R[(instruction >> 23) & 0x1FF];
                if (v.isString()) R[a] = PomeValue((double)v.asString().length());
                else if (v.isList()) R[a] = PomeValue((double)v.asList()->elements.size());
                else if (v.isTable()) R[a] = PomeValue((double)v.asTable()->elements.size());
            }
            DISPATCH();
        }

        LABEL_CONCAT: {
            #ifndef COMPUTED_GOTO
            case OpCode::CONCAT:
            #endif
            {
                PomeString* s = gc.allocate<PomeString>(R[(instruction >> 23) & 0x1FF].toString() + R[(instruction >> 14) & 0x1FF].toString());
                R[a] = PomeValue(s);
            }
            DISPATCH();
        }

        LABEL_JMP: {
            #ifndef COMPUTED_GOTO
            case OpCode::JMP:
            #endif
            ip += (static_cast<int>((instruction >> 14) & 0x3FFFF) - Chunk::MAXARG_sBx);
            DISPATCH();
        }

        LABEL_EQ: {
            #ifndef COMPUTED_GOTO
            case OpCode::EQ:
            #endif
            R[a] = PomeValue(R[(instruction >> 23) & 0x1FF] == R[(instruction >> 14) & 0x1FF]);
            DISPATCH();
        }

        LABEL_LT: {
            #ifndef COMPUTED_GOTO
            case OpCode::LT:
            #endif
            R[a] = PomeValue(R[(instruction >> 23) & 0x1FF] < R[(instruction >> 14) & 0x1FF]);
            DISPATCH();
        }

        LABEL_LE: {
            #ifndef COMPUTED_GOTO
            case OpCode::LE:
            #endif
            {
                PomeValue v1 = R[(instruction >> 23) & 0x1FF];
                PomeValue v2 = R[(instruction >> 14) & 0x1FF];
                R[a] = PomeValue(v1 < v2 || v1 == v2);
            }
            DISPATCH();
        }

        LABEL_TEST: {
            #ifndef COMPUTED_GOTO
            case OpCode::TEST:
            #endif
            {
                int rc = (instruction >> 14) & 0x1FF;
                if (R[a].asBool() == (rc != 0)) {
                    ip++;
                }
            }
            DISPATCH();
        }

        LABEL_TESTSET: {
            #ifndef COMPUTED_GOTO
            case OpCode::TESTSET:
            #endif
            {
                int rb = (instruction >> 23) & 0x1FF;
                if (R[rb].asBool() == (((instruction >> 14) & 0x1FF) != 0)) {
                    R[a] = R[rb];
                } else {
                    ip++;
                }
            }
            DISPATCH();
        }

        LABEL_CALL: {
            #ifndef COMPUTED_GOTO
            case OpCode::CALL:
            #endif
            {
                PomeValue callee = R[a];
                int rb = (instruction >> 23) & 0x1FF;
                if (callee.isNativeFunction()) {
                    args.clear();
                    int startIdx = 1;
                    if (rb > 1 && R[a + 1].isModule()) startIdx = 2;
                    for (int i = startIdx; i < rb; ++i) args.push_back(R[a + i]);
                    
                    SAVE_FRAME();
                    PomeValue res = callee.asNativeFunction()->call(args);
                    REFRESH_FRAME(); 
                    
                    R[a] = res;
                    DISPATCH();
                } else if (callee.isPomeFunction()) {
                    PomeFunction* func = callee.asPomeFunction();
                    int argCount = rb - 1;
                    
                    // NEW: Automatically strip implicit self if it's a module function
                    // that doesn't expect 'this' (matching Lua-style module calls).
                    int nextFrameBase = frameBase + a;
                    if (argCount > (int)func->parameters.size() && R[a + 1].isModule()) {
                        nextFrameBase += 1; // Skip the module object in R[a+1]
                    }

                    if (func->isAsync) {
                        PomeTask* task = gc.allocate<PomeTask>(func);
                        // For async, we also need to skip the module if present
                        int skip = (argCount > (int)func->parameters.size() && R[a + 1].isModule()) ? 1 : 0;
                        for (int i = skip; i < argCount; ++i) {
                            task->args.push_back(R[a + 1 + i]);
                        }
                        taskQueue.push_back(task);
                        R[a] = PomeValue(task);
                        SAVE_FRAME();
                        DISPATCH();
                    }

                    SAVE_FRAME();
                    if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                    CallFrame* nextFrame = &frames[frameCount++];
                    nextFrame->function = func;
                    nextFrame->chunk = func->chunk.get();
                    nextFrame->ip = func->chunk->code.data();
                    nextFrame->base = nextFrameBase;
                    nextFrame->destReg = a; 
                    nextFrame->task = nullptr;
                    
                    REFRESH_FRAME();
                    DISPATCH();
                } else if (callee.isClass()) {
                    PomeClass* klass = callee.asClass();
                    PomeInstance* instance = gc.allocate<PomeInstance>(klass);
                    PomeFunction* init = klass->findMethod("init");
                    if (init) {
                        for (int i = rb - 1; i >= 1; --i) R[a + i + 1] = R[a + i];
                        R[a + 1] = PomeValue(instance);
                        SAVE_FRAME();
                        if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = init;
                        nextFrame->chunk = init->chunk.get();
                        nextFrame->ip = init->chunk->code.data();
                        nextFrame->base = frameBase + a;
                        nextFrame->destReg = a;
                        
                        REFRESH_FRAME(); 
                        DISPATCH();
                    } else {
                        R[a] = PomeValue(instance);
                        DISPATCH();
                    }
                } else {
                    SAVE_FRAME();
                    runtimeError("Object is not callable.");
                    return PomeValue();
                }
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
            {
                PomeValue result = ((instruction >> 23) & 0x1FF) == 0 ? PomeValue() : R[a];
                int dest = currentFrame->destReg;
                PomeTask* currentTask = currentFrame->task;
                
                if (currentTask) {
                    currentTask->result = result;
                    currentTask->isCompleted = true;
                }

                frameCount--;
                if (frameCount == initialFrameIdx) {
                    currentModule = savedModule;
                    return result;
                }

                REFRESH_FRAME();
                if (dest != -1) {
                    R[dest] = result;
                }
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
            {
                double step = R[a + 2].asNumber();
                double idx = R[a].asNumber() + step;
                double limit = R[a + 1].asNumber();
                R[a] = PomeValue(idx);
                if ((step > 0 && idx <= limit) || (step < 0 && idx >= limit)) {
                    ip += (static_cast<int>((instruction >> 14) & 0x3FFFF) - Chunk::MAXARG_sBx);
                    R[a + 3] = PomeValue(idx);
                }
            }
            DISPATCH();
        }

        LABEL_FORPREP: {
            #ifndef COMPUTED_GOTO
            case OpCode::FORPREP:
            #endif
            {
                double initVal = R[a].asNumber();
                double step = R[a + 2].asNumber();
                R[a] = PomeValue(initVal - step);
                ip += (static_cast<int>((instruction >> 14) & 0x3FFFF) - Chunk::MAXARG_sBx);
            }
            DISPATCH();
        }

        LABEL_TFORCALL: {
            #ifndef COMPUTED_GOTO
            case OpCode::TFORCALL:
            #endif
            {
                int rb = (instruction >> 23) & 0x1FF;
                PomeValue iterObj = R[rb];
                if (iterObj.isInstance() && iterObj.asInstance()->klass->findMethod("next")) {
                    PomeInstance* instance = iterObj.asInstance();
                    PomeFunction* nextMethod = instance->klass->findMethod("next");
                    SAVE_FRAME();
                    if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                    CallFrame* nextFrame = &frames[frameCount++];
                    nextFrame->function = nextMethod;
                    nextFrame->chunk = nextMethod->chunk.get();
                    nextFrame->ip = nextMethod->chunk->code.data();
                    nextFrame->base = frameBase + a + 2; 
                    nextFrame->destReg = a;
                    
                    stack[nextFrame->base] = PomeValue(nextMethod);
                    stack[nextFrame->base + 1] = iterObj;
                    stack[nextFrame->base + 2] = R[rb + 1]; // state
                    
                    REFRESH_FRAME();
                    DISPATCH();
                } else if (iterObj.isTable()) {
                    auto& data = iterObj.asTable()->elements;
                    PomeValue lastKey = R[rb + 1];
                    auto it = (lastKey.isNil()) ? data.begin() : data.upper_bound(lastKey);
                    if (it != data.end()) {
                        R[a] = it->first;  
                        R[a + 1] = it->second; 
                        R[rb + 1] = it->first; // Update state
                    } else R[a] = PomeValue(); 
                } else if (iterObj.isList()) {
                    auto& elements = iterObj.asList()->elements;
                    PomeValue lastKey = R[rb + 1];
                    int nextIdx = lastKey.isNil() ? 0 : (int)lastKey.asNumber() + 1;
                    if (nextIdx >= 0 && nextIdx < (int)elements.size()) {
                        R[a] = elements[nextIdx];         
                        R[a + 1] = PomeValue((double)nextIdx); 
                        R[rb + 1] = PomeValue((double)nextIdx);
                    } else R[a] = PomeValue();
                } else R[a] = PomeValue();
            }
            DISPATCH();
        }

        LABEL_TFORLOOP: {
            #ifndef COMPUTED_GOTO
            case OpCode::TFORLOOP:
            #endif
            if (!R[a + 2].isNil()) {
                R[a + 1] = R[a + 2]; 
                ip += (static_cast<int>((instruction >> 14) & 0x3FFFF) - Chunk::MAXARG_sBx); 
            }
            DISPATCH();
        }

        LABEL_IMPORT: {
            #ifndef COMPUTED_GOTO
            case OpCode::IMPORT:
            #endif
            {
                std::string name = constants[(instruction >> 14) & 0x3FFFF].asString();
                auto it = moduleCache.find(name);
                if (it != moduleCache.end()) {
                    R[a] = it->second;
                } else if (moduleLoader) {
                    SAVE_FRAME();
                    PomeValue mod = moduleLoader(name);
                    REFRESH_FRAME();
                    if (mod.isNil()) {
                        runtimeError("Cannot find module '" + name + "'");
                        return PomeValue();
                    }
                    if (mod.isModule()) moduleCache[name] = mod;
                    R[a] = mod;
                } else {
                    R[a] = PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_EXPORT: {
            #ifndef COMPUTED_GOTO
            case OpCode::EXPORT:
            #endif
            if (currentModule) {
                PomeValue val = R[a];
                currentModule->exports[constants[(instruction >> 14) & 0x3FFFF]] = val;
                gc.writeBarrier(currentModule, val);
            }
            DISPATCH();
        }

        LABEL_INHERIT: {
            #ifndef COMPUTED_GOTO
            case OpCode::INHERIT:
            #endif
            {
                PomeValue classVal = R[a];
                PomeValue superVal = R[(instruction >> 23) & 0x1FF];
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
            }
            DISPATCH();
        }

        LABEL_GETSUPER: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETSUPER:
            #endif
            {
                PomeValue methodName = constants[(instruction >> 14) & 0x1FF]; 
                if (currentFrame->function && currentFrame->function->klass && currentFrame->function->klass->superclass) {
                    PomeClass* super = currentFrame->function->klass->superclass;
                    PomeFunction* method = super->findMethod(methodName.asString());
                    if (method) {
                        R[a] = PomeValue(method);
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
            }
            DISPATCH();
        }

        LABEL_GETITER: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETITER:
            #endif
            {
                PomeValue obj = R[(instruction >> 23) & 0x1FF];
                if (obj.isInstance()) {
                    PomeInstance* instance = obj.asInstance();
                    PomeFunction* iterMethod = instance->klass->findMethod("iterator");
                    if (iterMethod) {
                        SAVE_FRAME();
                        if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = iterMethod;
                        nextFrame->chunk = iterMethod->chunk.get();
                        nextFrame->ip = iterMethod->chunk->code.data();
                        nextFrame->base = frameBase + a + 2;
                        nextFrame->destReg = a;
                        
                        stack[nextFrame->base] = PomeValue(iterMethod);
                        stack[nextFrame->base + 1] = obj;
                        
                        REFRESH_FRAME();
                        DISPATCH();
                    } else {
                        R[a] = obj;
                    }
                } else {
                    R[a] = obj;
                }
            }
            DISPATCH();
        }

        LABEL_AND: {
            #ifndef COMPUTED_GOTO
            case OpCode::AND:
            #endif
            {
                PomeValue v1 = R[(instruction >> 23) & 0x1FF];
                R[a] = !v1.asBool() ? v1 : R[(instruction >> 14) & 0x1FF];
            }
            DISPATCH();
        }

        LABEL_OR: {
            #ifndef COMPUTED_GOTO
            case OpCode::OR:
            #endif
            {
                PomeValue v1 = R[(instruction >> 23) & 0x1FF];
                R[a] = v1.asBool() ? v1 : R[(instruction >> 14) & 0x1FF];
            }
            DISPATCH();
        }

        LABEL_SLICE: {
            #ifndef COMPUTED_GOTO
            case OpCode::SLICE:
            #endif
            {
                PomeValue obj = R[(instruction >> 23) & 0x1FF];
                PomeValue startVal = R[(instruction >> 14) & 0x1FF];
                PomeValue endVal = R[a + 1]; 
                if (obj.isList()) {
                    PomeList* list = obj.asList();
                    int s = startVal.isNil() ? 0 : (int)startVal.asNumber();
                    int e = endVal.isNil() ? (int)list->elements.size() : (int)endVal.asNumber();
                    PomeList* res = gc.allocate<PomeList>();
                    for (int i = s; i < e && i < (int)list->elements.size(); ++i) res->elements.push_back(list->elements[i]);
                    R[a] = PomeValue(res);
                }
            }
            DISPATCH();
        }

        LABEL_PRINT: {
            #ifndef COMPUTED_GOTO
            case OpCode::PRINT:
            #endif
            {
                int rb = (instruction >> 23) & 0x1FF;
                for (int i = 0; i < rb; ++i) {
                    std::cout << R[a + i].toString() << (i == rb - 1 ? "" : " ");
                }
                std::cout << std::endl;
            }
            DISPATCH();
        }

        LABEL_TRY: {
            #ifndef COMPUTED_GOTO
            case OpCode::TRY:
            #endif
            handlers.push_back({frameCount - 1, ip + (static_cast<int>((instruction >> 14) & 0x3FFFF) - Chunk::MAXARG_sBx), stackTop});
            DISPATCH();
        }

        LABEL_THROW: {
            #ifndef COMPUTED_GOTO
            case OpCode::THROW:
            #endif
            SAVE_FRAME();
            throwException(R[a]);
            DISPATCH();
        }

        LABEL_CATCH: {
            #ifndef COMPUTED_GOTO
            case OpCode::CATCH:
            #endif
            R[a] = pendingException;
            pendingException = PomeValue(); 
            DISPATCH();
        }

        LABEL_ASYNC: {
           #ifndef COMPUTED_GOTO
           case OpCode::ASYNC:
           #endif
           {
               PomeValue callee = R[(instruction >> 23) & 0x1FF];
               if (callee.isPomeFunction()) {
                   PomeFunction* func = callee.asPomeFunction();
                   PomeTask* task = gc.allocate<PomeTask>(func);
                   taskQueue.push_back(task);
                   R[a] = PomeValue(task);
               } else {
                   runtimeError("ASYNC requires a Pome function.");
                   return PomeValue();
               }
           }
           DISPATCH();
        }
        LABEL_AWAIT: {
            #ifndef COMPUTED_GOTO
            case OpCode::AWAIT:
            #endif
            {
                PomeValue val = R[(instruction >> 23) & 0x1FF];
                if (val.isObject() && val.asObject()->type() == ObjectType::TASK) { 
                    PomeTask* task = (PomeTask*)val.asObject();
                    if (!task->isCompleted) {
                        SAVE_FRAME();
                        if (frameCount >= (int)frames.size()) frames.resize(frames.size() * 2);
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = task->function;
                        nextFrame->chunk = task->function->chunk.get();
                        nextFrame->ip = task->function->chunk->code.data();
                        nextFrame->base = frameBase + a + task->args.size() + 2;
                        nextFrame->destReg = a;
                        nextFrame->task = task;
                        
                        if (nextFrame->base + (int)task->args.size() + 256 >= (int)stack.size()) {
                            stack.resize(stack.size() * 2 + task->args.size());
                        }

                        for (size_t i = 0; i < task->args.size(); ++i) {
                            stack[nextFrame->base + i] = task->args[i];
                        }
                        
                        REFRESH_FRAME();
                        DISPATCH();
                    } else {
                        R[a] = task->result;
                    }
                } else {
                    R[a] = val;
                }
            }
            DISPATCH();
        }

        LABEL_CLOSURE: {
            #ifndef COMPUTED_GOTO
            case OpCode::CLOSURE:
            #endif
            {
                PomeFunction* proto = constants[(instruction >> 14) & 0x3FFFF].asPomeFunction();
                PomeFunction* closure = gc.allocate<PomeFunction>();
                closure->name = proto->name;
                closure->parameters = proto->parameters;
                closure->chunk = proto->chunk; 
                closure->upvalueCount = proto->upvalueCount;
                closure->isAsync = proto->isAsync;
                closure->module = currentModule;
                closure->klass = proto->klass;

                for (int i = 0; i < closure->upvalueCount; ++i) {
                    uint32_t uv = *ip++;
                    int upB = (uv >> 23) & 0x1FF;
                    if (static_cast<OpCode>(uv & 0x3F) == OpCode::MOVE) {
                        closure->upvalues.push_back(R[upB]);
                    } else {
                        closure->upvalues.push_back(currentFrame->function->upvalues[upB]);
                    }
                }
                R[a] = PomeValue(closure);
            }
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

            frameCount = handler.frameIdx + 1;
            REFRESH_FRAME();
            
            ip = handler.catchIp;
            stackTop = handler.stackTop;
            currentFrame->ip = ip;

            goto LABEL_EXCEPTION_LOOP;
        }

        runEventLoop();
        return PomeValue(); 
    }

    void VM::runEventLoop() {
        while (!taskQueue.empty()) {
            PomeTask* task = taskQueue.front();
            taskQueue.pop_front();
            
            if (task->isCompleted) continue;
            
            interpret(task->function->chunk.get(), task->function->module);
        }
    }

    PomeValue VM::loadNativeModule(const std::string& libraryPath, PomeModule* moduleObj) {
        void* handle = dlopen(libraryPath.c_str(), RTLD_NOW);
        if (!handle) {
            runtimeError("Could not load native module: " + std::string(dlerror()));
            return PomeValue();
        }

        typedef void (*InitFn)(PomeModule*, GarbageCollector&);
        InitFn init = (InitFn)dlsym(handle, "pome_module_init");
        if (!init) {
            runtimeError("Native module missing pome_module_init");
            dlclose(handle);
            return PomeValue();
        }

        init(moduleObj, gc);
        return PomeValue(moduleObj);
    }

}
