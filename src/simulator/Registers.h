// ============================================================================
// Registers.h  -  Simplified simulated CPU registers (NOT real x86 registers).
//   R0..R7 general purpose, PC = index of next instruction,
//   SP = simulated stack pointer (starts at 1000, -1 per PUSH, +1 per POP).
// Flags: ZF zero, SF sign, CF carry/borrow (unsigned), OF signed overflow.
// ============================================================================
#pragma once
#include <sstream>
#include <string>

struct Flags {
    bool ZF = false, SF = false, CF = false, OF = false;
};

class RegisterFile {
public:
    static const int COUNT = 8;
    static const int SP_INITIAL = 1000;
    int R[COUNT] = {0};
    int PC = 0;
    int SP = SP_INITIAL;
    Flags flags;

    void reset() { for (int& r : R) r = 0; PC = 0; SP = SP_INITIAL; flags = Flags{}; }

    std::string compact() const {   // "R0=0 R1=10 ... PC=2 SP=1000 ZF=0 ..."
        std::ostringstream ss;
        for (int i = 0; i < COUNT; ++i) ss << "R" << i << "=" << R[i] << " ";
        ss << "PC=" << PC << " SP=" << SP << " ZF=" << flags.ZF << " SF=" << flags.SF
           << " CF=" << flags.CF << " OF=" << flags.OF;
        return ss.str();
    }
    std::string table() const {
        std::ostringstream ss;
        for (int i = 0; i < COUNT; ++i) ss << "  R" << i << " = " << R[i] << "\n";
        ss << "  PC = " << PC << "\n  SP = " << SP << "\n";
        return ss.str();
    }
    std::string flagTable() const {
        std::ostringstream ss;
        ss << "  ZF = " << flags.ZF << "\n  SF = " << flags.SF << "\n  CF = " << flags.CF << "\n  OF = " << flags.OF << "\n";
        return ss.str();
    }
};
