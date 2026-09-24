// ============================================================================
// Menu.h  -  safe console input helpers + menu printing.
// Every numeric input is validated; letters, blanks and out-of-range values
// produce a clear message instead of breaking std::cin. End-of-input (Ctrl+D,
// or a closed pipe) returns the menu's "back/exit" value so the program
// always terminates cleanly.
// ============================================================================
#pragma once
#include <iostream>
#include <string>
#include "../program/Assembler.h"

namespace Menu {

inline bool& eofReached() { static bool e = false; return e; }

inline std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    if (!std::getline(std::cin, s)) { eofReached() = true; std::cout << "\n"; return ""; }
    if (!s.empty() && s.back() == '\r') s.pop_back();
    return s;
}

// Reads an integer in [lo, hi]; re-prompts on invalid input.
inline int readInt(const std::string& prompt, int lo, int hi, int onEof = 0) {
    while (true) {
        std::string s = Assembler::trim(readLine(prompt));
        if (eofReached()) return onEof;
        int v;
        if (s.empty()) { std::cout << "  Please enter a number between " << lo << " and " << hi << ".\n"; continue; }
        if (!Assembler::parseNumber(s, v)) { std::cout << "  ERROR: '" << s << "' is not a valid number.\n"; continue; }
        if (v < lo || v > hi) { std::cout << "  ERROR: " << v << " is out of range (" << lo << "-" << hi << ").\n"; continue; }
        return v;
    }
}

inline bool confirm(const std::string& prompt) {
    std::string s = Assembler::upper(Assembler::trim(readLine(prompt + " (y/n): ")));
    return s == "Y" || s == "YES";
}

inline void header(const std::string& title) {
    std::cout << "\n================================================\n";
    int pad = static_cast<int>((48 - title.size()) / 2);
    std::cout << std::string(pad > 0 ? pad : 0, ' ') << title << "\n";
    std::cout << "================================================\n";
}

inline void ok(const std::string& m) { std::cout << "  OK: " << m << "\n"; }
inline void error(const std::string& m) { std::cout << "\n  ERROR:\n  " << m << "\n"; }

}  // namespace Menu
