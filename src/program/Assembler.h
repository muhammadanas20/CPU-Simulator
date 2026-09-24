// ============================================================================
// Assembler.h  -  Phase 11
// Converts source lines (DynamicArray<string>) into
//     - DynamicArray<Instruction>   (the Instruction Array)
//     - SymbolTable                 (label -> address, a hash table)
//
// This is NOT a lexer/parser (that belongs to the future TOA course). It only
// uses simple string splitting on spaces and commas.
//
// Two-pass algorithm:
//   Pass 1: walk all lines, count instructions, record every "LABEL:" with the
//           address of the NEXT instruction. Duplicate labels are errors.
//   Pass 2: decode each instruction, check operand kinds, and resolve jump
//           labels through the symbol table (undefined label -> error).
// Two passes are needed because a jump may refer to a label defined later
// (forward reference), e.g.  "JZ END" ... "END: HALT".
//
// Source syntax
//   ; comment            LABEL:            LABEL: MOV R1, 5   (label + instr)
// ============================================================================
#pragma once
#include <cctype>
#include <string>
#include "../data_structures/DynamicArray.h"
#include "../simulator/SymbolTable.h"
#include "Instruction.h"

struct AssemblyResult {
    DynamicArray<Instruction> instructions;
    SymbolTable symbols;
    DynamicArray<std::string> errors;
    bool ok() const { return errors.empty(); }
};

class Assembler {
public:
    static std::string trim(const std::string& s) {
        std::size_t b = 0, e = s.size();
        while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
        while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
        return s.substr(b, e - b);
    }
    static std::string upper(std::string s) {
        for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return s;
    }
    static bool isIdentifier(const std::string& s) {
        if (s.empty() || !(std::isalpha(static_cast<unsigned char>(s[0])) || s[0] == '_')) return false;
        for (char c : s) if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
        return true;
    }
    // "R0".."R7" -> 0..7, anything else -> -1
    static int registerIndex(const std::string& s) {
        std::string u = upper(s);
        if (u.size() == 2 && u[0] == 'R' && u[1] >= '0' && u[1] <= '7') return u[1] - '0';
        return -1;
    }
    static bool looksLikeRegister(const std::string& s) {
        std::string u = upper(s);
        if (u.size() < 2 || u[0] != 'R') return false;
        for (std::size_t i = 1; i < u.size(); ++i) if (!std::isdigit(static_cast<unsigned char>(u[i]))) return false;
        return true;
    }
    // Decimal (optionally negative) or 0x hex. Rejects "12a", overflow, etc.
    static bool parseNumber(const std::string& s, int& out) {
        if (s.empty()) return false;
        std::size_t i = 0;
        bool neg = false;
        if (s[0] == '-' || s[0] == '+') { neg = s[0] == '-'; i = 1; }
        int base = 10;
        if (s.size() > i + 2 && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) { base = 16; i += 2; }
        if (i >= s.size()) return false;
        long long v = 0;
        for (; i < s.size(); ++i) {
            int d;
            char c = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
            if (c >= '0' && c <= '9') d = c - '0';
            else if (base == 16 && c >= 'a' && c <= 'f') d = c - 'a' + 10;
            else return false;
            v = v * base + d;
            if (v > 2147483648LL) return false;
        }
        if (neg) v = -v;
        if (v > 2147483647LL || v < -2147483648LL) return false;
        out = static_cast<int>(v);
        return true;
    }

    // Strip comment, pull off an optional "LABEL:" prefix. Returns the rest.
    static std::string splitLabel(const std::string& rawLine, std::string& label, bool* hasLabel = nullptr) {
        std::string line = rawLine;
        std::size_t sc = line.find(';');
        if (sc != std::string::npos) line = line.substr(0, sc);
        line = trim(line);
        label.clear();
        std::size_t colon = line.find(':');
        if (hasLabel) *hasLabel = colon != std::string::npos;
        if (colon != std::string::npos) {
            label = trim(line.substr(0, colon));
            line = trim(line.substr(colon + 1));
        }
        return line;
    }

    // "ADD R1, R2" -> tokens {"ADD","R1","R2"}. Commas and spaces separate.
    static DynamicArray<std::string> tokenize(const std::string& line) {
        DynamicArray<std::string> toks;
        std::string cur;
        for (char c : line) {
            if (c == ',' || std::isspace(static_cast<unsigned char>(c))) {
                if (!cur.empty()) { toks.pushBack(cur); cur.clear(); }
            } else cur += c;
        }
        if (!cur.empty()) toks.pushBack(cur);
        return toks;
    }

