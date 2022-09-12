#include <sys/icc/icc_flags.h>

#ifdef mips
#define ICC0_BASE 0xbfffe300      /* ICC Q-Bus base address for ICC board 0 */
#define ICC1_BASE 0xbfffe308      /* ICC Q-Bus base address for ICC board 1 */
#define ICC2_BASE 0xbfffe310      /* ICC Q-Bus base address for ICC board 2 */
#define ICC3_BASE 0xbfffe318      /* ICC Q-Bus base address for ICC board 3 */
#else
#define ICC0_BASE 0x3fffe300      /* ICC Q-Bus base address for ICC board 0 */
#define ICC1_BASE 0x3fffe308      /* ICC Q-Bus base address for ICC board 1 */
#define ICC2_BASE 0x3fffe310      /* ICC Q-Bus base address for ICC board 2 */
#define ICC3_BASE 0x3fffe318      /* ICC Q-Bus base address for ICC board 3 */
#endif
typedef struct rtk_header
{
	struct unix_message *link;
	long    sender;
	short 	flag;
	char 	id[2];
} RTK_HEADER;

#define NOWAIT	0
#define WAIT	-1

#define Init_message(msg)	(msg)->link = NULL
extern int icc_initflg[];

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

struct  down_initprocs {
	int (*func)();
	int icc_nr;
};
