#include "exprfactory.h"
#include "instr.h"

static const ExopTbl optable[] = {
	{ EXOP_func,	250,	EXASSOCltor,	0,		0,		0 },
	{ EXOP_scale,	200,	EXASSOCltor,	0,		"<scale>",0 },
	{ EXOP_bit,		200,	EXASSOCltor,	0,		"<bit>",0 },
	{ EXOP_push,	200,	EXASSOCltor,	"<push> ",0,		0 },
	{ EXOP_PAREN,	150,	EXASSOCltor,	"(",	0,		")"	},
	{ EXOP_BRACKET,	150,	EXASSOCltor,	"[",	0,		"]"	},
	{ EXOP_PTROFF,	150,	EXASSOCltor,	0,		"->", 	0 },
	{ EXOP_OFF,		150,	EXASSOCltor,	0,		".",	0 },
	{ EXOP_BOOLNOT,	140,	EXASSOCrtol,	"!",	0,		0 },
	{ EXOP_BITNOT,	140,	EXASSOCrtol,	"~",	0,		0 },
	{ EXOP_LINC,	140,	EXASSOCrtol,	"++",	0,		0 },
	{ EXOP_RINC,	140,	EXASSOCrtol,	0,		0,		"++" },
	{ EXOP_LDEC,	140,	EXASSOCrtol,	"--",	0,		0 },
	{ EXOP_RDEC,	140,	EXASSOCrtol,	0,		0,		"--" },
	{ EXOP_PLUS,	140,	EXASSOCrtol,	"+",	0,		0 },
	{ EXOP_MINUS,	140,	EXASSOCrtol,	"-",	0,		0 },
	{ EXOP_DEREF,	140,	EXASSOCrtol,	"*",	0,		0 },
	{ EXOP_ADDR,	140,	EXASSOCrtol,	"&",	0,		0 },
	{ EXOP_CAST2,	140,	EXASSOCrtol,	"(short)",	0,	0 },
	{ EXOP_CAST4,	140,	EXASSOCrtol,	"(long)",	0,	0 },
	{ EXOP_SIZEOF,	140,	EXASSOCrtol,	"sizeof(",0,	")" },
	{ EXOP_SMUL,	130,	EXASSOCltor,	0,		" * ",	0 },
	{ EXOP_SDIV,	130,	EXASSOCltor,	0,		" / ",	0 },
	{ EXOP_SMOD,	130,	EXASSOCltor,	0,		" % ",	0 },
	{ EXOP_UMUL,	130,	EXASSOCltor,	0,		" U* ",	0 },
	{ EXOP_UDIV,	130,	EXASSOCltor,	0,		" U/ ",	0 },
	{ EXOP_UMOD,	130,	EXASSOCltor,	0,		" U% ",	0 },
	{ EXOP_ADD,		120,	EXASSOCltor,	0,		" + ",	0 },
	{ EXOP_ADD,		120,	EXASSOCltor,	0,		" + ",	0 },
	{ EXOP_SUB,		120,	EXASSOCltor,	0,		" - ",	0 },
	{ EXOP_LSH,		110,	EXASSOCltor,	0,		" << ",	0 },
	{ EXOP_URSH,	110,	EXASSOCltor,	0,		" U>> ",	0 },
	{ EXOP_SRSH,	110,	EXASSOCltor,	0,		" >> ",	0 },
	{ EXOP_tst,		105,	EXASSOCltor,	"<tst> ",0,		0 },
	{ EXOP_cmp,		105,	EXASSOCltor,	0,		" <cmp> ",0 },
	{ EXOP_SLT,		100,	EXASSOCltor,	0,		" < ",	0 },
	{ EXOP_SLE,		100,	EXASSOCltor,	0,		" <= ",	0 },
	{ EXOP_SGT,		100,	EXASSOCltor,	0,		" > ",	0 },
	{ EXOP_SGE,		100,	EXASSOCltor,	0,		" >= ",	0 },
	{ EXOP_ULT,		100,	EXASSOCltor,	0,		" U< ",	0 },
	{ EXOP_ULE,		100,	EXASSOCltor,	0,		" U<= ",	0 },
	{ EXOP_UGT,		100,	EXASSOCltor,	0,		" U> ",	0 },
	{ EXOP_UGE,		100,	EXASSOCltor,	0,		" U>= ",	0 },
	{ EXOP_EQ,		90,		EXASSOCltor,	0,		" == ",	0 },
	{ EXOP_NE,		90,		EXASSOCltor,	0,		" != ",	0 },
	{ EXOP_BITAND,	80,		EXASSOCltor,	0,		" & ",	0 },
	{ EXOP_BITXOR,	70,		EXASSOCltor,	0,		" ^ ",	0 },
	{ EXOP_BITOR,	60,		EXASSOCltor,	0,		" | ",	0 },
	{ EXOP_BOOLAND,	50,		EXASSOCltor,	0,		" && ",	0 },
	{ EXOP_BOOLOR,	40,		EXASSOCltor,	0,		" || ",	0 },
	{ EXOP_TERNARY,	30,		EXASSOCrtol,	0,		" ? ",	" : " },
	{ EXOP_ASN,		20,		EXASSOCrtol,	0,		" = ",	0 },
	{ EXOP_ADDASN,	20,		EXASSOCrtol,	0,		" += ",	0 },
	{ EXOP_SUBASN,	20,		EXASSOCrtol,	0,		" -= ",	0 },
	{ EXOP_UMULASN,	20,		EXASSOCrtol,	0,		" U*= ",	0 },
	{ EXOP_UDIVASN,	20,		EXASSOCrtol,	0,		" U/= ",	0 },
	{ EXOP_UMODASN,	20,		EXASSOCrtol,	0,		" U%= ",	0 },
	{ EXOP_SMULASN,	20,		EXASSOCrtol,	0,		" *= ",	0 },
	{ EXOP_SDIVASN,	20,		EXASSOCrtol,	0,		" /= ",	0 },
	{ EXOP_SMODASN,	20,		EXASSOCrtol,	0,		" %= ",	0 },
	{ EXOP_ANDASN,	20,		EXASSOCrtol,	0,		" &= ",	0 },
	{ EXOP_XORASN,	20,		EXASSOCrtol,	0,		" ^= ",	0 },
	{ EXOP_ORASN,	20,		EXASSOCrtol,	0,		" |= ",	0 },
	{ EXOP_LSHASN,	20,		EXASSOCrtol,	0,		" <<= ",	0 },
	{ EXOP_SRSHASN,	20,		EXASSOCrtol,	0,		" >>= ",	0 },
	{ EXOP_URSHASN,	20,		EXASSOCrtol,	0,		" U>>= ",	0 },
	{ EXOP_COMMA,	10,		EXASSOCltor,	0,		" , ",	0 },
	{ EXOP_unknown, 0, 		EXASSOCnone, 	0,		0,		0 }
};

