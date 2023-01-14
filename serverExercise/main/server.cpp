/**
 * Server class
 * @author shanjiang
 * @date 2023-01-07
 * @copyright GPL 2.0
 */

#include "server.hpp"

namespace avdance
{

Server::Server()
{
    std::cout << "construct..." << std::endl;
    
    m_iOn = 1;
    m_iRet = 0;
    m_iBacklog = 10;
    memset(m_cInBuff, 0, sizeof(char)*1024);
}

Server::~Server()
{
    std::cout << "destruct..." << std::endl;
}

int Server::create_socket()
{
    // step 1: create socket
    m_iSocket_fd = socket(AF_INET, SOCK_STREAM, 0);
    m_iRet = setsockopt(m_iSocket_fd, SOL_SOCKET, SO_REUSEADDR, &m_iOn, sizeof(m_iOn));
    
    m_struLocalAddr.sin_family = AF_INET; // AF_INET --> IPV4
    m_struLocalAddr.sin_port = PORT;
    m_struLocalAddr.sin_addr.s_addr = INADDR_ANY; // allow all addr traffics
    bzero(&(m_struLocalAddr.sin_zero), 8); // set padding size for TCP packet
    
    return m_iRet;
}

int Server::bind_socket()
{
    // step 2: bind socket to the port
    m_iRet = bind(m_iSocket_fd, (struct sockaddr*) &m_struLocalAddr, sizeof(struct sockaddr));
    return m_iRet;
}

int Server::start_listen()
{
    // step 3: start to listen
    m_iRet = listen(m_iSocket_fd, m_iBacklog);
    return m_iRet;
}

void Server::run()
{
    // step 4: accept --> receive & send
    socklen_t addr_len = sizeof(struct sockaddr);
    m_iAccept_fd = accept(m_iSocket_fd, (struct sockaddr*) &m_struRemoteAddr, &addr_len);

    while(1)
    {
        memset(&m_cInBuff, 0, sizeof(char) * MESSAGE_LEN);
        m_iRet = (int)recv(m_iAccept_fd, (void*)m_cInBuff, MESSAGE_LEN, 0);
        if(m_iRet == 0)
        {
            break;
        }
        
        std::cout << "recv: " << m_cInBuff << std::endl;
        send(m_iAccept_fd, (void*)m_cInBuff, MESSAGE_LEN, 0);
    }

    close(m_iAccept_fd);
}

} // namespace advance
