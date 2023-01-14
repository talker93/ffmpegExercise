/**
 * Server class
 * @author shanjiang
 * @date 2023-01-07
 * @copyright GPL 2.0
 */

#ifndef __SERVER_H__
#define __SERVER_H__

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

namespace avdance {

#define PORT 8111
#define MESSAGE_LEN 1024

class Server {
public:
    Server();
    ~Server();
    
    int create_socket();
    
    int bind_socket();
    
    int start_listen();
    
    void run();
    
private:
    int m_iSocket_fd, m_iAccept_fd;
    int m_iOn;
    int m_iRet;
    int m_iBacklog;
    struct sockaddr_in m_struLocalAddr, m_struRemoteAddr;
    char m_cInBuff[MESSAGE_LEN];
};

}  // namespace advance

#endif //__SERVER_H__
