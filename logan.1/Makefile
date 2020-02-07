all:	bt

bt:	main.o TreeTraversal.o FileOptionHandler.o
	gcc -o bt main.o TreeTraversal.o FileOptionHandler.o

main.o:	main.c
	gcc -c main.c

TreeTraversal.o: TreeTraversal.c
	gcc -c TreeTraversal.c

FileOptionHandler.o: FileOptionHandler.c
	gcc -c FileOptionHandler.c

clean:
	rm -f *.o
