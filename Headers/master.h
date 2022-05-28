#ifndef TEST_H__INCLUDED
#define TEST_H__INCLUDED
#include "common_ipcs.h"
#include "node.h"
#include "user.h"

/**
 * @brief inserts all the variables in the struct Config (coming from a FILE)
 * @param file the file you take the variables from
 * @return 1 if everything is fine, -1 if there was an error
 */
int insertConfig(FILE *file);

/**
 * @brief handle the different signals
 * @param signal the signal to handle 
 */
void handleSignal(int signal,siginfo_t* siginfo,void* context);

/**
 * @brief prints the No of nodes and users and their balances every second
 */
void printEverySecond();

/**
 * @brief prints all the infos required at the end of the simulation
 */
void printTerminationRequirments();

/**
 * @brief create all the users
 */
void createUsers();

/**
 * @brief create all the nodes
 */
void createNodes();


#endif