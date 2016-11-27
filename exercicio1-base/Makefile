main: contas.o i-banco.o
	gcc -pthread -o main contas.o i-banco.o

terminal: commandlinereader.o i-banco-terminal.o contas.o
	gcc -pthread -o terminal commandlinereader.o i-banco-terminal.o contas.o

commandlinereader.o : commandlinereader.c commandlinereader.h
	gcc $(CFLAGS) -c commandlinereader.c

contas.o : contas.c contas.h
	gcc $(CFLAGS) -c contas.c

i-banco-terminal.o: i-banco-terminal.c commandlinereader.h contas.h
	gcc $(CFLAGS) -c i-banco-terminal.c

i-banco.o: i-banco.c contas.h
	gcc $(CFLAGS) -c i-banco.c

clean:
	rm -f *.o main terminal i-banco-pipe log.txt

CFLAGS = -g -Wall -pedantic