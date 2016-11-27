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
#include <sys/types.h>
#include <sys/syscall.h>
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

#define READ 0
#define WRITE 1
#define tst(a,b) (mode== READ ? (b) : (a))

#define MAXARGS 4
#define BUFFER_SIZE 100
#define MAXTAREFA 20
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


/**Variaveis globais*/

char *args[MAXARGS + 1];
char buffer[BUFFER_SIZE];
/*Nome do pipe a criar*/
const char *ficheiro;

int indice = 0, espera = 0;
/*Guarda os pids de todos os processos criados*/
pid_t processos[MAXTAREFA];


/**Variaveis globais*/


int main(int argc, char** argv) {

	if (argc == 2) {
		/*
		char* str1 = strdup(argv[1]);
		char* str2 = ".txt";
		if((ficheiro = malloc(strlen(str1)+strlen(str2)+1)) != NULL){
		    ficheiro[0] = '\0';
		    strcat(ficheiro,str1);
		    strcat(ficheiro,str2);
		} else {
		    printf("Concatenacao falhou!\n");
		    exit(0);
		}
		*/
		ficheiro = strdup(argv[1]);
	}
	else {
		printf("Erro: Nao foi especificado nenhum ficheiro como pipe.\n");
    	exit(1);
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

			/*FIX ME*/
			/*
			puts("i-banco vai terminar.\n--");
			int i, pid, status;
			para_sair = 1;
			*/

			/*Forca todas as tarefas bloqueadas a avancar e terminarem-se*/
			/*
			for (i = 0; i < NUM_TRABALHADORAS; i++) {
				sem_post(&sem_ler);
			}
			*/
			/*Certifica-se de que todas as tarefas terminaram*/
			/*
			for (i = 0; i < NUM_TRABALHADORAS; i++) {
				pthread_join(tid[i], NULL);
			}
			*/
			/*Sair agora - chama kill a todos os processos filho*/
			/*
			if (args[1] != NULL && (strcmp(args[1], COMANDO_SAIR_AGORA) == 0)) {
				for (i = 0; i < indice; i++) {
					kill(processos[i], SIGUSR1);
				}
			}
			*/
			/*Termina os processos zombie antes de o programa acabar*/
			/*
			for (i = 0; i < indice; i++) {
				pid = wait(&status);
				printf("FILHO TERMINADO (PID=%d; ", pid);

				if (status == 0) {
					puts("terminou normalmente)");
				} else {
					puts("terminou abruptamente)");
				}
			}
			*/
			/*FIX ME*/
			puts("\ni-banco-terminal terminou.\n");
			return 0;
		}

		/* Sair terminal */
		else if (strcmp(args[0], COMANDO_SAIR_TERMINAL) == 0) {

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
				/*FIX ME*/
				/*
				while (contadorTarefas > 0 || buff_write_idx != buff_read_idx) {
					pthread_cond_wait(&pode_simular, &mutex_cond);
				}
				*/
				int p[2];
				if (pipe(p) < 0) {
					return NULL;
				}

				/*Cria um processo filho*/
				pid_t pid = fork();

				/*O processo filho faz a simulacao*/
				if (pid == 0) {
					
					close(tst(p[WRITE], p[READ]));
					close(tst(0, 1));
					dup(tst(p[READ], p[WRITE]));
					close(tst(p[READ], p[WRITE]));
					
					char nome[] = "i-banco-sim-%ld.txt";
					char pidlong[100];
					long pidnumber = getpid();
					sprintf(pidlong, nome, pidnumber);

					printf("%s\n", pidlong);

					puts("Comecou a simular");
					simular(anos, pidlong);
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
	//sem_wait(&sem_esc);
	//pthread_mutex_lock(&mutex_esc);

	int fd;
	fd = open(ficheiro, O_WRONLY, S_IWUSR);
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

	//pthread_mutex_unlock(&mutex_esc);
	/*Incrementa o semaforo de leitura (Permite que uma tarefa leia do pipe)*/
	//sem_post(&sem_ler);
}