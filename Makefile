
CC = gcc
CFLAG = -Wall

all : sfs.a

sfs.a : SFSFile.o SFS.o
	ar cr $@ $^

SFS.o : SFS.c SFS.h SFSDef.h
	$(CC) -c ${CFLAG} SFS.c -o $@

SFSFile.o : SFSFile.c SFS.h SFSDef.h
	$(CC) -c ${CFLAG} SFSFile.c -o $@

.PHONY : clean
clean :
	-rm *.o *.a
