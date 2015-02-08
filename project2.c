#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/prctl.h>
#define SIZE 10

void sigHandler (int);
void createServer(int, int, char*, char*[], int);
pid_t pid, secondPid, ppid;


struct serverProcess{
	int min;
	int max;
	int current;
	char name[50];
	pid_t pid;
	pid_t children[10];
	
} allServers[SIZE];


char str[50];
char *word;
int minProc, maxProc;
int var2;
int currentServer = 0;


int main(int argc, char *argv[])
{
	
	
	while(1){
		printf("Specify Command:\n");
		fgets(str, 50, stdin);
		word = strtok(str, " \n");
		
		if(!strcmp(word, "createServer")){
			
			word = strtok(NULL, " \n");
			sscanf (word, "%d", &minProc);
			word = strtok(NULL, " \n");
			sscanf (word, "%d", &maxProc);		
			word = strtok(NULL, " \n");
			if ((pid = fork()) < 0){
				perror("Fork failure");
				exit(1);
			}
			//Main Server Process
			else if (!pid){
				//Changes the name of the child process to the new name
				sprintf(argv[0], "%s", word);
				printf("PARENT SERVER PROCESS: %d\n", getpid());
				currentServer++;
				createServer(minProc, maxProc, word, argv, argc);
				pause();
				
			}
			//Process Manager
			else{
				allServers[currentServer].min = minProc;
				allServers[currentServer].max = maxProc;
				allServers[currentServer].current = minProc;
				strcpy(allServers[currentServer].name, word);
				allServers[currentServer].pid = pid;
				currentServer++;
				printf("PROCESS MANAGER: %d\n", getpid());
				printf("%d\n", allServers[0].min);
				printf("%d\n", allServers[0].max);
				printf("%d\n", allServers[0].current);
				printf("%s\n", allServers[0].name);
				printf("%d\n", allServers[0].pid);
			}
		}
		else if (!strcmp(word, "abortServer")){
			int i = 0;
			word = strtok(NULL, " \n");
			while( i < currentServer){
				if(!strcmp(word, allServers[i].name)){
					printf("We found a server with the name %s, PID %d. Aborting.\n", word, allServers[i].pid);
					kill(allServers[i].pid, SIGUSR1);
					kill(allServers[i].pid, SIGKILL);
				}
				i++;
			}
			printf("End of Abort");
		}

	}
	
	return(0);
		
}




void createServer(int minProcs, int maxProcs, char *serverName, char *argv[], int argc){
	//Add checking that min is less than max
	//TODO: This still doesn't work right
		
	if(minProcs >= maxProcs){
		perror("Min procs must be less than max procs");
		exit(0);
	}
	
	int i = 0;
	while ( i != minProcs){
		if ((pid = fork()) < 0){
			perror("Fork error");
		}
		//Server child clone
		else if(!pid){
			sprintf(argv[0], "%s %d", word, i);
			prctl(PR_SET_PDEATHSIG, SIGINT);
			pause();
		}
		//Main Server
		else{

			i++;
		}
	}
	
		
	printf("Pid: %d\n", getpid());	
	signal(SIGUSR1, sigHandler);
	pause();
	
}	

void
sigHandler (int sigNum)
{
	if(sigNum == SIGUSR1){
		printf("SIGUSER1");
		exit(0);
	}

	
    
} 

