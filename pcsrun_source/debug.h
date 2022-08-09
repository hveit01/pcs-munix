
/* debugflags */
#define DEBFLG_A	0x1000	/* trace all instrs executed */
#define DEBFLG_B	0x0004	/* dump buffer on syscalls */
#define DEBFLG_C	0x0800	/* break on call */
#define DEBFLG_D	0x0001	/* add debug output */
#define DEBFLG_E	0x0020	/* break on exec */
#define DEBFLG_F	0x0002	/* print stack top */
#define DEBFLG_K	0x2000	/* trace syscalls (K=Kernel) */
#define DEBFLG_L	0x0100	/* log to file debug.log */
#define DEBFLG_M	0x0008	/* break to monitor on start */
#define DEBFLG_O	0x0040	/* break on open/close */
#define DEBFLG_S	0x0200	/* break on any syscall */
#define DEBFLG_T	0x0400	/* trace call/return */
#define DEBFLG_U	0x0080	/* don't unlink files */
#define DEBFLG_W	0x4000	/* warn on read from NULL ptr, trap on write */
#define DEBFLG_X	0x0010	/* break on exit */
