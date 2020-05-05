CC = gcc
CFLAGS = -g

TARGETO = oss
TARGETP = userProcess

OBJECTO = oss.o
OBJECTP = userProcess.o
HEADER = structures.h

.SUFFIXES = .c .o .h

all: oss userProcess

$(TARGETO): $(OBJECTO) 
	$(CC) -o $(TARGETO) $(OBJECTO) -lm -lpthread

$(TARGETP): $(OBJECTP)
	$(CC) -o $(TARGETP) $(OBJECTP) -lm -lpthread

.c.o: $(HEADER)
	 $(CC) $(CFLAGS) -c $< -lm

.PHONY: clean
clean:
	rm -f *.o *.dat $(TARGETO) $(TARGETP)

