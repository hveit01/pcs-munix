#include "instr.h"
#include "segment.h"

_DataInstr::_DataInstr(int addr)
	: _Instr(addr), typ(ITYPEunknown)
{
	DByte b = seg->DByteAt(addr);
	if (!makereloc(b) && !makeascii(b) && !makeword())
		FATALERROR("Unrecognized DataInstr at " + to_hexstring(addr));
}

_DataInstr::_DataInstr(int addr, int sz)
	: _Instr(addr), typ(ITYPEunknown)
{
	switch (sz) {
	case 2:
		makeword(); break;
	case 4:
		makelong(); break;
	case -4:
		makereloc(seg->DByteAt(addr)); break;
	default:
		FATALERROR("Default case sz=" + std::to_string(sz));
	}
}

cString _DataInstr::toAsmString() const
{
	String astr(opstr);
	if (arg1) {
		astr += "\t";
		astr += arg1->toAsmString();
	}
	return astr;
}

int _DataInstr::Target() const
{
	return arg1->Target();
}

int _DataInstr::Value() const
{
	return arg1->Value();
}

Expr _DataInstr::MakeExpr(BB mybb)
{
	return 0;
}

bool _DataInstr::makereloc(DByte b)
{
	int ad = addr;
	Reloc r = seg->RelocByAddr(ad);
	if (r == 0) return false;
	
	opstr = "dc" + LField(OPWlong);
	
	int off = slong();
	int rel = addr + off;
	arg1 = new RelocArg(r, off);
	if (off >= 0 && off < seg->Size())
		seg->Enqueue(off, "DataInstr::makereloc");
	SetEnd();
	if (!seg->IsText()) endbb = false;
	typ = ITYPEpvar;
	size = 4;
	return true;
}

bool _DataInstr::makeascii(DByte b)
{
	char buf[1024];
	char *p = buf;
	int ad = addr;

	int ch;
	while ((ch = b->Value()) != 0 && 
		(isprint(ch) || ch==0x1b || ch=='\n' || ch=='\r' || ch=='\t')) {
		*p++ = ch; size++;
		b = seg->DByteAt(ad + size);
	}
	if (ch || 		// a non-printable char found, should not be a string
		buf==p) 	// no chars read
		return false;
	*p++ = 0;

	opstr = "dc" + LField(OPWbyte);

	arg1 = new StringArg(buf);
	SetEnd();
//	endbb = false;
	typ = ITYPEcvar;
	return true;
}

bool _DataInstr::makeword()
{
	opstr = "dc" + LField(OPWword);

	ItemWord iw = seg->WordAt(addr);
	size = 2;
	arg1 = new NumArg(iw->Value());
	typ = ITYPEivar;
//	endbb = false;
	return true;
}

bool _DataInstr::makelong()
{
	opstr = "dc" + LField(OPWlong);

	ItemLong il = seg->LongAt(addr);
	size = 4;
	arg1 = new NumArg(il->Value());
	typ = ITYPEivar;
//	endbb = false;
	return true;
}

void _DataInstr::LinkBB(BB mybb) const
{
	mybb->SetSType(STMT_table);
}

/****************************************************************************/

_BssInstr::_BssInstr(int addr, int sz)
	: _Instr(addr)
{
	size = sz;
	arg1 = new NumArg(sz);

	opstr = "ds" + LField(OPWbyte);
}

Expr _BssInstr::MakeExpr(BB mybb)
{
	return 0;
}

