/*pcs*/

#include <sys/types.h>
#include <sys/errno.h>

char **environ;
int errno;

static char ***execargs = (char***)0x3F7FEFFC;

static _trapjumpto0()
{
    0x4afc; /*illegal opcode */
}

/* init code for non-profiled programs */
_entry(args)
{
    short argc;
    char** argv;
    
    argv = (char**)&args;
    argc = *((short*)argv)++;

    *execargs = argv;
    environ = argv;

    while (*environ++ != 0);
    
    fp_init();
    
    main(argc, argv, environ);

    exit(0);
}
