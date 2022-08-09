#include "data.h"

/* this file is necessary to provide the correct linking order of monitor */

/* monitor data */
int con_type;               /* console type */
int coldinit;               /* cold start flag */
int echo;                   /* enable/disable echo on console */

union bufdata buffers;      /* shared: indblks, fd[0], fd[1], fd[2] */
struct iobuf fdbuffers[4]; /*

int monaddr;                /* entered address in monitor */
int monlast;                /* last addr used in monitor */
int moncur;                 /* current address in monitor */
int monrep;                 /* repeat count in monitor */
int coloncmd;               /* stores letter for colon cmds */
char monline[160];          /* line buffer for monitor */
char *chptr;                /* point to next char */
jmp_buf monreturn;          /* setjmp buffer for return to monitor */
char monreg[8];             /* name buffer for register name */
int monaddrsz;              /* size of address (short/long) */
struct specregs specregs;   /* storage for special CPU registers */
int monregflg;              /* distinguish between address from register or mem */

int noopenerr;              /* suppress error message when device open fails */

int iccloaded;              /* flag that icc has downloaded its kernel */
int hasicc;                 /* flag that icc exists in system */
int icc_noprint;            /* flag that echo on icc is suppressed */

char bootfile[52];          /* boot file */

int textsize;               /* .text size of kernel to load */
int datasize;               /* .data size of kernel to load */
struct exvec regframe;      /* exception frame */

int _unused;                /* filler? */

char il_bootfile[32];       /* bootfile for IL boot */

struct bkpt breakpoints[16]; /* settable breakpoints */
int monrepcnt;              /* repeat counter */
int montraceflg;            /* flag used in single step */
int monrunflg;              /* flag used in go cmd and single step */
int vbrsave;                /* temporary save for VBR */

/* console data */
char dh_char[2];
short dh_inited;
short dh11_char;
short col_column;
short col_line;
short _unused2[4];          /* filler ? */

/* disassembler data */
unsigned short ioffset;     /* instruction offset from start of instr */
short _unused3[2];          /* filler? */

/* util data */
int fmt_width;              /* format width for printf */

/*int con_current;      is last address of minitor memory */

/*end of minitor RAM - total size = 32K */
