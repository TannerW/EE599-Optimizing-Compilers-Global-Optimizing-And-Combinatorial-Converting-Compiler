/*	bb5.c

	Basic Block compiler in C:  Code Generator

	2017 by H. Dietz
*/

#include	"bb.h"

static	tuple	code = {
	&code, &code, UNDEF
};

static	tuple	*zero, *negone;

static int
ttoi(register tuple *t)
{
	/* tuple pointer to int position */
	register int i = 0;
	register tuple *p = code.next;

	while (p != &code) {
		if (p == t) return(i);
		++i;
		p = p->next;
	}

	return(-1);
}

static void
cgbin(char *s, tuple *p)
{
	printf("%s(%d, %d)", s, ttoi(p->targ[0]), ttoi(p->targ[1]));
}

static tuple *
mktuple(void)
{
	/* make a tuple and link it into code */
	register tuple *p = ((tuple *) malloc(sizeof(tuple)));

	p->next = &code;
	(p->prev = code.prev)->next = p;
	return(code.prev = p);
}

static void
rmtuple(register tuple *p)
{
	/* remove this tuple */

	(p->prev)->next = p->next;
	(p->next)->prev = p->prev;
	free((char *) p);
}

tuple *
binop(opcode o, tuple *t1, tuple *t2)
{
	register tuple *p;


	/* Constant folding */
	if ((t1->oarg == CONST) && (t2->oarg == CONST))
	switch (o) {
	case ADD:	return(cop(t1->carg + t2->carg));
	case SUB:	return(cop(t1->carg - t2->carg));
	case AND:	return(cop(t1->carg & t2->carg));
	case OR:	return(cop(t1->carg | t2->carg));
	case XOR:	return(cop(t1->carg ^ t2->carg));
	case GT:	return(cop(t1->carg > t2->carg));
	case GE:	return(cop(t1->carg >= t2->carg));
	case EQ:	return(cop(t1->carg == t2->carg));
	case SSL:	return(cop(t1->carg << t2->carg));
	case SSR:	return(cop(t1->carg >> t2->carg));
	}

	/* Normalize operand order for commutative ops */
	switch (o) {
	case ADD:
	case AND:
	case OR:
	case XOR:
	case EQ:
		/* Forces CONST 0 or -1 into second position so it's the
		   same place as for SUB
		*/
		if (ttoi(t1) < ttoi(t2)) {
			tuple *t = t1;
			t1 = t2;
			t2 = t;
		}
	}

	/* Check for special cases */
	switch (o) {
	case ADD:
		if (t2 == zero) return(t1);
		if (t1 == t2) return(binop(SSL, t1, cop(1)));
		break;
	case SUB:
		if (t2 == zero) return(t1);
		if (t1 == t2) return(zero);
		break;
	case AND:
		if (t2 == zero) return(zero);
		if (t2 == negone) return(t2);
		if (t1 == t2) return(t1);
		break;
	case OR:
		if (t2 == zero) return(t1);
		if (t2 == negone) return(negone);
		if (t1 == t2) return(t1);
		break;
	case XOR:
		if (t2 == zero) return(t1);
		if (t1 == t2) return(zero);
		break;
	case GT:
		if (t1 == t2) return(zero);
		break;
	case GE:
		if (t1 == t2) return(cop(1));
		break;
	case EQ:
		if (t1 == t2) return(cop(1));
		break;
	case SSR:
		if (t1 == negone) return(negone); /* Not in assignment! */
		/* Fall through... */
	case SSL:
		if (t2 == zero) return(t1);
		if (t1 == zero) return(zero);
		break;
	}

	/* Common subexpression elimination */
	p = code.prev;
	while ((p != t1) && (p != t2)) {
		/* Haven't gone back to either operand yet */
		if ((p->oarg == o) && (p->targ[0] == t1) && (p->targ[1] == t2)) {
			/* Redundant with this */
			return(p);
		}
		p = p->prev;
	}

	/* Make a new tuple */
	p = mktuple();
	p->oarg = o;
	p->targ[0] = t1;
	p->targ[1] = t2;
	return(p);
}

tuple *
cop(konst c)
{
	register tuple *p;

	/* Have we seen this before? */
	p = code.prev;
	while ((p != &code) && (p->oarg != LAB)) {
		/* Haven't gone back to start of block yet */
		if ((p->oarg == CONST) && (p->carg == c)) {
			/* Redundant with this */
			return(p);
		}
		p = p->prev;
	}

	/* Need new tuple */
	p = mktuple();
	p->oarg = CONST;
	p->carg = c;
	return(p);
}

#define	DIFFERENT	1
#define	SAME		2
#define	MAYBE		3

