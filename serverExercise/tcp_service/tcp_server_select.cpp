#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctime>
#include <fcntl.h>

#define PORT 8111
#define MESSAGE_LEN 1024
#define FD_SIZE 1024

int main(int argc, char* argv[]) {
    int socket_fd, accept_fd;
    int on = 1;
    int ret = 0;
    int backlog = 10;
    int flags;
    int max_fd = -1;
    int curpos = -1;
    int events = 0;
    fd_set fd_sets;
    int accept_fds[FD_SIZE] = {-1, };
    
    struct sockaddr_in localaddr, remoteaddr;
    
    char in_buff[MESSAGE_LEN] = {0, };
    
    // step 1: create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // get the socket flag
    flags = fcntl(socket_fd, F_GETFL, 0);
    // set socket to non-block
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    max_fd = socket_fd;
    
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
        FD_ZERO(&fd_sets);
        // put the socket into sets
        FD_SET(socket_fd, &fd_sets);
        // put the data socket into sets
        for(int i = 0; i < FD_SIZE; i++) {
            if(accept_fds[i] != -1) {
                if(accept_fds[i] > max_fd) {
                    max_fd = accept_fds[i];
                }
                FD_SET(accept_fds[i], &fd_sets);
            }
        }
        // listen to the event
        events = select(max_fd+1, &fd_sets, NULL, NULL, NULL);
        if(events < 0) {
            std::cout << "Failed to use select!" << std::endl;
            break;
        } else if(events == 0) {
            std::cout << "timeout...!" << std::endl;
            continue;
        } else if(events) {
            // socket is coming
            if(FD_ISSET(socket_fd, &fd_sets)) {
                for(int i = 0; i < FD_SIZE; i++) {
                    if(accept_fds[i] == -1) {
                        curpos = i;
                        break;
                    }
                }
                
                socklen_t addr_len = sizeof(struct sockaddr);
                accept_fd = accept(socket_fd, (struct sockaddr*) &remoteaddr, &addr_len);
                
                flags = fcntl(accept_fd, F_GETFL, 0);
                fcntl(accept_fd, F_SETFL, flags | O_NONBLOCK);
                
                accept_fds[curpos] = accept_fd;
            }
            // data is coming
            for(int i = 0; i < FD_SIZE; i++) {
                if(accept_fds[i] != -1 && FD_ISSET(accept_fds[i], &fd_sets)) {
                    memset(&in_buff, 0, sizeof(char) * MESSAGE_LEN);
                    ret = (int)recv(accept_fds[i], (void*)in_buff, MESSAGE_LEN, 0);
                    // ret = 0 when the peer orderly shutdown
                    if(ret == 0)
                        close(accept_fds[i]);
                    std::cout << "recv: " << in_buff << std::endl;
                    send(accept_fds[i], (void*)in_buff, MESSAGE_LEN, 0);
                }
            }
        }
    }
    close(socket_fd);
    
    return 0;
}
