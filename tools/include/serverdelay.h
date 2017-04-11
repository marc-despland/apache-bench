#ifndef _SERVERDELAY_H
#define _SERVERDELAY_H

#include <string>
#include <sys/epoll.h>
using namespace std;
#include <queue>

#include "delayresponse.h"


class ServerDelay {
public:
	ServerDelay(unsigned int port,unsigned int delay);
	void run();
protected:
	static int makeSocketNonBlocking(int socket);
	int listen();
	int accept();
	int add(int socket);
	static void close(int socketfd);
	static void read(int socket);

	int socketfd;
	unsigned int port;
	unsigned int size;
	int		pool;
	struct 	epoll_event *events;
	bool go;
	DelayResponse * current;
	unsigned int delay;
	queue<DelayResponse *> * waitingList;

};

#endif