#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"

void errExit(const char* format, ...) {
	va_list arg;
	va_start(arg, format);
	fprintf(stderr, format, arg);
	va_end(arg);
	exit(1);
}

int setNonBlocking(int fd) {
	return fcntl(fd, F_SETFL, O_NONBLOCK);
}


int getListenableSocket(const char* port) {
	int status;
	int listener;
	int yes = 1;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	
	memset(&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	status = getaddrinfo(NULL, port, &hints, &servinfo);
	if (status != 0) {
		errExit("getaddrinfo: %s\n", gai_strerror(status));
	}

	for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener == -1) {
			perror("socket");
			continue;
		}
		// Get rid of address in use errors
		status = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
		if (status == -1) {
			perror("setsockopt");
			close(listener);
			continue;
		}
		
		// Bind the socket
		status = bind(listener, p->ai_addr, p->ai_addrlen);
		if (status == -1) {
			perror("bind");
			close(listener);
			continue;
		}

		// Socket has been created and bound so we can exit the loop
		break;
	}


	freeaddrinfo(servinfo);

	setNonBlocking(listener);

	return listener;
}
