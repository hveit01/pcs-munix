/* includes for all used ports */

/* the ports for the various queues */

extern  PORT    ArpLancePort;
extern  PORT    LanceArpPort;
extern  PORT    IpArpPort;
extern  PORT    LanceIpPort;
extern  PORT    IpIcmpPort;
#define TcpIcmpPort IpIcmpPort
#define UdpIcmpPort IpIcmpPort
extern  PORT    TcpIpPort;
#define UdpIpPort TcpIpPort
extern  PORT    IpTcpPort;
extern  PORT    IpUdpPort;
extern  PORT    L5TcpPort;
extern  PORT    L5UdpPort;
extern  PORT    HostPort;
extern  PORT    TcpL5Port;
#define UdpL5Port TcpL5Port
#define ProtL5Port TcpL5Port

extern  PORT    LancePort;
extern  PORT    ArpPort;
extern  PORT    IpPort;
extern  PORT    TcpUdpPort;
extern  PORT    L5Port;

extern  PORT    HostIccPort;

extern  PORT    HostProtPort;
extern  PORT    ProtHostPort;

extern  PORT    LogPort;
