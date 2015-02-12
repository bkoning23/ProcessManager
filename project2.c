/*	Brendan Koning
	processManager.c	
	2/12/2015
	
	This program functions as a mock server process manager. The program
	spawns "servers", which then replicate themselves a specified number of
	times. The process manager can create new servers, abort created servers,
	increase the number of duplicates and decrease the number of duplicates.
	The program can also create a hierarchical display showing the servers and
	how many duplicates they have.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#define SIZE 10
#define MAXCHILDREN 20
#define MAXSERVERS 10

//TODO: Main server process dies abnormally, children don't.

void sigHandler (int);
void createServer(int, int, char*, char*[], int);
void printMessage(char*);

/*Structure that the process manager uses to hold
information about all of the servers. The allServers
array is one more than the total number of servers 
because if we delete a server when we have a full
array of servers we would have a SEGFAULT. This is 
because the array is updated by moving the next value
into the previous location in the array. Ex, we
remove server 9. We copy server 10 into server 9, and
server 11 into server 10. If our array can only hold 10
servers, we get a SEGFAULT. Increasing the size of the
server array to MAXSERVERS+1 prevents this as the last entry
will always be NULL.*/

struct serverProcess{
	int min, max, current;
	char name[50];
	pid_t pid;
} allServers[MAXSERVERS+1];

pid_t pid;
pid_t children[MAXCHILDREN];
char name[50], str[50], temp[50];
char *word;
int minProc, maxProc, currentProc, currentNameNumber, status;
int killedProcess = 0;

//CurrentServer Count
int currentServer = 0;

/*This is a flag that specifies that a new process is
going to be added, and how to add it.

0 - Do not make a new process.
1 - Make a new process.
2 - Make a new process, replacing a process that was terminated.

*/
int currentAction = 0;

/*This is a flag to specify if a child process was aborted
on purpose (using abortProcess) or not.

0 - Child process was aborted using abortProcess
1 - Child process was not aborted using abortProcess and
	will be recreated.

*/
int remakeChild = 1;


