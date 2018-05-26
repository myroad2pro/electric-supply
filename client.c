#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAXLINE 1024
#define PORT 3000
#define SHMSZ     4

typedef struct {
	char name[50];
        int MODE_DEFAULT;
        int MODE_SAVING;
} device;

int clientSocket;
int kbhit();
int getch();
void showMenuDevices();
void showMenu();
void getResponse();
void makeCommand(char* command, char* code, char* param1, char* param2);
void showMenuAction(char *deviceName);
char buffer[1024];

int main(){
	int shmid;
        key_t key;
        int *shm;
	key = 5678;

	if ((shmid = shmget(key, SHMSZ, 0666)) < 0) {
	    perror("shmget");
	    exit(1);
	}

	if ((shm = shmat(shmid, NULL, 0)) == (int*) -1) {
	    perror("shmat");
	    exit(1);
	}

	printf("%d\n", *shm);

	struct sockaddr_in serverAddr;
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(clientSocket < 0){
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Client Socket is created.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Connected to Server.\n");

	while(1){
		showMenuDevices();


	}

	return 0;
}

void showMenuDevices(){
        int choice;
        char c;
	device d;
        while (1) {
                choice = 0;
                printf("-----Welcome-----\n");
                printf("Please choose type of device to connect\n");
                printf("1. TV\n");
                printf("2. Air Conditioner\n");
                printf("3. PC\n");
		printf("4. Quit\n");
                printf("Your choice: ");
                while (choice == 0) {
                        if(scanf("%d",&choice) < 1) {
                                choice = 0;
                        }
                        if(choice < 1 || choice > 4) {
                                choice = 0;
                                printf("Invalid choice!\n");
                                printf("Enter again: ");
                        }
                        while((c = getchar())!='\n') ;
                }

                switch (choice) {
                case 1:
			strcpy(d.name, "TV");
			d.MODE_DEFAULT = 800;
			d.MODE_SAVING =  500;
			showMenuAction(d.name);
			break;


                case 2:
                        break;
                default:
                        exit(0);
                }
        }
}

void showMenuAction(char *deviceName) {
	int choice;
        char c;
	char command[100];
        while (1) {
                choice = 0;
                printf("-----Welcome-----\n");
                printf("Please choose an action:\n");
                printf("1. Run at default mode \n");
                printf("2. Run at saving mode\n");
                printf("3. Turn off and quit\n");
                printf("Your choice: ");
                while (choice == 0) {
                        if(scanf("%d",&choice) < 1) {
                                choice = 0;
                        }
                        if(choice < 1 || choice > 4) {
                                choice = 0;
                                printf("Invalid choice!\n");
                                printf("Enter again: ");
                        }
                        while((c = getchar())!='\n') ;
                }

                switch (choice) {
                case 1:
			makeCommand(command,"ON", deviceName, "800");
			send(clientSocket, command, strlen(command), 0);
			getResponse();
			while (1) {
				printf("%s\n","running" );
				if (kbhit()) {
					makeCommand(command,"STOP", deviceName, NULL);
					send(clientSocket, command, strlen(command), 0);
					getResponse();
					break;
				}
				sleep(1);
			}
                case 2:
                        break;
                default:
                        exit(0);
                }
        }
}
void getResponse(){
	char serverResponse[MAXLINE];
      	int n = recv(clientSocket, serverResponse, MAXLINE, 0);
      	if (n == 0) {
	      perror("The server terminated prematurely");
	      exit(4);
      	}
      	serverResponse[n] = '\0';
      	printf("%s\n", serverResponse);
}

int kbhit()
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

int getch()
{
	    int r;
	    unsigned char c;
	    if ((r = read(0, &c, sizeof(c))) < 0) {
	        return r;
	    } else {
	        return c;
	    }
}

void makeCommand(char* command, char* code, char* param1, char* param2){
        strcpy(command, code);
        strcat(command, "|");
        if (param1 != NULL) {
                strcat(command,param1);
                strcat(command,"|");
        }
        if (param2 != NULL) {
                strcat(command,param2);
                strcat(command,"|");
        }
}
