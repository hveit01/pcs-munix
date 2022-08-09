#ifndef __MEM_HPP__
#define __MEM_HPP__

class MEM {
	
public:
	MEM* next;
	char name[20];
	unsigned int base;
	unsigned int size;
	unsigned char* mem;
	int flags;
#define MEM_RO	1
#define MEM_STACK 2

public:
	MEM(const char* name, unsigned int addr, int size, int flags, MEM* link);
	~MEM();
	
	MEM* Clone(MEM* link); /* make an exact copy */
	static MEM* Delete(MEM* item); /* delete this item and returns next */

	unsigned char* GetMem() const { return mem; }
	
	void Replace(unsigned newbase, int newsize, int newflags); /* replace by cleared memory */
	void Grow(int incr); /* extend and copy memory */
};

/* functions that are non-class because they are called from Musashi */
unsigned int cpu_read_byte(unsigned int address);
unsigned int cpu_read_word(unsigned int address);
unsigned int cpu_read_long(unsigned int address);
unsigned int cpu_read_word_dasm(unsigned int address);
unsigned int cpu_read_long_dasm(unsigned int address);
void cpu_write_byte(unsigned int address, unsigned int value);
void cpu_write_word(unsigned int address, unsigned int value);
void cpu_write_long(unsigned int address, unsigned int value);

#endif
