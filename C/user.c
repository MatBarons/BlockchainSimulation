#include "../Headers/user.h"
#include "../Headers/node.h"
#include "../Headers/master.h"
#include "../Headers/common_ipcs.h"

Node* nodes_user;
User* users_user;
Ledger* ledger_user;
Config* config_user;
shm_id ids_users;

int non_ledger_size;
Transaction* non_ledgers_trans;
int last_id;


void dieNotifyMasterUser(pid_t pid_dead,int reason){
  int i,cont=0;
  for(i=0;i<config_user->SO_USERS_NUM;i++){
    if(users_user[i].pid == pid_dead){
      semLock(ids_users.sem_id,1);
      users_user[i].budget = balanceCalc();
      users_user[i].dead = reason;
      semRelease(ids_users.sem_id,1);
    }
  }
  /* checks if all users are dead */
  for(i=0;i<config_user->SO_USERS_NUM;i++){
    if(users_user[i].dead == -1){
      cont++;
      if(cont == config_user->SO_USERS_NUM){
        kill(getppid(),SIGUSR2);
      }
    }
  }
}

void signalHandlerUser(int sig,siginfo_t* siginfo,void* context){
  Transaction spec_tr;
switch (sig){
  /* SIGTERM from master means that one of the acceptable termination reasons was triggered */
  case SIGTERM:
    dieNotifyMasterUser(getpid(),0);
    shmdt(users_user);
    shmdt(nodes_user);
    shmdt(ledger_user);
    shmdt(config_user);
    exit(EXIT_SUCCESS);
    break;
  /* CTRL+C received from master */
  case SIGINT:
    shmdt(users_user);
    shmdt(nodes_user);
    shmdt(ledger_user);
    shmdt(config_user);
    exit(EXIT_FAILURE);
    break;
  case SIGUSR1:
    spec_tr = createTransaction();
    if(spec_tr.quantity !=0)
      sendTransaction(spec_tr);
  break;
  default:
  break;
  }
}

int calcReward(int money){
  int reward_calc;
  reward_calc = (int) ((money * config_user->SO_REWARD)/100);
  return reward_calc < 1 ? 1 : reward_calc;
}

int calcBalance(int curr){
  int sum,i=0,j=0;
  sum = curr;
  for(i=last_id;i<=ledger_user->last_id;i++){
    for(j=0;j<SO_BLOCK_SIZE-1;j++){
      if(ledger_user->array_of_blocks[i].transactions[j].receiver == getpid()){
        sum = sum + ledger_user->array_of_blocks[i].transactions[j].quantity;
        last_id = i;
      }
    }
  }
  return sum;
}

int balanceCalc(){
  int sum,i=0,j=0,k=0;
  sum = config_user->SO_INIT_BUDGET;
  
  for(i=0;i<ledger_user->last_id;i++){
    for(j=0;j<SO_BLOCK_SIZE;j++){
      if(ledger_user->array_of_blocks[i].transactions[j].receiver == getpid())
        sum = sum + ledger_user->array_of_blocks[i].transactions[j].quantity;
    }
  }

  for(k=0;k<non_ledger_size;k++){
    sum = sum - (non_ledgers_trans[k].quantity + non_ledgers_trans[k].reward);
  }
  return sum;
}

int calcQuantity(int balance){
  int quantita;
  if(balance == 2)
    return 2;
  do{
    quantita = rand()%balance;
  }while(quantita < 2);
  return quantita;
}

long rand_nSec(){
  long res;
  do{
    res = rand()%config_user->SO_MAX_TRANS_GEN_NSEC;
  }while(res < config_user->SO_MIN_TRANS_GEN_NSEC);
  return res;
}

int chooseRandReceiver(){
  int rec;
  do{
  rec = rand()%config_user->SO_USERS_NUM;
  }while(users_user[rec].pid == getpid() || users_user[rec].dead == -1);
  return rec;
}

Transaction createTransaction(){
  Transaction tr;
  struct timespec time_req;
  int temp,i;
  for(i=0;i<config_user->SO_USERS_NUM;i++){
    if(users_user[i].pid == getpid()){
      semLock(ids_users.sem_id,1);
      users_user[i].budget = balanceCalc();
      semRelease(ids_users.sem_id,1);
      if(users_user[i].budget > 2){
        if(clock_gettime(CLOCK_REALTIME,&time_req) == -1)
          perror("clock gettime failed in createTransaction()");
        temp = calcQuantity(users_user[i].budget);
        tr.receiver = users_user[chooseRandReceiver()].pid;
        tr.reward = calcReward(temp);
        tr.quantity = temp - tr.reward;
        tr.sender = getpid();
        tr.timestamp = time_req.tv_nsec;
        tr.n_of_hops = 0;

        semLock(ids_users.sem_id,1);
        users_user[i].budget = balanceCalc();
        semRelease(ids_users.sem_id,1);

      }else{
        tr.quantity = 0;
      }
      break;
    }
  }
  return tr;
}

int sendTransaction(Transaction tr){
  int error = 1;
  Message m;
  struct timespec t1,t2;
  t1.tv_sec = 0;
  t1.tv_nsec = rand_nSec();
  m.msg_type = nodes_user[chooseRandNode(config_user->SO_NODES_NUM)].pid;
  m.transaction = tr;
  semLock(ids_users.sem_id,4);
  if((msgsnd(ids_users.msgQ_id,&m,sizeof(m),IPC_NOWAIT)) == -1){
    error = -1;
  }
  semRelease(ids_users.sem_id,4);

  non_ledgers_trans[non_ledger_size] = tr;
  non_ledger_size++;

  /*--- simulate the wait for the answer from the node ---*/
  if(nanosleep(&t1,&t2) < 0 ){
    printf("Nano sleep failed in sendTransaction()");
    exit(EXIT_FAILURE);
  }
  return error;
}

void runUser(){
  Transaction trans;
  int cont=0;

  while(1){
    trans = createTransaction();
    if(trans.quantity != 0){
      if ( sendTransaction(trans) == -1 ){
        cont++;
      }else{
        cont = 0;
      }
    }else{
      cont++;
    }
    if(cont == config_user->SO_RETRY){
      dieNotifyMasterUser(getpid(),-1);
    }
  }
}

int main(int argc, char const *argv[]){
  struct sigaction sa;

  struct timespec t1;
  timespec_get(&t1,TIME_UTC);
  srand(t1.tv_nsec + getpid());
  
  ids_users.book_id = atoi(argv[1]);
  ids_users.nodes_id = atoi(argv[2]);
  ids_users.users_id = atoi(argv[3]);
  ids_users.msgQ_id = atoi(argv[4]);
  ids_users.sem_id = atoi(argv[5]);
  ids_users.config_id = atoi(argv[6]);

  nodes_user = (Node*)attachSHM(ids_users.nodes_id);
  users_user = (User*)attachSHM(ids_users.users_id);
  ledger_user = (Ledger*)attachSHM(ids_users.book_id);
  config_user = (Config*)attachSHM(ids_users.config_id);


  sa.sa_sigaction=&signalHandlerUser;
  bzero(&sa.sa_mask,sizeof(sa));
  sa.sa_flags=0;
  sigaction(SIGTERM,&sa,NULL);
  sigaction(SIGINT,&sa,NULL);
  sigaction(SIGUSR1,&sa,NULL);

  non_ledgers_trans = malloc(100000*sizeof(Transaction));
  non_ledger_size = 0;
  last_id =0;
  semLock(ids_users.sem_id,1);
  runUser();


  return 0;
}

