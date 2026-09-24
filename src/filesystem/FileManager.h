// ============================================================================
// FileManager.h  -  Phase 9
// Every raw file operation of the project goes through this class so the
// rest of the code never touches streams directly.
//
//   ifstream  -> readLines()           (read text line by line)
//   ofstream  -> writeLines()          (create / overwrite: std::ios::trunc)
//   fstream   -> appendLine(), copyFile() (std::ios::app / in|out modes)
//
// STL NOTE: std::filesystem is used ONLY for directory listing, file size,
// existence checks and deletion - OS services that are not a DS concept.
// ============================================================================
#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include "../data_structures/DynamicArray.h"

namespace fs = std::filesystem;

class FileManager {
public:
    static bool exists(const std::string& path) {
        std::error_code ec;
        return fs::exists(path, ec) && fs::is_regular_file(path, ec);
    }

    static bool ensureDirectory(const std::string& dir, std::string& err) {
        std::error_code ec;
        if (fs::exists(dir, ec)) return fs::is_directory(dir, ec) || (err = dir + " is not a directory", false);
        if (!fs::create_directories(dir, ec)) { err = "Cannot create directory '" + dir + "': " + ec.message(); return false; }
        return true;
    }

    // Read all lines (trailing '\r' stripped, so Windows files work too).
    static bool readLines(const std::string& path, DynamicArray<std::string>& out, std::string& err) {
        out.clear();
        if (!exists(path)) { err = "File '" + path + "' does not exist."; return false; }
        std::ifstream in(path);
        if (!in.is_open()) { err = "File '" + path + "' cannot be opened for reading."; return false; }
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            out.pushBack(line);
        }
        if (in.bad()) { err = "Read error while reading '" + path + "'."; return false; }
        return true;
    }

    // Create or overwrite (truncate) a file with the given lines.
    static bool writeLines(const std::string& path, const DynamicArray<std::string>& lines, std::string& err) {
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (!out.is_open()) { err = "File '" + path + "' cannot be opened for writing."; return false; }
        for (std::size_t i = 0; i < lines.size(); ++i) out << lines[i] << '\n';
        out.flush();
        if (!out) { err = "Write error on '" + path + "'."; return false; }
        return true;
    }

    static bool writeText(const std::string& path, const std::string& text, std::string& err) {
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (!out.is_open()) { err = "File '" + path + "' cannot be opened for writing."; return false; }
        out << text;
        if (!out) { err = "Write error on '" + path + "'."; return false; }
        return true;
    }

    // Append one line at the end (file is created if missing). Uses fstream.
    static bool appendLine(const std::string& path, const std::string& line, std::string& err) {
        std::fstream f(path, std::ios::out | std::ios::app);
        if (!f.is_open()) { err = "File '" + path + "' cannot be opened for appending."; return false; }
        f << line << '\n';
        return static_cast<bool>(f) || (err = "Append failed on '" + path + "'", false);
    }

    // Binary-safe copy using fstream in/out modes (used by "Save As").
    static bool copyFile(const std::string& from, const std::string& to, std::string& err) {
        std::fstream in(from, std::ios::in | std::ios::binary);
        if (!in.is_open()) { err = "Cannot open '" + from + "'."; return false; }
        std::fstream out(to, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!out.is_open()) { err = "Cannot create '" + to + "'."; return false; }
        out << in.rdbuf();
        return true;
    }

    static bool removeFile(const std::string& path, std::string& err) {
        std::error_code ec;
        if (!exists(path)) { err = "File '" + path + "' does not exist."; return false; }
        if (!fs::remove(path, ec)) { err = "Cannot delete '" + path + "': " + ec.message(); return false; }
        return true;
    }

    static long fileSize(const std::string& path) {
        std::error_code ec;
        auto s = fs::file_size(path, ec);
        return ec ? -1 : static_cast<long>(s);
    }

    // File names (not full paths) in `dir` ending with `ext`, e.g. ".asm".
    static DynamicArray<std::string> listFiles(const std::string& dir, const std::string& ext) {
        DynamicArray<std::string> names;
        std::error_code ec;
        if (!fs::is_directory(dir, ec)) return names;
        for (auto& entry : fs::directory_iterator(dir, ec))
            if (entry.is_regular_file() && entry.path().extension() == ext)
                names.pushBack(entry.path().filename().string());
        return names;
    }

    static std::string join(const std::string& dir, const std::string& name) {
        return (fs::path(dir) / name).generic_string();
    }
    static std::string stem(const std::string& path) { return fs::path(path).stem().string(); }
    static std::string fileName(const std::string& path) { return fs::path(path).filename().string(); }
};
