#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include "common.h"
#include "dbyte.h"
#include "reloc.h"
#include "symbol.h"
#include "instr.h"
#include "bb.h"

class Segment {
protected:
	String name;
	int base;
	std::map<int, Item> items; // list of bytes
	RelocVector relocs;
	std::deque<int> instq;
	std::map<int, Instr> instrs;
	CProcVector procs;
	int segsize;
	
	virtual void phase2(int addr) {}
	int dequeue(cString& where="");
	void clearat(int abs, int sz);

	CProc addproc(Symbol s);
	int byteat(int rel) const;
	bool validrel(int rel) const { return rel >= 0 && rel < segsize; }
	
public:
	Segment(cString& nam, int base, int segsize);
	virtual ~Segment() {}
	cString& Name() const { return name; }
	
	int Base() const { return base; }
	int Size() const { return segsize; } // overloads Item method!
	virtual bool IsText() const { return false; }
	virtual bool IsData() const { return false; }
	virtual bool IsBSS() const  { return false; }

	int Rel2Abs(int rel) const { return Base() + rel; }
	int Abs2Rel(int abs) const { return abs - Base(); }

	void AddReloc(Reloc dr);
	int RelocCnt() const { return relocs.size(); }
	Reloc RelocByAddr(int addr) const;
	const char *RelName(int addr) const;

	Symbol MakeLocal(int rel);
	void AddByte(DByte b) { items[b->Addr()] = b; }
	void MakeGlobal(Symbol sym);

	DByte DByteAt(int rel) const; // get byte at rel
	Instr InstrAt(int rel) const; // locate instr at rel
	Instr PrevInstrAt(int rel) const; // locate instr before rel
	Instr AddInstr(Instr inst);  // add an instruction to segment

	int OpcodeAt(int rel) const;
	Symbol SymbolAt(int rel) const;
	ItemWord WordAt(int rel) const;
	ItemWord SWordAt(int rel) const;
	ItemLong LongAt(int rel) const;
	ItemLong SLongAt(int rel) const { return LongAt(rel); }
	
	CProc Proc(cString& name) const;
	
	void Enqueue(int rel, cString& where="");
	
	bool DisassPhase1();
	bool DisassPhase2();
	virtual bool Decompile() { return true; }
	
	void DumpRelocs(std::ostream& os) const;
	void WriteAsm(std::ostream& os) const;
	virtual void WriteC(std::ostream& os, int indent) const {}
};

class TextSegment : public Segment
{
protected:
	void phase2(int addr);
//	void processsym(const char *symname, int addr);
public:
	TextSegment(cString& nam, int base, int size)
		: Segment(nam, base, size) {}
	~TextSegment() {}
	
	bool IsText() const { return true; }

	bool Decompile();
	void WriteC(std::ostream& os, int indent) const;
};

class DataSegment : public Segment
{
protected:
	void phase2(int addr);
public:
	DataSegment(cString& nam, int base, int size)
		: Segment(nam, base, size) {}
	~DataSegment() {}
	
	bool IsData() const { return true; }
	void WriteC(std::ostream& os, int indent) const;
};

class BSSSegment : public Segment
{
protected:
	void phase2(int addr);
public:
	BSSSegment(cString& nam, int base, int size)
		: Segment(nam, base, size) {}
	~BSSSegment() {}
	
	bool IsBSS() const { return true; }
	void WriteC(std::ostream& os, int indent) const;
};

class SegmentFactory
{
private:
	std::vector<Segment*> segs;

	static SegmentFactory *_instance;
	SegmentFactory();
//	void Dump(std::ostream&, const SCNHDR& hdr) const;

public:
	static SegmentFactory *Instance();
	
	Segment *MakeSegment(cString& name, int base, int size);
	
	Segment *Text() const { return segs[0]; }
	Segment *Data() const { return segs[1]; }
	Segment *Bss() const { return segs[2]; }
	Segment *SegByName(cString& name) const;
	
	bool Disassemble();
	bool Decompile();
	void WriteAsm(std::ostream& os) const;
	void WriteC(std::ostream& os) const;
};

#endif
