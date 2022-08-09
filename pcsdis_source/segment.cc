#include "segment.h"
#include "cproc.h"

Segment::Segment(cString& nam, int base, int segsize)
	: name(nam), base(base), items(), relocs(), instq(),
	  instrs(), procs(), segsize(segsize)
{
	for (int i=0; i<segsize; i++) {
		DByte b = std::make_shared<_DByte>(i,0);
		AddByte(b);
	}
}

Instr Segment::AddInstr(Instr inst)
{
	int addr = inst->Addr();
	int sz = inst->Size();
	instrs[addr] = inst->Self(inst);
	
	/* mark instructions */
	for (int i=0; i < sz; i++) {
		DByte b = DByteAt(addr+i);
		b->AddFlag(i ? DB_ISUCC : DB_IHEAD);
	}
	return inst;
}

void Segment::AddReloc(Reloc dr)
{
	Diag::Info(DIAGsegment,
		"AddReloc cnt=" + std::to_string(RelocCnt()) +
		" addr=" + to_hexstring(dr->Addr()) + 
		" idx=" + std::to_string(dr->Idx()) +
		" type=" + std::to_string(dr->RelocType()) +
		" size=" + std::to_string(dr->Size()) +
		" target=" + to_hexstring(dr->Target()) +
		" name=" + dr->Name());

	dr->SetIdx(RelocCnt());
	relocs.push_back(dr);
}

Reloc Segment::RelocByAddr(int rel) const
{
	int abs = Rel2Abs(rel);
	for (auto r : relocs)
		if (r->Addr()==abs) return r;
	return 0;
}

const char *Segment::RelName(int addr) const
{
	Reloc rel = RelocByAddr(addr);
	return rel ? rel->Name().c_str() : 0;
}

/* given a rel addr, will return Instr, if any at this address
 * was less robust before */
Instr Segment::InstrAt(int rel) const
{
	if (!validrel(rel)) return 0;
	for (auto it : instrs) {
		int addr = it.first;
		Instr inst = it.second;
		if (rel >= addr && rel < (addr+inst->Size()))
			return inst;
	}
	return 0;
}

Instr Segment::PrevInstrAt(int rel) const 
{
	if (!validrel(rel)) return 0;
	Instr cur = InstrAt(rel);
	if (!cur) return 0;
	int addr = cur->Addr();
	if (addr==0) return 0;
	return InstrAt(addr-1);
}

int Segment::OpcodeAt(int rel) const
{
	Instr inst = InstrAt(rel);
	if (inst == 0)
		FATALERROR("No instr at 0x" + to_hexstring(rel));
	else {
		std::cout << "OpcodeAt(0x"<<to_hexstring(rel)<<")= "<< inst->toAsmString()<<std::endl;
	}
	return inst->Opcode();
}

Symbol Segment::SymbolAt(int rel) const
{
	return DByteAt(rel)->Sym();
}

DByte Segment::DByteAt(int rel) const
{
	auto it = items.find(rel);
	if (it == items.end())
		FATALERROR(name + ": No DByte at " + std::to_string(rel));

	return std::dynamic_pointer_cast<_DByte>(it->second);
}

int Segment::byteat(int rel) const
{
	DByte b1 = DByteAt(rel);
	return b1->Value() & 0xff;
}

ItemWord Segment::WordAt(int rel) const
{
	int b1 = byteat(rel);
	int b2 = byteat(rel+1);
	return std::make_shared<_ItemWord>(rel, (b1<<8) | b2);
}

ItemWord Segment::SWordAt(int rel) const
{
	ItemWord iw = WordAt(rel);
	iw->SetValue(iw->Value16());
	return iw;
}

ItemLong Segment::LongAt(int rel) const
{
	int b1 = byteat(rel);
	int b2 = byteat(rel+1);
	int b3 = byteat(rel+2);
	int b4 = byteat(rel+3);

	return std::make_shared<_ItemLong>(rel, (((((b1<<8) | b2)<<8) | b3)<<8) | b4);
}

/* create a local symbol for some position in a segment */
Symbol Segment::MakeLocal(int rel)
{
//	std::cout << "MakeLocal(" << Name() << "): rel=" << rel << std::endl;
	DByte b = DByteAt(rel);
	Symbol sym = b->Sym();
	if (!sym) {
		SymbolTable *st = SymbolTable::Instance();
		sym = st->AddLocal(this, Rel2Abs(rel));
		b->SetSym(sym);
		Diag::Info(DIAGsegment, "Makelocal new=" + sym->Name());
	}
//    else
//		Diag::Info(DIAGsegment, "Makelocal is=" + sym->Name());
	return sym;
}

/* define a global symbol at given location; addr is relative to segment base */
void Segment::MakeGlobal(Symbol sym)
{
	Diag::Trace(DIAGsegment, 
		"MakeGlobal sym=" + sym->Name() +
		" abs=" + to_hexstring(sym->Abs()) + " idx=" + to_hexstring(sym->Idx()));

	if (sym->IsLocal()) sym->SetVisibility(SYMV_Global);
	int addr = sym->Rel();
	DByte b = DByteAt(addr);
	b->SetSym(sym);

	/* feed the queue of interesting locations */
	Enqueue(addr, "MakeGlobal");
}

