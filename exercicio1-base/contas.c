#include "contas.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#define atrasar() sleep(ATRASO)

int deveTerminar = 0;
		     
int contasSaldos[NUM_CONTAS];
pthread_mutex_t account_ctrl[NUM_CONTAS];

void lock_account(int account) {
  /* confirmar que antes foi verificado que se trata de uma conta válida */
  if ((errno = pthread_mutex_lock(&account_ctrl[account-1])) != 0) {
    perror("Error in pthread_mutex_lock()");
    exit(EXIT_FAILURE);
  }
}

void unlock_account(int account) {
  /* confirmar que antes foi verificado que se trata de uma conta válida */
  if ((errno = pthread_mutex_unlock(&account_ctrl[account-1])) != 0) {
    perror("Error in pthread_mutex_unlock()");
    exit(EXIT_FAILURE);
  }
}



int contaExiste(int idConta) {
  return (idConta > 0 && idConta <= NUM_CONTAS);
}

void destroy_mutex_contas(){
  for(int i = 0; i < NUM_CONTAS; i++)
    pthread_mutex_destroy(&account_ctrl[i]);
}

void inicializarContas() {
  int i;
  for (i=0; i<NUM_CONTAS; i++){
    contasSaldos[i] = 0;
    if ((errno = pthread_mutex_init(&account_ctrl[i], NULL)) != 0) {
      perror("Error in pthread_mutex_init()");
      exit(EXIT_FAILURE);
    }
  }
}

int debitarSemMutex(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta))
    return -1;

  if (contasSaldos[idConta - 1] < valor) {
    return -1;
  }
  atrasar();
  contasSaldos[idConta - 1] -= valor;
  
  return 0;
}

int debitar(int idConta, int valor) {
  int i;
  lock_account(idConta);
  i = debitarSemMutex(idConta, valor);
  unlock_account(idConta);
  return i;
}

int creditar(int idConta, int valor){
  int i;
  lock_account(idConta);
  i = creditarSemMutex(idConta, valor);
  unlock_account(idConta);
  return i;
}

int creditarSemMutex(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta))
    return -1;
    
  contasSaldos[idConta - 1] += valor;
  
  return 0;
}

int transferir(int idContaOrigem, int idContaDestino, int valor){
	
  if (!contaExiste(idContaOrigem) || !contaExiste(idContaDestino))
    return -1;
		
  if(idContaOrigem < idContaDestino){
    lock_account(idContaOrigem);
    lock_account(idContaDestino);
  }else if(idContaOrigem > idContaDestino){
    lock_account(idContaDestino);
    lock_account(idContaOrigem);
  }else
    return -1;
		
  if(debitarSemMutex(idContaOrigem, valor) < 0){
    unlock_account(idContaOrigem);
    unlock_account(idContaDestino);
    return -1;
  }
	
  creditarSemMutex(idContaDestino, valor);
	
  unlock_account(idContaOrigem);
  unlock_account(idContaDestino);
  return 0;
}

int lerSaldo(int idConta) {
  int saldo;

  atrasar();
  if (!contaExiste(idConta))
    return -1;

  lock_account(idConta);
  saldo = contasSaldos[idConta - 1];
  unlock_account(idConta);
  return saldo;
}


void trataSignal(int sig) {
  //printf("Processo %d recebeu o signal\n", getpid());
  deveTerminar = 1;
}

void simular(int numAnos) {
  int id, saldo;
  int ano = 0;
  
  for (ano = 0; 
       ano <= numAnos && !deveTerminar;
       ano++) {
    
    printf("SIMULAÇÃO: Ano %d\n=================\n", ano);
    for (id = 1; id<=NUM_CONTAS; id++) {
      if (ano > 0) {
        saldo = lerSaldo(id);
        creditar(id, saldo * TAXAJURO);
        saldo = lerSaldo(id);
        debitar(id, (CUSTOMANUTENCAO > saldo) ? saldo : CUSTOMANUTENCAO);
      }
      saldo = lerSaldo(id);
      /* A funcao printf pode ser interrompida pelo SIGUSR1,
	 retornando -1 e colocando errno=EINTR.
	 Para lidar com esse caso, repetimos o printf sempre
	 que essa situacao se verifique. 
	 Nota: este aspeto e' de nivel muito avancado, logo
	 so' foi exigido a projetos com nota maxima
      */
      while (printf("Conta %5d,\t Saldo %10d\n", id, saldo) < 0) {
        if (errno == EINTR)
          continue;
        else
          break;
      }
    }
  }
  
  if (deveTerminar)
    printf("Simulacao terminada por signal\n");
    //printf("Simulacao terminada por ordem do processo pai\n");
  
}
