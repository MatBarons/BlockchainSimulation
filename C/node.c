#include "../Headers/node.h"

Transaction* trans_pool;
Node* nodes_node;
Ledger* ledger_node;
Config* config_node;
int curr_pool_size;
shm_id ids_nodes;



int randomTime(){
  int ret_time = 0;
  do{
    ret_time = rand()%config_node->SO_MAX_TRANS_PROC_NSEC;
  }while(ret_time < config_node->SO_MIN_TRANS_PROC_NSEC);
  return ret_time;
}

void calcNodeBalance(int new_balance){
  int i;
  for(i=0;i<config_node->SO_NODES_NUM;i++){
    if(nodes_node[i].pid == getpid()){
      semLock(ids_nodes.sem_id,2);
      nodes_node[i].reward_sum += new_balance; 
      semRelease(ids_nodes.sem_id,2);
    }
  }
}

pid_t chooseRandFriend(){
  int randNode,i,result;
  
  randNode = rand()%config_node->SO_NODES_NUM;
  for(i=0;i<config_node->SO_NODES_NUM;i++){
    if(nodes_node[i].pid == getpid()){
      result = nodes_node[i].friends[randNode];
      break;  
    }
  }
  return result;
}

void notifyDieMasterNode(pid_t pr_ID){
  int i;
  for(i=0;i<config_node->SO_NODES_NUM;i++){
    if(nodes_node[i].pid == pr_ID){
      semLock(ids_nodes.sem_id,2);
      nodes_node[i].n_transQ = curr_pool_size;
      semRelease(ids_nodes.sem_id,2);
    }
  }
}

void sendToFriend(){
  Message m;
  m.msg_type = chooseRandFriend();
  m.transaction = trans_pool[curr_pool_size-1];
  memset(&trans_pool[curr_pool_size-1],0,sizeof(Transaction));
  curr_pool_size = curr_pool_size-1;
  semLock(ids_nodes.sem_id,4);
  msgsnd(ids_nodes.msgQ_id,&m,sizeof(m),IPC_NOWAIT);
  semRelease(ids_nodes.sem_id,4);
}

void signalHandlerNode(int sig,siginfo_t* siginfo,void* context){
  int i,j=0;
  switch (sig){
  /* SIGALRM means the node needs to send a transaction to a friend */
  case SIGALRM:
    if(curr_pool_size != 0)
      sendToFriend();
    alarm(config_node->SO_SIM_SEC/2);
    break;
  /* SIGTERM from master means that one of the acceptable termination reasons was triggered */
  case SIGTERM:
    notifyDieMasterNode(getpid());
    free(trans_pool);
    shmdt(ledger_node);
    shmdt(nodes_node);
    shmdt(config_node);
    exit(EXIT_SUCCESS);
    break;
  /* CTRL+C received from master */
  case SIGINT:
    free(trans_pool);
    shmdt(ledger_node);
    shmdt(nodes_node);
    shmdt(config_node);
    exit(EXIT_FAILURE);
    break;
  /* add the new node to list of friends of a few nodes */
  case SIGUSR1:
    for(i=0;i <config_node->SO_NODES_NUM;i++){
      if(nodes_node[i].pid == getpid()){
        while(1){
          if(nodes_node[i].friends[j] == -1){
            semLock(ids_nodes.sem_id,3);
            nodes_node[i].friends[j] = nodes_node[config_node->SO_NODES_NUM-1].pid;
            nodes_node[i].friends[j+1] = -1;
            semRelease(ids_nodes.sem_id,3);
            break;
          }
        }
      }
    }
  break;
  default:
  break;
  }
}



int check_master_trans(Transaction tr){
  int flag=1,i=0,j=0;
  if(ledger_node->last_id >= SO_REGISTRY_SIZE){
    /* no more space in the registry */
    flag = 0;
  }else{
    while(i <= ledger_node->last_id){
      while(j < SO_BLOCK_SIZE){
        /* checks if the transaction already exist in the ledger */
        if(ledger_node->array_of_blocks[i].transactions[j].timestamp == tr.timestamp && ledger_node->array_of_blocks[i].transactions[j].receiver == tr.receiver && ledger_node->array_of_blocks[i].transactions[j].sender == tr.sender)
          flag = -1;
        j++;
      }
    i++;
    }
  }
  return flag;
}

