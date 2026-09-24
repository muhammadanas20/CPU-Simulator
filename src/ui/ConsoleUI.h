// ============================================================================
// ConsoleUI.h  -  Phase 17: integrates every module behind console menus.
// The UI owns no algorithms itself; it only calls the managers and shows
// their data structures.
// ============================================================================
#pragma once
#include <iomanip>
#include <iostream>
#include <string>
#include "../filesystem/ReportManager.h"
#include "../filesystem/SnapshotManager.h"
#include "../program/ProgramManager.h"
#include "../simulator/CPU.h"
#include "Menu.h"

struct PerfStats {
    long lastSearchComparisons = 0, totalSearchComparisons = 0;
    long lastSortComparisons = 0, lastSortSwaps = 0, totalSortComparisons = 0, totalSortSwaps = 0;
    std::string lastSortAlgorithm = "-";
};

class ConsoleUI {
    std::string base_;
    std::string programsDir_, dataDir_, reportsDir_, snapshotsDir_;
    ProgramRepository repo_;
    ProgramManager pm_;
    CPU cpu_;
    PerfStats perf_;
    bool executionRecorded_ = false;
    int stepLimit_ = CPU::DEFAULT_STEP_LIMIT;
    bool verboseStep_ = true;

    // ----------------------------------------------------------------- utils
    static const char* keyName(SortKey k) {
        switch (k) { case SortKey::INSTRUCTIONS: return "Instruction Count"; case SortKey::EXECUTIONS: return "Execution Count";
                     case SortKey::FILE_SIZE: return "File Size"; default: return "Name"; }
    }
    void printRecordTable(const LinkedList<ProgramRecord>& list) {
        if (list.empty()) { std::cout << "  (repository is empty)\n"; return; }
        std::cout << "  " << std::left << std::setw(4) << "ID" << std::setw(22) << "Name" << std::right << std::setw(7) << "Instr"
                  << std::setw(7) << "Runs" << std::setw(8) << "Bytes" << "  " << std::left << std::setw(8) << "Status" << "Last run\n";
        list.forEach([](const ProgramRecord& r) {
            std::cout << "  " << std::left << std::setw(4) << r.id << std::setw(22) << r.name << std::right << std::setw(7)
                      << r.instructionCount << std::setw(7) << r.executionCount << std::setw(8) << r.fileSize << "  " << std::left
                      << std::setw(8) << r.status << r.lastExecution << "\n";
        });
        std::cout << std::right;
    }
    void printArrayTable(const DynamicArray<ProgramRecord>& a) {
        LinkedList<ProgramRecord> tmp;
        tmp.fromArray(a);
        printRecordTable(tmp);
    }
    void showListDiagram() {
        std::cout << "\n  PROGRAM REPOSITORY (singly linked list, " << repo_.size() << " nodes)\n\n  HEAD\n    |\n    v\n";
        for (auto* n = repo_.list().head(); n; n = n->next)
            std::cout << "  [ #" << n->data.id << " " << n->data.name << " | " << n->data.instructionCount << " instr | "
                      << n->data.executionCount << " runs | " << n->data.fileSize << " B | " << n->data.status << " ]\n        |\n        v\n";
        std::cout << "      NULL\n";
    }
    std::string chooseProgramName(const std::string& prompt) {
        printRecordTable(repo_.list());
        return Assembler::trim(Menu::readLine(prompt));
    }
    void showProgram() {
        const Program& p = pm_.current();
        if (!p.loaded()) { Menu::error("No program is open. Create or open one first."); return; }
        std::cout << "\n  " << p.name << (p.modified ? "  [modified - not saved]" : "") << "   (" << p.path << ")\n";
        std::cout << "  ---------------------------------------------\n";
        if (p.lines.empty()) std::cout << "  (empty)\n";
        for (std::size_t i = 0; i < p.lines.size(); ++i) std::cout << "  " << std::setw(3) << i + 1 << " | " << p.lines[i] << "\n";
    }

