CC = gcc
CFLAGS = -g

TARGETO = oss
TARGETP = prime

OBJECTO = main.o
OBJECTP = prime.o

.SUFFIXES = .c .o

all: oss prime

$(TARGETO): $(OBJECTO)
	$(CC) -o $(TARGETO) $(OBJECTO) -lm

$(TARGETP): $(OBJECTP)
	$(CC) -o $(TARGETP) $(OBJECTP) -lm

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f *.o *.log $(TARGETO) $(TARGETP)
