#ifndef _COFF_H_
#define _COFF_H_

#include <stdint.h>

struct	naout	{
	uint16_t	magic;
	uint16_t	vstamp;
	uint32_t	tsize,
		dsize,
		bsize,
		entry,
		ts,
		ds;
};

struct	filhd	{
	uint16_t	magic;
	uint16_t	nscns;
	uint32_t	timdat,
				symptr,
				nsyms;
	uint16_t	opthdr,
				flags;
};

struct	scnhdr	{
	char		s_name[8];
	uint32_t	s_paddr,
				s_vaddr,
				s_size,
				s_scnptr,
				s_relptr,
				s_lnnoptr;
	uint16_t	s_nreloc,
				s_nlnno;
	uint32_t	s_flags;
};

struct  ehd {
	struct filhd ef;
	struct naout af;
	struct scnhdr sf[4];
};

#pragma pack(2)
struct oexec {
	uint16_t a_magic;
	uint32_t a_text;
	uint32_t a_data;
	uint32_t a_bss;
	uint32_t a_syms;
	uint32_t a_entry;
	uint32_t a_stksiz;
	uint16_t a_flag;
};
#pragma pack()



#endif