    // ------------------------------------------------------- program mgmt
    void createProgram() {
        std::string err, name = Menu::readLine("New program name (e.g. myprog.asm): ");
        if (!pm_.create(name, err)) { Menu::error(err); return; }
        Menu::ok("Created '" + pm_.current().name + "' in memory. Opening editor - remember to Save.");
        editor();
    }
    void openProgram() {
        std::string name = chooseProgramName("Program name to open: "), err;
        if (!pm_.open(name, err)) { Menu::error(err); return; }
        Menu::ok("Opened '" + pm_.current().name + "' (" + std::to_string(pm_.current().lines.size()) + " lines).");
        showProgram();
    }
    void saveProgram() {
        std::string err;
        if (!pm_.save(err)) { Menu::error(err); return; }
        Menu::ok("Saved to " + pm_.current().path);
        reportAssembly();
    }
    void saveAs() {
        if (!pm_.current().loaded()) { Menu::error("No program is open."); return; }
        std::string name = Menu::readLine("Save as (new name): "), err;
        bool ow = false;
        std::string probe = name;
        if (probe.size() < 4 || probe.substr(probe.size() - 4) != ".asm") probe += ".asm";
        if (FileManager::exists(FileManager::join(programsDir_, Assembler::trim(probe)))) {
            ow = Menu::confirm("'" + probe + "' already exists. Overwrite?");
            if (!ow) { std::cout << "  Cancelled.\n"; return; }
        }
        if (!pm_.saveAs(name, ow, err)) { Menu::error(err); return; }
        Menu::ok("Saved as " + pm_.current().path);
    }
    void deleteProgram() {
        std::string name = chooseProgramName("Program name to DELETE: "), err;
        if (name.empty()) return;
        if (!Menu::confirm("Really delete '" + name + "' from disk and index?")) { std::cout << "  Cancelled.\n"; return; }
        if (!pm_.remove(name, err)) { Menu::error(err); return; }
        Menu::ok("Deleted '" + name + "'.");
    }
    void programDetails() {
        std::string name = chooseProgramName("Program name for details: ");
        long cmp = 0;
        ProgramRecord* r = repo_.find(name, &cmp);
        if (!r && name.find(".asm") == std::string::npos) r = repo_.find(name + ".asm", &cmp);
        if (!r) { Menu::error("Program '" + name + "' not found (" + std::to_string(cmp) + " nodes compared)."); return; }
        std::cout << "\n  PROGRAM METADATA  (found after " << cmp << " linked-list comparisons)\n"
                  << "  ID               : " << r->id << "\n  Name             : " << r->name << "\n  File path        : " << r->path
                  << "\n  Instruction count: " << r->instructionCount << "\n  Execution count  : " << r->executionCount
                  << "\n  File size        : " << r->fileSize << " bytes\n  Status           : " << r->status
                  << "\n  Last execution   : " << r->lastExecution << "\n";
    }
    void reportAssembly() {
        AssemblyResult a = Assembler::assemble(pm_.current().lines);
        if (a.ok()) std::cout << "  Assembly check: " << a.instructions.size() << " instructions, " << a.symbols.size() << " labels, no errors.\n";
        else {
            std::cout << "  Assembly check found " << a.errors.size() << " problem(s):\n";
            for (std::size_t i = 0; i < a.errors.size(); ++i) std::cout << "   - " << a.errors[i] << "\n";
        }
    }

    void editor() {
        if (!pm_.current().loaded()) { Menu::error("No program is open. Create or open one first."); return; }
        while (true) {
            Menu::header("PROGRAM EDITOR - " + pm_.current().name);
            std::cout << "1. Add instruction\n2. Insert instruction\n3. Modify instruction\n4. Delete instruction\n"
                         "5. Display program\n6. Save\n7. Undo  (" << pm_.undoStack().size() << " states)\n8. Redo  ("
                      << pm_.redoStack().size() << " states)\n9. Exit editor\n10. Check program (assemble)\n";
            int c = Menu::readInt("Enter choice: ", 1, 10, 9);
            std::string err;
            size_t n = pm_.current().lines.size();
            switch (c) {
                case 1: { std::string l = Menu::readLine("Line (e.g. MOV R1, 10 or LOOP:): "); if (!Menu::eofReached()) { pm_.addLine(l); Menu::ok("Added as line " + std::to_string(n + 1)); } break; }
                case 2: {
                    showProgram();
                    int pos = Menu::readInt("Insert BEFORE line number (1-" + std::to_string(n + 1) + "): ", 1, static_cast<int>(n) + 1, -1);
                    if (pos < 0) break;
                    std::string l = Menu::readLine("Line: ");
                    if (pm_.insertLine(pos - 1, l, err)) Menu::ok("Inserted."); else Menu::error(err);
                    break;
                }
                case 3: {
                    if (!n) { Menu::error("Program is empty - nothing to modify."); break; }
                    showProgram();
                    int pos = Menu::readInt("Line number to modify: ", 1, static_cast<int>(n), -1);
                    if (pos < 0) break;
                    std::cout << "  Current: " << pm_.current().lines[pos - 1] << "\n";
                    std::string l = Menu::readLine("New text: ");
                    if (pm_.modifyLine(pos - 1, l, err)) Menu::ok("Modified."); else Menu::error(err);
                    break;
                }
                case 4: {
                    if (!n) { Menu::error("Program is empty - nothing to delete."); break; }
                    showProgram();
                    int pos = Menu::readInt("Line number to delete: ", 1, static_cast<int>(n), -1);
                    if (pos < 0) break;
                    if (pm_.deleteLine(pos - 1, err)) Menu::ok("Deleted."); else Menu::error(err);
                    break;
                }
                case 5: showProgram(); break;
                case 6: saveProgram(); break;
                case 7: if (pm_.undo(err)) { Menu::ok("Undone."); showProgram(); } else Menu::error(err); break;
                case 8: if (pm_.redo(err)) { Menu::ok("Redone."); showProgram(); } else Menu::error(err); break;
                case 10: reportAssembly(); break;
                case 9:
                    if (pm_.current().modified && !Menu::eofReached() && Menu::confirm("Unsaved changes. Save now?")) saveProgram();
                    return;
            }
        }
    }

