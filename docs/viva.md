# Viva Preparation

## A. Data Structures (30)
1. **What is a dynamic array, and how does it differ from a static array?** A static array's size is fixed at compile time. A dynamic array keeps a heap block and replaces it with a larger one when it is full (`DynamicArray::resize`).
2. **Why double the capacity instead of adding a constant?** Doubling makes n pushes cost O(n) in total (amortized O(1)). Adding a constant c costs O(n²/c).
3. **What is amortized complexity?** The average cost per operation over a worst-case sequence. An occasional O(n) resize spread over n cheap pushes gives O(1) each.
4. **Why shrink at ¼ and not ½?** At ½, alternating push/pop at the boundary would resize every time (thrashing).
5. **Complexity of inserting in the middle of an array?** O(n), because the elements after it must shift right.
6. **Why does the project store instructions in an array and not a list?** PC is an index, and jumps need O(1) access to `program_[target]`.
7. **Singly vs doubly linked list?** A doubly linked list has a `prev` pointer as well, which allows backward traversal and O(1) delete given a node, at the cost of more memory. A singly list with a tail pointer is enough for our operations.
8. **Why keep a tail pointer?** Appending (history, repository) becomes O(1) instead of O(n).
9. **How do you delete the tail node of a singly list?** Find its predecessor (O(n)), set `prev->next = nullptr`, then `tail_ = prev`. `removeAt` and `removeIf` handle this case.
10. **Why is the repository a linked list?** Records come and go one at a time, deleting in the middle needs no shifting, and it is traversed sequentially for display and saving.
11. **How is the linked list sorted?** It is copied into a DynamicArray, sorted with the chosen algorithm, and the list is rebuilt. That costs O(n) extra time and space, next to the O(n²) sort.
12. **What is a stack? Give two uses in this project.** A LIFO structure. Uses: the CPU PUSH/POP stack and the undo/redo stacks.
13. **Stack overflow vs underflow?** Overflow is pushing onto a full stack; underflow is popping or peeking an empty one. Both throw custom exceptions, which the CPU turns into error messages.
14. **Why is the stack array-based with a fixed capacity?** It models a limited stack region, all operations are O(1), and it makes overflow demonstrable.
15. **Explain undo/redo with two stacks.** Before an edit, push the current state onto UNDO and clear REDO. Undo: push current onto REDO and pop UNDO. Redo is the mirror image.
16. **Why must redo be cleared after a new edit?** The redo states belong to a branch of history that no longer exists.
17. **What happens when the undo stack is full?** The oldest snapshot is discarded (through a temporary stack), so the most recent 50 states remain.
18. **What is a queue? Where is it used?** A FIFO structure. It is the instruction queue that feeds the CPU.
19. **Why a linked queue instead of an array queue?** Both ends are O(1), it never becomes full, and no circular index arithmetic is needed.
20. **What happens to the queue on a jump?** It is flushed and refilled from the target address, like a real prefetch-queue flush.
21. **What is a circular queue?** An array queue whose front/rear indices wrap modulo the capacity, reusing freed slots. It is a possible alternative implementation.
22. **What is hashing?** Mapping a key to an array index with a hash function, to get average O(1) access.
23. **Which hash function do you use?** djb2: `h = h*33 + c`, starting from 5381, then `% bucketCount`.
24. **What is a collision?** Two keys mapping to the same bucket. For example, OUTER and INNER both go to bucket 5 in factorial.asm.
25. **Separate chaining vs open addressing?** Chaining keeps a linked list per bucket. Open addressing probes other slots. Chaining never fills up and deletion is simple.
26. **What is the load factor, and what do you do about it?** α = n/m. When α > 1 we rehash to 2m+1 buckets.
27. **Worst case of hash search, and when does it happen?** O(n), when all keys fall into one bucket.
28. **Why is the symbol table a hash table rather than a sorted array?** Lookups are O(1) on average with no sorting needed, and labels are inserted in file order.
29. **What is the difference between an ADT and a data structure?** An ADT is the interface (for a stack: push, pop, peek). A data structure is a concrete implementation (an array-based stack).
30. **What is the Rule of Three/Five, and why did you implement it?** Classes that own heap memory need a copy constructor, assignment and destructor (plus move operations). Otherwise two objects share one block and it gets double-freed. Undo snapshots copy arrays, so deep copies are essential.

