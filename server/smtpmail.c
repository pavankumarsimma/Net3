#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define MAXSIZE 100
void handle_client(int cli_sock, struct sockaddr_in, struct sockaddr_in);
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
		while(wait(NULL)>0) ;
	}
	return 0;
}

void handle_client(cli_sock, cli_addr, serv_addr){
	char buffer[MAX];
	sprintf(buffer, "220 %s:%d Service Ready", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
	send(cli_sock, buffer, sizeof(buffer));
	return;
}