/* @(#) uisw.h          1.1     Nov. 06 1986    */

/*	These symbolic definitions define the system and internal
 *      commands that are known to MUNIX/NET.
 */

#define UIISEXEC        -34     /* shared text via MUNIX/NET open call */
#define UIISIGNAL       -33     /* signal handling */
#define UIIIPC          -32     /* task-to-task communication */
#define UIIGUARD        -31     /* existence of process */
#define UIIWRITE	-30	/* write "core" */
#define UIICCLO		-29	/* close "core" */
#define UIICOPEN	-28	/* open "core" */
#define UIIEXECN	-27	/* internal exec for use by namei */
#define UIIEXEC		-26	/* internal exec for use by #! */
#define UIIGUID		-25	/* internal get uid/gid for a inode */
#define UIIQUIT		-22	/* urun quit */
#define UIIXRUN 	-21	/* urun cmd after processing */
#define UIIURUN 	-20	/* urun cmd */
#define UIICONNVERS     -17     /* connect command with demand for version */
#define UIIXWHO		-16    	/* uwho after processing */
#define UIIUWHO		-12    	/* uwho command */
#define UIIPASS		-11  	/* internal pass thru */
#define UIIDECR         -10     /* terminate remote receiver */
#define UIIINCR         -9      /* generate remote receiver */
#define UIIXONN		-8 	/* connect after processing */
#define UIIICLO		-5 	/* internal close */
#define UIIREAD		-4 	/* internal read (read(ip) */
#define UIIACCTW	-3 	/* acct write call */
#define UIICLOF		-2 	/* closef system call */
#define UIICONN		-1 	/* connect command */
#define UIICERB         0       /* CERBERUS broadcast packet callnumber */
#define UIIREM          1       /* throw away any connects */
#define UIIREMVERS      2       /* throw away any connects but save version */
#define UISREAD		3 	/* read system call */
#define UISWRITE	4 	/* write system call */
#define UISOPEN		5 	/* open system call */
#define UISCLOS		6 	/* close system call */
#define UISCREAT	8 	/* create system call */
#define UISLINK		9 	/* link system call */
#define UISULINK	10 	/* unlink system call */
#define UISDOIT		11	/* uidoit system call */
#define UISCHDIR	12 	/* chdir system call */
#define UISMKNOD	14 	/* mknod system call */
#define UISCHMOD	15 	/* chmod system call */
#define UISCHOWN	16 	/* chown system call */
#define UISSTAT		18 	/* stat system call */
#define UISSEEK		19 	/* seek system call */
#define UISFSTAT	28 	/* fstat system call */
#define UISUTIME	30 	/* utime system call */
#define UISACCES	33 	/* access system call */
#define UISDUP		41 	/* dup system call */
#define UISFCNTL	45 	/* fcntl system call */
#define UISSACCT	51 	/* sysacct system call */
#define UISIOCTL	54 	/* ioctl system call */
#define UISFEXEC        56      /* forkexec system call */
#define UISEXEC		59   	/* exec system call */
#define UISCHROT	61 	/* chroot system call */
#define UISLREAD	63 	/* lread system call */
#define UISLWRITE	64 	/* lwrite system call */
#define UISLOCKF	81 	/* lockf system call */
#define UISSYMLINK      83      /* symlink system call */
#define UISREADLINK     84      /* readlink system call */
#define UISLSTAT        85      /* lstat system call */
#define UISRMDIR	91	/* rmdir system call */
#define UISMKDIR	92	/* mkdir system call */
#define UISGETDENTS	97	/* getdents systemd call */
#define UISALLOTHER \
7: case 13: case 17: \
case 20: case 21: case 22: case 23: case 24: case 25: \
case 26: case 27: case 29: \
case 31: case 32: case 34: case 35: \
case 36: case 37: case 38: case 39: \
case 40: case 42: case 43: case 44: \
case 46: case 47: case 48: case 49: \
case 50: case 52: case 53: case 55: \
case 56: case 57: case 58: \
case 60: case 62: case 65: \
case 66: case 67: case 68: case 69: \
case 70: case 71: case 72: case 73: case 74: case 75: \
case 76: case 77: case 78: case 79: \
case 80: case 82: \
case 86: case 87: case 88: case 89: case 90: \
case 93: case 94: case 95: case 96
