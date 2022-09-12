
/*	SccsId	@(#)maioctl.h	1.2	7/27/88	*/

/*
#ident "@(#) RELEASE:  4.0  07/25/88 INC/SYS:maioctl";
*/
/*
Modifications
vers    when    who     what
1.1     010170  NN      changed xxx to yyy (Example)
2.0     031888  mar     <tnflg> wurde eingefuehrt.
4.0     072588  mar     Versionsanpassung
*/

#define PAGE

/*
          structure fuer  MARS - teilnehmer - adresse    (tnadr)
          ------------------------------------------------------
*/


struct  tnadr   {
        dev_t    tndevnr;             /* major, minor                */
        char     tnipar[8];           /* TN - Initparameter          */
        unchar   tnra;                /* TN - routing adr.           */
        unchar   tnda;                /* TN - destination adr.       */
        unchar   tnka;                /* TN - kanal adr.             */
        unchar   tnd1;                /* dumy                        */
        int      tnflg;               /* Kanal-Betriebsmode flag     */
};




/*
<tnflg> -  Definitions   (see: mbl struct <mb_bsm>)
                         (user bsm - flag's) 
*/

/*
#define X21MOD 0x1         if set, line works in PAT Mode       
#define PONINF 0x2         if set, Power OFF control is enabled   
*/






/* Commands */

#define TNCMD ('M' << 8)
#define TNADR     (TNCMD | 0)        /* change TN-Adr. + Init. Par. */
#define SPIEGEL   (TNCMD | 1)        /* Spiegelmode einschalten     */
#define UNSPIEGEL (TNCMD | 2)        /* Spiegelmode ausschalten     */
#define SPAUSE    (TNCMD | 3)        /* Sendepause                  */
#define DMBL      (TNCMD | 4)        /* display mars - block        */
#define DTTY      (TNCMD | 5)        /* display tty - structure     */
#define DTTYEX    (TNCMD | 6)        /* display ttyex - structure   */
#define DTRACE    (TNCMD | 7)        /* output global Trace buffer  */
#define CDBSV     (TNCMD | 8)        /* change SERVER debug flag    */
#define CDBMKV    (TNCMD | 9)        /* change MKV    debug flag    */
#define CDBKIT    (TNCMD | 10)       /* change TTY    debug flag    */
#define DTRALL    (TNCMD | 11)       /* display Tracebuffers (all)  */
PAGE
/*
        ioctl - Systemcall
        ------------------



Form 1:   ioctl (fildes, command, arg)
======    struct tnadr *arg;


          commands:   TNADR    Einstellen der Geraete Zieladresse und
                               Uebergabe von Initalparametern (nur
                               ueber den Serverkanal '/dev/ttysv' 
                               moeglich).
                               Die Initialparameter werden nur ueber-
                               nommen, wenn  tnipar[0] = 1  ist,
                               andernfalls wird nach termio
                               initialisiert.
PAGE
Form 2:   ioctl (fildes, command, arg)
======    struct mbl *arg;


          commands:   DMBL     Display  MARS - Block !

                               Im ersten Wort des mbl's "mb_ident[1]"
                               muss die entsprechende major/minor #
                               desjenigen KIDAN - Geraetes angegeben
                               werden, dessen 'mbl' angezeigt werden
                               soll.
PAGE
Form 3:   ioctl (fildes, command, arg)
======    struct tty *arg;


          commands:   DTTY     Display  Devices tty - struct !

                               Im Wort t_cflag der tty-structure
                               muss die entsprechende major #, im Wort
                               t_lflag die ensprechende minor #
                               desjenigen KIDAN - Geraetes angegeben
                               werden, dessen 'tty-struct' angezeigt 
                               werden soll.
PAGE
Form 4:   ioctl (fildes, command, arg)
======    struct ttyex *arg;


          commands:   DTTYEX   Display  Devices ttyex - struct !

                               Im Wort ex_scnt der ttyex-structure
                               muss die entsprechende major #, im Wort
                               ex_stime die entsprechende minor #
                               desjenigen KIDAN - Geraetes angegeben
                               werden, dessen 'ttyex-struct' angezeigt 
                               werden soll.
PAGE
Form 5:   ioctl (fildes, command, arg)
======    int arg;


          commands: SPIEGEL    Der Spiegelmode kann nur ueber den
                               Serverkanal '/dev/ttysv' eingestellt
                               werden.
                               In 'arg' ist die 'major #, minor #'
                               desjenigen Geraetes zu hinterlegen,
                               das in den Spiegelmode geschalten 
                               werden soll.

                    UNSPIEGEL  Rueckschalten vom Spiegelmode in den
                               Normalmode.
                               Es gelten die gleichen Bedingungen wie
                               unter 'SPIEGEL'.

                    SPAUSE     Defaultmaessig wird beim Senden von
                               Zeichen zum Geraet vom tty-treiber
                               alle 80 Zeichen eine Sendepause von
                               26.7 msec eingelegt.
                               Mit diesem Command (auf den entsprech-
                               enden tty-kanal) kann die o.e. Sende-
                               pause verlaengert, gekuerzt, oder ganz
                               ausgeschalten werden.
                               arg = 0000  -->  keine Sendepause.
                               arg = 0xYYYY  --> alle 80 Zeichen wird
                               eine Sendepause von:

                                        0xYYYY * 833 mikrosec.
                               
                               eingelegt.

                               min. Sendepause:
                               0x0001 * 833 mikrosec. = 833 mikrosec.

                               max. Sendepause:
                               0xffff * 833 mikrosec. = 54,6 sec.



                     DTRACE
                     CDBSV
                     CDBMKV
                     CDBKIT
*/
