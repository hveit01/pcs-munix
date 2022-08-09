|-----------------------------------------------------------------------------
| the following routines form a block of relocatable routines
| which may be moved in memory to allow testing of all memory routines
| for efficiency it is written in assembler. 
| Critical calls use sa_ wrappers, but since monitor ROM is still mapped
| some uncritical code and data still points to ROM
|-----------------------------------------------------------------------------

| externals
        .extern _haschar, _getchar, _putchar, _printf, _gets
        .extern _mem_getlimits
    
        .extern sa_inkey, sa_crlf, sa_printf, sa_print_dec
        .extern sa_print_hex8, sa_print_hex4, sa_print_hex2, sa_print_hex1
        .extern hex2bin

PCR =   0x3ffc0000
ESR =   0x3ffe8002


| static variables in BSS
        .bss
memstart:
        .= .+4
memend:
        .= .+4
regsave:
        .= .+0x40
linebuf:
        .= .+0xbe
        .= .+0x1100

| initialized data
        .data
        .globl  _mem_version
_mem_version:
        .asciz  "Version 1.02"
        .even

| date of minitor
        .globl  _mon_date
_mon_date:
        .asciz  "11 Sep 86"
        .even

        .text
|
| This is the only entry point into these relocatable routines
|
|
| generally:
| A1 is memstart address
| A2 is memend address
| A3 is current address, or error location
| A4 is temporary save of frame pointer
| A6 frame pointer for regular C routines, pointer into input line
| D2 is value written to memory
| D3 is value read from memory
| D4 is current test number (1..9)
| D5 is reserved for error counting
| D6 is complement flag for house numbers, delay count
| D7 is reserved for the selected test flags
|   bit0
|   bit1      housenumber test
|   bit2      bit-shifting test
|   bit3      random-long test
|   bit4      random-word test
|   bit5      random-byte test
|   bit6      refresh test
|   bit7      opcode test
|   bit8      R/M/W test
|   bit9      force parity error
|   bit10 0a
|   bit11 0b
|   bit12 0c
|   bit13 0d
|   bit14 0e
|   bit15 0f
|   bit16     repeat tests
|   bit17 11  byte address mode
|   bit18 12  word address mode
|   bit19     set if house number test failed
|   bit20 14
|   bit21 15
|   bit22 16
|   bit23 17
|   bit24 18
|   bit25 19
|   bit26 1a
|   bit27 1b
|   bit28 1c
|   bit29 1d
|   bit30 1e
|   bit31 1f

        .globl _mem_runtests
