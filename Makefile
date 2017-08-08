
CC = gcc
CFLAG = -Wall

all : libsfs.a test

libsfs.a : SFSFile.o SFS.o
	ar cr $@ $^

SFS.o : SFS.c SFS.h SFSDef.h
	$(CC) -g -c ${CFLAG} SFS.c -o $@

SFSFile.o : SFSFile.c SFS.h SFSDef.h
	$(CC) -g -c ${CFLAG} SFSFile.c -o $@
	
test: Test.c SFS.h SFSDef.h
	$(CC) -g Test.c -L. -lsfs -o $@

.PHONY : clean
clean :
	-rm *.o *.a test
