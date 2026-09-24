// ============================================================================
// CPU.h  -  Phases 12/13: LOAD -> FETCH -> EXECUTE loop.
//
//  Program (.asm lines)
//     -> Assembler -> DynamicArray<Instruction> + SymbolTable (hash table)
//     -> load(): instructions copied into program_, ALL enqueued in iqueue_
//     -> step(): FETCH  = dequeue front of the instruction queue
//               EXECUTE = InstructionExecutor
//               RECORD  = append HistoryEntry to history_ (linked list)
//     -> jump taken: queue FLUSHED and REFILLED from the target address
//
// Invariant: when the queue is not empty, its front instruction has
// address == PC. step() re-checks this and refills if needed.
// ============================================================================
#pragma once
#include <string>
#include "../data_structures/LinkedList.h"
#include "../data_structures/Queue.h"
#include "../data_structures/Stack.h"
#include "../program/Assembler.h"
#include "InstructionExecutor.h"
#include "Memory.h"
#include "Registers.h"
#include "SymbolTable.h"

struct HistoryEntry {
    int step = 0;
    int pc = 0;
    std::string instruction;
    std::string before;   // register state before (compact form)
    std::string after;    // register state after
    std::string result;   // effect / message
};

enum class CpuState { EMPTY, READY, PAUSED, HALTED, ERROR };

inline const char* cpuStateName(CpuState s) {
    switch (s) {
        case CpuState::EMPTY: return "NO PROGRAM"; case CpuState::READY: return "READY";
        case CpuState::PAUSED: return "PAUSED"; case CpuState::HALTED: return "HALTED";
        default: return "ERROR";
    }
}

class CPU {
public:
    static const std::size_t STACK_CAPACITY = 64;
    static const int DEFAULT_STEP_LIMIT = 10000;

private:
    DynamicArray<Instruction> program_;
    SymbolTable symbols_;
    std::string programName_;
    RegisterFile regs_;
    Memory memory_;
    Stack<int> stack_{STACK_CAPACITY};
    Queue<Instruction> iqueue_;
    LinkedList<HistoryEntry> history_;
    CpuState state_ = CpuState::EMPTY;
    std::string lastError_;
    std::string statusMessage_;
    int steps_ = 0;
    int breakpoint_ = -1;         // "Pause" at this address during Run (-1 = none)
    std::size_t queueFlushes_ = 0;

    void refillQueue(int fromAddress) {
        if (!iqueue_.isEmpty()) ++queueFlushes_;
        iqueue_.clear();
        for (int i = fromAddress; i >= 0 && i < static_cast<int>(program_.size()); ++i) iqueue_.enqueue(program_[i]);
    }

public:
    // ---------------- LOAD ----------------
    void load(const std::string& name, const DynamicArray<Instruction>& instructions, const SymbolTable& symbols) {
        program_ = instructions;
        symbols_ = symbols;
        programName_ = name;
        reset();
    }
    // RESET: registers, memory, stack, history, queue back to the start.
    void reset() {
        regs_.reset();
        memory_.reset();
        stack_.clear();
        history_.clear();
        steps_ = 0;
        queueFlushes_ = 0;
        lastError_.clear();
        statusMessage_.clear();
        iqueue_.clear();
        refillQueue(0);
        state_ = program_.empty() ? CpuState::EMPTY : CpuState::READY;
    }

    // ---------------- FETCH + EXECUTE one instruction ----------------
    // Returns false when nothing was executed (halted, error, no program).
    bool step() {
        if (state_ == CpuState::EMPTY) { lastError_ = "Empty program: load a program first."; return false; }
        if (state_ == CpuState::HALTED || state_ == CpuState::ERROR) return false;
        if (regs_.PC == static_cast<int>(program_.size())) {   // fell off the end
            state_ = CpuState::HALTED;
            statusMessage_ = "Reached end of program without HALT.";
            return false;
        }
        if (regs_.PC < 0 || regs_.PC > static_cast<int>(program_.size())) {
            state_ = CpuState::ERROR;
            lastError_ = "Invalid jump: PC = " + std::to_string(regs_.PC) + " is outside the program.";
            return false;
        }
        if (iqueue_.isEmpty() || iqueue_.peek().address != regs_.PC) refillQueue(regs_.PC);

        Instruction ins = iqueue_.dequeue();                   // FETCH
        HistoryEntry h;
        h.step = ++steps_;
        h.pc = regs_.PC;
        h.instruction = ins.toString();
        h.before = regs_.compact();
        ExecOutcome o = InstructionExecutor::execute(ins, regs_, memory_, stack_);  // EXECUTE
        h.after = regs_.compact();
        h.result = o.message;
        history_.pushBack(h);                                  // RECORD

        switch (o.status) {
            case ExecStatus::JUMPED: refillQueue(regs_.PC); state_ = CpuState::PAUSED; break;
            case ExecStatus::HALTED: state_ = CpuState::HALTED; statusMessage_ = "Program completed successfully (HALT)."; break;
            case ExecStatus::ERROR:
                state_ = CpuState::ERROR;
                lastError_ = o.message;
                h.result = "ERROR: " + o.message;
                history_.update(history_.size() - 1, h);
                break;
            default: state_ = CpuState::PAUSED;
        }
        return true;
    }

    // ---------------- RUN until HALT / error / breakpoint / step limit -------
    // Returns a short description of why it stopped.
    std::string run(int stepLimit = DEFAULT_STEP_LIMIT) {
        int executed = 0;
        while (state_ != CpuState::HALTED && state_ != CpuState::ERROR && state_ != CpuState::EMPTY) {
            if (executed > 0 && regs_.PC == breakpoint_) {
                state_ = CpuState::PAUSED;
                return "Paused at breakpoint (address " + std::to_string(breakpoint_) + ").";
            }
            if (executed >= stepLimit) {
                state_ = CpuState::PAUSED;
                return "Paused after " + std::to_string(stepLimit) + " steps (possible infinite loop). Run again to continue.";
            }
            if (!step()) break;
            ++executed;
        }
        if (state_ == CpuState::EMPTY) return lastError_.empty() ? "No program loaded." : lastError_;
        if (state_ == CpuState::ERROR) return "ERROR: " + lastError_;
        return statusMessage_;
    }

    // ---------------- accessors ----------------
    const RegisterFile& registers() const { return regs_; }
    RegisterFile& registers() { return regs_; }
    const Memory& memory() const { return memory_; }
    Memory& memory() { return memory_; }
    const Stack<int>& stack() const { return stack_; }
    const Queue<Instruction>& queue() const { return iqueue_; }
    const LinkedList<HistoryEntry>& history() const { return history_; }
    void clearHistory() { history_.clear(); }
    const SymbolTable& symbols() const { return symbols_; }
    const DynamicArray<Instruction>& program() const { return program_; }
    const std::string& programName() const { return programName_; }
    CpuState state() const { return state_; }
    const std::string& lastError() const { return lastError_; }
    const std::string& statusMessage() const { return statusMessage_; }
    int steps() const { return steps_; }
    std::size_t queueFlushes() const { return queueFlushes_; }
    int breakpoint() const { return breakpoint_; }
    void setBreakpoint(int addr) { breakpoint_ = addr; }
    // After loading a snapshot the queue must follow the restored PC.
    void syncAfterStateRestore() {
        refillQueue(regs_.PC);
        if (state_ != CpuState::EMPTY) state_ = CpuState::PAUSED;
    }
};
