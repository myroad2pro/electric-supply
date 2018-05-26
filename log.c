#include <stdio.h>
#include <time.h>
#include <string.h>
int id=1;

void init() {
   FILE *fp;
   fp = fopen("log.txt", "w");
   fprintf(fp, "STT\t\t\tTime\t\t\t\tName\tAction\n");
   fclose(fp);
}

void writeLog(char* param1, char* param2) {
	//time
	time_t rawtime;
  	struct tm * timeinfo;
  	time ( &rawtime );
  	timeinfo = localtime ( &rawtime );
  	char *dateTime = asctime(timeinfo);
	dateTime[strlen(dateTime) - 1] = 0;
  	//append
	FILE *fp;
   	fp = fopen("log.txt", "a");
   	fprintf(fp, "%d\t%s\t%s\t%s\n", id, dateTime, param1, param2);
   	fclose(fp);
   	id++;
}
int main() {
	init();
   	writeLog("name","action");
   	writeLog("name","action");

   	writeLog("name","action");
   	writeLog("name","action");


  return 0;
}