## B. File Handling (20)
1. **ifstream vs ofstream vs fstream?** Input only, output only, and both/any mode. We use all three (see file_handling.md).
2. **What does `std::ios::trunc` do?** It empties the file when opening. That is how "overwrite" works.
3. **What does `std::ios::app` do?** Every write goes to the end of the file. It is used by `appendLine`.
4. **How do you detect that a file failed to open?** `if (!stream.is_open())` returns an error message.
5. **Why check `exists` before reading?** To tell "does not exist" apart from "cannot be opened", which gives better messages.
6. **How do you read a file line by line?** `while (std::getline(in, line))`.
7. **Why strip `'\r'`?** Files created on Windows end lines with `\r\n`, and the extra `\r` would break parsing.
8. **Text vs binary mode?** Binary does no newline translation. `copyFile` uses binary for an exact copy.
9. **What is the format of the program index?** A CSV with a header: id, name, path, instructions, executions, size, status, last_execution.
10. **What happens if the index is missing?** A new file with only the header is created, then `programs/` is scanned.
11. **What happens if a CSV row is corrupted?** It is skipped with a warning that shows the line number, and the file is rewritten cleanly.
12. **Why replace commas in names?** A comma would add a column and corrupt the CSV.
13. **What does a snapshot contain?** Registers, PC, SP, flags and the non-zero memory cells as `MEM address value`, ending with `END`.
14. **Why only non-zero cells?** A sparse format: smaller files, and the load starts from zeroed memory anyway.
15. **How is a partially corrupted snapshot handled?** It is parsed into temporaries and committed only if every line is valid, so the load is atomic.
16. **Why the `END` marker?** It detects truncated files (e.g. after a crash while writing).
17. **How are snapshot file names chosen?** `nextPath` tries snapshot_001, 002, … until it finds a free one, so nothing is overwritten.
18. **Is closing streams necessary?** The destructor closes them (RAII). We `flush` and check the stream state to detect write errors.
19. **Why is std::filesystem used, and is that allowed?** For directory listing, size, create and delete. These are OS services, not DS concepts, and every use is documented.
20. **How does Save As protect existing files?** It checks for existence and asks for confirmation. Without confirmation, `saveAs` returns an error.

## C. Project-specific (20)
1. **Describe the data flow from a .asm file to a report.** File → readLines (DynamicArray of lines) → Assembler (instruction array + hash symbol table) → Queue → CPU step (dequeue, execute) → history list → ReportManager.
2. **Why two passes in the assembler?** Forward references: a jump may use a label that is defined later.
3. **Is the assembler a parser/lexer?** No. It only splits on spaces and commas. Lexers and parsers are future TOA work.
4. **How does JNZ work?** If ZF = 0, PC ← label address and the queue is refilled. Otherwise PC++.
5. **How are flags computed for ADD?** ZF = result is 0, SF = result < 0, CF = unsigned carry, OF = the 64-bit result differs from the 32-bit result.
6. **What does CMP do?** It subtracts to set the flags, but stores nothing.
7. **What is SP if the values are in a Stack object?** A simulated pointer (1000, −1 per push) that mimics real hardware. The actual storage is the custom stack.
8. **How is an infinite loop handled?** Run stops after the step limit (10,000, configurable) and pauses. You can continue running.
9. **How does "Pause" work in a console program?** As a breakpoint: Run stops before executing the chosen address. Step is single-stepping.
10. **What does Reset clear?** Registers, flags, memory, stack, history, step count, and it refills the queue from address 0.
11. **What happens if a program has no HALT?** When PC reaches the end, the CPU halts with "Reached end of program without HALT."
12. **How are undefined labels reported?** `Line 4 (instruction 2): undefined label 'NOWHERE'.`
13. **Where are execution counts updated?** In `ConsoleUI::afterExecution` → `repo.recordExecution` → CSV. This happens once per run that reaches HALT or ERROR.
14. **How is binary search made valid on the repository?** The records are copied into an array and insertion-sorted by name first. Searching the unsorted list would be wrong.
15. **Why insertion sort before binary search?** It is simple, stable, O(n) on nearly-sorted data, and fine for small n.
16. **How would you add a MUL instruction?** Add it to the `Opcode` enum and `opcodeName`, add an operand rule in `Assembler::decode`, add a case in `InstructionExecutor`, and add a test.
17. **How do you know the code works?** 185 automated checks (every structure, algorithm, instruction and file case), every demo program verified, and sanitizer-clean runs.
18. **What input errors can the menus survive?** Letters, blanks, out-of-range numbers and end-of-input. `Menu::readInt` validates everything and loops.
19. **Why is everything header-only?** Templates must be in headers anyway. It also gives a one-command build.
20. **How does this connect to the COAL project?** The COAL layer would replace `InstructionExecutor`/`RegisterFile` with a detailed ALU and control unit. This layer keeps program management, the structures and the files.