/* Phase2 will generate instructions after all symbols are identified */
bool Segment::DisassPhase2()
{
	Diag::Trace(DIAGsegment, "DisassPhase2 " + Name());
	while (!instq.empty()) {
		int rel = dequeue(/*"DisassPhase2"*/);
		DByte b = DByteAt(rel);
		if (!b->IsFlag(DB_IHEAD))
			phase2(b->Addr());
	}
	return true;
}

void Segment::clearat(int rel, int sz)
{
	for (int i=0; i<sz; i++) {
		DByte b = DByteAt(rel+i);
		b->SetValue(0);
	}
}

void Segment::DumpRelocs(std::ostream& os) const
{
	for (auto r : relocs) {
		os << "Reloc r=" << r << " name=" << r->Name() << std::endl;
	}
}

/* This is called once after all segments have been read.
 * This is supposed to fix all relocs that are not global or external
 * but point to this or other segments, such as case references
 */
bool Segment::DisassPhase1()
{
	Diag::Trace(DIAGsegment, "DisassPhase1 " + Name());

	SegmentFactory *sf = SegmentFactory::Instance();
	for (auto r : relocs) {
		if (Diag::IsDiag(DIAGsegment)) r->Dump(DIAGstream);
		Segment *tgt = sf->SegByName(r->Name().c_str());
		if (tgt) { /* this is the target segment: .text, .data, or .bss */
			Diag::Info(DIAGsegment, "vaddr=" + to_hexstring(r->Addr()));
			int rel = Abs2Rel(r->Addr()); /* where reloc is applied  ins segment*/
			int sz = r->Size();

			/* where reloc points to */
			Item tit;
			if (sz==2) tit = WordAt(rel);
			else tit = LongAt(rel);
			int trel = tgt->Abs2Rel(tit->Value());
			Diag::Info(DIAGsegment, "trel=" + to_hexstring(trel));
			
			Symbol sym = tgt->MakeLocal(trel); /* create a local symbol in tgt sec */
			r->SetTarget(sym, trel);
			if (Diag::IsDiag(DIAGsegment)) r->Dump(DIAGstream);
			clearat(rel, sz); /* destroy offset */
			tgt->Enqueue(trel, "Reloc");
		}
	}
	return true;
}

void Segment::Enqueue(int rel, cString& where)
{
	Diag::Info(DIAGsegment, where + ": " +
		"ENQUEUE " + Name() + " at " + to_hexstring(rel));
	auto it = instrs.lower_bound(rel);
	if (it == instrs.end() || it->first != rel) {
		// not yet processed
		if (!where.empty()) {
			Diag::Info(DIAGsegment, where + ": " +
				"ENQUEUE " + Name() + " at " + to_hexstring(rel));
		}
		instq.push_back(rel);
	}
}

int Segment::dequeue(cString& where)
{
	int rel = instq.front();
	Diag::Info(DIAGsegment, where + ": " +
		"DEQUEUE " + Name() + " at " + to_hexstring(rel));
	instq.pop_front();
	if (!where.empty()) {
		Diag::Info(DIAGsegment, where + ": " +
			"DEQUEUE " + Name() + " at " + to_hexstring(rel));
	}
	return rel;
}

void Segment::WriteAsm(std::ostream& os) const
{
		for (auto i : instrs)
			i.second->WriteAsm(os);
}

CProc Segment::addproc(Symbol s)
{
	CProc p = _CProc::MakeProc(s);
	procs.push_back(p);
	return p;
}

/**********************************************************************/

void TextSegment::phase2(int rel)
{
	Instr inst;
	bool needlabel = false;
	Diag::Trace(DIAGsegment, "Phase2 text rel=" + to_hexstring(rel));
	do {
		DByte b = DByteAt(rel);
		if (needlabel) { // last instruction terminated a BB
			MakeLocal(rel); // enforce that current one has a label
			needlabel = false;
		}
		if (b->IsFlag(DB_IHEAD)) /* there is already an instruction */
			return;
		if (b->IsFlag(DB_ISUCC)) {
			Diag::Info(DIAGsegment, "Into instr rel=" + to_hexstring(rel));
			return;
		}
		inst = InstrFactory::Instance().MakeInst(this, rel);
		Diag::Info(DIAGsegment, "At " + to_hexstring(rel) +
			" inst=" + inst->toAsmString());

		rel += inst->Size();
		needlabel = inst->IsEndBB();	// ends a basic block
	} while (!inst->IsEnd());
	Diag::Trace(DIAGsegment, "End of phase2 rel=" + to_hexstring(rel));
}

