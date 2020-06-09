CFLAGS=-Wall
FICHEIROS=Makefile 
EXECUTAVEL=argus
SERVIDOR=server
APAGAR=log aux pipe
CC=gcc

all: argus.o server.o
	$(CC) -o $(EXECUTAVEL) argus.o 
	$(CC) -o $(SERVIDOR) server.o	

zip: $(FICHEIROS)
	zip -9 SO1920.zip $(FICHEIROS)

clean:
	rm -rf *.o $(EXECUTAVEL) $(SERVIDOR) $(APAGAR)

argus.o: argus.c argus.h
server.o: server.c argus.h