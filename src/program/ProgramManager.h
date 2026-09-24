// ============================================================================
// ProgramManager.h  -  Phases 10 + 14
// Owns the program currently being edited and implements:
//   create / open / save / save-as / delete   (via FileManager + Repository)
//   editing operations on the DynamicArray of source lines
//   UNDO / REDO with two Stacks of whole-program snapshots.
//
// UNDO / REDO algorithm
//   Before every modification:  push(current lines) onto UNDO; clear REDO.
//   Undo:  push(current) onto REDO;  current = pop(UNDO)
//   Redo:  push(current) onto UNDO;  current = pop(REDO)
//   A new edit after an undo clears REDO (that "future" no longer exists).
// We store full copies of the line array ("memento" snapshots): simple and
// correct; the cost is O(n) memory per snapshot. Capacity is limited to
// MAX_UNDO; when full, the oldest state is discarded (see pushUndo).
// ============================================================================
#pragma once
#include <string>
#include "../data_structures/Stack.h"
#include "../filesystem/FileManager.h"
#include "Program.h"
#include "ProgramRepository.h"

class ProgramManager {
public:
    static const std::size_t MAX_UNDO = 50;

private:
    Program current_;
    Stack<DynamicArray<std::string>> undo_{MAX_UNDO};
    Stack<DynamicArray<std::string>> redo_{MAX_UNDO};
    ProgramRepository& repo_;
    std::string programsDir_;

    // If the undo stack is full, drop the bottom (oldest) element by
    // rebuilding the stack without it. Rare, O(MAX_UNDO).
    void pushUndo(const DynamicArray<std::string>& state) {
        if (undo_.isFull()) {
            Stack<DynamicArray<std::string>> tmp(MAX_UNDO);
            while (!undo_.isEmpty()) tmp.push(undo_.pop());
            tmp.pop();   // oldest
            while (!tmp.isEmpty()) undo_.push(tmp.pop());
        }
        undo_.push(state);
    }
    void beforeEdit() {
        pushUndo(current_.lines);
        redo_.clear();
        current_.modified = true;
    }
    static bool validName(std::string& name, std::string& err) {
        name = Assembler::trim(name);
        if (name.empty()) { err = "Program name cannot be empty."; return false; }
        for (char c : name)
            if (c == '/' || c == '\\' || c == ',' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                err = std::string("Program name contains illegal character '") + c + "'.";
                return false;
            }
        if (name.size() < 4 || name.substr(name.size() - 4) != ".asm") name += ".asm";
        return true;
    }

public:
    ProgramManager(ProgramRepository& repo, std::string programsDir)
        : repo_(repo), programsDir_(std::move(programsDir)) {}

    Program& current() { return current_; }
    const Program& current() const { return current_; }
    const Stack<DynamicArray<std::string>>& undoStack() const { return undo_; }
    const Stack<DynamicArray<std::string>>& redoStack() const { return redo_; }

    // ---------------- file-level operations ----------------
    bool create(std::string name, std::string& err) {
        if (!validName(name, err)) return false;
        std::string path = FileManager::join(programsDir_, name);
        if (FileManager::exists(path)) { err = "Program '" + name + "' already exists. Open it instead."; return false; }
        current_ = Program{};
        current_.name = name;
        current_.path = path;
        current_.modified = true;
        undo_.clear(); redo_.clear();
        return true;   // not on disk until saved
    }

    bool open(std::string name, std::string& err) {
        if (!validName(name, err)) return false;
        std::string path = FileManager::join(programsDir_, name);
        if (ProgramRecord* r = repo_.find(name)) path = r->path;
        Program p;
        if (!FileManager::readLines(path, p.lines, err)) return false;
        p.name = name;
        p.path = path;
        current_ = p;
        undo_.clear(); redo_.clear();
        repo_.refresh(name, path);
        return true;
    }

    bool save(std::string& err) {
        if (!current_.loaded()) { err = "No program is open."; return false; }
        if (!FileManager::writeLines(current_.path, current_.lines, err)) return false;
        current_.modified = false;
        repo_.refresh(current_.name, current_.path);
        return true;
    }

    bool saveAs(std::string newName, bool overwrite, std::string& err) {
        if (!current_.loaded()) { err = "No program is open."; return false; }
        if (!validName(newName, err)) return false;
        std::string path = FileManager::join(programsDir_, newName);
        if (FileManager::exists(path) && !overwrite) { err = "File '" + newName + "' exists (overwrite not confirmed)."; return false; }
        current_.name = newName;
        current_.path = path;
        return save(err);
    }

    bool remove(std::string name, std::string& err) {
        if (!validName(name, err)) return false;
        std::string path = FileManager::join(programsDir_, name);
        if (ProgramRecord* r = repo_.find(name)) path = r->path;
        bool fileGone = FileManager::removeFile(path, err);
        bool indexGone = repo_.remove(name);
        if (!fileGone && !indexGone) return false;
        if (current_.name == name) { current_ = Program{}; undo_.clear(); redo_.clear(); }
        err.clear();
        return true;
    }

    // ---------------- editor operations (all undoable) ----------------
    void addLine(const std::string& line) { beforeEdit(); current_.lines.pushBack(line); }
    bool insertLine(std::size_t pos, const std::string& line, std::string& err) {
        if (pos > current_.lines.size()) { err = "Position " + std::to_string(pos + 1) + " is out of range."; return false; }
        beforeEdit();
        current_.lines.insert(pos, line);
        return true;
    }
    bool modifyLine(std::size_t pos, const std::string& line, std::string& err) {
        if (pos >= current_.lines.size()) { err = "Line " + std::to_string(pos + 1) + " does not exist."; return false; }
        beforeEdit();
        current_.lines.update(pos, line);
        return true;
    }
    bool deleteLine(std::size_t pos, std::string& err) {
        if (pos >= current_.lines.size()) { err = "Line " + std::to_string(pos + 1) + " does not exist."; return false; }
        beforeEdit();
        current_.lines.removeAt(pos);
        return true;
    }
    bool undo(std::string& err) {
        if (undo_.isEmpty()) { err = "Nothing to undo."; return false; }
        redo_.push(current_.lines);
        current_.lines = undo_.pop();
        current_.modified = true;
        return true;
    }
    bool redo(std::string& err) {
        if (redo_.isEmpty()) { err = "Nothing to redo."; return false; }
        pushUndo(current_.lines);
        current_.lines = redo_.pop();
        current_.modified = true;
        return true;
    }
};
