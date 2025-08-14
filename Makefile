BINC := ../../BINC
BOBJ := ../../BOBJ
BUILD := build
OBJECTS := obj
CFLAGS := -Wall -Wextra -pedantic -Wshadow -Werror

all: 

clean:
	rm -f lexer preproc obj/*

preproc: preproc_main.c $(OBJECTS)/preproc.o $(OBJECTS)/util.o $(OBJECTS)/args.o
	gcc -o preproc preproc_main.c $(OBJECTS)/preproc.o $(OBJECTS)/util.o $(OBJECTS)/args.o

$(OBJECTS)/preproc.o: preproc.c preproc.h $(BINC)/types.h util.h args.h
	gcc -o $(OBJECTS)/preproc.o -c preproc.c $(CFLAGS)

$(OBJECTS)/util.o: util.c util.h $(BINC)/types.h args.h
	gcc -o $(OBJECTS)/util.o -c util.c $(CFLAGS)

$(OBJECTS)/args.o: args.c args.h $(BINC)/types.h
	gcc -o $(OBJECTS)/args.o -c args.c $(CFLAGS)

test: test.c $(OBJECTS)/lexer.o $(OBJECTS)/preproc.o $(OBJECTS)/util.o $(OBJECTS)/args.o $(OBJECTS)/map.o
	gcc -o test test.c $(OBJECTS)/lexer.o $(OBJECTS)/preproc.o $(OBJECTS)/util.o $(OBJECTS)/args.o $(OBJECTS)/map.o

lexer: lexer_main.c $(OBJECTS)/lexer.o $(OBJECTS)/preproc.o $(OBJECTS)/util.o $(OBJECTS)/args.o $(OBJECTS)/map.o
	gcc -o lexer -DSTRIP_COMMENTS lexer_main.c $(OBJECTS)/lexer.o $(OBJECTS)/preproc.o $(OBJECTS)/util.o $(OBJECTS)/args.o $(OBJECTS)/map.o

$(OBJECTS)/lexer.o: lexer.c lexer.h preproc.h $(BINC)/types.h util.h args.h c-hashmap/map.h
	gcc -o $(OBJECTS)/lexer.o -c lexer.c $(CFLAGS)

$(OBJECTS)/map.o: c-hashmap/map.c c-hashmap/map.h
	gcc -o $(OBJECTS)/map.o -c c-hashmap/map.c $(CFLAGS)
