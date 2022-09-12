/* Commands for ICC floppy */

/* io control command */
#define IFCTRL (('r'<<8)|1)

/* structure for IFCTRL  */
struct  ifop    {
	short   if_op;          /* operations defined below */
};

#define IF_FORMAT	1
#define IF_SWAP         2
#define IF_NOSWAP       3
#define IF_ERRREPORT	20
#define IF_ERRCLEAR	21


/* example:
	struct ifop rop;
	rop.if_op = IF_FORMAT;
	ioctl(fd,IFCTRL,&rop);
*/
