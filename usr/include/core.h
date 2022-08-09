/* machine dependent stuff for core files */
#define TXTRNDSIZ 4096L
#define stacktop(siz) (0x3f7ff000)
#define stackbas(siz) (0x3f7ff000-siz)
