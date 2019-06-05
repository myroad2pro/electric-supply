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
#include <sys/msg.h>

struct mesg_buffer
{
    long mesg_type;
    char mesg_text[100];
} message1, message2;

typedef struct
{
    int warningThreshold, maxThreshold, currentSupply, status;
} t_systemInfo;

typedef struct
{
    int deviceID, normalVoltage, savingVoltage, status;
} t_deviceInfo;

enum t_infoType
{
    T_SYSTEM,
    T_DEVICE
};

enum t_accessType
{
    T_READ,
    T_WRITE
};

int main()
{
    key_t key1, key2;
    int msgId1, msgId2;
    char messageBuffer[100] = "", infoBuffer[100] = "";
    char *infoToken;

    // ftok to generate unique key
    key1 = ftok("keyfile", 3993); // for elePowerCtrl
    key2 = ftok("keyfile", 9339); // for powerSupplyInfoAccess
    // msgget creates a message queue
    // and returns identifier
    msgId1 = msgget(key1, 0666 | IPC_CREAT);
    msgId2 = msgget(key2, 0666 | IPC_CREAT);

    while (1)
    {
        // msgrcv to receive message
        if (msgrcv(msgId1, &message1, sizeof(message1), 1, 0) != -1)
        {
            int deviceID, mode;
            t_systemInfo systemInfo;
            t_deviceInfo equipInfo;
            char *token;
            // get deviceID and mode
            memset(messageBuffer, 0, sizeof(messageBuffer));
            strcpy(messageBuffer, message1.mesg_text);
            // extract 
            token = strtok(messageBuffer, "|");
            deviceID = atoi(token);
            token = strtok(NULL, "|");
            mode = atoi(token);
            // request to get SystemInfo
            sprintf(message2.mesg_text, "%d|%d|", T_READ, T_SYSTEM);
            // msgsnd to send message
            msgsnd(msgId2, &message2, sizeof(message2), 0);

            // receive systemInfo
            memset(message2.mesg_text, 0, sizeof(message2.mesg_text));
            if(msgrcv(msgId2, &message2, sizeof(message2), 1, 0) != -1){
                memset(infoBuffer, 0, sizeof(infoBuffer));
                strcpy(infoBuffer, message2.mesg_text);
                // extract info
                infoToken = strtok(infoBuffer, "|");
                systemInfo.warningThreshold = atoi(infoToken);
                infoToken = strtok(NULL, "|");
                systemInfo.maxThreshold = atoi(infoToken);
                infoToken = strtok(NULL, "|");
                systemInfo.currentSupply = atoi(infoToken);
                infoToken = strtok(NULL, "|");
                systemInfo.status = atoi(infoToken);
            }

            // request to get deviceInfo
            sprintf(message2.mesg_text, "%d|%d|%d|", T_READ, T_DEVICE, deviceID);
            // msgsnd to send message
            msgsnd(msgId2, &message2, sizeof(message2), 0);

            // receive deviceInfo
            memset(message2.mesg_text, 0, sizeof(message2.mesg_text));
            if(msgrcv(msgId2, &message2, sizeof(message2), 1, 0) != -1){
                memset(infoBuffer, 0, sizeof(infoBuffer));
                strcpy(infoBuffer, message2.mesg_text);
                // extract info
                infoToken = strtok(infoBuffer, "|");
                systemInfo.warningThreshold = atoi(infoToken);
                infoToken = strtok(NULL, "|");
                systemInfo.maxThreshold = atoi(infoToken);
                infoToken = strtok(NULL, "|");
                systemInfo.currentSupply = atoi(infoToken);
                infoToken = strtok(NULL, "|");
                systemInfo.status = atoi(infoToken);
            }
        }
    }

    // to destroy the message queue
    msgctl(msgId1, IPC_RMID, NULL);
    msgctl(msgId2, IPC_RMID, NULL);

    return 0;
}