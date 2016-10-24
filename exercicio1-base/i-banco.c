/*
// Projeto SO - exercicio 1, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
*/

#include "commandlinereader.h"
#include "contas.h"
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_SAIR_AGORA "agora"

#define DEBITAR 1
#define CREDITAR 2
#define LER_SALDO 3

#define MAXARGS 3
#define BUFFER_SIZE 100
#define MAXTAREFA 20
#define NUM_TRABALHADORAS 3
#define CMD_BUFFER_DIM  (NUM_TRABALHADORAS * 2)

typedef struct
{
	int operacao;
	int idConta;
	int valor;
} comando_t;

/*Adiciona um novo comando ao buffer*/
void novaTarefa(int op, int id, int val);
/*Funcao executada pelas tarefas*/
void *recebeComandos();
void tarefaDebitar();
void tarefaCreditar();
void tarefaLerSaldo();

char *args[MAXARGS + 1];
char buffer[BUFFER_SIZE];
int indice = 0, buff_write_idx = 0, buff_read_idx = 0, written = 0;
/*Guarda os pids de todos os processos criados*/
pid_t processos[MAXTAREFA];
/*Guarda a pool de tarefas a usar no programa*/
pthread_t tid[NUM_TRABALHADORAS];
/*Guarda os comandos a executar numa tarefa*/
comando_t cmd_buffer[CMD_BUFFER_DIM];
/*Usado como trinco entre as tarefas*/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
/*Usado para coordenar o uso das tarefas*/
sem_t sem_ler, sem_esc;

int main (int argc, char** argv) {

	signal(SIGUSR1, terminarASAP);
	inicializarContas();
	pthread_mutex_init(&mutex, NULL);
	sem_init(&sem_ler, 0, 0);
	sem_init(&sem_esc, 0, 6);

	int i;
	for (i = 0; i < NUM_TRABALHADORAS; i++){
		if (pthread_create(&tid[i], 0, recebeComandos, NULL) == 0) {
			puts("Criada uma nova tarefa!");
		}	
	}

	printf("Bem-vinda/o ao i-banco\n\n");

	while (1) {

		int numargs = readLineArguments(args, MAXARGS + 1, buffer, BUFFER_SIZE);

		if (numargs == 0)
			/* Nenhum argumento; ignora e volta a pedir */
			continue;

		/* EOF (end of file) do stdin ou comando "sair" */
		else if (numargs < 0 ||
			(numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))) {

			puts("i-banco vai terminar.\n--");
			int i, pid, status;

			/*Sair agora - chama kill a todos os processos filho*/
			if (args[1] != NULL && (strcmp(args[1], COMANDO_SAIR_AGORA) == 0)) {
				for (i = 0; i < indice; i++) {
					kill(processos[i], SIGUSR1);
				}
			}

			/*Termina os processos zombie antes de o programa acabar*/
			for (i = 0; i < indice; i++) {
				pid = wait(&status);
				printf("FILHO TERMINADO (PID=%d; ", pid);

				if (status == 0) {
					puts("terminou normalmente)");
				} else {
					puts("terminou abruptamente)");
				}
			}
			puts("--\ni-banco terminou.");
		}
			
		/* Debitar */
		else if (strcmp(args[0], COMANDO_DEBITAR) == 0) {

			if (numargs < 3) {
				printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_DEBITAR);
				continue;
			}			
			novaTarefa(DEBITAR, atoi(args[1]), atoi(args[2]));
		}

		/* Creditar */
		else if (strcmp(args[0], COMANDO_CREDITAR) == 0) {
			if (numargs < 2) {
				printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_CREDITAR);
				continue;
			}
			novaTarefa(CREDITAR, atoi(args[1]), atoi(args[2]));
		}

		/* Ler Saldo */
		else if (strcmp(args[0], COMANDO_LER_SALDO) == 0) {
			if (numargs < 2) {
				printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_LER_SALDO);
				continue;
			}
			novaTarefa(LER_SALDO, atoi(args[1]), -1);
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
					processos[indice++] = pid;
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
	pthread_join(tid[0], NULL);
	return 0;
}

void novaTarefa(int op, int id, int val) {

	sem_wait(&sem_esc);
	puts("Pedido recebido!");
	cmd_buffer[buff_write_idx].operacao = op;
	cmd_buffer[buff_write_idx].idConta = id;
	cmd_buffer[buff_write_idx].valor = val;
	if (++buff_write_idx == CMD_BUFFER_DIM) {
		buff_write_idx = 0;
	}
	sem_post(&sem_ler);
}

void *recebeComandos() {

	while (1) {
		sem_wait(&sem_ler);
		pthread_mutex_lock(&mutex);
		puts("Tarefa desbloqueada!");

		switch (cmd_buffer[buff_read_idx].operacao) {
			case DEBITAR:
				tarefaDebitar();
				break;

			case CREDITAR:
				tarefaCreditar();
				break;

			case LER_SALDO:
				tarefaLerSaldo();
				break;
		}
		if (++buff_read_idx == CMD_BUFFER_DIM) {
			buff_read_idx = 0;
		}

		pthread_mutex_unlock(&mutex);
		sem_post(&sem_esc);
	}
}

void tarefaDebitar() {
	comando_t com = cmd_buffer[buff_read_idx];
	
	if (debitar (com.idConta, com.valor) < 0)
		printf("%s(%d, %d): ERRO\n\n", COMANDO_DEBITAR, com.idConta, com.valor);
	else
		printf("%s(%d, %d): OK\n\n", COMANDO_DEBITAR, com.idConta, com.valor);
}

void tarefaCreditar() {
	comando_t com = cmd_buffer[buff_read_idx];

	if (creditar (com.idConta, com.valor) < 0)
		printf("%s(%d, %d): Erro\n\n", COMANDO_CREDITAR, com.idConta, com.valor);
	else
		printf("%s(%d, %d): OK\n\n", COMANDO_CREDITAR, com.idConta, com.valor);
}

void tarefaLerSaldo() {
	comando_t com = cmd_buffer[buff_read_idx];

	int saldo = lerSaldo(com.idConta);
	if (saldo < 0)
		printf("%s(%d): Erro.\n\n", COMANDO_LER_SALDO, com.idConta);
	else
		printf("%s(%d): O saldo da conta é %d.\n\n", COMANDO_LER_SALDO, com.idConta, saldo);
}