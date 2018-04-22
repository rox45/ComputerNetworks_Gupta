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

void error(const char *msg) {
    perror(msg);
    exit(0);
}

//Run system command and return output, from https://stackoverflow.com/questions/1583234/c-system-function-how-to-collect-the-output-of-the-issued-command
char* exec(char* command) {
    FILE* fp;
    char* line = NULL;
    // Following initialization is equivalent to char* result = ""; and just
    // initializes result to an empty string, only it works with
    // -Werror=write-strings and is so much less clear.
    char* result = (char*) calloc(1, 1);
    size_t len = 0;

    fflush(NULL);
    fp = popen(command, "r");
    if (fp == NULL) {
        error("Cannot execute command");
    }

    while(getline(&line, &len, fp) != -1) {
        // +1 below to allow room for null terminator.
        result = (char*) realloc(result, strlen(result) + strlen(line) + 1);
        // +1 below so we copy the final null terminator.
        strncpy(result + strlen(result), line, strlen(line) + 1);
        free(line);
        line = NULL;
    }

    fflush(fp);
    if (pclose(fp) != 0) {
        perror("Cannot close stream.\n");
    }
    return result;
}

int main(int argc, char *argv[]) {
    int sockfd; //File descriptor in file descriptor table, stores value returned by socket system call
    int portno; //Port number for server to accept connections
    int n;  //return value for read and write calls
    struct sockaddr_in serv_addr; //struct containing server address
    struct hostent *server;

    int bytesReceived = 0;

    char buffer[256];
    char* filename;

    FILE *fp; 
    char* checksum;


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
        fprintf(stderr, "ERROR no such host\n");
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
    
    memset(buffer, 0, 256);  //Clear/initialize buffer to 0

    //Keep prompting user to enter a valid filename
    while(strcmp(buffer, "server: sending file...") != 0) {

        printf("Enter file name: ");
    
        memset(buffer, 0, 256);
        fgets(buffer, 255, stdin);    //Write input to buffer

        //remove suspect newline from input
        if (buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
        }

        n = write(sockfd, buffer, strlen(buffer));  //Write file name to server

        filename = malloc(strlen(buffer) + 1);
        strcpy(filename, buffer);
        memset(buffer, 0, 256);

        read(sockfd, buffer, 256);  //read server response
        printf("\n%s\n", buffer);
    }

    //Create file
    fp = fopen(filename, "ab");
    if (NULL == fp) {
        fprintf(stderr, "Error opening file");
        exit(1);
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

    char command[256] = "openssl md5 ";
    strncat(command, filename, strlen(filename));
    checksum = exec(command); //Get the checksum by bash shell

    printf(checksum);

    free(filename);

    close(sockfd);
    return 0;
}