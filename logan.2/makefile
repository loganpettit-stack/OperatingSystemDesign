CC = gcc
CFLAGS = -g

TARGETO = oss
TARGETP = prime

OBJECTO = main.o
OBJECTP = prime.o

all: oss prime

$(TARGETO): $(OBJECTO)
	$(CC) -o $(TARGETO) $(OBJECTO)

$(TARGETP): $(OBJECTP)
	$(CC) -o $(TARGETP) $(OBJECTP)

.PHONY: clean
clean:
	rm -f *.o $(TARGETO) $(TARGETP)
