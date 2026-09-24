# Data Structure-Based CPU Program Management & Simulation System

A C++17 console application for a university **Data Structures Lab**. You can write small
assembly-like programs, store and organise them, and run them on a simplified simulated CPU.
Every major feature runs on a **hand-written data structure** (dynamic array, linked list,
stack, queue, hash table) or a **classic algorithm** (linear/binary search, bubble/selection/insertion sort).
All data is kept in files (`.asm` programs, a CSV index, reports and memory snapshots).

> This is the DS-Lab layer of a planned larger system. It has **no** lexer, parser, NFA/DFA, grammar or compiler.
> Those belong to a future Theory-of-Automata course (see [docs/future_extension.md](docs/future_extension.md)).

---

## 1. Problem statement
Students learn stacks, queues, lists and hash tables as separate, abstract exercises. This project puts
them inside one working system, so each structure has a **job**: the queue feeds instructions to the CPU,
the hash table resolves jump labels, the stacks run `PUSH/POP` and undo/redo, and the linked lists hold
the program repository and execution history.

## 2. Objectives
* Implement the required data structures from scratch, with no `std::vector` / `std::list` / `std::stack` / `std::queue` / `std::unordered_map`.
* Use each structure where it is the natural choice, and justify the choice.
* Implement and measure searching and sorting (comparisons and swaps are counted).
* Keep all state in files using `ifstream`, `ofstream` and `fstream`.
* Handle every normal user mistake with a specific message and never crash.

## 3. Features
| Area | What works |
|---|---|
| Programs | create, open, edit (add/insert/modify/delete lines), save, save-as (with overwrite confirmation), delete, list, details |
| Editor | undo / redo with two stacks (up to 50 states) and an assembly check |
| Repository | linked list of program records, kept in `data/program_index.csv`; auto-indexes new `.asm` files; skips corrupted rows safely |
| Search | linear search (name substring, opcode), binary search (exact name, after sorting), hash lookup (labels) |
| Sort | bubble / selection / insertion, by name / instruction count / execution count / file size, ascending or descending, with comparison and swap counts |
| CPU | load, run, step (shows before/after), pause (breakpoint), reset; 15 instructions; 8 registers + PC + SP; ZF/SF/CF/OF flags |
| Structures shown | instruction queue, CPU stack, undo/redo stacks, symbol-table buckets, repository list, history list, instruction array |
| Sandboxes | try array/stack/queue/hash operations on your own values (resize, overflow and collisions are visible) |
| Files | execution reports, history dumps, numbered memory snapshots (save/load/view/delete) |
| Stats | steps, queue/stack operations, hash collisions, search/sort comparisons, undo/redo depth |

## 4. Architecture (short)
```
 ui/ConsoleUI ──► program/ProgramManager ──► filesystem/FileManager ──► programs/*.asm
      │                 │   (undo/redo stacks)
      │                 └─► program/ProgramRepository (LinkedList) ──► data/program_index.csv
      │
      ├──► program/Assembler: lines ─► DynamicArray<Instruction> + SymbolTable(HashTable)
      │
      ├──► simulator/CPU: Queue<Instruction> ─► InstructionExecutor ─► RegisterFile / Memory / Stack<int>
      │                         └─► LinkedList<HistoryEntry>
      │
      └──► filesystem/ReportManager ─► reports/*.txt     filesystem/SnapshotManager ─► snapshots/*.dat
```
Details: [docs/architecture.md](docs/architecture.md)

## 5–9. Documentation index
| Document | Contents |
|---|---|
| [docs/architecture.md](docs/architecture.md) | components, how they connect, and a class-by-class explanation |
| [docs/data_structures.md](docs/data_structures.md) | each structure: purpose, members, methods, why it was chosen, complexity, example |
| [docs/algorithms.md](docs/algorithms.md) | searches, sorts, two-pass assembly, fetch-execute, undo/redo, with dry runs |
| [docs/file_handling.md](docs/file_handling.md) | every file format and stream used, plus error cases |
| [docs/instruction_set.md](docs/instruction_set.md) | the simplified instruction set, registers and flags |
| [docs/academic_mapping.md](docs/academic_mapping.md) | **where each DS concept is used**, the STL policy, and a complexity summary |
| [docs/dry_run.md](docs/dry_run.md) | `addition.asm` traced from file to report |
| [docs/viva.md](docs/viva.md) | 30 DS + 20 file-handling + 20 project questions, with answers |
| [docs/development_phases.md](docs/development_phases.md) | the 19 build phases: files, concept, complexity, tests, known pitfalls |
| [docs/future_extension.md](docs/future_extension.md) | how the future TOA and COAL layers plug in |

