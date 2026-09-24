; stack_demo.asm - LIFO order. expected: R3 = 20 (last pushed, first popped)
MOV R1, 10
MOV R2, 20
PUSH R1
PUSH R2
POP R3
HALT
