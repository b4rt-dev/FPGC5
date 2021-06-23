; Bootloader. Is stored in ROM.
; Writes logo to on screen as a form of POST. Initial color is white, and will be changed to blue/green after the bootloader is finished to indicate success.
; Copies SPI to SDRAM, or copies UART bootloader from ROM to SDRAM, and finally jumps to SDRAM after setting the color of the logo and clearing all registers
; Because assembler assumes the program is executed from SDRAM, the addresses need to be updated after compilation!

Main:

    ; LOGO
    ; set palette table
    load32 0xC00400 r1      ; pallette address
    load 0xFF r2            ; white as primary color, others black
    write 0 r1 r2

    ; copy pattern table
    load32 0xC00000 r3      ; r3 = data dest
    addr2reg LOGOTABLE r2   ; r2 = data source

    add r3 253 r1           ; r1 = loop end (if r3 matches)

    CopyPatternLoop:
        copy 0 r2 r3        ; copy data to vram
        add r2 1 r2         ; increase source address
        add r3 1 r3         ; increase dest address

        beq r3 r1 2         ; keep looping until all 252 words are copied
        jump CopyPatternLoop


    ; copy window tile table
    load32 0xC015E4 r3      ; r3 = data dest: window tile address 0xC01420 + position offset
    addr2reg TILETABLE r2   ; r2 = data source

    load 0 r4               ; r4 = loop counter
    load 96 r1              ; r1 = loop end
    load 16 r5              ; r5 = next line counter
    load 32 r8              ; r8 = shift variable


    CopyTileLoop:
        sub r8 8 r8         ; remove 8 from shift variable
        read 0 r2 r10       ; read data
        shiftr r10 r8 r10   ; shift data to right
        write 0 r3 r10      ; write shifted data to vram
        bne r0 r8 3         ; if we shifted the last byte
            add r2 1 r2         ; increase source address
            load 32 r8          ; set shift variable back
        add r3 1 r3         ; increase dest address
        sub r5 1 r5         ; reduce line counter
        add r4 1 r4         ; increase counter

        bne r5 r0 3
            load 16 r5
            add r3 24 r3

        beq r4 r1 2         ; keep looping until all tiles are copied
        jump CopyTileLoop



    CopySPI:
    load32 0xC02741 r1      ; r1 = Boot mode address: 0xC02741
    // This part is deprecated, but does not hurt to leave in
    // It was used when GPI[0] was used to dertermine the boot mode
    read 0 r1 r2            ; r2 = GPIO values
    load 0b00000001 r3      ; r3 = bitmask for GPI[0]
    and r2 r3 r3            ; r3 = GPI[0]

    ; if Boot mode is high, then jump to UART bootloader copy function
    beq r0 r3 2
        jump CopyUartLoader

    load32 0x800000 r1      ; r1 = address 0 of SPI: 0x800000
    load 0 r2               ; r2 = address 0 of SDRAM: 0x00, and loop var
    read 5 r1 r3            ; r3 = last address to copy +1, which is in line 6 of SPI code

    CopyLoop:
        copy 0 r1 r2            ; copy SPI to SDRAM

        add r1 1 r1             ; incr SPI address 
        add r2 1 r2             ; incr SDRAM address

        beq r2 r3 2             ; copy is done when SDRAM address == number of lines to copy
            jump CopyLoop           ; copy is not done yet, copy next address
    
    EndBootloader:
    ; before clearing registers, we change the color of the logo to blue/green-ish to indicate success

    load32 0xC00400 r1      ; pallette address
    load 0b10010 r2         ; Blue/green as main color, others black
    write 0 r1 r2

    ; clear registers
    load 0 r1
    load 0 r2
    load 0 r3
    load 0 r4
    load 0 r5
    load 0 r6
    load 0 r7
    load 0 r8
    load 0 r9
    load 0 r10
    load 0 r11
    load 0 r12
    load 0 r13
    load 0 r14
    load 0 r15

    jump 0                  ; bootloader is done, jump to sdram


    CopyUartLoader:

        addr2reg UARTBOOTLOADERDATAPART1 r1 ; r1 = (src) address of first part of UART bootloader data in ROM
        load 0 r2                           ; r2 = (dst) address 0 of SDRAM: 0x00, and loop var
        load 7 r4                           ; r4 = number of words to copy at the start

        CopyStartLoop:
            copy 0 r1 r2            ; copy ROM to SDRAM

            add r1 1 r1             ; incr ROM address 
            add r2 1 r2             ; incr SDRAM address

            beq r2 r4 2             ; copy is done when SDRAM address == number of words to copy at the start
                jump CopyStartLoop  ; copy is not done yet, copy next address


        addr2reg UARTBOOTLOADERDATAPART2 r1 ; r1 = (src) address of second part of UART bootloader data in ROM
        load32 0x3FDE07 r2          ; r2 = (dst) address 4185607 of SDRAM: 0x3FDE07, and loop var
        load32 0x3FDE66 r3          ; r3 = r2 + number of words to copy = 0x3FDE07 + 95 = 0x3FDE66

        CopyEndLoop:
            copy 0 r1 r2            ; copy ROM to SDRAM

            add r1 1 r1             ; incr ROM address 
            add r2 1 r2             ; incr SDRAM address

            beq r2 r3 2             ; copy is done when SDRAM address == number of lines to copy
                jump CopyEndLoop    ; copy is not done yet, copy next address

        jump EndBootloader     ; copy is done



