/*PCS*/

/* declarations for startup library */

/* this corresponds to the list order of bdevsw */
struct sadevsw {
    char *name;     /* device name, e.g. hk, iq, st */
    short cdev;     /* corresponding idx into cdevsw */
    short istape;   /* is a tape/streamer device */
    short npar;     /* # of partitions */
    short gap;      /* returned by checkgap() */
    int (*open)();
#define DEV_READ 1
#define DEV_WRITE 2
#define DEV_WCHK 3
#define DEV_TRKFMT 22
    int (*doio)();
    int (*trkformat)();
    int (*format)();
};

extern struct sadevsw sa_devsw[];

struct upar {
    short unit;     /* unit # */
    char *name;     /* name of disk, e.g. rl, xt */
    int nsecs;      /* total # of sectors */
    short secsz;    /* sector size */
    short sectrk;   /* sectors per track */
    short trkcyl;   /* # of tracks per cylinder */
    short ncyl;     /* # of cylinders */

#define UERR_READ   1
#define UERR_WRITE  2
#define UERR_WCHECK 3
#define UERR_MANUAL 4
#define UERR_COMPARE 5
    short reason;
};

#define FILL_PATTERN 0xeb6db6db

struct cpar {
    short off0;
#define RDWRITE 0
#define RDONLY 1
    char rdonly;
    char setbad;
    char off4;
    char off5;
    int fill;
    int off10;
};

