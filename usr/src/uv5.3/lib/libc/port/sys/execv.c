
extern char **environ;

int execv (path, argv)
char *path, *argv[];
{
	return execve(path, argv, environ);
}
