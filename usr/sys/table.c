static char *_Version =  "@(#) RELEASE:  1.0  Mae 04 1988 /usr/sys/table.c ";
/*
Modifications
vers    when    who     what
1.1     040388  NN      changed xxx to yyy (Example)
*/
#include "sys/types.h"
#include "sys/conf.h"
#include "sys/fstyp.h"
#include "sys/nami.h"
#include "sys/stream.h"
#include "conf.h"

#ifdef STREAM
extern struct streamtab timinfo, trwinfo;
#ifdef STRTEST
extern struct streamtab lmrinfo, lmeinfo, lmtinfo, lmbinfo;
#endif

struct fmodsw fmodsw[] = {
	"timod",        &timinfo,
	"tirdwr",       &trwinfo,
#ifdef STRTEST
	"lmodr",        &lmrinfo,
	"lmode",        &lmeinfo,
	"lmodt",        &lmtinfo,
	"lmodb",        &lmbinfo,
#endif
};
int fmodcnt = sizeof(fmodsw)/sizeof(fmodsw[0]);
#endif STREAM

struct fsinfo fsinfo[] = {
	0, 0, "",     0,
	0, 0, "S51K", NO_SEEK,
#ifdef RFS
	FS_NOICACHE, 0, "DUFST", NO_SEEK,
#endif
#ifdef SOCKET
	FS_NOICACHE, 0, "socket", 0,
#endif
};

short nfstyp = sizeof(fsinfo)/sizeof(fsinfo[0]);

int fsnull(), fsstray();
int *fsistray();
struct inode *fsinstray();

int     s5init(), s5iput(), s5iupdat(), s5readi(), s5writei(),
	s5itrunc(), s5statf(), s5namei(), s5mount(), s5umount(),
	s5openi(), s5closei(), s5update(), s5statfs(), s5access(),
	s5getdents(), s5allocmap(), s5readmap(), s5setattr(),
	s5notify(), s5fcntl(), s5ioctl();
int     *s5freemap();
struct inode *s5iread(), *s5getinode();

#ifdef RFS
int     duinit(), duiput(), duiupdat(), dureadi(), duwritei(),
	duitrunc(), dustatf(), dumount(),
	duopeni(), duclosei(), duupdate(), duaccess(),
	dugetdents(),
	dunotify(), dufcntl(), duioctl();
int     *dufreemap();
struct inode *duiread();
#endif
#ifdef SOCKET
extern int soc_init(), soc_iput(), soc_iupdat(), soc_readi(),
	soc_writei(), soc_statf(), soc_closei(), soc_statfs(),
	soc_access(), soc_notify(), soc_fcntl(), soc_ioctl();
#endif

