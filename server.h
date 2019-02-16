#ifndef _SERVER_H
#define _SERVER_H

void errExit(const char* format, ...);

int setNonBlocking(int fd);

int getListenableSocket(const char* port);
#endif // _SERVER_H
