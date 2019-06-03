/*	bb1.c

	Basic Block compiler in C:  Main Program

	2017 by H. Dietz
*/

#include	"bb.h"

int	outtyp = 0;	/* output type */

int
main(register int argc, register char **argv)
{
	register int i;

	if (argc == 1) {
usage:
		fprintf(stderr,
			"Usage: %s {options}\n"
			"-d\tenable gate-level dot output\n"
			"-g\tenable gate-level gate list output\n"
			"-p\tenable parallel word-level output\n"
			"-s\tenable sequential word-level output\n"
			"-v\tenable gate-level Verilog output\n",
			argv[0]);
		exit(1);
	}

	for (i=1; i<argc; ++i) {
		register char *p = argv[i];
		if (*(p++) != '-') goto usage;
		while (*p) switch (*(p++)) {
		case 'd': outtyp |= OUTDOT; break;
		case 'g': outtyp |= OUTGATE; break;
		case 'p': outtyp |= OUTPAR; break;
		case 's': outtyp |= OUTSEQ; break;
		case 'v': outtyp |= OUTVER; break;
		default: goto usage;
		}
	}
	if (outtyp == 0) goto usage;

	initv();	/* initialize var (symbol) table */
	nextt();	/* prime the input stream */
	newscope();	/* start scope for globals */
	prog();		/* parse program */
	codegen();	/* generate code */
	exit(0);
}
