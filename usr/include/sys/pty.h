#ifndef TIOC
#include "sys/termio.h"
#endif

#define  TCPTYDUMPSL     (TIOC | 64)    /* dump slave tty structure */
#define  TCPTYISSLOPEN   (TIOC | 65)    /* is slave site open       */
#define  TCPTYSETNEWMODE (TIOC | 66)    /* set new pty mode         */
#define  TCPTYRESETNEWMODE (TIOC | 67)  /* reset new pty mode       */

/* the New-mode uses t_tmflag for communication                     */
#define  TMPTYNEW        1              /* New-flag i t_tmflag          */
#define  TMPTYIOCTLSL    2              /* ioctl happend on slave site  */
#define  TMPSCLOSE       4              /* close happend on slave site  */

#ifdef SELECT
#define  TMPTYWSEL       4              /* write select on ptc           */
#define  TMPTYRSEL       8              /* read  select on ptc           */
#endif
