/****************************************************************
*                  P A C K E T                                  *
*                                                               *
* Packet Management stuff to request and release data packets   *
****************************************************************/

#ifdef USEPROTECT
/* we use packet protection */
#define Protect_pack(pack)      Protect(pack, sizeof(struct packet))
#define Unprotect_pack(pack)    Unprotect(pack, sizeof(struct packet))
#endif


#define MIN_PACKETS     5       /* if we have less than this number of */
				/* packets and we still buffer udp packets */
				/* then we will destroy some udp datagrams */

struct packet *Get_packet();    /* request a packet, waits */
void Free_packet();             /* release packet for other use */