UARTBOOTLOADERDATAPART1:
.dw 0b10010000000000000000000000001100 ; Jump to constant address 6, first part of of UART bootloader data (to SDRAM 0, 7 words long)
.dw 0b10010000011111111011110001001000 ; Jump to constant address 4185636
.dw 0b10010000011111111011110001100010 ; Jump to constant address 4185649
.dw 0b10010000011111111011110001100100 ; Jump to constant address 4185650
.dw 0b10010000011111111011110011001010 ; Jump to constant address 4185701
.dw 0b00000000001111111101111001100110 ; Length of program
.dw 0b11111111111111111111111111111111 ; Halt

UARTBOOTLOADERDATAPART2:
.dw 0b01110010011100100011000000000001 ; Set r1 to 0x2723, second part of UART bootloader data (to SDRAM 4185607, 95 words long)
.dw 0b01110000000011000000000100000001 ; Set highest 16 bits of r1 to 0xC0
.dw 0b11100000000000000001000100100010 ; Read at address in r1 with offset -1 to r2
.dw 0b01110000000000000000000000000100 ; Set r4 to 0
.dw 0b01010000000000000010010011010000 ; If r4 != r13, then jump to offset 2
.dw 0b00001010100000011000001000000010 ; Compute r2 << 24 and write result to r2
.dw 0b01110000000000000001000000000100 ; Set r4 to 1
.dw 0b01010000000000000010010011010000 ; If r4 != r13, then jump to offset 2
.dw 0b00001010100000010000001000000010 ; Compute r2 << 16 and write result to r2
.dw 0b01110000000000000010000000000100 ; Set r4 to 2
.dw 0b01010000000000000010010011010000 ; If r4 != r13, then jump to offset 2
.dw 0b00001010100000001000001000000010 ; Compute r2 << 8 and write result to r2
.dw 0b01110000000000000011000000000100 ; Set r4 to 3
.dw 0b01010000000000000010010011010000 ; If r4 != r13, then jump to offset 2
.dw 0b00001010100000000000001000000010 ; Compute r2 << 0 and write result to r2
.dw 0b00001001100000000001110100001101 ; Compute r13 + 1 and write result to r13
.dw 0b00000001100000000000001011101110 ; Compute r2 + r14 and write result to r14
.dw 0b01110000000000000100000000000100 ; Set r4 to 4
.dw 0b01100000000000000010010011010000 ; If r4 == r13, then jump to offset 2
.dw 0b00010000000000000000000000000000 ; Return from interrupt
.dw 0b00001011000000011000111000000100 ; Compute r14 >> 24 and write result to r4
.dw 0b11010000000000000000000101000000 ; Write value in r4 to address in r1 with offset 0
.dw 0b00001011000000010000111000000100 ; Compute r14 >> 16 and write result to r4
.dw 0b11010000000000000000000101000000 ; Write value in r4 to address in r1 with offset 0
.dw 0b00001011000000001000111000000100 ; Compute r14 >> 8 and write result to r4
.dw 0b11010000000000000000000101000000 ; Write value in r4 to address in r1 with offset 0
.dw 0b00001011000000000000111000000100 ; Compute r14 >> 0 and write result to r4
.dw 0b11010000000000000000000101000000 ; Write value in r4 to address in r1 with offset 0
.dw 0b00010000000000000000000000000000 ; Return from interrupt
.dw 0b01110000000000000000000000000001 ; Set r1 to 0x0000
.dw 0b01110000000001000000000100000001 ; Set highest 16 bits of r1 to 0x40
.dw 0b01110000000000000000000000000010 ; Set r2 to 0
.dw 0b01110000000000000000000000000011 ; Set r3 to 0
.dw 0b00000000000000000000001000000101 ; Compute r2 OR r0 and write result to r5
.dw 0b00000000000000000000000100000110 ; Compute r1 OR r0 and write result to r6
.dw 0b11000000000000000000011001010000 ; Copy from address in r6 to address in r5 with offset 0
.dw 0b00001001100000000001010100000101 ; Compute r5 + 1 and write result to r5
.dw 0b00001001100000000001011000000110 ; Compute r6 + 1 and write result to r6
.dw 0b00001001100000000001001100000011 ; Compute r3 + 1 and write result to r3
.dw 0b01100000000000000010001111100000 ; If r3 == r14, then jump to offset 2
.dw 0b10010000011111111011110001010100 ; Jump to constant address 4185642
.dw 0b00010000000000000000000000000000 ; Return from interrupt
.dw 0b00010000000000000000000000000000 ; Return from interrupt
.dw 0b01110000000000000100000000000001 ; Set r1 to 4
.dw 0b01100000000000000010110100010000 ; If r13 == r1, then jump to offset 2
.dw 0b10010000011111111011110000001110 ; Jump to constant address 4185607
.dw 0b01110010011100100011000000000001 ; Set r1 to 0x2723 
.dw 0b01110000000011000000000100000001 ; Set highest 16 bits of r1 to 0xC0
.dw 0b11100000000000000001000100100010 ; Read at address in r1 with offset -1 to r2
.dw 0b01110000000000000000000000000011 ; Set r3 to 0
.dw 0b01010000000000000010001111000000 ; If r3 != r12, then jump to offset 2
.dw 0b00001010100000011000001000000010 ; Compute r2 << 24 and write result to r2
.dw 0b01110000000000000001000000000011 ; Set r3 to 1
.dw 0b01010000000000000010001111000000 ; If r3 != r12, then jump to offset 2
.dw 0b00001010100000010000001000000010 ; Compute r2 << 16 and write result to r2
.dw 0b01110000000000000010000000000011 ; Set r3 to 2
.dw 0b01010000000000000010001111000000 ; If r3 != r12, then jump to offset 2
.dw 0b00001010100000001000001000000010 ; Compute r2 << 8 and write result to r2
.dw 0b01110000000000000011000000000011 ; Set r3 to 3
.dw 0b01010000000000000010001111000000 ; If r3 != r12, then jump to offset 2
.dw 0b00001010100000000000001000000010 ; Compute r2 << 0 and write result to r2
.dw 0b00001001100000000001110000001100 ; Compute r12 + 1 and write result to r12
.dw 0b00000001100000000000001010101010 ; Compute r2 + r10 and write result to r10
.dw 0b01110000000000000100000000000011 ; Set r3 to 4
.dw 0b00110000000000000010110000110000 ; If r12 >= r3, then jump to offset 2
.dw 0b00010000000000000000000000000000 ; Return from interrupt
.dw 0b01110000000000000000000000000100 ; Set r4 to 0x0000
.dw 0b01110000000001000000000100000100 ; Set highest 16 bits of r4 to 0x40
.dw 0b00000001100000000000101101000100 ; Compute r11 + r4 and write result to r4
.dw 0b11010000000000000000010010100000 ; Write value in r10 to address in r4 with offset 0
.dw 0b00001001100000000001101100001011 ; Compute r11 + 1 and write result to r11
.dw 0b01110000000000000000000000000001 ; Set r1 to 0
.dw 0b01110000000000000000000000000010 ; Set r2 to 0
.dw 0b01110000000000000000000000000011 ; Set r3 to 0
.dw 0b01110000000000000000000000000100 ; Set r4 to 0
.dw 0b01110000000000000000000000000101 ; Set r5 to 0
.dw 0b01110000000000000000000000000110 ; Set r6 to 0
.dw 0b01110000000000000000000000000111 ; Set r7 to 0
.dw 0b01110000000000000000000000001000 ; Set r8 to 0
.dw 0b01110000000000000000000000001001 ; Set r9 to 0
.dw 0b01110000000000000000000000001010 ; Set r10 to 0
.dw 0b01110000000000000000000000001100 ; Set r12 to 0
.dw 0b01100000000000000010101111100000 ; If r11 == r14, then jump to offset 2
.dw 0b00010000000000000000000000000000 ; Return from interrupt
.dw 0b01110010011100100011000000000001 ; Set r1 to 0x2723 
.dw 0b01110000000011000000000100000001 ; Set highest 16 bits of r1 to 0xC0
.dw 0b01110000000001100100000000000011 ; Set r3 to 100
.dw 0b11010000000000000000000100110000 ; Write value in r3 to address in r1 with offset 0
.dw 0b01110010011100111001000000000001 ; Set r1 to 0x2739
.dw 0b01110000000011000000000100000001 ; Set highest 16 bits of r1 to 0xC0
.dw 0b01110000000000000001000000000010 ; Set r2 to 1
.dw 0b11010000000000000000000100100000 ; Write value in r2 to address in r1 with offset 0
.dw 0b11010000000000000001000100100000 ; Write value in r2 to address in r1 with offset 1
.dw 0b00010000000000000000000000000000 ; Return from interrupt
.dw 0b00010000000000000000000000000000 ; Return from interrupt, end of UART bootloader data


