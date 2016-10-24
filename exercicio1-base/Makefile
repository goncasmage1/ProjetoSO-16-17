main: commandlinereader.o contas.o i-banco.o
	gcc -pthread -o main commandlinereader.o contas.o i-banco.o

commandlinereader.o : commandlinereader.c commandlinereader.h
	gcc $(CFLAGS) -c commandlinereader.c

contas.o : contas.c contas.h
	gcc $(CFLAGS) -c contas.c

i-banco.o: i-banco.c commandlinereader.h contas.h
	gcc $(CFLAGS) -c i-banco.c

clean:
	rm -f *.o main

CFLAGS = -g -Wall -pedantic