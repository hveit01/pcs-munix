/*
 *      Header file for MUNIX/NET Task-to-Task Communication
 *
 *
 */

#define GETPTNO         0       /* get existing port number */
#define GETSETDYN       (-1)    /* get existing port number or
				   set it to a dynamic value */
#define SETDYN          (-2)    /* set port number to a dynamic value */

#define RECVPORT        1       /* port number of parent receiver */
#define SYNCPORT        2       /* used for dlupdate packets */
#define NOPORT          0       /* invalid port number */
#define ANYPORT         (-1)    /* accept packets from any port */
#define ANYADDR         ((ipnetaddr)-1)

#define FSERVER         1       /* indicates file server */
