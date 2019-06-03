/*	bb6.c

	Convert tuples into gate-level representations

	2017 by H. Dietz
*/

#include "bb.h"

#define NANDLOGIC 1

gate_t	gate[MAXGATES];
int	gatesp = 0;
int	gatesneed = 0;
bus_t	bus_zero = { 0, 0, 0, 0, 0, 0, 0, 0 };

int
mkgate(register int arg0,
register int arg1,
register opcode op)
{
	/* Make new gate or find old one */
	register int i;

	/* Force initialization */
	if (gatesp == 0) {
		/* Force constants 0 and 1 to exist */
		gate[0].op = '0';
		gate[0].arg0 = 0;
		gate[0].arg1 = 0;
		gate[0].needed = 1;
		gate[1].op = '1';
		gate[1].arg0 = 1;
		gate[1].arg1 = 1;
		gate[1].needed = 1;
		gatesp = 2;
	}

	/* Normalize operand order */
	if (arg0 > arg1) {
		register int t = arg0;
		arg0 = arg1;
		arg1 = t;
	}

	/* Find old one */
	for (i=0; i<gatesp; ++i) {
		if ((gate[i].op == op) &&
		    (gate[i].arg0 == arg0) &&
		    (gate[i].arg1 == arg1)) {
			return(i);
		}
	}

	/* Make new one */
	++gatesp;
	gate[i].op = op;
	gate[i].arg0 = arg0;
	gate[i].arg1 = arg1;
	gate[i].needed = 0;
	return(i);
}

//#define	gatenot(a)		mkgate(a,1,'^')
//#define	gatenot(a)		mkgate(a,a,NAND)

int gatenot(register int a)
{
	if(NANDLOGIC)
	{
		return(mkgate(a,a,NAND));
	} else
	{
		return(mkgate(a,1,'^'));
	}
}

int
gatenand(register int a, register int b)
{
	/* Simplifications */
	if (a == 0) return(1);
	if (b == 0) return(1);
	if (b == 1 && a == 1) return(0);

	return(mkgate(a, b, NAND));
}

int
gateand(register int a, register int b)
{
	/* Simplifications */
	if (a == b) return(a);
	if (a == 0) return(0);
	if (a == 1) return(b);
	if (b == 0) return(0);
	if (b == 1) return(a);

	if(NANDLOGIC)
	{
		return(gatenand(gatenand(a,b),gatenand(a,b)));
	} else
	{
		return(mkgate(a, b, '&'));
	}
}

int
gateor(register int a, register int b)
{
	/* Simplifications */
	if (a == b) return(a);
	if (a == 0) return(b);
	if (a == 1) return(1);
	if (b == 0) return(a);
	if (b == 1) return(1);

	if(NANDLOGIC)
	{
		return(gatenand(gatenot(a),gatenot(b)));
	} else
	{
		return(mkgate(a, b, '|'));
	}
}

int
gatexor(register int a, register int b)
{
	/* Simplifications */
	if (a == b) return(0);
	if (a == 0) return(b);
	if (b == 0) return(a);

	if(NANDLOGIC)
	{
		return(gatenand(gatenand(a,gatenand(a,b)),gatenand(gatenand(a,b),b)));
	} else
	{
		return(mkgate(a, b, '^'));
	}
}

int
gatemux(register int i, register int t, register int e)
{
	/* Simplifications */
	if (i == 0) return(e);
	if (i == 1) return(t);
	if (t == e) return(t);

	/* No mux as a basic gate here */
	if(NANDLOGIC)
	{
		return(gatenand(gatenand(t, i),gatenand(gatenot(i), e)));
	} else
	{
		return(gateor(gateand(i, t), gateand(gatenot(i), e)));
	}
}

bus_t
busnot(bus_t a)
{
	register int i;
	bus_t r;

	forbus(i) {
		r.wire[i] = gatenot(a.wire[i]);
	}
	return(r);
}

