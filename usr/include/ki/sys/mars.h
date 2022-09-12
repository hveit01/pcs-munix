
/*	SccsId	@(#)mars.h	1.2	7/26/88	*/

/*
#ident "@(#) RELEASE:  4.0  25/07/88  HEAD/SYS:mars";
*/
/*
Modifications
vers    when    who     what
1.1     010170  NN      changed xxx to yyy (Example)
2.0     180388  mar     flags in mb_bsm wurden geaendert
4.0     250788  mar     versions Nr. wurde hochgezaehlt
*/


/*
          MARS - minor device struct  (for Mars minor dev. table)
          -------------------------------------------------------
*/


struct  mindev  {
        int          *ptr_dev;      /* pointer to devices tty  */
        int          *ptr_dex;      /* devices extended data   */
        struct  mbl  *ptr_mbl;      /* pointer to devices mbl  */
        struct  mkv  *ptr_mkv;      /* pointer to devices mkv  */
        char    ipar[8];            /* Initparameter           */
        unchar  ra;                 /* routing adr.            */
        unchar  da;                 /* destination adr.        */
        unchar  ka;                 /* kanal adr.              */
};














/*
          MARS - major device struct  (for MARS - major device table)
          -----------------------------------------------------------
*/


struct  majdev  {
        struct  mindev  *minptr;    /* pointer to minor dev. table */
        ushort  m_major;            /* eigene major #              */
        ushort  m_maxmin;           /* maximale Anzahl minor dev's */
        int     (*m_senauf) ();     /* funktionpointer             */
        int     (*m_posqui) ();     /*  "     "     "              */
        int     (*m_negqui) ();     /*  "     "     "              */
        int     (*m_empfd)  ();     /*  "     "     "              */
        int     (*m_empfc)  ();     /*  "     "     "              */
        int     (*m_error)  ();     /*  "     "     "              */
};
PAGE
/*
        define Mars - Block (mbl) structure and other KIDAN PARM's
        ----------------------------------------------------------
*/



#ifdef QKA
#define KIDMAJMIN  58            /* kleinste moegliche KIDAN major # */
#define KIDMAJMAX  59            /* groesste moegliche KIDAN major # */
#else
#define KIDMAJMIN  20            /* kleinste moegliche KIDAN major # */
#define KIDMAJMAX  29            /* groesste moegliche KIDAN major # */
#endif




#define DGRCNT 123
#define RA     00
#define FC     01
#define DA     02
#define SA     03
#define SS     04
#define KA     04
PAGE
struct  mbl  {

        short   mb_ident[2];    /* mbl identification               */
        ushort  mb_min;         /* eigene minor dev #               */
        ushort  mb_maj;         /* eigene major dev #               */
        ushort  mb_dev;         /* eigene device #                  */
        ushort  mb_m;           /* zugehoerige mkv #                */
        char    mb_squ;         /* squitt                           */
        short   mb_p2;          /* mkv parameter fuer erneut srq    */
        short   mb_sta;         /* mkv - Fehlermeldungen            */
        ushort  mb_call;        /* call merker                      */
        ushort  mb_bsm;         /* Betriebsmode                     */

        /*   - - - - - - - - - -  sende part  - - - - - - - - - -   */

        char    mb_sframe[5];   /* sende frame
                                    0    1    2    3    4
                                   sra  sfc  sda  ssa  sss          */

        short   mb_bvs;         /* Merker: BLV (senden)             */
        short   mb_spb;         /* Merker: Speicher belegt          */

        /*   - - - - - - - - -  empfangs part  - - - - - - - - -    */

        char    mb_eframe[5];   /* empfangs frame                     
                                    0    1    2    3    4
                                   era  efc  eda  esa  ess          */

        short   mb_ecc;         /* empfangs character count         */

        short   mb_bve;         /* Merker: BLV (empfang)            */
        
        /* ----------------- miscellaneous ------------------------ */

        char    mb_ipa[8];      /* aktuelle Initparameter           */
        short   mb_lon;         /* Merker: 'Link ON'                */
        short   mb_ini;         /* Merker: 'Init'                   */

        /* ------------------- wird nur vom mkv behandelt --------- */

        ushort  mb_sstat;       /* dgr Sendestatus                  */
                                /*    00 -->                        */

        ushort  mb_estat;       /* dgr Empfangsstatus               */
        short   mb_anm;         /* -1 = angemeldet, 0 = nicht angem.*/
       
        /*   Statistikzaehler einbauen                              */
};
PAGE
/* Betriebsmode  (mb_bsm) */
/* ====================== */

/* user bsm-flag's */

#define X21MOD     0x0001   /* if set, line works in PAT Mode        */
#define PONINF     0x0002   /* if set, Power OFF control is enabled  */

/*-------------------------------------------------------------------*/

/* driver bsm-flag's */

#define NOBVE      0x0100   /* if set, keine BVE - Behandlung        */
#define REFLECT    0x0200   /* if set, X.21 - Spiegelmode            */
