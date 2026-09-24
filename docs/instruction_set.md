# Simplified Instruction Set

> An **educational** instruction set, *not* x86. The registers are **simplified simulated CPU registers**.

## Machine model
| Item | Description |
|---|---|
| R0–R7 | 8 general-purpose 32-bit signed registers, all 0 at reset (R0 is conventionally used as "zero" but is writable) |
| PC | index of the next instruction in the instruction array |
| SP | simulated stack pointer: starts at 1000, −1 per PUSH, +1 per POP (the values themselves live in the custom `Stack<int>`, capacity 64) |
| Memory | 1024 words, addresses 0–1023 |
| Flags | ZF zero, SF sign (result < 0), CF carry/borrow (unsigned), OF signed overflow |

## Syntax
```
; comment
LABEL:                ; label on its own line
LABEL: MOV R1, 5      ; label + instruction
```
Case-insensitive. Operands are separated by a comma and/or spaces. Numbers can be decimal (`-5`) or hex (`0x1F`).
Labels: letter or `_` first, then letters, digits or `_`. They must not be register or opcode names.

## Instructions (15)
| Instruction | Operands | Effect | Flags |
|---|---|---|---|
| `MOV Rd, Rs/imm` | reg, reg\|imm | Rd ← value | – |
| `LOAD Rd, addr` / `LOAD Rd, [Rs]` | reg, imm\|[reg] | Rd ← MEM[addr] | – |
| `STORE Rs, addr` / `STORE Rs, [Rd]` | reg, imm\|[reg] | MEM[addr] ← Rs | – |
| `ADD Rd, Rs/imm` | | Rd ← Rd + v | ZF SF CF OF |
| `SUB Rd, Rs/imm` | | Rd ← Rd − v | ZF SF CF OF |
| `INC Rd` | reg | Rd ← Rd + 1 | ZF SF OF (CF unchanged) |
| `DEC Rd` | reg | Rd ← Rd − 1 | ZF SF OF (CF unchanged) |
| `PUSH Rs/imm` | reg\|imm | stack.push(v); SP−− | – |
| `POP Rd` | reg | Rd ← stack.pop(); SP++ | – |
| `CMP Ra, Rb/imm` | | compute Ra − v, store nothing | ZF SF CF OF |
| `JMP L` | label | PC ← address(L) | – |
| `JZ L` | label | if ZF = 1: PC ← address(L) | – |
| `JNZ L` | label | if ZF = 0: PC ← address(L) | – |
| `NOP` | – | nothing | – |
| `HALT` | – | stop; PC stays on HALT | – |

`[Rs]` (register-indirect) exists so that programs can walk arrays in memory (see `array_sum.asm`).

## Flag rules
* `ZF = (result == 0)`, `SF = (result < 0)`
* ADD: `CF` = the unsigned 32-bit sum overflowed; `OF` = the signed result does not fit in 32 bits (computed with 64-bit arithmetic).
* SUB/CMP: `CF = (unsigned)a < (unsigned)b` (borrow); `OF` = signed overflow.

## Errors
**Load time** (Assembler, with line numbers): invalid instruction, invalid register (`R8`), invalid number (`12a`),
wrong operand count or kind, invalid/reserved/duplicate label, undefined label, jump past the last instruction, empty program.
**Run time** (CPU): stack underflow/overflow, invalid memory address, PC outside the program, and the step limit (infinite loop guard).
A program without HALT stops cleanly with "Reached end of program without HALT."

## Example
```asm
MOV R1, 5
LOOP:
DEC R1
CMP R1, R0
JNZ LOOP
HALT
```
Steps: 1 (MOV) + 5 × (DEC, CMP, JNZ) + 1 (HALT) = **17**. Final R1 = 0, ZF = 1.
