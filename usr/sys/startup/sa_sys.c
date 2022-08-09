/*PCS specific*/
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/dir.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/signal.h>
#include <sys/inode.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/icc/isctrl.h>
#include <sys/mtio.h>
#include "sa.h"

static char *_Version = "@(#) RELEASE:  2.1  May 20 1987 /usr/sys/startup/sa_sys.c ";

extern int chkopen(), chkio(), chkftrk(), cemufmt();
extern iwformat();
extern swctrl1();
extern  swctrl2();

/* corresponds to bdevsw */
struct sadevsw sa_devsw[] = {
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*rm*/
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*rl*/
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*rx2*/
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*tm*/
	{ "hk",	0x0b,	0,		3,		4,		chkopen,chkio,	chkftrk,cemufmt	},	/*hk*/
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*rk*/
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*hp*/
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*td*/
	{ "st", 0x11,	1,		0,		0,		0,		0,		0,		0 		},	/*rmt*/
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*hl*/
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*ot*/
	{ "??", 0,		0,		4,		6,		0,		0,		0,		0 		},	/*sasi*/
	{ "hk2",0x14,	0,		3,		4,		chkopen,chkio,	chkftrk,cemufmt	},	/*hk2*/
	{ "ts",	0x1c,	1,		0,		0,		0,		0,		0,		0		},	/*ts*/
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*ras_ml*/
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*fd*/
	{ "??", 0,		0,		0,		0,		0,		0,		0,		0 		},	/*eagle*/
	{ "iw", 0x2d,	0,		4,		2,		iwformat,0,		0,		0 		},	/*iw*/
	{ "if", 0x2e,	0,		0,		1,		0,		0,		0,		0 		},	/*if*/
	{ "sw", 0x36,	0,		4,		2,		swctrl1,0,		0,		0 		},	/*sw*/
	{ "sw2",0x37,	0,		4,		2,		swctrl2,0,		0,		0 		},	/*sw2*/
	{ "is",	0x2f,	1,		0,		0,		0,		0,		0,		0 		},	/*not bdev */
	{ "iq",	0x2f,	1,		0,		0,		0,		0,		0,		0 		},	/*not bdev */
	{ 0,	0,		0,		0,		0,		0,		0,		0,		0 		}	/*end of list */
};

long sa_offset[32];	/* file offsets for 32 inodes */
int inode_set = 0;	/* use bits for inode[] */
int sa_tapefile;

static struct mtop mtop;
static short iscmd;

extern struct inode inode[]; /* 32 inodes */
extern int tbf[];


char cap(ch)
char ch;
{
	if (ch >= 'a' && ch <= 'z')
		return ((int)ch) - 0x20;
	else
		return ch;
}

static setusermmu()
{
}

int static getino()
{
	int i;
	int bit;
	
	if (inode_set == -1)		/* is full */
		return -1;
	
	i = 0; bit = 1;
	while (inode_set & bit) {
		bit = bit << 1;
		i, i++;					/* don't know why, but compiler makes code for this */
	}

	inode_set |= bit;
	return i;
}

sa_getdev(cp)
char *cp;
{
	int dev;
	int i, k;
	int unit;

	for (i = 0; cp[i] && cp[i] != '('; i++);	/* find end of dev name */
	if (cp[i] != '(' || i==0)
		return -1;

	for (dev = 0; sa_devsw[dev].name != 0; dev++) {
		for (k = 0; k < i && cap(cp[k]) == cap(sa_devsw[dev].name[k]); k++);
		if (k == i) break;
	}

	if (sa_devsw[dev].name == 0 || sa_devsw[dev].istape >= 2)
		return -1;

	i++;
	unit = 0;
	if (cp[i] < '0' || cp[i] > '9')
		return -1;
	for (; cp[i] >= '0' && cp[i] <= '9'; i++)
		unit = unit * 10 + cp[i] - '0';

	if (cp[i] == ',') {					/* is dev(unit, file#) */
		i++;
		k = 0;
		if (cp[i] < '0' || cp[i] > '9')
			return -1;
		for (; cp[i] >= '0' && cp[i] <= '9'; i++)
			k = k * 10 + cp[i] - '0';
		sa_tapefile = k;				/* set file */
		if (!sa_devsw[dev].istape) {	/* not a streamer/tape? */
			if ((1 << sa_devsw[dev].npar) <= k)	/* assume partition, chosen too high? */
				return -1;
			unit = k + (unit << sa_devsw[dev].npar);
		}
	} else
		sa_tapefile = -1;

	if (cp[i] != ')')
		return -1;
	
	dev = makedev(dev, unit);
	return dev;
}

