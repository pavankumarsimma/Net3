#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define MAXSIZE 400
#define LINESIZE 80
char *getLocalIP();
int main(int argc, char* argv[]){
    struct sockaddr_in serv_addr;
    int n;

    if (argc != 4){
        printf("Usage: ./a.out server_ip smtp_port pop3_port\n");
        exit(EXIT_FAILURE);
    }

    int option = 0;
    int flag = 1;

    char USER_NAME[MAXSIZE];
    char PASSWORD[MAXSIZE];

    printf("Enter the username: ");
    scanf("%s", USER_NAME);
    printf("Enter the password: ");
    scanf("%s", PASSWORD);
    while(flag == 1){
        printf("Menu: \n");
        printf("1. Manage Mail : Shows the stored mails of the logged in user only\n");
        printf("2. Send Mail : Allows the user to send a mail\n");
        printf("3. Quit : Quits the program.\n");
        printf("Select : ");
        scanf("%d", &option);
        if (option == 3){
            printf("Quitting\n");
            flag=0;
        }
        else if ( option==2){
            // smtp part
            int cli_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (cli_sock == -1){
                perror("Socket creation Failed");
                exit(EXIT_FAILURE);
            }
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
            serv_addr.sin_port = htons(atoi(argv[2]));
            int x = connect(cli_sock, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));
            if (x == -1){
                perror("Connect error");
                exit(EXIT_FAILURE);
            }
            char buffer[MAXSIZE];
            printf("Connected to SMTP server\n");
            int n;

            printf("Enter the mail in correct format\n");
            char mail[53][LINESIZE];
            for(int i=0; i<53; i++){
                memset(mail[i], '\0', LINESIZE);
            }
            getchar();
            int mail_endflag = 1;
            for(int i=0; i<53; i++){
                memset(mail[i], '\0', LINESIZE);
                fgets(mail[i], LINESIZE, stdin);
                // printf("Entered: %s", mail[i]);
                if ( strncmp(mail[i], ".", 1)==0 ){
                    break;
                }
                if ( i==52 ){
                    mail_endflag = 0;
                }
            }
            // format checking
            char from[MAXSIZE-15];
            char to[MAXSIZE-15];
            char subject[51];
            
            if (sscanf(mail[0], "From: %[^\n]", from) != 1) {
                printf("%s\n", from);
                printf("Incorrect 'From' format\n");
                close(cli_sock);
                continue;
            }
            int c=0;
            if (from[0] == '@'){
                printf("%s\n", from);
                printf("Incorrect 'From' format\n");
                close(cli_sock);
                continue;
            }
            for(int j=0; j<strlen(from); j++){
                if (from[j] == '@'){
                    if (c==0){
                        c = 1;
                    }
                    else {
                        c = 0;
                        break;
                    }
                }
            }
            if (c!=1){
                printf("%s\n", from);
                printf("Incorrect 'From' format\n");
                close(cli_sock);
                continue;
            }
            if (sscanf(mail[1], "To: %[^\n]", to) != 1) {
                printf("%s\n", to);
                printf("Incorrect 'To' format\n");
                close(cli_sock);
                continue;
            }
            c=0;
            if (to[0] == '@'){
                printf("%s\n", to);
                printf("Incorrect 'To' format\n");
                close(cli_sock);
                continue;
            }
            for(int j=0; j<strlen(to); j++){
                if (to[j] == '@'){
                    if (c==0){
                        c = 1;
                    }
                    else {
                        c = 0;
                        break;
                    }
                }
            }
            if (c!=1){
                printf("%s\n", to);
                printf("Incorrect 'To' format\n");
                close(cli_sock);
                continue;
            }
            if (sscanf(mail[2], "Subject: %50[^\n]", subject) != 1) {
                printf("%s\n", subject);
                printf("Incorrect 'Subject' format\n");
                close(cli_sock);
                continue;
            }
            if (mail_endflag == 0){
                printf("No <CRLF>.<CRLF> end to mail body\n");
                close(cli_sock);
                continue;
            }

            // for(int i=0; i<53; i++){
            //     printf("%s", mail[i]);
            // }
            //printf("Mail ok\n");


            // mail session
            int status;
            memset(buffer, '\0', MAXSIZE);
            n = recv(cli_sock, buffer, MAXSIZE, 0);
            if (n<0){
                perror("recv error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("S: %s\n", buffer);

            char server_ip[MAXSIZE-10];
            char response[MAXSIZE];

            sscanf(buffer, "%d %s Service Ready", &status, server_ip);
            if (status != 220) {
                printf("Error in sending mail: %s\n", buffer);
                close(cli_sock);
                continue;
            }

            memset(buffer, '\0', MAXSIZE);
            sprintf(buffer, "HELO %s\r\n", server_ip);
            n = send(cli_sock, buffer, strlen(buffer), 0);
            if (n<0){
                perror("send error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("C: %s\n", buffer);

            memset(buffer, '\0', MAXSIZE);
            n = recv(cli_sock, buffer, MAXSIZE, 0);
            if (n<0){
                perror("recv error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("S: %s\n", buffer);
            sscanf(buffer, "%d%s", &status, response);
            if (status != 250) {
                printf("Error in sending mail: %s\n", buffer);
                close(cli_sock);
                continue;
            }

            memset(buffer, '\0', MAXSIZE);
            sprintf(buffer, "MAIL FROM: <%s>\r\n", from);
            n = send(cli_sock, buffer, strlen(buffer), 0);
            if (n<0){
                perror("send error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("C: %s\n", buffer);

            memset(buffer, '\0', MAXSIZE);
            n = recv(cli_sock, buffer, MAXSIZE, 0);
            if (n<0){
                perror("recv error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("S: %s\n", buffer);
            sscanf(buffer, "%d%s", &status, response);
            if (status != 250) {
                printf("Error in sending mail: %s\n", buffer);
                close(cli_sock);
                continue;
            }

            memset(buffer, '\0', MAXSIZE);
            sprintf(buffer, "RCPT TO: <%s>\r\n", to);
            n = send(cli_sock, buffer, strlen(buffer), 0);
            if (n<0){
                perror("send error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("C: %s\n", buffer);

            memset(buffer, '\0', MAXSIZE);
            n = recv(cli_sock, buffer, MAXSIZE, 0);
            if (n<0){
                perror("recv error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("S: %s\n", buffer);
            sscanf(buffer, "%d%s", &status, response);
            if (status != 250) {
                printf("Error in sending mail: %s\n", buffer);
                close(cli_sock);
                continue;
            }

            memset(buffer, '\0', MAXSIZE);
            sprintf(buffer, "DATA\r\n");
            n = send(cli_sock, buffer, strlen(buffer), 0);
            if (n<0){
                perror("send error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("C: %s\n", buffer);

            memset(buffer, '\0', MAXSIZE);
            n = recv(cli_sock, buffer, MAXSIZE, 0);
            if (n<0){
                perror("recv error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("S: %s\n", buffer);
            sscanf(buffer, "%d%s", &status, response);
            if (status != 354) {
                printf("Error in sending mail: %s\n", buffer);
                close(cli_sock);
                continue;
            }

            for(int i=0; i<53; i++){
                memset(buffer, '\0', MAXSIZE);
                int k = strlen(mail[i]);
                mail[i][k-1] = '\0';
                sprintf(buffer, "%s\r\n", mail[i]);
                n = send(cli_sock, buffer, strlen(buffer), 0);
                if (n<0){
                    perror("send error");
                    close(cli_sock);
                    break;
                }
                if (n==0) {
                    printf("server disconnected\n");
                    close(cli_sock);
                    break;
                }
                printf("C: %s\n", buffer);

                if ( strncmp(mail[i], ".", 1)==0 ){
                    break;
                }
            }

            memset(buffer, '\0', MAXSIZE);
            n = recv(cli_sock, buffer, MAXSIZE, 0);
            if (n<0){
                perror("recv error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("S: %s\n", buffer);
            sscanf(buffer, "%d%s", &status, response);
            if (status != 250) {
                printf("Error in sending mail: %s\n", buffer);
                close(cli_sock);
                continue;
            }


            memset(buffer, '\0', MAXSIZE);
            sprintf(buffer, "QUIT\r\n");
            n = send(cli_sock, buffer, strlen(buffer), 0);
            if (n<0){
                perror("send error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("C: %s\n", buffer);

            memset(buffer, '\0', MAXSIZE);
            n = recv(cli_sock, buffer, MAXSIZE, 0);
            if (n<0){
                perror("recv error");
                close(cli_sock);
                continue;
            }
            if (n==0) {
                printf("server disconnected\n");
                close(cli_sock);
                continue;
            }
            printf("S: %s\n", buffer);
            sscanf(buffer, "%d%s", &status, response);
            if (status != 221) {
                printf("Error in closing connection: %s\n", buffer);
                close(cli_sock);
                continue;
            }
            printf("Mail sent successfully\n");
            close(cli_sock);
        }
        else if (option == 1){
            // pop3
            int cli_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (cli_sock == -1){
                perror("Socket creation Failed");
                exit(EXIT_FAILURE);
            }
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
            serv_addr.sin_port = htons(atoi(argv[3]));
            int x = connect(cli_sock, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));
            if (x == -1){
                perror("Connect error");
                exit(EXIT_FAILURE);
            }
            char buffer[MAXSIZE];
            printf("Connected to POP3 server\n");

            int n;
            n = recv(cli_sock, buffer, MAXSIZE, 0);
            printf("S: %s\n", buffer);

            char status[MAXSIZE];
            char response[MAXSIZE];
            // USER
            memset(buffer, '\0', MAXSIZE);
            sprintf(buffer, "USER %s\r\n", USER_NAME);
            n = send(cli_sock, buffer, strlen(buffer), 0);
            printf("C: %s\n", buffer);

            memset(buffer, '\0', MAXSIZE);
            n = recv(cli_sock, buffer, MAXSIZE, 0);
            printf("S: %s\n", buffer);

            sscanf(buffer, "%s %s", status, response);
            if ( strcmp(status, "-ERR") == 0){
                close(cli_sock);
                continue;
            }

            // PASS
            memset(buffer, '\0', MAXSIZE);
            sprintf(buffer, "PASS %s\r\n", PASSWORD);
            n = send(cli_sock, buffer, strlen(buffer), 0);
            printf("C: %s\n", buffer);

            n = recv(cli_sock, buffer, MAXSIZE, 0);
            printf("S: %s\n", buffer);

            sscanf(buffer, "%s %s", status, response);
            if ( strcmp(status, "-ERR") == 0){
                close(cli_sock);
                continue;
            }

            // trans
            int msg_count = 0;
            int total_size = 0;
            memset(buffer, '\0', MAXSIZE);
            sprintf(buffer, "STAT\r\n");
            n = send(cli_sock, buffer, strlen(buffer), 0);
            printf("C: %s\n", buffer);

            n = recv(cli_sock, buffer, MAXSIZE, 0);
            printf("S: %s\n", buffer);

            sscanf(buffer, "%s %s", status, response);
            if ( strcmp(status, "-ERR") == 0){
                close(cli_sock);
                continue;
            }
            sscanf(response, "%d %d", &msg_count, &total_size);

            memset(buffer, '\0', MAXSIZE);
            sprintf(buffer, "LIST\r\n");
            n = send(cli_sock, buffer, strlen(buffer), 0);
            printf("C: %s\n", buffer);

            int gh = 0;
            while(1){
                memset(buffer, '\0', MAXSIZE);
                n = recv(cli_sock, buffer, MAXSIZE, 0);
                if (gh == 0){
                    gh = 1;
                    if (buffer[0] == '-'){
                        gh = 2;
                        close(cli_sock);
                        break;
                    }
                }
                printf("S: ");
                int ff = 0;
                for(int i=0; i<n; i++){
                    printf("%c", buffer[i]);
                    if ( (i-2>=0 && i+2<=n-1) && (buffer[i-2]=='\r' && buffer[i-1]=='\n') && buffer[i]=='.' && (buffer[i+1]=='\r' && buffer[i+2]=='\n')){
                        ff = 1;
                        break;
                    }
                }
                printf("\n");
                if (ff == 1){
                    break;
                }
            }
            if (gh == 2){
                break;
            }

            while(1){
                int option = -1;
                // mails printing
                memset(buffer, '\0', MAXSIZE);
                sprintf(buffer, "STAT\r\n");
                n = send(cli_sock, buffer, strlen(buffer), 0);
                printf("C: %s\n", buffer);

                memset(buffer, '\0', MAXSIZE);
                n = recv(cli_sock, buffer, MAXSIZE, 0);
                printf("S: %s\n", buffer);

                sscanf(buffer, "%s %s", status, response);
                if ( strcmp(status, "-ERR") == 0){
                    close(cli_sock);
                    break;
                }
                sscanf(response, "%d %d", &msg_count, &total_size);

                printf("\n--------------------------------------------------\n");
                printf("Sl.No.<Sender’s email><date : hour : minute><Subject>");
                printf("\n--------------------------------------------------\n");
                for (int j=0; j<msg_count; j++){
                    memset(buffer, '\0', MAXSIZE);
                    sprintf(buffer, "RETR %d\r\n", j);
                    n = send(cli_sock, buffer, strlen(buffer), 0);
                    //printf("C: %s\n", buffer);

                    // parsing the mail
                    char sender_mail[MAXSIZE-50];
                    char recv_mail[MAXSIZE-50];
                    char received[MAXSIZE-50];
                    char subject[MAXSIZE-50];
                    //printf("S: ");
                    char data[600];
                    int index = 0;
                    while(1){
                        memset(buffer, '\0', MAXSIZE);
                        n = recv(cli_sock, buffer, MAXSIZE, 0);
                        sprintf(&data[index], "%s", buffer);
                        index += n;
                        int ff=0;
                        for (int i=0; i<n; i++){
                            //printf("%c", buffer[i]);
                            if ( (i-1>=0 && i+1<=n-1) && (buffer[i-1]=='\n') && buffer[i]=='.' && (buffer[i+1]=='\r')){
                                ff = 1;
                                break;
                            }
                        }
                        if (ff == 1){
                            break;
                        }
                    }
                    //printf("\n");

                    if (data[0] == '-'){
                        continue;
                    }
                    memset(response, '\0',400);
                    char body[MAXSIZE-50];
                    int sz=0;
                    sscanf(data, "+OK %d octets\r\nFrom: %s\r\nTo: %s\r\nSubject: %[^\r\n]\r\nReceived: %[^\r\n]\r\n%s", 
                    &sz, sender_mail, recv_mail, subject, received, body);
                    printf("%d. <%s>\t <%s>\t <%s>\n", j, sender_mail, received, subject);

                }
                // prompting
                printf("\n--------------------------------------------------\n");
                printf("(-1 for exit) Enter mail no. to see:");
                scanf("%d", &option);
                if (option == -1){
                    break;
                }
                else if(  option < msg_count && option >= 0){
                    memset(buffer, '\0', MAXSIZE);
                    sprintf(buffer, "RETR %d\r\n", option);
                    n = send(cli_sock, buffer, strlen(buffer), 0);
                    //printf("C: %s\n", buffer);
                    //printf("S: ");
                    char data[600];
                    int index = 0;
                    int size = 0;
                    int lf = 0;

                    memset(buffer, '\0', MAXSIZE);
                    n = recv(cli_sock, buffer, 11+sizeof(int), 0);
                    printf("%s", buffer);

                    char d[10];
                    sscanf(buffer, "+OK %s octets\r\n", d);
                    size = atoi(d);
                    // printf("Size = %d\n", size);
                    printf("\n--------------------------------------------------\n");
                    while(size--){
                        memset(buffer, '\0', MAXSIZE);
                        n = recv(cli_sock, buffer, 1, 0);
                        printf("%s", buffer);
                        
                    }
                    printf("\n");
                    if (data[0] == '-'){
                        continue;
                    }
                    getchar();
                    char y=getchar();
                    //printf("choose: %c", y);

                    if (y == 'd'){
                        memset(buffer, '\0', 0);
                        sprintf(buffer, "DELE %d\r\n", option);
                        n = send(cli_sock, buffer, strlen(buffer), 0);
                        printf("C: %s\n", buffer);

                        memset(buffer, '\0', 0);
                        n = recv(cli_sock, buffer, MAXSIZE, 0);
                        printf("S: %s\n", buffer);

                    }
                }
                else{
                    printf("Mail no. out of range, give again\n");
                }
            }

            memset(buffer, '\0', MAXSIZE);
            sprintf(buffer, "QUIT");
            n = send(cli_sock, buffer, strlen(buffer), 0);
            printf("S: %s\n", buffer);

            memset(buffer, '\0', MAXSIZE);
            n = recv(cli_sock, buffer, MAXSIZE, 0);
            printf("C: %s\n", buffer);

            // update state
            memset(buffer, '\0', MAXSIZE);
            sprintf(buffer, "QUIT");
            n = send(cli_sock, buffer, strlen(buffer), 0);
            printf("S: %s\n", buffer);

            memset(buffer, '\0', MAXSIZE);
            n = recv(cli_sock, buffer, MAXSIZE, 0);
            printf("C: %s\n", buffer);
            
        }
        else {
            printf("Invalid option\n");
            printf("Quitting\n");
            flag=0;
        }
    }

    return 0;
}

char *getLocalIP() {
    char buffer[1024];
    gethostname(buffer, sizeof(buffer));
    struct hostent *host = gethostbyname(buffer);
    return inet_ntoa(*((struct in_addr *)host->h_addr_list[0]));
}
