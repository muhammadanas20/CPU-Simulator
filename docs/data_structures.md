# Data Structures

Every structure lives in `src/data_structures/` and is written from scratch.

---
## 1. Dynamic Array — `DynamicArray<T>`
**Used for:** program source lines (`Program.lines`), the instruction array (`CPU::program_`), simulated memory (`Memory::cells_`) and temporary arrays for sorting and searching.

**Layout**
```
data_ ─► [ MOV R1,10 ][ MOV R2,20 ][ ADD R1,R2 ][ HALT ][ unused ]...
           0            1            2            3        size_=4  capacity_=8
```
**Dynamic resizing:** when `size_ == capacity_`, `resize(2*capacity_)` allocates a new block, moves every
element and frees the old one. One resize costs O(n), but it happens after n, then 2n, then 4n pushes, so
n pushes copy at most 1+2+4+…+n < 2n elements in total, which is **O(1) amortized** per push.
`removeAt` halves the capacity when only ¼ is used. It waits for ¼ rather than ½ so that a push/pop pair
at the boundary cannot trigger repeated resizes ("thrashing").

| Operation | Time | Notes |
|---|---|---|
| get / update / operator[] | O(1) | address = base + i·sizeof(T); bounds checked |
| pushBack | O(1) amortized | O(n) when resizing |
| insert(i) / removeAt(i) | O(n) | shifts the tail |
| search | O(n) | linear |
| display | O(capacity) | |
Space: O(capacity), at most about 4n after shrinking rules.

**Example (sandbox, menu 3 → 8):** start at capacity 2, then push 5, 7, 9. The display prints `RESIZE: capacity 2 -> 4`.

---
## 2. Singly Linked List — `LinkedList<T>`
**Used for:** the **Program Repository** (`LinkedList<ProgramRecord>`) and the **Execution History** (`LinkedList<HistoryEntry>`).
```
HEAD ─► [#1 addition.asm] ─► [#2 factorial.asm] ─► [#3 loop.asm] ─► NULL
                                                    ▲ TAIL
```
*ProgramRecord fields:* id, name, path, instructionCount, executionCount, fileSize, status, lastExecution (+ `next` in the node).
*HistoryEntry fields:* step, pc, instruction, before (registers + flags), after, result.

**Why a list?** Records are added one at a time, and deleting from the middle only relinks one pointer
(no shifting). We almost always walk from front to back (display, save CSV, write the report). The tail
pointer makes appending history O(1).

| Operation | Time |
|---|---|
| pushFront / pushBack | O(1) |
| insertAt(i) / removeAt(i) / get(i) | O(n) |
| find / removeIf (search) | O(n) |
| sort (via array) | O(n²) for the chosen algorithm + O(n) copy |
Space: O(n) plus one pointer per node.

**Deletion cases:** head (move head), middle (prev->next = victim->next), tail (also update `tail_`).
All three are tested.

---
## 3. Stack — `Stack<T>` (array-based, fixed capacity)
**Used for:**
1. **CPU stack:** `Stack<int>` with capacity 64. `PUSH R1` → `push(R1)`, `SP--`; `POP R3` → `R3 = pop()`, `SP++`.
2. **Undo stack** and **Redo stack:** `Stack<DynamicArray<string>>`, capacity 50 each.

```
TOP
 ↓
[20]    ← pushed last, popped first
[10]
BOTTOM
```
Popping an empty stack throws `StackUnderflowError`; pushing onto a full one throws `StackOverflowError`.
The CPU turns these into readable errors, e.g.
`Stack underflow: pop from empty stack at instruction 0 (line 1: POP R1)`.

### Undo / Redo
```
edit:   UNDO.push(current); REDO.clear(); apply edit
undo:   REDO.push(current); current = UNDO.pop()
redo:   UNDO.push(current); current = REDO.pop()
```
Trace (lines shown as lists):
| action | current | UNDO (top right) | REDO |
|---|---|---|---|
| start | [] | – | – |
| add "MOV R1,1" | [MOV] | [ [] ] | – |
| add "HALT" | [MOV,HALT] | [ [], [MOV] ] | – |
| undo | [MOV] | [ [] ] | [ [MOV,HALT] ] |
| redo | [MOV,HALT] | [ [], [MOV] ] | – |
| undo, then modify | [MOV R1,5] | [ [], [MOV] ] | – *(cleared)* |

When UNDO is full (50), the **oldest** snapshot is dropped by moving the stack through a temporary stack.
That is O(50) and rare. Each snapshot is a full copy, so memory is O(k·n) for k snapshots of n lines.
This is simple and always correct (the "memento" approach). Storing only the change (command objects) is a possible improvement.

All stack operations are O(1).

---
## 4. Queue — `Queue<T>` (linked, FIFO)
**Used for:** the **Instruction Queue** of the CPU.
```
FRONT
 ↓
[ 0: MOV R1, 10 ]
[ 1: MOV R2, 20 ]
[ 2: ADD R1, R2 ]
[ 3: HALT       ]
 ↑
REAR
```
**How the CPU really uses it** (`CPU::step`):
1. `load` enqueues the whole instruction array.
2. Each step **dequeues** the front. That dequeue is the FETCH.
3. If the instruction is a taken jump (`JMP`, `JZ`, `JNZ`), the queue is **flushed** and **refilled** starting at the target. The remaining queued instructions were the wrong ones, as in a real processor's prefetch queue.
4. Safety check: if the front's address ≠ PC (e.g. after loading a snapshot), the queue is refilled from PC.

Dequeuing an empty queue throws `QueueUnderflowError`. A linked queue never becomes full, and both ends are O(1).
Statistics show the enqueue/dequeue count and the number of flushes.

---
## 5. Hash Table — `HashTable<V>` (separate chaining)
**Used for:** the **Symbol Table** (label → instruction address).

Hash function (djb2): `h = 5381; for each char c: h = h*33 + c; bucket = h % bucketCount`.
Multiplying by 33 spreads nearby strings ("L1", "L2") into different buckets.

Real output for `factorial.asm` (CPU menu → 12):
```
HashTable  entries=3  buckets=11  load=0.272727  collisions=1
  Bucket 0  -> NULL
  ...
  Bucket 5  -> OUTER(2) -> INNER(6) -> NULL     ← collision: both hash to 5, chained
  Bucket 6  -> NULL
  Bucket 7  -> NEXT(9) -> NULL
  ...
```
* **insert:** hash, walk the chain; if the key exists, return false (the assembler reports *duplicate label*). Otherwise append. If the bucket was non-empty, count a collision.
* **search:** hash, then walk one chain.
* **delete:** relink `prev->next` (or the bucket head).
* **rehash:** when `size > bucketCount` (load factor α > 1), allocate `2m+1` buckets and re-insert everything. That is O(n) but rare, so insert stays amortized O(1).

| Operation | Average | Worst |
|---|---|---|
| insert / search / delete | O(1 + α) | O(n) – all keys in one bucket |
Space: O(m + n).

**Why separate chaining** instead of open addressing? Deletion is trivial (no "tombstones"), the table never
fills up, and it reuses the linked-list idea: two structures working together.

---
## 6. Structures that combine
* The hash table is an **array of linked lists**.
* The repository is a **linked list** that is sorted through a **dynamic array**.
* The undo stack is a **stack of dynamic arrays**.
* The CPU pipeline goes **array → queue → executor → linked list**.
