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
} message1, message2, message3;

typedef struct
{
    int warningThreshold, maxThreshold, currentSupply;
    char status[10];
} t_systemInfo;

typedef struct
{
    int deviceID, normalVoltage, savingVoltage, currentSupply;
    char status[10];
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

enum t_deviceMode
{
    OFF,
    NORMAL,
    SAVING
};

enum t_systemStatus
{
    OFF,
    NORMAL,
    WARNING,
    OVER
};

int main()
{
    key_t key1, key2, key3;
    int msgId1, msgId2, msgId3;
    char messageBuffer[100] = "", infoBuffer[100] = "";
    char *infoToken;

    // ftok to generate unique key
    key1 = ftok("keyfile", 3993); // to elePowerCtrl
    key2 = ftok("keyfile", 9339); // for powerSupplyInfoAccess
    key3 = ftok("keyfile", 6996); // from elePowerCtrl
    // msgget creates a message queue
    // and returns identifier
    msgId1 = msgget(key1, 0666 | IPC_CREAT);
    msgId2 = msgget(key2, 0666 | IPC_CREAT);
    msgId3 = msgget(key3, 0666 | IPC_CREAT);

    while (1)
    {
        // msgrcv to receive message
        if (msgrcv(msgId1, &message1, sizeof(message1), 1, 0) != -1)
        {
            int deviceID, requestedSupply;
            char requestedMode[10];
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
            strcpy(requestedMode, token);

            // request to get SystemInfo
            sprintf(message2.mesg_text, "%d|%d|", T_READ, T_SYSTEM);
            // msgsnd to send message
            msgsnd(msgId2, &message2, sizeof(message2), 0);

            // receive systemInfo
            memset(message2.mesg_text, 0, sizeof(message2.mesg_text));
            if (msgrcv(msgId2, &message2, sizeof(message2), 1, 0) != -1)
            {
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
                strcpy(systemInfo.status, infoToken);
            }

            // request to get deviceInfo
            sprintf(message2.mesg_text, "%d|%d|%d|", T_READ, T_DEVICE, deviceID);
            // msgsnd to send message
            msgsnd(msgId2, &message2, sizeof(message2), 0);

            // receive deviceInfo
            memset(message2.mesg_text, 0, sizeof(message2.mesg_text));
            if (msgrcv(msgId2, &message2, sizeof(message2), 1, 0) != -1)
            {
                memset(infoBuffer, 0, sizeof(infoBuffer));
                strcpy(infoBuffer, message2.mesg_text);
                // extract info
                infoToken = strtok(infoBuffer, "|");
                equipInfo.deviceID = deviceID;
                infoToken = strtok(NULL, "|");
                equipInfo.normalVoltage = atoi(infoToken);
                infoToken = strtok(NULL, "|");
                equipInfo.savingVoltage = atoi(infoToken);
                infoToken = strtok(NULL, "|");
                strcpy(equipInfo.status, infoToken);

                // set current supply of device
                if (strcmp(equipInfo.status, "OFF") == 0)
                    equipInfo.currentSupply = 0;
                else if (strcmp(equipInfo.status, "NORMAL") == 0)
                    equipInfo.currentSupply = equipInfo.normalVoltage;
                else if (strcmp(equipInfo.status, "SAVING") == 0)
                    equipInfo.currentSupply = equipInfo.savingVoltage;

                // set requested supply of device
                if (strcmp(requestedMode, "OFF") == 0)
                    requestedSupply = 0;
                else if (strcmp(requestedMode, "NORMAL") == 0)
                    requestedSupply = equipInfo.normalVoltage;
                else if (strcmp(requestedMode, "SAVING") == 0)
                    requestedSupply = equipInfo.savingVoltage;
            }

            if (strcmp(requestedMode, equipInfo.status) == 0)
            {
                memset(message3.mesg_text, 0, sizeof(message3.mesg_text));
                sprintf(message3.mesg_text, "%s|%d|%s|", systemInfo.status, equipInfo.currentSupply, equipInfo.status);
                // send message to powerSupply
                msgsnd(msgId3, &message3, sizeof(message3), 0);
            }
            else
            {
                if (strcmp(equipInfo.status, "OFF") == 0 && strcmp(requestedMode, "OFF") != 0)
                {
                    int predictedSupply = systemInfo.currentSupply + requestedSupply;
                    if (predictedSupply > systemInfo.maxThreshold)
                    {
                        if (strcmp(requestedMode, "NORMAL") == 0)
                        {
                            // adjust to saving mode
                            int modifiedSupply = systemInfo.currentSupply + equipInfo.savingVoltage;
                            if (modifiedSupply > systemInfo.maxThreshold)
                            {
                                memset(equipInfo.status, 0, sizeof(equipInfo.status));
                                strcpy(equipInfo.status, "OFF");
                                // send message to powerSupply
                                memset(message3.mesg_text, 0, sizeof(message3.mesg_text));
                                sprintf(message3.mesg_text, "%s|%d|%s|", systemInfo.status, equipInfo.currentSupply, equipInfo.status);

                                // log
                            }
                            else if (modifiedSupply >= systemInfo.warningThreshold)
                            {
                                // set system to WARNING state
                                memset(systemInfo.status, 0, sizeof(systemInfo.status));
                                strcpy(systemInfo.status, "WARNING");
                                // set device to SAVING mode
                                memset(equipInfo.status, 0, sizeof(equipInfo.status));
                                strcpy(equipInfo.status, "SAVING");
                                equipInfo.currentSupply = equipInfo.savingVoltage;
                                // send message to powerSupplyInfoAccess to write SystemInfo
                                sprintf(message2.mesg_text, "%d|%d|%d|%s|", T_WRITE, T_SYSTEM, modifiedSupply, systemInfo.status);
                                msgsnd(msgId2, &message2, sizeof(message2), 0);
                                // send message to powerSupplyInfoAccess to write deviceInfo
                                sprintf(message2.mesg_text, "%d|%d|%d|%d|%s|", T_WRITE, T_DEVICE, deviceID, equipInfo.currentSupply, equipInfo.status);
                                msgsnd(msgId2, &message2, sizeof(message2), 0);
                                // send message to powerSupply

                                // log
                            }
                        }
                        else if (strcmp(requestedMode, "SAVING") == 0)
                        {
                        }
                    }
                    else if (predictedSupply >= systemInfo.warningThreshold)
                    {
                    }
                    else
                    {
                    }
                }
            }
        }
    }

    // to destroy the message queue
    msgctl(msgId1, IPC_RMID, NULL);
    msgctl(msgId2, IPC_RMID, NULL);

    return 0;
}