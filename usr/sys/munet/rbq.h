
/* PCS lan buffer queue */

/* for some obscure reason this structure is missing from /usr/include/sys/* */
struct rbq {
    ushort r_bufid;             /* buffer id */
    caddr_t r_pkptr;           /* payload */
    struct rbq *r_next;         /* link to next item */
};
