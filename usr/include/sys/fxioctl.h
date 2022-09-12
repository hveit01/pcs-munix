 
typedef struct {
	short use_count;
	short nwords;
	short words[1];
	} gd_gacbuffer;
 
struct fxbuf {
	short *addr;
	int bytecnt;
	int stat;
	};
 
struct ver_struct {
	char	version[6];
	long	protocol;
};
 
struct data_back {
	unsigned short write_ptr;
	unsigned long time_out;
	unsigned long	status;
};
 
struct fxdata {/* parameter type (in/out) */ /* definition.......*/
	short mode;		/* INPUT  	- startup bits */
	long memsize;		/* INPUT	- requested memory size */
	long Dbase;		/* OUTPUT 	- Device base address */
	long memory_start;	/* OUTPUT 	- start of free memory */
	long memory_size;	/* OUTPUT	- total memory size of above */
	long window_size;	/* OUTPUT	- size of window into the above memory */
	long font1;		/* OUTPUT	- pointer to font 1 */
	long font2;		/* OUTPUT	- pointer to font 2 */
	short H2DQoffset;	/* OUTPUT	- host to device queue offset address */
	short H2Dsize;		/* OUTPUT	- size of the above queue */
	short D2HQoffset;	/* OUTPUT	- device to host queue offset address */
	short D2Hsize;		/* OUTPUT	- size of the above queue */
};
 
struct qbases {
	short H2DQoffset;
	short D2HQoffset;
};
 
struct	test_param {
	int	n;
	int	to_mode;
	int	from_mode;
	int	board_offset;
	int	byte_cnt;
	int	pattern;
	int	terminate;
	int	status_addr;
};
 
struct test_status {
	int	pass_no;
	int	byte_no;
	int	written;
	int	read;
};
 
#ifndef  _IOWR
#define FIOC ('F'<<8)
#define _IOWR(x,y,z) (FIOC|y)
#define _IOW(x,y,z)  (FIOC|y)
#define _IOR(x,y,z)  (FIOC|y)
#define _IO(x,y)     (FIOC|y)
#endif	/* sysV68 */
#define FXINIT                  ('F' << 8 | 0)
#define FXGETADDR		_IOWR(F, 0, unsigned long)
#define FXGQBASES		_IOWR(F, 1, struct qbases )
#define FXDATABACK		_IOWR(F, 2, struct data_back )
#define FXGETFLAGS		_IOR (F, 3, unsigned long)
#define FXSETFLAGS		_IOW (F, 4, unsigned long)
#define FXINITFLG		0x0000
#define FXALIVEFLG		0x0001
#define FXOPENFLG		0x0002
#define FXSLEEPFLG		0x0004
#define FXWAITINGFLG		0x0008
#define FXBUSYFLG		0x0010
#define FXMEMDEVFLG 		0x0100
#define FXMEMDEV_FLG 		0x0100
#define FXEXCLUSIVEFLG		0x0200
#define FXXONFLG		0x0400
#define FXWSELFLG		0x0800
#define FXRSELFLG		0x1000
#define FXTIMEOUTFLG		0x4000
#define FXERRORFLG		0x8000
 
#define	FXQWRITE		_IOWR(F, 5, struct fxbuf )
#define	FXQREAD			_IOWR(F, 6, struct fxbuf )
#define	FXSEGWRITE		_IOWR(F, 7, struct fxbuf )
#define FXCLEANQ		_IO  (F, 8  )
#define	FXWAITMEM		_IO  (F, 9 )
#define FXEXCLUSIVE		_IOWR(F, 10, unsigned long )
#define	FXXON			_IOWR(F, 11, struct fxdata )
#define FXTIMEOUT		_IOW (F, 12, unsigned long )
#define	FXREADSIG		_IOW (F, 13, unsigned long )
#define FXINQVERDRV		_IOWR(F, 14, struct ver_struct)
#define FXINQVERCGI		_IOWR(F, 15, struct ver_struct)
#define FXINQVERX		_IOWR(F, 16, struct ver_struct)
#define FXINQVERVT100		_IOWR(F, 17, struct ver_struct)
	/* debuging support ioctls */
#define FXCOMM			_IO  (F, 102)
#define	FXREADTST		_IOWR(F, 101, struct fxbuf )
#define FXWAITEMPTY		_IOWR(F, 103, struct fxbuf)
#define FXSOFTRESET		_IOWR(F, 104, struct fxbuf)
#define FXSETDEBUG		_IOWR(F, 105, unsigned long)
#define FXMEMTEST		_IOWR(F, 106, struct test_param)
#define FXGETDEBUG		_IOWR(F, 107, unsigned long)
#define FXSETVEC		_IOWR(F, 108, unsigned long)
 
 
 
/* error values */
#define FXNO_ERR		 00
#define FXFATAL_ERR		-1	/* FATAL (Crash and Burn) ERROR */
#define FXCMD_ERR		-2	/* driver command error */
#define FXEXCMODE_ERR		-3	/* exclusive mode error */
#define FXXMODE_ERR		-4	/* X server mode error */
#define FXARG_ERR		-5	/* driver command argument error */
#define FXMODE_ERR		-6	/* wrong mode error */
#define FXWRITE_ERR		-7	/* error in write */
#define FXREAD_ERR		-8	/* error in read */
#define FXTIMEOUT_ERR		-9	/* timeout error */
#define	FXNOTIMPL_ERR		-10	/* not implemented yet */
