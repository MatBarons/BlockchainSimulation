#include "../Headers/master.h"

Ledger* ledger;
User* all_users;
Node* all_nodes;
Config* all_conf;
shm_id ids;
pid_t printProcess;


char term_reason[100];

int insertConfig(FILE *file){
  int value;
  char config_name[100];
  int error_flag = 1;
  while(fscanf(file,"%s %d",config_name,&value) == 2){
    config_name[sizeof(config_name)/sizeof(char)] = '\0';
    if(config_name[0] == '#'){
      /* this is a comment identifier so we skip the line*/
    }else if(strcmp(config_name, "SO_USERS_NUM") == 0){
      all_conf->SO_USERS_NUM = value;
    }else if(strcmp(config_name, "SO_NODES_NUM") == 0){
      all_conf->SO_NODES_NUM = value;
    }else if(strcmp(config_name, "SO_INIT_BUDGET") == 0){
      all_conf->SO_INIT_BUDGET = value;
    }else if(strcmp(config_name, "SO_REWARD") == 0){
      all_conf->SO_REWARD = value;
    }else if(strcmp(config_name, "SO_MIN_TRANS_GEN_NSEC") == 0){
      all_conf->SO_MIN_TRANS_GEN_NSEC = value;
    }else if(strcmp(config_name, "SO_MAX_TRANS_GEN_NSEC") == 0){
      all_conf->SO_MAX_TRANS_GEN_NSEC = value;
    }else if(strcmp(config_name, "SO_RETRY") == 0){
      all_conf->SO_RETRY = value;
    }else if(strcmp(config_name, "SO_TP_SIZE") == 0){
      all_conf->SO_TP_SIZE = value;
    }else if(strcmp(config_name, "SO_MIN_TRANS_PROC_NSEC") == 0){
      all_conf->SO_MIN_TRANS_PROC_NSEC = value;
    }else if(strcmp(config_name, "SO_MAX_TRANS_PROC_NSEC") == 0){
      all_conf->SO_MAX_TRANS_PROC_NSEC = value;
    }else if(strcmp(config_name, "SO_SIM_SEC") == 0){
      all_conf->SO_SIM_SEC = value;
    }else if(strcmp(config_name, "SO_NUM_FRIENDS") == 0){
      all_conf->SO_NUM_FRIENDS = value;
    }else if(strcmp(config_name, "SO_HOPS") == 0){
      all_conf->SO_HOPS = value;
    }else{
      error_flag = -1; /*something went wrong */
    }
  }
  return error_flag;
}

void createUsers(){
  int i;
  pid_t pr_id;
  char* args_u[8];
  char bookID[12];
  char nodesID[12];
  char usersID[12];
  char msgID[12];
  char semID[12];
  char configID[12];

  sprintf(bookID,"%d",ids.book_id);
  sprintf(nodesID,"%d",ids.nodes_id);
  sprintf(usersID,"%d",ids.users_id);
  sprintf(msgID,"%d",ids.msgQ_id);
  sprintf(semID,"%d",ids.sem_id);
  sprintf(configID,"%d",ids.config_id);
  
  args_u[0] = "user";
  args_u[1] = bookID;
  args_u[2] = nodesID;
  args_u[3] = usersID;
  args_u[4] = msgID;
  args_u[5] = semID;
  args_u[6] = configID;
  args_u[7] = NULL;
  for(i=0;i<all_conf->SO_USERS_NUM;i++){
    
    switch (pr_id=fork()){
      case -1:
        raise(SIGINT);
      break;
      case 0:
        all_users[i].pid = getpid();
        all_users[i].budget = all_conf->SO_INIT_BUDGET;
        all_users[i].dead = 1;
        execv("user",args_u);
      break;
      default:
      break;
    }
  }
}

