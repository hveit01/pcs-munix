#include "binfile.h"
#include "reloc.h"
#include "segment.h"

BinFile::BinFile()
	: swap(false), is(0)
{
}

BinFile::~BinFile()
{
	delete is;
}

bool BinFile::Open(cString& infile)
{
	is = new std::ifstream(infile, std::ifstream::binary);
	check_stream("Binfile::Open");
	return true;
}

bool BinFile::Disassemble()
{
	Diag::Trace(DIAGfileread, "Disassembling");

	SegmentFactory *sf = SegmentFactory::Instance();
	return sf->Disassemble();
}

bool BinFile::Decompile()
{
	Diag::Trace(DIAGfileread, "Decompiling");

	SegmentFactory *sf = SegmentFactory::Instance();
	return sf->Decompile();
}

void BinFile::enqueuesyms()
{
	Diag::Trace(DIAGfileread, "enqueuesyms");

	SymbolTable *st = SymbolTable::Instance();
	for (int i=0; i<st->SymbolCnt(); i++) {
		Symbol sym = st->SymbolByIdx(i);
		Segment *seg = sym->GetSegment();
		if (seg && seg->Size() > 0) {
			(void)sym->Rel();
		}
	}
}

void BinFile::WriteAsm(std::ostream& os) const
{
	SegmentFactory *sf = SegmentFactory::Instance();
	sf->WriteAsm(os);
}

void BinFile::WriteC(std::ostream& os) const
{
	SegmentFactory *sf = SegmentFactory::Instance();
	sf->WriteC(os);
}

void BinFile::check_stream(cString& where) const
{
	if (!is->good()) FATALERROR("File read error in " + where);
}
