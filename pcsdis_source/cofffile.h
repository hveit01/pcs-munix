#ifndef _COFFFILE_H_
#define _COFFFILE_H_

#include "binfile.h"
#include "coff.h"
#include "reloc.h"

/*forward*/class Segment;

class COFFFile : public BinFile {
private:
	FILHDR filhdr;
	SYMENT syment;
	AUXENT auxent;

	bool readheader();
	void dumpheader(std::ostream&, const SCNHDR& hdr) const;
	void dumpfilhdr(std::ostream&) const;

	char *readsym();
	char *readaux();
	bool readsyms();
	bool readnames();

	bool readsegments();
	bool readsegment();
	bool readdata(Segment *sec, int pos, int sz);

	bool readrelocs(Segment *sec, int pos, int sz);
	Reloc readreloc();



public:
	COFFFile() : BinFile() {}
	~COFFFile() {}

	bool Read();
	void Dump(std::ostream&) const;
};

#endif