bool _Expr::IsCond() const
{
	switch (op) {
	case EXOP_NE:
	case EXOP_EQ:
	case EXOP_SLT:
	case EXOP_SLE:
	case EXOP_SGT:
	case EXOP_SGE:
	case EXOP_ULT:
	case EXOP_ULE:
	case EXOP_UGT:
	case EXOP_UGE:
		return true;
	default:
		return false;
	}
}

void _Expr::InvertCond()
{
	switch (op) {
	case EXOP_NE: 	op = EXOP_EQ; break;
	case EXOP_EQ: 	op = EXOP_NE; break;
	case EXOP_SLT:	op = EXOP_SGE; break;
	case EXOP_SLE:	op = EXOP_SGT; break;
	case EXOP_SGT:	op = EXOP_SLE; break;
	case EXOP_SGE:	op = EXOP_SLT; break;
	case EXOP_ULT:	op = EXOP_UGE; break;
	case EXOP_ULE:	op = EXOP_UGT; break;
	case EXOP_UGT:	op = EXOP_ULE; break;
	case EXOP_UGE:	op = EXOP_ULT; break;
	default:		break;
	}
}

const ExopTbl *_Expr::findop(ExprOp opid) const
{
	for (int i=0; optable[i].op; i++)
		if (optable[i].op == opid)
			return &optable[i];
	return 0;
}

