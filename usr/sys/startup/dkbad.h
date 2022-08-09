/*PCS MODIFIED FROM MUNIX 1.x */
/*      dkbad.h

 * Definitions needed to perform bad sector
 * revectoring ala DEC STD 144.

 * The bad sector information is located in the
 * first 5 even numbered blocks of the last track
 * of the disk pack. There are five identical
 * copies of the information, described by the
 * dkbad structure.

 * Replacement sectors are allocated starting with
 * the first sector before the bad sector information
 * and working backwards towards the beginning of the
 * disk. A maximum of 126 bad sectors are supported.
 * The position of the bad sector in the bad sector
 * table determines which replacement sector it
 * corresponds to.

 * The bad sector information and replacement sectors
 * are conventionally only accessible through the 'c'
 * file system partition of the disk. If that partition
 * is used for a file system, the user is responsible
 * for making sure that it does not overlap the bad
 * sector information or any replacement sectors.
 */

#define NBAD    126     /* max. number of bad sectors */
#define BSFSZ   512     /* size of bad sector file (in bytes) */

struct dkbad {
    long    bt_csn;                 /* cartridge serial number */
    short   bt_nbad;                /* number of known bad sectors */
    short   bt_flag;                /*PCS -1 => alignment cartridge */
    long    bt_sec[NBAD];           /* numbers of bad sectors */
};
    /* driver states needed for bad sector forwarding */
#define GOOD    1       /* I/O with good sectors (initial state) */
#define RBSF    2       /* read bad sector file from disk */
#define BAD     3       /* I/O with bad sector(s) */