    void programMenu() {
        while (true) {
            Menu::header("PROGRAM MANAGEMENT");
            std::cout << "Current: " << (pm_.current().loaded() ? pm_.current().name : "(none)") << "\n\n"
                      << "1. Create Program\n2. Open Program\n3. Edit Program\n4. Save Program\n5. Save As\n6. Delete Program\n"
                         "7. List Programs\n8. Search\n9. Sort\n10. Program Details (metadata)\n11. Rescan programs/ folder\n0. Back\n";
            switch (Menu::readInt("Enter choice: ", 0, 11)) {
                case 1: createProgram(); break;
                case 2: openProgram(); break;
                case 3: editor(); break;
                case 4: saveProgram(); break;
                case 5: saveAs(); break;
                case 6: deleteProgram(); break;
                case 7: printRecordTable(repo_.list()); break;
                case 8: searchMenu(); break;
                case 9: sortMenu(); break;
                case 10: programDetails(); break;
                case 11: { int a = repo_.syncWithDirectory(); Menu::ok(std::to_string(a) + " new file(s) added to the index."); break; }
                case 0: return;
            }
        }
    }

    // ------------------------------------------------------------- CPU
    void loadIntoCpu() {
        std::cout << "1. Load the program open in the editor" << (pm_.current().loaded() ? " (" + pm_.current().name + ")" : " (none)")
                  << "\n2. Load a program file by name\n0. Cancel\n";
        int c = Menu::readInt("Enter choice: ", 0, 2);
        DynamicArray<std::string> lines;
        std::string name, err;
        if (c == 0) return;
        if (c == 1) {
            if (!pm_.current().loaded()) { Menu::error("No program is open in the editor."); return; }
            lines = pm_.current().lines;
            name = pm_.current().name;
        } else {
            name = chooseProgramName("Program name: ");
            if (name.size() < 4 || name.substr(name.size() - 4) != ".asm") name += ".asm";
            std::string path = FileManager::join(programsDir_, name);
            if (ProgramRecord* r = repo_.find(name)) path = r->path;
            if (!FileManager::readLines(path, lines, err)) { Menu::error(err); return; }
        }
        AssemblyResult a = Assembler::assemble(lines);            // lines -> instruction array + symbol table
        if (!a.ok()) {
            std::cout << "\n  ERROR: '" << name << "' cannot be loaded (" << a.errors.size() << " error(s)):\n";
            for (std::size_t i = 0; i < a.errors.size(); ++i) std::cout << "   - " << a.errors[i] << "\n";
            return;
        }
        cpu_.load(name, a.instructions, a.symbols);              // array -> queue
        executionRecorded_ = false;
        Menu::ok("Loaded '" + name + "': " + std::to_string(a.instructions.size()) + " instructions, " +
                 std::to_string(a.symbols.size()) + " labels. Instruction queue filled with " + std::to_string(cpu_.queue().size()) + " entries.");
    }
    void afterExecution() {
        if (!executionRecorded_ && (cpu_.state() == CpuState::HALTED || cpu_.state() == CpuState::ERROR)) {
            repo_.recordExecution(cpu_.programName(), ReportManager::now());
            executionRecorded_ = true;
        }
    }
    void doRun() {
        if (cpu_.state() == CpuState::EMPTY) { Menu::error("No program loaded. Use 'Load Program' first."); return; }
        if (cpu_.state() == CpuState::HALTED || cpu_.state() == CpuState::ERROR) { Menu::error("Program already finished. Use Reset to run again."); return; }
        int before = cpu_.steps();
        std::string why = cpu_.run(stepLimit_);
        std::cout << "\n  Executed " << (cpu_.steps() - before) << " step(s). " << why << "\n\n" << cpu_.registers().table() << cpu_.registers().flagTable();
        afterExecution();
    }
    void doStep() {
        if (cpu_.state() == CpuState::EMPTY) { Menu::error("No program loaded. Use 'Load Program' first."); return; }
        if (cpu_.state() == CpuState::HALTED || cpu_.state() == CpuState::ERROR) { Menu::error("Program already finished (" + std::string(cpuStateName(cpu_.state())) + "). Use Reset."); return; }
        RegisterFile before = cpu_.registers();
        std::string instr = cpu_.queue().isEmpty() ? "?" : cpu_.queue().peek().toString();
        if (!cpu_.step()) { std::cout << "  " << cpu_.statusMessage() << cpu_.lastError() << "\n"; afterExecution(); return; }
        const RegisterFile& after = cpu_.registers();
        const HistoryEntry& h = cpu_.history().get(cpu_.history().size() - 1);
        std::cout << "\n  Step " << h.step << "   Current PC: " << h.pc << "\n\n  Instruction:\n  " << instr << "\n";
        if (verboseStep_) {
            std::string b, a;
            for (int i = 0; i < RegisterFile::COUNT; ++i)
                if (before.R[i] != after.R[i]) {
                    b += "  R" + std::to_string(i) + " = " + std::to_string(before.R[i]) + "\n";
                    a += "  R" + std::to_string(i) + " = " + std::to_string(after.R[i]) + "\n";
                }
            if (before.SP != after.SP) { b += "  SP = " + std::to_string(before.SP) + "\n"; a += "  SP = " + std::to_string(after.SP) + "\n"; }
            std::cout << "\n  Before:\n" << (b.empty() ? "  (no register changed)\n" : b) << "\n  After:\n" << (a.empty() ? "  (no register changed)\n" : a);
            std::cout << "\n  Flags: ZF=" << after.flags.ZF << " SF=" << after.flags.SF << " CF=" << after.flags.CF << " OF=" << after.flags.OF
                      << "    Next PC: " << after.PC << "\n";
        }
        std::cout << "  Result: " << h.result << "\n  State : " << cpuStateName(cpu_.state()) << "\n";
        if (cpu_.state() == CpuState::ERROR) Menu::error(cpu_.lastError());
        afterExecution();
    }
    void doPause() {
        std::cout << "Pause works as a BREAKPOINT: 'Run' stops before executing the chosen address.\nCurrent breakpoint: "
                  << (cpu_.breakpoint() < 0 ? std::string("none") : std::to_string(cpu_.breakpoint())) << "\n";
        showInstructionArray();
        int max = static_cast<int>(cpu_.program().size()) - 1;
        if (max < 0) { Menu::error("No program loaded."); return; }
        int bp = Menu::readInt("Breakpoint address (-1 to clear): ", -1, max, -2);
        if (bp == -2) return;
        cpu_.setBreakpoint(bp);
        Menu::ok(bp < 0 ? "Breakpoint cleared." : "Run will pause at address " + std::to_string(bp) + ".");
    }
    void showRegisters() {
        std::cout << "\n  SIMPLIFIED SIMULATED CPU REGISTERS   state: " << cpuStateName(cpu_.state()) << "\n" << cpu_.registers().table()
                  << "\n  FLAGS\n" << cpu_.registers().flagTable();
    }
    void showMemory() {
        std::cout << "1. Non-zero cells only\n2. Address range\n";
        if (Menu::readInt("Enter choice: ", 1, 2, 1) == 1) { cpu_.memory().displayNonZero(); return; }
        int from = Menu::readInt("From address (0-1023): ", 0, Memory::SIZE - 1);
        int to = Menu::readInt("To address (" + std::to_string(from) + "-1023): ", from, Memory::SIZE - 1, from);
        cpu_.memory().displayRange(from, to);
    }
    void showStack() {
        std::cout << "\n  CPU STACK (custom array stack, capacity " << cpu_.stack().capacity() << ")   SP = " << cpu_.registers().SP << "\n";
        cpu_.stack().display();
    }
    void showQueue() {
        std::cout << "\n  INSTRUCTION QUEUE (" << cpu_.queue().size() << " entries, " << cpu_.queue().operations()
                  << " enqueue/dequeue ops, " << cpu_.queueFlushes() << " flushes)\n";
        if (cpu_.queue().isEmpty()) { std::cout << "  FRONT -> (empty) <- REAR\n"; return; }
        std::cout << "  FRONT\n   |\n   v\n";
        cpu_.queue().forEach([](const Instruction& i) { std::cout << "  [ " << std::setw(3) << i.address << ": " << std::left << std::setw(16) << i.toString() << std::right << " ]\n"; });
        std::cout << "   ^\n   |\n  REAR\n";
    }
    void showHistory() {
        std::cout << "\n  EXECUTION HISTORY (linked list, " << cpu_.history().size() << " nodes)\n";
        if (cpu_.history().empty()) { std::cout << "  HEAD -> NULL\n"; return; }
        std::cout << "  HEAD\n";
        cpu_.history().forEach([](const HistoryEntry& h) {
            std::cout << "   -> [Step " << h.step << " | PC=" << h.pc << " | " << h.instruction << " | " << h.result << "]\n";
        });
        std::cout << "   -> NULL\n";
        if (Menu::confirm("Show full register state for each step?"))
            cpu_.history().forEach([](const HistoryEntry& h) {
                std::cout << "  Step " << h.step << ": " << h.instruction << "\n    before: " << h.before << "\n    after : " << h.after << "\n";
            });
    }
    void showInstructionArray() {
        const auto& p = cpu_.program();
        std::cout << "\n  INSTRUCTION ARRAY (DynamicArray)  size=" << p.size() << " capacity=" << p.capacity() << "\n";
        for (std::size_t i = 0; i < p.size(); ++i)
            std::cout << "  [" << std::setw(2) << i << "] " << std::left << std::setw(16) << p[i].toString() << std::right << " (line " << p[i].sourceLine << ")"
                      << (static_cast<int>(i) == cpu_.registers().PC ? "   <- PC" : "") << "\n";
    }
    void saveReport() {
        std::string path, err;
        if (ReportManager::saveReport(cpu_, reportsDir_, path, err)) Menu::ok("Report written to " + path);
        else Menu::error(err);
    }
    void saveHistory() {
        std::string path, err;
        if (ReportManager::saveHistory(cpu_, reportsDir_, path, err)) Menu::ok("History written to " + path);
        else Menu::error(err);
    }

