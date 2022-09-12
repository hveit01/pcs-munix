
extern char **environ;

int forkexecv(path, argv)
char *argv[];
{
	return forkexecve(path, argv, environ);
}
