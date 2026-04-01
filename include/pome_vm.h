#ifndef POME_VM_H
#define POME_VM_H

#include "pome_chunk.h"
#include "pome_value.h"
#include "pome_gc.h" 
#include <vector>
#include <map> 
#include <unordered_map>
#include <functional>
#include <deque>

namespace Pome {

    struct CallFrame {
        PomeFunction* function;
        PomeModule* module;
        Chunk* chunk;
        uint32_t* ip;
        int base; // Index in stack where R0 for this frame starts
        int destReg; // Register in PREVIOUS frame where result should be stored
        PomeTask* task = nullptr; // Task this frame belongs to (if any)
    };

    struct ExceptionHandler {
        int frameIdx;
        uint32_t* catchIp;
        int stackTop;
    };

    class VMException : public std::exception {
    public:
        PomeValue value;
        explicit VMException(PomeValue val) : value(val) {}
    };

    using ModuleLoader = std::function<PomeValue(const std::string&)>;

    class PomeUpvalue; // Added

    class VM {
    public:
        VM(GarbageCollector& gc, ModuleLoader loader);
        ~VM();

        PomeValue interpret(Chunk* chunk, PomeModule* module = nullptr);
        void registerNative(const std::string& name, NativeFn fn);
        void registerGlobal(const std::string& name, PomeValue value);
        
        void markRoots();
        GarbageCollector& getGC() { return gc; }
        PomeValue loadNativeModule(const std::string& libraryPath, PomeModule* moduleObj);
        void runEventLoop();
        PomeModule* getCurrentModule() const { return currentModule; }

        bool hasError = false;
        PomeValue pendingException;

    private:
        void runtimeError(const std::string& message);
        void throwException(PomeValue value);
        PomeUpvalue* captureUpvalue(PomeValue* local); // Added
        void closeUpvalues(PomeValue* last);           // Added
        
        PomeString* charCache[256] = {nullptr};

        GarbageCollector& gc; 
        ModuleLoader moduleLoader;
        std::unordered_map<PomeValue, PomeValue> globals; 
        std::unordered_map<std::string, PomeValue> moduleCache;
        PomeModule* currentModule = nullptr;
        
        std::vector<PomeValue> stack;
        int stackTop = 0;
        
        // Upvalues
        PomeUpvalue* openUpvalues = nullptr; // Added

        // Call Stack
        std::vector<CallFrame> frames;
        int frameCount;

        // Exception Handlers
        std::vector<ExceptionHandler> handlers;

        // Async Task Queue
        std::deque<PomeTask*> taskQueue;
        
        // Active pointers for current frame (optimizations)
        uint32_t* ip;
        int frameBase; 
    };

}

#endif // POME_VM_H
