#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/stat.h>
#define MAXSIZE 1000

int directoryExists(const char *path);
int main(){
	sleep(15);
	/*
	if (directoryExists("./simma") == 0) {
		printf("no dir\n");
	}
	else {
		printf("Dir exists\n");
	}
*/
	return 0;
}


int directoryExists(const char *path) {
    struct stat dirStat;
    if (stat(path, &dirStat) == 0) {
        return S_ISDIR(dirStat.st_mode);
    }
    return 0; // Directory doesn't exist or there was an error
}
