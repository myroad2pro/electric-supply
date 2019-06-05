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
    int deviceID;
    FILE *fp = NULL;
    char messageBuffer[100] = "", infoBuffer[100] = "";

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
        if (msgrcv(msgId2, &message2, sizeof(message2), 1, 0) != -1)
        {
            int accessType, infoType;
            char *messageToken;

            strcpy(messageBuffer, message2.mesg_text);
            messageToken = strtok(messageBuffer, "|");
            accessType = atoi(messageToken);
            messageToken = strtok(NULL, "|");
            infoType = atoi(messageToken);

            if (accessType == T_READ)
            {
                if (infoType == T_SYSTEM)
                {
                    fp = fopen("sysInfo", "r");
                    fgets(infoBuffer, 100, fp);

                    memset(message2.mesg_text, 0, sizeof(message2.mesg_text));
                    strcpy(message2.mesg_text, infoBuffer);
                    msgsnd(msgId2, &message2, sizeof(message2), 0);
                    fclose(fp);
                }
                else if (infoType == T_DEVICE)
                {
                    messageToken = strtok(NULL, "|");
                    deviceID = atoi(messageToken);
                    fp = fopen("deviceInfo", "r");
                    
                    fclose(fp);
                }
            }
            else if (accessType = T_WRITE)
            {
            }
        }
    }

    // to destroy the message queue
    msgctl(msgId2, IPC_RMID, NULL);

    return 0;
}