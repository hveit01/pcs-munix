#include "instr.h"
#include "segment.h"

#if 0
InstrFactory *InstrFactory::_instance = 0;

InstrFactory *InstrFactory::Instance()
{
	if (!InstrFactory::_instance)
		InstrFactory::_instance = new InstrFactory();
	return InstrFactory::_instance;
}
#endif

static struct MatchTbl {
	int mask;
	int check;
	cString mnemo;
	OpType opcls;
	ExprOp exop;
} matchtbl[] = {
	{ 0xffff, 0x003c, "ori", 	OP_CCR,		EXOP_unknown },		// not in C code?
	{ 0xffff, 0x007c, "ori", 	OP_SR,		EXOP_unknown },		// not in C code?
	{ 0xff00, 0x0000, "ori",	OP_IMM,		EXOP_ORASN },
	{ 0xffff, 0x023c, "andi",	OP_CCR,		EXOP_unknown  },	// not in C code?
	{ 0xffff, 0x027c, "andi",	OP_SR,		EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x0200, "andi", 	OP_IMM,		EXOP_ANDASN },
	{ 0xff00, 0x0400, "subi", 	OP_IMM,		EXOP_SUBASN },
	{ 0xfff0, 0x06c0, "rtm",	OP_RTM,		EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x06c0, "callm",	OP_CALLM,	EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x0600, "addi",	OP_IMM,		EXOP_ADDASN },
	{ 0xf9c0, 0x00c0, "cmp2",	OP_CMPCHK2,	EXOP_unknown  },	// not in C code?
	{ 0xffff, 0x0a3c, "eori",	OP_CCR,		EXOP_unknown  },	// not in C code?
	{ 0xffff, 0x0a7c, "eori",	OP_SR,		EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x0a00, "eori", 	OP_IMM,		EXOP_XORASN  },
	{ 0xff00, 0x0c00, "cmpi", 	OP_IMM,		EXOP_cmp  },
	{ 0xffc0, 0x0800, "btst", 	OP_BITI,	EXOP_unknown  },
	{ 0xffc0, 0x0840, "bchg", 	OP_BITI,	EXOP_unknown  },
	{ 0xffc0, 0x0880, "bclr", 	OP_BITI,	EXOP_unknown  },
	{ 0xffc0, 0x08c0, "bset", 	OP_BITI,	EXOP_unknown  },
	{ 0xff00, 0x0e00, "moves",	OP_MOVES,	EXOP_unknown  },	// not in C code?
	{ 0xf9ff, 0x08fc, "cas2",	OP_CAS2,	EXOP_unknown  },	// not in C code?
	{ 0xf9c0, 0x08c0, "cas",	OP_CAS,		EXOP_unknown  },	// not in C code?
	{ 0xf1c0, 0x0100, "btst",	OP_BITD,	EXOP_unknown  },
	{ 0xf1c0, 0x0140, "bchg",	OP_BITD,	EXOP_unknown  },
	{ 0xf1c0, 0x0180, "bclr",	OP_BITD,	EXOP_unknown  },
	{ 0xf138, 0x0108, "movep",	OP_MOVEP,	EXOP_unknown  },	// not in C code?
	{ 0xf000, 0x1000, "move",	OP_MOVEB,	EXOP_unknown  },
	{ 0xf1c0, 0x2040, "movea",	OP_MOVEL,	EXOP_unknown  },
	{ 0xf000, 0x2000, "move",	OP_MOVEL,	EXOP_unknown  },
	{ 0xf000, 0x3040, "movea",	OP_MOVEW,	EXOP_unknown  },
	{ 0xf000, 0x3000, "move",	OP_MOVEW,	EXOP_unknown  },
	{ 0xffc0, 0x40c0, "move",	OP_MVSTS1,	EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x42c0, "move",	OP_MVSTC1,	EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x4000, "negx",	OP_MONOPX,	EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x4200, "clr",	OP_MONOPX,	EXOP_clr  },
	{ 0xffc0, 0x44c0, "move",	OP_MVSTC2,	EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x4400, "neg",	OP_MONOPX,	EXOP_MINUS },
	{ 0xffc0, 0x46c0, "move",	OP_MVSTS2,	EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x4600, "not",	OP_MONOPX,	EXOP_BITNOT  },	
	{ 0xffb8, 0x4880, "ext",	OP_EXTX,	EXOP_CAST2 },
	{ 0xffb8, 0x4980, "extb",	OP_EXTX,	EXOP_CAST4 },
	{ 0xfff8, 0x4808, "link",	OP_LINKX,	EXOP_unknown  },
	{ 0xffc0, 0x4800, "nbcd",	OP_MONOP,	EXOP_unknown  },	// not in C code?
	{ 0xfff8, 0x4840, "swap",	OP_SWAP,	EXOP_unknown  },
	{ 0xfff8, 0x4848, "bkpt",	OP_BKPT,	EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x4840, "pea ",	OP_PEA,		EXOP_unknown  },
	{ 0xffff, 0x4afa, "bgnd",	OP_IMPLIED,	EXOP_unknown  },	// not in C code?
	{ 0xffff, 0x4afc, "illegal",OP_IMPLIED,	EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x4ac0, "tas",	OP_MONOP,	EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x4a00, "tst",	OP_MONOPX,	EXOP_tst },
	{ 0xffc0, 0x4c00, "mul", 	OP_MULDIVL,	EXOP__MUL },
	{ 0xffc0, 0x4c40, "div", 	OP_MULDIVL,	EXOP__DIV },
	{ 0xfff0, 0x4e40, "trap",	OP_TRAP,	EXOP_unknown  },	// not in C code?
	{ 0xfff8, 0x4e50, "link",	OP_LINKX,	EXOP_unknown  },
	{ 0xfff8, 0x4e58, "unlk",	OP_UNLK,	EXOP_unknown  },
	{ 0xfff8, 0x4e60, "move",	OP_USP,		EXOP_unknown  },
	{ 0xffff, 0x4e70, "reset",	OP_IMPLIED,	EXOP_unknown  },	// not in C code?
	{ 0xffff, 0x4e71, "nop",	OP_IMPLIED,	EXOP_unknown  },	// not in C code?
	{ 0xffff, 0x4e72, "stop",	OP_IARG,	EXOP_unknown  },	// not in C code?
	{ 0xffff, 0x4e73, "rte",	OP_IMPLIEDEND,	EXOP_unknown  },	// not in C code?
	{ 0xffff, 0x4e74, "rtd",	OP_IARGEND,	EXOP_unknown  },	// not in C code?
	{ 0xffff, 0x4e75, "rts",	OP_IMPLIEDEND,	EXOP_null  },
	{ 0xffff, 0x4e76, "trapv",	OP_IMPLIED,	EXOP_unknown  },	// not in C code?
	{ 0xffff, 0x4e77, "rtr",	OP_IMPLIEDEND,	EXOP_unknown  },	// not in C code?
	{ 0xfffe, 0x4e7a, "movec",	OP_MOVEC,	EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x4e80, "jsr ",	OP_MONOP,	EXOP_func },
	{ 0xffc0, 0x4ec0, "jmp ",	OP_JUMP,	EXOP_unknown  },
	{ 0xfb80, 0x4880, "movem",	OP_MOVEM,	EXOP_unknown  },
	{ 0xf1c0, 0x41c0, "lea ",	OP_LEA,		EXOP_unknown  },
	{ 0xf040, 0x4040, "chk ",	OP_CHK,		EXOP_unknown  },	// not in C code?
	{ 0xfff8, 0x50c8, "dbt",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x51c8, "dbf",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x52c8, "dbhi",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x53c8, "dbls",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x54c8, "dbcc",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x55c8, "dbcs",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x56c8, "dbne",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x57c8, "dbeq",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x58c8, "dbvc",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x59c8, "dbvs",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x5ac8, "dbpl",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x5bc8, "dbmi",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x5cc8, "dbge",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x5dc8, "dblt",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x5ec8, "dbgt",	OP_DBCC,	EXOP_unknown  },
	{ 0xfff8, 0x5fc8, "dble",	OP_DBCC,	EXOP_unknown  },
	{ 0xf0ff, 0x50fa, "trapcc",	OP_IARGW,	EXOP_unknown  },	// not in C code?
	{ 0xf0ff, 0x50fb, "trapcc",	OP_IARGL,	EXOP_unknown  },	// not in C code?
	{ 0xf0ff, 0x50fc, "trapcc",	OP_IMPLIED,	EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x50c0, "st",		OP_SCC,		EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x51c0, "sf",		OP_SCC,		EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x52c0, "shi",	OP_SCC,		EXOP_UGT },	// unsigned
	{ 0xffc0, 0x53c0, "sls",	OP_SCC,		EXOP_ULE },	// unsigned
	{ 0xffc0, 0x54c0, "scc/shs",	OP_SCC,		EXOP_UGE },	// unsigned
	{ 0xffc0, 0x55c0, "scs/slo",	OP_SCC,		EXOP_ULT },	// unsigned
	{ 0xffc0, 0x56c0, "sne",	OP_SCC,		EXOP_NE },
	{ 0xffc0, 0x57c0, "seq",	OP_SCC,		EXOP_EQ },
	{ 0xffc0, 0x58c0, "svc",	OP_SCC,		EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x59c0, "svs",	OP_SCC,		EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x5ac0, "spl",	OP_SCC,		EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x5bc0, "smi",	OP_SCC,		EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0x5cc0, "sge",	OP_SCC,		EXOP_SGE },
	{ 0xffc0, 0x5dc0, "slt",	OP_SCC,		EXOP_SLT },
	{ 0xffc0, 0x5ec0, "sgt",	OP_SCC,		EXOP_SGT },
	{ 0xffc0, 0x5fc0, "sle",	OP_SCC,		EXOP_SLE },
	{ 0xf100, 0x5000, "addq",	OP_QUICK,	EXOP_ADDASN  },
	{ 0xf100, 0x5100, "subq",	OP_QUICK,	EXOP_SUBASN  },
	{ 0xff00, 0x6000, "bra",	OP_BRXEND,	EXOP_unknown  },
	{ 0xff00, 0x6100, "bsr",	OP_BRX,		EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x6200, "bhi",	OP_BRX,		EXOP_UGT },	// unsigned
	{ 0xff00, 0x6300, "bls",	OP_BRX,		EXOP_ULE },	// unsigned
	{ 0xff00, 0x6400, "bcc/bhs",	OP_BRX,		EXOP_UGE },	// unsigned bhs
	{ 0xff00, 0x6500, "bcs/blo",	OP_BRX,		EXOP_ULT },	// unsigned blo
	{ 0xff00, 0x6600, "bne",	OP_BRX,		EXOP_NE },
	{ 0xff00, 0x6700, "beq",	OP_BRX,		EXOP_EQ },
	{ 0xff00, 0x6800, "bvc",	OP_BRX,		EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x6900, "bvs",	OP_BRX,		EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x6a00, "bpl",	OP_BRX,		EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x6b00, "bmi",	OP_BRX,		EXOP_unknown  },	// not in C code?
	{ 0xff00, 0x6c00, "bge",	OP_BRX,		EXOP_SGE },
	{ 0xff00, 0x6d00, "blt",	OP_BRX,		EXOP_SLT },
	{ 0xff00, 0x6e00, "bgt",	OP_BRX,		EXOP_SGT },
	{ 0xff00, 0x6f00, "ble",	OP_BRX,		EXOP_SLE },
	{ 0xf100, 0x7000, "moveq",	OP_MOVEQ,	EXOP_unknown  },
	{ 0xf1c0, 0x80c0, "divu",	OP_MULDIVW,	EXOP_UDIV },
	{ 0xf1c0, 0x81c0, "divs",	OP_MULDIVW,	EXOP_SDIV },
	{ 0xf1f0, 0x8100, "sbcd",	OP_BCD,		EXOP_unknown  },	// not in C code?
	{ 0xf1f0, 0x8140, "pack",	OP_BCD3,	EXOP_unknown  },	// not in C code?
	{ 0xf1f0, 0x8180, "unpk",	OP_BCD3,	EXOP_unknown  },	// not in C code?
	{ 0xf000, 0x8000, "or",		OP_ALU,		EXOP_ORASN  },
	{ 0xf0c0, 0x90c0, "suba",	OP_ALUA,	EXOP_SUBASN },
	{ 0xf130, 0x9100, "subx",	OP_BCDX,	EXOP_unknown  },	// not in C code?
	{ 0xf000, 0x9000, "sub",	OP_ALU,		EXOP_SUBASN },
	{ 0xf000, 0xa000, "aemul",	OP_DC,		EXOP_unknown  },	// not in C code?
	{ 0xf0c0, 0xb0c0, "cmpa",	OP_ALUA,	EXOP_cmp },
	{ 0xf138, 0xb108, "cmpm",	OP_CMPM,	EXOP_unknown  },	// not in C code?
	{ 0xf100, 0xb000, "cmp",	OP_ALU,		EXOP_cmp },
	{ 0xf000, 0xb000, "eor",	OP_ALU,		EXOP_XORASN },
	{ 0xf1c0, 0xc0c0, "mulu",	OP_MULDIVW,	EXOP_UMUL },
	{ 0xf1c0, 0xc1c0, "muls",	OP_MULDIVW,	EXOP_SMUL },
	{ 0xf1f0, 0xc100, "abcd",	OP_BCD,		EXOP_unknown  },	// not in C code?
	{ 0xf1f8, 0xc140, "exg",	OP_EXG,		EXOP_unknown  },
	{ 0xf1f8, 0xc148, "exg",	OP_EXG,		EXOP_unknown  },
	{ 0xf1f8, 0xc188, "exg",	OP_EXG,		EXOP_unknown  },
	{ 0xf000, 0xc000, "and",	OP_ALU,		EXOP_ANDASN  },
	{ 0xf0c0, 0xd0c0, "adda",	OP_ALUA,	EXOP_ADDASN  },
	{ 0xf130, 0xd100, "addx",	OP_BCDX,	EXOP_unknown  },	// not in C code?
	{ 0xf000, 0xd000, "add",	OP_ALU,		EXOP_ADDASN  },
	{ 0xffc0, 0xe0c0, "asr",	OP_SHFT,	EXOP_SRSHASN },
	{ 0xffc0, 0xe1c0, "asl",	OP_SHFT,	EXOP_LSHASN },
	{ 0xffc0, 0xe2c0, "lsr",	OP_SHFT,	EXOP_URSHASN },
	{ 0xffc0, 0xe3c0, "lsl",	OP_SHFT,	EXOP_LSHASN },
	{ 0xffc0, 0xe4c0, "roxr",	OP_SHFT,	EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0xe5c0, "roxl",	OP_SHFT,	EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0xe6c0, "ror",	OP_SHFT,	EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0xe7c0, "rol",	OP_SHFT,	EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0xe8c0, "bftst",	OP_BF,		EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0xe9c0, "bfextu",	OP_BFD,		EXOP_unknown  },	// C bitfield
	{ 0xffc0, 0xeac0, "bfchg",	OP_BF,		EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0xebc0, "bfexts",	OP_BFD,		EXOP_unknown  },	// not in C code?
	{ 0xffc0, 0xecc0, "bfclr",	OP_BF,		EXOP_zeroes },		// C bitfield
	{ 0xffc0, 0xedc0, "bfffo",	OP_BFD,		EXOP_unknown  },	// not in C code
	{ 0xffc0, 0xeec0, "bfset",	OP_BF,		EXOP_ones },		// C bitfield
	{ 0xffc0, 0xefc0, "bfins",	OP_BFS,		EXOP_unknown  },	// C bitfield
	{ 0xf118, 0xe000, "asr",	OP_SHFTX,	EXOP_SRSHASN },
	{ 0xf118, 0xe100, "asl",	OP_SHFTX,	EXOP_LSHASN },
	{ 0xf118, 0xe008, "lsr",	OP_SHFTX,	EXOP_URSHASN },
	{ 0xf118, 0xe108, "lsl",	OP_SHFTX,	EXOP_LSHASN },
	{ 0xf118, 0xe010, "roxr",	OP_SHFTX,	EXOP_unknown  },	// not in C code?
	{ 0xf118, 0xe110, "roxl",	OP_SHFTX,	EXOP_unknown  },	// not in C code?
	{ 0xf118, 0xe018, "ror",	OP_SHFTX,	EXOP_unknown  },	// not in C code?
	{ 0xf118, 0xe118, "rol",	OP_SHFTX,	EXOP_unknown  },	// not in C code?
	{ 0xf000, 0xf000, "femul",	OP_DC,		EXOP_unknown  },
	{ 0,      0,      "INVALID",OP_INVALID,	EXOP_unknown  },	// catchall
};

