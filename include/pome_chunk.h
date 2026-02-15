#ifndef POME_CHUNK_H
#define POME_CHUNK_H

#include <vector>
#include <cstdint>
#include "pome_opcode.h"
#include "pome_value.h"

namespace Pome {

    using Instruction = uint32_t;

    // A chunk of bytecode
    class Chunk {
    public:
        std::vector<Instruction> code;
        std::vector<PomeValue> constants;
        std::vector<int> lines; // Debug info: Line number for each instruction

        void write(Instruction instruction, int line) {
            code.push_back(instruction);
            lines.push_back(line);
        }
        
        // Add constant and return its index
        int addConstant(PomeValue value) {
            // Check if constant already exists to save space (simple linear search for now)
            for (size_t i = 0; i < constants.size(); ++i) {
                if (constants[i] == value) return static_cast<int>(i);
            }
            constants.push_back(value);
            return static_cast<int>(constants.size() - 1);
        }

        // --- Instruction Encoding Helpers ---
        // Op: 6 bits | A: 8 bits | C: 9 bits | B: 9 bits
        // B and C are swapped in standard Lua layout vs ABC order, usually it's Op A C B in bits to align B/C? 
        // Actually Lua 5.1 is:  Op(6) A(8) C(9) B(9)  (LSB ... MSB)
        
        static constexpr int SIZE_OP = 6;
        static constexpr int SIZE_A = 8;
        static constexpr int SIZE_C = 9;
        static constexpr int SIZE_B = 9;
        static constexpr int SIZE_Bx = SIZE_C + SIZE_B;

        static constexpr int POS_OP = 0;
        static constexpr int POS_A = POS_OP + SIZE_OP;
        static constexpr int POS_C = POS_A + SIZE_A;
        static constexpr int POS_B = POS_C + SIZE_C;
        static constexpr int POS_Bx = POS_C;

        static Instruction makeABC(OpCode op, int a, int b, int c) {
            return (static_cast<Instruction>(op) << POS_OP) |
                   (static_cast<Instruction>(a) << POS_A) |
                   (static_cast<Instruction>(c) << POS_C) |
                   (static_cast<Instruction>(b) << POS_B);
        }

        static Instruction makeABx(OpCode op, int a, int bx) {
            return (static_cast<Instruction>(op) << POS_OP) |
                   (static_cast<Instruction>(a) << POS_A) |
                   (static_cast<Instruction>(bx) << POS_Bx);
        }
        
        static Instruction makeAsBx(OpCode op, int a, int sbx) {
            return makeABx(op, a, sbx + MAXARG_sBx); // Store as unsigned with bias
        }

        // --- Decoding Helpers ---

        static OpCode getOpCode(Instruction i) {
            return static_cast<OpCode>((i >> POS_OP) & ((1 << SIZE_OP) - 1));
        }

        static int getA(Instruction i) {
            return (i >> POS_A) & ((1 << SIZE_A) - 1);
        }

        static int getB(Instruction i) {
            return (i >> POS_B) & ((1 << SIZE_B) - 1);
        }

        static int getC(Instruction i) {
            return (i >> POS_C) & ((1 << SIZE_C) - 1);
        }

        static int getBx(Instruction i) {
            return (i >> POS_Bx) & ((1 << SIZE_Bx) - 1);
        }
        
        static constexpr int MAXARG_sBx = (1 << SIZE_Bx) >> 1; // Bias

        static int getSBx(Instruction i) {
            return getBx(i) - MAXARG_sBx;
        }
    };

    // Debugging
    void disassembleChunk(Chunk& chunk, const char* name);
    int disassembleInstruction(Chunk& chunk, int offset);

}

#endif // POME_CHUNK_H
