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

typedef struct
{
	char name[100];
	int normalMode;
	int savingMode;
} device;

enum mode
{
	OFF,
	NORMAL,
	SAVING
};

device *deviceList;
int N;
int clientSocket;
char serverResponse[MAXLINE];
int *shm;
char *shm2;
char info[1000];
char systemStatus[10], equipStatus[10];

int kbhit();
int getch();
void showMenuDevices();
void showMenu();
void getResponse();
void makeCommand(char *command, char *code, char *param1, char *param2);
void showMenuAction(int deviceID);
void getShareMemoryPointer(char *key_from_server);
void runDevice(int deviceID, int isSaving);
void stopDevice(char *deviceName);
void getInfo(char *key_from_server);
int countEntityNumber(char *str);
char **tokenizeString(char *str);
device parseStringToStruct(char *str);
device *parseStringToStructArray(char *str);

int main()
{
	// khởi tạo kết nối IP
	struct sockaddr_in serverAddr;
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0)
	{
		printf("[-]Error in connection.\n");
		exit(1);
	}

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Connected to Server.\n");

	while (1)
	{
		showMenuDevices();
	}

	return 0;
}

void showMenuDevices()
{
	int choice;
	char c;
	while (1)
	{
		choice = 0;
		printf("-----Welcome-----\n");
		printf("Please choose type of device to connect\n\n");
		int i;
		printf("1. TV\n");
		printf("2. AR\n");
		printf("3. PC\n");
		printf("4. IRON\n");
		printf("5. LIGHT\n");
		printf("6. Quit\n\n");
		printf("-----------------\n");
		printf("Your choice: \n");
		while (choice == 0)
		{
			if (scanf("%d", &choice) < 1)
			{
				choice = 0;
			}
			if (choice < 1 || choice > 6)
			{
				choice = 0;
				printf("Invalid choice!\n");
				printf("Enter again: ");
			}
			while ((c = getchar()) != '\n')
				;
		}
		if (1 <= choice && choice <= 6)
		{
			showMenuAction(choice);
		}
		else
		{
			exit(0);
		}
	}
}

void showMenuAction(int deviceID)
{
	int choice;
	char c;
	while (1)
	{
		choice = 0;
		printf("Please choose an action:\n");
		printf("1. Run at default mode \n");
		printf("2. Run at saving mode\n");
		printf("3. Turn off and quit\n");
		printf("Your choice: ");
		while (choice == 0)
		{
			if (scanf("%d", &choice) < 1)
			{
				choice = 0;
			}
			if (choice < 1 || choice > 4)
			{
				choice = 0;
				printf("Invalid choice!\n");
				printf("Enter again: ");
			}
			while ((c = getchar()) != '\n')
				;
		}

		switch (choice)
		{
		case 1:
			runDevice(deviceID - 1, NORMAL);
			break;
		case 2:
			runDevice(deviceID - 1, SAVING);
			break;
		default:
			runDevice(deviceID - 1, OFF);
		}
	}
}
void getResponse()
{
	int n = recv(clientSocket, serverResponse, MAXLINE, 0);
	if (n == 0)
	{
		perror("The server terminated prematurely");
		exit(4);
	}
	serverResponse[n] = '\0';
}

