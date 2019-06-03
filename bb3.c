/*	bb3.c

	Basic Block compiler in C:  Lexicals

	2017 by H. Dietz
*/

#include	"bb.h"

	int	lookt;		/* next token of input */
static	int	lookc = ' ';	/* next character of input */
	var	*lookv;		/* next variable */
	konst	lookk;		/* next constant */
	char	looks[513];	/* next string */
	char	errbuf[513];	/* error message buffer */
static	int	line = 0;	/* line number */

static int
nextc(void)
{
	if (lookc == EOF) return(EOF);
	switch (lookc = getchar()) {
	case '\n':
		/* another source line... */	
		++line;
	}
	return(lookc);
}

int
nextt(void)
{
	/* get a token from the input stream */

retry:	/* comment & error retry for token */

	/* eat white space */
	while (isspace(lookc)) nextc();

	/* not a name, must be something else... */
	switch (lookc) {
	case '{':	case '}':
	case '(':	case ')':
	case '[':	case ']':
	case ',':	case ';':
	case '+':	case '-':
	case '&':	case '|':	case '^':
	case '~':
	case '*':	case '/':	case '%':
		lookt = lookc;
		nextc();
		break;

	case '=':
		nextc();
		if (lookc == '=') {
			nextc();
			lookt = EQ;
		} else {
			lookt = '=';
		}
		break;

	case '!':
		nextc();
		if (lookc == '=') {
			nextc();
			lookt = NE;
		} else {
			lookt = '!';
		}
		break;

	case '<':
		nextc();
		switch (lookc) {
		case '=':
			nextc();
			lookt = LE;
			break;
		case '<':
			nextc();
			lookt = SSL;
			break;
		default:
			lookt = '<';
		}
		break;

	case '>':
		nextc();
		switch (lookc) {
		case '=':
			nextc();
			lookt = GE;
			break;
		case '>':
			nextc();
			lookt = SSR;
			break;
		default:
			lookt = '>';
		}
		break;

	case EOF:
		return(lookt = EOF);

	case '0':
	case '1':	case '2':	case '3':
	case '4':	case '5':	case '6':
	case '7':	case '8':	case '9':
		/* A constant number, we hope */
		{
			register konst num = 0;

			do {
				num = (num * 10) + (lookc - '0');
				nextc();
			} while (isdigit(lookc));
			lookk = num;
			lookt = CONST;
		}
		break;

	default:
		if (isalpha(lookc) || (lookc == '_')) {
			register char *p = &(looks[0]);

			do {
				*(p++) = lookc;
				nextc();
			} while (isalnum(lookc) || (lookc == '_'));
			*p = 0;

			/* Lookup the item... */
			if ((lookv = findv(&(looks[0]))) != ((var *) NULL)) {
				lookt = lookv->type;
			} else {
				lookt = UNDEF;
			}
		} else {
			/* Nothing useful... */
			sprintf(errbuf, "bad character (0x%02x)", lookc);
			error(errbuf);
			nextc();
			goto retry;
		}
	}

	return(lookt);
}

void
error(char *fmat)
{
	/* print number of offending line & error message */

	fprintf(stderr, "Error in line %d:  ", line);
	fprintf(stderr, "%s", fmat);
	fprintf(stderr, "\n");
}

int
need(int x, char *y)
{
	if (lookt == x) {
		nextt();
	} else {
		sprintf(errbuf, "missing %s assumed", y);
		error(errbuf);
	}
}
