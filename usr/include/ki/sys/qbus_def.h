
/*	SccsId	@(#)qbus_def.h	1.2	7/27/88	*/

/*
#ident "@(#) RELEASE:  4.0  07/25/87  HEAD/SYS:qbus_def";
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
        char  ma_dum1;         /* 1   not used                 */
        char  dreg;            /* 2   Datenregister            */
        char  ma_dum3;         /* 3   not used                 */
        char  k_cs;            /* 4   command status reg.      */
        char  ma_dum5;         /* 5   not used                 */
        char  k_fclear;        /* 6   hardwarereset FIO's      */
        char  ma_dum7;         /* 7   not used                 */
        char  creg;            /* 8   comand register          */
};