void createNodes(){
  int i,j;
  pid_t pr_id;
  char* args_n[8];
  char bookID[12];
  char nodesID[12];
  char usersID[12];
  char msgID[12];
  char semID[12];
  char configID[12];

  sprintf(bookID,"%d",ids.book_id);
  sprintf(nodesID,"%d",ids.nodes_id);
  sprintf(usersID,"%d",ids.users_id);
  sprintf(msgID,"%d",ids.msgQ_id);
  sprintf(semID,"%d",ids.sem_id);
  sprintf(configID,"%d",ids.config_id);
  
  args_n[0] = "node";
  args_n[1] = bookID;
  args_n[2] = nodesID;
  args_n[3] = usersID;
  args_n[4] = msgID;
  args_n[5] = semID;
  args_n[6] = configID;
  args_n[7] = NULL;

  for(i=0;i<all_conf->SO_NODES_NUM;i++){
    switch(pr_id=fork()){
      case -1:
        raise(SIGINT);
      break;
      case 0:
        all_nodes[i].pid = getpid();
        all_nodes[i].reward_sum = 0;
        all_nodes[i].n_transQ = 0;
        for(j=0;j<all_conf->SO_NUM_FRIENDS;j++){
          all_nodes[i].friends[j] = all_nodes[chooseRandNode(all_conf->SO_NODES_NUM)].pid;
          if(j+1 == all_conf->SO_NUM_FRIENDS)
            all_nodes[i].friends[j+1] = -1;
        }
        execv("node",args_n);
      break;
      default:
      break;
    }
  }
}

void printTerminationRequirments(){
  int i,cont=0;

  for(i=0;i<all_conf->SO_USERS_NUM;i++){
    printf("utente %d con pid %d, bilancio %d\n",i,all_users[i].pid,all_users[i].budget);
  }
  
  for(i=0;i<all_conf->SO_NODES_NUM;i++){
    printf("Nodo %d, con pid %d, ha bilancio %d e il numero di transazioni ancora in transaction pool è %d \n",i,all_nodes[i].pid,all_nodes[i].reward_sum,all_nodes[i].n_transQ);
  }
  for(i=0;i<all_conf->SO_USERS_NUM;i++){
    if(all_users[i].dead == -1)
      cont++;
  }
  printf("Il numero di utenti morti prematuramente è: %d\n",cont);
  printf("il numero di blocchi presenti nel libro mastro è:%d\n",ledger->last_id+1);
  printf("Ragione della terminazione: %s\n",term_reason);
 
  
}

void printEverySecond(){
  pid_t print_process,pid_user_to_print,pid_node_to_print;
  struct timespec t1,t2;
  int i,max_users=0,max_nodes=0;
  t1.tv_sec = 1;
  t1.tv_nsec = 0;

  switch (print_process = fork())
  {
  case -1:
    raise(SIGINT);
    break;
  case 0: 
    while(1){
      
      printf("\n");
      printf("_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_\n");

      if(all_conf->SO_USERS_NUM >= MAX_PRINT_USERS){
        for(i=0;i<all_conf->SO_USERS_NUM;i++){
          if(all_users[i].budget > max_users && all_users[i].dead !=-1){
            max_users = all_users[i].budget;
            pid_user_to_print = all_users[i].pid;
          }
        }
        printf("Lo user con pid %d ha il bilancio più alto, che e: %d \n",pid_user_to_print,max_users);
      }else{
        for(i=0;i<all_conf->SO_USERS_NUM;i++){
          printf("User con pid %d, ha bilancio %d\n",all_users[i].pid,all_users[i].budget);
        }
      }


      if(all_conf->SO_NODES_NUM >= MAX_PRINT_NODES){
        for(i=0;i<all_conf->SO_NODES_NUM;i++){
          if(all_nodes[i].reward_sum > max_nodes){
            max_nodes = all_nodes[i].reward_sum;
            pid_node_to_print = all_nodes[i].pid;
          }
        }
        printf("Il nodo con pid %d ha il bilancio più alto, che e: %d \n",pid_node_to_print,max_nodes);
      }else{
        for(i=0;i<all_conf->SO_NODES_NUM;i++){
          printf("Node con pid %d, ha bilancio %d\n",all_nodes[i].pid,all_nodes[i].reward_sum);
        }
      }

      printf("_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_\n");
      printf("\n");

      if(nanosleep(&t1,&t2) < 0){
        printf("nanosleep failed in printEverySecond()");
        raise(SIGINT);
      }
    }
  default:
  printProcess = print_process;
    break;
  }
}