Transaction rewardTransaction(int premio){
  Transaction rwd_tr;
  struct timespec spec_tr;
  clock_gettime(CLOCK_REALTIME,&spec_tr);
  rwd_tr.sender = my_macro(-1);
  rwd_tr.receiver = getpid();
  rwd_tr.quantity = premio;
  rwd_tr.reward = 0;
  rwd_tr.timestamp = spec_tr.tv_nsec;
  return rwd_tr;
}

Block createBlock(){
  int z=0,premio=0;
  Block block_to_insert;
  struct timespec t1,t2;
  t1.tv_sec = 0;
  t1.tv_nsec = randomTime();
  while(z<SO_BLOCK_SIZE-1){
    block_to_insert.transactions[z] = trans_pool[z];
    premio = premio + block_to_insert.transactions[z].reward;
    memset(&trans_pool[z],0,sizeof(Transaction));
    z++;
  }
  block_to_insert.transactions[SO_BLOCK_SIZE-1] = rewardTransaction(premio);
  calcNodeBalance(premio);

  if(nanosleep(&t1,&t2) < 0){
    printf("nanosleep failed in createBlock()");
    exit(EXIT_FAILURE);
  }
  return block_to_insert;
}



void runNode(){
  Message m;
  int i=0;
  Block block_to_send;
  int flag;

  curr_pool_size = 0;
  /*alarm(config_node->SO_SIM_SEC/2);*/
  while(1){
    if(msgrcv(ids_nodes.msgQ_id,&m,sizeof(m),getpid(),0) == -1 && errno != EINTR && errno != ENOMSG){
      perror("msgrcv failed in runNode()");
      exit(EXIT_FAILURE);
    }else if(errno == ENOMSG){
      continue;
    }else{
      m.transaction.n_of_hops++;
      curr_pool_size++;
      if(curr_pool_size < config_node->SO_TP_SIZE){
        flag = check_master_trans(m.transaction);
        if(flag == 1){
          trans_pool[i] = m.transaction;
          i++;
          if(curr_pool_size >= SO_BLOCK_SIZE -1){
            block_to_send = createBlock();
            i=0;
            curr_pool_size = curr_pool_size - (SO_BLOCK_SIZE -1);
            semLock(ids_nodes.sem_id,3);
            ledger_node->last_id = ledger_node->last_id+1;
            block_to_send.id = ledger_node->last_id;
            ledger_node->array_of_blocks[ledger_node->last_id] = block_to_send;
            semRelease(ids_nodes.sem_id,3);
          }
        }else if(flag == 0){
          curr_pool_size --;
          kill(getppid(),SIGUSR1);
          /*no more space in the registry */
        }else{
          curr_pool_size--;
        }
      }else{
        if(m.transaction.n_of_hops == config_node->SO_HOPS){
          m.msg_type = getppid();
        }else{
          m.msg_type = chooseRandFriend();
        }
        semLock(ids_nodes.sem_id,4);
        msgsnd(ids_nodes.msgQ_id,&m,sizeof(m),IPC_NOWAIT);
        semRelease(ids_nodes.sem_id,4);
      }
    }
  }
}

int main(int argc, char const *argv[]){
  struct sigaction sa;

  struct timespec t1;
  timespec_get(&t1,TIME_UTC);
  srand(t1.tv_nsec + getpid());
  /* initializing the ids to access the shared memory */
  ids_nodes.book_id = atoi(argv[1]);
  ids_nodes.nodes_id = atoi(argv[2]);
  ids_nodes.users_id =atoi(argv[3]);
  ids_nodes.msgQ_id = atoi(argv[4]);
  ids_nodes.sem_id = atoi(argv[5]);
  ids_nodes.config_id = atoi(argv[6]);
  
  /* attaching and allocating memory */
  ledger_node = (Ledger*)attachSHM(ids_nodes.book_id);
  ledger_node->last_id = 0;
  nodes_node = (Node*)attachSHM(ids_nodes.nodes_id);
  config_node = (Config*)attachSHM(ids_nodes.config_id);
  trans_pool = malloc(config_node->SO_TP_SIZE * sizeof(Transaction));
  
  sa.sa_sigaction=&signalHandlerNode;
  bzero(&sa.sa_mask,sizeof(sa));
  sa.sa_flags=0;
  sigaction(SIGTERM,&sa,NULL);
  sigaction(SIGINT,&sa,NULL);
  sigaction(SIGUSR1,&sa,NULL);
  sigaction(SIGALRM,&sa,NULL);

  semLock(ids_nodes.sem_id,0);
  runNode();

  return 0;
}


