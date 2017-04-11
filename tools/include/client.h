#ifndef _CLIENT_H
#define _CLIENT_H

#include <string>
#include <sys/epoll.h>
using namespace std;
#include "state.h"

class Client {
public:
	Client(unsigned int source_port, unsigned int dst_port, string host);
	void run(int scenario);
protected:
	static int makeSocketNonBlocking(int socket);
	static void read(int socket, State * state);
	int connect();
	int add(int socket);

	unsigned int srcport;
	unsigned int dstport;
	string dsthost;
	unsigned int size;
	int		pool;
	struct 	epoll_event *events;
	bool go;
	int scenario;
	State * state;

};

#endif