int _Expr::Prec(ExprOp opid) const
{
	const ExopTbl *e = findop(opid);
	return e ? e->prec : 0;
}

ExprAssoc _Expr::Assoc(ExprOp opid) const
{
	const ExopTbl *e = findop(opid);
	return e ? e->assoc : EXASSOCnone;
}

_Expr::_Expr(ExprOp op)
	: op(op), excluded(false)
{
	estr = IsNull() ? "<NullExpr>" : "<Invalid Expr>";
}

cString& _Expr::toExprString(bool paren, ExprModifier omode) const
{
	return estr;
}

void _Expr::WriteC(std::ostream& os, int indent) const
{
	os << toExprString();
}

Expr _Expr::Arg1() const
{
	return ExprFactory::Instance().Null();
}

Expr _Expr::Arg2() const
{
	return ExprFactory::Instance().Null();
}

Expr _Expr::Arg3() const
{
	return ExprFactory::Instance().Null();
}


_ExprNumber::_ExprNumber(int val)
	: _Expr(EXOP_unknown), val(val)
{
	makeexpr(EXMODdec);
}

cString& _ExprNumber::toExprString(bool paren, ExprModifier omode) const
{
	const_cast<_ExprNumber*>(this)->makeexpr(omode);
	return estr;
}

bool _ExprNumber::operator==(cExpr& other) const
{
	if (IsA() != other->IsA()) return false;
	return val == other->asNumber();
}

void _ExprNumber::makeexpr(ExprModifier omode)
{
	switch (omode) {
	default:
	case EXMODdec:
		estr = std::to_string(val); break;
	case EXMODoct:
		estr = "0";
		estr += to_octstring(val); break;
	case EXMODhex:
		estr = "0x";
		estr += to_hexstring(val); break;
	}
}

_ExprString::_ExprString(cString& s)
	: _Expr()
{
	makeexpr(s);
}

bool _ExprString::operator==(cExpr& other) const
{
	if (IsA() != other->IsA()) return false;
	return other->asString() == estr;
}

void _ExprString::makeexpr(cString& s)
{
	estr = "\"";
	for (char const& c : s) {
		switch (c) {
		case '\n': estr += "\\n"; break;
		case '\r': estr += "\\r"; break;
		case '\t': estr += "\\t"; break;
		case '\f': estr += "\\f"; break;
		case '\e': estr += "\\e"; break;
		case '\\': estr += "\\\\"; break;
		case '\"': estr += "\\\""; break;
		case '\'': estr += "\\\'"; break;
		default:
			estr += c; break;
		}
	}
	estr += "\"";
}

_ExprName::_ExprName(cString& nam)
	: _Expr()
{
	estr = nam;
}

bool _ExprName::operator==(cExpr& other) const
{
	if (IsA() != other->IsA()) return false;
	return other->asString() == estr;
}

_ExprAreg::_ExprAreg(int num)
	: _Expr()
{
	estr = num == -1 ? "pc" : ("a" + std::to_string(num));
}

bool _ExprAreg::operator==(cExpr& other) const
{
	if (IsA() != other->IsA()) return false;
	return other->asNumber() == num;
}

_ExprDreg::_ExprDreg(int num)
	: _Expr()
{
	estr = "d" + std::to_string(num);
}

bool _ExprDreg::operator==(cExpr& other) const
{
	if (IsA() != other->IsA()) return false;
	return other->asNumber() == num;
}

_ExprArg::_ExprArg(int num)
	: _Expr(), num(num)
{
	estr = "arg" + std::to_string(num);
}

bool _ExprArg::operator==(cExpr& other) const
{
	if (IsA() != other->IsA()) return false;
	return other->asNumber() == num;
}

_Expr1::_Expr1(ExprOp op, Expr expr)
	: _Expr(op), expr(expr)
{
}

cString& _Expr1::toExprString(bool paren, ExprModifier omode) const
{
	const_cast<_Expr1*>(this)->makeexpr(paren, omode);
	return estr;
}

