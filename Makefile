all:	bborig bb bbgrad bbgates tar
	echo "all done"

test:	bborig bb bbgrad bbgates test.c testg.c
	./bborig -s <test.c >orig
	./bb -s <test.c >output
	./bbgrad -p <test.c >grad
	./bbgates -d <testg.c >gates.dot
	dot -Tps <gates.dot >gates.ps
	./bbgates -g <testg.c >gates
	./bbgates -p <testg.c >gates.par
	./bbgates -s <testg.c >gates.seq
	./bbgates -v <testg.c >gates.v

bb:	bb1.o bb2.o bb3.o bb4.o bb5.o bb6.o
	cc bb1.o bb2.o bb3.o bb4.o bb5.o bb6.o -o bb

bborig:	bb1.o bb2.o bb3.o bb4.o bb5orig.o bb6.o
	cc bb1.o bb2.o bb3.o bb4.o bb5orig.o bb6.o -o bborig

bbgrad:	bb1.o bb2.o bb3.o bb4.o bb5grad.o bb6.o
	cc bb1.o bb2.o bb3.o bb4.o bb5grad.o bb6.o -o bbgrad

bbgates:	bb1.o bb2.o bb3.o bb4.o bb5gates.o bb6.o
	cc bb1.o bb2.o bb3.o bb4.o bb5gates.o bb6.o -o bbgates

bb1.o:	bb1.c bb.h
	cc bb1.c -c -O

bb2.o:	bb2.c bb.h
	cc bb2.c -c -O

bb3.o:	bb3.c bb.h
	cc bb3.c -c -O

bb4.o:	bb4.c bb.h
	cc bb4.c -c -O

bb5orig.o:	bb5orig.c bb.h
	cc bb5orig.c -c -O

bb5.o:	bb5.c bb.h
	cc bb5.c -c -O

bb6.o:	bb6.c bb.h
	cc bb6.c -c -O

bb5grad.o:	bb5grad.c bb.h
	cc bb5grad.c -c -O

bb5gates.o:	bb5gates.c bb.h
	cc bb5gates.c -c -O

clean:	
	rm -f *.o bborig bb bbgrad bbgates

tar:	WilkersonSubmissionAssignment3.tgz
	echo "tar made"

WilkersonSubmissionAssignment3.tgz:	bb.h bb1.c bb2.c bb3.c bb4.c bb5orig.c bb5.c bb5grad.c bb5gates.c bb6.c Makefile test.c testg.c notes.pdf
	tar -zcvf WilkersonSubmissionAssignment3.tgz bb.h bb1.c bb2.c bb3.c bb4.c bb5orig.c bb5.c bb5grad.c bb5gates.c bb6.c Makefile test.c testg.c notes.pdf

