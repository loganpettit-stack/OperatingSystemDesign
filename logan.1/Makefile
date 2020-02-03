all:	bt

bt:	main.o TreeTraversal.o
	gcc -o bt main.o TreeTraversal.o

main.o:	main.c
	gcc -c main.c

TreeTraversal.o: TreeTraversal.c
	gcc -c TreeTraversal.c

clean:
	rm -f *.o
