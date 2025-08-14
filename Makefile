BUILD := build
OBJ := obj
CFLAGS := -Wall -Wextra -pedantic -Wshadow -Werror

all: $(BUILD)/lexer

clean:
	rm -f $(BUILD)/* $(OBJ)/*

$(BUILD):
	mkdir $(BUILD)

$(OBJ):
	mkdir $(OBJ)

$(BUILD)/preproc: preproc_main.c $(OBJ)/preproc.o $(OBJ)/util.o $(OBJ)/args.o $(BUILD)
	gcc -o $(BUILD)/preproc preproc_main.c $(OBJ)/preproc.o $(OBJ)/util.o $(OBJ)/args.o

$(OBJ)/preproc.o: preproc.c preproc.h types.h util.h args.h $(OBJ)
	gcc -o $(OBJ)/preproc.o -c preproc.c $(CFLAGS)

$(OBJ)/util.o: util.c util.h types.h args.h $(OBJ)
	gcc -o $(OBJ)/util.o -c util.c $(CFLAGS)

$(OBJ)/args.o: args.c args.h types.h $(OBJ)
	gcc -o $(OBJ)/args.o -c args.c $(CFLAGS)

$(BUILD)/test: test.c $(OBJ)/lexer.o $(OBJ)/preproc.o $(OBJ)/util.o $(OBJ)/args.o $(OBJ)/map.o $(BUILD)
	gcc -o $(BUILD)/test test.c $(OBJ)/lexer.o $(OBJ)/preproc.o $(OBJ)/util.o $(OBJ)/args.o $(OBJ)/map.o

$(BUILD)/lexer: lexer_main.c $(OBJ)/lexer.o $(OBJ)/preproc.o $(OBJ)/util.o $(OBJ)/args.o $(OBJ)/map.o $(BUILD)
	gcc -o $(BUILD)/lexer -DSTRIP_COMMENTS lexer_main.c $(OBJ)/lexer.o $(OBJ)/preproc.o $(OBJ)/util.o $(OBJ)/args.o $(OBJ)/map.o

$(OBJ)/lexer.o: lexer.c lexer.h preproc.h types.h util.h args.h c-hashmap/map.h $(OBJ)
	gcc -o $(OBJ)/lexer.o -c lexer.c $(CFLAGS)

$(OBJ)/map.o: c-hashmap/map.c c-hashmap/map.h $(OBJ)
	gcc -o $(OBJ)/map.o -c c-hashmap/map.c $(CFLAGS)
