/*
// Projeto SO - exercicio 1, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
*/

#include "commandlinereader.h"
#include "contas.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"

#define MAXARGS 3
#define BUFFER_SIZE 100


int main (int argc, char** argv) {

    char *args[MAXARGS + 1];
    char buffer[BUFFER_SIZE];

    inicializarContas();

    printf("Bem-vinda/o ao i-banco\n\n");
      
    while (1) {
        int numargs;
    
        numargs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE);

        /* EOF (end of file) do stdin ou comando "sair" */
        if (numargs < 0 ||
	        (numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))) {
            
            /* POR COMPLETAR */

            printf("Comando nao implementado\n");            
            
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
	           printf("%s(%d, %d): OK\n\n", COMANDO_DEBITAR, idConta, valor);
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
		      /* POR COMPLETAR */
		      printf("Comando nao implementado\n");
		    }

		    else {
		      printf("Comando desconhecido. Tente de novo.\n");
		    }
	} 
}

