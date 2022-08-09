#ifndef __PROC_HPP__
#define __PROC_HPP__

#include <stdio.h>

#define PCSNFILES 50
#define PCSNSIGS 33

/* opaque namelist */
struct nitem;
class NMAP {
private:
	nitem *root;
	const char *defname(unsigned ad);
public:
	NMAP() : root(0) {}
	~NMAP() { clear(); }
	void enter(unsigned long ad, const char *name);
	const char *find(unsigned ad);
	void clear();
	void dump();
};

class PROC {
private:
	static PROC* procs;
	static int thepid;
	
	PROC* next;
	char name[256];
	NMAP *names;

	char* copystr8(const char* s);
	void initsigs();
	void initfiles();
	void initstdio();
	void deleteenv();
	void initcpucontext();
	void clonemem(PROC* parent);
	
	static PROC* nextrunnable();
	void loadseg(FILE* fd, int offset, const char* name, int base, int size, int flag);
	void loadmap();

public:
	MEM* memlist;
	char* cpuctx;
	int stat;
	int m881;
	int pid;
	int ppid;
	unsigned int signals[PCSNSIGS];
	unsigned int waitptr;
	FILEENT files[PCSNFILES];
	char** envp;
	int envc;
	unsigned int tentry;
	unsigned int tstart;
	int tsize;
	unsigned int dstart;
	int dsize;
	unsigned int bstart;
	int bsize;
	unsigned int sstart;
	int ssize;
	
public:
	PROC(int ppid, const char** envp);
	~PROC();
	
	PROC* Clone();
	
	void CloneFiles(PROC* parent);
	void CloneEnv(const char **env);
	void DeleteMem();
	
	int Load(char* prog, int argc, char* argv[]);

	MEM* FindMem(const char* name) const; /* find mem by name */
	MEM* FindMem(unsigned int addr, int sz, bool fail=true);
	MEM* AllocMem(const char* name, unsigned int addr, int size, int flags);

	FILEENT* GetFile(int extfd, int* errval);
	FILEENT* GetFile(const char* name);
	FILEENT* GetEmptySlot(int* extfd, int* errval);
	void Unlink(const char* path);
	
	void SaveCpuContext(); /* save Musashi context here */
	void RestoreCpuContext(); /* store Musashi context back into simulator */
	
	const char* Name() const { return name; }
	int Pid() const { return pid; }
	bool IsPid(int p) const { return pid==p; }
	int Ppid() const { return ppid; }
	void Use881() { m881 = 1; }
	void SetStat(int newstat) { stat = newstat; }
	bool IsRunnable() const { return stat=='R'; }
	
	static PROC* cur;
	static PROC* GetProc(int pid);
	static void Switch(); /* switch to next runnable */
	void SwitchTo(); /* switch to this */
	
	static const char *FindName(unsigned addr);
	static void DebugList();
	
};

#endif