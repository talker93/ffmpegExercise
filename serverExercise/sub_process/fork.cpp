#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

void daemonize() {
	int fd;
	pid_t pid;

	if((pid = fork()) < 0) {
		std::cout << "can't create subprocess!" << std::endl;
	} else {
        // all code below, will be executed in different process
        // exit the parent process
		if(pid != 0) {
			exit(0);
		}
	}

    // construct new session with current process
	setsid();

    // re-direction
	if(chdir("/") < 0) {
		std::cout << "can't change dir!" << std::endl;
		exit(-1);
	}
	
	fd = open("/dev/null", O_RDWR);
	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);

	return;
}

int main(int argc, char* argv[])
{
	daemonize();
    // secure that the program keeps working
	while(1) {
		sleep(1);
	}
}
