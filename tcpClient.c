#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define MAXLINE 1024
#define PORT 3000

int clientSocket;
void showMenuLogin();
void getResponse();
char buffer[1024];
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
		showMenuLogin();


	}

	return 0;
}

void showMenuLogin(){
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
                        send(clientSocket, "long", strlen("long"), 0);
			getResponse();

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
        if (n <= 0) {
                perror("The server terminated prematurely");
                exit(4);
        }
        serverResponse[n] = '\0';
        printf("%s %d\n", serverResponse,n);
}
