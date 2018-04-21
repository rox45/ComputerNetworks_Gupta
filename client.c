#define _BSD_SOURCE
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd; //File descriptor in file descriptor table, stores value returned by socket system call
    int portno; //Port number for server to accept connections
    int n;  //return value for read and write calls
    struct sockaddr_in serv_addr; //struct containing server address
    struct hostent *server;

    int bytesReceived = 0;

    char buffer[256];

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(1);
    }

    portno = atoi(argv[2]);

    //Create socket, socket(family, type, protocol)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //AF_INET: IPv4 family; SOCK_STREAM: stream socket(TCP); 0: system default

    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    server = gethostbyname(argv[1]);

    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));  //Clear address structure

     //Setup the host_addr structure for use in bind call
     //Server byte order
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);

    serv_addr.sin_port = htons(portno);
    
    //Connect to server
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }
    

    printf("Enter file name: ");
    
    memset(buffer, 0, 256);  //Clear/initialize buffer to 0
    fgets(buffer, 255, stdin);    //Write input to buffer

    n = write(sockfd, buffer, strlen(buffer));  //Write file name to server

    //Create file
    FILE *fp;
    fp = fopen(buffer, "ab");
    if (NULL == fp) {
        printf("Error opening file");
        return 1;
    }

    //Recieve data
    while ((bytesReceived = read(sockfd, buffer, 256)) > 0) {
         printf("Bytes received %d\n", 1, bytesReceived);
         fwrite(buffer, 1, bytesReceived, fp);
    }

    if (bytesReceived < 0) {
        error("ERROR read error");
    }

    if (n < 0) {
         error("ERROR writing to socket");
     }

    memset(buffer, 0, 256);

    n = read(sockfd, buffer, 255);

    if (n < 0) {
         error("ERROR reading from socket");
     }

    printf("%s\n", buffer);

    fclose(fp);
    close(sockfd);
    return 0;
}