void handleSignal(int signal,siginfo_t* siginfo,void* context){
  int i;
  switch (signal){
    /* time for the simulation is over */
    case SIGALRM:
      strcpy(term_reason,"alarm triggered,SO_SIM_SEC have passed");
      for(i=0;i<all_conf->SO_USERS_NUM;i++){
        kill(all_users[i].pid,SIGTERM);
      }
      for(i=0;i<all_conf->SO_NODES_NUM;i++){
        kill(all_nodes[i].pid,SIGTERM);
      }
      kill(printProcess,SIGKILL);
      printTerminationRequirments();
      shmdt(all_nodes);
      shmdt(all_users);
      shmdt(all_conf);
      shmdt(ledger);
      destroySHM(ids.book_id);
      destroySHM(ids.config_id);
      semctl(ids.sem_id,0,IPC_RMID);
      msgctl(ids.msgQ_id,IPC_RMID,NULL);
      destroySHM(ids.nodes_id);
      destroySHM(ids.users_id);
      exit(EXIT_SUCCESS);
    break;
    /* CTRL+C received */
    case SIGINT:
    printf("premuto CTRL+C\n");
      strcpy(term_reason,"CTRL+C received");
      for(i=0;i<all_conf->SO_USERS_NUM;i++){
        kill(all_users[i].pid,SIGINT);
      }
      for(i=0;i<all_conf->SO_NODES_NUM;i++){
        kill(all_nodes[i].pid,SIGINT);
      }
      kill(printProcess,SIGKILL);
      shmdt(all_nodes);
      shmdt(all_users);
      shmdt(all_conf);
      shmdt(ledger);
      destroySHM(ids.book_id);
      destroySHM(ids.config_id);
      destroySHM(ids.msgQ_id);
      destroySHM(ids.nodes_id);
      destroySHM(ids.users_id);
      destroySHM(ids.sem_id);
      exit(EXIT_FAILURE);
    break;
    /* no more space on the REGISTRY */
    case SIGUSR1:
      strcpy(term_reason,"No more space in the ledger");
      for(i=0;i<all_conf->SO_USERS_NUM;i++){
        kill(all_users[i].pid,SIGTERM);
      }
      for(i=0;i<all_conf->SO_NODES_NUM;i++){
        kill(all_nodes[i].pid,SIGTERM);
      }
      kill(printProcess,SIGKILL);
      printTerminationRequirments();
      shmdt(all_nodes);
      shmdt(all_users);
      shmdt(all_conf);
      shmdt(ledger);
      destroySHM(ids.book_id);
      destroySHM(ids.config_id);
      destroySHM(ids.nodes_id);
      destroySHM(ids.users_id);
      semctl(ids.sem_id,0,IPC_RMID);
      msgctl(ids.msgQ_id,IPC_RMID,NULL);
      printf("%s\n",term_reason);
      exit(EXIT_SUCCESS);
      /* no more users alive */
      case SIGUSR2:
        strcpy(term_reason,"No more users alive");
        for(i=0;i<all_conf->SO_USERS_NUM;i++){
        kill(all_users[i].pid,SIGTERM);
      }
      for(i=0;i<all_conf->SO_NODES_NUM;i++){
        kill(all_nodes[i].pid,SIGTERM);
      }
      kill(printProcess,SIGKILL);
      printTerminationRequirments();
      shmdt(all_nodes);
      shmdt(all_users);
      shmdt(all_conf);
      shmdt(ledger);
      destroySHM(ids.book_id);
      destroySHM(ids.config_id);
      destroySHM(ids.nodes_id);
      destroySHM(ids.users_id);
      semctl(ids.sem_id,0,IPC_RMID);
      msgctl(ids.msgQ_id,IPC_RMID,NULL);
      printf("%s\n",term_reason);
      exit(EXIT_SUCCESS);
    default:
    break;
  }
}

