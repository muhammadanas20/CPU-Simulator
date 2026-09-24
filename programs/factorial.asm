; factorial.asm - computes 5! = 120 using repeated addition (there is no MUL).
; R1 = n (counts down), R2 = result, R4 = inner counter, R5 = value to add
; expected: R2 = 120 and MEM[100] = 120
        MOV R1, 5
        MOV R2, 1
OUTER:  MOV R5, R2        ; result * n  ==  result added (n-1) more times
        MOV R4, R1
        DEC R4
        JZ NEXT
INNER:  ADD R2, R5
        DEC R4
        JNZ INNER
NEXT:   DEC R1
        CMP R1, 1
        JNZ OUTER
        STORE R2, 100
        HALT
