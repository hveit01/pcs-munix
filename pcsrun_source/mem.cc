#include "m68k.h"
#include "pcsrun.h"

/* Read/write macros */
#define READ_BYTE(BASE, ADDR) (BASE)[ADDR]
#define READ_WORD(BASE, ADDR) (((BASE)[ADDR]<<8) |			\
							  (BASE)[(ADDR)+1])
#define READ_LONG(BASE, ADDR) (((BASE)[ADDR]<<24) |			\
							  ((BASE)[(ADDR)+1]<<16) |		\
							  ((BASE)[(ADDR)+2]<<8) |		\
							  (BASE)[(ADDR)+3])

#define WRITE_BYTE(BASE, ADDR, VAL) (BASE)[ADDR] = (VAL)&0xff
#define WRITE_WORD(BASE, ADDR, VAL) (BASE)[ADDR] = ((VAL)>>8) & 0xff;		\
									(BASE)[(ADDR)+1] = (VAL)&0xff
#define WRITE_LONG(BASE, ADDR, VAL) (BASE)[ADDR] = ((VAL)>>24) & 0xff;		\
									(BASE)[(ADDR)+1] = ((VAL)>>16)&0xff;	\
									(BASE)[(ADDR)+2] = ((VAL)>>8)&0xff;		\
									(BASE)[(ADDR)+3] = (VAL)&0xff

unsigned int cpu_read_byte(unsigned int address)
{
	MEM* mem = PROC::cur->FindMem(address, 1, false);
	return READ_BYTE(mem->mem, address-mem->base);
}

unsigned int cpu_read_word(unsigned int address)
{
	MEM* mem = PROC::cur->FindMem(address, 2, false);
	return READ_WORD(mem->mem, address-mem->base);
}

unsigned int cpu_read_long(unsigned int address)
{
	MEM* mem = PROC::cur->FindMem(address, 4, false);
	return READ_LONG(mem->mem, address-mem->base);
}

unsigned int cpu_read_word_dasm(unsigned int address)
{
	MEM* mem = PROC::cur->FindMem(address, 2);
	return READ_WORD(mem->mem, address-mem->base);
}

unsigned int cpu_read_long_dasm(unsigned int address)
{
	MEM* mem = PROC::cur->FindMem(address, 4);
	return READ_LONG(mem->mem, address-mem->base);
}

/* Write data to RAM or a device */
void cpu_write_byte(unsigned int address, unsigned int value)
{
	MEM* mem = PROC::cur->FindMem(address, 1);
	if (mem->flags & MEM_RO)
		exit_error("Fatal: Write to RO area %s, addr=0x%x, sz=%d\n", mem->name, address, 1);

	WRITE_BYTE(mem->mem, address-mem->base, value);
	
}

void cpu_write_word(unsigned int address, unsigned int value)
{
	MEM* mem = PROC::cur->FindMem(address, 2);
	if (mem->flags & MEM_RO)
		exit_error("Fatal: Write to RO area %s, addr=0x%x, sz=%d\n", mem->name, address, 2);
		
	WRITE_WORD(mem->mem, address-mem->base, value);
}

void cpu_write_long(unsigned int address, unsigned int value)
{
	MEM* mem = PROC::cur->FindMem(address, 4);
	if (mem->flags & MEM_RO)
		exit_error("Fatal: Write to RO area %s, addr=0x%x, sz=%d\n", mem->name, address, 4);

	WRITE_LONG(mem->mem, address-mem->base, value);
}

MEM::MEM(const char* nam, unsigned int addr, int sz, int flg, MEM* link)
	: next(link), base(addr), size(sz), flags(flg)
{
	strcpy(name, nam);
	mem = new unsigned char[size];
	memset(mem, 0, size);
	debug("  region %s: created: base=0x%x size=%d\n", name, addr, size);
}

MEM::~MEM()
{
	strcpy(name, "-deleted-");
	delete mem;
	next = 0;
	size = 0;
	base = 0;
}

void MEM::Replace(unsigned newbase, int newsize, int newflags)
{
	delete mem;
	base = newbase;
	size = newsize;
	flags = newflags;
	mem = new unsigned char[size];
	memset(mem, 0, size);

	debug("  region %s: changed: base=0x%x size=%d\n", name, base, size);
}

MEM* MEM::Clone(MEM* link)
{
	MEM* m = new MEM(name, base, size, flags, link);
	memcpy(m->mem, mem, size);
	return m;	
}

/* positive size will add to end of region, negative will lower base */
void MEM::Grow(int incr)
{
	size += incr > 0 ? incr : -incr; /* increase size */
	unsigned char* newm = new unsigned char[size];
	memset(newm, 0, size);
	if (incr > 0) {
		if (mem) memcpy(newm, mem, size-incr);
	} else {
		if (mem) memcpy(newm-incr, mem, size+incr); /* note newm-incr is > newm! */
		base += incr; /* lower base */
	}
	delete mem; /* free old mem */
	mem = newm;
	
	debug("  region %s: resized by %d, base=%x, size=%d\n", name, incr, base, size);
}

MEM* MEM::Delete(MEM* item)
{ /* delete this item and returns next */
	MEM* n = item->next;
	delete item;
	return n;
}