// server code for UDP socket programming
//https://www.geeksforgeeks.org/c-program-for-file-transfer-using-udp/
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
 
#define PORT_NO 15050
#define NET_BUF_SIZE 256
#define cipherKey 'S'
#define sendrecvflag 0
#define nofile "File Not Found!"
 
void error(const char *msg) {
    perror(msg);
    exit(1);
}

//Function to encrypt
char Cipher(char ch) {
    return ch ^ cipherKey;
}
 
//Function sending file
int sendFile(FILE* fp, char* buf, int s) {
    int len;

    if (fp == NULL) {
        strcpy(buf, nofile);
        len = strlen(nofile);
        buf[len] = EOF;
        for (int i = 0; i <= len; i++)
            buf[i] = Cipher(buf[i]);
        return 1;
    }
 
    char ch, ch2;

    for (int i = 0; i < s; i++) {
        ch = fgetc(fp);
        ch2 = Cipher(ch);
        buf[i] = ch2;
        if (ch == EOF)
            return 1;
    }
    return 0;
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

int main() {
    int sockfd, nBytes;
    struct sockaddr_in addr_con;
    int addrlen = sizeof(addr_con);
    addr_con.sin_family = AF_INET;
    addr_con.sin_port = htons(PORT_NO);
    addr_con.sin_addr.s_addr = INADDR_ANY;

    char buffer[NET_BUF_SIZE];
    char* filename;

    FILE* fp;
    char* checksum;
 
    // socket()
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 
    if (sockfd < 0)
        printf("\nfile descriptor not received!!\n");
    else
        printf("\nfile descriptor %d received\n", sockfd);
 
    // bind()
    if (bind(sockfd, (struct sockaddr*)&addr_con, sizeof(addr_con)) == 0)
        printf("\nSuccessfully binded!\n");
    else
        printf("\nBinding Failed!\n");
 
    //Keep reading until client supplies a valid filename
    while (1) {
        printf("\nWaiting for file name...\n");
 
        //Receive file name
        memset(buffer, 0, NET_BUF_SIZE);
 
        nBytes = recvfrom(sockfd, buffer,
                          NET_BUF_SIZE, sendrecvflag,
                          (struct sockaddr*)&addr_con, &addrlen);

        filename = malloc(strlen(buffer) + 1);
        strcpy(filename, buffer);
        memset(buffer, 0, NET_BUF_SIZE);

        fp = fopen(filename, "r");
        printf("\nFile Name Received: %s\n", buffer);

       //File not found, loop
        if (fp == NULL) {
            printf("file \"%s\" is not found\n", filename);
            sendto(sockfd, "server: file not found", 23, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);
        }
        else {
            printf("\nFile Successfully opened!\n");
            sendto(sockfd, "server: sending file...", 24, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);

            break;
        }
    }
    //Get checksum and send it to client
    char command[256] = "openssl md5 ";
    strncat(command, filename, strlen(filename));
    checksum = exec(command); //Get the checksum by bash shell

    printf(checksum);
    sendto(sockfd, checksum, strlen(checksum), sendrecvflag, (struct sockaddr*)&addr_con, addrlen);
    
    //Read the file and send 256 bytes at a time
    while (1) {

        if (sendFile(fp, buffer, NET_BUF_SIZE)) {
            sendto(sockfd, buffer, NET_BUF_SIZE,
               sendrecvflag, 
               (struct sockaddr*)&addr_con, addrlen);
            break;
        }

        sendto(sockfd, buffer, NET_BUF_SIZE, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);
        memset(buffer, 0, NET_BUF_SIZE);
    }

    fclose(fp);
    free(filename);
    return 0;
}

