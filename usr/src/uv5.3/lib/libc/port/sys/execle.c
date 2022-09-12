
int execle(path, arg0 /*, arg1, ..., argn, (char *)0, envp */)
char *path, *arg0;
{
	register char** p = &arg0;
	
	while(*p++);
	return execve(path, &arg0, *p);
}
