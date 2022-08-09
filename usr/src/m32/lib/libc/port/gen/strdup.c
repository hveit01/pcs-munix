
extern char* malloc(), *strcpy();

char* strdup(s)
char* s;
{
    char* copy = malloc(strlen(s)+1);
    if (!copy)
        return 0;
    
    return strcpy(copy, s);
}