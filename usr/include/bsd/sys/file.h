#include "//usr/include/sys/file.h"
#include "//usr/include/sys/fcntl.h"

/*
 * Access call.
 */
#define	F_OK		0	/* does file exist */
#define	X_OK		1	/* is it executable by caller */
#define	W_OK		2	/* writable by caller */
#define	R_OK		4	/* readable by caller */

/*
 * Lseek call.
 */
#define	L_SET		0	/* absolute offset */
#define	L_INCR		1	/* relative to current offset */
#define	L_XTND		2	/* relative to end of file */

#ifndef mips
/* fcntl call */
/* only for sockets */
#define F_GETOWN        20      /* Get owner */
#define F_SETOWN        21      /* Set owner */
#endif
