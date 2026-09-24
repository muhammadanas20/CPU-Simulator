# Algorithms

Code: `src/algorithms/Searching.h`, `src/algorithms/Sorting.h`, plus the algorithms inside the Assembler, CPU and ProgramManager.

---
## 1. Linear Search
**Concept:** check the elements one by one until a match is found or the data runs out.
**Implementation:** `Searching::linearSearch(arr, pred)`, `LinkedList::find(pred, &comparisons)`, `ProgramRepository::searchByName`.
**Used for:** program name substring search, opcode search in the loaded program, and repository lookup by name.
**Why here:** the data is not sorted by these keys, and substring matching cannot use ordering anyway.

**Dry run:** find opcode `ADD` in `[MOV, MOV, ADD, HALT]`
| i | element | match? | comparisons |
|---|---|---|---|
| 0 | MOV | no | 1 |
| 1 | MOV | no | 2 |
| 2 | ADD | **yes** | 3 |
(The opcode search in the UI continues to the end so it can list *all* occurrences.)

Time: best O(1), average and worst O(n). Space: O(1).

---
## 2. Binary Search
**Concept:** compare with the middle element and throw away the half that cannot contain the target.
**Precondition: the data must be sorted by the search key.** On unsorted data, "target > middle" says nothing
about which half the target is in, so binary search could miss an element that is present.
For that reason `ProgramRepository::binarySearchByName` first **copies the repository into an array and
insertion-sorts it by name**, then searches. The UI shows the sorted array and both costs, so the trade-off is honest.
Binary search pays off when you search the same sorted data many times.

```cpp
while (low <= high) {
    mid = low + (high - low) / 2;         // no overflow
    if (key(mid) == target) return mid;
    if (key(mid) <  target) low = mid + 1;
    else                    high = mid - 1;
}
```
**Dry run:** find `factorial.asm` in the sorted names
`[0 addition, 1 array_sum, 2 demo, 3 errors_demo, 4 factorial, 5 loop, 6 stack_demo, 7 subtraction]`
| low | high | mid | key(mid) | action |
|---|---|---|---|---|
| 0 | 7 | 3 | errors_demo | "errors" < "factorial" → low = 4 |
| 4 | 7 | 5 | loop | "loop" > "factorial" → high = 4 |
| 4 | 4 | 4 | factorial | **found**, 3 comparisons |

Time O(log n), space O(1). With 8 items the worst case is ⌊log₂8⌋+1 = 4 comparisons.

---
## 3. Bubble Sort
**Concept:** repeatedly swap neighbouring pairs that are out of order. After pass *p*, the *p* largest items are in their final places. If a pass makes no swap, stop early.
**Dry run** on `[5, 2, 9, 1]`:
| pass | after pass | swaps |
|---|---|---|
| 1 | 2 5 1 **9** | (5,2) (9,1) |
| 2 | 2 1 **5 9** | (5,1) |
| 3 | 1 **2 5 9** | (2,1) |
Comparisons 3+2+1 = 6, swaps 4.
Best O(n) (already sorted: one pass, no swaps), average and worst O(n²), space O(1), **stable**.

## 4. Selection Sort
**Concept:** for each position i, find the minimum of `a[i..n-1]` and swap it into position i.
**Dry run** on `[5, 2, 9, 1]`:
| i | min found | array |
|---|---|---|
| 0 | 1 (idx 3) | **1** 2 9 5 |
| 1 | 2 (idx 1) | 1 **2** 9 5 (no swap) |
| 2 | 5 (idx 3) | 1 2 **5** 9 |
Always n(n−1)/2 = 6 comparisons and at most n−1 swaps (here 2).
O(n²) in every case, space O(1), **not stable** (a long-distance swap can reorder equal keys).
Its good point is the minimum number of swaps.

## 5. Insertion Sort
**Concept:** grow a sorted prefix. Take the next element (`key`) and shift larger elements right until key's place is found.
**Dry run** on `[5, 2, 9, 1]`:
| i | key | array after | shifts |
|---|---|---|---|
| 1 | 2 | 2 5 9 1 | 1 |
| 2 | 9 | 2 5 9 1 | 0 |
| 3 | 1 | 1 2 5 9 | 3 |
Best O(n) (sorted input: 1 comparison per element), worst O(n²), space O(1), **stable**. Very good on nearly-sorted data.
That is why it is used to prepare data for binary search and to sort file listings.

### Comparing them in the program
Menu **5. Sort** prints `comparisons` and `swaps`/`shifts` for each run, and **8. Statistics** keeps session totals.
For 8 programs sorted by name, selection sort always shows 28 comparisons. Bubble sort on already-sorted data shows 7.

---
## 6. Hashing (djb2 + chaining)
See [data_structures.md §5](data_structures.md). Dry run for `hash("LOOP")` with 11 buckets:
`h=5381 → h*33+'L'(76) → … → h % 11`. The program prints the bucket in **Search → Symbol table lookup**, so you can verify it live.

---
## 7. Two-Pass Assembly (`Assembler::assemble`)
* **Pass 1:** for each line, strip the `;comment`, and if there is `LABEL:`, `symbols.define(LABEL, address)`. `address` counts only lines that hold an instruction.
* **Pass 2:** decode opcode and operands. Jump operands are looked up in the symbol table.

Why two passes: in `JZ NEXT … NEXT: DEC R1`, the label is used *before* it is defined (a forward reference).
Time O(total characters). Every error keeps its line number.

## 8. Fetch–Execute Loop (`CPU::step`)
```
if PC == program size          → HALTED ("end of program without HALT")
if queue empty or front ≠ PC   → refill queue from PC
ins = queue.dequeue()          ← FETCH
before = registers
outcome = execute(ins)         ← EXECUTE (PC++ or PC = target)
history.pushBack(step, PC, ins, before, after, result)   ← RECORD
if JUMPED: flush + refill queue from new PC
```
Each step is O(1) apart from the refill, which is O(n) and happens only on taken jumps.
**Run** repeats `step` until HALT, an error, the breakpoint or the step limit.

## 9. Undo/Redo
See [data_structures.md §3](data_structures.md). Each operation is O(1) for the stack plus an O(n) copy of the lines.

## 10. Complexity summary
| Operation in the program | Structure / algorithm | Time |
|---|---|---|
| Add line in editor | DynamicArray pushBack + undo snapshot | O(n) (the snapshot copy) |
| Insert/delete line | DynamicArray insert/removeAt | O(n) |
| Look up label | HashTable search | O(1) avg |
| Execute one instruction | Queue dequeue + switch | O(1) |
| Taken jump | queue flush + refill | O(n) |
| Record step | LinkedList pushBack | O(1) |
| Repository add | LinkedList pushBack | O(1) (+ O(n) to save CSV) |
| Repository delete / find | LinkedList | O(n) |
| Sort repository | bubble / selection / insertion | O(n²) |
| Binary search by name | sort + binary search | O(n²) + O(log n) |
| Save report | history traversal + 1024 memory cells | O(steps + M) |
