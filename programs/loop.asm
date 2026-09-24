; loop.asm - count R1 down from 5 to 0. R0 is 0 by default.
; expected: R1 = 0, DEC executed 5 times
MOV R1, 5

LOOP:
DEC R1
CMP R1, R0
JNZ LOOP

HALT
