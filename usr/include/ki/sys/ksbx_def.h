
/*	SccsId	@(#)ksbx_def.h	1.2	7/27/88	*/

/*
#ident "@(#) RELEASE:  4.0  07/25/88  HEAD/SYS:ksbx_def";
*/
/*
Modifications
vers    when    who     what
1.1     010170  NN      changed xxx to yyy (Example)
4.0     072588  mar     Versionsanpassung
*/

/*
           ksbx - X.21 Kontroller  (physical adress table)
           -----------------------------------------------
*/


struct  madevice  {
        char  dreg,    dum1;            /* Datenregister            */
        char  dum2,    dum3;            /* verboten                 */
        char  dum4,    dum5;            /* verboten                 */
        char  creg,    dum7;            /* comand register          */
        char  dum8,    dum9;            /* verboten                 */
        char  dum10,   dum11;           /* verboten                 */
        char  dum12,   dum13;           /* verboten                 */
        char  dum14,   dum15;           /* verboten                 */
};
