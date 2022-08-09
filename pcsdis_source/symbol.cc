#include "symbol.h"
#include "segment.h"

_Symbol::_Symbol(cString& nam, int secnum, int idx, int abs)
	: name(nam), nidx(0), sec(0), secnum(secnum), idx(idx), abs(abs),
	  rel(-1), vis(SYMV_Local)
{}

_Symbol::_Symbol(int nidx, int secnum, int idx, int abs)
	: name("NONAME"), nidx(nidx), sec(0), secnum(secnum), idx(idx), abs(abs),
	  rel(-1), vis(SYMV_Local)
{}

_Symbol::_Symbol(cString& nam, Segment *sec, int idx, int addr)
	: name(nam), nidx(0), sec(sec), secnum(0), idx(idx), abs(addr),
	  rel(-1), vis(SYMV_Local)
{}

void _Symbol::Dump(std::ostream& os) const
{
	os << "Symbol:" << std::endl;
	os <<      "  name:   " << name << std::endl;
	dprint(os, "  nidx:   ", (short)nidx);
	dprint(os, "  secnum: ", (short)secnum);
	dprint(os, "  idx:    ", (short)idx);
	dprint(os, "  abs:    ", (int)abs);
}

int _Symbol::Rel()
{
	if (rel == -1) {
		Segment *s = GetSegment();
		if (s) {
			rel = s->Abs2Rel(abs);
			s->MakeGlobal(self);
		} else
			rel = abs;
	}

	Diag::Info(DIAGsymbol, "Symbol " + Name() +
		" abs=" + to_hexstring(abs) +
		" rel=" + to_hexstring(rel));
	return rel;
}

Segment *_Symbol::GetSegment()
{
	if (nidx == 0xffffffff)
		return 0;
	if (!sec) {
		SegmentFactory *sf = SegmentFactory::Instance();
		switch(secnum) {
		case 1:
			sec = sf->Text(); break;
		case 2:
			sec = sf->Data(); break;
		case 3:
			sec = sf->Bss(); break;
		case 0:
		case 0xfffffffe:
		case -1:	/* this is a debug symbol entry */
			return 0;
		default:
			FATALERROR("GetSegment: secnum=" + std::to_string(secnum));
		}
	}
	return sec;
}

/*************************************************************/

SymbolTable *SymbolTable::_instance = 0;

SymbolTable *SymbolTable::Instance()
{
	if (!SymbolTable::_instance)
		SymbolTable::_instance = new SymbolTable();
	return SymbolTable::_instance;
}

Symbol SymbolTable::SymbolByIdx(int idx) const
{
	for (auto s : syms)
		if (s->Idx() == idx) return s;
	
	FATALERROR("No Symbol index=" + std::to_string(idx));
	return 0;
}

Symbol SymbolTable::AddSymbol(Symbol s)
{
	Diag::Trace(DIAGsymbol,	"Add " + s->Name() + 
		" at idx=" + std::to_string(s->Idx()));
	syms.push_back(s);
	return s->Self(s);
}

Symbol SymbolTable::AddSymbol(cString& nam, int secnum, int idx, int base)
{
	Symbol s = std::make_shared<_Symbol>(nam, secnum, idx, base);
	return AddSymbol(s);
}

Symbol SymbolTable::AddSymbol(int nidx, int secnum, int idx, int base)
{
	Symbol s = std::make_shared<_Symbol>(nidx, secnum, idx, base);
	return AddSymbol(s);
}

Symbol SymbolTable::AddSymbol(cString& nam, Segment *sec, int idx, int addr)
{
	Symbol s = std::make_shared<_Symbol>(nam, sec, idx, addr);
	return AddSymbol(s);
}

Symbol SymbolTable::SymbolByName(cString& name) const
{
	for (auto s : syms)
		if (s->Name() == name)
			return s;
	return 0;
}

const char* SymbolTable::SymbolName(int idx) const
{
	Symbol sym = SymbolByIdx(idx);
	return sym->Name().c_str();
}

Symbol SymbolTable::AddLocal(Segment *sec, int addr)
{
	String sname("lbl_");
	sname += to_hexstring(addr);
	Symbol sym = SymbolByName(sname);
	if (!sym) {
		int idx = SymbolCnt();
		sym = AddSymbol(sname, sec, idx, addr);
	}
	return sym;
}