sa_skip(fd, nskip)
int fd;
{
	int unused;
	int rc;
	int devmaj;
	
	if (fd < 0)
		return -1;

	devmaj = major(inode[fd].i_dev);
	if (nskip > 0) {
	
		switch (devmaj) {
		case 21:	/* IS device */
		case 22:	/* IQ device */
			iscmd = IS_NODISCON;
			sa_ioctl(fd, IS_IOCTL, &iscmd);
			iscmd = devmaj == 21 ? IS_QIC24 : IS_QIC11_9;
			sa_ioctl(fd, IS_IOCTL, &iscmd);
			iscmd = IS_SPACE;
			while (nskip-- > 0) {
				rc = sa_ioctl(fd, IS_IOCTL, &iscmd);
				if (rc == -1)
					return -1;
			}
			return 0;

		case 8:		/* ST device */
			rc = 0;
			
			while (nskip-- > 0) {
				while ((rc = sa_read(fd, tbf, 10240)) > 0);
				if (rc == -1)
					return -1;
			}
			return 0;

		case 13:	/* TS device */
			mtop.mt_op = MTREW;
			mtop.mt_count = 0;
			sa_ioctl(fd, MTIOCTOP, &mtop);
			
			mtop.mt_op = MTFSF;
			mtop.mt_count = nskip;
			return sa_ioctl(fd, MTIOCTOP, &mtop);

		default:
			return -1;
		}	
	}
	return 0;
}

int sa_open(devname, mode)
char *devname;
int mode;
{
	struct inode *ip;
	int var36;
	int dev;
	int fd;
	int retries;
	
	dev = sa_getdev(devname);
	if (dev < 0 || (fd = getino()) == -1)
		return -1;

	u.u_error = 0;
	if (sa_devsw[major(dev)].istape == 1) {
		if (sa_tapefile == -1)
			return -1;

		retries = 0;
		do {
			u.u_error = 0;
			(*cdevsw[sa_devsw[major(dev)].cdev].d_open)(minor(dev));
		} while (u.u_error && retries++ < 2);
	} else
		(*bdevsw[major(dev)].d_open)(minor(dev));
	
	if (u.u_error)
		return -1;

	ip = &inode[fd];
	ip->i_dev = dev;
	ip->i_number = 0;
	ip->i_flag = 0;
	ip->i_addr[NADDR] = 0;
	ip->i_count = 1;
	ip->i_mode = IFBLK|ILAND|ISYN;
	ip->i_nlink = 0;
	ip->i_size = 0;
	ip->i_uid = 0;
	ip->i_gid = 0;
	ip->i_addr[NADDR] = 0;
	ip->i_rdev = (sa_devsw[major(dev)].istape==0 ? Fs2BLK : 0) | dev;
	sa_offset[fd] = 0;
	if (sa_devsw[major(dev)].istape == 1) {
		if (sa_skip(fd, sa_tapefile) == -1) {
			sa_close(fd);
			return -1;
		}
	}
	sa_offset[fd] = 0;
	return fd;
}

