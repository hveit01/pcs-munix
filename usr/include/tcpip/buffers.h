/****************************************************************
*                       b u f f e r s . h                       *
*                                                               *
* This module offers an interface to a buffer management for    *
* UNIX_MESSAGES that are requested by various ICC-device-       *
* drivers (TCP-IP- software e.g.).                              *
*                                                               *
* Furthermore there are a few buffers of about 1KByte which are *
* used to transfer data between the host and an ICC. The device *
* drivers using those buffers are responsible for not overwri-  *
* ting the linkage-portion of such a buffer.                    *
*                                                               *
* Version 1.0   02 Mai 86       j&j                             *
*       changes:                                                *
****************************************************************/

/* some size definitions */

#define NR_UNIX_MESSAGES        32      /* pool size for UNIX_MESSAGEs */
#define NR_KBUFFERS             16      /* pool size for 'k'ernel bufs */
#define KBUF_SIZE               1520    /* data portion of kbuffers */


#ifndef BUFFERS_C
extern UNIX_MESSAGE *Request_msg();     /* get a msg, eventually sleep */
extern void Release_msg();              /* release a UNIX_MESSAGE, wakeup */
#endif

struct kbuffer {                      /* structure of communication buffers */
	char    data[KBUF_SIZE];      /* used between host and icc */
	struct kbuffer *link;
}; /* struct kbuffer */

#ifndef BUFFER_C
extern struct kbuffer *Req_kbuffer();   /* same as above, only for a */
extern void Rel_kbuffer();              /* different structure */
#endif
