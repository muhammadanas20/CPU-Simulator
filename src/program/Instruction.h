// ============================================================================
// Instruction.h  -  Phase 11
// Internal (decoded) form of one assembly instruction.
//   "ADD R1, R2"  ->  Instruction{ op=ADD, a={REGISTER,1}, b={REGISTER,2} }
// Storing decoded instructions means the CPU never re-reads text while running.
// ============================================================================
#pragma once
#include <ostream>
#include <string>

enum class Opcode { MOV, LOAD, STORE, ADD, SUB, INC, DEC, PUSH, POP, CMP, JMP, JZ, JNZ, NOP, HALT, INVALID };

enum class OperandType {
    NONE,
    REGISTER,     // R0..R7              value = register number
    IMMEDIATE,    // 10, -3, 0x1F        value = the number
    MEM_REGISTER, // [R2] (LOAD/STORE)   value = register holding the address
    LABEL         // LOOP (jumps)        value = resolved instruction address
};

struct Operand {
    OperandType type = OperandType::NONE;
    int value = 0;
    std::string text;   // original spelling, e.g. "LOOP"
};

inline const char* opcodeName(Opcode op) {
    switch (op) {
        case Opcode::MOV: return "MOV";   case Opcode::LOAD: return "LOAD";
        case Opcode::STORE: return "STORE"; case Opcode::ADD: return "ADD";
        case Opcode::SUB: return "SUB";   case Opcode::INC: return "INC";
        case Opcode::DEC: return "DEC";   case Opcode::PUSH: return "PUSH";
        case Opcode::POP: return "POP";   case Opcode::CMP: return "CMP";
        case Opcode::JMP: return "JMP";   case Opcode::JZ: return "JZ";
        case Opcode::JNZ: return "JNZ";   case Opcode::NOP: return "NOP";
        case Opcode::HALT: return "HALT"; default: return "INVALID";
    }
}

inline Opcode opcodeFromString(const std::string& s) {
    // Small fixed table - a linear scan over 15 entries is perfectly adequate.
    static const Opcode all[] = {Opcode::MOV, Opcode::LOAD, Opcode::STORE, Opcode::ADD, Opcode::SUB,
                                 Opcode::INC, Opcode::DEC, Opcode::PUSH, Opcode::POP, Opcode::CMP,
                                 Opcode::JMP, Opcode::JZ, Opcode::JNZ, Opcode::NOP, Opcode::HALT};
    for (Opcode op : all) if (s == opcodeName(op)) return op;
    return Opcode::INVALID;
}

struct Instruction {
    int address = 0;       // index in the instruction array (what PC holds)
    int sourceLine = 0;    // 1-based line in the .asm file (for error messages)
    Opcode op = Opcode::INVALID;
    Operand a, b;

    std::string toString() const {
        std::string s = opcodeName(op);
        auto fmt = [](const Operand& o) -> std::string {
            switch (o.type) {
                case OperandType::REGISTER: return "R" + std::to_string(o.value);
                case OperandType::IMMEDIATE: return std::to_string(o.value);
                case OperandType::MEM_REGISTER: return "[R" + std::to_string(o.value) + "]";
                case OperandType::LABEL: return o.text;
                default: return "";
            }
        };
        if (a.type != OperandType::NONE) s += " " + fmt(a);
        if (b.type != OperandType::NONE) s += ", " + fmt(b);
        return s;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Instruction& i) {
    return os << i.address << ": " << i.toString();
}
