// ============================================================================
// SymbolTable.h  -  label -> instruction address, backed by our HashTable.
// ============================================================================
#pragma once
#include <iostream>
#include <string>
#include "../data_structures/HashTable.h"

class SymbolTable {
    HashTable<int> table_{11};
public:
    bool define(const std::string& label, int address) { return table_.insert(label, address); }
    bool lookup(const std::string& label, int& address) const { return table_.search(label, address); }
    bool remove(const std::string& label) { return table_.remove(label); }
    bool contains(const std::string& label) const { return table_.contains(label); }
    void clear() { table_.clear(); }
    std::size_t size() const { return table_.size(); }
    std::size_t collisions() const { return table_.collisions(); }
    const HashTable<int>& table() const { return table_; }
    void display(std::ostream& os = std::cout) const {
        os << "SYMBOL TABLE (label -> address), separate chaining\n";
        table_.display(os);
    }
};
