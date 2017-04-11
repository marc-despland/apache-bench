#ifndef _SERVER_H
#define _SERVER_H

#include <string>
#include <sys/epoll.h>
using namespace std;

#include "stateserver.h"


class Server {
public:
	Server(unsigned int port);
	void run(int scenario);
protected:
	static int makeSocketNonBlocking(int socket);
	int listen();
	int accept();
	int add(int socket);
	static void close(int socketfd, StateServer * state);
	static void read(int socket, StateServer * state);

	int socketfd;
	unsigned int port;
	unsigned int size;
	int		pool;
	struct 	epoll_event *events;
	bool go;
	int scenario;
	StateServer * state;

};

#endif