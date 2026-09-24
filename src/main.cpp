// ============================================================================
// main.cpp  -  entry point.
// Usage:  ./cpu_ds [project_directory]
// The project directory must contain (or will get) programs/ data/ reports/
// snapshots/. Default: current directory.
// ============================================================================
#include <exception>
#include <iostream>
#include "ui/ConsoleUI.h"

int main(int argc, char* argv[]) {
    std::string base = argc > 1 ? argv[1] : ".";
    try {
        ConsoleUI ui(base);
        ui.run();
    } catch (const std::exception& e) {   // last line of defence - should never trigger
        std::cerr << "FATAL: unexpected error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
