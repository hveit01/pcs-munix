#include "common.h"
#include "coff.h"
#include "reloc.h"

_Reloc::_Reloc(Symbol sym, int vaddr, int type)
	: addr(vaddr), sym(sym), name(sym->Name()), idx(-1), type(type), target(0)
{
	switch (type) {
	case R_RELBYTE:
	case R_PCRBYTE:
		size = 1; break;
	case R_RELWORD:
	case R_PCRWORD:
		size = 2; break;
	case R_RELLONG:
	case R_PCRLONG:
		size = 4; break;
	default:
		FATALERROR("Default case type=" + std::to_string(type));
	}
}

void _Reloc::Dump(std::ostream& os) const
{
	dprint(os, "RELOC:", "");
	dprint(os, "  vaddr:  ", Addr());
	os <<      "  name:   " << name << std::endl;
	dprint(os, "  type:   ", RelocType());
}
