/* MUNIX/NET now knows only about the Internet Address of the various
	Cadmus machines that it wants to talk to, therefore to talk
	to them it must use the Address Resolution Protocol (ARP)
	to determine their Ethernet addresses. */
struct uarp_pkt {
	unsigned short arp_hrd;		/* Ethernet */
	unsigned short arp_pro;		/* IP */
	unsigned char arp_hln;		/* sizeof (Ether-addr) */
	unsigned char arp_pln;		/* sizeof (IP-addr) */
	unsigned short arp_op;		/* the operation that is desired */
	enetaddr arp_sha;		/* source Ether addr */
	unsigned long arp_spa;		/* source IP addr */
	enetaddr arp_tha;		/* target Ether addr */
	unsigned long arp_tpa;		/* target IP addr */
	};

#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2
#define ARP_HRD_E 0x0800
#define ARP_PRO_IP 0x0806
#define ARP_LEN_E 0x06
#define ARP_LEN_IP 0x04