int
sameval(tuple *p, tuple *q)
{
	/* Could p and q have the same value? */

	/* Same expression */
	if (p == q) return(SAME);

	/* Not same, but both constants */
	if ((p->oarg == CONST) && (q->oarg == CONST)) return(DIFFERENT);

	/* Differ by +/-/^ a different offset */
	switch (p->oarg) {
	case ADD:
	case SUB:
	case XOR:
		/* Sneaky recursive check for different offsets */
		if ((p->oarg == q->oarg) &&
		    (p->targ[0] == q->targ[0]) &&
		    (sameval(p->targ[1], q->targ[1]) == DIFFERENT)) {
			return(DIFFERENT);
		}

		/* Constant offset vs. 0? */
		if (p->targ[0] == q) {
			return(DIFFERENT);
		}
	}
	switch (q->oarg) {
	case ADD:
	case SUB:
	case XOR:
		/* Constant offset vs. 0? */
		if (q->targ[0] == p) {
			return(DIFFERENT);
		}
	}

	/* Not sure... */
	return(MAYBE);
}

tuple *
ldop(var *v)
{
	return(ldxop(v, zero));
}

tuple *
ldxop(var *v, tuple *t)
{
	register tuple *p;

	/* Value propagation */
	p = code.prev;
	while ((p != &code) && (p->oarg != LAB)) {
		if (p->varg == v)
		switch (p->oarg) {
		case STX:
			switch (sameval(t, p->targ[0])) {
			case SAME:
				return(p->targ[1]);
			case MAYBE:
				goto killed;
			}
			break;
		case LDX:
			/* Did this before? */
			if (t == p->targ[0]) return(p);
		}
		p = p->prev;
	}

killed:
	/* Must do load */
	p = mktuple();
	p->oarg = LDX;
	p->varg = v;
	p->targ[0] = t;
	return(p);
}

tuple *
stop(var *v, tuple *t)
{
	return(stxop(v, zero, t));
}

tuple *
stxop(var *v, tuple *t1, tuple *t2)
{
	register tuple *p = mktuple();

	p->oarg = STX;
	p->varg = v;
	p->targ[0] = t1;
	p->targ[1] = t2;
	return(p);
}

tuple *
labop(label l)
{
	register tuple *p = mktuple();

	p->oarg = LAB;
	p->larg[0] = l;

	/* Pre-install 0 and -1 in this block */
	zero = cop(0);
	negone = cop(-1);

	return(p);
}

tuple *
selop(tuple *t, label l1, label l2)
{
	register tuple *p = mktuple();

	p->oarg = SEL;
	p->targ[0] = t;
	p->larg[0] = l1;
	p->larg[1] = l2;
	return(p);
}

tuple *
killop(var *v)
{
	register tuple *p = mktuple();

	p->oarg = KILL;
	p->varg = v;
	return(p);
}

void
codegen(void)
{
	/* print listing of generated code */
	register tuple *p = code.next;

	while (p != &code) {
		printf("%d\t", ttoi(p));

		switch (p->oarg) {
		case ADD:
			cgbin("add", p);
			break;
		case SUB:
			cgbin("sub", p);
			break;
		case AND:
			cgbin("and", p);
			break;
		case OR:
			cgbin("or", p);
			break;
		case XOR:
			cgbin("xor", p);
			break;
		case GT:
			cgbin("gt", p);
			break;
		case GE:
			cgbin("ge", p);
			break;
		case EQ:
			cgbin("eq", p);
			break;
		case SSL:
			cgbin("ssl", p);
			break;
		case SSR:
			cgbin("ssr", p);
			break;
		case CONST:
			printf("const(%d)", p->carg);
			break;
		case LD:
			printf("_ld(%s{%d,%d})", (p->varg)->text,
				(p->varg)->deflev, (p->varg)->defblk);
			break;
		case LDX:
			/* Convert LDX into LD? */
			if (((p->targ[0])->oarg == CONST) &&
			    ((p->targ[0])->carg == 0)) {
				printf("ld(%s{%d,%d})", (p->varg)->text,
				       (p->varg)->deflev, (p->varg)->defblk);
			} else {
				printf("ldx(%s{%d,%d}, %d)", (p->varg)->text,
				       (p->varg)->deflev, (p->varg)->defblk,
				       ttoi(p->targ[0]));
			}
			break;
		case ST:
			printf("_st(%s{%d,%d}, %d)", (p->varg)->text,
				(p->varg)->deflev, (p->varg)->defblk,
				ttoi(p->targ[0]));
			break;
		case STX:
			/* Convert STX into ST? */
			if (((p->targ[0])->oarg == CONST) &&
			    ((p->targ[0])->carg == 0)) {
				printf("st(%s{%d,%d}, %d)", (p->varg)->text,
				       (p->varg)->deflev, (p->varg)->defblk,
				       ttoi(p->targ[1]));
			} else {
				printf("stx(%s{%d,%d}, %d, %d)", (p->varg)->text,
				       (p->varg)->deflev, (p->varg)->defblk,
				       ttoi(p->targ[0]), ttoi(p->targ[1]));
			}
			break;
		case LAB:
			printf("lab(%d)", p->larg[0]);
			break;
		case SEL:
			printf("sel(%d, %d, %d)", ttoi(p->targ[0]),
				p->larg[0], p->larg[1]);
			break;
		case KILL:
			printf("kill(%s{%d,%d})", (p->varg)->text,
				(p->varg)->deflev, (p->varg)->defblk);
			break;
		default:
			printf("{bad opcode %d}", p->oarg);
		}

		printf("\n");
		p = p->next;
	}
}