sa_close(fd)
int fd;
{
	struct inode *ip;
	int dummy;
	int dev;
	int rc;
	
	if (fd < 0)
		return -1;

	inode_set &= ~(1<<fd);
	ip = &inode[fd];
	dev = ip->i_dev;
	ip->i_flag = 0;
	ip->i_number = 0;
	ip->i_count = 0;
	if (sa_devsw[major(dev)].istape == 1)
		(*cdevsw[sa_devsw[major(dev)].cdev].d_close)(minor(dev));

	rc = 0;
	if (u.u_error)
		rc = -1;
	
	bflush(-1);
	binval(dev);
	return rc;
}

sa_lseek(fd, off, whence)
int fd;
long off;
int whence;
{
	if (fd < 0)
		return -1;

	if (whence == 2)		/* no seek from end allowed */
		return -1;
		
	if (whence == 0)
		sa_offset[fd] = off;
	else
		sa_offset[fd] += off;
	
	return sa_offset[fd];
}

int sa_transfer(fd, buf, sz, rw, isvirt)
int fd;
caddr_t buf;
int sz;
int rw;
int isvirt; /*buffer is virtual addr */
{
	int rc;

	u.u_error = 0;
	u.u_count = sz;
	if (sa_devsw[major(inode[fd].i_dev)].istape == 1)
		isvirt = 1;
	
	if (isvirt)
		u.u_base = &buf[-SYSVA];	/* poor man's logtophys */
	else
		u.u_base = buf;

	u.u_offset = sa_offset[fd];
	u.u_segflg = 1;
	if (isvirt == 0) {
		if (rw)
			writei(&inode[fd]);
		else
			readi(&inode[fd]);
	} else {
		if (rw)
			(*cdevsw[sa_devsw[major(inode[fd].i_dev)].cdev].d_write)(minor(inode[fd].i_dev));
		else
			(*cdevsw[sa_devsw[major(inode[fd].i_dev)].cdev].d_read)(minor(inode[fd].i_dev));
	}
	if (u.u_error)
		rc = -1;
	else {
		sa_offset[fd] += sz - u.u_count;
		rc = sz - u.u_count;
	}
	return rc;
}

int sa_read(fd, buf, sz)
{
	return sa_transfer(fd, buf, sz, 0, 0);
}

int sa_write(fd, buf, sz)
{
	return sa_transfer(fd, buf, sz, 1, 0);
}

int sa_ioctl(fd, cmd, arg)
int fd;
int cmd;
caddr_t arg;
{
	if (fd < 0)
		return -1;

	u.u_error = 0;
	arg = &arg[-SYSVA];
	(*cdevsw[sa_devsw[major(inode[fd].i_dev)].cdev].d_ioctl)
		(minor(inode[fd].i_dev), cmd, arg, 3);
		
	if (u.u_error)
		return -1;
	else
		return 0;
}

int sa_getc(fd)
int fd;
{
	char ch;
	int rc;
	
	if (sa_devsw[major(inode[fd].i_dev)].istape == 1)
		return -1;
	
	rc = read(fd, &ch, 1);
	if (rc == -1)
		return -1;
	else
		return ch & 0xff;
}

int sa_getw(fd)
int fd;
{
	register int ch; 
	register uint k;
	register char *bp;
	short buf;

	for (k = 0, buf = 0, bp = (char*)&buf; k < 2; k++) {
		ch = sa_getc(fd);
		if (ch < 0) {
			if (k == 0)
				return -1;
			else 
				return buf;
		} else
			*bp++ = ch;
	}
	return buf;
}

int sa_getl(fd)
int fd;
{
	register int ch; 
	register uint k;
	register char *bp;
	int buf;

	for (k = 0, buf = 0, bp = (char*)&buf; k < 4; k++) {
		ch = sa_getc(fd);
		if (ch < 0) {
			if (k == 0)
				return -1;
			else 
				return buf;
		} else
			*bp++ = ch;
	}
	return buf;
}

char sa_line[80];

