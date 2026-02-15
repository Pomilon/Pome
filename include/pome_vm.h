#ifndef POME_VM_H
#define POME_VM_H

#include "pome_chunk.h"
#include "pome_value.h"
#include "pome_gc.h" 
#include <vector>
#include <map> 
#include <functional>

namespace Pome {

    struct CallFrame {
        PomeFunction* function;
        Chunk* chunk;
        uint32_t* ip;
        int base; // Index in stack where R0 for this frame starts
        int destReg; // Register in PREVIOUS frame where result should be stored
    };

    using ModuleLoader = std::function<PomeValue(const std::string&)>;

    class VM {
    public:
        VM(GarbageCollector& gc, ModuleLoader loader);
        ~VM();

        void interpret(Chunk* chunk, PomeModule* module = nullptr);
        void registerNative(const std::string& name, NativeFn fn);
        void registerGlobal(const std::string& name, PomeValue value);
        
        void markRoots();
        GarbageCollector& getGC() { return gc; }
        PomeValue loadNativeModule(const std::string& libraryPath, PomeModule* moduleObj);

        bool hasError = false;

    private:
        void runtimeError(const std::string& message);
        GarbageCollector& gc; 
        ModuleLoader moduleLoader;
        std::map<PomeValue, PomeValue> globals; 
        std::map<std::string, PomeValue> moduleCache;
        PomeModule* currentModule = nullptr;
        
        std::vector<PomeValue> stack;
        int stackTop = 0;
        
        // Call Stack
        std::vector<CallFrame> frames;
        int frameCount;
        
        // Active pointers for current frame (optimizations)
        uint32_t* ip;
        int frameBase; 
    };

}

#endif // POME_VM_H