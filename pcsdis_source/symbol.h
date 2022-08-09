#ifndef _DSYMBOL_H_
#define _DSYMBOL_H_

#include "common.h"
#include "coff.h"

/* visibility */
enum SymVis {
	SYMV_Local,
	SYMV_Global,
	SYMV_Static
};

class _Symbol
{
	friend class SymbolTable;
private:
	Symbol self;
	String name;
	unsigned int nidx;
	Segment *sec;
	int secnum;
	int idx;
	int abs, rel;
	SymVis vis;
public:
	_Symbol(cString& nam, int secnum, int idx, int base);
	_Symbol(int nidx, int secnum, int idx, int base);
	_Symbol(cString& nam, Segment *sec, int idx, int addr);
	~_Symbol() {}
	Symbol Self(Symbol s=0) { if (s) self = s; return s; }
	bool operator==(cSymbol& other) const { return name==other->Name(); }
	
	cString& Name() const { return name; }
	void SetName(cString& nam) { name = nam; }
	bool HasNoName() const { return name == "NONAME"; }

	Segment *GetSegment();
	int Secnum() const { return secnum; }
	
	int Abs() const { return abs; }
	int Rel();
	int NameIdx() const { return nidx; }
	
	void SetVisibility(SymVis newv) { vis = newv; }
	bool IsGlobal() const { return vis==SYMV_Global; }
	bool IsStatic() const { return vis==SYMV_Static; }
	bool IsLocal() const { return vis==SYMV_Local; }

	int Idx() const { return idx; }
	
	void Dump(std::ostream& os) const;
};

class SymbolTable
{
private:
	SymbolVector syms;
	static SymbolTable *_instance;
	SymbolTable() : syms() {}
public:
	virtual ~SymbolTable() {}
	static SymbolTable *Instance();
	
	Symbol AddSymbol(Symbol s);
	Symbol AddSymbol(cString& nam, int secnum, int idx, int base);
	Symbol AddSymbol(int nidx, int secnum, int idx, int base);
	Symbol AddSymbol(cString& nam, Segment *sec, int idx, int addr);
	Symbol SymbolByIdx(int idx) const;
	Symbol SymbolByName(cString& name) const;

	const char *SymbolName(int idx) const;
	int SymbolCnt() const { return syms.size(); }
	
	Symbol AddLocal(Segment *sec, int addr);
};

#endif
