/*pcs*/

#include <sys/types.h>
#include <sys/errno.h>

char **environ;
int errno;
char *_countbase;
char **___Argv;

extern caddr_t etext, end;

static char ***execargs = (char***)0x3F7FEFFC;

static _trapjumpto0()
{
    0x4afc; /*illegal opcode */
}

/* init code for profiled programs */
_entry(args)
{
    char *profbuf;
    unsigned int profsz;
    short argc;
    char** argv;
    
    argv = (char**)&args;
    argc = *((short*)argv)++;

    *execargs = argv;
    environ = argv;

    while (*environ++ != 0);

    profsz = 
        (((int)((caddr_t)&etext - (caddr_t)_entry + 7) >> 3) << 1) + 2412;
    profbuf = (char*)&end;
    
    if (brk((caddr_t)&end + profsz) == -1) {
        write(2, "cannot profile- no core\n", 24);
        _exit(1);
    }
    _countbase = profbuf+12;
    
    fp_init();
    ___Argv = argv;

    /*monitor (lowpc, highpc, buffer, bufsize, nfunc)*/
    monitor(_entry, &etext, profbuf, profsz / 2, 300);
    
    main(argc, argv, environ);

    exit(0);
}

exit(code)
{
    _cleanup();
    monitor(0);
    _exit(code);
}
