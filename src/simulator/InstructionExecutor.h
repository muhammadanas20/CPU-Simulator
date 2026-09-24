// ============================================================================
// InstructionExecutor.h  -  Phase 12: the EXECUTE part of fetch-execute.
// Pure "ALU + control" logic: given one decoded instruction and the machine
// state, change the state and report what happened.
//
// Flag rules (computed with 64-bit math to detect overflow):
//   ADD a,b : r=a+b  ZF=(r==0) SF=(r<0) CF=unsigned carry  OF=signed overflow
//   SUB/CMP : r=a-b  ZF, SF, CF=unsigned borrow (ua<ub), OF=signed overflow
//             (CMP only sets flags, it does not store r)
//   INC/DEC : like ADD/SUB with 1, CF unchanged (as on x86)
//   MOV/LOAD/STORE/PUSH/POP/JMP/NOP : flags unchanged
// ============================================================================
#pragma once
#include <climits>
#include <cstdint>
#include <string>
#include "../data_structures/Stack.h"
#include "../program/Instruction.h"
#include "Memory.h"
#include "Registers.h"

enum class ExecStatus { OK, JUMPED, HALTED, ERROR };

struct ExecOutcome {
    ExecStatus status = ExecStatus::OK;
    std::string message;   // human readable effect, e.g. "R1 = 10 + 20 = 30"
};

class InstructionExecutor {
    static int value(const Operand& o, const RegisterFile& rf) {
        return o.type == OperandType::REGISTER ? rf.R[o.value] : o.value;
    }
    static std::string rn(int r) { return "R" + std::to_string(r); }

    static int addWithFlags(int a, int b, Flags& f, bool setCarry) {
        long long wide = static_cast<long long>(a) + b;
        int r = static_cast<int>(static_cast<uint32_t>(a) + static_cast<uint32_t>(b));
        f.ZF = r == 0; f.SF = r < 0; f.OF = wide != r;
        if (setCarry) f.CF = (static_cast<uint64_t>(static_cast<uint32_t>(a)) + static_cast<uint32_t>(b)) > 0xFFFFFFFFULL;
        return r;
    }
    static int subWithFlags(int a, int b, Flags& f, bool setCarry) {
        long long wide = static_cast<long long>(a) - b;
        int r = static_cast<int>(static_cast<uint32_t>(a) - static_cast<uint32_t>(b));
        f.ZF = r == 0; f.SF = r < 0; f.OF = wide != r;
        if (setCarry) f.CF = static_cast<uint32_t>(a) < static_cast<uint32_t>(b);
        return r;
    }

public:
    static ExecOutcome execute(const Instruction& ins, RegisterFile& rf, Memory& mem, Stack<int>& stack) {
        ExecOutcome out;
        const int a = ins.a.value;   // register index for most instructions
        try {
            switch (ins.op) {
                case Opcode::MOV:
                    rf.R[a] = value(ins.b, rf);
                    out.message = rn(a) + " = " + std::to_string(rf.R[a]);
                    break;
                case Opcode::LOAD: {
                    int addr = ins.b.type == OperandType::MEM_REGISTER ? rf.R[ins.b.value] : ins.b.value;
                    rf.R[a] = mem.read(addr);
                    out.message = rn(a) + " = MEM[" + std::to_string(addr) + "] = " + std::to_string(rf.R[a]);
                    break;
                }
                case Opcode::STORE: {
                    int addr = ins.b.type == OperandType::MEM_REGISTER ? rf.R[ins.b.value] : ins.b.value;
                    mem.write(addr, rf.R[a]);
                    out.message = "MEM[" + std::to_string(addr) + "] = " + std::to_string(rf.R[a]);
                    break;
                }
                case Opcode::ADD: {
                    int x = rf.R[a], y = value(ins.b, rf);
                    rf.R[a] = addWithFlags(x, y, rf.flags, true);
                    out.message = rn(a) + " = " + std::to_string(x) + " + " + std::to_string(y) + " = " + std::to_string(rf.R[a]);
                    break;
                }
                case Opcode::SUB: {
                    int x = rf.R[a], y = value(ins.b, rf);
                    rf.R[a] = subWithFlags(x, y, rf.flags, true);
                    out.message = rn(a) + " = " + std::to_string(x) + " - " + std::to_string(y) + " = " + std::to_string(rf.R[a]);
                    break;
                }
                case Opcode::INC:
                    rf.R[a] = addWithFlags(rf.R[a], 1, rf.flags, false);
                    out.message = rn(a) + " = " + std::to_string(rf.R[a]);
                    break;
                case Opcode::DEC:
                    rf.R[a] = subWithFlags(rf.R[a], 1, rf.flags, false);
                    out.message = rn(a) + " = " + std::to_string(rf.R[a]);
                    break;
                case Opcode::PUSH: {
                    int v = value(ins.a, rf);
                    stack.push(v);                 // may throw StackOverflowError
                    rf.SP--;
                    out.message = "pushed " + std::to_string(v) + ", SP = " + std::to_string(rf.SP);
                    break;
                }
                case Opcode::POP:
                    rf.R[a] = stack.pop();         // may throw StackUnderflowError
                    rf.SP++;
                    out.message = rn(a) + " = " + std::to_string(rf.R[a]) + " (popped), SP = " + std::to_string(rf.SP);
                    break;
                case Opcode::CMP: {
                    int x = rf.R[a], y = value(ins.b, rf);
                    subWithFlags(x, y, rf.flags, true);
                    out.message = "compare " + std::to_string(x) + " with " + std::to_string(y) +
                                  " -> ZF=" + std::to_string(rf.flags.ZF) + " SF=" + std::to_string(rf.flags.SF);
                    break;
                }
                case Opcode::JMP:
                    rf.PC = ins.a.value;
                    out.status = ExecStatus::JUMPED;
                    out.message = "jump to " + ins.a.text + " (" + std::to_string(ins.a.value) + ")";
                    return out;
                case Opcode::JZ: case Opcode::JNZ: {
                    bool take = (ins.op == Opcode::JZ) == rf.flags.ZF;
                    if (take) {
                        rf.PC = ins.a.value;
                        out.status = ExecStatus::JUMPED;
                        out.message = "ZF=" + std::to_string(rf.flags.ZF) + ", jump taken to " + ins.a.text + " (" + std::to_string(ins.a.value) + ")";
                        return out;
                    }
                    out.message = "ZF=" + std::to_string(rf.flags.ZF) + ", jump not taken";
                    break;
                }
                case Opcode::NOP:
                    out.message = "no operation";
                    break;
                case Opcode::HALT:
                    out.status = ExecStatus::HALTED;
                    out.message = "program halted";
                    return out;   // PC stays on the HALT instruction
                default:
                    out.status = ExecStatus::ERROR;
                    out.message = "Invalid instruction at address " + std::to_string(ins.address);
                    return out;
            }
        } catch (const std::exception& e) {
            out.status = ExecStatus::ERROR;
            out.message = std::string(e.what()) + " at instruction " + std::to_string(ins.address) +
                          " (line " + std::to_string(ins.sourceLine) + ": " + ins.toString() + ")";
            return out;
        }
        rf.PC++;   // sequential flow
        return out;
    }
};
