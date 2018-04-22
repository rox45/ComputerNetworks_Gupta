// client code for UDP socket programming
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
 
#define IP_PROTOCOL 0
#define IP_ADDRESS "127.0.0.1" // localhost
#define PORT_NO 15050
#define NET_BUF_SIZE 32
#define cipherKey 'S'
#define sendrecvflag 0
 
// funtion to clear buffer
void clearBuf(char* buffer)
{
    int i;
    for (i = 0; i < NET_BUF_SIZE; i++)
        buffer[i] = '\0';
}
 
// function for decryption
char Cipher(char ch)
{
    return ch ^ cipherKey;
}
 
// function to receive file
int recvFile(FILE* fp, char* buffer, int s)
{
    int i;
    char ch;
    for (i = 0; i < s; i++) {
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
 
// driver code
int main()
{
    int sockfd, nBytes;
    struct sockaddr_in addr_con;
    int addrlen = sizeof(addr_con);
    addr_con.sin_family = AF_INET;
    addr_con.sin_port = htons(PORT_NO);
    addr_con.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    char buffer[NET_BUF_SIZE];
    FILE* fp;
 
    // socket()
    sockfd = socket(AF_INET, SOCK_DGRAM, IP_PROTOCOL);
 
    if (sockfd < 0)
        printf("\nfile descriptor not received!!\n");
    else
        printf("\nfile descriptor %d received\n", sockfd);
 
    while (1) {
        printf("\nPlease enter file name to receive:\n");
        scanf("%s", buffer);
        sendto(sockfd, buffer, NET_BUF_SIZE,
               sendrecvflag, (struct sockaddr*)&addr_con,
               addrlen);

        fp = fopen(buffer, "ab");
 
        printf("\n---------Data Received---------\n");
        
        while (1) {
            // receive
            clearBuf(buffer);

            nBytes = recvfrom(sockfd, buffer, NET_BUF_SIZE,
                              sendrecvflag, (struct sockaddr*)&addr_con,
                              &addrlen);

            // process
            if (recvFile(fp, buffer, NET_BUF_SIZE)) {
                break;
            }
        }
        printf("\n-------------------------------\n");

        fclose(fp);

        close(sockfd);
    }
    return 0;
}
