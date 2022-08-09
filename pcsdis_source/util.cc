#include "common.h"
#include "bitvec.h"

char *Strdup(const char *s)
{
	char *d = new char[strlen(s)+1];
	strcpy(d, s);
	return d;
}

char* Strdup8(const char *s)
{
	char *d = new char[9];
	strncpy(d, s, 8);
	d[8] = 0;
	return d;
}

char* Strdup14(const char *s)
{
	char *d = new char[15];
	strncpy(d, s, 14);
	d[14] = 0;
	return d;
}

void Indent(std::ostream& os, int n)
{
	for (int i=0; i<n; i++) os << " "; 
}

void IndentNoCR(std::ostream& os, int n, cString& prefix)
{
	Indent(os, n); os << prefix;
}

void Indent(std::ostream& os, int n, cString& prefix)
{
	IndentNoCR(os, n, prefix); os << std::endl;
}

String Comment(cString& text)
{
	String s("/* ");
	s += text;
	s += " */";
	return s;
}

void dprint(std::ostream& os, cString& f, const char *v)
{
	os << f << v << std::endl;
}

void dprint(std::ostream& os, cString& f, uint8_t v)
{
	os << f << std::hex << std::setw(2) << v << std::endl;
}

void dprint(std::ostream& os, cString& f, uint16_t v)
{
	os << f << std::hex << std::setw(4) << v << std::endl;
}

void dprint(std::ostream& os,  cString& f, uint32_t v)
{
	os << f << std::hex << std::setw(8) << v << std::endl;
}

void dprint(std::ostream& os,  cString& f, int8_t v)
{
	os << f << std::hex << std::setw(2) << v << std::endl;
}

void dprint(std::ostream& os, cString& f, int16_t v)
{
	os << f << std::hex << std::setw(4) << v << std::endl;
}

void dprint(std::ostream& os, cString& f, int32_t v)
{
	os << f << std::hex << std::setw(8) << v << std::endl;
}

void dprint(std::ostream& os, cString& f, cString& v)
{
	os << f << v << std::endl;
}

/*****************************************************************************/

BitVec::BitVec(int sz, bool initval)
	: bvec(sz)
{
	/* cannot use SetAll/ClearAll here because initially bvec is empty */
	for (int i=0; i<sz; i++)
		bvec[i] = initval;
}

BitVec::BitVec(const BitVec* other)
	: bvec(other->Size())
{
	Copy(other);
}

void BitVec::SetAll()
{
	for (int i=0; i<Size(); i++)
		bvec[i] = true;
}

void BitVec::ClearAll()
{
	for (int i=0; i<Size(); i++)
		bvec[i] = false;
}

bool BitVec::Set(int n)
{
	try { bvec.at(n) = true; return true; }
	catch(const std::out_of_range& oor) { return false; }
}

bool BitVec::Clear(int n)
{
	try { bvec.at(n) = false; return true; }
	catch(const std::out_of_range& oor) { return false; }
}

bool BitVec::Get(int n) const
{
	try { return bvec.at(n); }
	catch(const std::out_of_range& oor) {
		FATALERROR("BitVec::Get");
		return false;
	}
}

void BitVec::Copy(const BitVec *other)
{
	bvec = other->bvec;
}

bool BitVec::operator==(const BitVec *other) const
{
	return Size() == other->Size() &&
		std::equal(bvec.begin(), bvec.end(), other->bvec.begin());
}

bool BitVec::operator!=(const BitVec *other) const
{
	return Size() != other->Size() ||
		!std::equal(bvec.begin(), bvec.end(), other->bvec.begin());
}

void BitVec::And(const BitVec *other)
{
	if (Size() != other->Size())
		FATALERROR("BitVec::And");
	for (int i=0; i<Size(); i++)
		bvec[i] = bvec[i] && other->bvec[i];
}

void BitVec::Or(const BitVec *other)
{
	if (Size() != other->Size())
		FATALERROR("BitVec::Or");
	for (int i=0; i<Size(); i++)
		bvec[i] = bvec[i] || other->bvec[i];
}

void BitVec::Dump(std::ostream& os) const
{
	os << "[";
	for (auto b : bvec)
		os << b?1:0;
	os << "]" << std::endl;
}

int BitVec::FirstNotEqual(int n) const
{
	for (int i=0; i <bvec.size(); i++)
		if (bvec[i] && i != n) return i;
	return -1;
}

/*****************************************************************************/

std::map<DiagFlag,bool> Diag::flags;
bool Diag::flagall = false;

cString Diag::flag2name(DiagFlag f)
{
	switch (f) {
	default:
		return "????";
	case DIAGbb:
		return  "BB  ";
	case DIAGcase:
		return "CASE";
	case DIAGexpr:
		return "EXPR";
	case DIAGfileread:
		return "RDR ";
	case DIAGinstr:
		return "INST";
	case DIAGloop:
		return "LOOP";
	case DIAGreloc:
		return "RELO";
	case DIAGproc:
		return "PROC";
	case DIAGsegment:
		return "SEG ";
	case DIAGtrace:
		return "TRAC";
	case DIAGsymbol:
		return "SYMB";
	}
}

void Diag::Set(DiagFlag f, bool tf)
{
	if (f==DIAG_all)
		flagall = tf;
	else
		flags[f] = tf;
}

bool Diag::IsDiag(DiagFlag f)
{
	if (flagall) return true;
	auto it = flags.find(f);
	return it != flags.end() && it->second;
}

void Diag::Info(cString& s1)
{
	DIAGstream << "[INFO] " << s1 << std::endl;
}

void Diag::Info(DiagFlag f, cString& s1)
{
	if (IsDiag(f))
		DIAGstream << "[" << flag2name(f) << "] " << s1 << std::endl;
}

void Diag::Trace(DiagFlag f, cString& s1)
{
	if (IsDiag(f))
		Info(DIAGtrace, s1);
}

void Diag::Error(cString& msg)
{
	DIAGstream << "*** Error *** " << msg << std::endl;
}

void Diag::Fatal(cString& msg, const char* file, int line)
{
	DIAGstream << "***Fatal*** in file=" << file 
		       << ", line=" << std::dec << line 
			   << ": " << msg << std::endl;
	exit(1);
}
