/* @(#)conf.h	6.1 */
/*
 * Declaration of block device switch. Each entry (row) is
 * the only link between the main unix code and the driver.
 * The initialization of the device switches is in the file conf.c.
 */
extern struct ldevsw
{
      int     (*lan_init)();
      int     (*lan_getbuf)();
      int     (*lan_sizbuf)();
      int     (*lan_relbuf)();
      int     (*lan_transmit)();
      int     (*lan_monitor)();
      int     (*lan_xname)();
      int     lan_error;
      ushort  lan_bufid;
} ldevsw[];

struct bdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_strategy)();
/*UH    int     (*d_print)();   ?? */
};
extern struct bdevsw bdevsw[];

/*
 * Character device switch.
 */
struct cdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_ioctl)();
#ifdef SELECT
	int     (*d_sel)();
#endif
	struct tty *d_ttys;
	struct streamtab *d_str;
};
extern struct cdevsw cdevsw[], conssw[];

#define	FMNAMESZ	8

struct fmodsw {
	char	f_name[FMNAMESZ+1];
	struct  streamtab *f_str;
};
extern struct fmodsw fmodsw[];

extern int	bdevcnt;
extern int	cdevcnt;
extern int	fmodcnt;

/*
 * Line discipline switch.
 */
struct linesw {
	int	(*l_open)();
	int	(*l_close)();
	int	(*l_read)();
	int	(*l_write)();
	int	(*l_ioctl)();
	int	(*l_input)();
	int	(*l_output)();
	int	(*l_mdmint)();
};
extern struct linesw linesw[];

extern int	linecnt;
/*
 * Terminal switch
 */
struct termsw {
	int	(*t_input)();
	int	(*t_output)();
	int	(*t_ioctl)();
};
extern struct termsw termsw[];

extern int	termcnt;