    void cpuMenu() {
        while (true) {
            Menu::header("CPU SIMULATION");
            std::cout << "Program: " << (cpu_.programName().empty() ? "(none)" : cpu_.programName()) << "   State: " << cpuStateName(cpu_.state())
                      << "   PC: " << cpu_.registers().PC << "   Steps: " << cpu_.steps() << "\n\n"
                      << "1. Load Program\n2. Run\n3. Step\n4. Pause (set breakpoint)\n5. Reset\n6. Registers\n7. Memory\n8. Stack\n"
                         "9. Queue\n10. Execution History\n11. Save Report\n12. Symbol Table\n13. Instruction Array\n14. Save History\n"
                         "15. Clear History\n0. Back\n";
            switch (Menu::readInt("Enter choice: ", 0, 15)) {
                case 1: loadIntoCpu(); break;
                case 2: doRun(); break;
                case 3: doStep(); break;
                case 4: doPause(); break;
                case 5: if (cpu_.state() == CpuState::EMPTY && cpu_.program().empty()) Menu::error("No program loaded."); else { cpu_.reset(); executionRecorded_ = false; Menu::ok("CPU reset: registers, memory, stack, queue and history cleared."); } break;
                case 6: showRegisters(); break;
                case 7: showMemory(); break;
                case 8: showStack(); break;
                case 9: showQueue(); break;
                case 10: showHistory(); break;
                case 11: saveReport(); break;
                case 12: cpu_.symbols().display(); break;
                case 13: showInstructionArray(); break;
                case 14: saveHistory(); break;
                case 15: cpu_.clearHistory(); Menu::ok("Execution history cleared."); break;
                case 0: return;
            }
        }
    }

