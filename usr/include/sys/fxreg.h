 
	/********	device code masks  ***************
	/*   7 6 5 4 3 2 1 0
	/*  +-+-+-+-+-+-+-+-+
	/*  |       | unit|S|
	/*  +-+-+-+-+-+-+-+-+
	/*  unit: fox board unit # 0 - 8
	/*  S:	 select call on vt100 or graphics?
	/********************************************/
#ifdef sysV68
#define minor(dev)		( dev & 0xff )		/* fx minor number */
#endif
#define FXUNIT(dev)	( minor(dev) >> 1 )	/* fx unit number */
#define FXISGRAPH(dev)	( dev & 1 )		/* true if fx is graphics device */
											/* false if fx is tty device */
#ifndef	mips
#define volatile
#endif	/* mips */
 
struct fxdevice {
	volatile short	graphics_interrupt;
#define	HIC_CGI_DATA_WRITTEN	1	/* i have given you cgi data */
/* #define	HIC_VT_DATA_WRITTEN		2	/*OLD i have given you vt100 data */
#define NEW_CURSOR_DEFINITION	3
#define INTERRUPT_PROCESSED		4
#define NEW_CURSOR_POSITION		5
#define GRAPHICS_REQ_INTERUPT	6
#define SOFT_RESET				7
/* #define	HIC_VT_ENABLE			8	/*OLD enable vt100, and interrupt me */
	volatile short	fx_interrupt;
#define	FIC_CGI_DATA_WRITTEN	1	/* cgi data in choq */
#define	FIC_KB_DATA_WRITTEN	2	/* keyboard data in vhoq */
#define	FIC_RETRIEVE_PIX_DATA	3	/* retrieve pixel data in choq */
#define	FIC_VT_ENABLE_SEEN	8	/* vt100 enabled */
#define FIC_REQUESTED_INTERUPT	0x10

#ifdef ULTRIX

/* #define FX_PAGES	((64*1024 + 64*1024)/NBPG) */
#define FX_PAGES	((1024*1024)/NBPG)
#define FX_BASE		0x200000
#define FX_INCR 	0x40000
#ifdef ULTRIX_1
#define FX_ADDR(i)	(umem[0] + FX_BASE + i*FX_INCR)
#else  /* ULTRIX_1 */
#define FX_ADDR(i)	(qmem[0] + FX_BASE + i*FX_INCR)
#endif  /* ULTRIX_1 */
#define FX_VECTOR(i)	(0520 + i*20)

#endif /* ULTIRX */
 
	volatile unsigned short	filler1;
	volatile unsigned short	chiq_read_ptr;
	volatile unsigned short	filler2;
	volatile unsigned short	chiq_write_ptr;
	volatile unsigned short	filler3;
	volatile unsigned short	chiq_size;
 
	volatile unsigned short	filler4;
	volatile unsigned short	choq_read_ptr;
	volatile unsigned short	filler5;
	volatile unsigned short	choq_write_ptr;
	volatile unsigned short	filler6;
	volatile unsigned short	choq_size;
 
	/* out queue is output from board */
	volatile unsigned short	filler7;
	volatile unsigned short	vhoq_read_ptr;
	/* ie. keyboard input to host */
	volatile unsigned short	filler8;
	volatile unsigned short	vhoq_write_ptr;
	volatile unsigned short	filler9;
	volatile unsigned short	vhoq_size;
 
	/* in queue is input to board */
	volatile unsigned short	filler10;
	volatile unsigned short	vhiq_read_ptr;
	/* ie. alpha output to board/screen */
	volatile unsigned short	filler11;
	volatile unsigned short	vhiq_write_ptr;
	volatile unsigned short	filler12;
	volatile unsigned short	vhiq_size;
	volatile unsigned short	filler13;
	volatile unsigned short	base_of_chiq;
	volatile unsigned short	filler14;
	volatile unsigned short	base_of_vhiq;
	volatile unsigned short	filler15;
	volatile unsigned short	base_of_choq;
	volatile unsigned short	filler16;
	volatile unsigned short	base_of_vhoq;
	volatile unsigned short	vt100_interrupt;
#define	VT_DATA_WRITTEN		2	/* vt100 data writen into queue to board */
#define VT_REQ_INTR			8	/* vt100 request interupt */
 
	volatile unsigned short	reserved0;
	volatile unsigned short	*x_current_cursor;
	volatile unsigned short	*x_requested_cursor;
	volatile unsigned short	cursor_x_position;
	volatile unsigned short	cursor_y_position;
};
 
struct fx_queue {
	volatile unsigned short	base;
	volatile unsigned short	end;
	volatile unsigned short	*read_ptr;
	volatile unsigned short	*write_ptr;
	volatile unsigned int	size;
};
 
struct csr {
	unsigned short status;
	unsigned char  ctl_big_endian;
	unsigned char  ctl_little_endian;
	unsigned char  filler[254];
	unsigned short mailbox;
};
#ifdef	BIG_ENDIAN
#  define ctl     ctl_big_endian
#else	/* BIG_ENDIAN */
#  define ctl     ctl_little_endian
#endif	/* BIG_ENDIAN */

struct fxchn {
	volatile unsigned char 		*base;	/* bases will always be the same*/
	volatile struct csr 		*csr;
#ifdef	BIG_ENDIAN
#define FXREADY 	0x2000
#else	/* BIG_ENDIAN */
#define FXREADY 	0x20
#endif	/* BIG_ENDIAN */
#define HOSTREADY 	0x20
#define FXINTERUPT	0x80 | HOSTREADY
	volatile struct fxdevice *cap;
	struct fx_queue 	chiq;
	struct fx_queue	choq;
	struct fx_queue 	vhiq;
	struct fx_queue	vhoq;
};

struct fx_softc {
    struct fxchn dev;
    struct proc *u_procp;
    struct proc *sel_procp;
    ulong selcolision;
#define BUFSZ	(1024*2)
    short cgibuf[BUFSZ];
    ulong timeout;
    ulong flags;
    ulong read_signal;
    ulong writelast, readlast, cntlast;
    ulong cgicnt, ioctlcnt, busycnt;
    ulong expect, event_flag;
    struct {
	short cmdcode;
	short graphics;
	short vt100;
	short vt100nod;
    } vec;
};
