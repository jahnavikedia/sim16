; power.asm -- Assignment 1, hand-translated from power.c
; Computes base^exponent = 8^2 = 64

.text
main:
        LDI   R0, 1          ; r = 1
        LDI   R1, 0          ; i = 0
        LD    R2, exponent   ; R2 = exponent
        LD    R3, base        ; R3 = base

loop:
        CMP   R1, R2         ; i - exponent
        JN    body           ; if (i < exponent), run body
        JMP   done           ; otherwise exit loop

body:
        MUL   R0, R3         ; r = r * base
        ADDI  R1, 1          ; i++
        JMP   loop

done:
        ST    R0, result     ; result = r
        OUT   R0             ; printf(result)
        HALT

.data
base:       .word 8
exponent:   .word 2
result:     .word 0