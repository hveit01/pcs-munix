
extern char **environ;

int execl(path, arg0 /* ,arg1 , ..., argn, (char *) 0*/)
char *path, *arg0;
{
    return execve(path, &arg0, environ);
}
