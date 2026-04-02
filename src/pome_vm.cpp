#include "pome_vm.h"
#include "pome_chunk.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <dlfcn.h>
#include "pome_stdlib.h"
#include "pome_shape.h"

namespace Pome {



    VM::VM(GarbageCollector& gc, ModuleLoader loader) 
        : gc(gc), moduleLoader(loader), frameCount(0), stack(65536) {
        stackTop = 0;
        rootShape = gc.allocate<PomeShape>(nullptr, PomeValue(), -1);
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

    PomeUpvalue* VM::captureUpvalue(PomeValue* local) {
        PomeUpvalue* prevUpvalue = nullptr;
        PomeUpvalue* upvalue = openUpvalues;

        while (upvalue != nullptr && upvalue->location > local) {
            prevUpvalue = upvalue;
            upvalue = upvalue->next;
        }

        if (upvalue != nullptr && upvalue->location == local) {
            return upvalue;
        }

        PomeUpvalue* createdUpvalue = gc.allocate<PomeUpvalue>(local);
        createdUpvalue->next = upvalue;

        if (prevUpvalue == nullptr) {
            openUpvalues = createdUpvalue;
        } else {
            prevUpvalue->next = createdUpvalue;
        }

        return createdUpvalue;
    }

    void VM::closeUpvalues(PomeValue* last) {
        while (openUpvalues != nullptr && openUpvalues->location >= last) {
            PomeUpvalue* upvalue = openUpvalues;
            upvalue->closedValue = *upvalue->location;
            upvalue->location = &upvalue->closedValue;
            openUpvalues = upvalue->next;
        }
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
        for (auto& arg : args) {
            arg.mark(gc);
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
        
        if (rootShape) gc.markObject(rootShape);
        for (int i = 0; i < 256; ++i) {
            if (charCache[i]) gc.markObject(charCache[i]);
        }

        PomeUpvalue* upvalue = openUpvalues;
        while (upvalue != nullptr) {
            gc.markObject(upvalue);
            upvalue = upvalue->next;
        }

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
        
        RootGuard moduleGuard(gc, module);

        int initialFrameIdx = frameCount;
        if (frameCount >= 8192) runtimeError("Stack overflow");
        
        CallFrame* frame = &frames[frameCount++];
        frame->function = nullptr;
        frame->module = module ? module : currentModule;
        frame->chunk = chunk;
        frame->ip = chunk->code.data();
        frame->base = stackTop;
        frame->destReg = 0;
        
        CallFrame* currentFrame;
        PomeValue* K;
        uint32_t* ip;
        int frameBase;
        PomeValue* R; 
        
        
        // Decoded operands (Must be local to prevent corruption in recursive calls)
        uint32_t instruction;
        int a, b, c, bx, sbx;
        
        #define REFRESH_FRAME() \
            currentFrame = &frames[frameCount - 1]; \
            K = currentFrame->chunk->constants.data(); \
            ip = currentFrame->ip; \
            frameBase = currentFrame->base; \
            currentModule = currentFrame->module; \
            if (frameBase + 512 >= (int)stack.size()) { \
                stack.resize(stack.size() * 2); \
            } \
            R = stack.data() + frameBase; \
            stackTop = frameBase + 256

        #define SAVE_FRAME() \
            currentFrame->ip = ip

        REFRESH_FRAME();

    LABEL_EXCEPTION_LOOP:
        if (gc.pendingGC) {
            gc.pendingGC = false;
            bool minor = gc.shouldCollectMinor();
            gc.collect(minor);
        }
        
        try {
#ifdef COMPUTED_GOTO
        static void* dispatchTable[] = {
    &&LABEL_MOVE, // 0
    &&LABEL_LOADK, // 1
    &&LABEL_LOADBOOL, // 2
    &&LABEL_LOADNIL, // 3
    &&LABEL_ADD, // 4
    &&LABEL_SUB, // 5
    &&LABEL_MUL, // 6
    &&LABEL_DIV, // 7
    &&LABEL_MOD, // 8
    &&LABEL_POW, // 9
    &&LABEL_UNM, // 10
    &&LABEL_NOT, // 11
    &&LABEL_LEN, // 12
    &&LABEL_CONCAT, // 13
    &&LABEL_JMP, // 14
    &&LABEL_EQ, // 15
    &&LABEL_LT, // 16
    &&LABEL_LE, // 17
    &&LABEL_TEST, // 18
    &&LABEL_TESTSET, // 19
    &&LABEL_CALL, // 20
    &&LABEL_TAILCALL, // 21
    &&LABEL_RETURN, // 22
    &&LABEL_GETGLOBAL, // 23
    &&LABEL_SETGLOBAL, // 24
    &&LABEL_GETUPVAL, // 25
    &&LABEL_SETUPVAL, // 26
    &&LABEL_CLOSURE, // 27
    &&LABEL_NEWLIST, // 28
    &&LABEL_NEWTABLE, // 29
    &&LABEL_GETTABLE, // 30
    &&LABEL_SETTABLE, // 31
    &&LABEL_SELF, // 32
    &&LABEL_FORLOOP, // 33
    &&LABEL_FORPREP, // 34
    &&LABEL_TFORCALL, // 35
    &&LABEL_TFORLOOP, // 36
    &&LABEL_IMPORT, // 37
    &&LABEL_EXPORT, // 38
    &&LABEL_INHERIT, // 39
    &&LABEL_GETSUPER, // 40
    &&LABEL_GETITER, // 41
    &&LABEL_AND, // 42
    &&LABEL_OR, // 43
    &&LABEL_SLICE, // 44
    &&LABEL_PRINT, // 45
    &&LABEL_TRY, // 46
    &&LABEL_THROW, // 47
    &&LABEL_CATCH, // 48
    &&LABEL_ASYNC, // 49
    &&LABEL_AWAIT, // 50
    &&LABEL_GETFIELD, // 51
    &&LABEL_SETFIELD, // 52
    &&LABEL_ADD_NN, // 53
    &&LABEL_SUB_NN, // 54
    &&LABEL_MUL_NN, // 55
    &&LABEL_DIV_NN, // 56
    &&LABEL_MOD_NN, // 57
    &&LABEL_LT_NN, // 58
    &&LABEL_LE_NN, // 59
    &&LABEL_GETGLOBAL_CACHE, // 60
    &&LABEL_GETTABLE_CACHE, // 61
    &&LABEL_SETTABLE_CACHE, // 62
    &&LABEL_GETFIELD_CACHE, // 63
    &&LABEL_SETFIELD_CACHE, // 64
    &&LABEL_CACHE, // 65
    &&LABEL_CACHE, // 66
    &&LABEL_CACHE, // 67
    &&LABEL_CACHE, // 68
    &&LABEL_CACHE, // 69
    &&LABEL_CACHE, // 70
    &&LABEL_CACHE, // 71
    &&LABEL_CACHE, // 72
    &&LABEL_CACHE, // 73
    &&LABEL_CACHE, // 74
    &&LABEL_CACHE, // 75
    &&LABEL_CACHE, // 76
    &&LABEL_CACHE, // 77
    &&LABEL_CACHE, // 78
    &&LABEL_CACHE, // 79
    &&LABEL_CACHE, // 80
    &&LABEL_CACHE, // 81
    &&LABEL_CACHE, // 82
    &&LABEL_CACHE, // 83
    &&LABEL_CACHE, // 84
    &&LABEL_CACHE, // 85
    &&LABEL_CACHE, // 86
    &&LABEL_CACHE, // 87
    &&LABEL_CACHE, // 88
    &&LABEL_CACHE, // 89
    &&LABEL_CACHE, // 90
    &&LABEL_CACHE, // 91
    &&LABEL_CACHE, // 92
    &&LABEL_CACHE, // 93
    &&LABEL_CACHE, // 94
    &&LABEL_CACHE, // 95
    &&LABEL_CACHE, // 96
    &&LABEL_CACHE, // 97
    &&LABEL_CACHE, // 98
    &&LABEL_CACHE, // 99
    &&LABEL_CACHE, // 100
    &&LABEL_CACHE, // 101
    &&LABEL_CACHE, // 102
    &&LABEL_CACHE, // 103
    &&LABEL_CACHE, // 104
    &&LABEL_CACHE, // 105
    &&LABEL_CACHE, // 106
    &&LABEL_CACHE, // 107
    &&LABEL_CACHE, // 108
    &&LABEL_CACHE, // 109
    &&LABEL_CACHE, // 110
    &&LABEL_CACHE, // 111
    &&LABEL_CACHE, // 112
    &&LABEL_CACHE, // 113
    &&LABEL_CACHE, // 114
    &&LABEL_CACHE, // 115
    &&LABEL_CACHE, // 116
    &&LABEL_CACHE, // 117
    &&LABEL_CACHE, // 118
    &&LABEL_CACHE, // 119
    &&LABEL_CACHE, // 120
    &&LABEL_CACHE, // 121
    &&LABEL_CACHE, // 122
    &&LABEL_CACHE, // 123
    &&LABEL_CACHE, // 124
    &&LABEL_CACHE, // 125
    &&LABEL_CACHE, // 126
    &&LABEL_CACHE, // 127
    &&LABEL_CACHE, // 128
    &&LABEL_CACHE, // 129
    &&LABEL_CACHE, // 130
    &&LABEL_CACHE, // 131
    &&LABEL_CACHE, // 132
    &&LABEL_CACHE, // 133
    &&LABEL_CACHE, // 134
    &&LABEL_CACHE, // 135
    &&LABEL_CACHE, // 136
    &&LABEL_CACHE, // 137
    &&LABEL_CACHE, // 138
    &&LABEL_CACHE, // 139
    &&LABEL_CACHE, // 140
    &&LABEL_CACHE, // 141
    &&LABEL_CACHE, // 142
    &&LABEL_CACHE, // 143
    &&LABEL_CACHE, // 144
    &&LABEL_CACHE, // 145
    &&LABEL_CACHE, // 146
    &&LABEL_CACHE, // 147
    &&LABEL_CACHE, // 148
    &&LABEL_CACHE, // 149
    &&LABEL_CACHE, // 150
    &&LABEL_CACHE, // 151
    &&LABEL_CACHE, // 152
    &&LABEL_CACHE, // 153
    &&LABEL_CACHE, // 154
    &&LABEL_CACHE, // 155
    &&LABEL_CACHE, // 156
    &&LABEL_CACHE, // 157
    &&LABEL_CACHE, // 158
    &&LABEL_CACHE, // 159
    &&LABEL_CACHE, // 160
    &&LABEL_CACHE, // 161
    &&LABEL_CACHE, // 162
    &&LABEL_CACHE, // 163
    &&LABEL_CACHE, // 164
    &&LABEL_CACHE, // 165
    &&LABEL_CACHE, // 166
    &&LABEL_CACHE, // 167
    &&LABEL_CACHE, // 168
    &&LABEL_CACHE, // 169
    &&LABEL_CACHE, // 170
    &&LABEL_CACHE, // 171
    &&LABEL_CACHE, // 172
    &&LABEL_CACHE, // 173
    &&LABEL_CACHE, // 174
    &&LABEL_CACHE, // 175
    &&LABEL_CACHE, // 176
    &&LABEL_CACHE, // 177
    &&LABEL_CACHE, // 178
    &&LABEL_CACHE, // 179
    &&LABEL_CACHE, // 180
    &&LABEL_CACHE, // 181
    &&LABEL_CACHE, // 182
    &&LABEL_CACHE, // 183
    &&LABEL_CACHE, // 184
    &&LABEL_CACHE, // 185
    &&LABEL_CACHE, // 186
    &&LABEL_CACHE, // 187
    &&LABEL_CACHE, // 188
    &&LABEL_CACHE, // 189
    &&LABEL_CACHE, // 190
    &&LABEL_CACHE, // 191
    &&LABEL_CACHE, // 192
    &&LABEL_CACHE, // 193
    &&LABEL_CACHE, // 194
    &&LABEL_CACHE, // 195
    &&LABEL_CACHE, // 196
    &&LABEL_CACHE, // 197
    &&LABEL_CACHE, // 198
    &&LABEL_CACHE, // 199
    &&LABEL_CACHE, // 200
    &&LABEL_CACHE, // 201
    &&LABEL_CACHE, // 202
    &&LABEL_CACHE, // 203
    &&LABEL_CACHE, // 204
    &&LABEL_CACHE, // 205
    &&LABEL_CACHE, // 206
    &&LABEL_CACHE, // 207
    &&LABEL_CACHE, // 208
    &&LABEL_CACHE, // 209
    &&LABEL_CACHE, // 210
    &&LABEL_CACHE, // 211
    &&LABEL_CACHE, // 212
    &&LABEL_CACHE, // 213
    &&LABEL_CACHE, // 214
    &&LABEL_CACHE, // 215
    &&LABEL_CACHE, // 216
    &&LABEL_CACHE, // 217
    &&LABEL_CACHE, // 218
    &&LABEL_CACHE, // 219
    &&LABEL_CACHE, // 220
    &&LABEL_CACHE, // 221
    &&LABEL_CACHE, // 222
    &&LABEL_CACHE, // 223
    &&LABEL_CACHE, // 224
    &&LABEL_CACHE, // 225
    &&LABEL_CACHE, // 226
    &&LABEL_CACHE, // 227
    &&LABEL_CACHE, // 228
    &&LABEL_CACHE, // 229
    &&LABEL_CACHE, // 230
    &&LABEL_CACHE, // 231
    &&LABEL_CACHE, // 232
    &&LABEL_CACHE, // 233
    &&LABEL_CACHE, // 234
    &&LABEL_CACHE, // 235
    &&LABEL_CACHE, // 236
    &&LABEL_CACHE, // 237
    &&LABEL_CACHE, // 238
    &&LABEL_CACHE, // 239
    &&LABEL_CACHE, // 240
    &&LABEL_CACHE, // 241
    &&LABEL_CACHE, // 242
    &&LABEL_CACHE, // 243
    &&LABEL_CACHE, // 244
    &&LABEL_CACHE, // 245
    &&LABEL_CACHE, // 246
    &&LABEL_CACHE, // 247
    &&LABEL_CACHE, // 248
    &&LABEL_CACHE, // 249
    &&LABEL_CACHE, // 250
    &&LABEL_CACHE, // 251
    &&LABEL_CACHE, // 252
    &&LABEL_CACHE, // 253
    &&LABEL_CACHE, // 254
    &&LABEL_CACHE // 255
};        

            #define DISPATCH() \
                do { \
                    instruction = *ip++; \
                    a = (instruction >> 8) & 0xFF; \
                    b = (instruction >> 16) & 0xFF; \
                    c = (instruction >> 24) & 0xFF; \
                    bx = (instruction >> 16) & 0xFFFF; \
                    sbx = (int)bx - 32767; \
                    goto *dispatchTable[static_cast<uint8_t>(instruction & 0xFF)]; \
                } while (false)

        DISPATCH();
#else
            #define DISPATCH() break
            while (true) {
                instruction = *ip++;
                a = (instruction >> 8) & 0xFF;
                b = (instruction >> 16) & 0xFF;
                c = (instruction >> 24) & 0xFF;
                bx = (instruction >> 16) & 0xFFFF;
                sbx = (int)bx - 32767;
                OpCode op = static_cast<OpCode>(instruction & 0xFF);
                switch (op) {
#endif

        LABEL_MOVE: {
            #ifndef COMPUTED_GOTO
            case OpCode::MOVE:
            #endif
            R[a] = R[b];
            DISPATCH();
        }

        LABEL_LOADK: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADK:
            #endif
            {
                PomeValue val = K[bx];
                if (val.isClass()) {
                    val.asClass()->module = currentModule;
                    for (auto const& [name, method] : val.asClass()->methods) {
                        method->module = currentModule;
                    }
                }
                R[a] = val;
            }
            DISPATCH();
        }

        LABEL_LOADBOOL: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADBOOL:
            #endif
            R[a] = PomeValue(b != 0);
            if (c != 0) ip++;
            DISPATCH();
        }

        LABEL_LOADNIL: {
            #ifndef COMPUTED_GOTO
            case OpCode::LOADNIL:
            #endif
            {
                for (int i = 0; i <= b; ++i) R[a + i] = PomeValue();
            }
            DISPATCH();
        }

        LABEL_GETUPVAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETUPVAL:
            #endif
            R[a] = *currentFrame->function->upvalues[b]->location;
            DISPATCH();
        }

        LABEL_SETUPVAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETUPVAL:
            #endif
            *currentFrame->function->upvalues[b]->location = R[a];
            DISPATCH();
        }

            LABEL_CLOSURE: {
            #ifndef COMPUTED_GOTO
            case OpCode::CLOSURE:
            #endif
            {
                PomeFunction* proto = K[bx].asPomeFunction();
                PomeFunction* closure = gc.allocate<PomeFunction>();
                closure->name = proto->name;
                closure->parameters = proto->parameters;
                closure->chunk = proto->chunk;
                closure->upvalueCount = proto->upvalueCount;
                closure->isAsync = proto->isAsync;
                closure->module = currentModule; 
                proto->module = currentModule; 

                for (int i = 0; i < closure->upvalueCount; ++i) {
                    Instruction uvMeta = *ip++;
                    OpCode op = static_cast<OpCode>(uvMeta & 0xFF);
                    int uvIdx = (uvMeta >> 16) & 0xFF;  // field B: bits 16-23
                    if (op == OpCode::MOVE) {
                        closure->upvalues.push_back(captureUpvalue(&R[uvIdx]));
                    } else {
                        closure->upvalues.push_back(currentFrame->function->upvalues[uvIdx]);
                    }
                }
                R[a] = PomeValue(closure);
            }
            DISPATCH();
        }

        LABEL_GETGLOBAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETGLOBAL:
            #endif
            {
                PomeValue key = K[bx];
                
                if (currentModule) {
                    auto it = currentModule->variables.find(key);
                    if (it != currentModule->variables.end()) {
                        R[a] = it->second;
                        int offset = (int)(ip - 1 - currentFrame->chunk->code.data());
                        auto& meta = currentFrame->chunk->metadata[offset];
                        meta.globalCache = it->second;   // cache by value (pointer-safe)
                        meta.globalCacheValid = true;
                        *(ip - 1) = Chunk::makeABx(OpCode::GETGLOBAL_CACHE, a, bx);
                        DISPATCH();
                    }
                }

                auto it = globals.find(key);
                if (it != globals.end()) {
                    R[a] = it->second;
                    int offset = (int)(ip - 1 - currentFrame->chunk->code.data());
                    auto& meta = currentFrame->chunk->metadata[offset];
                    meta.globalCache = it->second;       // cache by value (pointer-safe)
                    meta.globalCacheValid = true;
                    *(ip - 1) = Chunk::makeABx(OpCode::GETGLOBAL_CACHE, a, bx);
                } else {
                    R[a] = PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_GETGLOBAL_CACHE: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETGLOBAL_CACHE:
            #endif
            {
                int offset = (int)(ip - 1 - currentFrame->chunk->code.data());
                auto& meta = currentFrame->chunk->metadata[offset];
                if (meta.globalCacheValid) {
                    R[a] = meta.globalCache;
                } else {
                    *(ip - 1) = Chunk::makeABx(OpCode::GETGLOBAL, a, bx);
                    goto LABEL_GETGLOBAL;
                }
            }
            DISPATCH();
        }

        LABEL_SETGLOBAL: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETGLOBAL:
            #endif
            {
                PomeValue key = K[bx];
                if (currentModule) {
                    currentModule->variables[key] = R[a];
                    gc.writeBarrier(currentModule, R[a]);
                } else {
                    globals[key] = R[a];
                }
            }
            DISPATCH();
        }

        LABEL_GETTABLE: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETTABLE:
            #endif
            {
                PomeValue obj = R[b];
                PomeValue key = R[c];
                if (obj.isTable()) {
                    R[a] = obj.asTable()->get(key);
                } else if (obj.isList()) {
                    if (key.isNumber()) {
                        int idx = (int)key.asNumber();
                        PomeList* list = obj.asList();
                        PomeValue res;
                        if (list->isUnboxed) {
                            if (idx >= 0 && idx < (int)list->unboxedElements.size()) {
                                res = PomeValue(list->unboxedElements[idx]);
                                *(ip - 1) = Chunk::makeABC(OpCode::GETLIST_N, a, b, c);
                            }
                        } else {
                            if (idx >= 0 && idx < (int)list->elements.size()) {
                                res = list->elements[idx];
                                *(ip - 1) = Chunk::makeABC(OpCode::GETTABLE_CACHE, a, b, c);
                            }
                        }
                        if (res.isList()) {
                             std::cout << "[GETTABLE] WARNING: Retrieved LIST from list at idx=" << idx << " RSS=" << GarbageCollector::getRSS() << "KB" << std::endl;
                        }
                        R[a] = res;
                        DISPATCH();
                    } else {
                        R[a] = PomeValue();
                        DISPATCH();
                    }
                } else if (obj.isString()) {
                    if (key.isNumber()) {
                        int idx = (int)key.asNumber();
                        const std::string& s = obj.asString();
                        if (idx >= 0 && idx < (int)s.length()) {
                            unsigned char ch = (unsigned char)s[idx];
                            if (!charCache[ch]) {
                                std::string charStr = "";
                                charStr += (char)ch;
                                charCache[ch] = gc.allocate<PomeString>(charStr);
                            }
                            R[a] = PomeValue(charCache[ch]);
                        } else {
                            R[a] = PomeValue();
                        }
                    } else {
                        R[a] = PomeValue();
                    }
                } else if (obj.isInstance()) {
                    PomeInstance* inst = obj.asInstance();
                    PomeValue res = inst->get(key);
                    if (res.isNil()) {
                        PomeFunction* method = inst->klass->findMethod(key.toString());
                        if (method) res = PomeValue(method);
                    }
                    R[a] = res;
                } else if (obj.isModule()) {
                    PomeModule* mod = obj.asModule();
                    auto it = mod->exports.find(key);
                    if (it != mod->exports.end()) {
                        R[a] = it->second;
                    } else {
                        // std::cout << "Module export not found: '" << key.toString() << "' in module " << mod << " (path: " << mod->scriptPath << ")" << std::endl;
                        // std::cout << "Available exports: ";
                        // for(auto const& [ekey, eval] : mod->exports) std::cout << ekey.toString() << " ";
                        // std::cout << std::endl;
                        R[a] = PomeValue();
                    }
                } else {
                    SAVE_FRAME();
                    runtimeError("Property access on non-object.");
                    return PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_GETFIELD_CACHE: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETFIELD_CACHE:
            #endif
            {
                PomeValue obj = R[b];
                if (obj.isInstance()) {
                    PomeInstance* inst = obj.asInstance();
                    int offset = (int)(ip - 1 - currentFrame->chunk->code.data());
                    auto& meta = currentFrame->chunk->metadata[offset];
                    if (inst->klass == meta.klassCache) {
                        if (meta.indexCache >= 0 && meta.indexCache < (int)inst->properties.size()) {
                            R[a] = inst->properties[meta.indexCache];
                            DISPATCH();
                        }
 else if (meta.objectCache) {
                            R[a] = PomeValue(meta.objectCache);
                            DISPATCH();
                        }
                    }
                }
                *(ip - 1) = Chunk::makeABC(OpCode::GETTABLE, a, b, c);
                goto LABEL_GETTABLE;
            }
        }

        LABEL_GETTABLE_CACHE: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETTABLE_CACHE:
            #endif
            {
                PomeValue obj = R[b];
                PomeValue key = R[c];
                if (obj.isList() && key.isNumber()) {
                    PomeList* list = obj.asList();
                    int idx = (int)key.asNumber();
                    if (!list->isUnboxed) {
                        if (idx >= 0 && idx < (int)list->elements.size()) {
                            R[a] = list->elements[idx];
                            DISPATCH();
                        }
                    } else {
                        if (idx >= 0 && idx < (int)list->unboxedElements.size()) {
                            R[a] = PomeValue(list->unboxedElements[idx]);
                            DISPATCH();
                        }
                    }
                }
                *(ip - 1) = Chunk::makeABC(OpCode::GETTABLE, a, b, c);
                goto LABEL_GETTABLE;
            }
        }

        LABEL_GETLIST_N: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETLIST_N:
            #endif
            {
                PomeValue obj = R[b];
                PomeValue key = R[c];
                if (obj.isList() && key.isNumber()) {
                    PomeList* list = obj.asList();
                    int idx = (int)key.asNumber();
                    if (list->isUnboxed) {
                        if (idx >= 0 && idx < (int)list->unboxedElements.size()) {
                            R[a] = PomeValue(list->unboxedElements[idx]);
                            DISPATCH();
                        }
                    } else {
                        if (idx >= 0 && idx < (int)list->elements.size()) {
                            R[a] = list->elements[idx];
                            DISPATCH();
                        }
                    }
                }
                *(ip - 1) = Chunk::makeABC(OpCode::GETTABLE, a, b, c);
                goto LABEL_GETTABLE;
            }
        }

        LABEL_SETTABLE_CACHE: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETTABLE_CACHE:
            #endif
            {
                PomeValue obj = R[a];
                PomeValue key = R[b];
                PomeValue val = R[c];
                if (obj.isList() && key.isNumber()) {
                    PomeList* list = obj.asList();
                    int idx = (int)key.asNumber();
                    if (!list->isUnboxed) {
                        if (idx >= 0 && idx < (int)list->elements.size()) {
                            list->elements[idx] = val;
                            gc.writeBarrier(obj.asObject(), val);
                            DISPATCH();
                        } else if (idx == (int)list->elements.size()) {
                            size_t oldSize = list->extraSize();
                            list->push(val);
                            gc.updateSize(obj.asObject(), sizeof(PomeList) + oldSize, sizeof(PomeList) + list->extraSize());
                            gc.writeBarrier(obj.asObject(), val);
                            DISPATCH();
                        }
                    } else {
                        if (idx >= 0 && idx < (int)list->unboxedElements.size()) {
                            if (val.isNumber()) {
                                list->unboxedElements[idx] = val.asNumber();
                                DISPATCH();
                            } else {
                                list->box();
                                list->elements[idx] = val;
                                gc.writeBarrier(obj.asObject(), val);
                                DISPATCH();
                            }
                        } else if (idx == (int)list->unboxedElements.size()) {
                            size_t oldSize = list->extraSize();
                            list->push(val);
                            gc.updateSize(obj.asObject(), sizeof(PomeList) + oldSize, sizeof(PomeList) + list->extraSize());
                            gc.writeBarrier(obj.asObject(), val);
                            DISPATCH();
                        }
                    }
                }
                *(ip - 1) = Chunk::makeABC(OpCode::SETTABLE, a, b, c);
                goto LABEL_SETTABLE;
            }
        }

