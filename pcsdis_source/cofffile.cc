#include "cofffile.h"
#include "symbol.h"

/* this could be made more universal, sure,
 * but I only want to work on PCS object files for now, so
 * I won't check f_flags for byte order
 */

bool COFFFile::Read()
{
	/* read header */
	if (!readheader()) return false;

	/* advance to symbol table, and read symbols and nametable */
	is->seekg(filhdr.f_symptr, is->beg);
	check_stream("COFFFile::Read");

	if (!readsyms()) return false;
	if (!readnames()) return false; /* will read to end */
	
	/* jump back to sections after header , and read sections */
	is->clear();
	is->seekg(sizeof(FILHDR), is->beg);
	if (!readsegments()) return false;

	enqueuesyms();
//	dosegments();
	
	return true;
}

void COFFFile::dumpfilhdr(std::ostream& os) const
{
	if (Diag::NoDiag(DIAGfileread)) return;

	dprint(os, "FILHDR:", "");
	dprint(os, "  f_magic:  ", filhdr.f_magic);
	dprint(os, "  f_nscns:  ", filhdr.f_nscns);
	dprint(os, "  f_timdat: ", filhdr.f_timdat);
	dprint(os, "  f_symptr: ", filhdr.f_symptr);
	dprint(os, "  f_nsyms:  ", filhdr.f_nsyms);
	dprint(os, "  f_opthdr: ", filhdr.f_opthdr);
	dprint(os, "  f_flags:  ", filhdr.f_flags);
}

void COFFFile::Dump(std::ostream& os) const
{
	dumpfilhdr(os);
}

void COFFFile::dumpheader(std::ostream& os, const SCNHDR& hdr) const
{
	if (Diag::NoDiag(DIAGfileread)) return;

	dprint(os, "SCNHDR:", "");
	dprint(os, "  s_paddr:   ", hdr.s_paddr);
	dprint(os, "  s_vaddr:   ", hdr.s_vaddr);
	dprint(os, "  s_size:    ", hdr.s_size);
	dprint(os, "  s_scnptr:  ", hdr.s_scnptr);
	dprint(os, "  s_relptr:  ", hdr.s_relptr);
	dprint(os, "  s_lnnoptr: ", hdr.s_lnnoptr);
	dprint(os, "  s_nreloc:  ", hdr.s_nreloc);
	dprint(os, "  s_nlnno:   ", hdr.s_nlnno);
	dprint(os, "  s_s_flags: ", hdr.s_flags);
}


bool COFFFile::readheader()
{
	Diag::Trace(DIAGfileread, "readheader");
	is->read((char*)&filhdr, sizeof(filhdr));
	check_stream("COFFFile::readheader");

	int magic = filhdr.f_magic;

	setswap(true);
	if (swap) {
		swap2(filhdr.f_magic);
		magic = filhdr.f_magic;
		swap2(filhdr.f_nscns);
		swap4(filhdr.f_timdat);
		swap4(filhdr.f_symptr);
		swap4(filhdr.f_nsyms);
		swap2(filhdr.f_opthdr);
		swap2(filhdr.f_flags);
	}
	if (magic != PCSMAGIC && magic != MC68MAGIC)
		FATALERROR("Sorry, Filetype not supported yet, magic=" + to_hexstring(magic));

	if (filhdr.f_opthdr != 0)
		FATALERROR("Opthdr not supported yet");

	dumpfilhdr(DIAGstream);
	return true;
}

char *COFFFile::readsym()
{
	char *name = 0;
	is->read((char*)&syment, sizeof(SYMENT));
	if (swap) {
		if (syment.n_name[0] == 0) {
			swap4(syment.n_zeroes);
			swap4(syment.n_offset);
		}
		swap4(syment.n_value);
		swap2(syment.n_scnum);
		swap2(syment.n_type);
	}

	if (syment.n_name[0])
		name = Strdup8(syment.n_name);
	return name;
}

char *COFFFile::readaux()
{
	char *name = 0;
	is->read((char*)&auxent, sizeof(AUXENT));
	if (swap) {
		switch (syment.n_sclass) {
		case C_FILE:	/* x_fname */
			name = Strdup14(auxent.x_file.x_fname);
			break;
		case C_STAT:
		case C_EXT: /*x_scn */
			swap4(auxent.x_scn.x_scnlen);
			swap2(auxent.x_scn.x_nreloc);
			swap2(auxent.x_scn.x_nlinno);
			break;
		case C_STRTAG:	/* debug info: start of a structure */
		case C_MOS:		/* debug info: member of a structure */
		case C_EOS:		/* debug info: end of a structure */
		case C_TPDEF:	/* type definition */
		case C_UNTAG:	/* union tag */
		case C_ENTAG:	/* enum tag */
		case C_MOE	:	/* enum member */
		case C_FIELD:	/* bit field */
		case C_REGPARM:	/* register param */
		case C_ARG:		/* function arg */
		case C_BLOCK:	/* start/end bloock */
		case C_FCN:		/* start/end function */
			break;
		default:
			FATALERROR("Default case sclass=" + std::to_string(syment.n_sclass));
		}
	}
	return name;
}