int main(int argc, char *argv[])
{
	while(1){
		
		//Renames the original process to ProcessManager
		sprintf(argv[0], "%s", "ProcessManager");
		strcpy(name, "ProcessManager");
		
		//Prompts User for a command
		printf("Specify Command: ");
		fgets(str, 50, stdin);
		word = strtok(str, " \n");
		
		
		struct sigaction manager_action;
		manager_action.sa_handler = sigHandler;
		sigaction(SIGCHLD, &manager_action, NULL);
		
		if(!strcmp(word, "createServer")){
			
			if(currentServer == MAXSERVERS){
				printMessage("There are too many servers, delete one to make a new one.");
				continue;
			}
			
			//Stores information about the server the user wants to create
			word = strtok(NULL, " \n");
			sscanf (word, "%d", &minProc);
			word = strtok(NULL, " \n");
			sscanf (word, "%d", &maxProc);
			currentProc = minProc;
			currentNameNumber = currentProc;
			word = strtok(NULL, " \n");
			
			if(minProc < 1){
				printMessage("The minimum number of processes must be at least one.");
				continue;
			}
			if(minProc > maxProc){
				printMessage("The minimum number of processes must be less than or equal to the max number of processes.");
				continue;
			}
			if(maxProc > MAXCHILDREN){
				sprintf(temp, "The maximum number of processes cannot go above %d.", MAXCHILDREN);
				printMessage(temp);
				continue;
			}
			if(!strcmp(word, "ProcessManager")){
				printMessage("Server name cannot be ProcessManager.");
				continue;
			}
				
				
			
			if ((pid = fork()) < 0){
				perror("Fork failure");
				exit(1);
			}
			//Main Server Process
			else if (!pid){
				//Changes the name of the server process to the server name
				sprintf(argv[0], "%s", word);
				strcpy(name, word);
		
				createServer(minProc, maxProc, word, argv, argc);

				//Installs signal handlers for the main server
				struct sigaction new_action, abort_action;
				sigset_t block_mask;
				
				new_action.sa_handler = sigHandler;
				abort_action.sa_handler = sigHandler;
				
				
				sigemptyset(&block_mask);
				sigaddset(&block_mask, SIGCHLD);
				abort_action.sa_mask = block_mask;
				
				abort_action.sa_flags = 0;
				new_action.sa_flags = 0;
				sigaction(SIGUSR1, &new_action, NULL);
				sigaction(SIGCHLD, &new_action, NULL);
				sigaction(SIGUSR2, &new_action, NULL);		
						
				/*SIGTERM uses a mask because when we abort the server,
				we do not want a SIGCHLD signal being processed each
				time the main server kills a child process in order to 
				clean up. This ensures that the SIGCHLD signal is handled 
				after main server is done killing ALL of its child processes.
				In this implementation the SIGCHLD signal is not handled 
				because the main server process terminates at the 
				end of the SIGTERM signal handler.*/
				sigaction(SIGTERM, &abort_action, NULL);
						
				while(1){
					/*The main server process waits for a signal. In this 
					implementation there are four signals the main server waits for:
					SIGTERM - terminate the server and its children.
					SIGUSR1 - Abort a process (abortProcess)
					SIGUSR2 - Create a process (createProcess)
					SIGCHLD - A child process was killed.
					
					SIGTERM and SIGUSR1 are handled entirely in the signal handler.
					SIGUSR2 and SIGCHLD are partially handled here, because the new processes
					that may be created need to be renamed.
					*/
					pause();
					
					//Add a process copy
					if(currentAction >= 1){
						if ((pid = fork()) < 0){
							perror("Fork failure");
							exit(1);
						}
						else if (!pid){
							/*This is installed because the above sighandler overwrites
							SIGTERM. The children processes do not need to handle SIGTERM
							in a special way and use the default handler*/
							sprintf(argv[0], "%s %d", name, currentNameNumber);
							struct sigaction action;
							action.sa_handler = SIG_DFL;
							sigaction(SIGTERM, &action, NULL);
							pause();
						}
						//Parent records PID of new child, increments current process count
						else{
							sprintf(temp, "Copy Started. PID: %d", pid);
							printMessage(temp);
							/*If a child was terminated abnormally and we are replacing it, we find
							the PID of the terminated child in our array of children PIDs,
							and replace it with the PID of the new process.*/
							if(currentAction == 2){
								int j = 0;
								while(children[j] != killedProcess){
									j++;
								}
								children[j] = pid;
								currentNameNumber++;
							}
							/*If we are just adding a new process (createProcess) then we add
							the PID of the new process to the array of children PIDs and increment
							how many children we have.*/
							else{
								children[currentProc] = pid;
								currentProc++;
								currentNameNumber++;
							}
							currentAction = 0;
						}
					}
				}
				
			}
			//Process Manager
			else{
				//Stores information about the newly created server		
				sprintf(temp, "Server %s Created. PID: %d.", word, pid);
				printMessage(temp);
				allServers[currentServer].min = minProc;
				allServers[currentServer].max = maxProc;
				allServers[currentServer].current = minProc;
				strcpy(allServers[currentServer].name, word);
				allServers[currentServer].pid = pid;
				currentServer++;

				sleep(1);

			}
		}
		else if (!strcmp(word, "abortServer")){
			int i = 0;
			word = strtok(NULL, " \n");
			while( i < currentServer){
				//Checks for a server with the user submitted name
				if(!strcmp(word, allServers[i].name)){
					sprintf(temp, "Found a server with the name %s, PID %d. Aborting.", word, allServers[i].pid);
					printMessage(temp);
					if (kill(allServers[i].pid, SIGTERM) == -1){
						perror("Abort Send Error");
					}
					wait(&status);
					break;
				}
				i++;
			}
			/*This removes the server and its information from the information array. 
			This is done by moving each server struct forward one place in the array, 
			starting by overwritting the server we aborted.*/
			
			while (i < currentServer){
				allServers[i] = allServers[i+1];
				i++;
			}
			currentServer--;
			sprintf(temp, "%s Aborted.", word);
			printMessage(temp);
		}
		else if(!strcmp(word, "createProcess")){
			int i = 0;
			word = strtok(NULL, " \n");
			while ( i < currentServer){
				if(!strcmp(word, allServers[i].name)){
					if(allServers[i].max < allServers[i].current + 1){
						sprintf(temp, "Creating a process would result in too many processes.");
						printMessage(temp);
						break;
					}
					sprintf(temp, "Found a server to create a process, %d PID.", allServers[i].pid);
					printMessage(temp);
					if (kill(allServers[i].pid, SIGUSR2) == -1){
						perror("Create Send Error");
					}
					sleep(1);
					allServers[i].current = allServers[i].current + 1;
					break;
				}
				i++;
			}
		}
		else if(!strcmp(word, "abortProcess")){
			int i = 0;
			word = strtok(NULL, " \n");
			while ( i < currentServer){
				if(!strcmp(word, allServers[i].name)){
					if(allServers[i].min > allServers[i].current - 1){				
						printMessage("Aborting a process would result in too few processes.");
						break;
					}
					sprintf(temp, "Found a server to abort a process, %d PID.", allServers[i].pid);
					printMessage(temp);
					if (kill(allServers[i].pid, SIGUSR1) == -1){
						perror("Abort Send Error");
					}
					else{
						allServers[i].current = allServers[i].current - 1;
						sleep(1);
						break;
					}
					
				}
				i++;
			}
			
		}
		else if(!strcmp(word, "displayStatus")){
			sprintf(temp,"Server Count: %d\n", currentServer);
			printMessage(temp);
			int i;
			for(i = 0; i != currentServer; i++){
				struct serverProcess p = allServers[i];
				printf("Server %d Name: %s\n", i, p.name);
				printf("Server %d Current: %d\n", i, p.current);
				printf("Server %d Max: %d\n", i, p.max);
				printf("Server %d Min: %d\n", i, p.min);
			}
		}
			
			
			
		
	}
	return(0);
		
}

