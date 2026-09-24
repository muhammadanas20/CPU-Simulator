// ============================================================================
// ReportManager.h  -  Phase 15: writes reports/<program>_report.txt (ofstream)
// and saves the execution history on its own (history is a linked list that
// is traversed front-to-back).
// ============================================================================
#pragma once
#include <ctime>
#include <fstream>
#include <string>
#include "../simulator/CPU.h"
#include "FileManager.h"

class ReportManager {
public:
    static std::string now() {
        std::time_t t = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        return buf;
    }

    static bool saveReport(const CPU& cpu, const std::string& dir, std::string& pathOut, std::string& err) {
        if (cpu.state() == CpuState::EMPTY) { err = "No program loaded - nothing to report."; return false; }
        if (!FileManager::ensureDirectory(dir, err)) return false;
        pathOut = FileManager::join(dir, FileManager::stem(cpu.programName()) + "_report.txt");
        std::ofstream out(pathOut, std::ios::trunc);
        if (!out.is_open()) { err = "Cannot open '" + pathOut + "' for writing."; return false; }
        const std::string line(40, '-'), dline(40, '=');
        const RegisterFile& r = cpu.registers();
        out << dline << "\n        CPU EXECUTION REPORT\n" << dline << "\n\n";
        out << "Program:\n" << cpu.programName() << "\n\n";
        out << "Execution Date/Time:\n" << now() << "\n\n";
        out << "Instructions:\n" << cpu.program().size() << "\n\n";
        out << "Execution Steps:\n" << cpu.steps() << "\n\n";
        out << line << "\nFINAL REGISTERS\n" << line << "\n\n" << r.table() << "\n";
        out << line << "\nFLAGS\n" << line << "\n\n" << r.flagTable() << "\n";
        out << line << "\nFINAL MEMORY (non-zero cells)\n" << line << "\n\n";
        cpu.memory().displayNonZero(out);
        out << "\n" << line << "\nSTACK (" << cpu.stack().size() << " items, top first)\n" << line << "\n\n";
        for (std::size_t d = 0; d < cpu.stack().size(); ++d) out << "  [" << cpu.stack().fromTop(d) << "]\n";
        if (cpu.stack().isEmpty()) out << "  (empty)\n";
        out << "\n" << line << "\nEXECUTION\n" << line << "\n\n";
        cpu.history().forEach([&](const HistoryEntry& h) {
            out << "Step " << h.step << " (PC=" << h.pc << "):\n" << h.instruction << "\n    -> " << h.result << "\n\n";
        });
        out << line << "\nSTATUS\n" << line << "\n\n";
        if (cpu.state() == CpuState::ERROR) out << "Execution stopped with an error.\nErrors:\n  " << cpu.lastError() << "\n";
        else if (cpu.state() == CpuState::HALTED) out << cpu.statusMessage() << "\nErrors: none\n";
        else out << "Program not finished (state: " << cpuStateName(cpu.state()) << ").\nErrors: none\n";
        out << dline << "\n";
        if (!out) { err = "Write error on '" + pathOut + "'."; return false; }
        return true;
    }

    static bool saveHistory(const CPU& cpu, const std::string& dir, std::string& pathOut, std::string& err) {
        if (cpu.history().empty()) { err = "Execution history is empty."; return false; }
        if (!FileManager::ensureDirectory(dir, err)) return false;
        pathOut = FileManager::join(dir, FileManager::stem(cpu.programName()) + "_history.txt");
        std::ofstream out(pathOut, std::ios::trunc);
        if (!out.is_open()) { err = "Cannot open '" + pathOut + "' for writing."; return false; }
        out << "EXECUTION HISTORY  " << cpu.programName() << "  (" << now() << ")\n";
        cpu.history().forEach([&](const HistoryEntry& h) {
            out << "Step " << h.step << " | PC=" << h.pc << " | " << h.instruction << "\n  before: " << h.before
                << "\n  after : " << h.after << "\n  result: " << h.result << "\n";
        });
        return true;
    }
};
