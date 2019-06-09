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
} message1, message2, message3, message4, message5;

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
    D_OFF,
    D_NORMAL,
    D_SAVING
};

enum t_systemStatus
{
    S_OFF,
    S_NORMAL,
    S_WARNING,
    S_OVER
};

int main()
{
    printf("Starting...\n");
    key_t key1, key2, key3, key4;
    int msgId1, msgId2, msgId3, msgId4;
    char messageBuffer[100] = "", infoBuffer[100] = "";
    char *infoToken;
    long msgtype = 1;

    // ftok to generate unique key
    key1 = ftok("keyfile", 1); // to elePowerCtrl
    key2 = ftok("keyfile", 2); // to powerSupplyInfoAccess
    key3 = ftok("keyfile", 3); // from elePowerCtrl
    key4 = ftok("keyfile", 4); // from powerSupplyInfoAccess
    //printf("Success: Getting message queue keys %d %d %d %d\n", key1, key2, key3, key4);

    // msgget creates a message queue
    // and returns identifier
    msgId1 = msgget(key1, 0666 | IPC_CREAT);
    msgId2 = msgget(key2, 0666 | IPC_CREAT);
    msgId3 = msgget(key3, 0666 | IPC_CREAT);
    msgId4 = msgget(key4, 0666 | IPC_CREAT);
    // printf("Success: Getting message ID %d %d %d %d\n", msgId1, msgId2, msgId3, msgId4);

    while (1)
    {
        // msgrcv to receive message
        message1.mesg_type = msgtype;
        memset(message1.mesg_text, 0, sizeof(message1.mesg_text));
        if (msgrcv(msgId1, &message1, sizeof(message1), 1, 0) != -1)
        {
            printf("\nSuccess: Received Message from Connect Manager...\n");
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
            printf("Pending: Getting System Info...\n");
            sprintf(message2.mesg_text, "%d|%d|", T_READ, T_SYSTEM);
            message2.mesg_type = msgtype;
            // msgsnd to send message
            msgsnd(msgId2, &message2, sizeof(message2.mesg_text), 0);
            printf("Success: Sending Message to Power Supply Info Access...\n");

            // receive systemInfo
            message4.mesg_type = msgtype;
            memset(message4.mesg_text, 0, sizeof(message4.mesg_text));
            if (msgrcv(msgId4, &message4, sizeof(message4), 1, 0) != -1)
            {
                printf("Success: Received System Info from Power Supply Info Access...\n");
                memset(infoBuffer, 0, sizeof(infoBuffer));
                strcpy(infoBuffer, message4.mesg_text);
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
            printf("Pending: Getting Device Info...\n");
            sprintf(message2.mesg_text, "%d|%d|%d|", T_READ, T_DEVICE, deviceID);
            message2.mesg_type = msgtype;
            // msgsnd to send message
            msgsnd(msgId2, &message2, sizeof(message2.mesg_text), 0);
            printf("Success: Sending Message to Power Supply Info Access...\n");

            // receive deviceInfo
            message4.mesg_type = msgtype;
            memset(message4.mesg_text, 0, sizeof(message4.mesg_text));
            if (msgrcv(msgId4, &message4, sizeof(message4), 1, 0) != -1)
            {
                printf("Success: Received Device Info from Power Supply Info Access...\n");
                memset(infoBuffer, 0, sizeof(infoBuffer));
                strcpy(infoBuffer, message4.mesg_text);
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
                message3.mesg_type = msgtype;
                sprintf(message3.mesg_text, "%s|%d|%s|", systemInfo.status, equipInfo.currentSupply, equipInfo.status);
                // send message to powerSupply
                msgsnd(msgId3, &message3, sizeof(message3.mesg_text), 0);
                printf("Success: Sending Message to Power Supply...\n");
            }
            else
            {
                int predictedSupply = systemInfo.currentSupply - equipInfo.currentSupply + requestedSupply;

                if (predictedSupply <= systemInfo.maxThreshold)
                {
                    if (predictedSupply >= systemInfo.warningThreshold)
                    {
                        // set system to WARNING state
                        memset(systemInfo.status, 0, sizeof(systemInfo.status));
                        strcpy(systemInfo.status, "WARNING");
                        printf("System: WARNING\n");
                    }
                    else
                    {
                        // set system to NORMAL state
                        memset(systemInfo.status, 0, sizeof(systemInfo.status));
                        strcpy(systemInfo.status, "NORMAL");
                        printf("System: NORMAL\n");
                    }

                    // set device's current supply to requested supply
                    equipInfo.currentSupply = requestedSupply;
                    strcpy(equipInfo.status, requestedMode);
                    // send message to powerSupplyInfoAccess to write SystemInfo
                    sprintf(message2.mesg_text, "%d|%d|%d|%s|", T_WRITE, T_SYSTEM, predictedSupply, systemInfo.status);
                    message2.mesg_type = msgtype;
                    msgsnd(msgId2, &message2, sizeof(message2.mesg_text), 0);
                    printf("Success: Sending Request to Write System Info...\n");
                    // send message to powerSupplyInfoAccess to write deviceInfo
                    sprintf(message2.mesg_text, "%d|%d|%d|%d|%s|", T_WRITE, T_DEVICE, deviceID, equipInfo.currentSupply, equipInfo.status);
                    message2.mesg_type = msgtype;
                    msgsnd(msgId2, &message2, sizeof(message2.mesg_text), 0);
                    printf("Success: Sending Request to Write Device Info...\n");
                    // send message to powerSupply
                    memset(message3.mesg_text, 0, sizeof(message3.mesg_text));
                    message3.mesg_type = msgtype;
                    sprintf(message3.mesg_text, "%s|%d|%s|", systemInfo.status, equipInfo.currentSupply, equipInfo.status);
                    msgsnd(msgId3, &message3, sizeof(message3.mesg_text), 0);
                    printf("Success: Sending Message to Power Supply...\n\n");
                }
                else // predictedSupply > systemInfo.maxThreshold
                {
                    int totalPower = 0;
                    message2.mesg_type = msgtype;
                    memset(message2.mesg_text, 0, sizeof(message2.mesg_text));
                    // reduce every device's power to SAVING mode
                    sprintf(message2.mesg_text, "%d|%d|%d|%d|", T_WRITE, T_DEVICE, deviceID, -1);
                    msgsnd(msgId2, &message2, sizeof(message2.mesg_text), 0);
                    printf("Success: Sending Message to Power Supply Info Access...\n");

                    if (msgrcv(msgId4, &message4, sizeof(message4), 1, 0) != -1)
                    {
                        printf("Success: Received Total Power from Power Supply Info Access...\n");
                        memset(infoBuffer, 0, sizeof(infoBuffer));
                        strcpy(infoBuffer, message4.mesg_text);

                        infoToken = strtok(infoBuffer, "|");
                        totalPower = atoi(infoToken);
                        if (totalPower > systemInfo.maxThreshold) // still OVERLOAD
                        {
                            memset(equipInfo.status, 0, sizeof(equipInfo.status));
                            strcpy(equipInfo.status, "OFF");
                            equipInfo.currentSupply = 0;
                            // sending message to Info Access to turn off device
                            message2.mesg_type = msgtype;
                            memset(message2.mesg_text, 0, sizeof(message2.mesg_text));
                            sprintf(message2.mesg_text, "%d|%d|%d|%d|%s|", T_WRITE, T_DEVICE, deviceID, equipInfo.currentSupply, equipInfo.status);
                            msgsnd(msgId2, &message2, sizeof(message2.mesg_text), 0);
                            printf("Success: Sending Message to Power Supply Info Access...\n");

                            totalPower -= equipInfo.savingVoltage;
                            if (totalPower >= systemInfo.warningThreshold)
                            {
                                // set system to WARNING state
                                memset(systemInfo.status, 0, sizeof(systemInfo.status));
                                strcpy(systemInfo.status, "WARNING");
                                printf("System: WARNING\n");
                            }
                            else
                            {
                                // set system to NORMAL state
                                memset(systemInfo.status, 0, sizeof(systemInfo.status));
                                strcpy(systemInfo.status, "NORMAL");
                                printf("System: NORMAL\n");
                            }

                            // send message to powerSupplyInfoAccess to write SystemInfo
                            sprintf(message2.mesg_text, "%d|%d|%d|%s|", T_WRITE, T_SYSTEM, totalPower, systemInfo.status);
                            message2.mesg_type = msgtype;
                            msgsnd(msgId2, &message2, sizeof(message2.mesg_text), 0);
                            printf("Success: Sending Request to Write System Info...\n");

                            // send message to powerSupply
                            memset(message3.mesg_text, 0, sizeof(message3.mesg_text));
                            message3.mesg_type = msgtype;
                            sprintf(message3.mesg_text, "%s|%d|%s|", "OVER", equipInfo.currentSupply, equipInfo.status);
                            msgsnd(msgId3, &message3, sizeof(message3), 0);
                            printf("Success: Sending Message to Power Supply...\n");
                        }
                        else
                        {
                            if (totalPower >= systemInfo.warningThreshold)
                            {
                                // set system to WARNING state
                                memset(systemInfo.status, 0, sizeof(systemInfo.status));
                                strcpy(systemInfo.status, "WARNING");
                                printf("System: WARNING\n");
                            }
                            else
                            {
                                // set system to NORMAL state
                                memset(systemInfo.status, 0, sizeof(systemInfo.status));
                                strcpy(systemInfo.status, "NORMAL");
                                printf("System: NORMAL\n");
                            }

                            // send message to powerSupplyInfoAccess to write SystemInfo
                            sprintf(message2.mesg_text, "%d|%d|%d|%s|", T_WRITE, T_SYSTEM, totalPower, systemInfo.status);
                            message2.mesg_type = msgtype;
                            msgsnd(msgId2, &message2, sizeof(message2.mesg_text), 0);
                            printf("Success: Sending Request to Write System Info...\n");

                            // send message to powerSupply
                            memset(equipInfo.status, 0, sizeof(equipInfo.status));
                            strcpy(equipInfo.status, "SAVING");
                            equipInfo.currentSupply = equipInfo.savingVoltage;

                            memset(message3.mesg_text, 0, sizeof(message3.mesg_text));
                            message3.mesg_type = msgtype;
                            sprintf(message3.mesg_text, "%s|%d|%s|", "OVER", equipInfo.currentSupply, equipInfo.status);
                            msgsnd(msgId3, &message3, sizeof(message3.mesg_text), 0);
                            printf("Success: Sending Message to Power Supply...\n");
                        }
                    }
                }
            }
        }
    }

    // to destroy the message queue
    msgctl(msgId1, IPC_RMID, NULL);
    msgctl(msgId2, IPC_RMID, NULL);
    msgctl(msgId3, IPC_RMID, NULL);
    msgctl(msgId4, IPC_RMID, NULL);

    return 0;
}