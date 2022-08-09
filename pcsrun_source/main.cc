#include "pcsrun.h"
#include <time.h>
#include "m68k.h"

int quit;
int debugflg = 0;

/* Disassembler */
void make_hex(char* buff, unsigned int pc, unsigned int length)
{
	char* ptr = buff;

	for(;length>0;length -= 2)
	{
		sprintf(ptr, "%04x", cpu_read_word_dasm(pc));
		pc += 2;
		ptr += 4;
		if(length > 2)
			*ptr++ = ' ';
	}
}

static const char* fakeenvp[] = {
	"PATH=/bin:/usr/bin",
	"TMP=/tmp",
	"SHELL=/bin/sh",
	"TTY=/dev/tty0",
	"USER=pcsrun",
	"TERM=vt100",
	0,
	0
};

/* The main loop */
int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("Usage: pcsrun [-d...] <program file> [arguments]\n");
		exit(-1);
	}

	int off = 1;
	if (argv[off][0] == '-') {
		parse_debugflg(argv[off]);
		off++;
	}
	
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68020);
	m68k_set_fpu_enable(1);
	m68k_set_instr_hook_callback(cpu_instr_hook);

	PROC* root = new PROC(1, fakeenvp);
	if (!root->Load(argv[off], argc-off, argv+off)) {
		exit_error("Cannot load program %s\n", argv[off]);
	}
		
//	m68k_pulse_reset();

//	printf("Starting: PC=0x%08x A7=0x%08x\n", 
//		m68k_get_reg(curcontext->cpuctx, M68K_REG_PC),
//		m68k_get_reg(curcontext->cpuctx, M68K_REG_A7));
	
	quit = 0;
	while (!quit)
	{
		// Values to execute determine the interleave rate.
		// Smaller values allow for more accurate interleaving with multiple
		// devices/CPUs but is more processor intensive.
		// 100000 is usually a good value to start at, then work from there.

		// Note that I am not emulating the correct clock speed!
		m68k_execute(100000);
	}

	return 0;
}
