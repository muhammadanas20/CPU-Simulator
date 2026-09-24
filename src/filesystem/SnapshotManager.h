// ============================================================================
// SnapshotManager.h  -  Phase 16: save / load memory snapshots.
//
// File format (plain text, one record per line, '#' = comment):
//   # CPU-DS memory snapshot
//   PROGRAM addition.asm
//   REG R0 0          <- registers ARE included (R0..R7)
//   PC 3
//   SP 1000
//   FLAGS 0 0 0 0     <- ZF SF CF OF
//   MEM 100 30        <- address value   (only non-zero cells are written)
//   END
// Loading validates every line; on ANY error nothing is applied (the state
// is parsed into temporaries first, then committed).
// Files are auto-numbered snapshots/snapshot_001.dat, _002, ...
// ============================================================================
#pragma once
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "../simulator/CPU.h"
#include "FileManager.h"

class SnapshotManager {
public:
    static std::string nextPath(const std::string& dir) {
        for (int n = 1; n < 10000; ++n) {
            char buf[32];
            std::snprintf(buf, sizeof buf, "snapshot_%03d.dat", n);
            std::string p = FileManager::join(dir, buf);
            if (!FileManager::exists(p)) return p;
        }
        return FileManager::join(dir, "snapshot_overflow.dat");
    }

    static bool save(const CPU& cpu, const std::string& path, std::string& err) {
        std::ofstream out(path, std::ios::trunc);
        if (!out.is_open()) { err = "Cannot open '" + path + "' for writing."; return false; }
        const RegisterFile& r = cpu.registers();
        out << "# CPU-DS memory snapshot\n# format: MEM <address> <value> (non-zero cells only); registers included\n";
        out << "PROGRAM " << (cpu.programName().empty() ? "-" : cpu.programName()) << "\n";
        for (int i = 0; i < RegisterFile::COUNT; ++i) out << "REG R" << i << " " << r.R[i] << "\n";
        out << "PC " << r.PC << "\nSP " << r.SP << "\n";
        out << "FLAGS " << r.flags.ZF << " " << r.flags.SF << " " << r.flags.CF << " " << r.flags.OF << "\n";
        for (int a = 0; a < Memory::SIZE; ++a)
            if (int v = cpu.memory().read(a)) out << "MEM " << a << " " << v << "\n";
        out << "END\n";
        if (!out) { err = "Write error on '" + path + "'."; return false; }
        return true;
    }

    static bool load(CPU& cpu, const std::string& path, std::string& err) {
        std::ifstream in(path);
        if (!FileManager::exists(path)) { err = "Snapshot '" + path + "' does not exist."; return false; }
        if (!in.is_open()) { err = "Snapshot '" + path + "' cannot be opened."; return false; }
        RegisterFile regs;
        Memory mem;
        std::string line;
        int lineNo = 0;
        bool sawEnd = false;
        while (std::getline(in, line)) {
            ++lineNo;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ss(line);
            std::string tag;
            ss >> tag;
            auto bad = [&](const std::string& why) { err = "Corrupted snapshot line " + std::to_string(lineNo) + ": " + why + " -> \"" + line + "\""; return false; };
            if (tag == "PROGRAM") continue;
            if (tag == "END") { sawEnd = true; break; }
            if (tag == "REG") {
                std::string rname; long long v;
                if (!(ss >> rname >> v)) return bad("expected REG Rn value");
                int idx = Assembler::registerIndex(rname);
                if (idx < 0) return bad("invalid register");
                regs.R[idx] = static_cast<int>(v);
            } else if (tag == "PC") { if (!(ss >> regs.PC) || regs.PC < 0) return bad("invalid PC"); }
            else if (tag == "SP") { if (!(ss >> regs.SP)) return bad("invalid SP"); }
            else if (tag == "FLAGS") {
                int z, s, c, o;
                if (!(ss >> z >> s >> c >> o)) return bad("expected 4 flag bits");
                regs.flags = Flags{z != 0, s != 0, c != 0, o != 0};
            } else if (tag == "MEM") {
                int a, v;
                if (!(ss >> a >> v)) return bad("expected MEM address value");
                if (!Memory::valid(a)) return bad("invalid memory address");
                mem.write(a, v);
            } else return bad("unknown record '" + tag + "'");
        }
        if (!sawEnd) { err = "Corrupted snapshot: missing END marker (file truncated?)."; return false; }
        if (!cpu.program().empty() && regs.PC > static_cast<int>(cpu.program().size())) {
            err = "Snapshot PC " + std::to_string(regs.PC) + " is outside the loaded program."; return false;
        }
        cpu.registers() = regs;      // commit only after full validation
        cpu.memory() = mem;
        cpu.syncAfterStateRestore();
        return true;
    }
};
