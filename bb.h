/*	bb.h

	Basic Block compiler in C:  Header File

	2017 by H. Dietz
*/

#include	<stdlib.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>

#define	DEBUG			/* cause debugging printfs */
#undef	DEBUG

typedef	int	konst;		/* type of a constant value */
typedef	int	label;		/* type of a label value */
typedef	short	opcode;		/* type of an opcode */

/*	Gate-level stuff... */
#define	MAXGATES (1024*1024)	/* Size of gate pool */
#define	MAXDIM 8		/* Maximum dimension of array */
#define	BUSWIDTH 8		/* Bus width */
#define	forbus(I)	for (I=0; I<BUSWIDTH; ++I)
#define	forbusdim(I)	for (I=0; I<(BUSWIDTH*MAXDIM); ++I)
#define	forbusrev(I)	for (I=BUSWIDTH-1; I>=0; --I)
#define	forgates(I)	for (I=0; I<gatesp; ++I)

typedef struct {
	int	arg0, arg1;	/* Operands */
	int	newno;		/* Used to renumber */
	int	level;		/* Level-order schedule depth */
	opcode	op;		/* Opcode */
	char	needed;		/* Is this needed? */
} gate_t;

typedef struct {
	int	wire[BUSWIDTH*MAXDIM];
} bus_t;

/*	Symbol table (variable) struct... */
#define	var	struct _var
var {
	char	*text;		/* pointer to string */
	int	type;		/* token type of variable (e.g., IF) */
	int	dim;		/* size of dimension (1 means scalar) */
	int	deflev;		/* scope level of definition */
	int	defblk;		/* block number of definition */
	bus_t	bus;		/* bus value */
};

#define	MAXV	1024		/* maximum number of vars */
#define	MODV(x)	((x)&(MAXV-1))	/* x mod MAXV */

#define	VARBIAS	1000000		/* magic bias to ID var bits */
#define	VARPTR2NUM(X)	(((X)-&(symtab[0]))*VARBIAS)
#define	NUM2VARPTR(X)	(&(symtab[(X)/VARBIAS]))

/*	Tuple struct... */
#define	tuple	struct _tuple
tuple {
	tuple	*next, *prev;	/* sequential order links */
	opcode	oarg;		/* tuple opcode */
	tuple	*targ[2];	/* tuple arguments */
	konst	carg;		/* constant argument */
	label	larg[2];	/* label arguments */
	var	*varg;		/* variable name argument */
	int	refs;		/* how many refs to this? */
	int	slot;		/* scheduled parallel execution slot */
	bus_t	bus;		/* bus for bit-level value */
};

/*	token types... */
#define	ADD	'+'
#define	SUB	'-'
#define	AND	'&'
#define	OR	'|'
#define	XOR	'^'
#define	GT	'>'
#define	GE	512
#define	EQ	513
#define	SSL	514
#define	SSR	515
#define	CONST	516
#define	LD	517
#define	LDX	518
#define	ST	519
#define	STX	520
#define	LAB	521
#define	SEL	522
#define	KILL	523

#define	WORD	524
#define	NE	525
#define	LT	'<'
#define	LE	526

#define	NAND	527	/* for gate-level stuff */
#define	NOR	528	/* for gate-level stuff */

#define	INT	1024
#define	IF	1025
#define	ELSE	1026
#define	WHILE	1027
#define	BREAK	1028
#define	CONT	1029
#define	UNDEF	2048

/*	Output control... */
#define	OUTDOT	0x01
#define	OUTGATE	0x02
#define	OUTPAR	0x04
#define	OUTSEQ	0x08
#define	OUTVER	0x10


/*	bb1.c */
extern	int	outtyp;		/* output type */

/*	bb2.C */
extern	void	prog(void);	/* parser entry point */

/*	bb3.C */
extern	int	lookt;		/* next token of input */
extern	var	*lookv;		/* next variable */
extern	konst	lookk;		/* next constant */
extern	char	looks[513];	/* next string */
extern	char	errbuf[513];	/* error message buffer */
extern	int nextt(void);	/* get next token from input */
extern	void error(char *fmat);
extern	int need(int x, char *y);

/*	bb4.C */
extern	var	symtab[MAXV];	/* symbol table itself */
extern	var	*statevar;	/* state variable for state machine */
extern	int newscope(void);	/* make a new scope */
extern	int endscope(void);	/* end the current scope */
extern	var *findv(register char *s);	/* find var entry in symbol table */
extern	var *makev(register char *s);	/* make var entry in symbol table */
extern	void initv(void);	/* initialize symbol table */

/*	bb5.C */
extern	tuple	*binop(opcode o, tuple *t1, tuple *t2);
extern	tuple	*cop(konst c);
extern	tuple	*ldop(var *v);
extern	tuple	*ldxop(var *v, tuple *t);
extern	tuple	*stop(var *v, tuple *t);
extern	tuple	*stxop(var *v, tuple *t1, tuple *t2);
extern	tuple	*labop(label l);
extern	tuple	*selop(tuple *t, label l1, label l2);
extern	tuple	*killop(var *v);
extern	void	codegen(void);

/*	bb6.c */
extern	gate_t	gate[MAXGATES];
extern	int	gatesp;
extern	int	gatesneed;
extern	bus_t	bus_zero;
extern	bus_t	busop(opcode op, bus_t arg0, bus_t arg1);
extern	bus_t	busconst(int v);
extern	bus_t	busload(var *varg);
extern	bus_t	busstore(int guard, var *varg, bus_t bus);
extern	int	stateguard(int state);
extern	void	dumpgates(int haltstate);
extern	void	buslab(int guard, int t);
extern	void	bussel(int guard, bus_t bus, int t, int e);

