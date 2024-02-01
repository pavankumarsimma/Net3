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
	printf("Server listening ...\n");
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
	sprintf(buffer, "%d %s Service Ready\r\n", status, getLocalIP());
	n = send(cli_sock, buffer, strlen(buffer), 0);
	if (n<0) {
		perror("send error");
		exit(EXIT_FAILURE);
	}
	printf("S:  %s\n", buffer);
	char * localIP = getLocalIP();
	int flag=1;
	char SEND_USERNAME[MAXSIZE];
	char RECV_USERNAME[MAXSIZE];
	while(flag == 1){
		// request from client
		memset(buffer, '\0', MAXSIZE);
		n = recv(cli_sock, buffer, MAXSIZE, 0);
		if (n<0) {
			perror("recv error");
			exit(EXIT_FAILURE);
		}
		if (n==0) {
			printf("Client disconnected\n");
			exit(EXIT_FAILURE);
		}
		printf("C: %s\n", buffer);

		// response to client
		if (strncmp(buffer, "QUIT", 4)==0){
			memset(buffer, '\0', MAXSIZE);
			status = 221;
			sprintf(buffer, "%d %s closing connection\r\n", status, localIP);
			n = send(cli_sock, buffer, strlen(buffer), 0);
			if (n<0) {
				perror("send error");
				exit(EXIT_FAILURE);
			}
			printf("S:  %s\n", buffer);
			flag = 0;
		}
		else if (strncmp(buffer, "HELO", 4)==0){
			char ip[MAXSIZE];
			strcpy(ip, &buffer[5]);
			char msg[MAXSIZE-15];
			strcpy(msg, buffer);
			if ( strncmp(ip, localIP, strlen(localIP)) == 0){
				// ip ok
				memset(buffer, '\0', MAXSIZE);
				status = 250;
				sprintf(buffer, "%d OK %s\r\n", status, msg);
				n = send(cli_sock, buffer, strlen(buffer), 0);
				if(n <0 ){
					perror("send error");
					exit(EXIT_FAILURE);
				}
				printf("S: %s\n", buffer);
			}
			else {
				// wrong server address
				status = 600;
				memset(buffer, '\0', MAXSIZE);
				sprintf(buffer, "%d Wrong server address\r\n", status);
				n = send(cli_sock, buffer, strlen(buffer), 0);
				if (n<0) {
					perror("send error");
					exit(EXIT_FAILURE);
				}
				printf("S: %s\n", buffer);
			}
		}
		else if (strncmp(buffer, "DATA", 4) == 0){
			// DATA
			memset(buffer, '\0', MAXSIZE);
			status = 354;
			sprintf(buffer, "%d Enter mail, end with \".\" on a line by itself\r\n", status);
			n = send(cli_sock, buffer, strlen(buffer), 0);
			if (n<0) {
				perror("send error");
				exit(EXIT_FAILURE);
			}
			printf("S: %s\n", buffer);
			int count=0;
			while(1){
				memset(buffer, '\0', MAXSIZE);
				n = recv(cli_sock, buffer, MAXSIZE, 0);
				if (n<0) {
					perror("recv error");
					exit(EXIT_FAILURE);
				}
				if (n==0){
					printf("Client disconnected\n");
					exit(EXIT_FAILURE);
				}
				printf("C: %s\n", buffer);

				// file handling
				char filename[MAXSIZE+12];
				sprintf(filename, "./%s/mymailbox", RECV_USERNAME);
				int fd = open(filename, O_RDWR | O_CREAT | O_APPEND, 0666);
				int bytes_write;
				
				int msg_end=0;
				for(int i=0; i<n; i++){
					if(count==3){
						char received[80];
						memset(received, '\0', 80);
						time_t rawtime;
						struct tm *timeinfo;
						time(&rawtime);
						timeinfo = localtime(&rawtime);
						strftime(received, 80, "Received: %d : %H : %M\r\n", timeinfo);
						//printf("%s", received);
						write(fd, received, strlen(received));
						count=1000;
					}
					if (i>=1) {
						if ( buffer[i]=='\n' && buffer[i-1]=='\r' ){
							count++;
						}
					}
					bytes_write = write(fd, &buffer[i], 1);
					if (bytes_write == -1){
						perror("mailbox error\n");
						close(fd);
						close(cli_sock);
						exit(EXIT_FAILURE);
					}
					if ( (i-2>=0 && i+2<=n-1) && (buffer[i-2]=='\r' && buffer[i-1]=='\n') && buffer[i]=='.' && (buffer[i+1]=='\r' && buffer[i+2]=='\n') ){
						
						msg_end=1;
						write(fd, &buffer[i+1], strlen(&buffer[i+1]));
						memset(buffer, '\0', MAXSIZE);
						status = 250;
						sprintf(buffer, "%d OK Message accepted for delivery\r\n", status);
						n = send(cli_sock, buffer, strlen(buffer), 0);
						if (n<0) {
							perror("send error");
							exit(EXIT_FAILURE);
						}
						printf("S: %s\n", buffer);
						break;
					}
				}

				if (msg_end == 1){
					close(fd);
					break;
				}
				
			}
		}
		else if (strncmp(buffer, "MAIL", 4) == 0){
			// from address
			char from[MAXSIZE-28];
			strcpy(from, &buffer[11]);
			int ff = 0;
			int j=0;
			for (int i=0; i<strlen(from); i++){
				if (from[0]=='@'){
					break;
				}
				if (from[i] == '@'){
					if (ff==0) {
						ff = i;
					}
					else {
						ff = 0;
						break;
					}
				}
				if (from[i] == '\r'){
					j = i;
					break;
				}
			}
			if (ff==0){
				// from address format wrong
				status = 600;
				memset(buffer, '\0', MAXSIZE);
				sprintf(buffer, "%d address format wrong\r\n", status);
				n = send(cli_sock, buffer, strlen(buffer), 0);
				if (n<0) {
					perror("send error");
					exit(EXIT_FAILURE);
				}
				printf("S: %s\n", buffer);
			}
			else {
				// correct format of from address
				char path[MAXSIZE+2];
				char user[MAXSIZE];
				sscanf(buffer, "MAIL FROM: %s\r\n", path);
				strcpy(from, path);
				memset(user, '\0', MAXSIZE);
				for (int i=0; i<strlen(path); i++){
					if (path[i] == '@') break;
					user[i] = path[i];
				}
				// printf("%s\n", user);
				sprintf(path, "./%s", user);
				strcpy(SEND_USERNAME, user);
				if (directoryExists(path) == 0){
					//  user doen't exist
					memset(buffer, '\0', MAXSIZE);
					status = 550;
					sprintf(buffer, "%d No such user\r\n", status);
					n = send(cli_sock, buffer, strlen(buffer), 0);
					if (n < 0){
						perror("send error");
						exit(EXIT_FAILURE);
					}
					printf("S: %s\n", buffer);
				}
				else {
					memset(buffer, '\0', MAXSIZE);
					status = 250;
					sprintf(buffer, "%d %s... Sender ok\r\n", status, from);
					n = send(cli_sock, buffer, strlen(buffer), 0);
					if (n < 0){
						perror("send error");
						exit(EXIT_FAILURE);
					}
					printf("S: %s\n", buffer);
				}
			}
		}	
		else if (strncmp(buffer, "RCPT", 4) == 0){
			// to address
			char to[MAXSIZE];
			strcpy(to, &buffer[9]);
			int ff = 0;
			int j=0;
			for (int i=0; i<strlen(to); i++){
				if (to[0]=='@'){
					break;
				}
				if (to[i] == '@'){
					if (ff==0) {
						ff = i;
					}
					else {
						ff = 0;
						break;
					}
				}
				if (to[i] == '\r'){
					j = i;
					break;
				}
			}
			if (ff==0){
				// from address format wrong
				status = 600;
				memset(buffer, '\0', MAXSIZE);
				sprintf(buffer, "%d address format wrong\r\n", status);
				n = send(cli_sock, buffer, strlen(buffer), 0);
				if (n<0) {
					perror("send error");
					exit(EXIT_FAILURE);
				}
				printf("S: %s\n", buffer);
			}
			else {
				// correct format of from address
				char path[MAXSIZE+2];
				char user[MAXSIZE];
				sscanf(buffer, "RCPT TO: %s\r\n", path);
				strcpy(to, path);
				memset(user, '\0', MAXSIZE);
				for (int i=0; i<strlen(path); i++){
					if (path[i] == '@') break;
					user[i] = path[i];
				}
				//printf("%s\n", user);
				sprintf(path, "./%s", user);
				strcpy(RECV_USERNAME, user);
				if (directoryExists(path) == 0){
					//  user doen't exist
					memset(buffer, '\0', MAXSIZE);
					status = 550;
					sprintf(buffer, "%d No such user\r\n", status);
					n = send(cli_sock, buffer, strlen(buffer), 0);
					if (n < 0){
						perror("send error");
						exit(EXIT_FAILURE);
					}
					printf("S: %s\n", buffer);
				}
				else {
					memset(buffer, '\0', MAXSIZE);
					status = 250;
					sprintf(buffer, "%d root... Recipient ok\r\n", status);
					n = send(cli_sock, buffer, strlen(buffer), 0);
					if (n < 0){
						perror("send error");
						exit(EXIT_FAILURE);
					}
					printf("S: %s\n", buffer);
				}
			}
		}
		else {
			memset(buffer, '\0', MAXSIZE);
			status = 600;
			sprintf(buffer, "%d command not found\r\n", status);
			n = send(cli_sock, buffer, strlen(buffer), 0);
			if (n<0) {
				perror("send error");
				exit(EXIT_FAILURE);
			}
			printf("S:  %s\n", buffer);
		}
	}

	
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

