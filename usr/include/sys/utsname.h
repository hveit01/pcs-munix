/* @(#)utsname.h	6.1 */
struct utsname {
	char	sysname[9];
	char	nodename[9];
	char	release[9];
	char	version[9];
	enetaddr uiname;
	ushort	serialnum;
	ipnetaddr ipname;
};
extern struct utsname utsname;

struct  ethname {
	enetaddr ethaddr;
	short    stnaddr;
};
