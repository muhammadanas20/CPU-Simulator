# Execution Dry Run — `addition.asm`

```asm
; addition.asm - add two numbers
; expected: R1 = 30
MOV R1, 10
MOV R2, 20
ADD R1, R2
HALT
```

## 1. Program File → FileManager
Menu: `2 CPU Simulation → 1 Load Program → 2 by name → addition`.
`ConsoleUI::loadIntoCpu` looks up `addition.asm` in the repository linked list (linear search) to get the path,
then calls `FileManager::readLines("./programs/addition.asm", lines, err)`:
* `std::ifstream` opens the file (a missing file gives `File '...' does not exist.`)
* `std::getline` reads each line and strips any `\r`; each line is `pushBack`-ed into `DynamicArray<string>`

```
lines: [0]"; addition.asm - add two numbers" [1]"; expected: R1 = 30" [2]"MOV R1, 10" [3]"MOV R2, 20" [4]"ADD R1, R2" [5]"HALT"
size 6, capacity 8 (resized 4 → 8 once)
```

## 2. Instruction Array (Assembler)
**Pass 1** (labels): lines 1–2 are only comments (the `:` inside the comment is ignored because comments are stripped first).
No labels, so the symbol table stays empty. The instruction count is 4.
**Pass 2** (decode): `tokenize("MOV R1, 10") → {"MOV","R1","10"}`, `opcodeFromString("MOV")`, then `R1 → REGISTER 1` and `10 → IMMEDIATE 10`.

| address | line | op | a | b |
|---|---|---|---|---|
| 0 | 3 | MOV | REG 1 | IMM 10 |
| 1 | 4 | MOV | REG 2 | IMM 20 |
| 2 | 5 | ADD | REG 1 | REG 2 |
| 3 | 6 | HALT | – | – |

## 3. Queue
`CPU::load` copies the array, calls `reset()` (all registers 0, SP 1000, memory cleared, history cleared), and enqueues addresses 0..3:
```
FRONT → [0 MOV R1,10] [1 MOV R2,20] [2 ADD R1,R2] [3 HALT] ← REAR     state READY
```

## 4. CPU → Registers → Execution History (Run)
| step | PC before | FETCH (dequeue) | EXECUTE | registers after | queue after | history node |
|---|---|---|---|---|---|---|
| 1 | 0 | MOV R1, 10 | R1 ← 10, PC++ | R1=10 | [1][2][3] | Step1 PC0 "R1 = 10" |
| 2 | 1 | MOV R2, 20 | R2 ← 20, PC++ | R2=20 | [2][3] | Step2 PC1 "R2 = 20" |
| 3 | 2 | ADD R1, R2 | 10+20=30; ZF=0 SF=0 CF=0 OF=0; PC++ | R1=30 | [3] | Step3 PC2 "R1 = 10 + 20 = 30" |
| 4 | 3 | HALT | status HALTED, PC stays 3 | – | [] | Step4 PC3 "program halted" |

History list after the run:
```
HEAD → [Step1] → [Step2] → [Step3] → [Step4] → NULL
```
Each node also stores the compact before/after state, e.g. step 3 before:
`R0=0 R1=10 R2=20 ... PC=2 SP=1000 ZF=0 SF=0 CF=0 OF=0`.
The UI then calls `repo.recordExecution("addition.asm", now)`: executions goes 0 → 1, and `program_index.csv` is rewritten.

## 5. Report File
`CPU → 11 Save Report` → `ReportManager::saveReport` opens `reports/addition_report.txt` with `std::ofstream (trunc)` and writes
the header, registers (`R1 = 30, R2 = 20, PC = 3, SP = 1000`), flags (all 0), memory (all 0), stack (empty),
and then **traverses the history list** to print `Step 1 … Step 4`. It ends with the status
`Program completed successfully (HALT).` The real file is in `reports/addition_report.txt`.
