#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 1024 /*max text line length*/
#define SERV_PORT 3000 /*port*/
#define LISTENQ 8 /*maximum number of client connections*/


int listenSock, connectSock, n;
pid_t pid;
char request[MAXLINE];
struct sockaddr_in serverAddr, clientAddr;
socklen_t clilen;


void sig_chld(int singno){
        pid_t pid;
        int stat;
        while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
                printf("child %d terminated\n", pid);
        return;
}

int main(){

        if((listenSock = socket(AF_INET,SOCK_STREAM,0)) < 0) {
                printf("Loi tao socket\n");
                exit(1);
        }
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(SERV_PORT);

        int enable = 1;
        if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
                perror("setsockopt(SO_REUSEADDR) failed");

        if(bind(listenSock,(struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
                printf("Loi bind\n");
                exit(2);
        }

        listen(listenSock,LISTENQ);
        clilen = sizeof(clientAddr);

        while (1) {
                connectSock = accept (listenSock, (struct sockaddr *) &clientAddr, &clilen);
                if((pid=fork()) == 0) {
                        close(listenSock);
                        while ((n = recv(connectSock, request, MAXLINE,0)) > 0)  {
                                char *message = "long";
                                if (message != NULL) {
                                        send(connectSock, message, strlen(message), 0);
                                }
                        }
                        close(connectSock);
                        exit(0);
                }
                signal(SIGCHLD,sig_chld);
                close(connectSock);
        }

}