int kbhit()
{
	struct timeval tv = {0L, 0L};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

int getch()
{
	int r;
	unsigned char c;
	if ((r = read(0, &c, sizeof(c))) < 0)
	{
		return r;
	}
	else
	{
		return c;
	}
}

void makeCommand(char *command, char *code, char *param1, char *param2)
{
	strcpy(command, code);
	strcat(command, "|");
	if (param1 != NULL)
	{
		strcat(command, param1);
		strcat(command, "|");
	}
	if (param2 != NULL)
	{
		strcat(command, param2);
		strcat(command, "|");
	}
}

void runDevice(int deviceID, int mode)
{
	char command[100];
	char response[MAXLINE];
	char buffer[20];
	char param[20];
	int countDown = 10;
	char deviceName[100];
	char *token;
	sprintf(deviceName, "%d", deviceID);
	int voltage;

	switch (mode)
	{
	case OFF:
		strcat(deviceName, "|OFF|");
		break;
	case NORMAL:
		strcat(deviceName, "|NORMAL|");
		break;
	case SAVING:
		strcat(deviceName, "|SAVING|");
		break;
	default:
		break;
	}

	if (mode == OFF)
	{
		stopDevice(deviceName);
	}
	else if (mode == NORMAL || mode == SAVING)
	{
		makeCommand(command, "ON", deviceName, buffer);
		send(clientSocket, command, strlen(command), 0);
		getResponse();
	}

	memset(response, 0, sizeof(response));
	strcpy(response, serverResponse);
	token = strtok(response, "|");
	strcpy(systemStatus, token);
	token = strtok(NULL, "|");
	voltage = atoi(token);
	token = strtok(NULL, "|");
	strcpy(equipStatus, token);

	if (strcmp(systemStatus, "OVER") == 0)
	{
		if (strcmp(equipStatus, "OFF") == 0)
		{
			while (countDown > 0)
			{
				printf("System is overloaded.\nThe device will be turn off in %d seconds.\n", countDown);
				countDown--;
				if (countDown == 0)
				{
					stopDevice(deviceName);
					break;
				}
			}
		}
	}
	else
	{
		printf("System is %s.\nThe current device is running %s mode at %d W\n", systemStatus, equipStatus, voltage);
	}
	// while (1)
	// {
	// 	if (*shm <= threshold)
	// 	{
	// 		printf("The current device is running at %d W\n Press enter to stop this device\n", voltage);
	// 	}
	// 	else if (*shm <= maxThreshold)
	// 	{
	// 		printf("The threshold is exceeded. The supply currently is %d\n", *shm);
	// 	}
	// 	else
	// 	{
	// 		printf("Maximum threshold is exceeded. A device will be turn off in %d\n", countDown);
	// 		countDown--;
	//
	// 	}

	// 	if (kbhit())
	// 	{
	// 		stopDevice(deviceName);
	// 		break;
	// 	}
	// 	sleep(1);
	// }
}

void stopDevice(char *deviceName)
{
	char command[100];
	makeCommand(command, "STOP", deviceName, NULL);
	send(clientSocket, command, strlen(command), 0);
	getResponse();
}

int countEntityNumber(char *str)
{
	int i;
	int count = 0;
	for (i = 0; i < strlen(str); ++i)
	{
		if (str[i] == ',')
		{
			count++;
		}
	}
	return count;
}

char **tokenizeString(char *str)
{
	int count = countEntityNumber(str);
	char **tokenArray;
	tokenArray = (char **)malloc(count * sizeof(char *));
	char *dup = strdup(str);
	char *token;
	int i;
	token = strtok(dup, ",");
	tokenArray[0] = token;
	for (i = 1; i < count; ++i)
	{
		tokenArray[i] = strtok(NULL, ",");
	}
	return tokenArray;
}

device parseStringToStruct(char *str)
{
	device hanhvl;
	char *dup = strdup(str);
	char *token;
	int i;
	token = strtok(dup, "|");
	strcpy(hanhvl.name, token);
	token = strtok(NULL, "|");
	hanhvl.normalMode = atoi(strdup(token));
	token = strtok(NULL, "|");
	hanhvl.savingMode = atoi(strdup(token));
	return hanhvl;
}

device *parseStringToStructArray(char *str)
{
	int count = countEntityNumber(str);
	char *dup = strdup(str);
	device *hanhvl;
	hanhvl = (device *)malloc(count * sizeof(device));
	int i;
	for (i = 0; i < count; ++i)
	{
		hanhvl[i] = parseStringToStruct(tokenizeString(dup)[i]);
	}
	return hanhvl;
}
