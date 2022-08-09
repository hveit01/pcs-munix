#ifndef SIM__HEADER
#define SIM__HEADER

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>


#include "debug.h"
#include "coff.h"
#include "mem.hpp"
#include "file.hpp"
#include "proc.hpp"

void cpu_instr_hook();
void cpu_trap13();
void cpu_trap14();

//MEM* getmembyname(PROC* ctx, const char* name);
//MEM* getmem(PROC* ctx, const char* name, unsigned int addr, int size, int flags);
int checkbp(unsigned int addr);
extern int quit;

/* utilities */
unsigned int swapl(unsigned int val);
unsigned int swapw(unsigned short val);
char* xlatpath(const char* pcspath);

/* debug and error support */
extern int instsum, instcnt;
extern int debugflg;
void trace_regs();

void debug(const char* fmt, ...);
void debug2(const char* fmt, ...);
void debug_coffhdr(const struct ehd* hdr);
void debug_memarea(struct scnhdr* sf, const char* name, unsigned int base, unsigned int size, unsigned int pos);
void parse_debugflg(const char *arg);
void print_debugflg();

void monitor(const char* prompt);
void exit_error(const char* fmt, ...);
const char *errstr(int err);

#endif /* SIM__HEADER */
