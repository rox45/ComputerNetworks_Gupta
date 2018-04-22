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
        error("ERROR cannot execute command");
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
        perror("ERROR cannot close stream\n");
    }
    return result;
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd;    //File descriptors in file descriptor table, stores value returned by socket system call
    int portno;    //Port number for server to accept connections
    struct sockaddr_in serv_addr, cli_addr;    //struct containing server address
    int n; //return value for read and write calls
    socklen_t clilen;   //Stores the length of the client address

    char buffer[256];
    char* filename;
    
    FILE* fp;
    char* checksum;

    if (argc < 2) {
        fprintf(stderr, "ERROR no port provided\n");
        exit(1);
    }

    //Create socket, socket(family, type, protocol)
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //AF_INET: IPv4 family; SOCK_STREAM: stream socket(TCP); 0: system default
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


    //Keep reading until client supplies a valid filename
    while(1) {
        printf("Waiting for file name...\n");

        //Clear/initialize buffer to 0
        memset(buffer, 0, 256);

        n = read(newsockfd, buffer, 256);

        if (n < 0) {
            error("ERROR reading from socket");
        }

        //Receive the filename
        filename = malloc(strlen(buffer) + 1);
        strcpy(filename, buffer);
        memset(buffer, 0, 256);

        fp = fopen(filename, "rb");        
        printf("File name received: %s\n", filename);


        //If file not found, loop
        if (fp == NULL) {
            printf("File \"%s\" was not found\n", filename);
            write(newsockfd, "Message from server: File not found", 36);
        }
        //File found
        else {
            write(newsockfd, "Message from server: Sending file...", 37);

            //Get checksum of file and send it to client
            char command[256] = "openssl md5 ";
            strncat(command, filename, strlen(filename));
            checksum = exec(command); //Get the checksum by bash shell

            printf("File %s\n", checksum);
            write(newsockfd, checksum, strlen(checksum) + 1);

            break;
        }
    }

    //Read the file and send 256 bytes at a time
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

    free(filename);

    fclose(fp);
    close(newsockfd);
    close(sockfd);
     
    return 0; 
}
