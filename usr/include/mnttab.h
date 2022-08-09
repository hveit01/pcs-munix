#ifndef	NMOUNT
#define	NMOUNT	40
#endif

/* Format of the /etc/mnttab file which is set by the mount(1m)
 * command
 */
/* CADMUS added next two sizes so programs dont use absolutes... */
#define	SIZE_mt_dev	25
#define	SIZE_mt_filsys	50

struct mnttab {
	char	mt_dev[ SIZE_mt_dev ],
		mt_filsys[ SIZE_mt_filsys ];
		short	mt_ro_flg;
	time_t	mt_time;
};
