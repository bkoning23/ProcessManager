
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>

void sigHandler (int);
void createServer(int, int, char*, char*[]);
pid_t pid, secondPid, ppid;

char str[50];
char *word;
int minProc, maxProc;
int var2;


int main(int argc, char *argv[])
{
	printf("Specify Command:\n");
	fgets(str, 50, stdin);
	word = strtok(str, " \n");
	
	if(!strcmp(word, "createServer")){
		if ((pid = fork()) < 0){
			perror("Fork failure");
			exit(1);
		}
		else if (!pid){
			word = strtok(NULL, " \n");
			sscanf (word, "%d", &minProc);
			word = strtok(NULL, " \n");
			sscanf (word, "%d", &maxProc);		
			word = strtok(NULL, " \n");
			printf("Pid: %d\n", getpid());
			if ((pid = fork()) < 0){
				perror("Fork error");
			}
			else if(!pid){
				sprintf(argv[0], "%s", word);
				printf("PARENT SERVER PROCESS: %d\n", getpid());
				createServer(minProc, maxProc, word, argv);
			}
		}
		else{
			sleep(2);
			printf("PROCESS MANAGER: %d\n", getpid());
		}
	}

	return(0);
		

}




void createServer(int minProcs, int maxProcs, char *serverName, char *argv[]){
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
		else if(!pid){
			sprintf(argv[0], "%s %d", word, i);
			break;
		}
		else{
			i++;
		}
	}
			
	printf("Pid: %d\n", getpid());	
	sleep(20);
	
	//HELLOOOOOOOOOOOOO
	//printf("%d %d %s\n", minProcs, maxProcs, serverName);


}	





/*
void
sigHandler (int sigNum)
{
	if(sigNum == SIGUSR1){
		printf("received a SIGUSR1 signal.\n");
		signal(SIGUSR1, sigHandler);
	}
	if(sigNum == SIGUSR2){
		printf("received a SIGUSR2 signal.\n");
		signal(SIGUSR2, sigHandler);
	}
	if(sigNum == SIGINT){
		//If the child receives the signal it does nothing. 
		if (!firstPid){				
		}
		//Parent prints that it is closing and kills the child process 
		else{
			printf("received. Closing. \n");
			kill(firstPid, SIGKILL);
			exit(0);
		}
	}
	
    
} 
*/
