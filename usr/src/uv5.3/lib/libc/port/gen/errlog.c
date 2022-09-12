
extern int _utssys();

int errlog(msg, msz)
char *msg;
int msz;
{
	return _utssys(12, msz, msg);
}