bool TextSegment::Decompile()
{
	/* collect procedures */
	CProc curproc = 0;
	for (auto it : instrs) {
		Instr inst = it.second;
		Symbol sym = inst->Sym();
		if (sym && (sym->IsGlobal() || sym->IsStatic()))
			curproc = addproc(sym);
		curproc->AddInstr(inst);
	}
	
	/* link BBs within each procedure */
	for (auto p : procs)
		if (!p->Decompile()) return false;
	return true;
}

void TextSegment::WriteC(std::ostream& os, int indent) const
{
	os << "/* TEXT SEGMENT */" << std::endl;
	for (auto p : procs)
		p->WriteC(os, indent);
}

/**********************************************************************/

void DataSegment::phase2(int rel)
{
	Instr inst;
	Diag::Trace(DIAGsegment, "Phase2 data rel=" + to_hexstring(rel));
	do {
		DByte b = DByteAt(rel);
//		DIAGstream << "*** Follow: 0x" << std::hex << rel << std::endl;
		if (b->IsFlag(DB_IHEAD))
			return;
		if (b->IsFlag(DB_ISUCC)) {
			Diag::Info(DIAGsegment, "Into instr rel=" + to_hexstring(rel));
			return;
		}
		inst = InstrFactory::Instance().MakeData(this, rel);
		Diag::Info(DIAGsegment, "At " + to_hexstring(rel) +
			" inst=" + inst->toAsmString());
		
		rel += inst->Size();		
	} while (!inst->IsEnd());
	Diag::Trace(DIAGsegment, "End of phase2 rel=" + to_hexstring(rel));
}

void DataSegment::WriteC(std::ostream& os, int indent) const
{
	os << "/* DATA SEGMENT (not yet) */" << std::endl;
}

/**********************************************************************/

void BSSSegment::phase2(int rel)
{
	Instr inst;
	Diag::Trace(DIAGsegment, "Phase2 bss rel=" + to_hexstring(rel));
	do {
		DByte b = DByteAt(rel);
//		DIAGstream << "*** Follow: 0x" << std::hex << rel << std::endl;
		if (b->IsFlag(DB_IHEAD))
			return;
		if (b->IsFlag(DB_ISUCC)) {
			Diag::Info(DIAGsegment, "Into instr rel=" + to_hexstring(rel));
			return;
		}
		
		/* uninitialized bytes until next known symbol */
		int ad = rel;
		do {
			ad++;
		} while (ad < Size() && (b=DByteAt(ad))->Sym()==0);

		inst = InstrFactory::Instance().MakeBss(this, rel, ad-rel);
		Diag::Info(DIAGsegment, "At " + to_hexstring(rel) +
			" inst=" + inst->toAsmString());
		
		rel += inst->Size();		
	} while (rel < Size());
	Diag::Trace(DIAGsegment, "End of phase2 rel=" + to_hexstring(rel));
}

void BSSSegment::WriteC(std::ostream& os, int indent) const
{
	os << "/* BSS SEGMENT (not yet) */" << std::endl;
}

/**********************************************************************/

SegmentFactory *SegmentFactory::_instance = 0;

SegmentFactory *SegmentFactory::Instance()
{
	if (!SegmentFactory::_instance)
		SegmentFactory::_instance = new SegmentFactory();
	return SegmentFactory::_instance;
}

SegmentFactory::SegmentFactory()
	: segs(3)
{
	segs[0] = segs[1] = segs[2] = 0;
}

Segment *SegmentFactory::MakeSegment(cString& nam, int base, int size)
{
	Diag::Trace(DIAGsegment, "MakeSegment " + nam);
	Segment *seg;
	if (nam == ".text")
		seg = segs[0] = new TextSegment(nam, base, size);
	else if (nam == ".data")
		seg = segs[1] = new DataSegment(nam, base, size);
	else if (nam == ".bss")
		seg = segs[2] = new BSSSegment(nam, base, size);
	else {
//		segs.push_back(seg = new Segment(nam, base, size));
		FATALERROR("Not yet handled");
		/* generic segment - not correctly handled yet */
	}
	return seg;
}

Segment *SegmentFactory::SegByName(cString& nam) const
{
	for (auto s : segs)
		if (s->Name() == nam) return s;
	return 0;
}

bool SegmentFactory::Disassemble()
{
	/* fix the unnamed relocs that point to same or other segs, 
	 * but are not global, such as case references
	 */
	 for (auto s : segs)
		if (!s->DisassPhase1()) return false;

	/* generate instructions and data items */
	for (auto s : segs)
		if (!s->DisassPhase2()) return false;

	return true;
}

bool SegmentFactory::Decompile()
{
	for (auto s : segs)
		if (!s->Decompile()) return false;
	return true;
}

void SegmentFactory::WriteAsm(std::ostream& os) const
{
	for (auto s : segs)
		s->WriteAsm(os);
}

void SegmentFactory::WriteC(std::ostream& os) const
{
	/* data segments first */
	segs[1]->WriteC(os, 0);
	segs[2]->WriteC(os, 0);
	segs[0]->WriteC(os, 0);
	/* XXX other segments? */
}
