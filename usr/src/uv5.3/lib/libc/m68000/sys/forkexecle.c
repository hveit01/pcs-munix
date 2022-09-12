
int forkexecle(path, argp)
char *path;
char *argp[];
{
	char ***envp = &argp;
	
	while (*envp++);
	return forkexecve(path, &argp, envp);
}