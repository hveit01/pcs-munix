/* defines for the Motorola 68881 floating point coprocessor */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* Control Register (FPCR)
  31            23              15              7
 +--------------------------------------------------------+
 |              0               |     Enable    |   Mode  |
 +--------------------------------------------------------+

 Enable byte: (if bit set, corresponding trap is enabled)
    15     14     13      12     11    10     9       8
 +--------------------------------------------------------+
 | BSUN | SNAN | OPERR | OVFL | UNFL | DZ | INEX2 | INEX1 |
 +--------------------------------------------------------+

 Mode byte: (select rounding mode and precision)
     7      6      5      4      3      2     1      0
 +-------------------------------------------------------+
 |    PREC     |     RND     |            0              |
 +-------------------------------------------------------+

*/

#define BSUN     0x8000 /* branch/set on unordered */
#define SNAN     0x4000 /* signalling not-a-number */
#define OPERR    0x2000 /* operand error */
#define OVFL     0x1000 /* overflow */
#define UNFL     0x0800 /* underflow */
#define DZ       0x0400 /* divide by zero */
#define INEX2    0x0200 /* inexact operation */
#define INEX1    0x0100 /* inexact decimal input */

/* PREC values */
#define RPEXT   0x0000  /* rounding precision extended (bits 7,6 = 00) */
#define RPSIN   0x0040  /* rounding precision single (bits 7,6 = 01) */
#define RPDBL   0x0080  /* rounding precision double (bits 7,6 = 10) */

/* RND values */
#define RNNEAR  0x0000  /* rounding mode to nearest (bits 5,4 = 00) */
#define RNZERO  0x0010  /* rounding mode to zero (bits 5,4 = 01) */
#define RNMINF  0x0020  /* rounding mode to -infinity (bits 5,4 = 10) */
#define RNPINF  0x0030  /* rounding mode to +infinity (bits 5,4 = 11) */

/* Status Register (FPSR)
  31            23              15              7
 +--------------------------------------------------------+
 |    Cond      |     Quo       |     Exc       |   Acc   |
 +--------------------------------------------------------+

 Condition Code Byte (Cond, set by FMOD or FREM):
    31     30     29     28     27     26    25      24
 +--------------------------------------------------------+
 |              0            |  N   |  Z  |   I   | NAN   |
 +--------------------------------------------------------+

 Quotient Byte (Quo):
    23     22     21     20     19     18    17     16
 +-------------------------------------------------------+
 | sign |  Seven least significant bits of quotient      |
 +-------------------------------------------------------+

 Exception Byte (Exc, set during last instruction):
    15     14     13      12     11    10     9       8
 +--------------------------------------------------------+
 | BSUN | SNAN | OPERR | OVFL | UNFL | DZ | INEX2 | INEX1 |
 +--------------------------------------------------------+

 Accrued Exception Byte (acc):
     7      6      5     4    3      2      1      0
 +-----------------------------------------------------+
 |  IOP | OVFL | UNFL | DZ | INEX |         0          |
 +-----------------------------------------------------+

*/

/* Instruction Address Register FPIAR */
/* contains address of last executed floating point instruction (32 bits) */