const MatchTbl *InstrFactory::match(int opcode) const
{
	MatchTbl *mp = &matchtbl[0];
	while (mp->mask) {
		if ((opcode & mp->mask) == mp->check)
			return mp;
		mp++;
	}
	return mp; /* last element */
}

cString& InstrFactory::GetMnemo(Item op) const
{
	const MatchTbl *mp = match(op->Value());
	return mp->mnemo;
}

Instr InstrFactory::MakeInst(Segment *seg, int rel)
{
	ItemWord op = seg->WordAt(rel);
	_Instr::SetFocus(seg);

	const MatchTbl *mp = match(op->Value());
	
//	std::cout << "Opcode=" << std::hex << op->Value() << std::dec << " opcls=" << mp->opcls << std::endl;

	_Instr *inst = 0;
	switch(mp->opcls) {
	default:
		FATALERROR("InstFactory::default: rel="+std::to_string((int)rel) +
			" Opcode="+to_hexstring(op->Value())+" opcls="+std::to_string((int)mp->opcls));
	case OP_CCR:
		inst = new NCC(Ccr)(op); break;
	case OP_SR:
		inst = new NCC(Sr)(op); break;
	case OP_IMM:
		inst = new ImmInstr(op, mp->exop); break;
	case OP_RTM:
		inst = new NCC(Rtm)(op); break;
	case OP_CALLM:
		inst = new NCC(Callm)(op); break;
	case OP_CMPCHK2:
		inst = new NCC(CmpChk2)(op); break;
	case OP_BITI:
		inst = new BitiInstr(op); break;
	case OP_MOVES:
		inst = new NCC(Moves)(op); break;
	case OP_CAS2:
		inst = new Cas2Instr(op); break;
	case OP_CAS:
		inst = new NCC(Cas)(op); break;
	case OP_BITD:
		inst = new NCC(Bitd)(op); break;
	case OP_MOVEP:
		inst = new NCC(Movep)(op); break;
	case OP_MOVEB:
		inst = new MoveInstr(op, OPWbyte); break;
	case OP_MOVEW:
		inst = new MoveInstr(op, OPWword); break;
	case OP_MOVEL:
		inst = new MoveInstr(op, OPWlong); break;
	case OP_MVSTS1:
		inst = new NCC(MoveSR0)(op); break;
	case OP_MVSTC1:
		inst = new NCC(MoveCCR0)(op); break;
	case OP_MVSTS2:
		inst = new NCC(MoveSR1)(op); break;
	case OP_MVSTC2:
		inst = new NCC(MoveCCR1)(op); break;
	case OP_MONOPX:
		inst = new MonopXInstr(op, mp->exop); break;
	case OP_EXTX:
		inst = new ExtXInstr(op, mp->exop); break;
	case OP_LINKX:
		inst = new LinkXInstr(op); break;
	case OP_MONOP:
		inst = new MonopInstr(op, mp->exop); break;
	case OP_SWAP:
		inst = new NCC(Swap)(op); break;
	case OP_BKPT:
		inst = new NCC(Bkpt)(op); break;
	case OP_PEA:
		inst = new PeaInstr(op); break;
	case OP_IMPLIEDEND:
		inst = new ImpliedInstr(op, mp->exop);
		inst->SetEnd();
		inst->SetEndBB();
		break;
	case OP_IMPLIED:
		inst = new ImpliedInstr(op, mp->exop); break;
	case OP_MULDIVL:
		inst = new MulDivLInstr(op, mp->exop); break;
	case OP_TRAP:
		inst = new NCC(Trap)(op); break;
	case OP_UNLK:
		inst = new UnlkInstr(op); break;
	case OP_USP:
		inst = new NCC(MoveUsp)(op); break;
	case OP_IARGEND:
		inst = new IArgInstr(op);
		inst->SetEnd();
		inst->SetEndBB();
		break;
	case OP_IARG:
		inst = new IArgInstr(op); break;
	case OP_MOVEC:
		inst = new NCC(Movec)(op); break;
	case OP_MOVEM:
		inst = new MovemInstr(op); break;
	case OP_LEA:
		inst = new LeaInstr(op); break;
	case OP_CHK:
		inst = new NCC(Chk)(op); break;
	case OP_QUICK:
		inst = new QuickInstr(op, mp->exop); break;
	case OP_DBCC:
		inst = new DbccInstr(op);
		break;
	case OP_IARGW:
		inst = new IArgInstr(op, 1); break;
	case OP_IARGL:
		inst = new IArgInstr(op, 2); break;
	case OP_SCC:
		inst = new SccInstr(op, mp->exop); break;
	case OP_BRXEND:
		inst = new BrxXInstr(op, mp->exop);
		inst->SetEnd();
		inst->SetEndBB();
		break;
	case OP_BRX:
		inst = new BrxXInstr(op, mp->exop); break;
	case OP_MOVEQ:
		inst = new MoveqInstr(op); break;
	case OP_MULDIVW:
		inst = new MulDivWInstr(op, mp->exop); break;
	case OP_BCD:
		inst = new NCC(BcdX0)(op); break;
	case OP_BCDX:
		inst = new NCC(BcdX1)(op); break;
	case OP_BCD3:
		inst = new NCC(Bcd3)(op); break;
	case OP_ALU:
		inst = new AluInstr(op, mp->exop); break;
	case OP_ALUA:
		inst = new AluAInstr(op, mp->exop); break;
	case OP_DC:
		inst = new DcInstr(op); break;
	case OP_CMPM:
		inst = new NCC(Cmpm)(op); break;
	case OP_SHFT:
		inst = new ShftInstr(op, mp->exop); break;
	case OP_SHFTX:
		inst = new ShftXInstr(op, mp->exop); break;
	case OP_BF:
		inst = new BfInstr(op, mp->exop); break;
	case OP_BFD:
		inst = new BfdInstr(op); break;
	case OP_BFS:
		inst = new BfsInstr(op); break;
	case OP_JUMP:
		inst = new JumpInstr(op); break;
	}

	Instr ip(inst); // translate into shared ptr
//	ip->DumpAsm(std::cout);
	return seg->AddInstr(ip);
}

Instr InstrFactory::MakeCaseVal(Segment *seg, int rel, int csz)
{
	_Instr::SetFocus(seg);
	Instr ip = std::make_shared<_DataInstr>(rel, csz);
	return seg->AddInstr(ip);
}

Instr InstrFactory::MakeCaseTgt(Segment *seg, int rel)
{
	_Instr::SetFocus(seg);
	Instr ip = std::make_shared<_DataInstr>(rel, -4);
	return seg->AddInstr(ip);
}


Instr InstrFactory::MakeData(Segment *seg, int rel)
{
	_Instr::SetFocus(seg);
	Instr ip = std::make_shared<_DataInstr>(rel);
	return seg->AddInstr(ip);
}

Instr InstrFactory::MakeBss(Segment *seg, int rel, int sz)
{
	_Instr::SetFocus(seg);
	Instr ip = std::make_shared<_BssInstr>(rel, sz);
	return seg->AddInstr(ip);
}
