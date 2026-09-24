; array_sum.asm - store 5 numbers at MEM[200..204] and sum them
; uses register-indirect LOAD R5, [R2]
; expected: R4 = 150 and MEM[300] = 150
MOV R1, 10
STORE R1, 200
MOV R1, 20
STORE R1, 201
MOV R1, 30
STORE R1, 202
MOV R1, 40
STORE R1, 203
MOV R1, 50
STORE R1, 204
MOV R2, 200     ; pointer
MOV R3, 5       ; count
MOV R4, 0       ; sum
SUM_LOOP:
LOAD R5, [R2]
ADD R4, R5
INC R2
DEC R3
JNZ SUM_LOOP
STORE R4, 300
HALT
