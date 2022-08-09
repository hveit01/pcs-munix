| PCS specific
| unixlock.a68
|
| provides spinlock semaphore
|
    .text
    .global _Locked
_Locked:
sema = 4
    movl    sp@(sema), a0           | obtain semaphore
    movl    #32000, d0              | wait time
1$:
    subql   #1, d0                  | decrement timer
    beqs    99$                     | time expired
    tas     a0@                     | released?
    bnes    1$                      | no, spin
99$:
    rts                             | exit

    .global _Set_lock
_Set_lock:
sema = 4
    movl    sp@(sema), a0           | obtain semaphore
1$:
    tas     a0@                     | wait for sema
    bnes    1$                      | spin

    rts

    .global _Lock
_Lock:
sema = 4
    movl    sp@(sema), a0           | obtain semaphore
    movw    sr, a0@(2)              | store old SR
    movw    #0x2700, sr             | SPL 7
1$:
    tas     a0@                     | wait for sema
    bnes    1$                      | spin

    rts

    .global _Unlock
_Unlock:
sema = 4
    movl    sp@(sema), a0           | obtain semaphore
    clrb    a0@                     | release sema
    movw    a0@(2), sr              | restore old SR
    rts