    // ------------------------------------------------ DS playgrounds
    void arrayPlayground() {
        DynamicArray<int> a(2);
        while (true) {
            std::cout << "\n  DYNAMIC ARRAY SANDBOX (starts with capacity 2 to show resizing)\n"; a.display();
            std::cout << "1. Insert at end  2. Insert at index  3. Remove  4. Update  5. Get  6. Search  0. Back\n";
            int c = Menu::readInt("Enter choice: ", 0, 6);
            if (c == 0) return;
            try {
                if (c == 1) { std::size_t cap = a.capacity(); a.pushBack(Menu::readInt("Value: ", -1000000, 1000000)); if (a.capacity() != cap) std::cout << "  RESIZE: capacity " << cap << " -> " << a.capacity() << " (all elements copied)\n"; }
                else if (c == 2) { int i = Menu::readInt("Index: ", 0, 1000000); a.insert(i, Menu::readInt("Value: ", -1000000, 1000000)); }
                else if (c == 3) a.removeAt(Menu::readInt("Index: ", 0, 1000000));
                else if (c == 4) { int i = Menu::readInt("Index: ", 0, 1000000); a.update(i, Menu::readInt("Value: ", -1000000, 1000000)); }
                else if (c == 5) { int i = Menu::readInt("Index: ", 0, 1000000); std::cout << "  a[" << i << "] = " << a.get(i) << "\n"; }
                else { int v = Menu::readInt("Value: ", -1000000, 1000000); long i = a.search(v); std::cout << (i < 0 ? "  Not found\n" : "  Found at index " + std::to_string(i) + "\n"); }
            } catch (const std::exception& e) { Menu::error(e.what()); }
        }
    }
    void stackPlayground() {
        Stack<int> s(5);
        while (true) {
            std::cout << "\n  STACK SANDBOX (capacity 5 to show overflow)\n"; s.display();
            std::cout << "1. Push  2. Pop  3. Peek  4. isEmpty/isFull  0. Back\n";
            int c = Menu::readInt("Enter choice: ", 0, 4);
            if (c == 0) return;
            try {
                if (c == 1) s.push(Menu::readInt("Value: ", -1000000, 1000000));
                else if (c == 2) std::cout << "  Popped " << s.pop() << "\n";
                else if (c == 3) std::cout << "  Top = " << s.peek() << "\n";
                else std::cout << "  isEmpty=" << s.isEmpty() << " isFull=" << s.isFull() << "\n";
            } catch (const std::exception& e) { Menu::error(e.what()); }
        }
    }
    void queuePlayground() {
        Queue<int> q;
        while (true) {
            std::cout << "\n  QUEUE SANDBOX\n  FRONT -> "; q.forEach([](int v) { std::cout << "[" << v << "] "; }); std::cout << "<- REAR\n";
            std::cout << "1. Enqueue  2. Dequeue  3. Peek  0. Back\n";
            int c = Menu::readInt("Enter choice: ", 0, 3);
            if (c == 0) return;
            try {
                if (c == 1) q.enqueue(Menu::readInt("Value: ", -1000000, 1000000));
                else if (c == 2) std::cout << "  Dequeued " << q.dequeue() << "\n";
                else std::cout << "  Front = " << q.peek() << "\n";
            } catch (const std::exception& e) { Menu::error(e.what()); }
        }
    }
    void hashPlayground() {
        HashTable<int> h(5);
        while (true) {
            std::cout << "\n  HASH TABLE SANDBOX (5 buckets to force collisions; rehash when load > 1)\n"; h.display();
            std::cout << "1. Insert  2. Search  3. Delete  0. Back\n";
            int c = Menu::readInt("Enter choice: ", 0, 3);
            if (c == 0) return;
            std::string k = Assembler::trim(Menu::readLine("Key: "));
            if (k.empty()) { Menu::error("Key cannot be empty."); continue; }
            std::cout << "  hash(\"" << k << "\") = " << h.hash(k) << "\n";
            if (c == 1) { int v = Menu::readInt("Value: ", -1000000, 1000000); if (!h.insert(k, v)) Menu::error("Duplicate key '" + k + "'."); }
            else if (c == 2) { int v; if (h.search(k, v)) std::cout << "  " << k << " -> " << v << "\n"; else Menu::error("Key '" + k + "' not found."); }
            else { if (!h.remove(k)) Menu::error("Key '" + k + "' not found."); }
        }
    }
    void dsMenu() {
        while (true) {
            Menu::header("DATA STRUCTURES");
            std::cout << "Live structures used by the project:\n"
                         "1. Dynamic Array   (instruction array + editor lines)\n2. Linked List     (program repository)\n"
                         "3. Stack           (CPU stack + undo/redo)\n4. Queue           (instruction queue)\n"
                         "5. Hash Table      (symbol table)\n6. Program Repository (table)\n7. Execution History\n"
                         "Sandboxes (try the operations yourself):\n8. Dynamic Array sandbox\n9. Stack sandbox\n10. Queue sandbox\n11. Hash Table sandbox\n0. Back\n";
            switch (Menu::readInt("Enter choice: ", 0, 11)) {
                case 1:
                    showInstructionArray();
                    if (pm_.current().loaded()) std::cout << "\n  EDITOR LINES (DynamicArray<string>) size=" << pm_.current().lines.size() << " capacity=" << pm_.current().lines.capacity() << "\n";
                    break;
                case 2: showListDiagram(); break;
                case 3:
                    showStack();
                    std::cout << "\n  UNDO STACK: " << pm_.undoStack().size() << "/" << pm_.undoStack().capacity() << " snapshots   REDO STACK: "
                              << pm_.redoStack().size() << "/" << pm_.redoStack().capacity() << "\n";
                    for (std::size_t d = 0; d < pm_.undoStack().size() && d < 5; ++d)
                        std::cout << "   undo[top-" << d << "]: program with " << pm_.undoStack().fromTop(d).size() << " lines\n";
                    break;
                case 4: showQueue(); break;
                case 5: cpu_.symbols().display(); break;
                case 6: printRecordTable(repo_.list()); break;
                case 7: showHistory(); break;
                case 8: arrayPlayground(); break;
                case 9: stackPlayground(); break;
                case 10: queuePlayground(); break;
                case 11: hashPlayground(); break;
                case 0: return;
            }
        }
    }