/*This function simplifies message printing by allowing the
programmer to specify only the message to print. This function
adds the name of the process that calls it to the front of the
message.*/
void printMessage(char *message){
	printf("[%s] %s\n", name, message);
}

/*Creates new copies of a server process using information specified 
by the user */
void createServer(int minProcs, int maxProcs, char *serverName, char *argv[], int argc){
	int i = 0;
	while ( i != minProcs){
		if ((pid = fork()) < 0){
			perror("Fork error");
		}
		//Server child clone
		else if(!pid){
			sprintf(argv[0], "%s %d", word, i);
			pause();
		}
		//Main Server
		else{
			children[i] = pid;
			sprintf(temp, "Copy Started. PID: %d", pid);
			printMessage(temp);
			i++;
		}
	}
}	

void
sigHandler (int sigNum)
{
	if(sigNum == SIGTERM){
		int i = 0;
		remakeChild = 0;
		while ( children[i] != 0 ){
			kill(children[i], SIGTERM);
			sprintf(temp, "Killed %d", children[i]);
			printMessage(temp);
			i++;
		}
		printMessage("Aborting.");
		exit(0);
	}
	//Abort Process
	else if(sigNum == SIGUSR1){
		remakeChild = 0;
		sprintf(temp, "Copy aborted. PID: %d", children[currentProc-1]);
		printMessage(temp);
		kill(children[currentProc-1], SIGTERM);
		wait(&status);
		children[currentProc-1] = '\0';
		currentProc--;		
	}
	//Add process
	else if(sigNum == SIGUSR2){
		currentAction = 1;
	}
	
	else if(sigNum == SIGCHLD){

		if(!strcmp(name, "ProcessManager")){
			wait(&status);
		}
		else if (!remakeChild){
			remakeChild = 1;
		}
		else{
			killedProcess = wait(&status);
			if(killedProcess == -1){
				perror("Killed Process returned -1");
			}
			//Process killed abnormally
			currentAction = 2;
			sprintf(temp, "Child Process %d aborted abnormally, restarting.", killedProcess);
			printMessage(temp);
		}
	}

} 

