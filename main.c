#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <stdbool.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include <netdb.h>

#include <signal.h>

#include "server.h"

#define BUFSIZE 100

typedef struct {
	int nRequests;
	int nTransferred;
	struct sockaddr_storage* addr;
} client;

void usage(const char* program);

void eventLoop(int epoll_fd, int server_fd, const size_t bufLen);

bool clearSocket(int socket, char* buf, const size_t len);

/*
 * Creates the server's listening socket and start the event loop
 */
int main(int argc, char** argv) {
	int status;
	int server_fd;
	char* port = "7000"; // default port
	size_t bufLen = BUFSIZE; // default buffer size

	int epoll_fd;
	struct epoll_event event;

	// TODO: Add signal handler for SIGTERM

	// Parse the command line arguments
	int c;
	int p;
	while ((c = getopt(argc, argv, "p:b:")) != -1) {
		switch (c) {
			case 'p':
				// hack
				p = strtol(optarg, NULL, 10);
				if (p < 1000) {
					usage(argv[10]);
					exit(1);
				}
				port = optarg;
				break;
			case 'b':
				bufLen = strtoul(optarg, NULL , 10);
				break;
			default:
				usage(argv[0]);
				exit(1);
		}
	}

	printf("Starting the server\n");
	printf("Port: %s\nBuffer length: %lu\n", port, bufLen);

	server_fd = getListenableSocket(port);

	status = listen(server_fd, 10);
	if (status == -1) {
		errExit("listen: %s\n", strerror(errno));
	}

	epoll_fd = epoll_create1(0); // might need flags
	if (epoll_fd == -1) {
		errExit("epoll_create1: %s\n", strerror(errno));
	}
	
	// Register the server socket for epoll events
	event.data.fd = server_fd;
	event.events = EPOLLIN | EPOLLET;
	status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);
	if (status == -1) {
		errExit("epoll_ctl: %s\n", strerror(errno));
	}

	// Start the event loop
	eventLoop(epoll_fd, server_fd, bufLen);

	// Cleanup
	close(server_fd);
	close(epoll_fd);
}

void usage(const char* program) {
	fprintf(stderr, "Usage: %s -p [port] -b [buffer size]\n", program);
}

void eventLoop(int epoll_fd, int server_fd, const size_t bufLen) {
	const int MAX_EVENTS = 256;
	struct epoll_event current_event, event;
	int n_ready;
	int status;
	struct sockaddr_storage remote_addr;
	socklen_t addr_len;

	char* buffer = calloc(bufLen, sizeof (char));
	struct epoll_event *events = calloc(MAX_EVENTS, sizeof(struct epoll_event));

	while (true) {
		n_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (n_ready == -1) {
			errExit("epoll_wait: %s\n", strerror(errno));
		}

		// A file descriptor is ready
		for (int i = 0; i < n_ready; ++i) {
			int client_fd;
			current_event = events[i];
			// An error or hangup occurred
			// Client might have closed their side of the connection
			if (current_event.events & (EPOLLHUP | EPOLLERR)) {
				fprintf(stderr, "epoll: EPOLLERR or EPOLLHUP\n");
				continue;
			}
			// The server is receiving a connection request
			if (current_event.data.fd == server_fd) {
				// TODO: Should this block of code be in a loop?
				client_fd = accept(server_fd, (struct sockaddr*) &remote_addr, &addr_len);
				if (client_fd == -1) {
					if (errno != EAGAIN && errno != EWOULDBLOCK) {
						perror("accept");
					}
					continue;
				}
				setNonBlocking(client_fd);

				// Add the client socket to the epoll instance
				event.data.fd = client_fd;
				event.events = EPOLLIN | EPOLLET;
				status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
				if (status == -1) {
					errExit("epoll_ctl: %s\n", strerror(errno));
				}

				char host[BUFSIZE], serv[BUFSIZE];
				status = getnameinfo((struct sockaddr*) &remote_addr, addr_len, host, BUFSIZE, serv, BUFSIZE, NI_NUMERICHOST | NI_NUMERICSERV);
				if (status == -1) {
					fprintf(stderr, "getnameinfo: %s\n", gai_strerror(status));
					continue;
				}
				printf("Connection from %s:%s\n", host, serv);
				continue;
			} 
			if (!clearSocket(current_event.data.fd, buffer, bufLen)) { // One of the sockets has data to read
				close(current_event.data.fd);
			}
		}
	}

	if (events) {
		free(events);
	}
	if (buffer) {
		free(buffer);
	}
}

bool clearSocket(int socket, char* buf, const size_t len) {
	int n = 0;
	int bytesLeft = len;
	char *bp;

	while (true) {
		bp = buf;
		while (n < len) {
			n = recv(socket, bp, bytesLeft, 0);
			if (errno == EAGAIN || n == 0) { // need this for edge-triggered mode
				return false;
			}
			bp += n;
			bytesLeft -= n;
		}
		printf("sending: %s\n", buf);
		n = send(socket, buf, len, 0);
		if (n == -1) {
			// client closed the connection
			return false;
		}
		return true;
	}
	close(socket);
	return false;
}