## 10–12. Build and run
Requirements: any C++17 compiler (g++ ≥ 9, clang ≥ 9, MSVC 2019+). No external libraries. Works fully offline.

```bash
# Option A - plain g++ (all classes are header-only, so there is one .cpp per program)
g++ -std=c++17 -Wall -Wextra -O2 -o cpu_ds src/main.cpp
g++ -std=c++17 -Wall -Wextra -O2 -o cpu_ds_tests tests/test_main.cpp

# Option B - make
make            # builds both
make test       # runs the tests

# Option C - CMake
cmake -S . -B build && cmake --build build
./build/cpu_ds_tests      # run from the project root

# Run (from the project root, so programs/ data/ reports/ snapshots/ are found)
./cpu_ds                  # or: ./cpu_ds /path/to/project
```
On g++ 8 add `-lstdc++fs` for `std::filesystem`.

## 13. Example programs (`programs/`)
| File | Shows | Expected result |
|---|---|---|
| addition.asm | MOV, ADD | R1 = 30 |
| subtraction.asm | SUB | R1 = 30 |
| stack_demo.asm | PUSH/POP (LIFO) | R3 = 20 |
| loop.asm | labels, CMP, JNZ | R1 = 0 after 17 steps |
| factorial.asm | nested loops, JZ, STORE | R2 = 120, MEM[100] = 120 |
| array_sum.asm | memory, `LOAD R5, [R2]` | R4 = 150, MEM[300] = 150 |
| demo.asm | every instruction type | R7 = 4 |
| errors_demo.asm | **deliberately broken**: shows 5 different error messages | fails to load |

Example output files made by actually running these: `reports/*_report.txt`, `reports/*_history.txt`,
`snapshots/snapshot_00N.dat` and `data/program_index.csv`.

## 14. Screenshots
```
================================================
       DATA STRUCTURE CPU PROGRAM MANAGER
================================================
1. Program Management     6. Reports
2. CPU Simulation         7. Memory Snapshots
3. Data Structures        8. Statistics
4. Search                 9. Settings
5. Sort                   0. Exit
```
```
  Step 3   Current PC: 2
  Instruction:
  ADD R1, R2
  Before:            After:
  R1 = 10            R1 = 30
  Flags: ZF=0 SF=0 CF=0 OF=0    Next PC: 3
```
*(Placeholder: add real terminal screenshots to `docs/screenshots/` for your submission.)*

## 15. Testing
`tests/test_main.cpp` runs **185 checks** without any external framework. It covers array insert/delete/resize/search,
list insert at beginning/middle/end, delete and search, stack overflow/underflow, queue underflow, hash
collisions and delete, all three sorts (including best case and stability), file create/read/append/overwrite
plus missing/empty files, every instruction, flags, runtime errors, queue flush on jumps, undo/redo, CSV
persistence and corruption, reports, snapshots, and every shipped demo program. It also runs cleanly under
`-fsanitize=address,undefined`.

## 16. Future enhancements
MUL/DIV/CALL/RET, visual GUI front-end, merge/quick sort, a BST or AVL index for programs, and the full
TOA pipeline (lexer → parser → AST → compiler emitting this `.asm` format). See [docs/future_extension.md](docs/future_extension.md).

## Project layout
```
src/data_structures/  DynamicArray.h LinkedList.h Stack.h Queue.h HashTable.h
src/algorithms/       Searching.h Sorting.h
src/program/          Instruction.h Assembler.h Program.h ProgramRepository.h ProgramManager.h
src/simulator/        Registers.h Memory.h SymbolTable.h InstructionExecutor.h CPU.h
src/filesystem/       FileManager.h ReportManager.h SnapshotManager.h
src/ui/               Menu.h ConsoleUI.h
src/main.cpp          entry point
tests/test_main.cpp   test suite
programs/ data/ reports/ snapshots/ docs/
```
Changes from the suggested layout: `Assembler.h` was **added** (it turns text into instructions and
fills the symbol table, and it is deliberately *not* a parser), and there is a `Makefile` next to `CMakeLists.txt`.
