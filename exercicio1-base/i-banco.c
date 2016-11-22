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

#define DEBITAR 1
#define CREDITAR 2
#define LER_SALDO 3
#define TRANSFERIR 4

#define MAXARGS 4
#define BUFFER_SIZE 100
#define MAXTAREFA 20
#define NUM_TRABALHADORAS 3
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

/*********************************************************************
*	novaTarefa(int op, int id, int val):
*comando
*	Descricao:	Cria um novo comando_t baseado nas informacoes passadas
*				e insere-o no buffer
*	Parametros: op -	O numero da operacao associado a uma das tres
*						operacoes (debitar, creditar e lerSaldo)
*				id -	O ID da conta introduzida
*				val -	O valor introduzido para ser usado na operacao
*				duas_contas - Indica se o valor da segunda conta
								deve ser considerado
*	Returns: 	void
**********************************************************************/
void novaTarefa(int op, int id_1, int id_2, int val, int duas_contas);
/*********************************************************************
*	recebeComandos():
*
*	Descricao:	Funcao usada pelas tarefas, recebe comandos do buffer
*	Parametros: Nenhum
*	Returns: 	void
**********************************************************************/
void *recebeComandos();
/*********************************************************************
*	tarefaDebitar(comando_t comando):
*
*	Descricao:	Funcao chamada pela tarefa ao executar o comando "debitar"
*	Parametros: comando -	Informacoes sobre o comando a processar
*	Returns: 	void
**********************************************************************/
void tarefaDebitar(comando_t comando);
/*********************************************************************
*	tarefaCreditar(comando_t comando):
*
*	Descricao : Funcao chamada pela tarefa ao executar o comando "creditar"
*	Parametros: comando -	Informacoes sobre o comando a processar
*	Returns: 	void
**********************************************************************/
void tarefaCreditar(comando_t comando);
/*********************************************************************
*	tarefaLerSaldo(comando_t comando):
*
*	Descricao : Funcao chamada pela tarefa ao executar o comando "lerSaldo"
*	Parametros: comando -	Informacoes sobre o comando a processar
*	Returns: 	void
**********************************************************************/
void tarefaLerSaldo(comando_t comando);

/*********************************************************************
*	tarefaTransferir(comando_t comando):
*
*	Descricao : Funcao chamada pela tarefa ao executar o comando "transferir"
*	Parametros: comando -	Informacoes sobre o comando a processar
*	Returns: 	void
**********************************************************************/
void tarefaTransferir(comando_t comando);

/**Variaveis globais*/

char *args[MAXARGS + 1];
char buffer[BUFFER_SIZE];
int indice = 0, buff_write_idx = 0, buff_read_idx = 0;
/*"Booleans"*/
int para_sair = 0, contadorTarefas = 0, espera = 0;
/*Guarda os pids de todos os processos criados*/
pid_t processos[MAXTAREFA];
/*Guarda a pool de tarefas a usar no programa*/
pthread_t tid[NUM_TRABALHADORAS];
/*Condicoes para sincronizar as tarefas e as simulacoes*/
pthread_cond_t pode_simular;
/*Vetor de comandos que guarda os comandos a executar numa tarefa*/
comando_t cmd_buffer[CMD_BUFFER_DIM];
/*Declaracao de mutexes (trincos)
	1 trinco de escrita
	1 trinco de leitura
	1 trinco de condicao
	N trincos para cada uma das contas*/
pthread_mutex_t mutex_esc, mutex_ler, mutex_cond, mutex_contas[NUM_CONTAS];
/*Declaracao de semaforos
	1 semaforo de leitura
	1 semaforo de escritura
	N semaforos para cada uma das contas*/
sem_t sem_ler, sem_esc, sem_contas[NUM_CONTAS];

/**Variaveis globais*/

