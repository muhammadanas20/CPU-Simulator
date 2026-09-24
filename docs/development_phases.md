# Development Phases (1 → 19)

Each phase lists: files · concept · complexity · how it was tested · pitfalls.
Build commands for every phase: `g++ -std=c++17 -Wall -Wextra -o cpu_ds_tests tests/test_main.cpp && ./cpu_ds_tests`
(the test functions follow the phase order, so you can comment out later ones while rebuilding step by step).

| # | Phase | Files | Key concept & complexity | Tested by | Pitfalls found / to watch |
|---|---|---|---|---|---|
| 1 | Skeleton | dirs, `CMakeLists.txt`, `Makefile`, `.gitignore`, `main.cpp` | header-only layout, layered dependencies | builds | run from the project root so the relative directories are found |
| 2 | Dynamic Array | `DynamicArray.h` | doubling → amortized O(1) push; O(n) insert/remove | `testDynamicArray` | shallow copy = double free → Rule of Five; `explicit` default constructor broke `Program{}` → added a separate default constructor |
| 3 | Linked List | `LinkedList.h` | head+tail, O(1) ends, O(n) middle | `testLinkedList` | forgetting to update `tail_` when deleting the last node (tested: push after tail delete) |
| 4 | Stack | `Stack.h` | array LIFO, O(1), overflow/underflow exceptions | `testStack` | `pop` must check emptiness *before* decrementing an unsigned counter |
| 5 | Queue | `Queue.h` | linked FIFO, O(1) | `testQueue` | set `rear_ = nullptr` when the last node leaves |
| 6 | Hash Table | `HashTable.h`, `SymbolTable.h` | djb2, chaining, rehash at α > 1 | `testHashTable` (forced collisions) | re-insert during rehash must not count as new collisions; deleting the head of a chain |
| 7 | Searching | `Searching.h` | O(n) vs O(log n); sorted precondition | `testSearchSort` | `mid = low + (high-low)/2`; use signed indices so `high = -1` works |
| 8 | Sorting | `Sorting.h` | bubble (early exit), selection, insertion; counts | `testSearchSort` (+ stability, best case) | `pass + 1 < n` guards `size_t` underflow when n = 0 |
| 9 | File Manager | `FileManager.h` | ifstream / ofstream trunc / fstream app / binary copy | `testFiles` | CRLF lines; distinguish missing vs unopenable |
| 10 | Program Manager | `ProgramRepository.h`, `ProgramManager.h`, `Program.h` | list repository, CSV persistence, corruption tolerance | `testProgramManagerUndoRedo` | commas in names; duplicate rows; files deleted outside the app → `MISSING` |
| 11 | Instruction repr. | `Instruction.h`, `Assembler.h` | two-pass assembly with a hash symbol table | `testAssembler` | **real bug found:** a `:` inside a comment (`; expected: R1 = 30`) was treated as a label → now only the comment-stripped text is checked |
| 12 | CPU execution | `Registers.h`, `Memory.h`, `InstructionExecutor.h`, `CPU.h` | fetch = dequeue; flush on jump; 64-bit overflow checks | `testCPU`, `testQueueDuringExecution` | signed overflow is UB in C++ → the arithmetic is done in `uint32_t`/`long long` |
| 13 | Execution history | `CPU.h` (`HistoryEntry`) | linked list append O(1) | `testCPU` (history size/content) | record the error text in the node when a step fails |
| 14 | Undo/Redo | `ProgramManager.h` | two stacks of snapshots, capacity 50 | `testProgramManagerUndoRedo` | clear redo on a new edit; drop the oldest when full |
| 15 | Reports | `ReportManager.h` | ofstream + history traversal | `testReportsSnapshots` | reporting with no program loaded → error, not an empty file |
| 16 | Snapshots | `SnapshotManager.h` | sparse text format, atomic load | `testReportsSnapshots` | truncated files → `END` marker; PC beyond the program |
| 17 | Integration | `Menu.h`, `ConsoleUI.h`, `main.cpp` | menus call managers; visualisation | scripted sessions (piped stdin) | end-of-input would loop forever → `readInt` returns the "back" value on EOF |
| 18 | Testing | `tests/test_main.cpp` | 185 checks, no framework | `make test`, ASan/UBSan | tests must run from the project root (demo programs are read from `programs/`) |
| 19 | Documentation | `README.md`, `docs/*` | – | – | keep the docs in sync with the menu numbers |

## Representative expected outputs
**Phase 2:** capacity growth when pushing into `DynamicArray<int>(2)`: `2 → 4 → 8 → 16 …` (`resizeCount` goes up once per doubling).
**Phase 6:** `factorial.asm` symbol table → `Bucket 5 -> OUTER(2) -> INNER(6) -> NULL`, `Bucket 7 -> NEXT(9) -> NULL`, collisions = 1.
**Phase 8:** selection sort on 8 items always performs 28 comparisons. Bubble sort on sorted input performs 7 with 0 swaps.
**Phase 12:** `loop.asm` finishes in 17 steps with R1 = 0, ZF = 1. `factorial.asm` finishes in 62 steps with R2 = 120.
**Phase 18:** `185 checks passed, 0 failed.`

## Known limitations (honest list)
* The snapshot stores SP but not the CPU stack contents.
* Undo snapshots copy the whole program (fine for lab-sized programs).
* "Pause" is a breakpoint or step limit, because a console app cannot be interrupted mid-run without threads.
* R0 is not hard-wired to zero. It is 0 only by convention.