bus_t
busaddc(int carry, bus_t a, bus_t b)
{
	register int i, x;
	bus_t r;

	/* Add, without carry out */
	for (i=0; i<(BUSWIDTH-1); ++i) {
		x = gatexor(a.wire[i], b.wire[i]);
		r.wire[i] = gatexor(carry, x);
		carry = gateor(gateand(carry, x), gateand(a.wire[i], b.wire[i]));
	}
	x = gatexor(a.wire[i], b.wire[i]);
	r.wire[i] = gatexor(carry, x);

	return(r);
}

bus_t
busmul(bus_t a,
bus_t b)
{
	/* Not currently used... but we could multiply */
	register int i, j;
	bus_t r;

	for (i=0; i<BUSWIDTH; ++i) {
		r.wire[i] = 0;
	}
	for (i=0; i<BUSWIDTH; ++i) {
		/* Add r += a if b */
		if (b.wire[i] == 1) {
			r = busaddc(0, r, a);
		} else if (b.wire[i] != 0) {
			bus_t a2;
			for (j=0; j<BUSWIDTH; ++j) {
				a2.wire[j] = gateand(a.wire[j], b.wire[i]);
			}
			r = busaddc(0, r, a2);
		}

		/* Shift a <<= 1 */
		for (j=BUSWIDTH-1; j>0; --j) {
			a.wire[j] = a.wire[j-1];
		}
		a.wire[0] = 0;
	}

	return(r);
}

bus_t
busconst(int v)
{
	bus_t bus;
	register int i;

	forbus(i) {
		/* Constants are 0 or 1 for each wire */
		bus.wire[i] = ((v & (1 << i)) != 0);
	}
	return(bus);
}

bus_t
busloadx(var *varg, int sub)
{
	/* Load a constant subscripted variable */
	register int vnum = VARPTR2NUM(varg);
	bus_t bus;
	register int i;

	sub *= BUSWIDTH;
	forbus (i) bus.wire[i] = vnum + i + sub;
	return(bus);
}

bus_t
busload(var *varg)
{
	/* Load a variable */
	return(busloadx(varg, 0));
}

bus_t
busstorex(int guard, var *varg, bus_t bus, int sub)
{
	/* Store a variable if guard */
	register int vnum = VARPTR2NUM(varg);
	register int i;

	sub *= BUSWIDTH;
	forbus (i) varg->bus.wire[i + sub] = gatemux(guard,
						     bus.wire[i],
						     varg->bus.wire[i + sub]);
	return(varg->bus);
}

bus_t
busstore(int guard, var *varg, bus_t bus)
{
	/* Store a variable if guard */
	return(busstorex(guard, varg, bus, 0));
}

int
stateguard(int state)
{
	/* Compute a single bit guard meaning in this state */
	register int vnum = VARPTR2NUM(statevar);
	register int i, g = 1;

	forbusrev (i) g = gateand(g, gatexor(vnum+i, ((state & (1<<i)) ? 0 : 1)));
	return(g);
}

void
buslab(int guard, int t)
{
	/* Lab on next block */
	busstore(guard, statevar, busconst(t));
}

void
bussel(int guard, bus_t bus, int t, int e)
{
	/* Select operation */
	register int i, a = bus.wire[0];
	bus_t bust = busconst(t);
	bus_t buse = busconst(e);

	forbus (i) a = gateor(a, bus.wire[i]);
	forbus (i) bust.wire[i] = gatemux(a, bust.wire[i], buse.wire[i]);
	busstore(guard, statevar, bust);
}