bool COFFFile::readsyms()
{
	Diag::Trace(DIAGfileread,"readsyms");

	SymbolTable *st = SymbolTable::Instance();
	
	bool isstatic = false;
	int oldsclass;
	int auxcnt = 0;
	for (int i = 0; i < filhdr.f_nsyms; i++) {
		int idx = st->SymbolCnt();
		char *name;
		if (auxcnt == 0) {
			name = readsym();
			auxcnt = syment.n_numaux; // number of aux records following
			isstatic = syment.n_sclass == C_STAT;
			oldsclass = syment.n_sclass;
		} else {
			name = readaux();
			auxcnt--;
			if (name && syment.n_sclass==C_FILE)
				st->SymbolByIdx(idx-1)->SetName(name); // fix previous name
			else
				syment.n_offset = -1; // aux record for section data
			syment.n_sclass = 0;
		}
		/* ignore function or block debug info for now */

		Symbol sym = name ?
			st->AddSymbol(name, syment.n_scnum, idx, syment.n_value) :
			st->AddSymbol(syment.n_offset, syment.n_scnum, idx, syment.n_value);
		if (isstatic) sym->SetVisibility(SYMV_Static);
		
		if (Diag::IsDiag(DIAGsymbol) || Diag::IsDiag(DIAGfileread))
			sym->Dump(DIAGstream);

		check_stream("COFFFile::readsyms");
	}
	return true;
}

bool COFFFile::readnames()
{
	Diag::Trace(DIAGfileread,"readnames");

	int32_t sz;
	SymbolTable *st = SymbolTable::Instance();

	if (is->eof())	/* if at end after last read (readsyms) then there is no nametable at all */
		return true;

	is->read((char*)&sz, sizeof(int32_t)); // get number of entries

	swap4(sz);
	if (sz) {
		char *nametbl = new char[sz+sizeof(int32_t)];
		is->read(nametbl+sizeof(int32_t), sz);

		/* fix the symbols */
		for (int i=0; i<filhdr.f_nsyms; i++) {
			Symbol sym = st->SymbolByIdx(i);
			const char *name;
			if (sym->Secnum() == -2) // file record
				name = st->SymbolName(sym->Idx()+1);
			else if (sym->HasNoName() && sym->NameIdx() != -1) // name is in nametbl
				name = &nametbl[sym->NameIdx()];
			else continue;
			sym->SetName(name);
		}
//		delete nametbl;
	}
	
	return true;
}

bool COFFFile::readsegment()
{
	Diag::Trace(DIAGfileread, "readsegment");

	/* read reader */
	SCNHDR hdr;
	is->read((char*)&hdr, sizeof(SCNHDR));
	check_stream("COFFFile::readsegment");

	if (swap) {
		swap4(hdr.s_paddr);
		swap4(hdr.s_vaddr);
		swap4(hdr.s_size);
		swap4(hdr.s_scnptr);
		swap4(hdr.s_relptr);
		swap4(hdr.s_lnnoptr);
		swap2(hdr.s_nreloc);
		swap2(hdr.s_nlnno);
		swap4(hdr.s_flags);
	}
/*	if (hdr.s_lnnoptr != 0)
		FATALERROR("Not yet: s_lnnoptr!=0");*/
/*	if (hdr.s_nlnno != 0)
		FATALERROR("Not yet: s_nlnno!=0");*/
	if (hdr.s_paddr != hdr.s_vaddr)
		FATALERROR("Not yet: s_paddr!=s_vaddr");
	
	dumpheader(DIAGstream, hdr);

	char nbuf[16]; /*long enough */
	const char *np = hdr.s_name;
	int i;
	for (i=0; i<8 && np[i]; i++)
		nbuf[i] = np[i];
	nbuf[i] = 0;

	SegmentFactory *sf = SegmentFactory::Instance();
	Segment *seg = sf->MakeSegment(nbuf, hdr.s_vaddr, hdr.s_size);

	if (!readdata(seg, hdr.s_scnptr, hdr.s_size)) return false;
	return readrelocs(seg, hdr.s_relptr, hdr.s_nreloc);
}

bool COFFFile::readdata(Segment *seg, int pos, int sz)
{
	if (pos == 0 && sz > 0) return true;	// don't read .BSS from file
	
	std::streampos oldpos = is->tellg();
	is->seekg(pos, is->beg);

	for (int addr = 0; addr < sz; addr++) {
		int byte = is->get();
		check_stream("COFFFile::readdata(1)");
		auto sdbyte = std::make_shared<_DByte>(addr, byte);
		seg->AddByte(sdbyte); // add byte to segment
	}
	is->seekg(oldpos, is->beg);
	check_stream("COFFFile::readdata(2)");
	return true;
}

Reloc COFFFile::readreloc()
{
	RELOC rel;
	is->read((char*)&rel, sizeof(RELOC));
	check_stream("COFFFile::readreloc");
	
	if (swap) {
		swap4(rel.r_vaddr);
		swap4(rel.r_symndx);
		swap2(rel.r_type);
	}
	
	SymbolTable *st = SymbolTable::Instance();
	Symbol sym = st->SymbolByIdx(rel.r_symndx);
	return std::make_shared<_Reloc>(sym, rel.r_vaddr, rel.r_type);
}

bool COFFFile::readrelocs(Segment *seg, int pos, int sz)
{
	Diag::Trace(DIAGfileread, "readrelocs");

	std::streampos oldpos = is->tellg();
	is->seekg(pos, is->beg);

	for (int i = 0; i < sz; i++) {
		Reloc rel = readreloc();
		if (!rel) return false;
		if (Diag::IsDiag(DIAGreloc))
			rel->Dump(DIAGstream);
		seg->AddReloc(rel);
	}
	is->seekg(oldpos, is->beg);

	Diag::Trace(DIAGfileread, "readrelocs done for " + seg->Name());
	if (Diag::IsDiag(DIAGfileread))
		seg->DumpRelocs(DIAGstream);

	check_stream("COFFFile::readrelocs");
	return true;
}

bool COFFFile::readsegments()
{
	Diag::Trace(DIAGfileread, "readsegments");

	for (int i=0; i < filhdr.f_nscns; i++)
		if (!readsegment()) return false;
	
	return true;
}
