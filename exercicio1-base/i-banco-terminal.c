/*
// Projeto SO - exercicio 1, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
*/

#include "commandlinereader.h"
#include "contas.h"
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_TRANSFERIR "transferir"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_SAIR_AGORA "agora"
#define COMANDO_SAIR_TERMINAL "sair-terminal"

#define DEBITAR 1
#define CREDITAR 2
#define LER_SALDO 3
#define TRANSFERIR 4

#define MAXARGS 4
#define BUFFER_SIZE 100
#define CMD_BUFFER_DIM  (NUM_TRABALHADORAS * 2)

/*struct que representa um comando introduzido pelo utilizador*/
typedef struct
{
	int operacao;
	int idConta_1;
	int idConta_2;
	int valor;
	/*Indica se precisamos de considerar o valor da conta 2*/
	int com_conta_2;
} comando_t;


/**Variaveis globais*/
char *args[MAXARGS + 1];
/*Nome do pipe a criar*/
const char *ficheiro;
/*Guarda o pid do i-banco*/
pid_t banco_pid;
/**Variaveis globais*/


int main(int argc, char** argv) {

	if (argc == 2) {
		ficheiro = strdup(argv[1]);
	}
	else {
		perror("Erro: Nao foi especificado nenhum ficheiro como pipe.\n");
    	exit(1);
	}

	/*Cria um processo filho*/
	pid_t pid_banco = fork();

	/*O processo filho corre o i-banco*/
	if (pid_banco == 0) {
		//execv("nome do executavel", ficheiro);
	}
	/*O processo pai associa o pid do processo filho
	ao seu pid do i-banco*/
	else if (pid_banco > 0){
		banco_pid = pid_banco;
	}
	else {
		puts("Erro a criar o processo filho");
	}

	printf("Bem-vinda/o ao i-banco\n\n");

	while (1) {

		int numargs = readLineArguments(args, MAXARGS + 1, buffer, BUFFER_SIZE);

		if (numargs == 0)
			/* Nenhum argumento; ignora e volta a pedir */
			continue;

		/* EOF (end of file) do stdin ou comando "sair-terminal" */
		else if (numargs < 0 ||
			(numargs > 0 && (strcmp(args[0], COMANDO_SAIR_TERMINAL) == 0))) {

			puts("\ni-banco-terminal terminou.\n");
			return 0;
		}
			
		/* Debitar */
		else if (strcmp(args[0], COMANDO_DEBITAR) == 0) {

			if (numargs < 3) {
				printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_DEBITAR);
				continue;
			}
			novaTarefa(DEBITAR, atoi(args[1]), -1, atoi(args[2]), 0);
		}

		/* Creditar */
		else if (strcmp(args[0], COMANDO_CREDITAR) == 0) {
			if (numargs < 3) {
				printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_CREDITAR);
				continue;
			}
			novaTarefa(CREDITAR, atoi(args[1]), -1, atoi(args[2]), 0);
		}

		/* Ler Saldo */
		else if (strcmp(args[0], COMANDO_LER_SALDO) == 0) {
			if (numargs < 2) {
				printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_LER_SALDO);
				continue;
			}
			novaTarefa(LER_SALDO, atoi(args[1]), -1, 0, 0);
		}

		/* Transferir */
		else if (strcmp(args[0], COMANDO_TRANSFERIR) == 0) {
			if (numargs < 4) {
				printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_TRANSFERIR);
				continue;
			}
			novaTarefa(TRANSFERIR, atoi(args[1]), atoi(args[2]), atoi(args[3]), 1);
		}

		/* Simular */
		else if (strcmp(args[0], COMANDO_SIMULAR) == 0) {


			int anos = atoi(args[1]);

			/*Se o numero de anos for valido*/
			if (anos > 0) {

				/*Aguarda que todas as tarefas que estao a ser executadas terminem*/
				espera = 1;
				while (contadorTarefas > 0 || buff_write_idx != buff_read_idx) {
					pthread_cond_wait(&pode_simular, &mutex_cond);
				}
				/*Cria um processo filho*/
				pid_t pid = fork();

				/*O processo filho faz a simulacao*/
				if (pid == 0) {
					simular(anos);
					exit(0);
				}
				/*O processo pai adiciona o pid do
				processo filho ao vetor de pid's */
				else if (pid > 0){
					processos[indice++] = pid;

					/*Indica que as tarefas podem resumir as operacoes*/
					espera = 0;
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
	return 0;
}

void novaTarefa(int op, int id_1, int id_2, int val, int duas_contas) {

	/*Decrementa o semaforo de escrita (Indica que o pipe tem um comando novo para ler)*/
	sem_wait(&sem_esc);
	pthread_mutex_lock(&mutex_esc);

	int fd;
	fd = open(ficheiro, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1) {
		perror("Erro a abrir o ficheiro indicado.\n");
    	exit(1);
	}
	/*Adiciona um comando ao pipe*/
	comando_t com;
	com.operacao = op;
	com.idConta_1 = id_1;
	com.idConta_2 = id_2;
	com.valor = val;
	com.com_conta_2 = duas_contas;

	if (write(fd, &com, sizeof(comando_t)) == -1) {
		perror("Erro a escrever no ficheiro indicado.\n");
    	exit(1);
	}
	if (close(fd) == -1) {
		perror("Erro a fechar o ficheiro indicado.\n");
    	exit(1);
	}

	pthread_mutex_unlock(&mutex_esc);
	/*Incrementa o semaforo de leitura (Permite que uma tarefa leia do pipe)*/
	sem_post(&sem_ler);
}