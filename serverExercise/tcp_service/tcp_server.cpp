#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8111
#define MESSAGE_LEN 1024

int main(int argc, char* argv[]) {
    int socket_fd, accept_fd;
    int on = 1;
    int ret = 0;
    int backlog = 10;
    
    struct sockaddr_in localaddr, remoteaddr;
    
    char in_buff[MESSAGE_LEN] = {0, };
    
    // step 1: create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    
    localaddr.sin_family = AF_INET; // AF_INET --> IPV4
    localaddr.sin_port = PORT;
    localaddr.sin_addr.s_addr = INADDR_ANY; // allow traffics from all addr
    bzero(&(localaddr.sin_zero), 8); // set the padding size for TCP package
    
    // step 2: bind socket to the port
    ret = bind(socket_fd, (struct sockaddr*) &localaddr, sizeof(struct sockaddr));
    
    // step 3: start to listen
    ret = listen(socket_fd, backlog);
    
    // step 4: accept --> receive & send
    for(;;) {
        socklen_t addr_len = sizeof(struct sockaddr);
        accept_fd = accept(socket_fd, (struct sockaddr*) &remoteaddr, &addr_len);
        for(;;) {
            memset(&in_buff, 0, sizeof(char) * MESSAGE_LEN);
            ret = recv(accept_fd, (void*)in_buff, MESSAGE_LEN, 0);
            if(ret == 0) {
                break;
            }
            
            std::cout << "recv: " << in_buff << std::endl;
            send(accept_fd, (void*)in_buff, MESSAGE_LEN, 0);
        }
        
        close(accept_fd);
    }
    
    return 0;
}
