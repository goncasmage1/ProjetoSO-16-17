/*
// Projeto SO - exercicio 1, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
*/

#include "commandlinereader.h"
#include "contas.h"
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_SAIR_AGORA "agora"

#define MAXARGS 3
#define BUFFER_SIZE 100
#define MAXTAREFA 20


int main (int argc, char** argv) {

	char *args[MAXARGS + 1];
	char buffer[BUFFER_SIZE];
	/*Guarda o numero de processos filhos criados*/
	int index = 0;
	/*Guarda os pids de todos os processos criados*/
	pid_t processos[MAXTAREFA];
	signal(SIGUSR1, terminarASAP);


	inicializarContas();

	printf("Bem-vinda/o ao i-banco\n\n");

	while (1) {

		int numargs;
		numargs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE);

		/* EOF (end of file) do stdin ou comando "sair" */
		if (numargs < 0 ||
			(numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))) {

			puts("i-banco vai terminar.\n--");
			int i, pid, status;

			/*Sair agora - chama kill a todos os processos filho*/
			if (args[1] != NULL && (strcmp(args[1], COMANDO_SAIR_AGORA) == 0)) {
				for (i = 0; i < index; i++) {
					kill(processos[i], SIGUSR1);
				}
			}

			/*Termina os processos zombie antes de o programa acabar*/
			for (i = 0; i < index; i++) {
				pid = wait(&status);
				printf("FILHO TERMINADO (PID=%d; ", pid);

				if (status == 0) {
					puts("terminou normalmente)");
				} else {
					puts("terminou abruptamente)");
				}
			}

			puts("--\ni-banco terminou.");

			exit(EXIT_SUCCESS);
		}

		else if (numargs == 0)
			/* Nenhum argumento; ignora e volta a pedir */
			continue;
			
		/* Debitar */
		else if (strcmp(args[0], COMANDO_DEBITAR) == 0) {
			int idConta, valor;

			if (numargs < 3) {
				printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_DEBITAR);
			   continue;
			}

			idConta = atoi(args[1]);
			valor = atoi(args[2]);

			if (debitar (idConta, valor) < 0)
				printf("%s(%d, %d): ERRO\n\n", COMANDO_DEBITAR, idConta, valor);
			else
				printf("%s(%d, %d): OK\n\n", COMANDO_DEBITAR, idConta, valor);
		}

		/* Creditar */
		else if (strcmp(args[0], COMANDO_CREDITAR) == 0) {
			int idConta, valor;

			if (numargs < 3) {
				printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_CREDITAR);
				continue;
			}

			idConta = atoi(args[1]);
			valor = atoi(args[2]);
		
			if (creditar (idConta, valor) < 0)
				printf("%s(%d, %d): Erro\n\n", COMANDO_CREDITAR, idConta, valor);
			else
				printf("%s(%d, %d): OK\n\n", COMANDO_CREDITAR, idConta, valor);
		}

		/* Ler Saldo */
		else if (strcmp(args[0], COMANDO_LER_SALDO) == 0) {
			int idConta, saldo;

			if (numargs < 2) {
				printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_LER_SALDO);
				continue;
			}
			idConta = atoi(args[1]);
			saldo = lerSaldo (idConta);
			if (saldo < 0)
				printf("%s(%d): Erro.\n\n", COMANDO_LER_SALDO, idConta);
			else
				printf("%s(%d): O saldo da conta é %d.\n\n", COMANDO_LER_SALDO, idConta, saldo);
		}

		/* Simular */
		else if (strcmp(args[0], COMANDO_SIMULAR) == 0) {
			int anos = atoi(args[1]);

			/*Se o numero de anos for valido*/
			if (anos > 0) {
				//Cria um processo filho
				pid_t pid = fork();

				/*O processo filho faz a simulacao*/
				if (pid == 0) {
					simular(anos);
					exit(0);
				}
				/*O processo pai adiciona o pid do
				processo filho ao vetor de pid's */
				else if (pid > 0){
					processos[index++] = pid;
				}
				else {
					puts("Erro a criar o processo filho");
				}
			}
			else 
				printf("Numero de anos invalido\n");
		} else {
			printf("Comando desconhecido. Tente de novo.\n");
		}
	}
}

