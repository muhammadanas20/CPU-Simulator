# Academic Mapping — DS Concepts → Project

## Concept map
| DS Concept | Project Application | File / Class | Menu to demonstrate |
|---|---|---|---|
| Array | Instruction storage (PC = index) | `CPU::program_` (`DynamicArray<Instruction>`) | CPU → 13 |
| Dynamic Array (resizing) | Growing program source while editing | `Program::lines`, `DynamicArray.h` | Data Structures → 1, 8 |
| Array as memory | 1024-word simulated memory | `Memory::cells_` | CPU → 7 |
| Linked List | Program repository | `ProgramRepository::list_` | Data Structures → 2 |
| Linked List | Execution history | `CPU::history_` | CPU → 10 |
| Stack | CPU PUSH / POP | `CPU::stack_` (`Stack<int>`, cap 64) | CPU → 8 |
| Stack (×2) | Undo / Redo | `ProgramManager::undo_, redo_` | Editor → 7/8, DS → 3 |
| Queue | Instruction processing (fetch = dequeue, flush on jump) | `CPU::iqueue_` | CPU → 9 |
| Hash Table (chaining) | Symbol table: label → address | `SymbolTable`, `HashTable.h` | CPU → 12, DS → 11 |
| Linear Search | Program name search, opcode search, find record | `Searching::linearSearch`, `LinkedList::find` | Search → 1, 3 |
| Binary Search | Exact program name, on name-sorted array | `Searching::binarySearch` | Search → 2 |
| Bubble / Selection / Insertion Sort | Organise repository by 4 keys | `Sorting.h`, `ProgramRepository::sort` | Sort |
| Two-pass algorithm | Forward label references | `Assembler::assemble` | load any program |
| File Handling | Program persistence (.asm) | `FileManager`, `ProgramManager` | Program Mgmt |
| File Handling | Metadata (CSV index) | `ProgramRepository::load/save` | automatic |
| File Handling | Reports | `ReportManager` | Reports, CPU → 11 |
| File Handling | Memory snapshots | `SnapshotManager` | Memory Snapshots |
| Exception handling | Underflow / overflow / bad index | `Stack`, `Queue`, `DynamicArray`, `Memory` | error demos |
| OOP / templates | Generic containers, manager classes | all | – |

## Required operations → implementation
| Structure | Required ops | Implemented as |
|---|---|---|
| Dynamic array | insert, remove, update, get, search, resize, display | `insert/pushBack, removeAt, update, get, search, resize, display` |
| Linked list | insert, delete, search, update, display, sort | `pushFront/pushBack/insertAt, removeAt/removeIf, find, update, forEach (UI draws it), ProgramRepository::sort` |
| History list | view, clear, save | CPU → 10, 15, 14 |
| Stack | push, pop, peek, isEmpty, isFull, display | same names |
| Queue | enqueue, dequeue, peek, isEmpty, display | same names + `forEach` for display |
| Hash table | hash, insert, search, delete, display, collisions | same names; `collisions()` counter; rehash |

## STL policy
Custom (required): **Dynamic array, Linked list, Stack, Queue, Hash table, all searches, all sorts.**
STL still used: `std::string`, streams, `std::filesystem` (directory services only), `std::swap/move`,
string streams and exception classes. See [architecture.md §4](architecture.md). No STL container or algorithm is used.

## Complexity analysis (summary)
| Structure | Access | Search | Insert | Delete | Space |
|---|---|---|---|---|---|
| DynamicArray | O(1) | O(n) | O(1)* end / O(n) middle | O(n) | O(n) |
| LinkedList (head+tail) | O(n) | O(n) | O(1) ends / O(n) middle | O(1) head / O(n) other | O(n) |
| Stack (array) | O(1) top | – | O(1) push | O(1) pop | O(cap) |
| Queue (linked) | O(1) front | – | O(1) enqueue | O(1) dequeue | O(n) |
| HashTable (chaining) | – | O(1) avg / O(n) worst | O(1)* avg | O(1) avg | O(m+n) |
\* amortized

| Algorithm | Best | Average | Worst | Space | Stable |
|---|---|---|---|---|---|
| Linear search | O(1) | O(n) | O(n) | O(1) | – |
| Binary search | O(1) | O(log n) | O(log n) | O(1) | – |
| Bubble sort (early exit) | O(n) | O(n²) | O(n²) | O(1) | yes |
| Selection sort | O(n²) | O(n²) | O(n²) | O(1) | no |
| Insertion sort | O(n) | O(n²) | O(n²) | O(1) | yes |
| Assembly (2 passes) | O(L) | O(L) | O(L) | O(L) | – |
| One CPU step | O(1) | O(1) | O(n) on refill | O(1) | – |

More detail: [algorithms.md §10](algorithms.md).
