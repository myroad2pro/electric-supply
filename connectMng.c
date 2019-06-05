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
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHMSZ 8
#define KEY "1234"
#define MAXLINE 1024   /*max text line length*/
#define SERV_PORT 3000 /*port*/
#define LISTENQ 10     /*maximum number of client connections*/

struct mesg_buffer
{
    long mesg_type;
    char mesg_text[100];
} message1, message2;

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
    OFF,
    NORMAL,
    SAVING
};

enum systemStatus
{
    OFF,
    NORMAL,
    WARNING,
    OVER
};



int listenSock,
    connectSock, n;
pid_t pid;
char request[MAXLINE];
struct sockaddr_in serverAddr, clientAddr;
socklen_t clilen;
command cmd;
char *shm2;

// lấy địa chỉ bộ nhớ dùng chung của log
void getInfo(char *key_from_server)
{
    int shmid;
    key_t key;
    key = atoi(key_from_server);

    if ((shmid = shmget(key, 1000, 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }

    if ((shm2 = shmat(shmid, NULL, 0)) == (char *)-1)
    {
        perror("shmat");
        exit(1);
    }
}

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

    // tạo bộ nhớ dùng chung
    if ((shmid = shmget(key, 8, IPC_CREAT | 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }

    // lấy địa chỉ bộ nhớ dùng chung
    if ((shm = shmat(shmid, NULL, 0)) == (int *)-1)
    {
        perror("shmat");
        exit(1);
    }

    *shm = 0;
    int *currentDevice = shm + 1;
    *currentDevice = 2;
    getInfo("1111"); // sử dụng log của hệ thống

    // connectMng
    // khởi tạo kết nối IP
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
                // phần dưới này chuyển sang powerSupply()
            }
            if (currentVoltage != 0)
            {
                *shm = *shm - currentVoltage;
                currentVoltage = 0;
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
    t_deviceInfo equipInfo;
    int voltage, mode;
    char response[100];
    key_t key1, key2;
    int msgId1, msgId2;

    // ftok to generate unique key
    key1 = ftok("keyfile", 3993); // for elePowerCtrl
    key2 = ftok("keyfile", 9339); // for powerSupplyInfoAccess
    // msgget creates a message queue
    // and returns identifier
    msgId1 = msgget(key1, 0666 | IPC_CREAT);
    msgId2 = msgget(key2, 0666 | IPC_CREAT);

    if (strcmp(cmd.code, "STOP") == 0)
    {
        //mode = elePowerCtrl(deviceId, OFF);
        sprintf(message1.mesg_text, "%d|%d|", deviceId, OFF);

        // msgsnd to send message
        msgsnd(msgid1, &message1, sizeof(message1), 0);
        // *shm = *shm - currentVoltage;
        // currentVoltage = 0;
        // sprintf(shm2, "%s|%s|%s|", cmd.params[0], "STOP", "0");
    }
    else
    {
        if(strcmp(cmd.params[1]) == "NORMAL") mode = NORMAL;
        else if(strcmp(cmd.params[1]) == "SAVING") mode = SAVING;

        sprintf(message1.mesg_text, "%d|%d|", deviceId, mode);

        // msgsnd to send message
        msgsnd(msgid1, &message1, sizeof(message1), 0);

        //sprintf(, "%s|%s|%s|", cmd.params[0], cmd.params[1], cmd.params[2]);
        // currentVoltage = atoi(cmd.params[2]);
        // *shm = *shm + currentVoltage;
    }

    //send(connectSock, KEY, 4, 0);
}

int elePowerCtrl(int deviceID, int requestedMode)
{
    key_t key1, key2;
    int msgId1, msgId2;

    // ftok to generate unique key
    key1 = ftok("keyfile", 3993);
    key2 = ftok("keyfile", 9339);
    // msgget creates a message queue
    // and returns identifier
    msgId1 = msgget(key1, 0666 | IPC_CREAT);
    msgId2 = msgget(key2, 0666 | IPC_CREAT);

    t_systemInfo sysInfo;
    t_deviceInfo equipInfo;

    sysInfo = (t_systemInfo *)powSupplyInfoAccess(T_READ, T_SYSTEM, NULL);
    equipInfo = (t_deviceInfo *)powSupplyInfoAccess(T_READ, T_DEVICE, (void *)&deviceID);
    switch (requestedMode)
    {
    case OFF:
        deviceMode = OFF;
        return deviceMode;
        break;
    case NORMAL:

        break;
    case SAVING:
        break;
    }
}

// tra ve key cho tien trinh goi de truy cap vao vung nho dung chung
key_t powSupplyInfoAccess(int accessMode, int infoType, void *params)
{
    if (infoType == T_SYSTEM)
    {
    }
    else if (infoType == T_DEVICE)
    {
    }
    else
        return -1;
}