    // ------------------------------------------------ search / sort
    void searchMenu() {
        Menu::header("SEARCH");
        std::cout << "1. Linear search programs by name (substring)\n2. Binary search program by exact name\n"
                     "3. Linear search instruction by opcode (loaded program)\n4. Symbol table lookup (hash)\n0. Back\n";
        int c = Menu::readInt("Enter choice: ", 0, 4);
        if (c == 0) return;
        if (c == 1) {
            std::string q = Menu::readLine("Name contains: ");
            long cmp = 0;
            DynamicArray<ProgramRecord> hits = repo_.searchByName(q, cmp);
            perf_.lastSearchComparisons = cmp; perf_.totalSearchComparisons += cmp;
            std::cout << "  Linear search visited all " << cmp << " nodes (O(n)); " << hits.size() << " match(es).\n";
            printArrayTable(hits);
        } else if (c == 2) {
            std::string q = Assembler::trim(Menu::readLine("Exact program name (e.g. addition.asm): "));
            if (q.find('.') == std::string::npos) q += ".asm";
            DynamicArray<ProgramRecord> sorted; SortStats ss;
            SearchResult r = repo_.binarySearchByName(q, sorted, ss);
            perf_.lastSearchComparisons = r.comparisons; perf_.totalSearchComparisons += r.comparisons;
            std::cout << "  Binary search needs data sorted by name, so the repository was copied to an array and\n"
                      << "  insertion-sorted first (" << ss.comparisons << " comparisons). Sorted array:\n";
            for (std::size_t i = 0; i < sorted.size(); ++i) std::cout << "   [" << i << "] " << sorted[i].name << "\n";
            if (r.index >= 0) std::cout << "  FOUND '" << q << "' at sorted index " << r.index << " after " << r.comparisons << " comparison(s) (O(log n)).\n";
            else std::cout << "  '" << q << "' NOT found after " << r.comparisons << " comparison(s).\n";
        } else if (c == 3) {
            if (cpu_.program().empty()) { Menu::error("No program loaded into the CPU."); return; }
            std::string op = Assembler::upper(Assembler::trim(Menu::readLine("Opcode (e.g. ADD): ")));
            Opcode target = opcodeFromString(op);
            if (target == Opcode::INVALID) { Menu::error("'" + op + "' is not a valid opcode."); return; }
            long cmp = 0; int found = 0;
            for (std::size_t i = 0; i < cpu_.program().size(); ++i) {
                ++cmp;
                if (cpu_.program()[i].op == target) { std::cout << "  address " << i << ": " << cpu_.program()[i].toString() << "\n"; ++found; }
            }
            perf_.lastSearchComparisons = cmp; perf_.totalSearchComparisons += cmp;
            std::cout << "  " << found << " occurrence(s), " << cmp << " comparisons (linear search - the array is in program order, not sorted by opcode).\n";
        } else {
            std::string l = Assembler::upper(Assembler::trim(Menu::readLine("Label: ")));
            int addr;
            std::cout << "  hash(\"" << l << "\") -> bucket " << cpu_.symbols().table().hash(l) << "\n";
            if (cpu_.symbols().lookup(l, addr)) std::cout << "  " << l << " -> address " << addr << "\n";
            else Menu::error("Undefined label '" + l + "' in the loaded program.");
        }
    }
    void sortMenu() {
        Menu::header("SORT PROGRAMS");
        if (repo_.size() == 0) { Menu::error("Repository is empty."); return; }
        std::cout << "1. Bubble Sort\n2. Selection Sort\n3. Insertion Sort\n0. Back\n";
        int m = Menu::readInt("Method: ", 0, 3);
        if (!m) return;
        std::cout << "\nSort By:\n1. Name\n2. Instruction Count\n3. Execution Count\n4. File Size\n";
        int k = Menu::readInt("Key: ", 1, 4, 1);
        std::cout << "\nOrder:\n1. Ascending\n2. Descending\n";
        bool desc = Menu::readInt("Order: ", 1, 2, 1) == 2;
        SortStats s = repo_.sort(static_cast<SortMethod>(m), static_cast<SortKey>(k), desc);
        perf_.lastSortAlgorithm = s.algorithm; perf_.lastSortComparisons = s.comparisons; perf_.lastSortSwaps = s.swaps;
        perf_.totalSortComparisons += s.comparisons; perf_.totalSortSwaps += s.swaps;
        std::cout << "\n  " << s.algorithm << " by " << keyName(static_cast<SortKey>(k)) << (desc ? " (descending)" : " (ascending)") << "\n  n = " << repo_.size()
                  << ", comparisons = " << s.comparisons << ", " << (m == 3 ? "shifts" : "swaps") << " = " << s.swaps << "\n  Complexity: " << s.complexity << "\n\n";
        printRecordTable(repo_.list());
        std::cout << "  (new order saved to the index file)\n";
    }

