# Architecture

## 1. Layers
```
┌──────────────────────────── ui ─────────────────────────────┐
│ Menu.h (safe input)   ConsoleUI.h (all menus, visualisation) │
└──────────────┬─────────────────────┬───────────────────┬─────┘
               │                     │                   │
┌──────── program ────────┐ ┌──── simulator ─────┐ ┌──── filesystem ─────┐
│ ProgramManager (undo)   │ │ CPU                 │ │ FileManager         │
│ ProgramRepository       │ │ InstructionExecutor │ │ ReportManager       │
│ Assembler, Instruction  │ │ RegisterFile Memory │ │ SnapshotManager     │
│ Program                 │ │ SymbolTable         │ │                     │
└──────────────┬──────────┘ └─────────┬───────────┘ └──────────┬──────────┘
               └──────────── algorithms (Searching, Sorting) ───┘
               └──────────── data_structures (Array, List, Stack, Queue, Hash) ┘
```
Dependencies only point **downwards**. The data structures know nothing about CPUs, and the CPU knows
nothing about menus. That is why the tests can use every module without the UI.

All classes are **header-only** (`.h` with inline member functions). This keeps the build to one command
(`g++ src/main.cpp`), and it is required anyway for the templates (`DynamicArray<T>`, `Stack<T>`…).

## 2. Main data flow
```
programs/addition.asm ──FileManager::readLines (ifstream)──► DynamicArray<string>  (Program.lines, editable)
        │
        ▼ Assembler::assemble  (pass 1: labels → SymbolTable/HashTable; pass 2: decode)
DynamicArray<Instruction>  +  SymbolTable
        │
        ▼ CPU::load  – copies the array, enqueues every instruction
Queue<Instruction>  FRONT→[0][1][2][3]←REAR
        │
        ▼ CPU::step  – FETCH = dequeue; EXECUTE = InstructionExecutor; jump ⇒ flush+refill queue
RegisterFile, Memory(DynamicArray<int>), Stack<int>
        │
        ▼ every step appends
LinkedList<HistoryEntry>
        │
        ▼ ReportManager (ofstream)             SnapshotManager (ofstream / ifstream)
reports/addition_report.txt                    snapshots/snapshot_001.dat
```
Metadata: `ProgramRepository` (a LinkedList of `ProgramRecord`) ⇄ `data/program_index.csv`.

## 3. Class reference
The format for each class is: **Purpose · Data members · Methods · DS used · Why · Time · Space · Example**.

### DynamicArray<T> (`data_structures/DynamicArray.h`)
* **Purpose:** a growable array that replaces `std::vector`.
* **Members:** `T* data_`, `size_`, `capacity_`, `resizeCount_`.
* **Methods:** `pushBack, insert, removeAt, update, get/operator[], search, resize (private), display, clear, assign, swapElements`.
* **Why:** instructions are addressed by index (PC = index), so O(1) random access is essential.
* **Time:** get/update O(1); pushBack amortized O(1); insert/remove O(n); search O(n). **Space:** O(capacity) ≤ 2n (plus shrink at ¼).
* **Example:** `DynamicArray<int> a(2); a.pushBack(1); a.pushBack(2); a.pushBack(3); // capacity 2→4`

### LinkedList<T> (`LinkedList.h`)
* **Purpose:** a singly linked list with head and tail pointers.
* **Members:** `head_, tail_, size_`; the node is `ListNode{data,next}`.
* **Methods:** `pushFront, pushBack, insertAt, removeAt, removeIf, find, indexOf, get, update, forEach, toArray, fromArray, clear`.
* **Why:** the repository and history grow one record at a time, and repository deletes happen in the middle with no shifting. History only ever appends and is read in order.
* **Time:** push front/back O(1); indexed ops, search and delete O(n). **Space:** O(n) plus one pointer per node.

### Stack<T> (`Stack.h`)
* **Purpose:** a fixed-capacity LIFO stack.
* **Members:** `T* data_, capacity_, count_, operations_`.
* **Methods:** `push, pop, peek, isEmpty, isFull, size, fromTop, clear, display`. Throws `StackOverflowError` / `StackUnderflowError`.
* **Why:** CPU PUSH/POP is LIFO by definition, and undo must return the *most recent* state first. A fixed capacity models a real, limited stack region, so overflow can be demonstrated.
* **Time:** all O(1). **Space:** O(capacity).

### Queue<T> (`Queue.h`)
* **Purpose:** a FIFO queue on linked nodes.
* **Members:** `front_, rear_, size_, operations_`.
* **Methods:** `enqueue, dequeue, peek, isEmpty, size, clear, forEach`. Throws `QueueUnderflowError`.
* **Why:** instructions are consumed in program order (first in, first executed), exactly like a CPU prefetch queue.
* **Time:** O(1) enqueue/dequeue. **Space:** O(n).

