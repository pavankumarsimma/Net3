#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define MAXSIZE 100

char *getLocalIP();
int main(){
    struct sockaddr_in serv_addr;
    int n;

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

    memset(buffer, '\0', MAXSIZE);
    n = recv(cli_sock, buffer, MAXSIZE, 0);
    if (n<0) {
        perror("send - recv error");
        exit(EXIT_FAILURE);
    }
    printf("S:  %s\n", buffer);

    memset(buffer, '\0', MAXSIZE);
    sprintf(buffer, "HELO %s.%d\r\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
    n = send(cli_sock, buffer, strlen(buffer), 0);
    if (n<0) {
        perror("send - recv error");
        exit(EXIT_FAILURE);
    }
    printf("C:  %s\n", buffer);

    memset(buffer, '\0', MAXSIZE);
    n = recv(cli_sock, buffer, MAXSIZE, 0);
    if (n<0) {
        perror("send - recv error");
        exit(EXIT_FAILURE);
    }
    printf("S:  %s\n", buffer);

    memset(buffer, '\0', MAXSIZE);
    sprintf(buffer, "MAIL FROM: %s\r\n", getLocalIP());
    n = send(cli_sock, buffer, strlen(buffer), 0);
    if (n<0) {
        perror("send - recv error");
        exit(EXIT_FAILURE);
    }
    printf("C:  %s\n", buffer);

    memset(buffer, '\0', MAXSIZE);
    n = recv(cli_sock, buffer, MAXSIZE, 0);
    if (n<0) {
        perror("send - recv error");
        exit(EXIT_FAILURE);
    }
    printf("S:  %s\n", buffer);

    char to[MAXSIZE];
    printf("Enter the receipient address: ");
    scanf("%s", to);
    memset(buffer, '\0', MAXSIZE);
    sprintf(buffer, "RCPT TO: %s\r\n", to);
    n = send(cli_sock, buffer, strlen(buffer), 0);
    if (n<0) {
        perror("send - recv error");
        exit(EXIT_FAILURE);
    }
    printf("C:  %s\n", buffer);

    memset(buffer, '\0', MAXSIZE);
    n = recv(cli_sock, buffer, MAXSIZE, 0);
    if (n<0) {
        perror("send - recv error");
        exit(EXIT_FAILURE);
    }
    printf("S:  %s\n", buffer);

    return 0;
}

char *getLocalIP() {
    char buffer[1024];
    gethostname(buffer, sizeof(buffer));
    struct hostent *host = gethostbyname(buffer);
    return inet_ntoa(*((struct in_addr *)host->h_addr_list[0]));
}