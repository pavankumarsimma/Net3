#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#define MAXSIZE 1000

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

	memset(buffer, '\0', MAXSIZE);
	sprintf(buffer, "220 %s Service Ready\r\n", inet_ntoa(serv_addr.sin_addr));
	n = send(cli_sock, buffer, strlen(buffer), 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("[-] %s\n", buffer);

	memset(buffer, '\0', MAXSIZE);
	n = recv(cli_sock, buffer, MAXSIZE, 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("[+] %s\n", buffer);

	memset(buffer, '\0', MAXSIZE);
	sprintf(buffer, "250 OK Hello %s\r\n", inet_ntoa(serv_addr.sin_addr));
	n = send(cli_sock, buffer, strlen(buffer), 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("[-] %s\n", buffer);

	memset(buffer, '\0', MAXSIZE);
	n = recv(cli_sock, buffer, MAXSIZE, 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("[+] %s\n", buffer);

	char sender[MAXSIZE];
	sscanf(buffer, "MAIL FROM: %s\r\n", sender);
	memset(buffer, '\0', MAXSIZE);
	sprintf(buffer, "250 %s... Sender ok\r\n", sender);
	n = send(cli_sock, buffer, strlen(buffer), 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("[-] %s\n", buffer);

	memset(buffer, '\0', MAXSIZE);
	n = recv(cli_sock, buffer, MAXSIZE, 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("[+] %s\n", buffer);

	char receiver[MAXSIZE];
	sscanf(buffer, "RCPT TO: %s\r\n", receiver);
	memset(buffer, '\0', MAXSIZE);
	sprintf(buffer, "250 root... Recipient ok\r\n");
	n = send(cli_sock, buffer, strlen(buffer), 0);
	if (n<0) {
		perror("send - recv error");
		exit(EXIT_FAILURE);
	}
	printf("[-] %s\n", buffer);


	return;
}
