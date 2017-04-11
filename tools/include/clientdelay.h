#ifndef _CLIENTDELAY_H
#define _CLIENTDELAY_H

#include <string>
#include <sys/epoll.h>
using namespace std;
#include <map>
#include <ctime>

class ClientDelay {
public:
	ClientDelay(unsigned int dst_port, string host, unsigned int nbrequest, unsigned int concurrent);
	void run();
protected:
	static int makeSocketNonBlocking(int socket);
	static void read(int socket);
	int connect();
	int add(int socket);
	map<int, time_t> * duration;

	unsigned int dstport;
	string dsthost;
	unsigned int size;
	int		pool;
	struct 	epoll_event *events;
	bool go;
	unsigned int nbrequest;
	unsigned int concurrent;

};

#endif