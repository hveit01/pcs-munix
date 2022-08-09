/*
 * Access control:
 * A user does a call request. A call-req packet is assembled.
 * A `struct x25access' is filled with the users id and this packet.
 * The `x25demon' process reads /dev/svc0 and will get the `struct x25access'
 * from the kernel to make the decision.
 * He will examine `acc_uid' & `acc_callp', decide the case by
 * filling `acc_allow' and call the X25SETACCESS ioctl giving the
 * same `struct x25access'.
 */

struct x25access {
	ushort	acc_uid;
	ushort	acc_refnr;
	char	acc_callp[PACKETSIZE];		/* Kernel -> User only. */
	ushort	acc_len;			/* Length of callp. */
	short	acc_allow;			/* User -> Kernel only. */
};
	
