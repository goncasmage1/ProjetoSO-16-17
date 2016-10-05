#include "contas.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define atrasar() sleep(ATRASO)
			 
int contasSaldos[NUM_CONTAS];


int contaExiste(int idConta) {
  return (idConta > 0 && idConta <= NUM_CONTAS);
}

void inicializarContas() {
  int i;
  for (i=0; i<NUM_CONTAS; i++)
	contasSaldos[i] = 0;
}

int debitar(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta))
	return -1;
  if (contasSaldos[idConta - 1] < valor)
	return -1;
  atrasar();
  contasSaldos[idConta - 1] -= valor;
  return 0;
}

int creditar(int idConta, int valor) {
	atrasar();
	if (!contaExiste(idConta))
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


void simular(int numAnos) {
	int novosSaldos[NUM_CONTAS], i, j, k;

	//Copia os saldos das contas para um novo vetor
	for (k = 0; k < NUM_CONTAS; k++)
		novosSaldos[k] = contasSaldos[k];

	//Percorre os anos indicados
	for (i = 0; i < numAnos; i++) {
		printf("SIMULACAO: Ano %d\n=================\n", numAnos );

		//Percorre todas as contas
		for (j = 0; j < NUM_CONTAS; j++) {
			printf("Conta %d, Saldo %d \n ", j + 1, novosSaldos[j]);
			novosSaldos[j] += novosSaldos[j] * TAXAJURO - CUSTOMANUTENCAO;
		}
		puts("");
	}
}