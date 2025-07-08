NAME		:= ha2
OBJFILES	:= build/operations.o \
				 build/filesystem.o \
				 build/utils.o \
				 build/ha2.o  \
				 build/linenoise.o
CFLAGS		:= -Wall -g -D DEBUG
CC			:= clang

build/$(NAME): $(OBJFILES) | build
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c -o $@ $^

build:
	mkdir -p $@

build/operations.so: src/operations.c
	clang -shared -fPIC -o ./build/operations.so ./src/operations.c ./src/filesystem.c

test: build/operations.so
	python3 -m pytest

test_%:build/operations.so
	python3 -m pytest -k $@

clean:
	rm -f build/* 

pack:
	zip submission.zip src/operations.c