bus_t
busop(opcode op, bus_t arg0, bus_t arg1)
{
	bus_t bus;
	register int i, j, k;

	switch (op) {
	case ADD:
		bus = busaddc(0, arg0, arg1);
		return(bus);
	case SUB:
		bus = busaddc(1, arg0, busnot(arg1));
		return(bus);
	case AND:
		forbus (i){ bus.wire[i] = gateand(arg0.wire[i], arg1.wire[i]);}
		return(bus);
	case OR:
		forbus (i) {bus.wire[i] = gateor(arg0.wire[i], arg1.wire[i]);}
		return(bus);
	case XOR:
		forbus (i) {bus.wire[i] = gatexor(arg0.wire[i], arg1.wire[i]);}
		return(bus);
	case GT:
		/* Signed greater by (arg1-arg0) is negative */
		bus = busaddc(1, arg1, busnot(arg0));
		i = bus.wire[BUSWIDTH-1];
		bus = busconst(0);
		bus.wire[0] = i;
		return(bus);
	case GE:
		/* Signed greater equal by (arg0-arg1) is non-negative */
		bus = busaddc(1, arg0, busnot(arg1));
		i = bus.wire[BUSWIDTH-1];
		bus = busconst(0);
		bus.wire[0] = gatenot(i);
		return(bus);
	case EQ:
		/* Xor and then tree reduce */
		forbus (i) bus.wire[i] = gatexor(arg0.wire[i], arg1.wire[i]);
		for (i=1; i<BUSWIDTH; i+=i) {
			for (j=0; (j+i)<BUSWIDTH; j+=i) {
				bus.wire[j] = gateor(bus.wire[j], bus.wire[j+i]);
			}
		}
		bus.wire[i] = gatenot(bus.wire[i]);
		for (i=1; i<BUSWIDTH; ++i) bus.wire[i] = 0;
		return(bus);
	case SSL:
		/* Barrel shifter */
		forbus (j) bus.wire[j] = arg0.wire[j];
		for (i=0; i<BUSWIDTH; ++i) {
			forbus (j) {
				/* Sign extend logic */
				k = j + i;
				if (k >= BUSWIDTH) k = BUSWIDTH-1;
				bus.wire[j] = gatemux(arg1.wire[i],
						      bus.wire[k],
						      bus.wire[j]);
			}
		}
		return(bus);
	case SSR:
		/* Barrel shifter */
		forbus (j) bus.wire[j] = arg0.wire[j];
		for (i=0; i<BUSWIDTH; ++i) {
			forbus (j) {
				bus.wire[j] = gatemux(arg1.wire[i],
						      ((j > i) ? 0 : bus.wire[k]),
						      bus.wire[j]);
			}
		}
		return(bus);
	}

	return(arg0);
}


void
recurmark(register int i)
{
	/* Recursively mark needed vars and gates */
	if (i >= VARBIAS) return;
	if (++(gate[i].needed) > 1) return;
	recurmark(gate[i].arg0);
	recurmark(gate[i].arg1);
}

char *
gatename(register int a)
{
	/* Convert gate a into a printable string */
	static char namestr[1024];

	if (a < 2) {
		sprintf(namestr, "_%u", a);
	} else if (a < VARBIAS) {
		sprintf(namestr, "G%u", gate[a].newno);
	} else {
		var *p = NUM2VARPTR(a);
		sprintf(namestr,
			"%s_%d_%d_%d",
			p->text,
			p->deflev,
			p->defblk,
			(a % VARBIAS));
	}
	return(namestr);
}

char *
vname(register int a)
{
	/* Convert gate a into a verilog name */
	static char namestr[1024];

	if (a < 2) {
		sprintf(namestr, "%u", a);
	} else if (a < VARBIAS) {
		sprintf(namestr, "w[%d]", gate[a].newno-2);
	} else {
		var *p = NUM2VARPTR(a);
		sprintf(namestr,
			"%s_%d_%d[%d]",
			p->text,
			p->deflev,
			p->defblk,
			(a % VARBIAS));
	}
	return(namestr);
}

static char *
gatefunc(opcode op)
{
	switch (op) {
	case AND:	return("and");
	case OR:	return("or");
	case XOR:	return("xor");
	case NAND:	return("nand");
	case NOR:	return("nor");
	}
	return("BADOP");
}

