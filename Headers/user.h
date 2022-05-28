#ifndef TEST_H_INCLUDED
#define TEST_H_INCLUDED
#include "common_ipcs.h"


/**
 * @brief notifies the master when a user die
 * @param pid_dead the pid of the process who died
 * @param reason -1 if it died prematurely,0 if it died at the end
 */
void dieNotifyMasterUser(pid_t pid_dead,int reason);

/**
 * @brief handlers the signals sended from the user
 * @param sig the signal to handle
 */
void signalHandlerUser(int sig,siginfo_t* siginfo,void* context);

/**
 * @brief calculate the reward for the node who process the transaction using SO_REWARD 
 * @param money the randomly money to send which need the budget substracted from
 * @return returns the reward (int)
 */
int calcReward(int money);

/**
 * @brief calculate the current balance of the node
 * @return int 
 */
int calcBalance(int curr);

int balanceCalc();

/**
 * @brief calculate the money to send 
 * @param balance the current balance of the user
 * @return int 
 */
int calcQuantity(int balance);
/**
 * @brief choose a random number of nSec to sleep
 * 
 * @return long 
 */
long rand_nSec();

/**
 * @brief returns a random Node used as a target of the transaction
 * @return the index of the User (Array of users)
 */
int chooseRandReceiver();

/**
 * @brief create the transaction by checking balance
 * @return the transaction to send
 */
Transaction createTransaction();



/**
 * @brief sends the transaction via msgsnd()
 * @param tr the transaction to send
 * @return 1 if everything OK, -1 if something went wrong
 */
int sendTransaction(Transaction tr);

/**
 * @brief runs all the users
 */
void runUser();

/**
 * @brief clears an array the users has to store all the transaction he sent that haven't been verified yet -- it removes the transaction after checking that it has been verified
 */
void clearNonLedgersTransArray();

/**
 * @brief move the elements in the array to clear it from all the verified transactions
 * @param found the index of the found element
 */
void moveElementsInArray(int found);



#endif



