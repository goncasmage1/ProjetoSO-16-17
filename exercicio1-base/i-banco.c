/*
// Projeto SO - exercicio 1, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
*/

#include "contas.h"
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define gettid() syscall(SYS_gettid)

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
#define SIMULAR 5
#define SAIR 6
#define SAIR_AGORA 7

#define NUM_TRABALHADORAS 3
#define CMD_BUFFER_DIM  (NUM_TRABALHADORAS * 2)
#define LOGBUFSIZE 100
#define PERM 0777

/*struct que representa um comando introduzido pelo utilizador*/
typedef struct
{
	int operacao;
	int idConta_1;
	int idConta_2;
	int valor;
	/*Indica se precisamos de considerar o valor da conta 2*/
	int com_conta_2;
	long process_id;
} comando_t;

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

/*********************************************************************
*	tarefaSimular(comando_t comando):
*
*	Descricao : Funcao chamada pela tarefa ao executar o comando "sair"
*	Parametros: comando -	Informacoes sobre o comando a processar
*	Returns: 	void
**********************************************************************/
void tarefaSimular(comando_t comando);

/*********************************************************************
*	tarefaSair():
*
*	Descricao : Funcao chamada pela tarefa ao executar o comando "sair"
*	Returns: 	void
**********************************************************************/
void tarefaSair();

/**Variaveis globais*/

/*Nome do pipe a criar*/
const char *ficheiro = "i-banco-pipe";
const char* log_file = "log.txt";
int indice = 0, buff_write_idx = 0, buff_read_idx = 0, fd_ban;
/*"Booleans"*/
int para_sair = 0, sair_agora = 0, contadorTarefas = 0, espera = 0;
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

	unlink(ficheiro);
	unlink(log_file);

	if (mkfifo(ficheiro, PERM) == -1) {
		perror("Erro a criar o pipe!");
		exit(0);
	}
	fd_ban = open(ficheiro, O_RDONLY);
	if (fd_ban == -1) {
		perror("Erro a abrir o ficheiro indicado.\n");
    	exit(1);
	}

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

	while (!para_sair) {
		;
	}

	puts("i-banco vai terminar.\n--");

	/*Forca todas as tarefas bloqueadas a avancar e terminarem-se*/
	for (i = 0; i < NUM_TRABALHADORAS; i++) {
		sem_post(&sem_ler);
	}
	
	/*Certifica-se de que todas as tarefas terminaram*/
	for (i = 0; i < NUM_TRABALHADORAS; i++) {
		pthread_join(tid[i], NULL);
	}

	int pid, status;
	if (sair_agora) {
		/*Sair agora - chama kill a todos os processos filho*/
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
	return 0;
}

