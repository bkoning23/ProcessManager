#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/prctl.h>
#define SIZE 10
#define MAXCHILDREN 20
#define MAXSERVERS 10

void sigHandler (int);
void createServer(int, int, char*, char*[], int);
void printMessage(char*);

pid_t pid, secondPid, ppid;
pid_t children[25];
int status;

struct serverProcess{
	int min;
	int max;
	int current;
	char name[50];
	pid_t pid;
	pid_t children[25];
	
} allServers[SIZE];

char name[50];
char str[50];
char temp[50];
char *word;
int minProc, maxProc, currentProc;
int var2;
int killedProcess = 0;

//CurrentServer Count
int currentServer = 0;

int currentAction = 0;
int remakeChild = 1;


int main(int argc, char *argv[])
{
	
	
	while(1){
		
		sprintf(argv[0], "%s", "ProcessManager");
		strcpy(name, "ProcessManager");
		
		
		printf("Specify Command: ");
		fgets(str, 50, stdin);
		word = strtok(str, " \n");
		
		if(!strcmp(word, "createServer")){
			
			if(currentServer == MAXSERVERS){
				printMessage("There are too many servers, delete one to make a new one.");
				continue;
			}
			
			word = strtok(NULL, " \n");
			sscanf (word, "%d", &minProc);
			word = strtok(NULL, " \n");
			sscanf (word, "%d", &maxProc);
			currentProc = minProc;
			word = strtok(NULL, " \n");
			
			if(minProc > maxProc){
				printMessage("Min processes must be less than or equal to max processes.");
				continue;
			}
			if(maxProc > MAXCHILDREN){
				sprintf(temp, "Max processes cannot go above %d.", MAXCHILDREN);
				printMessage(temp);
				continue;
			}
				
			
			if ((pid = fork()) < 0){
				perror("Fork failure");
				exit(1);
			}
			//Main Server Process
			else if (!pid){
				//Changes the name of the child process to the new name
				sprintf(argv[0], "%s", word);
				strcpy(name, word);
				
				//Prints that a new server has been made.
				
				currentServer++;
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
				sigaction(SIGUSR1, &abort_action, NULL);
				sigaction(SIGCHLD, &new_action, NULL);
				sigaction(SIGUSR2, &new_action, NULL);
				
				// IF THERE IS A PROBLEM THIS WAS CHANGED RECENTLY
				
				sigaction(SIGTERM, &abort_action, NULL);
						
				while(1){
					pause();
					//Add a process thread
					if(currentAction >= 1){
						if ((pid = fork()) < 0){
							perror("Fork failure");
							exit(1);
						}
						else if (!pid){
							//This is installed because the above sighandlers install something for
							//SIGTERM, which is used to end a child process.
							sprintf(argv[0], "%s %d", name, currentProc);
							struct sigaction action;
							action.sa_handler = SIG_DFL;
							sigaction(SIGTERM, &action, NULL);
							pause();
						}
						//Parent records PID of new child, increments current process count
						else{
							sprintf(temp, "Copy Started. PID: %d", pid);
							printMessage(temp);
							if(currentAction == 2){
								int j = 0;
								while(children[j] != killedProcess){
									;
								}
								children[j] = pid;
							}
							else{
								children[currentProc] = pid;
								currentProc++;
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
				//printf("PROCESS MANAGER: %d\n", getpid());
				//printf("%d\n", allServers[0].min);
				//printf("%d\n", allServers[0].max);
				//printf("%d\n", allServers[0].current);
				//printf("%s\n", allServers[0].name);
				//printf("%d\n", allServers[0].pid);
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
					kill(allServers[i].pid, SIGTERM);
					wait(&status);
					break;
				}
				i++;
			}
			//This removes the server and its information from the information array
			while (i < currentServer){
				allServers[i] = allServers[i+1];
				i++;
			}
			currentServer--;
			sprintf(temp, "%s Aborted", word);
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
						printf("Kill was sent");
						allServers[i].current = allServers[i].current - 1;
						sleep(1);
						break;
					}
					
				}
				i++;
			}
			
		}
		else if(!strcmp(word, "displayStatus")){
			printf("Server Count: %d\n", currentServer);
			int i;
			for(i =0; i != currentServer; i++){
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


void printMessage(char *message){

	printf("[%s] %s\n", name, message);


}


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
		printf("SIGTERM\n");
		int i = 0;
		remakeChild = 0;
		while ( children[i] != 0 ){
			kill(children[i], SIGTERM);
			sprintf(temp, "Killed %d", children[i]);
			//printf("Killed %d\n", children[i]);
			printMessage(temp);
			i++;
		}
		exit(0);
	}
	//Abort Process
	else if(sigNum == SIGUSR1){
		remakeChild = 0;
		sprintf(temp, "THIS IS WHO WE ARE KILLING %d", children[currentProc-1]);
		printMessage(temp);
		kill(children[currentProc-1], SIGTERM);
		wait(&status);
		children[currentProc-1] = NULL;
		currentProc--;		
	}
	//Add process
	else if(sigNum == SIGUSR2){
		printf("SIGUSR2\n");
		currentAction = 1;
	}
	
	else if(sigNum == SIGCHLD){
		
		
		
		if (!remakeChild){
			printf("Process purposely aborted\n");
			remakeChild = 1;
		}
		else{
			killedProcess = wait(&status);
			if(killedProcess == -1){
				perror("Killed Process returned -1");
			}
			currentAction = 2;
			printf("SIGCHLD, %d, %d", currentAction, killedProcess);
		}
	}

} 

