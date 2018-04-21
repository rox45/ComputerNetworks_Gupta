/* The port number is passed as an argument */
#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd;    //File descriptors in file descriptor table, stores value returned by socket system call
    int portno;    //Port number for server to accept connections
    struct sockaddr_in serv_addr, cli_addr;    //struct containing server address
    int n; //return value for read and write calls
    socklen_t clilen;

    char buffer[256];
    char* filename;
    
    FILE* fp;

    int boolFileFound = 0;

    if (argc < 2) {
        fprintf(stderr, "ERROR no port provided\n");
        exit(1);
    }

    //Create socket
    sockfd =  socket(AF_INET, SOCK_STREAM, 0); ///AF_INET: IPv4 family; SOCK_STREAM: stream socket(TCP); 0: system default
    
    if (sockfd < 0) {
       error("ERROR opening socket");
    }

    //Clear address structure
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    portno = atoi(argv[1]);

    //Setup the host_addr structure for use in bind call
    //Server byte order
    serv_addr.sin_family = AF_INET;  

    //Fill with current host's IP address
    serv_addr.sin_addr.s_addr = INADDR_ANY;  

    //Convert short integer value for port must be converted into network byte order
    serv_addr.sin_port = htons(portno);

    //Bind the socket to the address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    //Listen for incoming connections with a backlog queue of 5
    listen(sockfd, 5);

    clilen = sizeof(cli_addr);

    /* accept() blocks until client connects to server, returns file descriptor which */
    /* should be used for all communications on this connection  */
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0) {
        error("ERROR on accept");
    }

    printf("server: got connection from %s port %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));


    //This send() function sends the 13 bytes of the string to the new socket
    //send(newsockfd, "ack!\n", 13, 0);

    while(!boolFileFound) {
        //Clear/initialize buffer to 0
        memset(buffer, 0, 256);

        n = read(newsockfd, buffer, 255);

        if (n < 0) {
            error("ERROR reading from socket");
        }

        //Copy filename from buffer
        filename = malloc(strlen(buffer) + 1);
        strcpy(filename, buffer);
        memset(buffer, 0, 256);

        //Open file
        fp = fopen(filename, "rb");

        if (fp == NULL) {
            printf("file \"%s\" is not found\n", filename);
            write(newsockfd, "server: file not found", 23);
        }
        else {
            write(newsockfd, "server: sending file...", 24);
            boolFileFound = 1;
        }
    }

    while (1) {
        //Read 256 bytes
        int nread = fread(buffer, 1, 256, fp);
        printf("Bytes read %d\n", nread);

        //Read success, send data
        if (nread > 0) {
            printf("Sending\n");
            write(newsockfd, buffer, nread);
        }

        if (nread < 256) {
            if (feof(fp)) {
                printf("End of file\n");
            }
            if (ferror(fp)) {
                fprintf(stderr, "Error reading\n");
                exit(1);
            }
            break;
        }
     }

    memset(buffer, 0, 256);

    fclose(fp);
    close(newsockfd);
    close(sockfd);
     
    return 0; 
}