/*

	memset(buffer, '\0', MAXSIZE);
	n = recv(cli_sock, buffer, MAXSIZE, 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("C:  %s\n", buffer);

	memset(buffer, '\0', MAXSIZE);
	sprintf(buffer, "250 OK Hello %s\r\n", inet_ntoa(serv_addr.sin_addr));
	n = send(cli_sock, buffer, strlen(buffer), 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("S:  %s\n", buffer);

	memset(buffer, '\0', MAXSIZE);
	n = recv(cli_sock, buffer, MAXSIZE, 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("C:  %s\n", buffer);

	char sender[MAXSIZE];
	sscanf(buffer, "MAIL FROM: %s\r\n", sender);
	memset(buffer, '\0', MAXSIZE);
	sprintf(buffer, "250 %s... Sender ok\r\n", sender);
	n = send(cli_sock, buffer, strlen(buffer), 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("S:  %s\n", buffer);

	memset(buffer, '\0', MAXSIZE);
	n = recv(cli_sock, buffer, MAXSIZE, 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("C:  %s\n", buffer);

	char receiver[MAXSIZE];
	sscanf(buffer, "RCPT TO: %s\r\n", receiver);
	memset(buffer, '\0', MAXSIZE);
	sprintf(buffer, "250 root... Recipient ok\r\n");
	n = send(cli_sock, buffer, strlen(buffer), 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("S:  %s\n", buffer);

*/