        LABEL_SETLIST_N: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETLIST_N:
            #endif
            {
                PomeValue obj = R[a];
                PomeValue key = R[b];
                PomeValue val = R[c];
                if (obj.isList() && key.isNumber()) {
                    PomeList* list = obj.asList();
                    int idx = (int)key.asNumber();
                    if (list->isUnboxed) {
                        if (idx >= 0 && idx < (int)list->unboxedElements.size()) {
                            if (val.isNumber()) {
                                list->unboxedElements[idx] = val.asNumber();
                                DISPATCH();
                            } else {
                                list->box();
                                list->elements[idx] = val;
                                gc.writeBarrier(obj.asObject(), val);
                                DISPATCH();
                            }
                        } else if (idx == (int)list->unboxedElements.size()) {
                            size_t oldSize = list->extraSize();
                            list->push(val);
                            gc.updateSize(obj.asObject(), sizeof(PomeList) + oldSize, sizeof(PomeList) + list->extraSize());
                            gc.writeBarrier(obj.asObject(), val);
                            DISPATCH();
                        }
                    } else {
                        if (idx >= 0 && idx < (int)list->elements.size()) {
                            list->elements[idx] = val;
                            gc.writeBarrier(obj.asObject(), val);
                            DISPATCH();
                        } else if (idx == (int)list->elements.size()) {
                            size_t oldSize = list->extraSize();
                            list->push(val);
                            gc.updateSize(obj.asObject(), sizeof(PomeList) + oldSize, sizeof(PomeList) + list->extraSize());
                            gc.writeBarrier(obj.asObject(), val);
                            DISPATCH();
                        }
                    }
                }
                *(ip - 1) = Chunk::makeABC(OpCode::SETTABLE, a, b, c);
                goto LABEL_SETTABLE;
            }
        }

        LABEL_SETFIELD_CACHE:
        LABEL_CACHE: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETFIELD_CACHE:
            case OpCode::CACHE:
            #endif
            DISPATCH();
        }

        LABEL_SETTABLE: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETTABLE:
            #endif
            {
                PomeValue obj = R[a];
                PomeValue key = R[b];
                PomeValue val = R[c];
                if (obj.isTable()) {
                    obj.asTable()->set(key, val);
                    gc.writeBarrier(obj.asObject(), val);
                } else if (obj.isInstance()) {
                    PomeInstance* inst = obj.asInstance();
                    int index = inst->shape->getIndex(key);
                    if (index >= 0) {
                        if (index >= (int)inst->properties.size()) inst->properties.resize(index + 1);
                        inst->properties[index] = val;
                    } else {
                        inst->shape = inst->shape->transition(gc, key);
                        inst->properties.push_back(val);
                    }
                    gc.writeBarrier(inst, val);
                } else if (obj.isList()) {
                    if (key.isNumber()) {
                        int idx = (int)key.asNumber();
                        PomeList* list = obj.asList();
                        size_t oldSize = list->extraSize();
                        if (idx >= 0 && idx < (int)(list->isUnboxed ? list->unboxedElements.size() : list->elements.size())) {
                            if (list->isUnboxed) {
                                if (val.isNumber()) {
                                    list->unboxedElements[idx] = val.asNumber();
                                } else {
                                    list->box();
                                    list->elements[idx] = val;
                                    gc.writeBarrier(obj.asObject(), val);
                                }
                            } else {
                                list->elements[idx] = val;
                                gc.writeBarrier(obj.asObject(), val);
                            }
                            DISPATCH();
                        } else if (idx == (int)(list->isUnboxed ? list->unboxedElements.size() : list->elements.size())) {
                            list->push(val);
                            gc.updateSize(obj.asObject(), sizeof(PomeList) + oldSize, sizeof(PomeList) + list->extraSize());
                            gc.writeBarrier(obj.asObject(), val);
                            DISPATCH();
                        }
                    }
                    DISPATCH();
                } else if (obj.isModule()) {
                    obj.asModule()->exports[key] = val;
                    gc.writeBarrier(obj.asObject(), val);
                } else {
                    SAVE_FRAME();
                    runtimeError("Cannot set property on non-object.");
                    return PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_NEWLIST: {
            #ifndef COMPUTED_GOTO
            case OpCode::NEWLIST:
            #endif
            {
                PomeList* list = gc.allocateList();
                if (b > 0) {
                    list->ensureCapacity(b);
                    for (int i = 0; i < b; ++i) {
                        PomeValue val = R[a + 1 + i];
                        list->push(val);
                        gc.writeBarrier(list, val);
                    }
                    gc.updateSize(list, sizeof(PomeList), sizeof(PomeList) + list->extraSize());
                }
                R[a] = PomeValue(list); 
            }
            DISPATCH();
        }

        LABEL_LIST_ADD_SCALAR: {
            #ifndef COMPUTED_GOTO
            case OpCode::LIST_ADD_SCALAR:
            #endif
            {
                PomeValue listVal = R[b];
                PomeValue scalarVal = R[c];
                if (listVal.isList() && scalarVal.isNumber()) {
                    PomeList* list = listVal.asList();
                    double scalar = scalarVal.asNumber();
                    if (list->isUnboxed) {
                        for (double& d : list->unboxedElements) {
                            d += scalar;
                        }
                    } else {
                        for (auto& val : list->elements) {
                            if (val.isNumber()) val = PomeValue(val.asNumber() + scalar);
                        }
                        list->tryUnbox();
                    }
                    R[a] = listVal;
                }
            }
            DISPATCH();
        }

        LABEL_LIST_SUM: {
            #ifndef COMPUTED_GOTO
            case OpCode::LIST_SUM:
            #endif
            {
                PomeValue listVal = R[b];
                if (listVal.isList()) {
                    PomeList* list = listVal.asList();
                    double sum = 0;
                    if (list->isUnboxed) {
                        for (double d : list->unboxedElements) sum += d;
                    } else {
                        for (auto& val : list->elements) {
                            if (val.isNumber()) sum += val.asNumber();
                        }
                    }
                    R[a] = PomeValue(sum);
                }
            }
            DISPATCH();
        }

        LABEL_NEWTABLE: {
            #ifndef COMPUTED_GOTO
            case OpCode::NEWTABLE:
            #endif
            R[a] = PomeValue(gc.allocate<PomeTable>(rootShape));
            DISPATCH();
        }

        LABEL_ADD: {
            #ifndef COMPUTED_GOTO
            case OpCode::ADD:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    *(ip - 1) = Chunk::makeABC(OpCode::ADD_NN, a, b, c);
                    R[a] = PomeValue(v1.asNumber() + v2.asNumber());
                } else if (v1.isInstance()) {
                    PomeInstance* instance = v1.asInstance();
                    PomeFunction* method = instance->klass->findMethod("__add__");
                    if (method) {
                        SAVE_FRAME();
                        if (frameCount >= 8192) runtimeError("Stack overflow");
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = method;
                        nextFrame->module = method->module;
                        nextFrame->chunk = method->chunk.get();
                        nextFrame->ip = method->chunk->code.data();
                        nextFrame->base = frameBase + a + 3;
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

        LABEL_ADD_NN: {
            #ifndef COMPUTED_GOTO
            case OpCode::ADD_NN:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(v1.asNumber() + v2.asNumber());
                } else {
                    *(ip - 1) = Chunk::makeABC(OpCode::ADD, a, b, c);
                    goto LABEL_ADD;
                }
            }
            DISPATCH();
        }

        LABEL_SUB: {
            #ifndef COMPUTED_GOTO
            case OpCode::SUB:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    *(ip - 1) = Chunk::makeABC(OpCode::SUB_NN, a, b, c);
                    R[a] = PomeValue(v1.asNumber() - v2.asNumber());
                } else if (v1.isInstance()) {
                    PomeInstance* instance = v1.asInstance();
                    PomeFunction* method = instance->klass->findMethod("__sub__");
                    if (method) {
                        SAVE_FRAME();
                        if (frameCount >= 8192) runtimeError("Stack overflow");
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = method;
                        nextFrame->module = method->module;
                        nextFrame->chunk = method->chunk.get();
                        nextFrame->ip = method->chunk->code.data();
                        nextFrame->base = frameBase + a + 3;
                        nextFrame->destReg = a;
                        stack[nextFrame->base] = PomeValue(method);
                        stack[nextFrame->base + 1] = v1;
                        stack[nextFrame->base + 2] = v2;
                        REFRESH_FRAME();
                        DISPATCH();
                    } else {
                        SAVE_FRAME();
                        runtimeError("Arithmetic on non-number.");
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

        LABEL_SUB_NN: {
            #ifndef COMPUTED_GOTO
            case OpCode::SUB_NN:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(v1.asNumber() - v2.asNumber());
                } else {
                    *(ip - 1) = Chunk::makeABC(OpCode::SUB, a, b, c);
                    goto LABEL_SUB;
                }
            }
            DISPATCH();
        }

        LABEL_MUL: {
            #ifndef COMPUTED_GOTO
            case OpCode::MUL:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    *(ip - 1) = Chunk::makeABC(OpCode::MUL_NN, a, b, c);
                    R[a] = PomeValue(v1.asNumber() * v2.asNumber());
                } else if (v1.isInstance()) {
                    PomeInstance* instance = v1.asInstance();
                    PomeFunction* method = instance->klass->findMethod("__mul__");
                    if (method) {
                        SAVE_FRAME();
                        if (frameCount >= 8192) runtimeError("Stack overflow");
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = method;
                        nextFrame->module = method->module;
                        nextFrame->chunk = method->chunk.get();
                        nextFrame->ip = method->chunk->code.data();
                        nextFrame->base = frameBase + a + 3;
                        nextFrame->destReg = a;
                        stack[nextFrame->base] = PomeValue(method);
                        stack[nextFrame->base + 1] = v1;
                        stack[nextFrame->base + 2] = v2;
                        REFRESH_FRAME();
                        DISPATCH();
                    } else {
                        SAVE_FRAME();
                        runtimeError("Arithmetic on non-number.");
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

        LABEL_MUL_NN: {
            #ifndef COMPUTED_GOTO
            case OpCode::MUL_NN:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(v1.asNumber() * v2.asNumber());
                } else {
                    *(ip - 1) = Chunk::makeABC(OpCode::MUL, a, b, c);
                    goto LABEL_MUL;
                }
            }
            DISPATCH();
        }

        LABEL_DIV: {
            #ifndef COMPUTED_GOTO
            case OpCode::DIV:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    if (v2.asNumber() == 0.0) {
                        SAVE_FRAME();
                        runtimeError("Division by zero.");
                        return PomeValue();
                    }
                    *(ip - 1) = Chunk::makeABC(OpCode::DIV_NN, a, b, c);
                    R[a] = PomeValue(v1.asNumber() / v2.asNumber());
                } else if (v1.isInstance()) {
                    PomeInstance* instance = v1.asInstance();
                    PomeFunction* method = instance->klass->findMethod("__div__");
                    if (method) {
                        SAVE_FRAME();
                        if (frameCount >= 8192) runtimeError("Stack overflow");
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = method;
                        nextFrame->module = method->module;
                        nextFrame->chunk = method->chunk.get();
                        nextFrame->ip = method->chunk->code.data();
                        nextFrame->base = frameBase + a + 3;
                        nextFrame->destReg = a;
                        stack[nextFrame->base] = PomeValue(method);
                        stack[nextFrame->base + 1] = v1;
                        stack[nextFrame->base + 2] = v2;
                        REFRESH_FRAME();
                        DISPATCH();
                    } else {
                        SAVE_FRAME();
                        runtimeError("Arithmetic on non-number.");
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

        LABEL_DIV_NN: {
            #ifndef COMPUTED_GOTO
            case OpCode::DIV_NN:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(v1.asNumber() / v2.asNumber());
                } else {
                    *(ip - 1) = Chunk::makeABC(OpCode::DIV, a, b, c);
                    goto LABEL_DIV;
                }
            }
            DISPATCH();
        }

        LABEL_MOD: {
            #ifndef COMPUTED_GOTO
            case OpCode::MOD:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    *(ip - 1) = Chunk::makeABC(OpCode::MOD_NN, a, b, c);
                    R[a] = PomeValue(std::fmod(v1.asNumber(), v2.asNumber()));
                } else {
                    SAVE_FRAME();
                    runtimeError("Arithmetic on non-number.");
                    return PomeValue();
                }
            }
            DISPATCH();
        }

        LABEL_MOD_NN: {
            #ifndef COMPUTED_GOTO
            case OpCode::MOD_NN:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(std::fmod(v1.asNumber(), v2.asNumber()));
                } else {
                    *(ip - 1) = Chunk::makeABC(OpCode::MOD, a, b, c);
                    goto LABEL_MOD;
                }
            }
            DISPATCH();
        }

        LABEL_POW: {
            #ifndef COMPUTED_GOTO
            case OpCode::POW:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
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
                PomeValue v1 = R[b];
                if (v1.isNumber()) {
                    R[a] = PomeValue(-v1.asNumber());
                } else if (v1.isInstance()) {
                    PomeInstance* instance = v1.asInstance();
                    PomeFunction* method = instance->klass->findMethod("__neg__");
                    if (method) {
                        SAVE_FRAME();
                        if (frameCount >= 8192) runtimeError("Stack overflow");
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = method;
                        nextFrame->module = method->module;
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
                }
            }
            DISPATCH();
        }

        LABEL_NOT: {
            #ifndef COMPUTED_GOTO
            case OpCode::NOT:
            #endif
            R[a] = PomeValue(!R[b].asBool());
            DISPATCH();
        }

        LABEL_LEN: {
            #ifndef COMPUTED_GOTO
            case OpCode::LEN:
            #endif
            {
                PomeValue v = R[b];
                if (v.isString()) R[a] = PomeValue((double)v.asString().length());
                else if (v.isList()) R[a] = PomeValue((double)(v.asList()->isUnboxed ? v.asList()->unboxedElements.size() : v.asList()->elements.size()));
                else if (v.isTable()) R[a] = PomeValue((double)(v.asTable()->properties.size() + v.asTable()->backfill.size()));
            }
            DISPATCH();
        }

        LABEL_CONCAT: {
            #ifndef COMPUTED_GOTO
            case OpCode::CONCAT:
            #endif
            {
                const PomeValue& v1 = R[b];
                const PomeValue& v2 = R[c];
                if (v1.isString() && v2.isString()) {
                    const std::string& s1 = v1.asString();
                    const std::string& s2 = v2.asString();
                    std::string res;
                    res.reserve(s1.size() + s2.size());
                    res.append(s1);
                    res.append(s2);
                    R[a] = PomeValue(gc.allocate<PomeString>(std::move(res)));
                } else {
                    R[a] = PomeValue(gc.allocate<PomeString>(v1.toString() + v2.toString()));
                }
            }
            DISPATCH();
        }

        LABEL_JMP: {
            #ifndef COMPUTED_GOTO
            case OpCode::JMP:
            #endif
            ip += sbx;
            DISPATCH();
        }

        LABEL_EQ: {
            #ifndef COMPUTED_GOTO
            case OpCode::EQ:
            #endif
            R[a] = PomeValue(R[b] == R[c]);
            DISPATCH();
        }
        LABEL_LT: {
            #ifndef COMPUTED_GOTO
            case OpCode::LT:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    *(ip - 1) = Chunk::makeABC(OpCode::LT_NN, a, b, c);
                    R[a] = PomeValue(v1.asNumber() < v2.asNumber());
                } else if (v1.isInstance()) {
                    PomeInstance* instance = v1.asInstance();
                    PomeFunction* method = instance->klass->findMethod("__lt__");
                    if (method) {
                        SAVE_FRAME();
                        if (frameCount >= 8192) runtimeError("Stack overflow");
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = method;
                        nextFrame->module = method->module;
                        nextFrame->chunk = method->chunk.get();
                        nextFrame->ip = method->chunk->code.data();
                        nextFrame->base = frameBase + a + 3;
                        nextFrame->destReg = a;
                        stack[nextFrame->base] = PomeValue(method);
                        stack[nextFrame->base + 1] = v1;
                        stack[nextFrame->base + 2] = v2;
                        REFRESH_FRAME();
                        DISPATCH();
                    } else {
                        R[a] = PomeValue(v1 < v2);
                    }
                } else {
                    R[a] = PomeValue(v1 < v2);
                }
            }
            DISPATCH();
        }

        LABEL_LT_NN: {
            #ifndef COMPUTED_GOTO
            case OpCode::LT_NN:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(v1.asNumber() < v2.asNumber());
                } else {
                    *(ip - 1) = Chunk::makeABC(OpCode::LT, a, b, c);
                    goto LABEL_LT;
                }
            }
            DISPATCH();
        }

        LABEL_LE: {
            #ifndef COMPUTED_GOTO
            case OpCode::LE:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    *(ip - 1) = Chunk::makeABC(OpCode::LE_NN, a, b, c);
                    R[a] = PomeValue(v1.asNumber() <= v2.asNumber());
                } else {
                    R[a] = PomeValue(v1 <= v2);
                }
            }
            DISPATCH();
        }

        LABEL_LE_NN: {
            #ifndef COMPUTED_GOTO
            case OpCode::LE_NN:
            #endif
            {
                PomeValue v1 = R[b];
                PomeValue v2 = R[c];
                if (v1.isNumber() && v2.isNumber()) {
                    R[a] = PomeValue(v1.asNumber() <= v2.asNumber());
                } else {
                    *(ip - 1) = Chunk::makeABC(OpCode::LE, a, b, c);
                    goto LABEL_LE;
                }
            }
            DISPATCH();
        }

        LABEL_TEST: {
            #ifndef COMPUTED_GOTO
            case OpCode::TEST:
            #endif
            if (R[a].asBool() == (c != 0)) {
                ip++;
            }
            DISPATCH();
        }

        LABEL_TESTSET: {
            #ifndef COMPUTED_GOTO
            case OpCode::TESTSET:
            #endif
            if (R[b].asBool() == (c != 0)) {
                R[a] = R[b];
            } else {
                ip++;
            }
            DISPATCH();
        }

        LABEL_CALL: {
            #ifndef COMPUTED_GOTO
            case OpCode::CALL:
            #endif
            {
                PomeValue callee = R[a];
                int argCount = b - 1;
                if (callee.isNativeFunction()) {
                    int offset = (int)(ip - 1 - currentFrame->chunk->code.data());
                    auto& meta = currentFrame->chunk->metadata[offset];
                    
                    if (meta.objectCache != callee.asObject()) {
                        meta.objectCache = callee.asObject();
                        const std::string& name = callee.asNativeFunction()->getName();
                        if (name == "len") meta.indexCache = 1;
                        else if (name == "push") meta.indexCache = 2;
                        else meta.indexCache = 0;
                    }
                    
                    if (meta.indexCache == 1 && argCount >= 1) {
                        int valIdx = a + 1;
                        if (R[valIdx].isModule() && argCount == 2) valIdx++;
                        PomeValue v = R[valIdx];
                        if (v.isList()) R[a] = PomeValue((double)(v.asList()->isUnboxed ? v.asList()->unboxedElements.size() : v.asList()->elements.size()));
                        else if (v.isString()) R[a] = PomeValue((double)v.asString().length());
                        else if (v.isTable()) R[a] = PomeValue((double)(v.asTable()->properties.size() + v.asTable()->backfill.size()));
                        else R[a] = PomeValue();
                        DISPATCH();
                    } else if (meta.indexCache == 2 && argCount >= 2) {
                        int lstIdx = a + 1;
                        int valIdx = a + 2;
                        if (R[lstIdx].isModule() && argCount == 3) {
                            lstIdx++;
                            valIdx++;
                        }
                        PomeValue lstVal = R[lstIdx];
                        PomeValue val = R[valIdx];
                        if (lstVal.isList()) {
                            PomeList* lst = lstVal.asList();
                            size_t oldExtra = lst->extraSize();
                            lst->push(val);
                            gc.updateSize(lst, sizeof(PomeList) + oldExtra, sizeof(PomeList) + lst->extraSize());
                            gc.writeBarrier(lst, val);
                            R[a] = val;
                            DISPATCH();
                        }
                    }
                    
                    args.clear();
                    for (int i = 1; i <= argCount; ++i) args.push_back(R[a + i]);
                    SAVE_FRAME();
                    PomeValue res = callee.asNativeFunction()->call(args);
                    REFRESH_FRAME(); 
                    R[a] = res;
                    DISPATCH();
                } else if (callee.isPomeFunction()) {
                    PomeFunction* func = callee.asPomeFunction();
                    int nextFrameBase = frameBase + a;

                    // Zero out registers for the new frame (avoid garbage from previous calls)
                    for (int i = argCount + 1; i < 64; ++i) {
                        stack[nextFrameBase + i] = PomeValue();
                    }

                    if (argCount < (int)func->parameters.size()) {
                        for (int i = argCount; i < (int)func->parameters.size(); ++i) {
                            R[a + 1 + i] = PomeValue();
                        }
                    }
                    if (func->isAsync) {
                        PomeTask* task = gc.allocate<PomeTask>(func);
                        for (int i = 0; i < argCount; ++i) {
                            task->args.push_back(R[a + 1 + i]);
                        }
                        taskQueue.push_back(task);
                        R[a] = PomeValue(task);
                        SAVE_FRAME();
                        DISPATCH();
                    }
                    SAVE_FRAME();
                    if (frameCount >= 8192) runtimeError("Stack overflow");
                    CallFrame* nextFrame = &frames[frameCount++];
                    nextFrame->function = func;
                    nextFrame->module = func->module;
                    nextFrame->chunk = func->chunk.get();
                    nextFrame->ip = func->chunk->code.data();
                    nextFrame->base = nextFrameBase;
                    nextFrame->destReg = a; 
                    nextFrame->task = nullptr;
                    REFRESH_FRAME();
                    DISPATCH();
                } else if (callee.isClass()) {
                    PomeClass* klass = callee.asClass();
                    PomeFunction* init = klass->findMethod("init");

                    // Instant Init Optimization:
                    if (init && !klass->fieldNames.empty() && argCount == (int)klass->fieldNames.size()) {
                        if (!klass->classShape) {
                            PomeShape* s = rootShape;
                            std::vector<std::string> orderedFields(klass->fieldNames.size());
                            for (auto const& [name, index] : klass->fieldNames) {
                                if (index < orderedFields.size()) orderedFields[index] = name;
                            }
                            for (const auto& name : orderedFields) {
                                s = s->transition(gc, PomeValue(gc.allocate<PomeString>(name)));
                            }
                            klass->classShape = s;
                            gc.writeBarrier(klass, PomeValue(s));
                        }
                        
                        PomeInstance* instance = gc.allocate<PomeInstance>(klass, klass->classShape);
                        instance->properties.resize(klass->classShape->propertyIndex + 1);
                        
                        for (auto const& [name, index] : klass->fieldNames) {
                            if (index < (int)instance->properties.size()) {
                                PomeValue val = R[a + 1 + index];
                                instance->properties[index] = val;
                                gc.writeBarrier(instance, val);
                            }
                        }
                        R[a] = PomeValue(instance);
                        DISPATCH();
                    }

                    PomeInstance* instance = gc.allocate<PomeInstance>(klass, rootShape);

                    if (init) {
                        for (int i = argCount; i >= 1; --i) R[a + i + 1] = R[a + i];
                        R[a] = PomeValue(instance); // return value
                        R[a + 1] = PomeValue(instance); // this
                        SAVE_FRAME();
                        if (frameCount >= 8192) runtimeError("Stack overflow");
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = init;
                        nextFrame->module = init->module;
                        nextFrame->chunk = init->chunk.get();
                        nextFrame->ip = init->chunk->code.data();
                        nextFrame->base = frameBase + a;
                        nextFrame->destReg = -1; // Don't overwrite instance in R[a]
                        nextFrame->task = nullptr;
                        REFRESH_FRAME();
                        DISPATCH();
                    } else {
                        R[a] = PomeValue(instance);
                        DISPATCH();
                    }                } else {
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
            {
                int nArgs = b - 1;
                PomeValue callee = R[a];
                if (callee.isPomeFunction()) {
                    PomeFunction* func = callee.asPomeFunction();
                    closeUpvalues(R);
                    for (int i = 0; i <= nArgs; ++i) R[i] = R[a + i];

                    currentFrame->function = func;                    currentFrame->module = func->module;
                    currentFrame->chunk = func->chunk.get();
                    currentFrame->ip = func->chunk->code.data();
                    
                    REFRESH_FRAME();
                    DISPATCH();
                } else {
                    goto LABEL_CALL;
                }
            }
            DISPATCH();
        }

        LABEL_RETURN: {
            #ifndef COMPUTED_GOTO
            case OpCode::RETURN:
            #endif
            {
                PomeValue result = (b == 0) ? PomeValue() : R[a];
                int dest = currentFrame->destReg;
                PomeTask* currentTask = currentFrame->task;
                
                closeUpvalues(R);

                if (currentTask) {
                    currentTask->result = result;
                    currentTask->isCompleted = true;
                }
                frameCount--;
                if (frameCount <= initialFrameIdx) {
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
                double step = R[a+2].asNumber();
                double idx = R[a].asNumber() + step;
                double limit = R[a+1].asNumber();
                if ((step > 0 && idx <= limit) || (step <= 0 && idx >= limit)) {
                    R[a] = PomeValue(idx);
                    R[a+3] = PomeValue(idx);
                    ip += sbx;
                }
            }
            DISPATCH();
        }

        LABEL_FORPREP: {
            #ifndef COMPUTED_GOTO
            case OpCode::FORPREP:
            #endif
            R[a].asNumber(); // Ensure it's a number
            R[a+1].asNumber();
            R[a+2].asNumber();
            R[a] = PomeValue(R[a].asNumber() - R[a+2].asNumber());
            ip += sbx;
            DISPATCH();
        }

        LABEL_TFORCALL: {
            #ifndef COMPUTED_GOTO
            case OpCode::TFORCALL:
            #endif
            {
                PomeValue iterObj = R[b];
                if (iterObj.isInstance() && iterObj.asInstance()->klass->findMethod("next")) {
                    PomeInstance* instance = iterObj.asInstance();
                    PomeFunction* nextMethod = instance->klass->findMethod("next");
                    SAVE_FRAME();
                    if (frameCount >= 8192) runtimeError("Stack overflow");
                    CallFrame* nextFrame = &frames[frameCount++];
                    nextFrame->function = nextMethod;
                    nextFrame->module = nextMethod->module;
                    nextFrame->chunk = nextMethod->chunk.get();
                    nextFrame->ip = nextMethod->chunk->code.data();
                    nextFrame->base = frameBase + a + 2; 
                    nextFrame->destReg = a;
                    stack[nextFrame->base] = PomeValue(nextMethod);
                    stack[nextFrame->base + 1] = iterObj;
                    stack[nextFrame->base + 2] = R[b + 1]; 
                    REFRESH_FRAME();
                    DISPATCH();
                } else if (iterObj.isTable()) {
                    PomeTable* table = iterObj.asTable();
                    std::vector<PomeValue> sortedKeys = table->getSortedKeys();
                    PomeValue state = R[b + 1];
                    int idx = state.isNil() ? 0 : (int)state.asNumber();
                    if (idx < (int)sortedKeys.size()) {
                        PomeValue key = sortedKeys[idx];
                        R[a] = key;
                        R[a + 1] = table->get(key);
                        R[b + 1] = PomeValue((double)(idx + 1));
                    } else {
                        R[a] = PomeValue();
                    }
                } else if (iterObj.isList()) {
                    PomeList* list = iterObj.asList();
                    int size = list->isUnboxed ? (int)list->unboxedElements.size() : (int)list->elements.size();
                    PomeValue lastKey = R[b + 1];
                    int nextIdx = lastKey.isNil() ? 0 : (int)lastKey.asNumber() + 1;
                    if (nextIdx >= 0 && nextIdx < size) {
                        if (list->isUnboxed) {
                            R[a] = PomeValue(list->unboxedElements[nextIdx]); // Element
                        } else {
                            R[a] = list->elements[nextIdx];               // Element
                        }
                        R[a + 1] = PomeValue((double)nextIdx);           // Index (secondary)
                        R[b + 1] = PomeValue((double)nextIdx);           // Update state
                    } else R[a] = PomeValue();
                } else R[a] = PomeValue();
            }
            DISPATCH();
        }

        LABEL_GETFIELD: {
            #ifndef COMPUTED_GOTO
            case OpCode::GETFIELD:
            #endif
            {
                PomeValue obj = R[b];
                PomeValue key = K[c];
                if (obj.isInstance()) {
                    PomeInstance* inst = obj.asInstance();
                    PomeValue res = inst->get(key);
                    if (res.isNil()) {
                        PomeFunction* method = inst->klass->findMethod(key.asString());
                        if (method) res = PomeValue(method);
                    }
                    R[a] = res;
                } else if (obj.isTable()) {
                    R[a] = obj.asTable()->get(key);
                } else if (obj.isModule()) {
                    PomeModule* mod = obj.asModule();
                    auto it = mod->exports.find(key);
                    if (it != mod->exports.end()) {
                        R[a] = it->second;
                    } else {
                        auto it2 = mod->variables.find(key);
                        if (it2 != mod->variables.end()) R[a] = it2->second;
                        else R[a] = PomeValue();
                    }
                } else if (obj.isList()) {
                    if (key.isString() && key.asString() == "len") {
                        R[a] = PomeValue((double)(obj.asList()->isUnboxed ? obj.asList()->unboxedElements.size() : obj.asList()->elements.size()));
                    } else R[a] = PomeValue();
                } else if (obj.isString()) {
                    if (key.isString() && key.asString() == "len") {
                        R[a] = PomeValue((double)obj.asString().length());
                    } else R[a] = PomeValue();
                } else {
                    R[a] = PomeValue(); 
                }
            }
            DISPATCH();
        }

        LABEL_SETFIELD: {
            #ifndef COMPUTED_GOTO
            case OpCode::SETFIELD:
            #endif
            {
                PomeValue obj = R[a];
                PomeValue key = K[c];
                PomeValue val = R[b];
                if (obj.isInstance()) {
                    PomeInstance* inst = obj.asInstance();
                    int index = inst->shape->getIndex(key);
                    if (index >= 0) {
                        if (index >= (int)inst->properties.size()) inst->properties.resize(index + 1);
                        inst->properties[index] = val;
                    } else {
                        inst->shape = inst->shape->transition(gc, key);
                        inst->properties.push_back(val);
                    }
                    gc.writeBarrier(inst, val);
                } else if (obj.isTable()) {
                    PomeTable* tbl = obj.asTable();
                    int index = tbl->shape->getIndex(key);
                    if (index >= 0) {
                        if (index >= (int)tbl->properties.size()) tbl->properties.resize(index + 1);
                        tbl->properties[index] = val;
                    } else {
                        tbl->shape = tbl->shape->transition(gc, key);
                        tbl->properties.push_back(val);
                    }
                    gc.writeBarrier(tbl, val);
                } else if (obj.isModule()) {
                    obj.asModule()->exports[key] = val;
                    gc.writeBarrier(obj.asObject(), val);
                }
            }
            DISPATCH();
        }

        LABEL_TFORLOOP: {
            #ifndef COMPUTED_GOTO
            case OpCode::TFORLOOP:
            #endif
            if (!R[a + 2].isNil()) {
                R[a + 1] = R[a + 2]; 
                ip += sbx; 
            }
            DISPATCH();
        }

        LABEL_IMPORT: {
            #ifndef COMPUTED_GOTO
            case OpCode::IMPORT:
            #endif
            {
                std::string fullName = K[bx].asString();
                auto it = moduleCache.find(fullName);
                PomeValue mod;
                if (it != moduleCache.end()) {
                    mod = it->second;
                } else if (moduleLoader) {
                    SAVE_FRAME();
                    mod = moduleLoader(fullName);
                    REFRESH_FRAME();
                    if (mod.isNil()) {
                        runtimeError("Cannot find module '" + fullName + "'");
                        return PomeValue();
                    }
                    if (mod.isModule()) moduleCache[fullName] = mod;
                } else {
                    mod = PomeValue();
                }

                size_t firstDot = fullName.find('.');
                if (firstDot != std::string::npos && firstDot > 0) {
                    std::string topName = fullName.substr(0, firstDot);
                    PomeTable* rootTable;
                    auto itRoot = moduleCache.find(topName);
                    if (itRoot != moduleCache.end() && itRoot->second.isTable()) {
                        rootTable = itRoot->second.asTable();
                    } else {
                        rootTable = gc.allocate<PomeTable>(rootShape);
                        moduleCache[topName] = PomeValue(rootTable);
                    }
                    
                    PomeTable* currentTable = rootTable;
                    std::string remaining = fullName.substr(firstDot + 1);
                    size_t dotPos;
                    while ((dotPos = remaining.find('.')) != std::string::npos) {
                        std::string part = remaining.substr(0, dotPos);
                        PomeValue partVal(gc.allocate<PomeString>(part));
                        if (currentTable->backfill.find(partVal) == currentTable->backfill.end() || !currentTable->backfill[partVal].isTable()) {
                            PomeTable* nextTable = gc.allocate<PomeTable>(rootShape);
                            currentTable->backfill[partVal] = PomeValue(nextTable);
                            currentTable = nextTable;
                        } else {
                            currentTable = currentTable->backfill[partVal].asTable();
                        }
                        remaining = remaining.substr(dotPos + 1);
                    }
                    currentTable->set(PomeValue(gc.allocate<PomeString>(remaining)), mod);
                }

                R[a] = mod; 
            }
            DISPATCH();
        }

        LABEL_EXPORT: {
            #ifndef COMPUTED_GOTO
            case OpCode::EXPORT:
            #endif
            if (currentModule) {
                PomeValue val = R[a];
                currentModule->exports[K[bx]] = val;
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
                PomeValue superVal = R[b];
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
                PomeValue methodName = K[c]; 
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
                PomeValue obj = R[b];
                if (obj.isInstance()) {
                    PomeInstance* instance = obj.asInstance();
                    PomeFunction* iterMethod = instance->klass->findMethod("iterator");
                    if (iterMethod) {
                        SAVE_FRAME();
                        if (frameCount >= 8192) runtimeError("Stack overflow");
                        CallFrame* nextFrame = &frames[frameCount++];
                        nextFrame->function = iterMethod;
                        nextFrame->module = iterMethod->module;
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
                PomeValue v1 = R[b];
                R[a] = !v1.asBool() ? v1 : R[c];
            }
            DISPATCH();
        }

        LABEL_OR: {
            #ifndef COMPUTED_GOTO
            case OpCode::OR:
            #endif
            {
                PomeValue v1 = R[b];
                R[a] = v1.asBool() ? v1 : R[c];
            }
            DISPATCH();
        }

        LABEL_SLICE: {
            #ifndef COMPUTED_GOTO
            case OpCode::SLICE:
            #endif
            {
                PomeValue obj = R[b];
                PomeValue startVal = R[c];
                PomeValue endVal = R[c + 1];
                if (obj.isList()) {
                    PomeList* list = obj.asList();
                    int size = list->isUnboxed ? (int)list->unboxedElements.size() : (int)list->elements.size();
                    int s = startVal.isNil() ? 0 : (int)startVal.asNumber();
                    int e = endVal.isNil() ? size : (int)endVal.asNumber();
                    if (s < 0) s = 0;
                    if (e > size) e = size;
                    
                    PomeList* res = gc.allocateList();
                    if (e > s) {
                        size_t diff = 0;
                        if (list->isUnboxed) {
                            res->ensureCapacity(e - s);
                            res->isUnboxed = true;
                            res->unboxedElements.assign(list->unboxedElements.begin() + s, list->unboxedElements.begin() + e);
                            diff = res->unboxedElements.capacity() * sizeof(double);
                        } else {
                            res->ensureCapacity(e - s);
                            res->isUnboxed = false;
                            res->elements.assign(list->elements.begin() + s, list->elements.begin() + e);
                            for (auto& val : res->elements) {
                                gc.writeBarrier(res, val);
                            }
                            diff = res->elements.capacity() * sizeof(PomeValue);
                        }
                        gc.updateSize(res, sizeof(PomeList), sizeof(PomeList) + diff);
                    }
                    R[a] = PomeValue(res);
                } else if (obj.isString()) {
                    const std::string& str = obj.asString();
                    int s = startVal.isNil() ? 0 : (int)startVal.asNumber();
                    int e = endVal.isNil() ? (int)str.length() : (int)endVal.asNumber();
                    if (s < 0) s = 0;
                    if (e > (int)str.length()) e = (int)str.length();
                    PomeString* res = nullptr;
                    if (s >= e) {
                        res = gc.allocate<PomeString>("");
                    } else {
                        res = gc.allocate<PomeString>(str.substr(s, e - s));
                    }
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
                for (int i = 0; i < b; ++i) {
                    std::cout << R[a + i].toString() << (i == b - 1 ? "" : " ");
                }
                std::cout << std::endl;
            }
            DISPATCH();
        }

        LABEL_TRY: {
            #ifndef COMPUTED_GOTO
            case OpCode::TRY:
            #endif
            {
                ExceptionHandler handler;
                handler.frameIdx = frameCount;
                handler.stackTop = stackTop;
                handler.catchIp = ip + sbx;
                handlers.push_back(handler);
            }
            DISPATCH();
        }

        LABEL_THROW: {
            #ifndef COMPUTED_GOTO
            case OpCode::THROW:
            #endif
            throw VMException{R[a]};
        }

        LABEL_CATCH: {
            #ifndef COMPUTED_GOTO
            case OpCode::CATCH:
            #endif
            R[a] = pendingException;
            DISPATCH();
        }

        LABEL_ASYNC: {
            #ifndef COMPUTED_GOTO
            case OpCode::ASYNC:
            #endif
            DISPATCH();
        }

        LABEL_AWAIT: {
            #ifndef COMPUTED_GOTO
            case OpCode::AWAIT:
            #endif
            {
                PomeValue v = R[b];
                if (v.isTask()) {
                    PomeTask* task = (PomeTask*)v.asObject();
                    if (task->isCompleted) {
                        R[a] = task->result;
                    } else {
                        ip--; // Re-execute AWAIT
                        SAVE_FRAME();
                        return PomeValue(); // Yield
                    }
                }
            }
            DISPATCH();
        }

        #ifndef COMPUTED_GOTO
            default:
                runtimeError("Unknown opcode.");
                frameCount = initialFrameIdx;
                return PomeValue();
            }
        }
        #endif

    } catch (VMException& e) {
        if (handlers.empty()) {
            hasError = true;
            pendingException = e.value;
            frameCount = initialFrameIdx;
            throw;
        }
        ExceptionHandler h = handlers.back();
        handlers.pop_back();
        frameCount = h.frameIdx;
        stackTop = h.stackTop;
        REFRESH_FRAME();
        ip = h.catchIp;
        pendingException = e.value;
        goto LABEL_EXCEPTION_LOOP;
    }
    
    frameCount = initialFrameIdx;
    return PomeValue();
}

PomeValue VM::loadNativeModule(const std::string& libraryPath, PomeModule* moduleObj) {
    void* handle = dlopen(libraryPath.c_str(), RTLD_NOW);
    if (!handle) {
        runtimeError("Could not load native module: " + std::string(dlerror()));
        return PomeValue();
    }

    using PomeInitFn = void(*)(GarbageCollector&, PomeModule*);
    PomeInitFn init = (PomeInitFn)dlsym(handle, "PomeInitModule");
    if (!init) {
        runtimeError("Native module missing PomeInitModule entry point: " + libraryPath);
        dlclose(handle);
        return PomeValue();
    }

    init(gc, moduleObj);
    return PomeValue(moduleObj);
}

void VM::runEventLoop() {
    while (!taskQueue.empty()) {
        PomeTask* task = taskQueue.front();
        taskQueue.pop_front();
        
        if (task->isCompleted) continue;
        
        interpret(task->function->chunk.get(), task->function->module);
    }
}

}
