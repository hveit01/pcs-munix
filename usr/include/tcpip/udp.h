/****************************************************************
*                       U D P                                   *
*                                                               *
* The follwing include file describes the structure of a UDP    *
* packet.                                                       *
*                                                               *
* change history :                                              *
*       17/03/86 jh     original version                        *
****************************************************************/

struct udp_packet {
	Ushort  udp_src_port;   /* source port number of packet */
	Ushort  udp_dest_port;  /* dest port number of packet */
	Ushort  udp_length;     /* length of data */
	Ushort  udp_checksum;   /* errors should not occur, but... */
	/* here the data should follow */
	char    udp_data;

};
#ifdef ON_ICC
struct udp_port                 /* port management stuff */
{
	Ushort    port;           /* the port we are waiting for */
	Ushort    conn;           /* local connection name */
}; /* struct udp_port */
#endif