sa_main()
{
	register reg_t *rp;
	register preg_t *prp;
	register pte_t *pt;
	register int i;
	
	if ((rp = allocreg(0, 1)) == 0 || (prp = attachreg(rp, &u, 0, 2, 3)) == 0)
		panic("aunix: cannot allocate region");
	if (growreg(&u, prp, 256, 0, 6) < 0)
		panic("aunix: cannot grow region");

	pt = rp->r_list[0];
	for (i = 0; i < 256; i++)
		pt[i].pgi.pg_pte = i | (PG_V|PG_PROT);

	regrele(rp);

	printf("coldstart (check, mkfs, restor) (y/n) :");
	gets(sa_line);
	if (sa_line[0] == 'j' || sa_line[0] == 'y') {	/* 'j' = 'ja': this is a German UNIX :-) */
		do {
			printf("\tcheck  = c\n");
			printf("\tmkfs   = m\n");
			printf("\trestor = r\n");
			printf("\txd     = x\n");
			printf("\tquit   = q\n");
			printf("select function: ");
			gets(sa_line);
			switch (sa_line[0]) {
			case 'X':
			case 'x':
				xd();
				break;
			case 'R':
			case 'r':
				restor();
				break;
			case 'M':
			case 'm':
				mkfs();
				break;
			case 'C':
			case 'c':
				diskcheck();
				break;
			case 'S':					/* undocumented */
			case 's':
				sweeprom();
				break;
			case 'T':					/* undocumented */
			case 't':
				sa_swbadfix();
				break;
			default:
				break;
			}
		} while (sa_line[0] != 'q');

		printf("Start UNIX (y/n) : ");
		gets(sa_line);
		if (sa_line[0] != 'j' && sa_line[0] != 'y')
			prom_warm();
	}

	/* start unix */
	delay(4);

	inoinit();
	u.u_error = 0;

	reglock(rp);
	for (i = 0; i < 256; i++)
		pt[i].pgi.pg_pte = 0;
	detachreg(&u, prp);

	clratb();
}

checkmenu()
{
	int i, k;
	for (i = k = 0; sa_devsw[i].name; i++) {
		if (sa_devsw[i].istape == 0 && sa_devsw[i].open) {
			printf("\t%s", sa_devsw[i].name);
			if (k++ & 1)
				printf("\n");
		}
	}
	printf("\n");
}

int checkgap(fd)
int fd;
{
	int dev = major(inode[fd].i_dev);
	return sa_devsw[dev].gap;
}

ltol3(tgt, src, n)
register char *tgt, *src;
int n;
{
	register int i;
	for (i = 0; i < n; i++) {
		src++;
		*tgt++ = *src++;
		*tgt++ = *src++;
		*tgt++ = *src++;
	}
}

l3tol(tgt, src, n)
register char *tgt, *src;
int n;
{
	register int i;
	for (i = 0; i < n; i++) {
		*tgt++ = 0;
		*tgt++ = *src++;
		*tgt++ = *src++;
		*tgt++ = *src++;
	}
}

int atol(cp)
register char *cp;
{
	register char *cp1;
	register int ch;
	register int base = 10;
	int val;
	int sign = 1;
	
	if (*cp == '-') {
		cp++;
		sign = 0;
	}
	if (*cp == '+')
		cp++;

	if (*cp == 'x') {
		cp++;
		base = 16;
	} else if (*cp == '0' && *(cp+1) == 'x') {
		cp += 2;
		base = 16;
	} else if (*cp == '0') {
		base = 8;
	}
	for (cp1 = cp; *cp1; ) {
		if (*cp1++ == '.')
			base = 10;
	}

	for (val = 0; *cp != '\0'; cp++) {
		ch = *cp;
		if (ch >= '0' && ch <= '9') {
			val = val * base - (ch - '0');
		} else if (ch >= 'a' && ch <= 'f' && base == 16) {
			val = val * base - (ch - '7'-0x20);
		} else break;
	}
	return sign ? -val : val;
}
