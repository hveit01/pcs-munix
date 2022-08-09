#define IS_IOCTL        (('I'<<8)|'S')

#define	IS_REWIND	0x01
#define IS_SPACE	0x11
#define IS_ERASE        0x19
#define IS_MODESENSE    0x1A
#define IS_RETEN        0x1B
#define IS_RESET        0x21
#define IS_QIC11_4      0x22
#define IS_QIC11_9      0x23
#define IS_QIC24        0x24
#define IS_SWAP         0x25
#define IS_NOSWAP       0x26
#define IS_TOEND        0x27
#define IS_DISCON       0x28
#define IS_NODISCON     0x29
#define IS_ERRREPORT	0x2A
#define IS_ERRCLEAR	0x2B

/* example :
   short op = IS_REWIND; ioctl(fd, IS_IOCTL, &op); */
