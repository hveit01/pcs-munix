
/*	SccsId	@(#)ttyex.h	1.2	7/27/88	*/

/*
#ident "@(#) RELEASE:  4.0  07/25/88 INC/SYS:ttyex";
*/
/*
Modifications
vers    when    who     what
1.1     010170  NN      changed xxx to yyy (Example)
4.0     072588  mar     lokal Trace Buffer jetzt 100 gross.
                        Ueberschreibung von 'ex_send_dgr'
                        wird verhindert.
*/

/*
          KIDAN extended data for tty's
          -----------------------------
*/


#define LTRACE  100            /* local trace buffer length          */

struct  ttyex   {

        ushort  ex_send_dgr;   /* lokales Control (dgr-ende)         */

        short   ex_dgrcnt;     /* DGR char.count (max.123, min.1)    */

        short   ex_scnt;       /* Sendezeichenzaehler fuer S-pause.  */
        short   ex_stime;      /* timer fuer Sendepause.             */
                               /* 0000 = keine Sendepause            */
                               /* xxxx = mal 833 mikrosec. S.pause   */
                               /* nach <ex_scnt> Zeichen.            */

        short   ex_tpdev;      /* Parameter switch fuer Control -
                                  fortsetzungsroutine:
                                     0 = use parameter 'dev' in call
                                    -1 = use  "     "  'tp'  in call
                               */

        short   ex_senerl;     /* Merker: Sendeerlaubnis bzw. DGROUT
                                    0 --> keine Sen.erl. erst srq.
                                    1 --> SRQ an mkv gesendet
                                    2 --> senden erlaubt   
                                    3 --> warten auf SQUITT        */

        short   ex_ernsrq;     /* flg fuer erneute Sendeaufforderung
                                    0x01 --> srequest fuer daten 
                                    0x02 --> srequest fuer control */

        ushort  ex_saktiv;     /* dgr senden aktiv
                                    0x01 daten senden aktiv
                                    0x02 --> control senden aktiv  */

        int     ex_devstat;    /* device status
                                    0x1000 -> PERROR
                                    0x4000 -> OVERRUN
                                    0x2000 -> FRERROR              */

        int     ex_tzb1[2];    /* temporaerer Zeichenbuffer        */

        int     ex_timo_id;    /* timout ID                        */

        char    ex_tzb[6];     /* temporaerer Zeichenbuffer        */
PAGE
        char    ex_ponip;      /* Merker: pon in progress          */

        char    ex_beidbv;     /* Merker: beidseitig BV            */

        ushort  ex_scl;        /* sende Control length in bytes    */
        char    ex_scbuf[8];   /* Sende - Control buffer           */
        char    ex_ecbuf[8];   /* control empfangs buffer          */

        int     (*ex_senp)();  /* funktion pointer SQUITT          */
        int     ex_scase;      /* sub pointer SEN                  */

        int     (*ex_empp)();  /* funktion pointer empfang         */
        int     ex_ecase;      /* sub pointer empfang              */

        int     (*ex_serlp)(); /* funktion pointer senerl.         */
        int     ex_serlcase;   /* sub pointer                      */

        ushort  ex_ltc;             /* local trace count           */
        ushort  ex_ltb[LTRACE+2];   /* local trace buffer          */
};








/* device status   <ex_devstat> */
/* ---------------------------- */


/*
#define  PERROR   0x1000     >
#define  FRERROR  0x2000      >  defined in tty.h
#define  OVERROR  0x4000     >
*/
#define  PONWAIT  0x1        /* wait Power ON 9008                 */
#define  WRACT    0x2        /* Syst. Call 'WRITE' active          */
#define  RDACT    0x4        /* Syst. Call 'READ'  active          */
PAGE
/* define  'KIDAN tty - Power ON/OFF - Table */

struct  kit_pontab  {
        ushort  kit_major;
        ushort  kit_minor;
        ushort  kit_flag;
};
