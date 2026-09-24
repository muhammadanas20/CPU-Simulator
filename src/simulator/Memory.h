// ============================================================================
// Memory.h  -  simulated word-addressed data memory, 1024 int cells.
// Backed by our DynamicArray<int>. Addresses outside 0..1023 raise an error
// that the CPU reports as "Invalid memory address".
// ============================================================================
#pragma once
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include "../data_structures/DynamicArray.h"

class Memory {
public:
    static const int SIZE = 1024;

private:
    DynamicArray<int> cells_;

public:
    Memory() : cells_(SIZE) { cells_.assign(SIZE, 0); }

    static bool valid(int addr) { return addr >= 0 && addr < SIZE; }
    int read(int addr) const {
        if (!valid(addr)) throw std::out_of_range("Invalid memory address " + std::to_string(addr) + " (valid 0-" + std::to_string(SIZE - 1) + ")");
        return cells_[addr];
    }
    void write(int addr, int value) {
        if (!valid(addr)) throw std::out_of_range("Invalid memory address " + std::to_string(addr) + " (valid 0-" + std::to_string(SIZE - 1) + ")");
        cells_[addr] = value;
    }
    void reset() { for (int i = 0; i < SIZE; ++i) cells_[i] = 0; }
    int nonZeroCount() const { int c = 0; for (int i = 0; i < SIZE; ++i) if (cells_[i]) ++c; return c; }

    void displayRange(int from, int to, std::ostream& os = std::cout) const {
        if (from < 0) from = 0;
        if (to >= SIZE) to = SIZE - 1;
        for (int a = from; a <= to; ++a) {
            if ((a - from) % 8 == 0) os << "\n  " << std::setw(4) << a << ":";
            os << std::setw(7) << cells_[a];
        }
        os << "\n";
    }
    void displayNonZero(std::ostream& os = std::cout) const {
        int shown = 0;
        for (int a = 0; a < SIZE; ++a)
            if (cells_[a]) { os << "  [" << a << "] = " << cells_[a] << "\n"; ++shown; }
        if (!shown) os << "  (all " << SIZE << " cells are 0)\n";
    }
};