TILETABLE:
.db 0  1  2  3  4  5  0  0  0  0  0  0  0  0  0  0
.db 6  7  8  9  10 11 12 0  0  0  0  0  0  0  0  0
.db 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28
.db 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44
.db 45 46 47 48 49 50 51 0  0  52 53 54 55 56 0  57
.db 58 59 60 61 62 63 0  0  0  0  0  0  0  0  0  0

LOGOTABLE: ; 252 words long
.dw 0 0 0 0 ; tile 0, background, so always empty
.dw 0b00000000000000000000000000000011
.dw 0b00000000000000110000000000000000
.dw 0b00000000110000000000001111110000
.dw 0b00000000111111000000000000111111 ; tile  1
.dw 0b00000000000000001100000000000000
.dw 0b11110000000000001111110000000000
.dw 0b00111111000011110000110000111111
.dw 0b00000000111111000000001111111100 ; tile  2
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000011
.dw 0b11111111000011111111111111000011
.dw 0b00000011111100000000000011111100 ; tile  3
.dw 0b00000000000000000011110000000000
.dw 0b11111100000000001111000000000000
.dw 0b11000000001111000000000011111100
.dw 0b00000011111100000000111111000000 ; tile  4
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000011000000000000 ; tile  5
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000011110000
.dw 0b00000000111111000000000000111111
.dw 0b00110000000011111111110000000011 ; tile  6
.dw 0b11110000000011111111110000000000
.dw 0b00111111000000000000111111000011
.dw 0b00000011000011110000000000111111
.dw 0b11000000111111001100001111110000 ; tile  7
.dw 0b00001111110011110011111100000011
.dw 0b11111100000000001111110000000000
.dw 0b11001111000000000000001111000000
.dw 0b00000000111100000000000000111100 ; tile  8
.dw 0b00000000001111111100000000001111
.dw 0b11110000000000110011110000000000
.dw 0b00001111000000000000001111000000
.dw 0b00000000111100000000000000111100 ; tile  9
.dw 0b00001111000000001100000000000011
.dw 0b11110000000011111111110000111111
.dw 0b00111111000011000000111111000000
.dw 0b00000011111100000000000011111100 ; tile  10
.dw 0b11111100000000001111000000000000
.dw 0b11000000000000000000000011110000
.dw 0b00000011111100000000111111000000
.dw 0b00111111000000000011110000000011 ; tile  00
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b11000000000000001111000000000000 ; tile  12
.dw 0b00111111000000000000111111000000
.dw 0b00000011111100000000000011000011
.dw 0b00000000000011110000000000111111
.dw 0b00000000111111000000000011110000 ; tile  13
.dw 0b00001111110000000011111100000000
.dw 0b11111100000000001111000000000000
.dw 0b11000000001111110000000000110011
.dw 0b00000000111111110000001111000000 ; tile  14
.dw 0b00000000111111110000000011111111
.dw 0b00000000111111110000000011111111
.dw 0b00000000111111110000000011111111
.dw 0b00000000111111110000000011111111 ; tile  15
.dw 0b11111111111111111111111111111111
.dw 0b11111111111111111100000000000000
.dw 0b11000000000000001100000000000000
.dw 0b11111111111111001111111111111100 ; tile  16
.dw 0b11111100001111111111110000001111
.dw 0b11111100000000110000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000 ; tile  17
.dw 0b00000000000011111100000000111111
.dw 0b11110000111111001111110000110000
.dw 0b00111111000000000000111111000000
.dw 0b00111111111100001111000011110000 ; tile  18
.dw 0b11000011111111110000001111111111
.dw 0b00000011111111110000001111111111
.dw 0b00000011111111110000001111111111
.dw 0b00000011111111110000001111111111 ; tile  19
.dw 0b11111111111111001111111111111100
.dw 0b11111111111111000000000000111111
.dw 0b00000000001111110000000000111111
.dw 0b00000000001111110000000000111111 ; tile  20
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000001111000000001111
.dw 0b11110000000011111111000000001111
.dw 0b11110000000011111111000000001111 ; tile  21
.dw 0b00111111111111110011111111111111
.dw 0b00111111111111111111110000000000
.dw 0b11111100000000001111110000000000
.dw 0b11111100000000001111110000000000 ; tile  22
.dw 0b11111111110000001111111111000000
.dw 0b11111111110000000000001111111111
.dw 0b00000011111111110000001111111111
.dw 0b00000000000000000000000000000000 ; tile  23
.dw 0b00000000000000110000000000000011
.dw 0b00000000000000110000000011111111
.dw 0b00000000111111110000000011111111
.dw 0b00000000111111110000000011111111 ; tile  24
.dw 0b11111111111111111111111111111111
.dw 0b11111111111111111100000000000000
.dw 0b11000000000000001100000000000000
.dw 0b11000000000000001100000000000000 ; tile  25
.dw 0b11111100000000001111110000000000
.dw 0b11111100000000000011111111110000
.dw 0b00111111111100000011111111110000
.dw 0b00000000000000000000000000000000 ; tile  26
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000001111
.dw 0b00000000000011110000000011111111
.dw 0b00000000111111000000111111111100 ; tile  27
.dw 0b00111111110000000011111111000000
.dw 0b00111111110000001111111111000000
.dw 0b11111111110000001111111111000000
.dw 0b00111111110000000011111111000000 ; tile  28
.dw 0b00000000111100000000000011110000
.dw 0b00000000111111000000000000111111
.dw 0b00000000000011110000000011000011
.dw 0b00000011111100000000111111000000 ; tile  29
.dw 0b00001111000000000011110000001111
.dw 0b11110000000011001100000000001111
.dw 0b11000000001111001111000011110000
.dw 0b11111111110000000011111100000000 ; tile  30
.dw 0b00000000111111111100000011111111
.dw 0b11000000111111111100000011111111
.dw 0b00000000111111110000000011111111
.dw 0b00000000111111110000000011111111 ; tile  31
.dw 0b11111111111111001100000000000000
.dw 0b11000000000000001100000000000000
.dw 0b11000000000011111100000000001100
.dw 0b11000000000011111111000000000000 ; tile  32
.dw 0b00000000000000110000000011111111
.dw 0b00000000110011000000000011111100
.dw 0b11000000000000001100000000000000
.dw 0b11000000000000111111000000001111 ; tile  33
.dw 0b11000000111100000000000011110000
.dw 0b00000011111100000000111111000000
.dw 0b00111111000000001111110000110000
.dw 0b11110000111111001100000000111111 ; tile  34
.dw 0b00000011111111110000001111111111
.dw 0b00000011111111110000001111111111
.dw 0b00000011111111110000001111111111
.dw 0b00000011111111110000001111111111 ; tile  35
.dw 0b00000000001111111111111111111100
.dw 0b11111111111111001111111111111100
.dw 0b11111111111111000000000000000000
.dw 0b00000000000000000000000000000000 ; tile  36
.dw 0b11110000000011110000000000001111
.dw 0b00000000000011110000000000001111
.dw 0b00000000000011110000000000001111
.dw 0b00000000000011110000000000001111 ; tile  37
.dw 0b11111100000000001111110000000000
.dw 0b11111100000000001111110000000000
.dw 0b11111100000000001111110000000000
.dw 0b11111100000000001111110000000000 ; tile  38
.dw 0b00000000000000001111111111111111
.dw 0b11111111111111111111111111111111
.dw 0b11111111111111110000001111111111
.dw 0b00000011111111110000001111111111 ; tile  39
.dw 0b00000000111111110000000011111111
.dw 0b00000000111111110000000011111111
.dw 0b00000000111111110000000011111111
.dw 0b00000000111111110000000011111111 ; tile  40
.dw 0b11000000000000001100000000000000
.dw 0b11000000000000001100000000000000
.dw 0b11000000000000001100000000000000
.dw 0b11000000000000001100000000000000 ; tile  41
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000011111111110000
.dw 0b00111111111100000011111111110000 ; tile  42
.dw 0b00001111110000001111111111000000
.dw 0b11111100000000001111111111111111
.dw 0b11111111111111111111111111111111
.dw 0b00000000000000000000000000000000 ; tile  43
.dw 0b00111111110000000011111111000000
.dw 0b00111111110000001111111111111111
.dw 0b11111111111111111111111111111111
.dw 0b00111111110000000011111111000000 ; tile  44
.dw 0b00111111000000001111110000000011
.dw 0b00110000000011110000000000111111
.dw 0b00000000111111000000000011110000
.dw 0b00000000000000000000000000000000 ; tile  45
.dw 0b00001111110000001100001111110000
.dw 0b11000000111111000000000000111111
.dw 0b00000011000011110000111111000011
.dw 0b00111111000000001111110000000000 ; tile  46
.dw 0b00000000111111110000000011111111
.dw 0b00000000111111110000000000001111
.dw 0b11000000000011111111000000111100
.dw 0b11111100111100000011111111000000 ; tile  47
.dw 0b11111100000000001100111100000000
.dw 0b11000011110000000000000011110000
.dw 0b00000000001111000000000000001111
.dw 0b00000000000000110000000000001111 ; tile  48
.dw 0b00111100001111110000111111111100
.dw 0b00000011111100000000111111000000
.dw 0b00111111000011001111110000111111
.dw 0b11110000000011111100000000000011 ; tile  49
.dw 0b00000000000011110011110000000011
.dw 0b00111111000000000000111111000000
.dw 0b00000011111100000000000011110000
.dw 0b11000000000000001111000000000000 ; tile  50
.dw 0b11000011111111111111001111111111
.dw 0b11000011111111110000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000 ; tile  51
.dw 0b00111111111111110011111111111111
.dw 0b00111111111111110000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000 ; tile  52
.dw 0b11111111111111111111111111111111
.dw 0b11111111111111110000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000 ; tile  53
.dw 0b00000000000000110000000000000011
.dw 0b00000000000000110000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000 ; tile  54
.dw 0b11111111111111111111111111111111
.dw 0b11111111111111110000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000 ; tile  55
.dw 0b11111100000000001111110000000000
.dw 0b11111100000000000000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000 ; tile  56
.dw 0b00111111110000000011111111000000
.dw 0b00111111110000000000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000 ; tile  57
.dw 0b00000000000000110000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000 ; tile  58
.dw 0b11110000000011111100000000111111
.dw 0b00000000111111000000001111110000
.dw 0b00000011110000000000000000000000
.dw 0b00000000000000110000000000000011 ; tile  59
.dw 0b00001111110000000000001111110000
.dw 0b00000000111111000000110000111111
.dw 0b00111111000011111111110000000000
.dw 0b11110000000000001100000000000000 ; tile  60
.dw 0b00000000001111110000000011111100
.dw 0b00000011111100001111111111000011
.dw 0b11111111000011110000000000000011
.dw 0b00000000000000000000000000000000 ; tile  61
.dw 0b00001111000000000000111111000000
.dw 0b00000011111100000000000011111100
.dw 0b11000000001111001111000000000000
.dw 0b11111100000000000011110000000000 ; tile  62
.dw 0b11111100000000000011000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000
.dw 0b00000000000000000000000000000000 ; tile  63


; interrupt handlers are required for assembler, but will be removed in bootloader, since no interrupts are possible during bootloading

Int1:
    reti

Int2:
    reti

Int3:
    reti

Int4:
    reti