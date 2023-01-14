#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8111
#define MESSAGE_LEN 1024

int main(int argc, char* argv[])
{
    int socket_fd, ret;
    
    struct sockaddr_in serveraddr;
    
    char sendbuf[MESSAGE_LEN] = {0, };
    char recvbuf[MESSAGE_LEN] = {0, };
    
    // step 1: create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = PORT;
    serveraddr.sin_addr.s_addr = inet_addr("206.189.201.71"); // string to binary
    
    // step 2: connects to the server
    ret = connect(socket_fd, (struct sockaddr*) &serveraddr, sizeof(struct sockaddr));
    
    while(1) {
        memset(sendbuf, 0, MESSAGE_LEN);
        gets(sendbuf);
        
        // step 3: send data
        send(socket_fd, sendbuf, strlen(sendbuf), 0);
        
        // add a quit handle
        if(strcmp(sendbuf, "quit") == 0) {
            break;
        }
        
        // step 4: receive data
        ret = recv(socket_fd, recvbuf, MESSAGE_LEN, 0);
        std::cout <<  "received: " << recvbuf << std::endl;
    }
    
    close(socket_fd);
}