void *recebeComandos() {

	while (!para_sair) {
		/*Retorna se o utilizador introduziu o comando sair*/
		if (para_sair) {
			return NULL;
		}

		//pthread_mutex_lock(&mutex_ler);
		/*Le o comando do pipe*/
		int n, continua = 1;
		comando_t com;

		while ((n = read(fd_ban, &com, sizeof(comando_t))) <= 0) ;

		//pthread_mutex_unlock(&mutex_ler);

		/*Troca os indices das contas se o comando for transferir
		e se a primeira conta tiver um indice maior que a primeira*/
		if (com.operacao == SAIR) {
			tarefaSair();
			continua = 0;
		}
		if (com.operacao == SIMULAR && continua) {
			tarefaSimular(com);
			continua = 0;
		}
		if (continua) {
			int conta_1, conta_2;
			if (com.com_conta_2) {
				conta_1 = (com.idConta_1 < com.idConta_2) ? (com.idConta_1 - 1) : (com.idConta_2 - 1);
				conta_2 = (com.idConta_1 < com.idConta_2) ? (com.idConta_2 - 1) : (com.idConta_1 - 1);

				/*Se o indice for o mesmo*/
				if (conta_1 == conta_2) {
					continua = 0;
				}
			}
			else {
				conta_1 = (com.idConta_1 - 1);
				conta_2 = (com.idConta_2 - 1);
			}
		}

		contadorTarefas++;
		/*Verifica se as contas especificadas nao estao a ser acedidas
		e se sao validas*/
		if (contaExiste(conta_1 + 1) && continua) {
			sem_wait(&sem_contas[conta_1]);
			pthread_mutex_lock(&mutex_contas[conta_1]);
			printf("Trancado indice %d\n", conta_1);

			if (com.com_conta_2) {
				if (contaExiste(conta_2 + 1)) {
					sem_wait(&sem_contas[conta_2]);
					pthread_mutex_lock(&mutex_contas[conta_2]);
					printf("Trancado indice %d\n", conta_2);
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

				if ((com.com_conta_2) && (contaExiste(conta_2 + 1))) {
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
		if ((!contaExiste(conta_1 + 1) || !continua)){
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
					break;
			}
		}
		/*Incrementa o semaforo de escrita (Indica que foi lido um comando do pipe)*/
		//sem_post(&sem_esc);
	}
	return NULL;
}

void tarefaDebitar(comando_t comando) {
	int fd = open(log_file, O_WRONLY | O_APPEND | O_CREAT, PERM);
	if (fd == -1) {
		perror("Erro a abrir o log.\n");
    	exit(1);
	}
	char buf[LOGBUFSIZE];
	if (!(debitar (comando.idConta_1, comando.valor) < 0)){
		snprintf(buf, LOGBUFSIZE, "%ld: %s(%d, %d): OK\n\n", gettid(), COMANDO_DEBITAR, comando.idConta_1, comando.valor);
		write(fd, buf, sizeof(char) * strlen(buf));
	}
	if (close(fd) == -1) {
		perror("Erro a fechar o log.\n");
    	exit(1);
	}
}

void tarefaCreditar(comando_t comando) {
	int fd = open(log_file, O_WRONLY | O_APPEND | O_CREAT, PERM);
	if (fd == -1) {
		perror("Erro a abrir o log.\n");
    	exit(1);
	}
	char buf[LOGBUFSIZE];
	if (!(creditar (comando.idConta_1, comando.valor) < 0)){
		snprintf(buf, LOGBUFSIZE, "%ld: %s(%d, %d): OK\n\n", gettid(), COMANDO_CREDITAR, comando.idConta_1, comando.valor);
		write(fd, buf, sizeof(char) * strlen(buf));
	}
	if (close(fd) == -1) {
		perror("Erro a fechar o log.\n");
    	exit(1);
	}
}

void tarefaLerSaldo(comando_t comando) {
	int fd = open(log_file, O_WRONLY | O_APPEND | O_CREAT, PERM);
	if (fd == -1) {
		perror("Erro a abrir o log.\n");
    	exit(1);
	}
	char buf[LOGBUFSIZE];
	int saldo = lerSaldo(comando.idConta_1);
	if (!(saldo < 0)){
		snprintf(buf, LOGBUFSIZE, "%ld: %s(%d): O saldo da conta é %d.\n\n", gettid(), COMANDO_LER_SALDO, comando.idConta_1, saldo);
		write(fd, buf, sizeof(char) * strlen(buf));
	}
	if (close(fd) == -1) {
		perror("Erro a fechar o log.\n");
    	exit(1);
	}
}

void tarefaTransferir(comando_t comando) {
	int fd = open(log_file, O_WRONLY | O_APPEND | O_CREAT, PERM);
	if (fd == -1) {
		perror("Erro a abrir o log.\n");
    	exit(1);
	}
	char buf[LOGBUFSIZE];
	if (!(transferir(comando.idConta_1, comando.idConta_2, comando.valor) < 0)){
		snprintf(buf, LOGBUFSIZE, "%ld: %s(%d, %d, %d): OK\n\n", gettid(), COMANDO_TRANSFERIR, comando.idConta_1, comando.idConta_2, comando.valor);
		write(fd, buf, sizeof(char) * strlen(buf));
	}
	if (close(fd) == -1) {
		perror("Erro a fechar o log.\n");
    	exit(1);
	}
}

void tarefaSimular(comando_t comando) {
	int anos = comando.idConta_1;

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
}

void tarefaSair() {
	para_sair = 1;
}

void tarefaSairAgora() {
	para_sair = 1;
	sair_agora = 1;
}

