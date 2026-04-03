#ifndef POME_OPCODE_H
#define POME_OPCODE_H

#include <cstdint>

namespace Pome {

    enum class OpCode : uint8_t {
        // --- Data Movement ---
        MOVE,       // R(A) := R(B)
        LOADK,      // R(A) := K(Bx)  (Load constant)
        LOADBOOL,   // R(A) := (Bool)B; if (C) skip next (optimization)
        LOADNIL,    // R(A), R(A+1), ..., R(A+B) := nil

        // --- Arithmetic ---
        ADD,        // R(A) := RK(B) + RK(C)
        SUB,        // R(A) := RK(B) - RK(C)
        MUL,        // R(A) := RK(B) * RK(C)
        DIV,        // R(A) := RK(B) / RK(C)
        MOD,        // R(A) := RK(B) % RK(C)
        POW,        // R(A) := RK(B) ^ RK(C)
        UNM,        // R(A) := -R(B) (Unary minus)

        // --- Logic ---
        NOT,        // R(A) := not R(B)
        LEN,        // R(A) := length of R(B)
        CONCAT,     // R(A) := R(B) .. ... .. R(C)
        JMP,        // PC += sBx
        EQ,         // if ((RK(B) == RK(C)) ~= A) then PC++
        LT,         // if ((RK(B) <  RK(C)) ~= A) then PC++
        LE,         // if ((RK(B) <= RK(C)) ~= A) then PC++
        TEST,       // if not (R(A) <=> C) then PC++
        TESTSET,    // if (R(B) <=> C) then R(A) := R(B) else PC++

        // --- Function Call ---
        CALL,       // R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
        TAILCALL,
        RETURN,
        GETGLOBAL,
        SETGLOBAL,
        GETUPVAL,
        SETUPVAL,
        CLOSURE,
        NEWLIST,
        NEWTABLE,
        GETTABLE,
        SETTABLE,
        SELF,
        FORLOOP,
        FORPREP,
        TFORCALL,
        TFORLOOP,
        IMPORT,
        EXPORT,
        INHERIT, // Added
        GETSUPER, // Added
        GETITER,
        AND,
        OR,
        SLICE,
        PRINT,
        TRY,        // Added for error handling
        THROW,      // Added for error handling
        CATCH,      // Retrieve pending exception
        ASYNC,      // Create a task from a function
        AWAIT,      // Suspend and wait for a task
        GETFIELD,   // R(A) := R(B).K(C)
        SETFIELD,   // R(A).K(C) := R(B)

        // --- Specialized Opcodes (PEP 659 style) ---
        ADD_NN,     // Specialized ADD for Number + Number
        SUB_NN,
        MUL_NN,
        DIV_NN,
        MOD_NN,
        LT_NN,
        LE_NN,
        GETGLOBAL_CACHE,
        GETTABLE_CACHE,
        SETTABLE_CACHE,
        GETFIELD_CACHE,
        SETFIELD_CACHE,

        // Specialized List Opcodes
        LIST_ADD_SCALAR,
        LIST_SUM,
        GETLIST_N,  // Generic numeric
        SETLIST_N,
        GETLIST_D,  // Specialized Double
        SETLIST_D,
        GETLIST_I,  // Specialized Int32
        SETLIST_I,

        OP_COUNT
    };

}

#endif // POME_OPCODE_H
