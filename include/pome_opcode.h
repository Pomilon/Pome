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
        GETITER,
        AND,
        OR,
        SLICE,
        PRINT
    };

}

#endif // POME_OPCODE_H
