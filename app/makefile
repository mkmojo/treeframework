# makefile for digits recognizer

.SUFFIXES: .o .cpp

CC = g++ -D x86
CFLAGS = -g -O3 -D_REENTRANT -std=c++0x
LFLAGS = -lm

CTAGS =     ctags
BIN = run
CFILES =   main.cpp
OBJECTS =  main.o
HFILES = ../lib/treedef.hpp
OTHERSOURCES =  
SOURCES =   $(HFILES) $(CFILES) $(OTHERSOURCES)

.cpp.o:  
	$(CC) -c $(CFLAGS) $*.cpp

$(BIN): $(OBJECTS)
	$(CC) $(LFLAGS) -o $(BIN) $(OBJECTS)

tags: $(CFILES) $(HFILES) $(EXTRACTCFILES) $(RECOGCFILES)
# for vi:
	$(CTAGS) -d -t -T -w $(CFILES) $(HFILES) 
	sort -o tags tags

clean:
	-rm $(BIN) $(OBJECTS) 2> /dev/null
