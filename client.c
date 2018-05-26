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


int clientSocket;
char serverResponse[MAXLINE];
int *shm;


int kbhit();
int getch();
void showMenuDevices();
void showMenu();
void getResponse();
void makeCommand(char* command, char* code, char* param1, char* param2);
void showMenuAction(char *deviceName, int MODE_DEFAULT, int MODE_SAVING);
void getShareMemoryPointer(char * key_from_server);
void runDevice(int voltage, char* deviceName);
void stopDevice(char *deviceName);


int main(){
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
			showMenuAction("TV",3000,500);
			break;
                case 2:
			showMenuAction("Air Conditioner",1000,500);
                        break;
		case 3:
			showMenuAction("PC",700,300);
			break;
                default:
                        exit(0);
                }
        }
}

void showMenuAction(char *deviceName, int MODE_DEFAULT, int MODE_SAVING) {
	int choice;
        char c;
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
			runDevice(MODE_DEFAULT,deviceName);
			break;
                case 2:
			runDevice(MODE_SAVING,deviceName);
			break;
                default:
                        exit(0);
                }
        }
}
void getResponse(){
      	int n = recv(clientSocket, serverResponse, MAXLINE, 0);
      	if (n == 0) {
	      perror("The server terminated prematurely");
	      exit(4);
      	}
      	serverResponse[n] = '\0';
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


void getShareMemoryPointer(char * key_from_server){
	int shmid;
        key_t key;
	key = atoi(key_from_server);

	if ((shmid = shmget(key, SHMSZ, 0666)) < 0) {
	    perror("shmget");
	    exit(1);
	}

	if ((shm = shmat(shmid, NULL, 0)) == (int*) -1) {
	    perror("shmat");
	    exit(1);
	}
}

void runDevice(int voltage, char *deviceName){
	char command[100];
	char response[100];
	char buffer[20];
	int countDown ;
	snprintf(buffer, 10, "%d", voltage);
	makeCommand(command,"ON", deviceName,buffer);
	send(clientSocket, command, strlen(command), 0);
	getResponse();
	getShareMemoryPointer(serverResponse);
	*shm = *shm + voltage;
	countDown = 10;
	while (1) {
		if (*shm<=4500){
			printf("The current device is running with %d\n Press enter to stop this device\n",*shm);
		}
		else if(*shm <= 5000){
			printf("The threshold is exceeded. The supply currently is %d\n",*shm);
		}
		else{
			printf("Maximum threshold is exceeded. A device will be turn off in %d\n", countDown);
			countDown--;
			if(countDown < 0){
				stopDevice(deviceName);
				break;
			}
		}

		if (kbhit()) {
			stopDevice(deviceName);
			break;
		}
		sleep(1);
	}
}

void stopDevice(char *deviceName)
{
	char command[100];
	makeCommand(command,"STOP", deviceName, NULL);
	send(clientSocket, command, strlen(command), 0);
	getResponse();
}
