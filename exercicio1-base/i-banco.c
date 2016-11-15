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

		int numargs = readLineArguments(args, MAXARGS + 1, buffer, BUFFER_SIZE);

		if (numargs == 0)
			/* Nenhum argumento; ignora e volta a pedir */
			continue;

		/* EOF (end of file) do stdin ou comando "sair" */
		else if (numargs < 0 ||
			(numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))) {

			puts("i-banco vai terminar.\n--");
			int i, pid, status;
			para_sair = 1;

			/*Forca todas as tarefas bloqueadas a avancar e terminarem-se*/
			for (i = 0; i < NUM_TRABALHADORAS; i++) {
				sem_post(&sem_ler);
			}
			/*Certifica-se de que todas as tarefas terminaram*/
			for (i = 0; i < NUM_TRABALHADORAS; i++) {
				pthread_join(tid[i], NULL);
			}

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
			if (numargs < 2) {
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
			novaTarefa(LER_SALDO, atoi(args[1]), -1, -1, 0);
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

	/*Decrementa o semaforo de escrita (Indica que o buffer tem menos um espaco livre)*/
	sem_wait(&sem_esc);
	pthread_mutex_lock(&mutex_esc);

	/*Adiciona um comando ao buffer*/
	cmd_buffer[buff_write_idx].operacao = op;
	cmd_buffer[buff_write_idx].idConta_1 = id_1;
	cmd_buffer[buff_write_idx].idConta_2 = id_2;
	cmd_buffer[buff_write_idx].valor = val;
	cmd_buffer[buff_write_idx].com_conta_2 = duas_contas;
	buff_write_idx = (buff_write_idx + 1) % CMD_BUFFER_DIM;

	pthread_mutex_unlock(&mutex_esc);
	/*Incrementa o semaforo de leitura (Permite que uma tarefa leia do buffer)*/
	sem_post(&sem_ler);
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
		comando_t com = cmd_buffer[buff_read_idx];
		buff_read_idx = (buff_read_idx + 1) % CMD_BUFFER_DIM;
		pthread_mutex_unlock(&mutex_ler);

		int continua = 1;
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
		if (contaExiste(conta_1) && continua) {
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
		if (espera && contadorTarefas == 0 && buff_write_idx == buff_read_idx) {
			pthread_cond_signal(&pode_simular);
		}

		/*Imprime erros na introducao da conta*/
		if (!contaExiste(conta_1) || !continua){
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
	FILE* file = fopen("log.txt", "ab");
	if (debitar (comando.idConta_1, comando.valor) < 0)
		fprintf(file, "%s(%d, %d): ERRO\n\n", COMANDO_DEBITAR, comando.idConta_1, comando.valor);
	else
		fprintf(file, "%s(%d, %d): OK\n\n", COMANDO_DEBITAR, comando.idConta_1, comando.valor);
	fclose(file);
}

void tarefaCreditar(comando_t comando) {
	FILE* file = fopen("log.txt", "ab");
	if (creditar (comando.idConta_1, comando.valor) < 0)
		fprintf(file, "%s(%d, %d): Erro\n\n", COMANDO_CREDITAR, comando.idConta_1, comando.valor);
	else
		fprintf(file, "%s(%d, %d): OK\n\n", COMANDO_CREDITAR, comando.idConta_1, comando.valor);
	fclose(file);
}

void tarefaLerSaldo(comando_t comando) {
	FILE* file = fopen("log.txt", "ab");
	int saldo = lerSaldo(comando.idConta_1);
	if (saldo < 0)
		fprintf(file, "%s(%d): Erro.\n\n", COMANDO_LER_SALDO, comando.idConta_1);
	else
		fprintf(file, "%s(%d): O saldo da conta é %d.\n\n", COMANDO_LER_SALDO, comando.idConta_1, saldo);
	fclose(file);
}

void tarefaTransferir(comando_t comando) {
	FILE* file = fopen("log.txt", "ab");
	if (transferir(comando.idConta_1, comando.idConta_2, comando.valor) < 0) {
		fprintf(file, "Erro ao transferir %d da conta %d para a conta %d.\n\n", comando.valor, comando.idConta_1, comando.idConta_2);
	}
	else {
		fprintf(file, "%s(%d, %d, %d): OK\n\n", COMANDO_TRANSFERIR, comando.idConta_1, comando.idConta_2, comando.valor);
	}
	fclose(file);
}