    // ------------------------------------------------ reports / snapshots
    DynamicArray<std::string> pickFile(const std::string& dir, const std::string& ext, std::string& chosen) {
        DynamicArray<std::string> files = FileManager::listFiles(dir, ext);
        Sorting::insertionSort(files, [](const std::string& a, const std::string& b) { return a < b; });
        chosen.clear();
        if (files.empty()) { std::cout << "  (no " << ext << " files in " << dir << "/)\n"; return files; }
        for (std::size_t i = 0; i < files.size(); ++i) std::cout << "  " << i + 1 << ". " << files[i] << "\n";
        int c = Menu::readInt("Choose file (0 = cancel): ", 0, static_cast<int>(files.size()));
        if (c) chosen = FileManager::join(dir, files[c - 1]);
        return files;
    }
    void viewFile(const std::string& path) {
        DynamicArray<std::string> lines; std::string err;
        if (!FileManager::readLines(path, lines, err)) { Menu::error(err); return; }
        if (lines.empty()) std::cout << "  (file is empty)\n";
        for (std::size_t i = 0; i < lines.size(); ++i) std::cout << lines[i] << "\n";
    }
    void reportsMenu() {
        while (true) {
            Menu::header("REPORTS");
            std::cout << "1. Save execution report\n2. Save execution history\n3. View a report\n4. Delete a report\n0. Back\n";
            int c = Menu::readInt("Enter choice: ", 0, 4);
            std::string f, err;
            if (c == 0) return;
            if (c == 1) saveReport();
            else if (c == 2) saveHistory();
            else if (c == 3) { pickFile(reportsDir_, ".txt", f); if (!f.empty()) viewFile(f); }
            else { pickFile(reportsDir_, ".txt", f); if (!f.empty() && Menu::confirm("Delete " + f + "?")) { if (FileManager::removeFile(f, err)) Menu::ok("Deleted."); else Menu::error(err); } }
        }
    }
    void snapshotMenu() {
        while (true) {
            Menu::header("MEMORY SNAPSHOTS");
            std::cout << "Snapshots contain registers, PC, SP, flags and all non-zero memory cells.\n\n"
                         "1. Save Memory Snapshot\n2. Load Memory Snapshot\n3. View snapshot file\n4. Delete snapshot\n0. Back\n";
            int c = Menu::readInt("Enter choice: ", 0, 4);
            std::string f, err;
            if (c == 0) return;
            if (c == 1) {
                if (!FileManager::ensureDirectory(snapshotsDir_, err)) { Menu::error(err); continue; }
                f = SnapshotManager::nextPath(snapshotsDir_);
                if (SnapshotManager::save(cpu_, f, err)) Menu::ok("Snapshot saved to " + f + " (" + std::to_string(cpu_.memory().nonZeroCount()) + " non-zero cells).");
                else Menu::error(err);
            } else if (c == 2) {
                pickFile(snapshotsDir_, ".dat", f);
                if (f.empty()) continue;
                if (SnapshotManager::load(cpu_, f, err)) { Menu::ok("Snapshot loaded. Registers and memory restored; queue refilled from PC."); showRegisters(); }
                else Menu::error(err);
            } else if (c == 3) { pickFile(snapshotsDir_, ".dat", f); if (!f.empty()) viewFile(f); }
            else { pickFile(snapshotsDir_, ".dat", f); if (!f.empty() && Menu::confirm("Delete " + f + "?")) { if (FileManager::removeFile(f, err)) Menu::ok("Deleted."); else Menu::error(err); } }
        }
    }

