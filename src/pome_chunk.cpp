#include "../include/pome_chunk.h"
#include <iostream>
#include <iomanip>

namespace Pome {

    // Helper to print instruction for debugging
    void disassembleChunk(Chunk& chunk, const char* name);
    int disassembleInstruction(Chunk& chunk, int offset);

    void disassembleChunk(Chunk& chunk, const char* name) {
        std::cout << "== " << name << " ==" << std::endl;
        
        for (int offset = 0; offset < (int)chunk.code.size(); ) {
            offset = disassembleInstruction(chunk, offset);
        }
    }

    static int simpleInstruction(const char* name, int offset) {
        std::cout << name << std::endl;
        return offset + 1;
    }

    static int byteInstruction(const char* name, Chunk& chunk, int offset) {
        // uint8_t slot = chunk.code[offset + 1];
        // std::cout << std::left << std::setw(16) << name << std::setw(4) << (int)slot << std::endl;
        return offset + 2; 
    }
    
    // Register VM Disassembler
    int disassembleInstruction(Chunk& chunk, int offset) {
        std::cout << std::setw(4) << std::setfill('0') << offset << " " << std::setfill(' ');
        
        if (offset > 0 && chunk.lines[offset] == chunk.lines[offset - 1]) {
            std::cout << "   | ";
        } else {
            std::cout << std::setw(4) << chunk.lines[offset] << " ";
        }
        
        Instruction instruction = chunk.code[offset];
        OpCode op = Chunk::getOpCode(instruction);
        int a = Chunk::getA(instruction);
        int b = Chunk::getB(instruction);
        int c = Chunk::getC(instruction);
        int bx = Chunk::getBx(instruction);
        int sbx = Chunk::getSBx(instruction);

        switch (op) {
            case OpCode::MOVE:
                std::cout << "MOVE      R" << a << " R" << b << std::endl;
                break;
            case OpCode::LOADK:
                std::cout << "LOADK     R" << a << " K" << bx << " (" << chunk.constants[bx].toString() << ")" << std::endl;
                break;
            case OpCode::LOADBOOL:
                std::cout << "LOADBOOL  R" << a << " " << b << " " << c << std::endl;
                break;
            case OpCode::LOADNIL:
                std::cout << "LOADNIL   R" << a << " " << b << std::endl;
                break;
            case OpCode::ADD:
                std::cout << "ADD       R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::SUB:
                std::cout << "SUB       R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::MUL:
                std::cout << "MUL       R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::DIV:
                std::cout << "DIV       R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::MOD:
                std::cout << "MOD       R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::POW:
                std::cout << "POW       R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::UNM:
                std::cout << "UNM       R" << a << " R" << b << std::endl;
                break;
            case OpCode::LEN:
                std::cout << "LEN       R" << a << " R" << b << std::endl;
                break;
            case OpCode::CONCAT:
                std::cout << "CONCAT    R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::TESTSET:
                std::cout << "TESTSET   R" << a << " R" << b << " " << c << std::endl;
                break;
            case OpCode::TAILCALL:
                std::cout << "TAILCALL  R" << a << " " << b << " " << c << std::endl;
                break;
            case OpCode::NEWLIST:
                std::cout << "NEWLIST   R" << a << " " << b << " " << c << std::endl;
                break;
            case OpCode::SELF:
                std::cout << "SELF      R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::FORLOOP:
                std::cout << "FORLOOP   R" << a << " " << sbx << " (Target: " << (offset + 1 + sbx) << ")" << std::endl;
                break;
            case OpCode::FORPREP:
                std::cout << "FORPREP   R" << a << " " << sbx << " (Target: " << (offset + 1 + sbx) << ")" << std::endl;
                break;
            case OpCode::TFORCALL:
                std::cout << "TFORCALL  R" << a << " " << c << std::endl;
                break;
            case OpCode::TFORLOOP:
                std::cout << "TFORLOOP  R" << a << " " << sbx << " (Target: " << (offset + 1 + sbx) << ")" << std::endl;
                break;
            case OpCode::IMPORT:
                std::cout << "IMPORT    R" << a << " K" << bx << " (" << chunk.constants[bx].toString() << ")" << std::endl;
                break;
            case OpCode::EXPORT:
                std::cout << "EXPORT    R" << a << " K" << bx << " (" << chunk.constants[bx].toString() << ")" << std::endl;
                break;
            case OpCode::AND:
                std::cout << "AND       R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::OR:
                std::cout << "OR        R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::SLICE:
                std::cout << "SLICE     R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::GETGLOBAL:
                std::cout << "GETGLOBAL R" << a << " K" << bx << " (" << chunk.constants[bx].toString() << ")" << std::endl;
                break;
            case OpCode::SETGLOBAL:
                std::cout << "SETGLOBAL R" << a << " K" << bx << " (" << chunk.constants[bx].toString() << ")" << std::endl;
                break;
            case OpCode::GETTABLE:
                std::cout << "GETTABLE  R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::SETTABLE:
                std::cout << "SETTABLE  R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::NEWTABLE:
                std::cout << "NEWTABLE  R" << a << " " << b << " " << c << std::endl;
                break;
            case OpCode::CALL:
                std::cout << "CALL      R" << a << " " << b << " " << c << std::endl;
                break;
            case OpCode::PRINT:
                std::cout << "PRINT     R" << a << std::endl;
                break;
            case OpCode::RETURN:
                std::cout << "RETURN    R" << a << " " << b << std::endl;
                break;
            case OpCode::JMP:
                std::cout << "JMP       " << sbx << " (Target: " << (offset + 1 + sbx) << ")" << std::endl;
                break;
            case OpCode::TEST:
                std::cout << "TEST      R" << a << " " << c << std::endl;
                break;
            case OpCode::LT:
                std::cout << "LT        R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::LE:
                std::cout << "LE        R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::EQ:
                std::cout << "EQ        R" << a << " R" << b << " R" << c << std::endl;
                break;
            case OpCode::NOT:
                std::cout << "NOT       R" << a << " R" << b << std::endl;
                break;
            default:
                std::cout << "Unknown opcode " << (int)op << std::endl;
                break;
        }
        
        return offset + 1;
    }

}