int main(int argc, char const *argv[]){
  FILE* fp1;
  struct sigaction sa;
  int i,c;
  Message m;
  pid_t newNode;
  key_t key_book,key_users,key_nodes,key_msgQ;

  char* args_m[8];
  char bookID[12];
  char nodesID[12];
  char usersID[12];
  char msgID[12];
  char semID[12];
  char configID[12];

  args_m[0] = "node";
  args_m[1] = bookID;
  args_m[2] = nodesID;
  args_m[3] = usersID;
  args_m[4] = msgID;
  args_m[5] = semID;
  args_m[6] = configID;
  args_m[7] = NULL;


/* ---------------- CREATION OF SHARED MEMORY  ---------------- */
  c = generateSHM(ftok(argv[2],625),sizeof(Config));
  ids.config_id = c;
  all_conf = (Config*)attachSHM(ids.config_id);
  printf("Inizializzazione parametri\n");
  /* Here the config are sets */
  fp1 = fopen(argv[1],"r");
  if(insertConfig(fp1) == -1){
    printf("Error in opening/reading parameters from the config file");
    exit(EXIT_FAILURE);
  }
  fclose(fp1);

  /* Here the message queue and all the shared memories are created and attached */
  printf("Creazione memoria\n");
  key_book = ftok(argv[2],1);
  key_users =  ftok(argv[2],5);
  key_nodes = ftok(argv[2],25);
  key_msgQ = ftok(argv[2],125);
  ids.book_id = generateSHM(key_book,sizeof(Ledger));
  ids.users_id = generateSHM(key_users,all_conf->SO_USERS_NUM * sizeof(User));
  ids.nodes_id = generateSHM(key_nodes,all_conf->SO_NODES_NUM * sizeof(Node) *2);
  ids.sem_id = semget(IPC_PRIVATE,6,IPC_CREAT | IPC_EXCL |0666);
  ids.msgQ_id = msgget(key_msgQ, 0666 | IPC_CREAT);

  /* setting up semaphores  and some values in the shared memory*/
  for(i=0;i<6;i++)
    semSet_disp(ids.sem_id,i);
  ledger = (Ledger*)attachSHM(ids.book_id);
  ledger->last_id = 0;
  all_nodes = (Node*)attachSHM(ids.nodes_id); 
  all_users = (User*)attachSHM(ids.users_id);
  printf("Memoria creata\n");
  
  /* Here the signals are set */
  sa.sa_sigaction=&handleSignal;
  bzero(&sa.sa_mask,sizeof(sa));
  sa.sa_flags=0;
  sigaction(SIGALRM,&sa,NULL);
  sigaction(SIGUSR1,&sa,NULL);
  sigaction(SIGUSR2,&sa,NULL);
  sigaction(SIGINT,&sa,NULL);
  /* ------------------ CREATES NODES AND USERS ------------------ */
  printf("Creazione nodi\n");
  createNodes();
  printf("Nodi creati\n");
  printf("Creazione users\n");
  createUsers();
  printf("Users creati\n");
  /*------------------- STARTS NODES AND USERS ------------------- */
  printf("Inizio simulazione\n");
  for(i=0;i<all_conf->SO_NODES_NUM;i++){
  semRelease(ids.sem_id,0);
  }
  alarm(all_conf->SO_SIM_SEC);
  for(i=0;i<all_conf->SO_USERS_NUM;i++){
    semRelease(ids.sem_id,1);
  }

  /* ------------------ START SIMULATIONS ------------------ */

  printEverySecond(); 
  while(1){
    if(msgrcv(ids.msgQ_id,&m,sizeof(m),getpid(),IPC_NOWAIT) == -1 && errno != EINTR && errno != ENOMSG){
      perror("msgrcv failed in main master");
      exit(EXIT_FAILURE);
    }else if(errno == ENOMSG){
      continue;
    }else{
      /* creates a new node */
      newNode = fork();
      switch (newNode)
      {
      case 0:
        semLock(ids.sem_id,0);
        execv("node",args_m);
      break;
      default:
        semLock(ids.sem_id,2);
        all_nodes[all_conf->SO_NODES_NUM].pid = newNode;
        for(i=0;i<all_conf->SO_NUM_FRIENDS;i++)
          all_nodes[all_conf->SO_NODES_NUM].friends[i] =all_nodes[chooseRandNode(all_conf->SO_NODES_NUM)].pid;
        semRelease(ids.sem_id,2);
        m.msg_type = newNode;
        semLock(ids.sem_id,4);
        msgsnd(ids.msgQ_id,&m,sizeof(m),IPC_NOWAIT);
        semRelease(ids.sem_id,4);
        semLock(ids.sem_id,5);
        all_conf->SO_NODES_NUM++;
        semRelease(ids.sem_id,5);
        for(i=0;i<all_conf->SO_NODES_NUM && all_nodes[i].pid != newNode ;i++)
          kill(all_nodes[chooseRandNode(all_conf->SO_NODES_NUM)].pid,SIGUSR1);
        semRelease(ids.sem_id,0);
      break;
      }
    }
  }
  /* the simulation is terminated by the signal handler in a different way depending on the reason */
  return 0;
}
