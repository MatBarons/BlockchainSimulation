#include "../Headers/common_ipcs.h"


/*
  SEMAPHORES
*/

/* 
  0 to syncro
  1 for users
  2 for nodes
  3 for ledger
  4 for msgQ
  5 for config
*/

void semLock(int sem_ID,int index){
  struct sembuf my_sem;
  my_sem.sem_num = index;
  my_sem.sem_flg = 0;
  my_sem.sem_op = -1;
  semop(sem_ID,&my_sem,1);
}

void semRelease(int sem_ID,int index){
  struct sembuf my_sem;
  my_sem.sem_num = index;
  my_sem.sem_flg = 0;
  my_sem.sem_op = 1;
  semop(sem_ID,&my_sem,1);
}

void semSet_disp(int sem_ID,int index){
  union semun arg;
  arg.val=1;
  semctl(sem_ID,index,SETVAL,arg);
}

void semSet_inUse(int sem_ID,int index){
  union semun arg;
  arg.val=0;
  semctl(sem_ID,index,SETVAL,arg);
}


/*
  SHARED MEMORY
*/


void destroySHM(int id){
  int k;
  if((k = shmctl(id,IPC_RMID,NULL)) < 0)
    perror("shmctl failed - error deleting SHM");
}

int generateSHM(key_t key, int size){
  int shm_id;
  if((shm_id = shmget(key,size,IPC_CREAT|0666)) == -1){
    perror("shmget failed - error in SHM creation");
    destroySHM(shm_id);
    exit(EXIT_FAILURE);
  }
  return shm_id;
}

shm_id createIPCs(const char* source,int users,int nodes){
  key_t key_book, key_users, key_nodes,key_msgQ;
  shm_id all_id;
  printf("Possiamo farcela\n");
  key_book = ftok(source,1);
  key_users =  ftok(source,5);
  key_nodes = ftok(source,25);
  key_msgQ = ftok(source,125);
  printf("Ancora un passo\n");

  /*----- Shared Memory MasterLib / Users / Nodes -----*/
  all_id.book_id = generateSHM(key_book,sizeof(Ledger));
  all_id.users_id = generateSHM(key_users,users * sizeof(User));
  all_id.nodes_id = generateSHM(key_nodes,nodes * sizeof(Node) *2);
  

  /*----- Creates the array of semaphores and sets their value ----- */
  


  /*----- Creates the message queue -----*/
  all_id.msgQ_id = msgget(key_msgQ, 0666 | IPC_CREAT);
  if(all_id.msgQ_id == -1){
    perror("msgget failed -- message queue creation error");
    exit(EXIT_FAILURE);
  }
  return all_id;
}

void freeMemory(shm_id ids_free){
  if(msgctl(ids_free.msgQ_id, IPC_RMID, NULL) == -1) /* delete the queue */
    perror("msgctl failed -- queue can't be deleted");
  
  semctl(ids_free.sem_id,0,IPC_RMID);
  destroySHM(ids_free.book_id);
  destroySHM(ids_free.users_id);
  destroySHM(ids_free.nodes_id);
}

void* attachSHM(int shm_id){
  void* k;
  k = (void*) shmat(shm_id,NULL,0);
  if(k < (void*)(0))
    perror("attach shm -- failed shmat");
  return k; 
}


/* choosing a random node is useful for all processes */

int chooseRandNode(int num){
  int c;
  c= rand()%num;
  return c;
}

