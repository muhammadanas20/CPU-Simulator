// ============================================================================
// ProgramRepository.h  -  Phases 3/7/8/9
// The repository of known programs, stored as a LinkedList<ProgramRecord>
// and persisted to data/program_index.csv.
//
// CSV format (header + one row per program):
//   id,name,path,instructions,executions,size,status,last_execution
// Corrupted rows (wrong column count / non-numeric fields) are SKIPPED with a
// warning instead of crashing; the index is rewritten cleanly on next save.
// ============================================================================
#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include "../algorithms/Searching.h"
#include "../algorithms/Sorting.h"
#include "../data_structures/LinkedList.h"
#include "../filesystem/FileManager.h"
#include "Assembler.h"

struct ProgramRecord {
    int id = 0;
    std::string name;
    std::string path;
    int instructionCount = 0;
    int executionCount = 0;
    long fileSize = 0;
    std::string status = "OK";         // OK | ERRORS | MISSING
    std::string lastExecution = "never";
};

inline std::ostream& operator<<(std::ostream& os, const ProgramRecord& r) {
    return os << "#" << r.id << " " << r.name;
}

enum class SortKey { NAME = 1, INSTRUCTIONS, EXECUTIONS, FILE_SIZE };
enum class SortMethod { BUBBLE = 1, SELECTION, INSERTION };

class ProgramRepository {
private:
    LinkedList<ProgramRecord> list_;
    std::string indexPath_;
    std::string programsDir_;
    int nextId_ = 1;

    static bool splitCsv(const std::string& line, DynamicArray<std::string>& cols) {
        cols.clear();
        std::string cur;
        for (char c : line) { if (c == ',') { cols.pushBack(cur); cur.clear(); } else cur += c; }
        cols.pushBack(cur);
        return true;
    }
    static std::string sanitize(std::string s) {  // commas would break the CSV
        for (char& c : s) if (c == ',' || c == '\n') c = '_';
        return s;
    }

public:
    DynamicArray<std::string> warnings;   // messages produced while loading

    ProgramRepository(std::string indexPath, std::string programsDir)
        : indexPath_(std::move(indexPath)), programsDir_(std::move(programsDir)) {}

    LinkedList<ProgramRecord>& list() { return list_; }
    const LinkedList<ProgramRecord>& list() const { return list_; }
    std::size_t size() const { return list_.size(); }

