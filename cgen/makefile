L = ../front/
CC = gcc -g -I$(L)
LIBS = $(L)scanner.o $(L)token.o $(L)ast.o $(L)sym.o $(L)type.o $(L)loc.o

all: cgen1 cgen2 

cgen1: cgen1.o code.o $(L)parser1.c
	$(CC) -o cgen1 cgen1.o code.o $(L)parser1.c $(LIBS)

cgen2: cgen2.o code.o $(L)parser2.c
	$(CC) -o cgen2 cgen2.o code.o $(L)parser2.c $(LIBS)

cgen1.o : cgen1.c code.h 
cgen2.o : cgen2.c code.h
code.o : code.c code.h

.PHONY:test
test: cgen1 cgen2
	@echo "Test 1(inside block)"
	@echo
	@cat -n test/test01.txt
	@echo "----"
	@./cgen1 < test/test01.txt
	@echo "----"
	@echo "Test 2(outside block)"
	@echo
	@cat -n test/test02.txt
	@echo "----"
	@./cgen2 < test/test02.txt

clean:
	-rm cgen? *.o out? core*