    static AssemblyResult assemble(const DynamicArray<std::string>& lines) {
        AssemblyResult r;
        // ---------------- Pass 1: labels ----------------
        int address = 0;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            std::string label;
            bool hasLabel = false;
            std::string rest = splitLabel(lines[i], label, &hasLabel);
            int lineNo = static_cast<int>(i) + 1;
            if (hasLabel) {
                std::string L = upper(label);
                if (!isIdentifier(L))
                    r.errors.pushBack("Line " + std::to_string(lineNo) + ": invalid label name '" + label + "'.");
                else if (registerIndex(L) >= 0 || looksLikeRegister(L) || opcodeFromString(L) != Opcode::INVALID)
                    r.errors.pushBack("Line " + std::to_string(lineNo) + ": label '" + label +
                                      "' is a reserved word (register or opcode).");
                else if (!r.symbols.define(L, address))
                    r.errors.pushBack("Line " + std::to_string(lineNo) + ": duplicate label '" + L + "'.");
            }
            if (!rest.empty()) ++address;
        }
        const int count = address;

        // ---------------- Pass 2: decode ----------------
        address = 0;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            std::string label;
            std::string rest = splitLabel(lines[i], label);
            if (rest.empty()) continue;
            int lineNo = static_cast<int>(i) + 1;
            Instruction ins;
            ins.address = address;
            ins.sourceLine = lineNo;
            std::string err;
            if (decode(rest, ins, r.symbols, count, err)) r.instructions.pushBack(ins);
            else r.errors.pushBack("Line " + std::to_string(lineNo) + " (instruction " +
                                   std::to_string(address) + "): " + err);
            ++address;
        }
        if (count == 0) r.errors.pushBack("Empty program: no instructions found.");
        return r;
    }

private:
    enum Allowed { REG = 1, IMM = 2, MEMREG = 4, LAB = 8 };

    static bool parseOperand(const std::string& tok, int allowed, const SymbolTable& syms, int count,
                             Operand& out, std::string& err) {
        out.text = tok;
        int r = registerIndex(tok);
        if (r >= 0) {
            if (!(allowed & REG)) { err = "register '" + tok + "' not allowed here."; return false; }
            out.type = OperandType::REGISTER; out.value = r; return true;
        }
        if (looksLikeRegister(tok)) { err = "invalid register '" + tok + "' (valid: R0-R7)."; return false; }
        if (tok.size() >= 3 && tok.front() == '[' && tok.back() == ']') {
            std::string inner = trim(tok.substr(1, tok.size() - 2));
            int ri = registerIndex(inner);
            if (!(allowed & MEMREG)) { err = "memory operand '" + tok + "' not allowed here."; return false; }
            if (ri < 0) { err = "invalid register '" + inner + "' inside brackets."; return false; }
            out.type = OperandType::MEM_REGISTER; out.value = ri; return true;
        }
        int n;
        if (parseNumber(tok, n)) {
            if (!(allowed & IMM)) { err = "number '" + tok + "' not allowed here."; return false; }
            out.type = OperandType::IMMEDIATE; out.value = n; return true;
        }
        if (allowed & LAB) {
            std::string L = upper(tok);
            if (!isIdentifier(L)) { err = "invalid label '" + tok + "'."; return false; }
            int addr;
            if (!syms.lookup(L, addr)) { err = "undefined label '" + L + "'."; return false; }
            if (addr < 0 || addr >= count) { err = "invalid jump: label '" + L + "' points past the last instruction."; return false; }
            out.type = OperandType::LABEL; out.value = addr; out.text = L; return true;
        }
        if (std::isdigit(static_cast<unsigned char>(tok[0])) || tok[0] == '-' || tok[0] == '+')
            err = "invalid number '" + tok + "'.";
        else err = "invalid operand '" + tok + "'.";
        return false;
    }

    static bool decode(const std::string& text, Instruction& ins, const SymbolTable& syms, int count, std::string& err) {
        DynamicArray<std::string> t = tokenize(text);
        std::string m = upper(t[0]);
        ins.op = opcodeFromString(m);
        if (ins.op == Opcode::INVALID) { err = "invalid instruction '" + t[0] + "'."; return false; }
        std::size_t nOps = t.size() - 1;
        auto need = [&](std::size_t n) {
            if (nOps != n) {
                err = std::string(opcodeName(ins.op)) + " expects " + std::to_string(n) +
                      " operand(s) but got " + std::to_string(nOps) + ".";
                return false;
            }
            return true;
        };
        switch (ins.op) {
            case Opcode::MOV: case Opcode::ADD: case Opcode::SUB: case Opcode::CMP:
                return need(2) && parseOperand(t[1], REG, syms, count, ins.a, err) &&
                       parseOperand(t[2], REG | IMM, syms, count, ins.b, err);
            case Opcode::LOAD: case Opcode::STORE:
                return need(2) && parseOperand(t[1], REG, syms, count, ins.a, err) &&
                       parseOperand(t[2], IMM | MEMREG, syms, count, ins.b, err);
            case Opcode::INC: case Opcode::DEC: case Opcode::POP:
                return need(1) && parseOperand(t[1], REG, syms, count, ins.a, err);
            case Opcode::PUSH:
                return need(1) && parseOperand(t[1], REG | IMM, syms, count, ins.a, err);
            case Opcode::JMP: case Opcode::JZ: case Opcode::JNZ:
                return need(1) && parseOperand(t[1], LAB, syms, count, ins.a, err);
            case Opcode::NOP: case Opcode::HALT:
                return need(0);
            default:
                err = "invalid instruction."; return false;
        }
    }
};
