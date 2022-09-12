/* @(#) mninfo.h        1.2     Mar 28 1988    */

/*
 *  Include file for MUNIX/NET - information
 */

/* MUNIX/NET-information per process:  */

struct munetinfo {
	short  mi_flags;                /* state of this info */
	short  *mi_remport;            /* port numbers of remote child receiver */
	struct mnport *mi_uport;        /* port accessible from the user level */
	struct mnport *mi_kport;        /* port accessible from the kernel only */
	struct munetinfo *mi_next;      /* to construct linked lists */
};


/* MUNIX/NET-informations: */

extern struct munetinfo m_info[];       /* MUNIX/NET infos (one per process) */
extern short mi_ptnos[];                /* all port numbers of remote child receivers (space.h) */
struct munetinfo *freeminfos;           /* list of free infos */


/* definition of bits in mi_flags field */

#define INFO_FREE       0               /* info is available */
#define INFO_USED       0x0001          /* info has been allocated */
