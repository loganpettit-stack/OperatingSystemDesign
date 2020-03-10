CC = gcc
CFLAGS = -g

TARGETO = oss
TARGETP = prime

OBJECTO = master.o
OBJECTP = bin_adder.o

.SUFFIXES = .c .o

all: master bin_adder

$(TARGETO): $(OBJECTO)
        $(CC) -o $(TARGETO) $(OBJECTO) -lm

$(TARGETP): $(OBJECTP)
        $(CC) -o $(TARGETP) $(OBJECTP) -lm

.c.o:
        $(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
        rm -f *.o *.log $(TARGETO) $(TARGETP)

