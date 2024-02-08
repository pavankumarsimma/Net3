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
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#define MAXSIZE 1000

#define AUTH 1
#define TRANS 2
#define UPDATE 3

char *getLocalIP();
int directoryExists(const char *path);
void handle_client(int cli_sock, struct sockaddr_in cli_addr, struct sockaddr_in serv_addr);
int main(int argc, char* argv[]){
	int my_port = 0;
	if (argc==1) {
		printf("Usage: ./a.out <PORT>");
		exit(0);
	}
	my_port = atoi(argv[1]);
	
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in serv_addr, cli_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(my_port);
	// bind 
	if ( bind(server_socket, (const struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		perror("Binding error");
		exit(EXIT_FAILURE);
	}  
	
	// server will listen for the incoming connetions
	int x = listen(server_socket, 5);
	if (x==-1) {
		perror("Listen Error");
		exit(EXIT_FAILURE);
	}
	printf("Server listening on port %d ...\n", my_port);
	while (1) {
		int len = sizeof(cli_addr);
		int new_socket = accept(server_socket, (struct sockaddr*)&cli_addr, &len);
		if (new_socket == -1) {
			perror("accept error");
			exit(EXIT_FAILURE);
		}
		// fork a child process
		int pid = fork();
		if (pid == 0){
			// child
			close(server_socket);
			// handle the client
			handle_client(new_socket, cli_addr, serv_addr);
			
			close(new_socket);
			exit(EXIT_SUCCESS);
		}
		close(new_socket);
		while(waitpid(pid, NULL, 0)>0) ;
	}
	return 0;
}

void handle_client(int cli_sock, struct sockaddr_in cli_addr, struct sockaddr_in serv_addr){
	char buffer[MAXSIZE];
	int n;
	printf("Accepted connection from %s.%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
	int status = 220;
	memset(buffer, '\0', MAXSIZE);
	int state = AUTH;
    printf("State: %d", state);
	return;
}



char *getLocalIP() {
    char buffer[1024];
    gethostname(buffer, sizeof(buffer));
    struct hostent *host = gethostbyname(buffer);
    return inet_ntoa(*((struct in_addr *)host->h_addr_list[0]));
}

int directoryExists(const char *path) {
    struct stat dirStat;
    if (stat(path, &dirStat) == 0) {
        return S_ISDIR(dirStat.st_mode);
    }
    return 0; // Directory doesn't exist or there was an error
}
