#include "contas.h"
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define atrasar() sleep(ATRASO)
			 
int contasSaldos[NUM_CONTAS];
int terminarAgora;

int contaExiste(int idConta) {
	return (idConta > 0 && idConta <= NUM_CONTAS);
}

void inicializarContas() {
	int i;
	for (i = 0 ; i < NUM_CONTAS; i++)
		contasSaldos[i] = 0;
}

int debitar(int idConta, int valor) {
	atrasar();
	if (!contaExiste(idConta) || contasSaldos[idConta - 1] < valor || valor <= 0)
		return -1;
	atrasar();
	contasSaldos[idConta - 1] -= valor;
	return 0;
}

int creditar(int idConta, int valor) {
	atrasar();
	if (!contaExiste(idConta) || valor <= 0)
		return -1;
	contasSaldos[idConta - 1] += valor;
	return 0;
}

int lerSaldo(int idConta) {
	atrasar();
	if (!contaExiste(idConta))
		return -1;
	return contasSaldos[idConta - 1];
}

int transferir(int idConta_1, int idConta_2, int valor) {
	atrasar();
	if (contaExiste(idConta_1) &&
		contaExiste(idConta_2) &&
		contasSaldos[idConta_1] >= valor) {

		contasSaldos[idConta_1] -= valor;
		contasSaldos[idConta_2] += valor;
		return 0;
	}
	return -1;
}

void simular(int numAnos) {
	int novosSaldos[NUM_CONTAS], i, j, k;

	/*Copia os saldos das contas para um novo vetor*/
	for (k = 1; k <= NUM_CONTAS; k++)
		novosSaldos[k-1] = lerSaldo(k);

	/*Percorre os anos indicados*/
	for (i = 0; i < numAnos; i++) {
		printf("SIMULACAO: Ano %d\n=================\n", i + 1);

		/*Percorre todas as contas*/
		for (j = 0; j < NUM_CONTAS; j++) {
			printf("Conta %d, Saldo %d\n", j + 1, novosSaldos[j]);
			novosSaldos[j] += (novosSaldos[j] * TAXAJURO - CUSTOMANUTENCAO) - 0.5;
			if (novosSaldos[j] < 0) {
				novosSaldos[j] = 0;
			}
		}
		if (terminarAgora) {
			puts("Simulacao terminada por signal");
			exit(EXIT_SUCCESS);
		}
	}
	puts("\n");
}

void terminarASAP() {
	terminarAgora = 1;
}