### HashTable<V> (`HashTable.h`) and SymbolTable (`simulator/SymbolTable.h`)
* **Purpose:** map a label to an instruction address.
* **Members:** `Node** buckets_, bucketCount_, size_, collisions_`.
* **Methods:** `hash (djb2 % m), insert (rejects duplicates), search, remove, contains, clear, display, rehash (private)`.
* **Why:** every jump needs its label's address. Average O(1) lookup beats scanning, and separate chaining reuses linked lists.
* **Time:** average O(1 + α), worst O(n). **Space:** O(m + n).
* `SymbolTable` is a thin wrapper that gives the operations domain names (`define`, `lookup`).

### Instruction / Operand / Opcode (`program/Instruction.h`)
The decoded form of one line: `address, sourceLine, op, a, b`. Decoding once at load time means execution
never re-reads text.

### Assembler (`program/Assembler.h`)
A two-pass converter from text lines to `DynamicArray<Instruction>` and `SymbolTable`, with line-numbered
error messages. It only splits strings on spaces and commas, so it is **not** a lexer or parser.
Time O(L·k) for L lines of k characters.

### Program (`program/Program.h`)
`name`, `path`, `DynamicArray<string> lines` and a `modified` flag: the program being edited.

### ProgramRepository (`program/ProgramRepository.h`)
* **Members:** `LinkedList<ProgramRecord> list_`, index path, programs dir, `nextId_`, `warnings`.
* **Methods:** `load, save, syncWithDirectory, refresh, recordExecution, remove, find, searchByName, binarySearchByName, sort`.
* **Sorting design:** list → DynamicArray → bubble/selection/insertion → rebuild list. The O(n) copy is small next to the O(n²) sort, and index-based sorting is the form taught in class.

### ProgramManager (`program/ProgramManager.h`)
* **Members:** `Program current_`, `Stack<DynamicArray<string>> undo_, redo_` (capacity 50), repository reference.
* **Methods:** `create, open, save, saveAs, remove, addLine, insertLine, modifyLine, deleteLine, undo, redo`.
* Every editing method calls `beforeEdit()`, which pushes a copy of the lines onto undo and clears redo.

### RegisterFile / Flags (`simulator/Registers.h`)
`R[8], PC, SP (starts at 1000), Flags{ZF,SF,CF,OF}`. These are simplified simulated registers, not x86 registers.

### Memory (`simulator/Memory.h`)
1024 `int` cells in a `DynamicArray<int>`; `read`/`write` throw on an invalid address.

### InstructionExecutor (`simulator/InstructionExecutor.h`)
One big `switch` on the opcode. It changes registers, memory and the stack, sets flags using 64-bit
arithmetic to detect overflow, and returns `OK / JUMPED / HALTED / ERROR` with a readable message.
Each instruction is O(1).

### CPU (`simulator/CPU.h`)
* **Members:** instruction array, symbol table, registers, memory, `Stack<int>` (capacity 64), `Queue<Instruction>`, `LinkedList<HistoryEntry>`, state, steps, breakpoint.
* **Methods:** `load, reset, step, run(stepLimit), setBreakpoint, syncAfterStateRestore` and accessors.
* **Queue invariant:** the front of the queue is always the instruction at PC. A taken jump flushes the queue and refills it from the target.
* **Run** stops at HALT, an error, the breakpoint (the "Pause" feature) or the step limit (10,000, guarding against infinite loops).

### FileManager, ReportManager, SnapshotManager (`filesystem/`)
See [file_handling.md](file_handling.md).

### Menu / ConsoleUI (`ui/`)
`Menu::readInt` validates every number. `ConsoleUI` holds one of each manager and draws the structures
(queue FRONT/REAR, stack TOP, list HEAD→NULL, hash buckets).

## 4. Where the STL is still used (and why that is acceptable)
| STL item | Where | Reason |
|---|---|---|
| `std::string` | everywhere | text handling is not a DS topic in this lab |
| `std::ifstream/ofstream/fstream` | filesystem | required file handling |
| `std::filesystem` | FileManager only | listing directories, file size, delete: OS services |
| `std::swap`, `std::move` | array/stack internals | language utilities |
| `std::ostringstream/istringstream` | formatting, snapshot parsing | text formatting |
| `std::exception` types | errors | standard error reporting |

**Not used:** `std::vector`, `std::list`, `std::stack`, `std::queue`, `std::map`, `std::unordered_map`, `std::sort`, `std::find`.
