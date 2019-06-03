/*	bb4.c

	Basic Block compiler in C:  Symbol Table

	2017 by H. Dietz
*/

#include	"bb.h"

var	symtab[MAXV];	/* symbol table itself */
var	*statevar;	/* state variable for state machine */

/*	block numbers for valid scopes... */
static	int blknum[] = {
	0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};
static	int curblk = 0;		/* current block number */
static	int curlev = 0;		/* current block level */

static	var *findsv(register char *s, int lev, int mkit);	/* single-scope version of findv() */
static	char *strsav(register char *s);		/* save a copy of a string */

static int
hash(register char *s)
{
	/* compute a cheap & dirty hash of a string...
	*/
	register int len = strlen(s);

	return(len * s[len-1] + s[0]);
}

static void
dumpv(void)
{
	/* dump variables */
	register int i;

	for (i=0; i<MAXV; ++i) {
		if (symtab[i].type != UNDEF) {
			fprintf(stderr, "%s{%d, %d} type=%d\n",
				symtab[i].text,
				symtab[i].deflev,
				symtab[i].defblk,
				symtab[i].type);
		}
	}

	printf("\n");
}

int
newscope(void)
{
	blknum[++curlev] = ++curblk;
	return(curlev);
}

int
endscope(void)
{
	/* emit kills of vars defined only in this scope... */
	register var *v = &(symtab[MAXV-1]);

	do {
		if ((v->type != UNDEF) &&
		    (v->deflev == curlev) &&
		    (v->defblk == blknum[curlev])) {
		    	killop(v);
		}
	} while (--v >= &(symtab[0]));

	/* and oh yes...  undo the scope */
	return(--curlev);
}

var *
findv(register char *s)
{
	/* find symbol named s...  including scope stuff.
	*/
	register int lev = curlev;
	register var *p;

	do {
		if ((p = findsv(s, lev, 0)) != NULL) {
			return(p);
		}
	} while (--lev >= 0);

	/* Not found...  return NULL pointer */
	return(NULL);
}

var *
makev(register char *s)
{
	/* make a new symbol named s in current scope.
	*/

	return(findsv(s, curlev, WORD));
}

static var *
findsv(register char *s, int lev, int mkit)
{
	/* find symbol named s in scope lev with correct block num.
	   if not found & mkit, make new entry with type = mkit.
	   endless loop results if table overflows.
	*/
	register var *p = &(symtab[ MODV( hash(s) ) ]);

	while (p->type != UNDEF) {
		/* See if entry matches */
		if ((p->deflev == lev) &&
		    (p->defblk == blknum[lev]) &&
		    (!strcmp(p->text, s))) {
			/* found it! */
			return(p);
		}

		/* nope...  bump to examine next entry */
		if (++p >= &(symtab[MAXV])) p = &(symtab[0]);
	}

	/* no such entry -- make one */
	if (mkit) {
		p->text = strsav(s);
		p->type = mkit;
		p->deflev = lev;
		p->defblk = blknum[lev];
		return(p);
	}

	return(NULL);
}

void
initv(void)
{
	/* initialize symbol table...
	*/
	register var *p = &(symtab[MAXV-1]);
	register int i;

	do {
		/* Initialize all busses to "self" */
		forbusdim (i) p->bus.wire[i] = VARPTR2NUM(p) + i;
		p->type = UNDEF;
	} while (--p >= &(symtab[0]));

	/* install keywords into hash table */
	p = makev("int"); p->type = INT;
	p = makev("if"); p->type = IF;
	p = makev("else"); p->type = ELSE;
	p = makev("while"); p->type = WHILE;
	p = makev("break"); p->type = BREAK;
	p = makev("continue"); p->type = CONT;

	/* install state variable (for gate-level design) */
	statevar = (p = makev("STATE"));
	p->type = WORD;
	p->dim = 1;
	p->deflev = 0;
	p->defblk = 0;
	forbusdim (i) p->bus.wire[i] = 0;
}

static char *
strsav(register char *s)
{
	/* save a copy of the string s...
	*/
	register char *p = malloc( strlen(s)+1 );

	strcpy(p, s);
	return(p);
}