    // ---------------- persistence ----------------
    bool load() {
        warnings.clear();
        list_.clear();
        nextId_ = 1;
        if (!FileManager::exists(indexPath_)) {
            warnings.pushBack("Index '" + indexPath_ + "' not found - creating a new one.");
            return save();
        }
        DynamicArray<std::string> lines;
        std::string err;
        if (!FileManager::readLines(indexPath_, lines, err)) { warnings.pushBack(err); return false; }
        DynamicArray<std::string> c;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            if (Assembler::trim(lines[i]).empty()) continue;
            if (i == 0 && lines[i].rfind("id,", 0) == 0) continue;   // header
            splitCsv(lines[i], c);
            ProgramRecord r;
            int size = 0;
            bool good = c.size() == 8 && Assembler::parseNumber(c[0], r.id) &&
                        Assembler::parseNumber(c[3], r.instructionCount) &&
                        Assembler::parseNumber(c[4], r.executionCount) &&
                        Assembler::parseNumber(c[5], size) && !c[1].empty() && r.id > 0;
            if (!good) {
                warnings.pushBack("Corrupted metadata at " + indexPath_ + " line " + std::to_string(i + 1) +
                                  " - row skipped: \"" + lines[i] + "\"");
                continue;
            }
            if (find(c[1])) { warnings.pushBack("Duplicate entry '" + c[1] + "' in index skipped."); continue; }
            r.name = c[1]; r.path = c[2]; r.fileSize = size; r.status = c[6]; r.lastExecution = c[7];
            if (!FileManager::exists(r.path)) r.status = "MISSING";
            if (r.id >= nextId_) nextId_ = r.id + 1;
            list_.pushBack(r);
        }
        return true;
    }

    bool save() const {
        DynamicArray<std::string> out;
        out.pushBack("id,name,path,instructions,executions,size,status,last_execution");
        list_.forEach([&](const ProgramRecord& r) {
            std::ostringstream ss;
            ss << r.id << ',' << sanitize(r.name) << ',' << sanitize(r.path) << ',' << r.instructionCount << ','
               << r.executionCount << ',' << r.fileSize << ',' << r.status << ',' << sanitize(r.lastExecution);
            out.pushBack(ss.str());
        });
        std::string err;
        std::size_t slash = indexPath_.find_last_of('/');
        if (slash != std::string::npos) FileManager::ensureDirectory(indexPath_.substr(0, slash), err);
        if (!FileManager::writeLines(indexPath_, out, err)) { std::cerr << "ERROR: " << err << "\n"; return false; }
        return true;
    }

    // Add .asm files that exist on disk but not in the index; refresh stats.
    int syncWithDirectory() {
        int added = 0;
        DynamicArray<std::string> files = FileManager::listFiles(programsDir_, ".asm");
        for (std::size_t i = 0; i < files.size(); ++i) {
            if (!find(files[i])) { refresh(files[i], FileManager::join(programsDir_, files[i])); ++added; }
        }
        for (auto* n = list_.head(); n; n = n->next)
            if (!FileManager::exists(n->data.path)) n->data.status = "MISSING";
        save();
        return added;
    }

    // Insert or update a record from the file on disk (counts instructions).
    ProgramRecord& refresh(const std::string& name, const std::string& path) {
        ProgramRecord* r = find(name);
        if (!r) {
            ProgramRecord nr;
            nr.id = nextId_++;
            nr.name = name;
            list_.pushBack(nr);
            r = find(name);
        }
        r->path = path;
        DynamicArray<std::string> lines;
        std::string err;
        if (FileManager::readLines(path, lines, err)) {
            AssemblyResult a = Assembler::assemble(lines);
            r->instructionCount = static_cast<int>(a.instructions.size());
            r->status = a.ok() ? "OK" : "ERRORS";
            r->fileSize = FileManager::fileSize(path);
        } else r->status = "MISSING";
        save();
        return *r;
    }

    void recordExecution(const std::string& name, const std::string& when) {
        if (ProgramRecord* r = find(name)) { r->executionCount++; r->lastExecution = when; save(); }
    }

    bool remove(const std::string& name) {
        bool ok = list_.removeIf([&](const ProgramRecord& r) { return r.name == name; });
        if (ok) save();
        return ok;
    }

    // ---------------- searching ----------------
    ProgramRecord* find(const std::string& name, long* comparisons = nullptr) {
        return list_.find([&](const ProgramRecord& r) { return r.name == name; }, comparisons);
    }

    // Linear search by case-insensitive substring: all matches.
    DynamicArray<ProgramRecord> searchByName(const std::string& part, long& comparisons) const {
        DynamicArray<ProgramRecord> hits;
        std::string p = Assembler::upper(part);
        list_.forEach([&](const ProgramRecord& r) {
            ++comparisons;
            if (Assembler::upper(r.name).find(p) != std::string::npos) hits.pushBack(r);
        });
        return hits;
    }

    // Binary search for an exact name. Copies to an array and sorts it by name
    // first (insertion sort), because binary search REQUIRES sorted data.
    SearchResult binarySearchByName(const std::string& name, DynamicArray<ProgramRecord>& sortedOut,
                                    SortStats& sortStats) const {
        list_.toArray(sortedOut);
        sortStats = Sorting::insertionSort(sortedOut, [](const ProgramRecord& a, const ProgramRecord& b) {
            return a.name < b.name;
        });
        return Searching::binarySearch(sortedOut, name, [](const ProgramRecord& r) -> const std::string& { return r.name; });
    }

    // ---------------- sorting ----------------
    // The list is copied into a DynamicArray, sorted with the chosen
    // algorithm, and the list is rebuilt in the new order (O(n) extra).
    SortStats sort(SortMethod method, SortKey key, bool descending = false) {
        auto less = [key, descending](const ProgramRecord& a, const ProgramRecord& b) {
            bool lt;
            switch (key) {
                case SortKey::INSTRUCTIONS: lt = descending ? a.instructionCount > b.instructionCount : a.instructionCount < b.instructionCount; break;
                case SortKey::EXECUTIONS: lt = descending ? a.executionCount > b.executionCount : a.executionCount < b.executionCount; break;
                case SortKey::FILE_SIZE: lt = descending ? a.fileSize > b.fileSize : a.fileSize < b.fileSize; break;
                default: lt = descending ? a.name > b.name : a.name < b.name;
            }
            return lt;
        };
        DynamicArray<ProgramRecord> arr;
        list_.toArray(arr);
        SortStats s;
        if (method == SortMethod::BUBBLE) s = Sorting::bubbleSort(arr, less);
        else if (method == SortMethod::SELECTION) s = Sorting::selectionSort(arr, less);
        else s = Sorting::insertionSort(arr, less);
        list_.fromArray(arr);
        save();
        return s;
    }
};
