CFLAGS=-Wall
FICHEIROS=Makefile argus.c argus.h argusd.c
EXECUTAVEL=argus
SERVIDOR=argusd
APAGAR=log log.idx historico aux pipe
CC=gcc

all: argus.o argusd.o 
	$(CC) -o $(EXECUTAVEL) argus.o 
	$(CC) -o $(SERVIDOR) argusd.o 

zip: $(FICHEIROS)
	zip -9 SO1920.zip $(FICHEIROS)

clean:
	rm -rf *.o $(EXECUTAVEL) $(SERVIDOR) $(APAGAR)

argus.o: argus.c argus.h
argusd.o: argusd.c argus.h