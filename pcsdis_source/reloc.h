#ifndef _RELOC_H_
#define _RELOC_H_

#include "item.h"
#include "symbol.h"

class _Reloc {
private:
	int addr;
	Symbol sym;
	String name;
	int idx;
	int type;
	int size;
	int target;
public:
	_Reloc(Symbol sym, int vaddr, int type);
	virtual ~_Reloc() {}
	virtual cString&  Name() const { return sym->Name(); }

	void SetIdx(int newidx) { idx = newidx; }
	int Idx() const { return idx; }
	
	int RelocType() const { return type; }
	int Size() const { return size; } // overloaded from _Item
	int Addr() const { return addr; }
	Symbol Sym() const { return sym; }
	
	int Target() const { return target; }
	void SetTarget(Symbol nsym, int tgt) {
		target = tgt; sym = nsym; }

	void Dump(std::ostream&) const;
};

#endif