int main (int argc, char** argv) {

	/*Inicializacoes*/
	signal(SIGUSR1, terminarASAP);
	inicializarContas();
	if (pthread_mutex_init(&mutex_ler, NULL) != 0) {
		printf("Erro a iniciar trinco!");
		return 0;
	}
	if (pthread_mutex_init(&mutex_esc, NULL) != 0) {
		printf("Erro a iniciar trinco!");
		return 0;
	}
	if (pthread_mutex_init(&mutex_cond, NULL) != 0) {
		printf("Erro a iniciar trinco!");
		return 0;
	}
	if (pthread_cond_init(&pode_simular, NULL) != 0) {
		printf("Erro a iniciar trinco!");
		return 0;
	}
	if (sem_init(&sem_ler, 0, 0) == -1) {
		printf("Erro a iniciar semaforo!");
		return 0;
	}
	if (sem_init(&sem_esc, 0, CMD_BUFFER_DIM) == -1) {
		printf("Erro a iniciar semaforo!");
		return 0;
	}

	int i;
	for (i = 0; i < NUM_CONTAS; i++) {
		if (sem_init(&sem_contas[i], 0, 1) == -1) {
			printf("Erro a iniciar semaforo!");
			return 0;
		}
		if (pthread_mutex_init(&mutex_contas[i], NULL) != 0) {
			printf("Erro a iniciar trinco!");
			return 0;
		}
	}
	for (i = 0; i < NUM_TRABALHADORAS; i++){
		if (pthread_create(&tid[i], 0, recebeComandos, NULL) != 0) {
			puts("Erro a criar tarefa!");
		}
	}
	/*Inicializacoes*/

	printf("Bem-vinda/o ao i-banco\n\n");

	while (1) {
		;
	}
	return 0;
}

