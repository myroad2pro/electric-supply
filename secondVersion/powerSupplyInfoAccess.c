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

int main()
{
    key_t key1, key2;
    int msgId1, msgId2;
    int deviceID;
    FILE *fp = NULL;
    char messageBuffer[100] = "", infoBuffer[100] = "";
    char *infoToken, *messageToken;;
    t_systemInfo systemInfo;
    t_deviceInfo deviceInfo;

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
                    memset(infoBuffer, 0, sizeof(infoBuffer));
                    fgets(infoBuffer, 100, fp);

                    memset(message2.mesg_text, 0, sizeof(message2.mesg_text));
                    strcpy(message2.mesg_text, infoBuffer);
                    msgsnd(msgId2, &message2, sizeof(message2), 0);
                    fclose(fp);
                    fp = NULL;
                }
                else if (infoType == T_DEVICE)
                {
                    messageToken = strtok(NULL, "|");
                    deviceID = atoi(messageToken);
                    fp = fopen("deviceInfo", "r");
                    int i;
                    for (i = 0; i <= deviceID; i++)
                    {
                        memset(infoBuffer, 0, sizeof(infoBuffer));
                        fgets(infoBuffer, 100, fp);
                    }

                    memset(message2.mesg_text, 0, sizeof(message2.mesg_text));
                    strcpy(message2.mesg_text, infoBuffer);
                    msgsnd(msgId2, &message2, sizeof(message2), 0);
                    fclose(fp);
                    fp = NULL;
                }
            }
            else if (accessType = T_WRITE)
            {
                if (infoType == T_SYSTEM)
                {
                    // read from message
                    messageToken = strtok(NULL, "|");
                    systemInfo.currentSupply = atoi(messageToken);
                    messageToken = strtok(NULL, "|");
                    strcpy(systemInfo.status, messageToken);

                    // read from systemInfo
                    fp = fopen("sysInfo", "r");
                    memset(infoBuffer, 0, sizeof(infoBuffer));
                    fgets(infoBuffer, 100, fp);
                    // extract info
                    infoToken = strtok(infoBuffer, "|");
                    systemInfo.warningThreshold = atoi(infoToken);
                    infoToken = strtok(NULL, "|");
                    systemInfo.maxThreshold = atoi(infoToken);
                    // reopen
                    freopen("sysInfo", "w", fp);
                    fprintf(fp, "%d|%d|%d|%s|", systemInfo.warningThreshold, systemInfo.maxThreshold, systemInfo.currentSupply, systemInfo.status);
                    fclose(fp);
                    fp = NULL;
                }
                else if (infoType = T_DEVICE)
                {
                    // read from message
                    messageToken = strtok(NULL, "|");
                    deviceID = atoi(messageToken);
                    messageToken = strtok(NULL, "|");
                    deviceInfo.currentSupply = atoi(messageToken);
                    messageToken = strtok(NULL, "|");
                    strcpy(deviceInfo.status, messageToken);

                    // copy contents from deviceInfo to temp
                    FILE *fin = fopen("deviceInfo", "r");
                    FILE *fout = fopen("temp", "w");
                    int lineNo = 0;
                    while (!feof(fin))
                    {
                        memset(infoBuffer, 0, sizeof(infoBuffer));
                        fgets(infoBuffer, 100, fin);

                        if (lineNo == deviceID)
                        {   
                            char deviceName[100];
                            infoToken = strtok(infoBuffer, "|");
                            strcpy(deviceName, infoToken);
                            infoToken = strtok(NULL, "|");
                            deviceInfo.normalVoltage = atoi(infoToken);
                            infoToken = strtok(NULL, "|");
                            deviceInfo.savingVoltage = atoi(infoToken);
                            fprintf(fout, "%s|%d|%d|%s|\n", deviceName, deviceInfo.normalVoltage, deviceInfo.savingVoltage, deviceInfo.status);
                        }else{
                            fprintf(fout, "%s", infoBuffer);
                        }
                        lineNo++;
                    }
                    fclose(fin);
                    fin = NULL;
                    fclose(fout);
                    fout = NULL;
                    remove("deviceInfo");
                    rename("temp", "deviceInfo");
                }
            }
            free(messageToken);
            messageToken = NULL;
            free(infoToken);
            infoToken = NULL;
        }
    }

    // to destroy the message queue
    msgctl(msgId2, IPC_RMID, NULL);

    return 0;
}