void _Expr1::makeexpr(bool paren, ExprModifier omode)
{
	estr = "";
	
	const ExopTbl *exop = findop(op);
	int argprec = expr->Prec();
	bool needparen = exop->prec > argprec;

	if (paren) estr += '(';
	switch(op) {
	default:
		FATALERROR("Default case op=" + std::to_string((int)op));
	case EXOP_RINC:
	case EXOP_RDEC:
		estr += expr->toExprString(needparen, omode);
		estr += exop->rsym;
		break;
	case EXOP_CAST2:
	case EXOP_CAST4:
	case EXOP_push:
	case EXOP_tst:
	case EXOP_BOOLNOT:
	case EXOP_BITNOT:
	case EXOP_LINC:
	case EXOP_LDEC:
	case EXOP_PLUS:
	case EXOP_MINUS:
	case EXOP_DEREF:
	case EXOP_ADDR:
		estr += exop->lsym;
		estr += expr->toExprString(needparen, omode);
	}
	if (paren) estr += ')';
}

bool _Expr1::operator==(cExpr& other) const
{
	if (Op() != other->Op()) return false;
	const Expr1 oexpr = std::dynamic_pointer_cast<_Expr1>(other);
	return *expr == oexpr->expr;
}

_Expr2::_Expr2(ExprOp op, Expr left, Expr right)
	: _Expr(op), left(left), right(right)
{
}

bool _Expr2::operator==(cExpr& other) const
{
	if (Op() != other->Op()) return false;
	const Expr2 oexpr = std::dynamic_pointer_cast<_Expr2>(other);
	return *left == oexpr->left &&
		   *right == oexpr->right;
}

cString& _Expr2::toExprString(bool paren, ExprModifier omode) const
{
	const_cast<_Expr2*>(this)->makeexpr(paren, omode);
	return estr;
}

void _Expr2::makeexpr(bool paren, ExprModifier omode)
{
	estr = "";

	const ExopTbl *exop = findop(op);
	int myprec = exop->prec;
	bool lparen = myprec > left->Prec();
	bool rparen = myprec > right->Prec();
//	if (exop->msym)
//		std::cout << "makeexpr: " << op << " msym=" << exop->msym << std::endl;

	if (paren) estr += '(';
	switch(op) {
	default:
		FATALERROR("Default case op=" + std::to_string((int)op));
	case EXOP_BRACKET:
		estr += left->toExprString(lparen, omode);
		estr += exop->lsym;
		estr += right->toExprString(false, omode);
		estr += exop->rsym;
		break;
	case EXOP_PTROFF:
	case EXOP_OFF:
		/* TODO: dereference struct */
		estr += left->toExprString(lparen, omode);
		estr += exop->msym;
		estr += "off";
		estr += right->toExprString(rparen, omode);
		break;
	case EXOP_SMUL:
	case EXOP_UMUL:
	case EXOP_scale:	/* synthetic scaling for array access */
	case EXOP_SDIV:
	case EXOP_UDIV:
	case EXOP_SMOD:
	case EXOP_UMOD:
	case EXOP_ADD:
	case EXOP_SUB:
	case EXOP_LSH:
	case EXOP_SRSH:
	case EXOP_URSH:
	case EXOP_ULT:
	case EXOP_ULE:
	case EXOP_UGT:
	case EXOP_UGE:
	case EXOP_SLT:
	case EXOP_SLE:
	case EXOP_SGT:
	case EXOP_SGE:
	case EXOP_EQ:
	case EXOP_NE:
	case EXOP_cmp:
	case EXOP_BITAND:
	case EXOP_BITXOR:
	case EXOP_BITOR:
	case EXOP_BOOLAND:
	case EXOP_BOOLOR:
	case EXOP_ASN:
	case EXOP_ADDASN:
	case EXOP_SUBASN:
	case EXOP_SMULASN:
	case EXOP_SDIVASN:
	case EXOP_SMODASN:
	case EXOP_UMULASN:
	case EXOP_UDIVASN:
	case EXOP_UMODASN:
	case EXOP_ANDASN:
	case EXOP_XORASN:
	case EXOP_ORASN:
	case EXOP_LSHASN:
	case EXOP_URSHASN:
	case EXOP_SRSHASN:
	case EXOP_COMMA:
		estr += left->toExprString(lparen, omode);
		estr += exop->msym;
		estr += right->toExprString(rparen, omode);
		break;
	case EXOP_bit:
		estr += left->toExprString(lparen, omode);
		estr += "sz";
		estr += right->toExprString(rparen, omode);
	}
	if (paren) estr += ')';
}