    void statistics() {
        Menu::header("STATISTICS");
        long totalRuns = 0, totalInstr = 0;
        repo_.list().forEach([&](const ProgramRecord& r) { totalRuns += r.executionCount; totalInstr += r.instructionCount; });
        std::cout << "  Programs in repository      : " << repo_.size() << "\n  Total instructions (all)    : " << totalInstr
                  << "\n  Total executions (all)      : " << totalRuns << "\n\n  Loaded program              : " << (cpu_.programName().empty() ? "-" : cpu_.programName())
                  << "\n  Number of instructions      : " << cpu_.program().size() << "\n  Execution steps             : " << cpu_.steps()
                  << "\n  History nodes               : " << cpu_.history().size() << "\n  Queue operations            : " << cpu_.queue().operations()
                  << "  (flushes: " << cpu_.queueFlushes() << ")\n  CPU stack operations        : " << cpu_.stack().operations()
                  << "\n  Symbol table entries        : " << cpu_.symbols().size() << "\n  Hash collisions             : " << cpu_.symbols().collisions()
                  << "\n\n  Last search comparisons     : " << perf_.lastSearchComparisons << "   (session total " << perf_.totalSearchComparisons << ")"
                  << "\n  Last sort                   : " << perf_.lastSortAlgorithm << "\n  Last sort comparisons/swaps : " << perf_.lastSortComparisons << " / "
                  << perf_.lastSortSwaps << "   (session total " << perf_.totalSortComparisons << " / " << perf_.totalSortSwaps << ")"
                  << "\n  Undo / Redo stack sizes     : " << pm_.undoStack().size() << " / " << pm_.redoStack().size() << "\n";
    }
    void settings() {
        while (true) {
            Menu::header("SETTINGS");
            std::cout << "Base directory: " << base_ << "\n1. Run step limit (" << stepLimit_ << ")\n2. Detailed step output ("
                      << (verboseStep_ ? "ON" : "OFF") << ")\n3. Reload program index from disk\n0. Back\n";
            int c = Menu::readInt("Enter choice: ", 0, 3);
            if (c == 0) return;
            if (c == 1) stepLimit_ = Menu::readInt("New limit (1-1000000): ", 1, 1000000, stepLimit_);
            else if (c == 2) verboseStep_ = !verboseStep_;
            else { repo_.load(); printWarnings(); repo_.syncWithDirectory(); Menu::ok("Index reloaded."); }
        }
    }
    void printWarnings() { for (std::size_t i = 0; i < repo_.warnings.size(); ++i) std::cout << "  WARNING: " << repo_.warnings[i] << "\n"; }

public:
    explicit ConsoleUI(const std::string& base)
        : base_(base), programsDir_(FileManager::join(base, "programs")), dataDir_(FileManager::join(base, "data")),
          reportsDir_(FileManager::join(base, "reports")), snapshotsDir_(FileManager::join(base, "snapshots")),
          repo_(FileManager::join(dataDir_, "program_index.csv"), programsDir_), pm_(repo_, programsDir_) {}

    void run() {
        std::string err;
        for (const std::string& d : {programsDir_, dataDir_, reportsDir_, snapshotsDir_})
            if (!FileManager::ensureDirectory(d, err)) Menu::error(err);
        repo_.load();
        printWarnings();
        int added = repo_.syncWithDirectory();
        if (added) std::cout << "  Indexed " << added << " new program file(s) from " << programsDir_ << "/\n";
        while (true) {
            Menu::header("DATA STRUCTURE CPU PROGRAM MANAGER");
            std::cout << "\n1. Program Management\n2. CPU Simulation\n3. Data Structures\n4. Search\n5. Sort\n6. Reports\n"
                         "7. Memory Snapshots\n8. Statistics\n9. Settings\n0. Exit\n\n";
            switch (Menu::readInt("Enter choice: ", 0, 9)) {
                case 1: programMenu(); break;
                case 2: cpuMenu(); break;
                case 3: dsMenu(); break;
                case 4: searchMenu(); break;
                case 5: sortMenu(); break;
                case 6: reportsMenu(); break;
                case 7: snapshotMenu(); break;
                case 8: statistics(); break;
                case 9: settings(); break;
                case 0:
                    if (pm_.current().modified && !Menu::eofReached() && Menu::confirm("'" + pm_.current().name + "' has unsaved changes. Save before exit?")) saveProgram();
                    repo_.save();
                    std::cout << "Goodbye.\n";
                    return;
            }
            if (Menu::eofReached()) { repo_.save(); std::cout << "End of input - exiting.\n"; return; }
        }
    }
};
