# Future Extension — TOA and COAL Integration Plan

```
                 FUTURE SYSTEM
                       │
               High-Level Language
                       ↓
                    Lexer              (TOA: regular expressions → NFA → DFA)
                       ↓
                    Parser             (TOA: context-free grammar)
                       ↓
                     AST
                       ↓
                   Compiler            (code generation)
                       ↓
                   Assembly  ── the .asm format defined in instruction_set.md
                       ↓
          ┌────────────┴────────────┐
          ↓                         ↓
       DS Layer (THIS PROJECT)   COAL Layer
          ↓                         ↓
 Program Management          CPU Architecture
 Data Structures              ALU
 File Handling                Registers
                              Memory
                              Flags
                              Control Unit
```

## What is deliberately NOT implemented now
Lexer, regex tokenizer, NFA, DFA, formal grammar, parser, AST, compiler. `Assembler.h` uses plain string splitting on purpose.

## Integration points already in place
| Future component | Plugs into | How |
|---|---|---|
| Compiler back-end | `programs/*.asm` + `ProgramRepository::refresh` | the compiler writes `.asm`; the repository indexes it automatically on the next scan |
| Real lexer/parser for assembly | replace `Assembler::tokenize/decode` | keep the output types `DynamicArray<Instruction>` + `SymbolTable` |
| COAL ALU | `InstructionExecutor` | replace the switch with ALU/control-unit classes; `ExecOutcome` stays the interface |
| COAL registers / memory | `RegisterFile`, `Memory` | extend (e.g. 16-bit, byte addressing, segment registers) |
| Pipeline studies | `CPU::iqueue_` | the prefetch queue already flushes on branches |
| Debugger / tracing | `LinkedList<HistoryEntry>` | history already records before/after state per step |

## Suggested DS-level enhancements
* Store the stack contents in snapshots (`STACK v` records).
* CALL/RET using the CPU stack for return addresses.
* A BST/AVL index of programs for O(log n) search without re-sorting.
* Merge sort / quick sort to compare against the O(n²) sorts.
* Command-pattern undo (store only the difference) to cut O(n) memory per snapshot.
* A circular-array queue as an alternative implementation, with a benchmark.
