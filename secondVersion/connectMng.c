#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#define SHMSZ 8
#define KEY "1234"
#define MAXLINE 1024   /*max text line length*/
#define SERV_PORT 3000 /*port*/
#define LISTENQ 10     /*maximum number of client connections*/

struct mesg_buffer
{
    long mesg_type;
    char mesg_text[100];
} message1, message2, message3;

typedef struct
{
    char code[100];
    char params[2][256];
} command;

typedef struct
{
    int warningThreshold, maxThreshold, currentSupply, status;
} t_systemInfo;

typedef struct
{
    int deviceID, normalVoltage, savingVoltage, status;
} t_deviceInfo;

enum deviceMode
{
    D_OFF,
    D_NORMAL,
    D_SAVING
};

enum systemStatus
{
    S_OFF,
    S_NORMAL,
    S_WARNING,
    S_OVER
};

int listenSock,
    connectSock, n;
pid_t pid;
char request[MAXLINE];
struct sockaddr_in serverAddr, clientAddr;
socklen_t clilen;
command cmd;
char *shm2;
t_systemInfo systemInfo;

void powerSupply(command cmd);

void sig_chld(int singno)
{
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("child %d terminated\n", pid);
    return;
}

command convertRequestToCommand(char *request)
{
    char copy[100];
    strcpy(copy, request);
    command cmd;
    char *token;
    int i = 0;
    token = strtok(copy, "|");
    strcpy(cmd.code, token);
    while (token != NULL)
    {
        token = strtok(NULL, "|");
        if (token != NULL)
        {
            strcpy(cmd.params[i++], token);
        }
    }
    return cmd;
}

int main()
{

    int shmid;
    key_t key;
    int *shm;
    key = atoi(KEY);
    int currentVoltage = 0;
    // connectMng
    // khởi tạo kết nối IP
    printf("Creating IP connection...\n\n");
    if ((listenSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Loi tao socket\n");
        exit(1);
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERV_PORT);

    int enable = 1;
    if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    if (bind(listenSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("Loi bind\n");
        exit(2);
    }

    listen(listenSock, LISTENQ);
    clilen = sizeof(clientAddr);

    // nhận kết nối từ client
    while (1)
    {
        printf("Getting connection from client...\n\n");
        connectSock = accept(listenSock, (struct sockaddr *)&clientAddr, &clilen);
        // tạo tiến trình con
        if ((pid = fork()) == 0)
        {
            close(listenSock);
            // nhận request từ client
            // powerSupply
            while ((n = recv(connectSock, request, MAXLINE, 0)) > 0)
            {
                request[n] = '\0';
                cmd = convertRequestToCommand(request);
                printf("Succeed getting command from client\n\n");
                // phần dưới này chuyển sang powerSupply()
                powerSupply(cmd);
            }

            close(connectSock);
            exit(0);
        }
        signal(SIGCHLD, sig_chld);
        close(connectSock);
    }
}

void powerSupply(command cmd)
{
    int deviceId = atoi(cmd.params[0]);
    t_deviceInfo deviceInfo;
    int voltage, mode;
    char response[100], infoBuffer[100];
    char *infoToken;
    key_t key1, key2, key3;
    int msgId1, msgId2, msgId3;
    long msgtype = 1;

    // ftok to generate unique key
    key1 = ftok("keyfile", 1); // to elePowerCtrl
    key2 = ftok("keyfile", 2); // for powerSupplyInfoAccess
    key3 = ftok("keyfile", 3); // from elePowerCtrl
    // printf("Success: Getting message queue keys %d %d %d\n\n", key1, key2, key3);

    // msgget creates a message queue
    // and returns identifier
    msgId1 = msgget(key1, 0666 | IPC_CREAT);
    msgId2 = msgget(key2, 0666 | IPC_CREAT);
    msgId3 = msgget(key3, 0666 | IPC_CREAT);
    // printf("Success: Getting message ID %d %d %d\n\n", msgId1, msgId2, msgId3);

    printf("Sending message to Power Control...\n\n");
    if (strcmp(cmd.code, "STOP") == 0)
    {
        //mode = elePowerCtrl(deviceId, OFF);
        sprintf(message1.mesg_text, "%d|%s|", deviceId, "OFF");
        // msgsnd to send message
        message1.mesg_type = msgtype;
        if(msgsnd(msgId1, &message1, sizeof(message1.mesg_text), 0) == -1) printf("Failed: Sending message to Power Control...\n\n");
    }
    else
    {
        sprintf(message1.mesg_text, "%d|%s|", deviceId, cmd.params[1]);
        // msgsnd to send message
        message1.mesg_type = msgtype;
        if(msgsnd(msgId1, &message1, sizeof(message1.mesg_text), 0) == -1) printf("Failed: Sending message to Power Control...\n\n");
    }

    if (msgrcv(msgId3, &message3, sizeof(message3), 1, 0) != -1)
    {
        printf("Success: Getting message to Power Control...\n\n");
        memset(infoBuffer, 0, sizeof(infoBuffer));
        strcpy(infoBuffer, message3.mesg_text);
        send(connectSock, infoBuffer, sizeof(infoBuffer), 0);
        printf("Success: Sending response to client ...\n\n");
    }

    //send(connectSock, KEY, 4, 0);
}