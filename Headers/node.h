#ifndef H_INCLUDED
#define H_INCLUDED
#include "common_ipcs.h"


/**
 * @brief gives a random time between SO_MIN_TRANS_PROC_NSEC and SO_MAX_TRANS_PROC_NSEC
 * @return returns the chosen number
 */
int randomTime();

/**
 * @brief calculate the new balance after processing a block
 * @param new_balance the reward to sum to the node current budget
 */
void calcNodeBalance(int new_balance);

/**
 * @brief choose a friend to send the transaction to
 * 
 * @return the pid of the chosen friend
 */
pid_t chooseRandFriend();

/**
 * @brief called  at the end of the simulation, update paramethers
 * @param pr_ID the pid of the node to stop
 */
void notifyDieMasterNode(pid_t pr_ID);

/**
 * @brief handle the signals
 * @param sig the signal to be handled
 */
void signalHandlerNode(int sig,siginfo_t* siginfo,void* context);

/**
 * @brief checks if a transaction already exists or if the registry is full
 * @return different int depending on the check (-1 if it already exist, 0 if registry full,1 if everything is ok)
 */
int check_master_trans(Transaction tr);

/**
 * @brief create the reward transaction with sender = -1
 * @param premio the sum of all rewards to give the nodes
 * @return the reward transaction
 */
Transaction rewardTransaction(int premio);

/**
 * @brief create a block and give it to the master
 * @param reward the reward to give the node
 */
Block createBlock();

/**
 * @brief sends a transaction to a friend (every n of seconds->triggered by alarm)
 */
void sendToFriend();

/**
 * @brief scans the messageQueue to receive messages
 */
void runNode();






#endif