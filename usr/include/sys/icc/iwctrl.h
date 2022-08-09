#define	IW_FORMAT	(('S'<<8)|0x4)
#define IW_REASSIGN	(('S'<<8)|0x7)
#define	IW_MODESEL	(('S'<<8)|0x15)
#define IW_FORM_RDCAP	(('S'<<8)|0x24)
#define IW_RDCAPACITY	(('S'<<8)|0x25)
#define IW_MODESENSE	(('S'<<8)|0x1A)
#define IW_READBBL	(('S'<<8)|0x1D)
#define IW_READGDL	(('S'<<8)|0x37)
#define	IW_SOFTERR	(('S'<<8)|0x70)
#define	IW_DISCON	(('S'<<8)|0x71)
#define	IW_CTRLRNAME	(('S'<<8)|0x72)
#define	IW_ERRREPORT	(('S'<<8)|0x73)
#define	IW_ERRCLEAR	(('S'<<8)|0x74)

#define NBLK 124
#define MODESENSELENGTH 130

union iw_iocb {
struct {
	ushort	lun;			/* dummy */
	ushort	block_size;		/* 512 or 1024 */
	ushort	num_cyls;		/* number of cylinders */
	byte	num_heads;		/* number of data heads */
	byte	num_secs;		/* number of sectors per track */
	ushort 	rwc_cyl;		/* reduced write current cyl */
	ushort	wrt_precomp_cyl;
	byte	num_alt_cyls;		/* number of alternative cyls */
	byte	flag;
	byte	level;
	byte 	num_defects;

	/* hd_cyl is combined as hi 4 bits = head, low 12 bits = cyl */
	union {
		struct { ushort hd_cyl, bytes_f_index; } defects[NBLK];
		struct { ulong block, foo; } blocks[NBLK];
		struct { ulong l; } l[NBLK];
	} d;
} a;
struct { ushort	i0, i1, i2, i3, i4, i5, i6, i7, i8, i9; } i;
struct { byte 	b1, b0, b3, b2, b5, b4, b7, b6, b9, b8; } b;
struct { ulong	l0, l1, l2, l3; } l;
byte bb[4*NBLK];
};
