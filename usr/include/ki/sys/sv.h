
/*	SccsId	@(#)sv.h	1.2	7/27/88	*/

/*
#ident "@(#) RELEASE:  4.0  07/25/88  HEAD/SYS:sv";
*/
/*
Modifications
vers    when    who     what
1.1     010170  NN      changed xxx to yyy (Example)
4.0     072588  mar     Versionsanpassung
*/

#define PAGE
/*
           server (sv)  data - structure
           -----------------------------
*/


struct  sv   {
        short  sv_ident;        /* sv identification                */
        short  sv_min;          /* eigene minor device #            */
        short  sv_state;        /* sv - status  (siehe t_tstate)    */

        int    sv_timo_id;      /* timeout ID                       */

        char   *sv_bufadr;      /* saved buffer address             */

        ushort sv_nschar;       /* # of sende chars. in dgr         */
        char   sv_sbuf[123];    /* sende buffer                     */

        ushort sv_nechar;       /* # of empf. chars. in dgr         */
        char   sv_ebuf[123];    /* empfangs buffer                  */

        ushort sv_send_dgr;     /* lokales Control (dgr-ende)       */

        short  sv_dgrcnt;       /* DGR char.count (max.123, min.1)  */


        short  sv_senerl;       /* Merker: Sendeerlaubnis bzw. DGROUT
                                      0 --> keine Sen.erl. erst srq.
                                      1 --> SRQ an mkv gesendet
                                      2 --> senden erlaubt   
                                      3 --> warten auf SQUITT        */

        short  sv_ernsrq;       /* flg fuer erneute Sendeaufforderung
                                      0x01 --> srequest fuer daten 
                                      0x02 --> srequest fuer control */

        ushort sv_saktiv;       /* dgr senden aktiv
                                      0x01 daten senden aktiv
                                      0x02 --> control senden aktiv  */

        char   sv_ponip;        /* Merker: pon in progress          */

        ushort sv_scl;          /* sende Control length in bytes    */
        char   sv_scbuf[8];     /* Sende - Control buffer           */
        char   sv_ecbuf[8];     /* control empfangs buffer          */

        int    (*sv_senp)();    /* funktion pointer SQUITT          */
        int    sv_scase;        /* sub pointer SEN                  */

        int    (*sv_empp)();    /* funktion pointer empfang         */
        int    sv_ecase;        /* sub pointer empfang              */

        int    (*sv_serlp)();   /* funktion pointer senerl.         */
        int    sv_serlcase;     /* sub pointer                      */
};




/*  Commands:
    --------- */


#define SVCNTR ('a' << 8)
#define ADAB   (SVCNTR | 0)     /* Adressabfrage                    */
#define ADRUM  (SVCNTR | 1)     /* Adressumschaltung                */
PAGE
/*
        ioctl - Systemcall
        ------------------



Form 1:   ioctl (fildes, command, arg)
======    struct tnadr *arg;


          commands:   ADAB     Adressabfrage

                      ADRUM    Adressumschaltung
PAGE
Form 2:   ioctl (fildes, command, arg)
======    int arg;


*/