void
dumpgates(int haltstate)
{
	register int i, j, k, maxlevel = 0;

	/* Mark which gates are really used by assignments */
	for (i=0; i<MAXV; ++i) {
		if (symtab[i].type == WORD) {
			forbus (j) recurmark(symtab[i].bus.wire[j]);
		}
	}

	/* Renumber the needed ones and set level depth */
	gatesneed = 0;
	forgates (i) {
		if (gate[i].needed) {
			gate[i].newno = gatesneed;
			++gatesneed;

			/* compute level in level-order schedule */
			if (i < 2) {
				gate[i].level = 0;
			} else {
				register int a0 = gate[i].arg0;
				register int a1 = gate[i].arg1;
				a0 = ((a0 >= VARBIAS) ? 0 : gate[a0].level);
				a1 = ((a1 >= VARBIAS) ? 0 : gate[a1].level);
				gate[i].level = (a0 = 1 + ((a0 > a1) ? a0 : a1));
				if (a0 > maxlevel) maxlevel = a0;
			}
		} else {
			gate[i].newno = -i; /* to make errors obvious */
		}
	}

	/* Output gate assignments? */
	if (outtyp & OUTGATE) {
		//printf("TEST!!!!!");
		/* Output the logic formula for each gate */
		for (i=2; i<gatesp; ++i) {
			//printf("TEST2!!!!!");
			if (gate[i].needed) {
				printf("G%u = %s(%s, ", gate[i].newno, gatefunc(gate[i].op), gatename(gate[i].arg0));
				printf("%s)\n", gatename(gate[i].arg1));
			}
		}

		/* Spit-out needed output definitions */
		for (i=0; i<MAXV; ++i) {
			if (symtab[i].type == WORD) {
				forbus (j) {
					/* Any variable bit that changed value */
					if (symtab[i].bus.wire[j] != (VARPTR2NUM(&(symtab[i])) + j)) {
						printf("_%s = ", gatename((i * VARBIAS) + j));
						printf("%s\n", gatename(symtab[i].bus.wire[j]));
					}
				}
			}
		}

		printf("\n\n");
	}

	/* Output verilog code? */
	if (outtyp & OUTVER) {
		printf("module statemachine(halt, clk);\n"
		       "output halt;\n"
		       "input clk;\n");

		/* Define all variables */
		for (i=0; i<MAXV; ++i) {
			if (symtab[i].type == WORD) {
				var *p = NUM2VARPTR(i * VARBIAS);

				printf("reg [%d:0] %s_%d_%d%s;\n",
				       ((BUSWIDTH * p->dim) - 1),
				       p->text,
				       p->deflev,
				       p->defblk,
				       ((p == statevar) ? " = 0" : ""));
			}
		}
		printf("wire [%d:0] w;\n\n", gatesneed-3);

		/* Output the assignments */
		for (i=2; i<gatesp; ++i) {
			if (gate[i].needed) {
				/* Note: vname uses a static buffer,
				   so need two printfs below
				*/
				printf("%s(w[%u], %s, ",
				       gatefunc(gate[i].op),
				       gate[i].newno-2,
				       vname(gate[i].arg0));
				printf("%s);\n",
				       vname(gate[i].arg1));
			}
		}
		printf("assign halt = (STATE_0_0 == %u);\n", haltstate);

		/* Spit-out clocked updates */
		printf("always @(posedge clk) if (!halt) begin\n");
		for (i=0; i<MAXV; ++i) {
			if (symtab[i].type == WORD) {
				forbus (j) {
					/* Any variable bit that changed value */
					if (symtab[i].bus.wire[j] != (VARPTR2NUM(&(symtab[i])) + j)) {
						printf("\t%s <= ", vname((i * VARBIAS) + j));
						printf("%s;\n", vname(symtab[i].bus.wire[j]));
					}
				}
			}
		}
		printf("end\n"
		       "endmodule\n"
		       "\n"
		       "module testbench;\n"
		       "wire halt;\n"
		       "reg clk = 0;\n"
		       "statemachine m(halt, clk);\n"
		       "initial begin\n"
		       "\t$dumpfile;\n"
		       "\t$dumpvars(1");

		/* Dump all variables */
		for (i=0; i<MAXV; ++i) {
			if (symtab[i].type == WORD) {
				var *p = NUM2VARPTR(i * VARBIAS);

				printf(",\nm.%s_%d_%d", p->text, p->deflev, p->defblk);
			}
		}
		printf(");\n"
		       "\twhile (!halt) begin\n"
		       "\t\t#1 clk <= !clk;\n"
		       "\tend\n"
		       "\t$finish;\n"
		       "end\n"
		       "endmodule\n");
	}

	/* Output dot file? */
	if (outtyp & OUTDOT) {
		printf("digraph gates {\n"
		       "ordering=out;\n"
		       "clusterrank=global;\n"
		       "size=\"6,4\";\n"
		       "ratio=fill;\n"
		       "remincross=true;\n"
		       "rankdir=LR;\n"
		       "style=\"invis\";\n"
		       "node [fontname=Helvetica];\n");

		/* Spit-out input definitions */
		printf("node [color=\"%f,1.0,1.0\"];\n"
		       "subgraph cluster_%d { rank=same;\n",
		       0.0, 0);
		printf("%s [label=\"", gatename(0));
		printf("%s\"];\n", gatename(0));
		printf("%s [label=\"", gatename(1));
		printf("%s\"];\n", gatename(1));
		for (i=0; i<MAXV; ++i) {
			if (symtab[i].type == WORD) {
				forbus (j) {
					printf("%s [label=\"", gatename((i * VARBIAS) + j));
					printf("%s\"];\n", gatename((i * VARBIAS) + j));
				}
			}
		}
		printf("}\n");

		/* Output the gates as same-rank nodes */
		for (k=1; k<=(maxlevel+1); ++k) {
			printf("node [color=\"%f,1.0,1.0\"];\n"
			       "subgraph cluster_%d { rank=same;\n",
			       (k / (maxlevel+2.0)),
			       k);
			for (i=2; i<gatesp; ++i) {
				if (gate[i].needed && (gate[i].level == k)) {
					printf("G%u [label=\"%s\"];\n", gate[i].newno, gatefunc(gate[i].op));
				}
			}

			for (i=0; i<MAXV; ++i) {
				if (symtab[i].type == WORD) {
					forbus (j) {
						/* Any variable bit that changed value */
						if (symtab[i].bus.wire[j] != (VARPTR2NUM(&(symtab[i])) + j)) {
							register int lev = symtab[i].bus.wire[j];

							if ((lev < 2) || (lev >= VARBIAS)) {
								lev = 1;
							} else {
								lev = gate[lev].level + 1;
							}

							if (lev == k) {
								printf("_%s [label=\"", gatename((i * VARBIAS) + j));
								printf("_%s\"];\n", gatename((i * VARBIAS) + j));
							}
						}
					}
				}
			}

			printf("}\n");
		}

		/* Spit-out needed output definitions */
		printf("{\n"
		       "rank = same;\n");
		for (i=0; i<MAXV; ++i) {
			if (symtab[i].type == WORD) {
				forbus (j) {
					/* Any variable bit that changed value */
					if (symtab[i].bus.wire[j] != (VARPTR2NUM(&(symtab[i])) + j)) {
						printf("_%s [label=\"", gatename((i * VARBIAS) + j));
						printf("_%s\"];\n", gatename((i * VARBIAS) + j));
					}
				}
			}
		}
		printf("}\n");

		/* Output the wires to each gate */
		for (i=2; i<gatesp; ++i) {
			if (gate[i].needed) {
				/* Arc color matches source */

				k = gate[i].arg0;
				if (k >= VARBIAS) k = 0; else k = gate[k].level;
				printf("%s -> G%u ", gatename(gate[i].arg0), gate[i].newno);
				printf("[color=\"%f,1.0,1.0\"];\n",
				       (k / (maxlevel+2.0)));

				k = gate[i].arg1;
				if (k >= VARBIAS) k = 0; else k = gate[k].level;
				printf("%s -> G%u ", gatename(gate[i].arg1), gate[i].newno);
				printf("[color=\"%f,1.0,1.0\"];\n",
				       (k / (maxlevel+2.0)));
			}
		}

		/* Output the wires to each variable */
		for (i=0; i<MAXV; ++i) {
			if (symtab[i].type == WORD) {
				forbus (j) {
					/* Any variable bit that changed value */
					if (symtab[i].bus.wire[j] != (VARPTR2NUM(&(symtab[i])) + j)) {
						/* Arc color matches source */
						k = symtab[i].bus.wire[j];
						if (k >= VARBIAS) k = 0; else k = gate[k].level;
						printf("%s -> ", gatename(symtab[i].bus.wire[j]));
						printf("_%s ", gatename((i * VARBIAS) + j));
						printf("[color=\"%f,1.0,1.0\"];\n",
						       (k / (maxlevel+2.0)));
					}
				}
			}
		}

		printf("}\n");
	}
}