struct fstypsw fstypsw[] = {
{       fsstray,                /* 0  int             (*fs_init)();     */
	fsstray,                /* 1  int             (*fs_iput)();     */
	fsinstray,              /* 2  struct inode    *(*fs_iread)();   */
	fsstray,                /* 3  int             (*fs_filler)();   */
	fsstray,                /* 4  int             (*fs_iupdat)();   */
	fsstray,                /* 5  int             (*fs_readi)();    */
	fsstray,                /* 6  int             (*fs_writei)();   */
	fsstray,                /* 7  int             (*fs_itrunc)();   */
	fsstray,                /* 8  int             (*fs_statf)();    */
	fsstray,                /* 9  int             (*fs_namei)();    */
	fsstray,                /*10  int             (*fs_mount)();    */
	fsstray,                /*11  int             (*fs_umount)();   */
	fsinstray,              /*12  struct inode    *(*fs_getinode)();*/
	fsstray,                /*13  int             (*fs_openi)();    */
	fsstray,                /*14  int             (*fs_closei)();   */
	fsstray,                /*15  int             (*fs_update)();   */
	fsstray,                /*16  int             (*fs_statfs)();   */
	fsstray,                /*17  int             (*fs_access)();   */
	fsstray,                /*18  int             (*fs_getdents)(); */
	fsstray,                /*19  int             (*fs_allocmap)(); */
	fsistray,               /*20  int             *(*fs_freemap)(); */
	fsstray,                /*21  int             (*fs_readmap)();  */
	fsstray,                /*22  int             (*fs_setattr)();  */
	fsstray,                /*23  int             (*fs_notify)();   */
	fsstray,                /*24  int             (*fs_fcntl)();    */
	fsstray,                /*25  int             (*fs_fsinfo)();   */
	fsstray,                /*26  int             (*fs_ioctl)();    */
	fsstray,                /*27  int             (*fs_fill)();     */
	fsstray,                /*28  int             (*fs_fill)();     */
	fsstray,                /*29  int             (*fs_fill)();     */
	fsstray,                /*30  int             (*fs_fill)();     */
	fsstray,                /*31  int             (*fs_fill)();     */
},
{       s5init,                 /* 0  int             (*fs_init)();     */
	s5iput,                 /* 1  int             (*fs_iput)();     */
	s5iread,                /* 2  struct inode    *(*fs_iread)();   */
	fsstray,                /* 3  int             (*fs_filler)();   */
	s5iupdat,               /* 4  int             (*fs_iupdat)();   */
	s5readi,                /* 5  int             (*fs_readi)();    */
	s5writei,               /* 6  int             (*fs_writei)();   */
	s5itrunc,               /* 7  int             (*fs_itrunc)();   */
	s5statf,                /* 8  int             (*fs_statf)();    */
	s5namei,                /* 9  int             (*fs_namei)();    */
	s5mount,                /*10  int             (*fs_mount)();    */
	s5umount,               /*11  int             (*fs_umount)();   */
	s5getinode,             /*12  struct inode    *(*fs_getinode)();*/
	s5openi,                /*13  int             (*fs_openi)();    */
	s5closei,               /*14  int             (*fs_closei)();   */
	s5update,               /*15  int             (*fs_update)();   */
	s5statfs,               /*16  int             (*fs_statfs)();   */
	s5access,               /*17  int             (*fs_access)();   */
	s5getdents,             /*18  int             (*fs_getdents)(); */
	s5allocmap,             /*19  int             (*fs_allocmap)(); */
	s5freemap,              /*20  int             *(*fs_freemap)(); */
	s5readmap,              /*21  int             (*fs_readmap)();  */
	s5setattr,              /*22  int             (*fs_setattr)();  */
	s5notify,               /*23  int             (*fs_notify)();   */
	s5fcntl,                /*24  int             (*fs_fcntl)();    */
	fsstray,                /*25  int             (*fs_fsinfo)();   */
	s5ioctl,                /*26  int             (*fs_ioctl)();    */
	fsnull,                 /*27  int             (*fs_fill)();     */
	fsnull,                 /*28  int             (*fs_fill)();     */
	fsnull,                 /*29  int             (*fs_fill)();     */
	fsnull,                 /*30  int             (*fs_fill)();     */
	fsnull,                 /*31  int             (*fs_fill)();     */
},
#ifdef RFS
{       duinit,                 /* 0  int             (*fs_init)();     */
	duiput,                 /* 1  int             (*fs_iput)();     */
	duiread,                /* 2  struct inode    *(*fs_iread)();   */
	fsstray,                /* 3  int             (*fs_filler)();   */
	duiupdat,               /* 4  int             (*fs_iupdat)();   */
	dureadi,                /* 5  int             (*fs_readi)();    */
	duwritei,               /* 6  int             (*fs_writei)();   */
	duitrunc,               /* 7  int             (*fs_itrunc)();   */
	dustatf,                /* 8  int             (*fs_statf)();    */
	fsstray,                /* 9  int             (*fs_namei)();    */
	dumount,                /*10  int             (*fs_mount)();    */
	fsstray,                /*11  int             (*fs_umount)();   */
	fsinstray,              /*12  struct inode    *(*fs_getinode)();*/
	duopeni,                /*13  int             (*fs_openi)();    */
	duclosei,               /*14  int             (*fs_closei)();   */
	duupdate,               /*15  int             (*fs_update)();   */
	fsstray,                /*16  int             (*fs_statfs)();   */
	duaccess,               /*17  int             (*fs_access)();   */
	dugetdents,             /*18  int             (*fs_getdents)(); */
	fsstray,                /*19  int             (*fs_allocmap)(); */
	dufreemap,              /*20  int             *(*fs_freemap)(); */
	fsstray,                /*21  int             (*fs_readmap)();  */
	fsstray,                /*22  int             (*fs_setattr)();  */
	dunotify,               /*23  int             (*fs_notify)();   */
	dufcntl,                /*24  int             (*fs_fcntl)();    */
	fsstray,                /*25  int             (*fs_fsinfo)();   */
	duioctl,                /*26  int             (*fs_ioctl)();    */
	fsnull,                 /*27  int             (*fs_fill)();     */
	fsnull,                 /*28  int             (*fs_fill)();     */
	fsnull,                 /*29  int             (*fs_fill)();     */
	fsnull,                 /*30  int             (*fs_fill)();     */
	fsnull,                 /*31  int             (*fs_fill)();     */
},
#endif
#ifdef SOCKET
{       soc_init,               /* 0  int             (*fs_init)();     */
	soc_iput,               /* 1  int             (*fs_iput)();     */
	fsinstray,              /* 2  struct inode    *(*fs_iread)();   */
	fsstray,                /* 3  int             (*fs_filler)();   */
	soc_iupdat,             /* 4  int             (*fs_iupdat)();   */
	soc_readi,              /* 5  int             (*fs_readi)();    */
	soc_writei,             /* 6  int             (*fs_writei)();   */
	fsstray,                /* 7  int             (*fs_itrunc)();   */
	soc_statf,              /* 8  int             (*fs_statf)();    */
	fsstray,                /* 9  int             (*fs_namei)();    */
	fsstray,                /*10  int             (*fs_mount)();    */
	fsstray,                /*11  int             (*fs_umount)();   */
	fsinstray,              /*12  struct inode    *(*fs_getinode)();*/
	fsstray,                /*13  int             (*fs_openi)();    */
	soc_closei,             /*14  int             (*fs_closei)();   */
	fsstray,                /*15  int             (*fs_update)();   */
	soc_statfs,             /*16  int             (*fs_statfs)();   */
	soc_access,             /*17  int             (*fs_access)();   */
	fsstray,                /*18  int             (*fs_getdents)(); */
	fsstray,                /*19  int             (*fs_allocmap)(); */
	fsistray,               /*20  int             *(*fs_freemap)(); */
	fsstray,                /*21  int             (*fs_readmap)();  */
	fsstray,                /*22  int             (*fs_setattr)();  */
	soc_notify,             /*23  int             (*fs_notify)();   */
	soc_fcntl,              /*24  int             (*fs_fcntl)();    */
	fsstray,                /*25  int             (*fs_fsinfo)();   */
	soc_ioctl,              /*26  int             (*fs_ioctl)();    */
	fsnull,                 /*27  int             (*fs_fill)();     */
	fsnull,                 /*28  int             (*fs_fill)();     */
	fsnull,                 /*29  int             (*fs_fill)();     */
	fsnull,                 /*30  int             (*fs_fill)();     */
	fsnull,                 /*31  int             (*fs_fill)();     */
},
#endif
};
