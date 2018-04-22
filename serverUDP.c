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
#define cipherKey 'S'
#define sendrecvflag 0
 
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
    int sockfd; //File descriptor in file descriptor table, stores value returned by socket system call
    int nBytes; //Number of bytes read
    struct sockaddr_in addr_con;    //struct containing server address
    int addrlen = sizeof(addr_con); //Stores the length of the address

    addr_con.sin_family = AF_INET;
    addr_con.sin_port = htons(PORT_NO);
    addr_con.sin_addr.s_addr = INADDR_ANY;

    char buffer[256];
    char* filename;

    FILE* fp;
    char* checksum;
 
    //Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);    //AF_INET: IPv4 family; SOCK_DGRAM: datagram socket(UDP); 0: system default
 
    if (sockfd < 0) {
        error("ERROR opening socket");
    }
 
    //Bind the socket to the address
    if (bind(sockfd, (struct sockaddr*)&addr_con, sizeof(addr_con)) < 0) {
        error("ERROR on binding");
    }
        
 
    //Keep reading until client supplies a valid filename
    while (1) {
        printf("Waiting for file name...\n");
 
        //Receive file name
        memset(buffer, 0, 256);
        nBytes = recvfrom(sockfd, buffer, 256, sendrecvflag, (struct sockaddr*)&addr_con, &addrlen);

        filename = malloc(strlen(buffer) + 1);
        strcpy(filename, buffer);
        memset(buffer, 0, 256);

        fp = fopen(filename, "r");
        printf("File name received: %s\n", filename);

       //If file not found, loop
        if (fp == NULL) {
            printf("File \"%s\" was not found\n", filename);
            sendto(sockfd, "Message from server: File not found", 36, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);  //Send negative acknoledgment
        }
        //File found
        else {
            sendto(sockfd, "Message from server: Sending file...", 37, sendrecvflag, (struct sockaddr*)&addr_con, addrlen); //Send acknoledgment

            //Get checksum of file and send it to client
            char command[256] = "openssl md5 ";
            strncat(command, filename, strlen(filename));
            checksum = exec(command); //Get the checksum by bash shell

            printf(checksum);
            sendto(sockfd, checksum, strlen(checksum), sendrecvflag, (struct sockaddr*)&addr_con, addrlen);
    
            break;
        }
    }

    //Read the file and send 256 bytes at a time
    while (1) {

        if (sendFile(fp, buffer, 256)) {
            sendto(sockfd, buffer, 256,
               sendrecvflag, 
               (struct sockaddr*)&addr_con, addrlen);
            break;
        }

        sendto(sockfd, buffer, 256, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);
        memset(buffer, 0, 256);
    }

    fclose(fp);
    free(filename);
    return 0;
}

