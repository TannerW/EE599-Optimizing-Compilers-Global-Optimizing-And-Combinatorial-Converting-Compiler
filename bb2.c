/*	bb2.c

	Basic Block compiler in C:  Parser

	2017 by H. Dietz
*/

#include	"bb.h"

/*	stack for break/continue labels...  */
#define	MAXLAB	64
static	label	brklab[MAXLAB];
static	label	*brksp = &(brklab[-1]);
static	label	contlab[MAXLAB];
static	label	*contsp = &(contlab[-1]);

extern	void	prog(void);	/* parser entry point */
static	void	decl(void);	/* variable declarations */
static	void	stat(void);	/* statements */
static	tuple	*expr(void);	/* expressions... */
static	tuple	*xor(void);
static	tuple	*and(void);
static	tuple	*eq(void);
static	tuple	*relop(void);
static	tuple	*shift(void);
static	tuple	*addsub(void);
static	tuple	*muldiv(void);
static	tuple	*unary(void);
static	label	newlab(int n);	/* create new labels */
static	tuple	*helpmul(register tuple *t1, register tuple *t2);	/* generate in-line multiply code */
static	tuple	*helpss(register opcode o, register tuple *t1, register tuple *t2);	/* generate in-line shift code */

void
prog(void)
{
	for (;;) {
		switch (lookt) {
		case INT:
			/* global declarations */
			nextt();
			decl();
			need(';', ";");
			break;

		case UNDEF:
			/* function definition */
			nextt();
			need('(', "(");
			newscope();
			decl();
			need(')', ")");
			labop( newlab(1) );
			stat();
			endscope();
			labop( newlab(1) );
			break;

		default:
			error("bad global data or function declaration");
			nextt();
			break;

		case EOF:
			return;
		}
	}
}

static void
decl(void)
{
	while ((lookt == WORD) || (lookt == UNDEF)) {
		/* variable name */
		register var *p = makev(&(looks[0]));

		/* check for dimension */
		if (nextt() == '[') {
			nextt();
			if (lookt != CONST) {
				error("missing dimension (1 assumed)");
				p->dim = 1;
			} else {
				p->dim = lookk;
				nextt();
			}
			need(']', "]");

			if ((outtyp & (OUTDOT | OUTGATE | OUTVER)) && (p->dim > MAXDIM)) {
				error("truncated array dimension too large for gate design");
				p->dim = MAXDIM;
			}
		} else {
			/* assume it is a scalar */
			p->dim = 1;
		}

		if (lookt == ',') {
			nextt();
		}
	}
}

static void
stat(void)
{
	switch (lookt) {
	case '{':
		nextt();
		newscope();
		while ((lookt != EOF) && (lookt != '}')) stat();
		endscope();
		need('}', "}");
		break;

	case IF:
		nextt();
		{
			register tuple *t = expr();
			register label lab = (*(++brksp) = newlab(3));

			selop(t, lab+1, lab+2);
			/* then clause... */
			labop(lab+1);
			stat();
			selop(NULL, lab, lab);
			/* else clause... */
			labop(lab+2);
			if (lookt == ELSE) {
				/* generate else code... */
				nextt();
				stat();
			}
			selop(NULL, lab, lab);
			/* end if... */
			labop(lab);
		}
		--brksp;
		break;

	case BREAK:
		nextt();
		selop(NULL, *brksp, *brksp);
		need(';', ";");
		break;

	case WHILE:
		nextt();
		{
			register label clab = (*(++brksp) = newlab(1));
			register label blab = (*(++brksp) = newlab(2));
			register tuple *t;

			labop(clab);
			t = expr();
			selop(t, blab+1, blab);
			labop(blab+1);
			stat();
			selop(NULL, clab, clab);
			labop(blab);
		}
		--brksp;
		--contsp;
		break;

	case CONT:
		nextt();
		selop(NULL, *contsp, *contsp);
		need(';', ";");
		break;

	case INT:
		nextt();
		decl();
		need(';', ";");
		break;

	case WORD:
		/* assignment statement */
		{
			register var *v = lookv;
			register tuple *t1 = NULL;
			register tuple *t2 = NULL;

			/* if var isn't defined, define it... */
			if (v == ((var *) NULL)) {
				sprintf(errbuf, "undefined variable %s", &(looks[0]));
				error(errbuf);
				v = makev(&(looks[0]));
			}

			/* look for subscript... */
			if (nextt() == '[') {
				if (outtyp & (OUTDOT | OUTGATE | OUTVER)) {
					error("subscripted store not allowed for gate-level output");
				}
				nextt();
				t1 = expr();
				need(']', "]");
			}

			need('=', "=");
			t2 = expr();
			need(';', ";");

			/* do the appropriate store... */
			if (t1 == NULL) {
				stop(v, t2);
			} else {
				stxop(v, t1, t2);
			}
		}
		break;

	case ';':
		nextt();
		break;

	default:
		error("bad statement syntax");
		nextt();
	}
}

static tuple *
expr(void)
{
	register tuple *t = xor();

	while (lookt == '|') {
		nextt();
		t = binop(OR, t, xor());
	}
	return(t);
}

static tuple *
xor(void)
{
	register tuple *t = and();

	while (lookt == '^') {
		nextt();
		t = binop(XOR, t, and());
	}
	return(t);
}

static tuple *
and(void)
{
	register tuple *t = eq();

	while (lookt == AND) {
		nextt();
		t = binop(AND, t, eq());
	}
	return(t);
}

