#include <iostream>
#include <stdlib.h>
#include "server.hpp"


int main(int argc, char* argv[]) {
    // create a sub-process
    // let the rest code executing under this process
    if(daemon(0, 0) == -1) {
        std::cout << "Daemon creation error\n" << std::endl;
        exit(-1);
    }
    
    avdance::Server* pServer = new avdance::Server();
    if(pServer) {
        if((pServer -> create_socket()) < 0)
            exit(-1);
        if((pServer -> bind_socket()) < 0)
            exit(-1);
        if((pServer -> start_listen()) < 0)
            exit(-1);
        pServer -> run();
    }
    
    return 0;
}
