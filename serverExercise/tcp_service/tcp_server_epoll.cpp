/*
 The epoll is only supported by Linux system
 can't run on macos
 */
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctime>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>

#define PORT 8111
#define MESSAGE_SIZE 1024
#define MAX_EVENTS 20
#define TIMEOUT 500

int main(int argc, char* argv[]) {
    int socket_fd = -1;
    int accept_fd = -1;
    int curpos = 0;
    int on = 1;
    int ret = 0;
    int backlog = 10;
    int flags = 1;
    
    char in_buff[MESSAGE_SIZE] = {0, };
    struct sockaddr_in localaddr, remoteaddr;
    
    int epoll_fd;
    struct epoll_event ev, events[MAX_EVENTS];
    int event_number;
    
    // step 1: create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1) {
	    std::cout << "create socket error" << std::endl;
	    exit(1);
    }

    flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    
    localaddr.sin_family = AF_INET; // AF_INET --> IPV4
    localaddr.sin_port = PORT;
    localaddr.sin_addr.s_addr = INADDR_ANY; // allow traffics from all addr
    bzero(&(localaddr.sin_zero), 8); // set the padding size for TCP package
    
    // step 2: bind socket to the port
    ret = bind(socket_fd, (struct sockaddr*) &localaddr, sizeof(struct sockaddr));
    
    // step 3: start to listen
    ret = listen(socket_fd, backlog);
    
    // step 4: accept --> receive & send
    epoll_fd = epoll_create(256);
    
    ev.events = EPOLLIN;
    ev.data.fd = socket_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev);
    
    for(;;) {
        event_number = epoll_wait(epoll_fd, events, MAX_EVENTS, TIMEOUT);
        for(int i = 0; i < event_number; i++) {
            if(events[i].data.fd == socket_fd) {
                socklen_t addr_len = sizeof(struct sockaddr);
                accept_fd = accept(socket_fd, (struct sockaddr*) &remoteaddr, &addr_len);
                
                flags = fcntl(accept_fd, F_GETFL, 0);
                fcntl(accept_fd, F_SETFL, flags | O_NONBLOCK);
                
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = accept_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, accept_fd, &ev);
            } else if(events[i].events & EPOLLIN) {
                do{
                    memset(&in_buff, 0, sizeof(char) * MESSAGE_SIZE);
                    ret = (int)recv(events[i].data.fd, (void*)in_buff, MESSAGE_SIZE, 0);
                    // ret = 0 when the peer orderly shutdown
                    if(ret == 0) {
                        close(events[i].data.fd);
                        break;
                    }
                } while(ret < -1 && errno == EINTR);
                if(ret == MESSAGE_SIZE) {
                    std::cout << "may have data..." << std::endl;
                }
                if(ret < 0) {
                    switch(errno) {
                        case EAGAIN:
                            break;
                        default:
                            break;
                    }
                }
                if(ret > 0) {
                    std::cout << "recv: " << in_buff << std::endl;
                    send(events[i].data.fd, (void*)in_buff, MESSAGE_SIZE, 0);
                }
            }
        }
    }
    std::cout << "quit server...\n" << std::endl;
    close(socket_fd);
    
    return 0;
}
