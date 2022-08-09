/* PCS MC68901 MFP register definitions */
/* hv20211020 */

/* the MC68901 MFP is word sized, but registers
 * are accessto on the odd bytes only */
struct mfpregs {
     unsigned char _0,  gdpr;
#define MFPGP_GPI7  0x80
#define MFPGP_GPI6  0x40
#define MFPGP_GPI5  0x20
#define MFPGP_GPI4  0x10
#define MFPGP_GPI3  0x08
#define MFPGP_GPI2  0x04
#define MFPGP_GPI1  0x02
#define MFPGP_GPI0  0x01
     unsigned char _2,  aer;
#define MFPAE_GPI7  0x80
#define MFPAE_GPI6  0x40
#define MFPAE_GPI5  0x20
#define MFPAE_GPI4  0x10
#define MFPAE_GPI3  0x08
#define MFPAE_GPI2  0x04
#define MFPAE_GPI1  0x02
#define MFPAE_GPI0  0x01
     unsigned char _4,  ddr;
#define MFPDD_GPI7  0x80
#define MFPDD_GPI6  0x40
#define MFPDD_GPI5  0x20
#define MFPDD_GPI4  0x10
#define MFPDD_GPI3  0x08
#define MFPDD_GPI2  0x04
#define MFPDD_GPI1  0x02
#define MFPDD_GPI0  0x01
     unsigned char _6,  iera;
#define MFPIA_GPI7  0x80
#define MFPIA_GPI6  0x40
#define MFPIA_TMRA  0x20
#define MFPIA_RBF   0x10
#define MFPIA_RERR  0x08
#define MFPIA_TBE   0x04
#define MFPIA_TERR  0x02
#define MFPIA_TMRB  0x01
     unsigned char _8,  ierb;
#define MFPIB_GPI5  0x80
#define MFPIB_GPI4  0x40
#define MFPIB_TMRC  0x20
#define MFPIB_TMRD  0x10
#define MFPIB_GPI3  0x08
#define MFPIB_GPI2  0x04
#define MFPIB_GPI1  0x02
#define MFPIB_GPI0  0x01
     unsigned char _a,  ipra;
#define MFPPA_GPI7  0x80
#define MFPPA_GPI6  0x40
#define MFPPA_TMRA  0x20
#define MFPPA_RBF   0x10
#define MFPPA_RERR  0x08
#define MFPPA_TBE   0x04
#define MFPPA_TERR  0x02
#define MFPPA_TMRB  0x01
     unsigned char _c,  iprb;
#define MFPPB_GPI5  0x80
#define MFPPB_GPI4  0x40
#define MFPPB_TMRC  0x20
#define MFPPB_TMRD  0x10
#define MFPPB_GPI3  0x08
#define MFPPB_GPI2  0x04
#define MFPPB_GPI1  0x02
#define MFPPB_GPI0  0x01
     unsigned char _e,  isra;
#define MFPSA_GPI7  0x80
#define MFPSA_GPI6  0x40
#define MFPSA_TMRA  0x20
#define MFPSA_RBF   0x10
#define MFPSA_RERR  0x08
#define MFPSA_TBE   0x04
#define MFPSA_TERR  0x02
#define MFPSA_TMRB  0x01
     unsigned char _10, isrb;
#define MFPSB_GPI5  0x80
#define MFPSB_GPI4  0x40
#define MFPSB_TMRC  0x20
#define MFPSB_TMRD  0x10
#define MFPSB_GPI3  0x08
#define MFPSB_GPI2  0x04
#define MFPSB_GPI1  0x02
#define MFPSB_GPI0  0x01
     unsigned char _12, imra;
#define MFPMA_GPI7  0x80
#define MFPMA_GPI6  0x40
#define MFPMA_TMRA  0x20
#define MFPMA_RBF   0x10
#define MFPMA_RERR  0x08
#define MFPMA_TBE   0x04
#define MFPMA_TERR  0x02
#define MFPMA_TMRB  0x01
     unsigned char _14, imrb;
#define MFPMB_GPI5  0x80
#define MFPMB_GPI4  0x40
#define MFPMB_TMRC  0x20
#define MFPMB_TMRD  0x10
#define MFPMB_GPI3  0x08
#define MFPMB_GPI2  0x04
#define MFPMB_GPI1  0x02
#define MFPMB_GPI0  0x01
     unsigned char _16, vr;
#define MFPVR_VEC   0xf0 /* bit7-4 of the vector, mask*/
#define MFPVR_IV    0x0f /* channel# of highest prio channel */
#define  MFPIV_GPI7 0x0f
#define  MFPIV_GPI6 0x0e
#define  MFPIV_TMRA 0x0d
#define  MFPIV_RBF  0x0c
#define  MFPIV_RERR 0x0b
#define  MFPIV_TBE  0x0a
#define  MFPIV_TERR 0x09
#define  MFPIV_TMRB 0x08
#define  MFPIV_GPI5 0x07
#define  MFPIV_GPI4 0x06
#define  MFPIV_TMRC 0x05
#define  MFPIV_TMRD 0x04
#define  MFPIV_GPI3 0x03
#define  MFPIV_GPI2 0x02
#define  MFPIV_GPI1 0x01
#define  MFPIV_GPI0 0x00
     unsigned char _18, tacr;
#define MFPTA_RST   0x10
#define MFPTA_AC    0x0f /*mask*/
#define  MFPTA_STOP 0x00
#define  MFPAC_D4   0x01
#define  MFPAC_D10  0x02
#define  MFPAC_D16  0x03
#define  MFPAC_D50  0x04
#define  MFPAC_D64  0x05
#define  MFPAC_D100 0x06
#define  MFPAC_D200 0x07
#define  MFPAC_ECM  0x08
#define  MFPAC_P4   0x09
#define  MFPAC_P10  0x0a
#define  MFPAC_P16  0x0b
#define  MFPAC_P50  0x0c
#define  MFPAC_P64  0x0d
#define  MFPAC_P100 0x0e
#define  MFPAC_P200 0x0f
     unsigned char _1a, tbcr;
#define MFPTB_RST   0x10
#define MFPTB_AC    0x0f /*mask*/
#define  MFPTB_STOP 0x00
#define  MFPBC_D4   0x01
#define  MFPBC_D10  0x02
#define  MFPBC_D16  0x03
#define  MFPBC_D50  0x04
#define  MFPBC_D64  0x05
#define  MFPBC_D100 0x06
#define  MFPBC_D200 0x07
#define  MFPBC_ECM  0x08
#define  MFPBC_P4   0x09
#define  MFPBC_P10  0x0a
#define  MFPBC_P16  0x0b
#define  MFPBC_P50  0x0c
#define  MFPBC_P64  0x0d
#define  MFPBC_P100 0x0e
#define  MFPBC_P200 0x0f
     unsigned char _1c, tcdcr;
#define MFPTC_CC    0x70 /*mask*/
#define  MFPCC_STOP 0x00
#define  MFPCC_D4   0x10
#define  MFPCC_D10  0x20
#define  MFPCC_D16  0x30
#define  MFPCC_D50  0x40
#define  MFPCC_D64  0x50
#define  MFPCC_D100 0x60
#define  MFPCC_D200 0x70
#define MFPTD_DC    0x07 /*mask*/
#define  MFPTD_STOP 0x00
#define  MFPTD_D4   0x01
#define  MFPTD_D10  0x02
#define  MFPTD_D16  0x03
#define  MFPTD_D50  0x04
#define  MFPTD_D64  0x05
#define  MFPTD_D100 0x06
#define  MFPTD_D200 0x07
     unsigned char _1e, tadr; /* 8 bit value */
     unsigned char _20, tbdr; /* 8 bit value */
     unsigned char _22, tcdr; /* 8 bit value */
     unsigned char _24, tddr; /* 8 bit value */
     unsigned char _26, scr; /* 8 bit value */
     unsigned char _28, ucr;
#define MFPUC_CLK   0x80
#define MFPUC_CL    0x60 /*mask*/
#define  MFPUC_CL8  0x00
#define  MFPUC_CL7  0x20
#define  MFPUC_CL6  0x40
#define  MFPUC_CL5  0x60
#define MFPUC_ST    0x18 /*mask*/
#define  MFPUC_STS  0x00
#define  MFPUC_ST1  0x08
#define  MFPUC_ST15 0x10
#define  MFPUC_ST2  0x18
#define MFPUC_PE    0x04
#define MFPUC_PEVN  0x02
     unsigned char _2a, rsr;
#define MFPRS_BF    0x80
#define MFPRS_OE    0x40
#define MFPRS_PE    0x20
#define MFPRS_FE    0x10
#define MFPRS_FSOB  0x08
#define MFPRS_MOCP  0x04
#define MFPRS_SS    0x02
#define MFPRS_RE    0x01
     unsigned char _2c, tsr;
#define MFPTS_BE    0x80
#define MFPTS_UE    0x40
#define MFPTS_AT    0x20
#define MFPTS_END   0x10
#define MFPTS_B     0x08
#define MFPTS_H     0x04
#define MFPTS_L     0x02
#define MFPTS_TE    0x01
     unsigned char _2e, udr; /* 8 bit value */
};

extern struct mfpregs _mfp[];
