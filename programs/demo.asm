; demo.asm - touches every instruction type
START:  MOV R1, 7
        MOV R2, 3
        NOP
        SUB R1, R2        ; R1 = 4
        PUSH R1
        INC R2            ; R2 = 4
        CMP R1, R2        ; equal -> ZF = 1
        JZ EQUAL
        MOV R6, -1        ; skipped
EQUAL:  POP R3            ; R3 = 4
        STORE R3, 50
        LOAD R7, 50       ; R7 = 4
        JMP END
        MOV R6, 999       ; skipped
END:    HALT
