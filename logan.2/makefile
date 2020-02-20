CC = gcc
CFLAGS = -g

TARGETO = oss
TARGETP = prime

OBJECTO = main.o
OBJECTP = prime.o

all: oss prime

$(TARGETO): $(OBJECTO)
	$(CC) -o $(TARGETO) $(OBJECTO) -lm

$(TARGETP): $(OBJECTP)
	$(CC) -o $(TARGETP) $(OBJECTP) -lm

.PHONY: clean
clean:
	rm -f *.o $(TARGETO) $(TARGETP)
