
/*	SccsId	@(#)vme_def.h	1.2	7/27/88	*/

/*
#ident "@(#) RELEASE:  4.0  25/07/88  HEAD/SYS:vme_def";
*/
/*
Modifications
vers    when    who     what
1.1     010170  NN      changed xxx to yyy (Example)
4.0     072588  mar     Versionsanpassung
*/

/*
           vme - X.21 Adapter      (physical adress table)
           -----------------------------------------------
*/


struct  madevice  {
        char  ma_dum1;           /* not used                 */
        char  dreg;              /* Datenregister            */
        char  ma_dum[5];         /* not used                 */
        char  creg;              /* comand register          */
};
