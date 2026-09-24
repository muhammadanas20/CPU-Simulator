// ============================================================================
// Program.h  -  an editable program = name + file path + source lines.
// Source lines live in a DynamicArray<string> so the editor can insert,
// modify and delete lines by index in O(n) / O(1).
// ============================================================================
#pragma once
#include <string>
#include "../data_structures/DynamicArray.h"

struct Program {
    std::string name;                 // e.g. "addition.asm"
    std::string path;                 // e.g. "programs/addition.asm"
    DynamicArray<std::string> lines;  // raw source text, one line per slot
    bool modified = false;
    bool loaded() const { return !name.empty(); }
};
