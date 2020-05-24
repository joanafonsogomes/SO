CFLAGS=-Wall
FICHEIROS=Makefile 
EXECUTAVEL=argus
CC=gcc

$(EXECUTAVEL): argus.o
	$(CC) -o $(EXECUTAVEL) argus.o

zip: $(FICHEIROS)
	zip -9 SO1920.zip $(FICHEIROS)

clean:
	rm -rf *.o $(EXECUTAVEL)

argus.o: argus.c