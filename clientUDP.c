// client code for UDP socket programming
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
 
#define IP_ADDRESS "127.0.0.1" // localhost
#define PORT_NO 15050
#define cipherKey 'S'
#define sendrecvflag 0
 
void error(const char *msg) {
    perror(msg);
    exit(1);
}

//Function for decryption
char Cipher(char ch)
{
    return ch ^ cipherKey;
}
 
//Function to receive file
int recvFile(FILE* fp, char* buffer, int s)
{
    char ch;

    for (int i = 0; i < s; i++) {
        ch = buffer[i];
        ch = Cipher(ch);
        if (ch == EOF)
            return 1;
        else {
            fprintf(fp, "%c", ch);
        }
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
    addr_con.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    char buffer[256];
    char* filename;

    FILE* fp;
    char* localChecksum;
    char* serverChecksum;
 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 
    if (sockfd < 0)
        printf("\nfile descriptor not received!!\n");
    else
        printf("\nfile descriptor %d received\n", sockfd);


        memset(buffer, 0, 256);

    //Keep reading until client supplies a valid filename
    while(strcmp(buffer, "Message from server: Sending file...") != 0) {
        printf("\nPlease enter file name to receive:\n");
        scanf("%s", buffer);

        sendto(sockfd, buffer, 256, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);

        filename = malloc(strlen(buffer) + 1);
        strcpy(filename, buffer);
        memset(buffer, 0, 256);

        recvfrom(sockfd, buffer, 256, sendrecvflag, (struct sockaddr*)&addr_con, &addrlen);
    }

    remove(filename);
    fp = fopen(filename, "ab");

    //Read server file checksum
    memset(buffer, 0, 256);
    recvfrom(sockfd, buffer, 256, sendrecvflag, (struct sockaddr*)&addr_con, &addrlen);
    serverChecksum = malloc(strlen(buffer) + 1);
    strcpy(serverChecksum, buffer);  
    memset(buffer, 0, 256);

    while (1) {
        //Receive
        memset(buffer, 0, 256);

        nBytes = recvfrom(sockfd, buffer, 256, sendrecvflag, (struct sockaddr*)&addr_con, &addrlen);

        //Process
        if (recvFile(fp, buffer, 256)) {
            break;
        }
    }

    char command[256] = "openssl md5 ";
    strncat(command, filename, strlen(filename));
        localChecksum = exec(command); //Get the checksum by bash shell

        printf("%s\n", serverChecksum);
        printf("%s\n", localChecksum);

        if (strcmp(serverChecksum, localChecksum) != 0) {
            printf("File is corrupted\n");
        }

        fclose(fp);
        close(sockfd);

        free(serverChecksum);
        free(filename);

        return 0;
    }
