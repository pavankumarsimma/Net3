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

typedef struct mail{
	int number;
	int size;
	int del;
	char data[580];
}mail;

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
    int state = 0;
	printf("Accepted connection from %s.%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
	int status = 220;
	memset(buffer, '\0', MAXSIZE);
	
	state = AUTH;
	sprintf(buffer, "+OK POP3 server ready\r\n");
	n = send(cli_sock, buffer, strlen(buffer), 0);
	printf("S: %s\n", buffer);

	char USER_NAME[MAXSIZE-40];
	char PASSWORD[MAXSIZE-40];
	int msg_count;
	int total_size;
	int ORI_COUNT;
	int ORI_SIZE;
	int stat_c = 0;
	mail * box;
	
	while(1) {
		memset(buffer, '\0', MAXSIZE);
		n = recv(cli_sock, buffer, MAXSIZE, 0);
		printf("C: %s\n", buffer);
		if (n == 0){
			printf("Client disconnected\n");
			break;
		}
		if (n<0) {
			perror("recv error!");
			break;
		}

		char request[MAXSIZE];
		char code[5];
		sscanf(buffer, "%s %s", code, request);
		if (state == AUTH){ 
			if (strncmp(code, "USER", 4)==0 ){
				// USER
				for(int j=5; j<n-2; j++){
					USER_NAME[j-5] = buffer[j];
				}
				FILE* file = fopen("user.txt", "r");
				if (file == NULL) {
					perror("File open error");
					exit(EXIT_FAILURE);
				}
				char line[MAXSIZE];
				int found=0;
				while(fgets(line, MAXSIZE, file)!=NULL){
					char name[MAXSIZE];
					char password[MAXSIZE];
					sscanf(line, "%s %s", name, password);
					if (strcmp(name, USER_NAME) == 0){
						// matched username
						found = 1;
						sprintf(PASSWORD, "%s", password);
						break;
					}
				}
				if (found == 0){
					memset(buffer, '\0', MAXSIZE);
					sprintf(buffer, "-ERR sorry, no mailbox for %s here\r\n", USER_NAME);
					n = send(cli_sock, buffer, strlen(buffer), 0);
					printf("S: %s\n", buffer);
				}
				else {
					memset(buffer, '\0', MAXSIZE);
					sprintf(buffer, "+OK %s is a valid mailbox\r\n", USER_NAME);
					n = send(cli_sock, buffer, strlen(buffer), 0);
					printf("S: %s\n", buffer);
				}
				fclose(file);
			}
			else if( strncmp(buffer, "PASS", 4) == 0){
				// PASS
				char password[MAXSIZE];
				memset(password, '\0', MAXSIZE);
				for(int j=5; j<n-2; j++){
					password[j-5] = buffer[j];
				}
				printf("%s",password);
				if (strcmp(password, PASSWORD) == 0){
					memset(buffer, '\0', MAXSIZE);
					sprintf(buffer, "+OK maildrop locked and ready\r\n");
					n = send(cli_sock, buffer, strlen(buffer), 0);
					printf("S: %s\n", buffer);
					state = TRANS;
				}
				else {
					memset(buffer, '\0', MAXSIZE);
					sprintf(buffer, "-ERR invalid password\r\n");
					n = send(cli_sock, buffer, strlen(buffer), 0);
					printf("S: %s\n", buffer);
				}
			}
			else if (strncmp(buffer, "QUIT", 4) == 0){
				memset(buffer, '\0', MAXSIZE);
				sprintf(buffer, "+OK POP3 server signing off\r\n");
				n = send(cli_sock, buffer, strlen(buffer), 0);
				printf("S: %s\n", buffer);
				break;
			}
		}
		else if( state == TRANS ){
			// transaction state
			
			if (strncmp(code, "STAT", 4) == 0 && stat_c == 0){
				// stat
				char path[MAXSIZE];
				msg_count = 0;
				sprintf(path, "./%s/mymailbox", USER_NAME);
				FILE* file = fopen(path, "r");
				char line[MAXSIZE];
				total_size = 0;
				while( fgets(line, MAXSIZE, file)!=NULL ){
					//printf("%d\n", strlen(line));
					total_size += strlen(line);
					if (strcmp(line, ".\r\n")==0 ){
						msg_count++;
					}
				}
				ORI_COUNT = msg_count;
				ORI_SIZE = total_size;
				//printf("Msgs: %d %d\n", msg_count, total_size);
				fclose(file);

				memset(buffer, '\0', MAXSIZE);
				sprintf(buffer, "+OK %d %d\r\n", msg_count, total_size);
				n = send(cli_sock, buffer, strlen(buffer), 0);
				printf("S: %s\n", buffer);
				stat_c++;
			}
			else if(strncmp(code, "STAT", 4) == 0 && stat_c != 0){
				memset(buffer, '\0', MAXSIZE);
				sprintf(buffer, "+OK %d %d\r\n", msg_count, total_size);
				n = send(cli_sock, buffer, strlen(buffer), 0);
				printf("S: %s\n", buffer);
			}
			else if (strncmp(code, "LIST", 4) == 0){
				box = (mail*) calloc( (msg_count), sizeof(mail));

				char path[MAXSIZE];
				sprintf(path, "./%s/mymailbox", USER_NAME);
				FILE* file = fopen(path, "r");
				char line[MAXSIZE];
				int count=0;
				int index=0;
				while( fgets(line, MAXSIZE, file)!=NULL ){
					box[count].size += strlen(line);
					box[count].del = 0;
					box[count].number = count;
					sprintf(&box[count].data[index], "%s", line);
					index +=  strlen(line);
					if (strncmp(line, ".\r\n", 3)==0 ){
						count++;
						index=0;
					}
				}
				fclose(file);
				
				if ( n>=7 ){
					// particular message size
					int number;
					sscanf(buffer, "LIST %d\r\n", &number);
					if (number >= msg_count){
						memset(buffer, '\0', MAXSIZE);
						sprintf(buffer, "-ERR no such message, only %d messages in maildrop\r\n", msg_count);
						n = send(cli_sock, buffer, strlen(buffer), 0);
						printf("S: %s\n", buffer);
					}
					else {
						for (int i=0; i<msg_count; i++){
							if (box[i].number == number){
								memset(buffer, '\0', MAXSIZE);
								sprintf(buffer, "+OK %d %d\r\n", number, box[i].size);
								n = send(cli_sock, buffer, strlen(buffer), 0);
								printf("S: %s\n", buffer);
								break;
							}
						}
					}
				}
				else {
					// all messages size
					memset(buffer, '\0', MAXSIZE);
					sprintf(buffer, "+OK %d messages (%d octets)\r\n", msg_count, total_size);
					n = send(cli_sock, buffer, strlen(buffer), 0);
					printf("S: %s\n", buffer);
					for (int i=0; i<msg_count; i++){
						memset(buffer, '\0', MAXSIZE);
						sprintf(buffer, "%d %d\r\n", box[i].number, box[i].size);
						n = send(cli_sock, buffer, strlen(buffer), 0);
						printf("S: %s\n", buffer);
					}
					memset(buffer, '\0', MAXSIZE);
					sprintf(buffer, ".\r\n");
					n = send(cli_sock, buffer, strlen(buffer), 0);
					printf("S: %s\n", buffer);
				}
			}
			else if (strncmp(code, "RETR", 4) == 0){
				int number;
				sscanf(buffer, "RETR %d\r\n", &number);
				if (number >= msg_count){
					memset(buffer, '\0', MAXSIZE);
					sprintf(buffer, "-ERR no such message\r\n");
					n = send(cli_sock, buffer, strlen(buffer), 0);
					printf("S: %s\n", buffer);
				}
				else {
					for(int i=0; i<ORI_COUNT; i++){
						if (box[i].number == number && box[i].del == 0){
							memset(buffer, '\0', MAXSIZE);
							sprintf(buffer, "+OK %d octets\r\n", box[i].size);
							n = send(cli_sock, buffer, strlen(buffer), 0);
							printf("S: %s\n", buffer);
							printf("S: ");
							for (int j=0; j<box[i].size; j++){
								memset(buffer, '\0', MAXSIZE);
								sprintf(buffer, "%c", box[i].data[j]);
								n = send(cli_sock, buffer, strlen(buffer), 0);
								printf("%c", buffer[0]);
							}
						}
					}
				}
			}
			else if (strncmp(code, "DELE", 4) == 0){
				int number;
				sscanf(buffer, "DELE %d\r\n", &number);
				if (number >= msg_count){
					memset(buffer, '\0', MAXSIZE);
					sprintf(buffer, "-ERR no such message\r\n");
					n = send(cli_sock, buffer, strlen(buffer), 0);
					printf("S: %s\n", buffer);
				}
				else {
					for(int i=0; i<ORI_COUNT; i++){
						if (box[i].number == number){
							memset(buffer, '\0', MAXSIZE);
							sprintf(buffer, "+OK message %d deleted\r\n", number);
							n = send(cli_sock, buffer, strlen(buffer), 0);
							printf("S: %s\n", buffer);
							box[i].del = 1;
							total_size -= box[i].size;
						}
						else if (box[i].number > number){
							box[i].number--;
						}
					}
					msg_count--;
				}
			}
			else if (strncmp(code, "RSET", 4) == 0){
				for (int i=0; i<ORI_COUNT; i++){
					box[i].del = 0;
					box[i].number = i;
				}
				msg_count = ORI_COUNT;
				total_size = ORI_SIZE;
				memset(buffer, '\0', MAXSIZE);
				sprintf(buffer, "+OK maildrop has %d messages (%d octets)\r\n", msg_count, total_size);
				n = send(cli_sock, buffer, strlen(buffer), 0);
				printf("S: %s\n", buffer);
			}
			else if (strncmp(code, "QUIT", 4) == 0){
				memset(buffer, '\0', MAXSIZE);
				sprintf(buffer, "+OK POP3 update state\r\n");
				n = send(cli_sock, buffer, strlen(buffer), 0);
				printf("S: %s\n", buffer);
				state = UPDATE;
			}
			else {
				memset(buffer, '\0', MAXSIZE);
				sprintf(buffer, "-ERR NO Command\r\n");
				n = send(cli_sock, buffer, strlen(buffer), 0);
				printf("S: %s\n", buffer);
				continue;
			}
		}
		else if (state == UPDATE){
			// update
			if (strncmp(code, "QUIT", 4) == 0){
				char path[MAXSIZE];
				sprintf(path, "./%s/mymailbox", USER_NAME);

				int fd = open(path, O_WRONLY | O_TRUNC, 0777);
				if (fd == -1){
					perror("file open error");
					memset(buffer, '\0', MAXSIZE);
					sprintf(buffer, "-ERR some deleted messages not removed\r\n");
					n = send(cli_sock, buffer, strlen(buffer), 0);
					printf("S: %s\n", buffer);
					break;
				}
				int fg = 0;
				for(int i=0; i<ORI_COUNT; i++){
					fg = 0;
					if ( box[i].del == 0){
						for (int j=0; j<box[i].size; j++){
							int m = write(fd, &box[i].data[j], 1);
							if (m < 0){
								memset(buffer, '\0', MAXSIZE);
								sprintf(buffer, "-ERR some deleted messages not removed\r\n");
								n = send(cli_sock, buffer, strlen(buffer), 0);
								printf("S: %s\n", buffer);
								fg = 1;
								break;
							}
						}
					}
					if (fg == 1) {
						break;
					}
				}
				if (fg == 1) {
					close(fd);
					break;
				}
				close(fd);
				memset(buffer, '\0', MAXSIZE);
				sprintf(buffer, "+OK POP3 server signing off\r\n");
				n = send(cli_sock, buffer, strlen(buffer), 0);
				printf("S: %s\n", buffer);
				break;
			}
			else {
				memset(buffer, '\0', MAXSIZE);
				sprintf(buffer, "-ERR NO Command\r\n");
				n = send(cli_sock, buffer, strlen(buffer), 0);
				printf("S: %s\n", buffer);
				continue;
			}
		}

	}
	free(box);
    printf("Connection closed!!\n");
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