_mem_runtests:
        moveml #0xffff, regsave         | save all regs
    
        movec  vbr, a0                  | get VBR
        movl   #membuserror, a0@(8)     | set buserror handler for memtest
        movl   #memaddrerror, a0@(12)   | set addr error handler for memtest
        movl   #memillerror, a0@(16)    | set ill op error handler for memtest
        clrl   d5                       | clear error count
        clrl   d7                       | clear testflags

        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTestselect, sp@-       | printf("Testprogramm-Auswahl...", )
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        | loop: query list of tests
1$:     moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTestPrompt, sp@        | printf("Testnummern...",0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #linebuf, sp@-           | line buffer
        jsr    _gets                    | get a line of data
        addql  #4, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        movl   #linebuf, a6             | point to linebuf[0]
        cmpb   #0x0d, a6@               | is char CR?
        beqs   3$                       | yes empty line
        cmpb   #0, a6@                  | is '\0' ?
        beqs   3$                       | yes empty line
        clrl   d0                       | char

        | get (another) test number
2$:     movb   a6@+, d0                 | get test#
        cmpb   #0x31, d0                | < '1', reject line
        bcss   1$
        cmpb   #0x39, d0                | > '9', reject line
        bhis   1$
        jsr    hex2bin                  | make binary
        bset   d0, d7                   | set bit in test flags
        cmpb   #0x0d, a6@               | at end of input?
        beqs   4$                       | yes, is end of input, go on
        cmpb   #0, a6@                  | i '\0' byte?
        beqs   4$                       | yes, is end of input, go on
        cmpb   #0x2c, a6@+              | comma follows?
        beqs   2$                       | yes, check for another test#
        bras   1$                       | invalid char, reject line
        
        | empty line, enable all tests, except force parity error
3$:     movw   #0x1FE, d7               | enable all tests

        | ask for memory limits
4$:     movl   #memend, sp@-            | obtain start/end address into memstart/memend
        movl   #memstart, sp@-
        jsr    _mem_getlimits
        addql  #8, sp
        movl   memstart, a1             | A1 is start address
        movl   memend, a2               | A2 is end address

        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aRepeat, sp@            | prompt for repeat test
        jsr    _printf                  | printf("\r\nDauertest (CR/n) ? ", 0)
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #linebuf, sp@-           | get reply into buffer
        jsr    _gets
        addql  #4, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        movl   #linebuf, a6             | check reply char
        cmpb   #0x4e, a6@               | is it 'N'?
        beqs   5$                       | yes, not repeat
        cmpb   #0x6e, a6@               | is it 'n'?
        beqs   5$                       | yes, not repeat
        bset   #0x10, d7                | set flag: repeat tests

        | now start tests
5$:     moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aStart, sp@-            | printf("\r\n\n\nQU68/32 - SPEICHERTEST ...", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        movl   a1, d1                   | print start address
        jsr    sa_print_hex8
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aMinus, sp@-            | printf(" - ");
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        movl   a2, d1                   | print end address
        jsr    sa_print_hex8
        jsr    sa_crlf                  | print CRLF
    
        btst    #0x10, d7               | repeat test?
        beqs   6$                       | no, skip
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aRepeatInfo, sp@-       | printf("\n    ....Dauertest....\r\n", 0);
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        | loop, repeating test
6$:     btst   #1, d7                   | do house number tests?
        beqs   7$                       | no, skip
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTestHouse, sp@-        | printf("\n** HOUSE NUMBER - Test\r\n", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bsrw   hntests                  | run house number tests

7$:     btst   #2, d7                   | do bit-shifting tests?
        beqs   8$                       | no, skip
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTestBitSh, sp@-        | printf("\n** BIT-SHIFTING - Test\r\n", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bsrw   mem_bitshift             | run bitshift tests

8$:     btst   #3, d7                   | do random long tests?
        beqs   9$                       | no, skip
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTestRanL, sp@-         | printf("\n** RANDOM-LONG - Test\r\n", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bsrw   mem_randomlong           | run random long tests

9$:     btst   #4, d7                   | do random word tests?
        beqs   10$
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTestRanW, sp@-         | printf("\n** RANDOM-WORD - Test\r\", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bsrw   mem_randomword           | run random word tests

10$:    btst   #5,d7                    | do random byte tests?
        beqs   11$                      | no
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTestRanB, sp@-         | printf("\n** RANDOM-BYTE - Test\r\n", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bsrw   mem_randombyte           | run random byte tests

11$:    btst   #6, d7                   | do refresh test?
        beqs   12$                      | no
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTestRefr, sp@-         | printf("\n** REFRESH - Test\r\n", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bsrw   mem_refresh              | do refresh test

12$:    btst   #7, d7                   | do opcode test?
        beqs   13$                      | no
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTestOpc, sp@-          | printf("\n** OPCODE - Test\r\n", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bsrw   mem_opcode               | do opcode tests

13$:    btst   #8, d7                   | do RMW tests?
        beqs   14$
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTestRMW, sp@-          | printf("\n** READ/MODIFY/WRITE - Test\r\n",0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bsrw   mem_rmw                  | do RMW test

14$:    btst   #9, d7                   | do parity error test?
        beqs   15$
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTestPar, sp@-          | printf("\n** FORCE PARITY ERROR - Test\r\n", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bsrw   mem_forceparity          | do force parity error test

        | all tests run once, repeat flag set?
15$:    btst   #0x10, d7                | repeat?
        bnew   6$                       | is set, loop
        
        bsrw   memfinish                | report errors
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aAlldone, sp@-          | print summary
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aFinished, sp@-         | printf("beendet\r\n\n\n", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bras   erexit1                  | exit

        | terminated with CTRL-C
memctrlc:
        bsrw   memfinish                | go to finish

        | terminated with memory error
memtermerr:
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aAlldone, sp@-          | print summary
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aCanceled, sp@-         | printf("abgebrochen\r\n\n\n", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        | exit from memory test
erexit1:
        moveml regsave, #0xffff         | restore all registers
        rts

        | finish tests, report number of errors, if any
memfinish:
        tstl   d5                       | errors?
        beqs   noerr                    | no, skip
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aErrors, sp@-           | printf("\r\n\nAnzahl der Speicherfehler : ",0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        | print # of errors
printecnt:
        movl   d5, d1                   | print error count
        jsr    sa_print_dec
        bras   finexit                  | exit

        | no errors
noerr:  moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aNoError, sp@-          | printf("\r\nKein Speicherfehler aufgetreten !", 0);
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        | exit
finexit:
        rts

aTestHouse: .asciz  "\n** HOUSENUMBER - Test\r\n"
aTestBitSh: .asciz  "\n** BIT-SHIFTING - Test\r\n"
aTestRanL:  .asciz  "\n** RANDOM-LONG - Test\r\n"
aTestRanW:  .asciz  "\n** RANDOM-WORD - Test\r\n"
aTestRanB:  .asciz  "\n** RANDOM-BYTE - Test\r\n"
aTestRefr:  .asciz  "\n** REFRESH - Test\r\n"
aTestOpc:   .asciz  "\n** OPCODE - Test\r\n"
aTestRMW:   .asciz  "\n** READ/MODIFY/WRITE - Test\r\n"
aTestPar:   .asciz  "\n** FORCE PARITY ERROR - Test\r\n"
aErrors:    .asciz  "\r\n\nAnzahl der Speicherfehler : "
aNoError:   .asciz  "\r\n\nKein Speicherfehler aufgetreten !"
aAlldone:   .ascii  "\r\n----------------------------------\r\n"
            .asciz  "QU68/32 - SPEICHERTEST "
aFinished:  .asciz  "beendet\d\n\n\n"
aCanceled:  .asciz  "abgebrochen\r\n\n\n"
        .even

    | house number test
hntests:
        bclr   #19, d7                  | clear error bit
        moveq  #1, d4                   | test number 1

        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTrueAddr, sp@-         | printf("True address",0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        movl  a6, a4                    | save frame ptr
        clrw   d6                       | clear complement flag
        bsrw   hnumup                   | do house number uncomplemented upwards

        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aTrueAddr, sp@-         | printf("True address",0)
        jsr    _printf
        addql  #8,sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bsrw   hnumdown                 | do house number uncomplemented downwards

        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aComplAddr, sp@-        | printf("Complement address")
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        movl   a6, a4                   | save frame pointer
        addqw  #1, d6                   | set complement flag
        bsrs   hnumup                   | do house number complemented upwards
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aComplAddr, sp@-        | printf("Complement address",0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff
        bras   hnumdown                 | do house number complemented downwards 
                                        | and return

        | do house numbers upwards
hnumup:
| curaddr = a3
| writeval = d2
| readval = d3
|endaddr = a2
        moveml #0xff00, sp@-            | save all
        moveml #0x00fe, sp@-
        movl   #0, sp@-
        movl   #aUp, sp@-               | printf(" - up\r\n", 0)
        jsr    _printf
        addql  #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

        movl   a1, a3                   | curaddr = startaddr

2201$:  movl   a3, d2                   | address into writeval
        btst   #0, d6                   | is complement flag set?
        beqs   2202$                    | no, skip
        notl   d2                       | negate addres

2202$:  movl   d2, a3@                  | save house number
        movl   a3@, d3                  | read it out again
        cmpl   d3, d2                   | same?
        beqs   2203$                    | yes, continue
        bsrw   printerr                 | is not, print error
        bset   #19, d7                  | set error flag

2203$:  bsrw   chkbreak                 | check key, and break to memtest
        addql  #4, a3                   | next addr
        cmpl   a3, a2                   | less than end addr?
        bccs   2201$                    | loop
        bras   hncommon


    | do house numbers downwards
hnumdown:
| endaddr = a2
| writeval = d2
| curaddr = a3
| readval = d3
| startaddr = a1
| testflg = d7
        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aDown, sp@-            | printf(" - down\r\n")
        jsr     _printf
        addql   #8,sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff
        movl    a2, a3                  | curaddr = endaddr

1$:     movl    a3, d2                  | writeval is curaddr or ~curaddr
        btst    #0, d6                  | complement flag?
        beqs    2$
        notl    d2                      | complement

2$:     movl    d2, a3@                 | save value
        movl    a3@, d3                 | read it back
        cmpl    d3, d2                  | same?
        beqs    3$                      | yes
        bsrw    printerr                | print mem error
        bset    #19, d7                 | set error flag

3$:     bsrw    chkbreak                | check for break back to memtest
        subql   #4, a3                  | previous addr
        cmpl    a1, a3                  | done?
        bccs   1$                       | no, loop

hncommon:
        btst    #19, d7                 | error seen?
        bnew    hnexit                  | yes, exit
        movl    a1, a3                  | reload startaddr

hnloop: movl    a3, d2                  | writeval = addr
        btst    #0, d6                  | need to complement?
        beqs    5$                      | no
        notl    d2                      | complement writeval

5$:     movl    a3@, d3                 | compare with value read
        cmpl    d3, d2                  | same?
        beqs    hnnoerr                 | yes, continue
        bsrw    printerr                | no, print mem error
        btst    #1, d6                  | was complementing?
        beqw    6$                      | no, skip

        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aXlinkDn, sp@-         | printf("---> Adressverkopplung abwaerts : ")
        jsr     _printf
        addql   #8,sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff
        braw    7$                      | else

6$:     moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aXlinkUp, sp@-         | printf("---> Adressverkopplung aufwaerts : ")
        jsr     _printf
        addql   #8, sp
        moveml sp@+, #0x7f00            | restore all
        moveml sp@+, #0x00ff

7$:     movl    a4, a6                  | restore frame pointer
        jsr     sa_printf               | print pushed printf arg
        jsr     sa_crlf                 | do CRLF

hnnoerr:
        bsrw    chkbreak                | check for abort
        addql   #4, a3                  | next address
        cmpl    a3, a2                  | at end?
        bccs    hnloop                  | no, loop

hnexit: bclr    #19, d7                 | clear error flag
        rts

aTrueAddr:  .asciz  "True address"
aComplAddr: .asciz  "Complement address"
aUp:        .asciz  " - up\r\n"
aDown:      .asciz  " - down\r\n"
aXlinkUp:   .asciz  "---> Adressverkopplung aufwaerts : "
aXlinkDn:   .asciz  "---> Adressverkopplung abwaerts : "
        .even

    | test with random long numbers
mem_randomlong:
| startaddr = a1
| curaddr = a3
| writeval = d2
| endaddr = a2
| readval = d3
        moveq   #3, d4                  | test #3
        movl    a1, a3                  | start addr
        movl    #0x12345678, d2         | init value
1$:     movl    d2, a3@+                | store values
        bsrs    crc                     | pertubate writeval through LFSR
        cmpl    a3, a2                  | at end?
        bccs   1$                       | no, loop

        movl    a1, a3                  | reload start
        movl    #0x12345678, d2         | and init value
2$:     movl    a3@, d3                 | read value
        cmpl    d3, d2                  | same?
        beqs    3$                      | yes, continue
        bsrw    printerr                | no, report error

3$:     bsrw    chkbreak                | check for break
        bsrs    crc                     | perturbate value
        addql   #4, a3                  | advance to next cell
        cmpl    a3, a2                  | at end?
        bccs    2$                      | no, loop
        rts

| LFSR linear feedback shift register
| uses d2 for next random value
| generator polynomial: x^32 + x^30 + x^29 + x^27
| I doubt this is very good, a recommendation for long sequences
| would be x^32 + x^22 + x^2 + x^1
crc:
| writeval = d2
        movw    #0, cc                  | clear X bit
        btst    #26, d2                 | bit 26 set?
        beqs    1$
        eorw    #0x10, cc               | complement X bit

1$:     btst    #28, d2                 | bit 28 set?
        beqs    2$
        eorw    #0x10, cc               | complement X bit

2$:     btst    #29, d2                 | bit 29 set?
        beqs    3$
        eorw    #0x10, cc               | complement X bit

3$:     btst    #31, d2                 | bit 31 set?
        beqs    4$
        eorw    #0x10, cc               | complement X bit
4$:     roxll   #1, d2                  | rotate X into; rotate
        rts

| test with shifted bits
mem_bitshift:
| writeval = d2
        moveq   #2, d4                  | test #2
        moveq   #1, d2                  | bit 0 set

1$:     bsrw   fillmem                  | fill memory
        bsrw   cmpmem                   | compare stored value
        lsll   #1,d2                    | shift 1 bit left
        bccs   1$                       | loop over all 32 bits
        notl   d2                       | complement bits
        bsrw   fillmem                  | fill
        bsrw   cmpmem                   | and compare
        subql  #1, d2                   | clear bit 0

2$:     bsrw   fillmem                  | fill and compare
        bsrw   cmpmem
        roll   #1,d2                    | rotate left
        bcss   2$                       | until no carry (=0 after 32 iterations)
        rts

| write pattern 55555555 delay long and check if value stored
| same for value aaaaaaaa

mem_refresh:
| writeval = d2
        moveq   #6,d4                   | test # 6
        movl    #0x55555555,d2          | fill with 55555555
        bsrs    1$                      | fill, wait and compare values
        notl    d2                      | complement to aaaaaaaa

1$:     bsrw    fillmem                 | fill memory
        bsrs    mem_delay               | wait some time
        bsrw    cmpmem                  | check if values are correct
        rts

mem_delay:
        clrl    d6                      | clear loop count
1$:     addl    #4000, d6               |
        bccs    1$                      | about 1,07Mio iterations
        rts


| rest opcode execute by putting RTS instructions in mem cand calling this addr
mem_opcode:
| startaddr = a1
| curaddr = a3
| endaddr = a2
| writeval = d2
| readval = d3
        moveq   #7, d4                  | test #7
        movl    #0x4E754E75, d2         | RTS, RTS instruction
        bsrw    fillmem                 | fill memory
        movl    a1, a3                  | set start addr

1$:     jsr     a3@                     | call address
        addql   #2, a3                  | to next RTS
        cmpl    a3, a2                  | at end?
        bhis    1$                      | no loop

        movl    a1, a3                  | reload start
2$:     movl    a3@, d3                 | read ram
        cmpl    d3, d2                  | ram unmodified?
        beqs    3$                      | yes, ok
        bsrw    printerr                | no, report error
        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aMemChgd, sp@-         | printf("---> Speicherinhalt durch ...")
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

3$:     bsrw    chkbreak                | check for break
        addql   #4, a3                  | next addr
        cmpl    a3, a2                  | at end?
        bccs    2$
        rts
aMemChgd:   .asciz  "---> Speicherinhalt durch Befehlsausfuehrung veraendert !\r\n"

| do read/modify/write test by execution TAS instructions
mem_rmw:
| startaddr = a1
| curaddr = a3
| writeval = d2
| readval = d3
| endaddr = a2
| testflg = d7
        moveq   #8, d4                  | test #8
        bset    #17, d7                | set flag for byte
        movl    a1, a3                  | curaddr = startaddr

1$:     clrb    d2
        clrb    a3@                     | clear at a3
        movb    a3@, d3                 | read out value
        tas     a3@                     | test and set
        beqs    2$                      | was clear, now set?
        bsrw    printerr                | no, report error

2$:     bsrw    chkbreak                | check for break
        movb    #0x80, d2               | set writval
        movb    a3@, d3                 | read it out
        tas     a3@                     | test and set
        bmis    3$                      | was set?
        bsrw    printerr                | no, error

3$:     bsrw    chkbreak                | check for break
        cmpl    a3, a2                  | at end?
        bcss    99$                     | yes, done
        addql   #1, a3                  | next byte
        bras    1$                      | loop

99$:    bclr    #17, d7                 | clear byte flag
        rts

| same as mem_randomlong, but on 8 bits
mem_randombyte:
        moveq   #5, d4                  | test #5
        bset    #17, d7                 | set byte flag
        movl    a1, a3                  | curaddr = startaddr
        movl    #0x12345678, d2         | init value for crc

1$:     bsrw   crc                      | calc next CRC
        movb    d2, a3@                 | store it
        movb    a3@, d3                 | read it back
        cmpb    d3, d2                  | same ?
        beqs    2$                      | yes, skip
        bsrw    printerr                | no report error

2$:     bsrw    chkbreak                | check for break
        cmpl    a3, a2                  | at end?
        bcss    99$                     | yes exit
        addql   #1, a3                  | next byte
        bras    1$                      | loop

99$:    bclr    #17, d7                 | clear byte flag
        rts


| same as mem_randomlong, except on 16 bit
mem_randomword:
        moveq   #4,d4                   | test #4
        bset    #18, d7                 | set word flag
        movl    a1, a3                  | curaddr = startaddr
        movl    #0x12345678, d2         | init value

1$:     bsrw    crc                     | calc crc
        movw    d2, a3@                 | store value
        movw    a3@, d3                 | read back
        cmpw    d3, d2                  | same?
        beqs    2$                      | yes, skip
        bsrw    printerr                | no, print mem error

2$:     bsrw    chkbreak                | check for break
        addql   #2, a3                  | next word
        cmpl    a3, a2                  | at end?
        bccs    1$                      | no, loop
        bclr    #18, d7                 | clear word flag
        rts

| enforce a parity error, i.e. check parity logic
| this is not part of standard tests
mem_forceparity:
| writeval = d2
        moveq   #9, d4                  | test #9
        bset    #17, d7                 | byte mode
        movl    a0,sp@-                 | save a0
        movec   vbr, a0                 | point to VBR
        movl    #memparerr, a0@(8)      | set buserror vector
        movl    sp@+, a0                | restore a0
        movw    #0x5A5A, d2             | set value
        bsrs    writepar                | fill memory with 0x5a
        bsrs    readpar                 | read with parity, should be error
        movw    #0x5B5B, d2             | try the same for 0x5b
        bsrs    writepar
        bsrs    readpar
        movl    a0, sp@-                | save a0
        movec   vbr, a0                 | reset buserror vector to
        movl    #membuserror, a0@(8)    | default memtest bus error
        movl    sp@+, a0                | restore a0
        bsrw    fillmem                 | fill memory
        bclr    #17, d7                 | reset byte mode
        rts

| write bytes with parity set
writepar:
| startaddr = a1
| curaddr = a3
| writeval = d2
| endaddr = a2
        movl    a1, a3                  | curaddr = startaddr
        movw    #0x31, PCR              | set PCR: RUN, parity disable?

1$:     movb    d2, a3@+                | save byte
        cmpl    a3, a2                  | at end?
        bccs    1$                      | no, loop
        movw    #0x21, PCR              | reset parity enable?
        rts

| read bytes with wrong parity
readpar:
| startaddr = a1
| curaddr = a3
| readval = d3
| endaddr = a2
        movl    a1, a3                  | curaddr = startaddr

        | loop
readpar1:
        movb    a3@, d3                 | try read out memory
        nop                             | wait
        nop

readpar2:
        nop
        bsrw    printerr                | report error: there should have been
                                        | a parity error
        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aNoParErr, sp@-        | printf("---> Kein Parity-Fehler erzeugt !\r\n",0)
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff
        bras    chkend                  | check for end of loop

        | parity bus error vector point here
memparerr:
        nop                             | buserror vector for parity test
        movw    sp@(6), d0              | error fram on stack
        btst    #12, d0                 | is frame type 1010?
        bnew    1$                      | yes, skip
        addl    #32, sp                 | no, pop 68010 exception frame
        braw    2$

1$:     addl    #92, sp                 | yes, pop 68020 exception frame

2$:     movw    ESR, d0                 | get ESR
        btst    #8, d0                  | test bit 8 - PBUS-PARITY error
        beqs    readpar2                | no error, but should be, report error

chkend: bsrw    chkbreak                | check for break
        cmpl    a3, a2                  | at end?
        bcss    3$                      | yes, exit
        addql   #1, a3                  | advance
        bras    readpar1                | loop

3$:     rts

aNoParErr:  .asciz  "---> Kein Parity-Fehler erzeugt !\r\n"




| fill memory with a value in d2
fillmem:
| startaddr = a1
| curaddr = a3
| writeval = d2
| endaddr = a2
        movl    a1, a3                  | curaddr = startaddr

1$:     movl    d2, a3@+                | store value and advance
        cmpl    a3, a2                  | at end? 
        bccs    1$                      | no, loop
        rts



| compare mem with value in d2
cmpmem:
| startaddr = a1
| curaddr = a3
| readval = d3
| writeval = d2
| endaddr = a2
        movl a1, a3                     | curaddr = startaddr

1$:     movl    a3@, d3                 | read out value
        cmpl    d3, d2                  |  same?
        beqs    2$                      | yes, goon
        bsrs    printerr                | no, report error

2$:     bsrs    chkbreak                | check for break
        addql   #4, a3                  | next addr
        cmpl    a3, a2                  | at end?
        bccs    1$                      | no, loop
        rts

| check for key, CTRL-C, break test if yes

chkbreak:
        jsr     sa_inkey                | is key hit?
        beqw    chkret                  | no key, exit
        cmpb    #3, d0                  | is CTRL-C?
        beqw    memctrlc                | yes, goto ctrl_c
        braw    printerr2               | no, report error



| print error location
printerr:
| errnum = d5
| testnum = d4
| curaddr = a3
| writeval = d2
| testflgs = d7
| readval = d3
        tstw    PCR                     | read PCR
        movw    #0x0221, PCR            | set RUN,PBUS-CONF,unknownbit9

        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aError_0, sp@-         | printf("-> Error: ", 0)
        jsr     _printf
        addql   #8,sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

        addql   #1, d5                  | add errcnt
        movw    #0x%021, PCR            | set RUN,PBUS-CONF
    
printerr2:
        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aTest, sp@-            | printf("Test #", 0)
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

        movw    d4, d1                  | print test#
        jsr     sa_print_hex1

        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aAddr, sp@-            | printf(" addr: ", 0)
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

        movl    a3, d1                  | print error address
        jsr     sa_print_hex8

        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aWrite_2, sp@-         | printf(" write: ", 0)
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

        movl    d2, d1                  | value written
        btst    #17, d7                 | byte mode flag?
        bnes    print8bit               | yes, print 8bit
        btst    #18, d7                 | word; flag 18?
        bnew    print16bit              | yes, print 16 bit
        jsr     sa_print_hex8           | no, print write value as 32bit

        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aRead_2, sp@-          | printf(" read: ")
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

        movl    d3, d1                  | print value read as 32bit
        jsr     sa_print_hex8

    | print error summary
printesum:
        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aErrSum, sp@-          | printf(" err-sum: ")
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

        bsrw    printecnt               | print error count (d5)
        jsr     sa_crlf                 | print crlf

chkret: rts

    | print value as 8bit
print8bit:
        jsr     sa_print_hex2           | write value as 8 bit

        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aRead_2, sp@-          | printf(" read: ")
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

        movb    d3, d1                  | read value as 8 bit
        jsr     sa_print_hex2
        bras    printesum               | print error count

print16bit:
        jsr     sa_print_hex4           | print as 16 bit
        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aRead_2, sp@-          | printf(" read: ")
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

        movw    d3, d1                  | print read value as 16 bit
        jsr     sa_print_hex4
        braw    printesum               | print error count

aError_0:   .asciz  "-> Error: "
aTest:      .asciz  "Test #"
aAddr:      .asciz  " addr: "
aWrite_2:   .asciz  " write: "
aRead_2:    .asciz  " read: "
aErrSum:    .asciz  " err-sum: "


| this is the bus error vector for memory test
membuserror:
        movw    ESR, d1                 | get ESR
        btst    #3, d1                  | check bit 3 (invalid address)
        beqs    10667$

        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aTimeoutError, sp@-    | printf("\r\n\n--> Timeout Error", 0)
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

        bras    prtaccaddr              | fault addr

10667$: btst    #1, d1                  | test bit 1 parity error
        beqs    10668$                  | not set

        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aParityError, sp@-     | printf("\r\n\n--> Parity Error", 0)
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

        bras    prtaccaddr              | fault addr

10668$: moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aBusErrorEsr, sp@-     | printf("\r\n\n---> Bus Error : ESR = ", 0)
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff
        jsr     sa_print_hex4           | print ESR which is in D1

    | print fault address
prtaccaddr:
        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aAccessAdress, sp@-    | printf("    access address: ", 0)
        jsr     _printf
        addql   #8,sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff
        addl    #16, sp                 | advance to fault address in stack frame.
                    | Note this is a type 10 or 11 exception frame,
                    | so at 0x10 there is the 'data cycle fault address'

prterradr:
        movl    sp@, d1                 | get address
        jsr     sa_print_hex8           | print it
        braw    memtermerr              | memory error

| this is the address error vector used by memtest
memaddrerror:
        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aOddAddressErro, sp@-  | printf("\r\n\n--> Odd Address Error", 0)
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff

        bras    prtaccaddr              | print fault addr


| this is the ill opcode error vector used by memtest
memillerror:
        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aInstructionErr, sp@-  | printf("\r\n\n---> Instruction Error", 0)
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff
        addl    #2, sp                  | Adjust SP
                        | This is a regular exception frame which has PC at sp+2
        moveml  #0xff00, sp@-           | save all
        moveml  #0x00fe, sp@-
        movl    #0, sp@-
        movl    #aPc_0, sp@-            | printf("    pc: ", 0)
        jsr     _printf
        addql   #8, sp
        moveml  sp@+, #0x7f00           | restore all
        moveml  sp@+, #0x00ff
        
        braw    prterradr               | print d1

|-----------------------------------------------------------------------------
| end of relocatable area
|-----------------------------------------------------------------------------
    .globl  _mem_endtests
_mem_endtests:

| this does not need to be copied, remains in ROM .text
aBusErrorEsr:   .asciz  "\r\n\n---> Bus Error : ESR = "
aTimeoutError:  .asciz  "\r\n\n---> Timeout Error"
aParityError:   .asciz  "\r\n\n---> Parity Error"
aAccessAdress:  .asciz  "    access adress: "
aPc_0:          .asciz  "    pc: "
aOddAddressErro:.asciz  "\r\n\n---> Odd Address Error"
aInstructionErr:.asciz  "\r\n\n---> Instruction Error"
aTestselect:    .ascii  "\r\n\nTestprogramm-Auswahlliste :\r\n"
                .ascii  "---------------------------\r\n"
                .ascii  "\n"
                .ascii  "Housenumber     = 1             Refresh            = 6\r\n"
                .ascii  "Bit-Shifting    = 2             Opcode             = 7\r\n"
                .ascii  "Random-Long     = 3             Read/Modify/Write  = 8\r\n"
                .ascii  "Random-Word     = 4             Force Parity Error = 9\r\n"
                .ascii  "Random-Byte     = 5             Tests 1...8        = CR\r\n"
                .asciz  "________________________________________________________"
aTestPrompt:    .ascii  "\r\n\nTestnummern (getrennt d. "
                .asciz  "\',\') : "
aUnused1:       .asciz  "\r\nSpeicher-Teilbereich testen (j/CR) ? "
aRepeat:        .asciz  "\r\nDauertest (CR/n) ? "
aStart:         .ascii  "\r\n\n\nQU68/32 - SPEICHERTEST  ---  Start\r\n"
                .ascii  "----------------------------------\r\n\n"
                .asciz  "Speicherbereich :  "
aMinus:         .asciz  " - "
aRepeatInfo:    .asciz  "\n    ******  Dauertest  ******\r\n"
                .even
