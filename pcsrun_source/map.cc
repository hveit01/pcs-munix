#include "pcsrun.h"


/* yes, I know that libstdc++ has a map template, 
 * and it is surely faster, but I don't care */

struct nitem {
	nitem *next;
	char *name;
	unsigned addr;
	nitem(unsigned long ad, const char *nam, nitem *nx)
		: addr(ad), next(nx) {
		name = strdup(nam);
	}
	~nitem() { delete name; }
};

void NMAP::enter(unsigned long ad, const char *name) {
	root = new nitem(ad, name, root);
}
	
const char *NMAP::find(unsigned ad) {
	for (nitem *i = root; i; i = i->next) {
		if (i->addr == ad) return i->name;
	}
	return defname(ad);
}

const char *NMAP::defname(unsigned ad) {
	static char adbuf[30];
	sprintf(adbuf, "$%08x", ad);
//	sprintf(adbuf, "%08x", ad);
	return adbuf;
}

void NMAP::clear() {
	for (nitem *i = root; i; ) {
		nitem *nx = i->next;
		delete i;
		i = nx;
	}
	root = 0;
}

void NMAP::dump() {
	printf("Dump of symbols:\n");
	for (nitem *it=root; it; it = it->next)
		printf("%08x: %s\n", it->addr, it->name);
}
