/* @(#)utsname.h	6.1 */

#ifndef _ENETADDR
#define _ENETADDR
typedef struct {unsigned short hi,mi,lo;} enetaddr; /* Ethernet address */
typedef unsigned long ipnetaddr;                    /* IP address */
#endif

struct utsname {
	char	sysname[9];
	char	nodename[9];
	char	release[9];
	char	version[9];
	enetaddr uiname;
	unsigned short  serialnum;
	ipnetaddr ipname;
};
extern struct utsname utsname;

struct  ethname {
	enetaddr ethaddr;
	short    stnaddr;
};
