#ifndef _BINFILE_H_
#define _BINFILE_H_

#include "common.h"
#include "reloc.h"
#include "segment.h"

/*forward*/class SegmentFactory;

class BinFile
{
protected:
	bool swap;
	void enqueuesyms();
	void dosegments();
	void setswap(bool newswap) { swap = newswap; }
	std::istream* is;
	void check_stream(cString& where) const;
public:
	BinFile();
	virtual ~BinFile();
	
	virtual bool Open(cString& file);
	virtual bool Read() { return false; }
	
	bool Disassemble();
	bool Decompile();
	
	std::istream *Istream() { return is; }
	bool IsSwap() const { return swap; }
	
	void WriteAsm(std::ostream& os) const;
	void WriteC(std::ostream& os) const;
};

#endif