_Expr3::_Expr3(ExprOp op, Expr cond, Expr left, Expr right)
	: _Expr(op), cond(cond), left(left), right(right)
{
}

cString& _Expr3::toExprString(bool paren, ExprModifier omode) const
{
	const_cast<_Expr3*>(this)->makeexpr(paren, omode);
	return estr;
}

bool _Expr3::operator==(cExpr& other) const
{
	if (Op() != other->Op()) return false;
	const Expr3 oexpr = std::dynamic_pointer_cast<_Expr3>(other);
	return *cond == oexpr->cond &&
		   *left == oexpr->left &&
		   *right == oexpr->right;
}

void _Expr3::makeexpr(bool paren, ExprModifier omode)
{
	estr = "";

	const ExopTbl *exop = findop(op);
	int myprec = exop->prec;
	bool cparen = myprec > cond->Prec();
	bool lparen = myprec > left->Prec();
	bool rparen = myprec > right->Prec();

	if (paren) estr += '(';
	switch(op) {
	default:
		FATALERROR("Default case op=" + std::to_string((int)op));
	case EXOP_TERNARY:
		estr += cond->toExprString(cparen, omode);
		estr += exop->msym;
		estr += left->toExprString(lparen, omode);
		estr += exop->rsym;
		estr += right->toExprString(rparen, omode);
	}
	if (paren) estr += ')';
}

_ExprFunc::_ExprFunc(cString& nam)
	: _Expr(EXOP_func), name(nam), call(0), argcnt(0), ret(0)
{
}

_ExprFunc::_ExprFunc(Expr callexpr)
	: _Expr(EXOP_func), name("LAMBDA"), call(callexpr), argcnt(0), ret(0)
{
}

bool _ExprFunc::operator==(cExpr& other) const
{
	if (IsA() != other->IsA()) return false;
	const ExprFunc o = std::dynamic_pointer_cast<_ExprFunc>(other);
	return o->Name() == name;
}

void _ExprFunc::AddArg(Expr arg)
{
	args.push_front(arg);
}

Expr _ExprFunc::GetArg(int n) const
{
	try { return args.at(n); }
	catch (const std::out_of_range& oor) {
		Diag::Error("GetArg n=" + std::to_string(n));
		return 0; 
	}
}

cString& _ExprFunc::toExprString(bool paren, ExprModifier omode) const
{
	String s;
	if (call) {
		s = "(*" + call->toExprString(true, omode);
		s += ")";
	} else
		s = name;
	bool first = true;
	s += "(";
	for (auto e : args) {
		if (!first) s += ",";
		s += e->toExprString(false, omode);
		first = false;
	}
	if (args.size() <= argcnt) {
		if (!first) s += ",";
		s += "...";
	}
	s += ")";
	const_cast<_ExprFunc*>(this)->estr = s;
	return estr;
}

_ExprAsm::_ExprAsm(Instr inst, bool genwords)
	: inst(inst)
{
	if (genwords) {
		estr = "{";
		for (auto const i : inst->Words()) {
			estr += " 0x";
			estr += to_hexstring(i->Value16());
			estr += "; ";
		}
		estr += "} ";
	} else
		estr = "";

	estr += "/* ";
	estr += inst->toAsmString();
	estr += " */";
}

// compare helper
bool exprcmp(const Expr& i, const Expr& j) {
	return *i == j;
}

bool _ExprAsm::operator==(cExpr& other) const
{
	if (IsA() != other->IsA()) return false;
	ExprAsm o = std::dynamic_pointer_cast<_ExprAsm>(other);
	return *inst == o->inst;
}