void *recebeComandos() {

	while (!para_sair) {
		/*Decrementa o semaforo de leitura (Espera para que possa ler do buffer)*/
		sem_wait(&sem_ler);
		/*Retorna se o utilizador introduziu o comando sair*/
		if (para_sair) {
			return NULL;
		}

		pthread_mutex_lock(&mutex_ler);
		/*Le o comando do buffer*/
		int continua = 1;
		int fd;
		if (fd = open(ficheiro, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1) {
			perror("Erro a abrir o ficheiro indicado.\n");
	    	exit(1);
		}
		comando_t com;
		if (read(fd, &com, sizeof(struct comando_t)) == -1) {
			perror("Erro a ler o ficheiro indicado.\n");
    		exit(1);
		}
		if (close(fd) == -1) {
			perror("Erro a fechar o ficheiro indicado.\n");
	    	exit(1);
		}
		pthread_mutex_unlock(&mutex_ler);

		/*Troca os indices das contas se o comando for transferir
		e se a primeira conta tiver um indice maior que a primeira*/
		int conta_1, conta_2;
		if (com.com_conta_2) {
			conta_1 = (com.idConta_1 < com.idConta_2) ? com.idConta_1 : com.idConta_2;
			conta_2 = (com.idConta_1 < com.idConta_2) ? com.idConta_2 : com.idConta_1;

			/*Se o indice for o mesmo*/
			if (conta_1 == conta_2) {
				continua = 0;
			}
		}
		else {
			conta_1 = com.idConta_1;
			conta_2 = com.idConta_2;
		}

		contadorTarefas++;
		/*Verifica se as contas especificadas nao estao a ser acedidas
		e se sao validas*/
		if (contaExiste(conta_1) && continua && leu_fich) {
			sem_wait(&sem_contas[conta_1]);
			pthread_mutex_lock(&mutex_contas[conta_1]);

			if (com.com_conta_2) {
				if (contaExiste(conta_2)) {
					sem_wait(&sem_contas[conta_2]);
					pthread_mutex_lock(&mutex_contas[conta_2]);
				}
				/*Imprime erro na introducao da conta*/
				else {
					printf("Erro ao transferir %d da conta %d para a conta %d.\n\n", com.valor, com.idConta_1, com.idConta_2);
					continua = 0;
				}
			}
			/*Se nao houver erro na introducao da segunda conta*/
			if (continua) {
				switch (com.operacao) {
					case DEBITAR:
						tarefaDebitar(com);
						break;

					case CREDITAR:
						tarefaCreditar(com);
						break;

					case LER_SALDO:
						tarefaLerSaldo(com);
						break;

					case TRANSFERIR:
						if ((com.com_conta_2) && (contaExiste(com.idConta_2))) {
							tarefaTransferir(com);
						}
						break;
				}

				if ((com.com_conta_2) && (contaExiste(conta_2))) {
					pthread_mutex_unlock(&mutex_contas[conta_2]);
					sem_post(&sem_contas[conta_2]);
				}

				pthread_mutex_unlock(&mutex_contas[conta_1]);
				sem_post(&sem_contas[conta_1]);
			}
		}
		contadorTarefas--;
		/*Verifica se ainda ha tarefas a executar operacoes,
		se nao houverem liberta o trinco condicional do comando simular*/
		if (leu_fich && espera && contadorTarefas == 0 && buff_write_idx == buff_read_idx) {
			pthread_cond_signal(&pode_simular);
		}

		/*Imprime erros na introducao da conta*/
		if ((!contaExiste(conta_1) || !continua)){
			switch(com.operacao) {
				case DEBITAR:
					printf("%s(%d, %d): ERRO\n\n", COMANDO_DEBITAR, com.idConta_1, com.valor);
					break;

				case CREDITAR:
					printf("%s(%d, %d): Erro\n\n", COMANDO_CREDITAR, com.idConta_1, com.valor);
					break;

				case LER_SALDO:
					printf("%s(%d): Erro.\n\n", COMANDO_LER_SALDO, com.idConta_1);
					break;

				case TRANSFERIR:
					printf("Erro ao transferir %d da conta %d para a conta %d.\n\n", com.valor, com.idConta_1, com.idConta_2);
					break;

				default:
					puts("Erro generico");
					break;
			}
		}
		/*Incrementa o semaforo de escrita (Indica que o buffer tem mais um espaco livre)*/
		sem_post(&sem_esc);
	}
	return NULL;
}

void tarefaDebitar(comando_t comando) {
	FILE* file = fopen("log.txt", "a");
	if (debitar (comando.idConta_1, comando.valor) < 0)
		//printf("%s(%d, %d): ERRO\n\n", COMANDO_DEBITAR, comando.idConta_1, comando.valor);
	else
		fprintf(file, "%d: %s\n", gettid(), COMANDO_DEBITAR);
		//printf("%s(%d, %d): OK\n\n", COMANDO_DEBITAR, comando.idConta_1, comando.valor);
	fclose(file);
}

void tarefaCreditar(comando_t comando) {
	FILE* file = fopen("log.txt", "a");
	if (creditar (comando.idConta_1, comando.valor) < 0)
		//printf("%s(%d, %d): Erro\n\n", COMANDO_CREDITAR, comando.idConta_1, comando.valor);
	else
		fprintf(file, "%d: %s\n", gettid(), COMANDO_CREDITAR);
		//printf("%s(%d, %d): OK\n\n", COMANDO_CREDITAR, comando.idConta_1, comando.valor);
	fclose(file);
}

void tarefaLerSaldo(comando_t comando) {
	FILE* file = fopen("log.txt", "a");
	int saldo = lerSaldo(comando.idConta_1);
	if (saldo < 0)
		//printf("%s(%d): Erro.\n\n", COMANDO_LER_SALDO, comando.idConta_1);
	else
		fprintf(file, "%d: %s\n", gettid(), COMANDO_LER_SALDO);
		//printf("%s(%d): O saldo da conta é %d.\n\n", COMANDO_LER_SALDO, comando.idConta_1, saldo);
	fclose(file);
}

void tarefaTransferir(comando_t comando) {
	FILE* file = fopen("log.txt", "a");
	if (transferir(comando.idConta_1, comando.idConta_2, comando.valor) < 0)
		//printf("Erro ao transferir %d da conta %d para a conta %d.\n\n", comando.valor, comando.idConta_1, comando.idConta_2);
	else
		fprintf(file, "%d: %s\n", gettid(), COMANDO_TRANSFERIR);
		//printf("%s(%d, %d, %d): OK\n\n", COMANDO_TRANSFERIR, comando.idConta_1, comando.idConta_2, comando.valor);
	fclose(file);
}

uint64_t gettid() {
    return (uint64_t)pthread_self();
}