static tuple *
eq(void)
{
	register tuple *t = relop();

	for (;;) {
		switch (lookt) {
		case EQ:
			nextt();
			t = binop(EQ, t, relop());
			break;
		case NE:
			nextt();
			t = binop(XOR, cop(1), binop(EQ, t, relop()));
			break;
		default:
			return(t);
		}
	}
}

static tuple *
relop(void)
{
	register tuple *t = shift();

	for (;;) {
		switch (lookt) {
		case '>':
			nextt();
			t = binop('>', t, shift());
			break;
		case GE:
			nextt();
			t = binop(GE, t, shift());
			break;
		case '<':
			nextt();
			t = binop('>', shift(), t);
			break;
		case LE:
			nextt();
			t = binop(GE, shift(), t);
			break;
		default:
			return(t);
		}
	}
}

static tuple *
shift(void)
{
	register tuple *t = addsub();

	for (;;) {
		switch (lookt) {
		case SSL:
			nextt();
			t = helpss(SSL, t, addsub());
			break;
		case SSR:
			nextt();
			t = helpss(SSR, t, addsub());
			break;
		default:
			return(t);
		}
	}
}
	

static tuple *
addsub(void)
{
	register tuple *t = muldiv();

	for (;;) {
		switch (lookt) {
		case '+':
			nextt();
			t = binop('+', t, muldiv());
			break;
		case '-':
			nextt();
			t = binop('-', t, muldiv());
			break;
		default:
			return(t);
		}
	}
}

static tuple *
muldiv(void)
{
	register tuple *t = unary();

	for (;;) {
		switch (lookt) {
		case '*':
			nextt();
			t = helpmul(t, unary());
			break;
		case '/':
		case '%':
			nextt();
			error("unimplemented operator");
			t = unary();
			break;
		default:
			return(t);
		}
	}
}

static tuple *
unary(void)
{
	switch (lookt) {
	case '(':
		/* parenthesized expression */
		nextt();
		{
			register tuple *t = expr();
			need(')', ")");
			return(t);
		}

	case '-':
		/* negative */
		nextt();
		return( binop(SUB, cop(0), unary()) );

	case '~':
		/* bitwise not */
		nextt();
		return( binop(XOR, cop(-1), unary()) );

	case '!':
		/* logical not */
		nextt();
		return( binop(EQ, cop(0), unary()) );

	case CONST:
		/* constant value */
		{
			register tuple *t = cop(lookk);
			nextt();
			return(t);
		}

	case UNDEF:
	case WORD:
		/* variable reference */
		{
			register var *v = lookv;

			/* if var isn't defined, define it... */
			if (v == ((var *) NULL)) {
				sprintf(errbuf, "undefined variable %s", &(looks[0]));
				error(errbuf);
				v = makev(&(looks[0]));
			}

			/* look for subscript... */
			if (nextt() == '[') {
				register tuple *t;
				if (outtyp & (OUTDOT | OUTGATE | OUTVER)) {
					error("subscripted load not allowed for gate-level output");
				}
				nextt();
				t = expr();
				need(']', "]");
				return( ldxop(v, t) );
			} else {
				return( ldop(v) );
			}
		}
	}
}

static label
newlab(int n)
{
	static label nextlab = 0;

	nextlab += n;
	return(nextlab - n);
}

static int mask[] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000
};

static tuple *
helpmul(register tuple *t1, register tuple *t2)
{
	/* jump-free in-line multiply */
	register int bit;
	register tuple *t, *t3;

	t3 = cop(0);

	/* handle multiply by a constant... */
	if ((t1->oarg == CONST) || (t2->oarg == CONST)) {
		if (t1->oarg != CONST) {
			t = t1; t1 = t2; t2 = t;
		}

		for (bit=0; bit<32; ++bit) {
			if (t1->carg & mask[bit]) {
				t = t2;
				if (bit & 1) t = binop(SSL, t, cop(1));
				if (bit & 2) t = binop(SSL, t, cop(2));
				if (bit & 4) t = binop(SSL, t, cop(4));
				if (bit & 8) t = binop(SSL, t, cop(8));
				if (bit & 16) t = binop(SSL, t, cop(16));
				t3 = binop(ADD, t3, t);
			}
		}
	} else {
		/* handle multiply by a non-constant... */
		for (bit=0; bit<32; ++bit) {
			t = binop(AND, t1, cop(1));
			t = binop(SUB, cop(0), t);
			t = binop(AND, t, t2);
			t3 = binop(ADD, t3, t);
			t2 = binop(SSR, t2, cop(1));
		}
	}

	return(t3);
}

static tuple *
helpss(register opcode o, register tuple *t1, register tuple *t2)
{
	/* jump-free in-line shift */

	/* handle shift by a constant... */
	if (t2->oarg == CONST) {
		if (t2->carg & 1) t1 = binop(o, t1, cop(1));
		if (t2->carg & 2) t1 = binop(o, t1, cop(2));
		if (t2->carg & 4) t1 = binop(o, t1, cop(4));
		if (t2->carg & 8) t1 = binop(o, t1, cop(8));
		if (t2->carg & 16) t1 = binop(o, t1, cop(16));
	} else {
		/* handle shift by a non-constant... */
		register int bit;
		register tuple *t;

		for (bit=0; bit<5; ++bit) {
			register tuple *t3, *t4;

			t = binop(AND, t2, cop(1));
			t = binop(SUB, cop(0), t);
			t4 = binop(AND, t1, t);
			t = binop(XOR, t, cop(-1));
			t3 = binop(o, t1, cop(mask[bit]));
			t3 = binop(AND, t3, t);
			t1 = binop(OR, t3, t4);
			t2 = binop(SSR, t2, cop(1));
		}
	}

	return(t1);
}
