#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define MAXSIZE 1000
int main(){
    struct sockaddr_in serv_addr;

    int cli_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (cli_sock == -1){
        perror("Socket creation Failed");
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(3400);
    int x = connect(cli_sock, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (x == -1){
        perror("Connect error");
        exit(EXIT_FAILURE);
    }
    char buffer[MAXSIZE];
    printf("Connected to server\n");

    recv(cli_sock, buffer, MAXSIZE, 0);
    printf("[+] %s\n", buffer);

    sprintf(buffer, "HELO %s.%d", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
    send(cli_sock, buffer, sizeof(buffer), 0);
    printf("[-] %s\n", buffer);

    recv(cli_sock, buffer, MAXSIZE, 0);
    printf("[+] %s\n", buffer);

    
    return 0;
}