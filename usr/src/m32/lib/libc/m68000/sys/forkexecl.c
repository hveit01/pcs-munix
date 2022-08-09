
extern char **environ;

int forkexecl(path, /*variable */args)
char *path;
{
    return forkexecve(path, &args, environ);
}
