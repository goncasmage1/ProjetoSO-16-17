i-banco: contas.o i-banco.o
	gcc -pthread -o i-banco contas.o i-banco.o

i-banco-terminal: commandlinereader.o i-banco-terminal.o contas.o
	gcc -pthread -o i-banco-terminal commandlinereader.o i-banco-terminal.o contas.o

commandlinereader.o : commandlinereader.c commandlinereader.h
	gcc $(CFLAGS) -c commandlinereader.c

contas.o : contas.c contas.h
	gcc $(CFLAGS) -c contas.c

i-banco-terminal.o: i-banco-terminal.c commandlinereader.h contas.h
	gcc $(CFLAGS) -c i-banco-terminal.c

i-banco.o: i-banco.c contas.h
	gcc $(CFLAGS) -c i-banco.c

clean:
	rm -f *.o *.txt *terminal-* i-banco i-banco-terminal i-banco-pipe

CFLAGS = -g -Wall -pedantic