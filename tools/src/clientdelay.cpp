#include "clientdelay.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>

#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include "log.h"

#define MAXEVENTS 1024


ClientDelay::ClientDelay(unsigned int dst_port, string host, unsigned int nbrequest, unsigned int concurrent) {
	this->events = (struct epoll_event *) calloc (MAXEVENTS, sizeof(struct epoll_event));
	this->pool = epoll_create1 (0);
	this->dstport=dst_port;
	this->dsthost=host;
	this->nbrequest=nbrequest;
	this->concurrent=concurrent;
	this->duration=new map<int, time_t>();

}


int ClientDelay::makeSocketNonBlocking(int socket) {
	int flags, s;
	flags = fcntl (socket, F_GETFL, 0);
	if (flags == -1) {
     return -1;
    }

	flags |= O_NONBLOCK;
	s = fcntl (socket, F_SETFL, flags);
	if (s == -1) {
      return -2;
    }
    return 1;
}



int ClientDelay::connect() {
	struct sockaddr_in server;
	long hostaddress;
	long srcaddress;
	struct hostent *serverent;
	//struct epoll_event *events;

	bzero(&server,sizeof(server));
	hostaddress = inet_addr(this->dsthost.c_str());
	if ( (long)hostaddress != (long)4294967295) {
		Log::logger->log("CLIENT",DEBUG) << this->dsthost << " is an IP address : "<< hostaddress <<endl;
		bcopy(&hostaddress,&server.sin_addr,sizeof(hostaddress));
	} else {
		Log::logger->log("CLIENT",DEBUG) << this->dsthost << " is probably an host name" <<endl;
		serverent = gethostbyname(this->dsthost.c_str());
		if (serverent == NULL){
			Log::logger->log("CLIENT",ERROR) << this->dsthost << " can't find its IP address" <<endl;
			return -1;
		}
		bcopy(serverent->h_addr,&server.sin_addr,serverent->h_length);
	}
	server.sin_port = htons(this->dstport);
	server.sin_family = AF_INET;
	int socketfd;
	if ( (socketfd = ::socket(AF_INET,SOCK_STREAM,0)) < 0) {
		Log::logger->log("CLIENT",ERROR) << "Can't connect : " << strerror(errno) <<endl;
		return -2;
	}


	struct sockaddr_in source;
	bzero(&(source),sizeof(struct sockaddr_in));
	srcaddress=inet_addr("0.0.0.0");
	bcopy(&srcaddress,&source.sin_addr,sizeof(srcaddress));
	source.sin_port = 0;
	source.sin_family = AF_INET;
	if (::bind(socketfd, (struct sockaddr *)&(source), sizeof(struct sockaddr_in))<0) {
		Log::logger->log("CLIENT",ERROR) << "Can't bind : " << strerror(errno) <<endl;
		return -3;
	}

	ClientDelay::makeSocketNonBlocking(socketfd);

	int result=::connect( socketfd,(struct sockaddr *)&(server), sizeof(server));
	if ((result<0) && (errno!=EINPROGRESS)) {
	//if (::connect( socketfd,(struct sockaddr *)&(server), sizeof(server)) < 0 ) {
		Log::logger->log("CLIENT",ERROR) << "Can't connect : " << strerror(errno) <<endl;
		return -4;
	}

	Log::logger->log("CLIENT", DEBUG) << "Conected to " << this->dsthost << " : " << this->dstport <<endl;
	(*this->duration)[socketfd]=time(0);
	return socketfd;
}


int ClientDelay::add(int socket) {
	if (! ClientDelay::makeSocketNonBlocking(socket)) {
		return -1;
	} 
	struct epoll_event event;
	event.data.fd = socket;
  	event.events = EPOLLOUT | EPOLLIN | EPOLLET | EPOLLRDHUP;
  	int s = epoll_ctl (this->pool, EPOLL_CTL_ADD, socket, &event);
  	if (s == -1) {
  		Log::logger->log("CLIENT", ERROR) << "Epoll failed to add socket "<< errno << " : " << strerror(errno) <<endl;
  		return -2;
  	}
  	return 1;
}

void ClientDelay::read(int socket) {
	int readerror=0;
	ssize_t count;
	do {
		char buf[512];
		count=::read (socket,  buf, sizeof buf);
		readerror=errno;
		buf[count]=0;
		if (count>0) Log::logger->log("CLIENT",DEBUG) << "Reading  on socket "<<socket<< " "<<count << " bytes : " << buf<<endl;
		if (errno==EBADF) Log::logger->log("CLIENT",ERROR) << "Can't read, the socket has been destroy !!! "<<endl;
		Log::logger->log("CLIENT",DEBUG) << "Reading  on socket "<<socket<< " "<<count << " bytes with error " << readerror<< " : " << strerror(readerror)<<endl;
	} while ((readerror==0) && (count>0));
}


void ClientDelay::run() {
	unsigned int count=0;
	unsigned int worker=0;
	time_t starttime=time(0);
	while (count<this->nbrequest || worker>0) {
		Log::logger->log("CLIENT", DEBUG) << "We have request to manage ("<<count<<", "<< worker<<")"<<endl;
		int tosend=(difftime(time(0), starttime)*this->concurrent)-count;
		//while (worker<this->concurrent && count<this->nbrequest) {
		if (count<this->nbrequest) {
			Log::logger->log("CLIENT", DEBUG) << "We have to create "<<tosend<< " requests"<<endl;
			for (int i=0; i<tosend; i++) {
				Log::logger->log("CLIENT", DEBUG) << "We need new worker"<<endl;

				int socket=this->connect();
				if (socket>0) {
					this->add(socket);
					count++;
					worker++;
				}
			}
		}
		
		Log::logger->log("CLIENT", DEBUG) << "Cleaning event"<<endl;
		bzero(this->events, MAXEVENTS * sizeof(struct epoll_event));
		Log::logger->log("CLIENT", DEBUG) << "Start wainting for event"<<endl;
		int n = epoll_wait (this->pool, this->events, MAXEVENTS, 100);
		Log::logger->log("CLIENT", DEBUG) <<  "We received " << n << " events " << errno<<endl;
		for (int i = 0; i < n; i++) {
			Log::logger->log("CLIENT", DEBUG) <<  "Enterring event loop " <<endl;
			if ((this->events[i].events & EPOLLERR) || (this->events[i].events & EPOLLHUP) || (!((this->events[i].events & EPOLLIN) || (this->events[i].events & EPOLLOUT)))) {
				if (this->events[i].events & EPOLLHUP) {
					Log::logger->log("CLIENT", DEBUG) << "Hang up happened on the associated file descriptor." <<endl;
					//worker--;
				}
				if (this->events[i].events & EPOLLERR) {
					Log::logger->log("CLIENT", NOTICE) << "Socket event : EPOLLERR "  <<" Duration="<<difftime(time(0), (*this->duration)[this->events[i].data.fd]) << " (sent: " <<count<<", waiting: "<< worker<<")"<<endl;
					::close(this->events[i].data.fd);
					worker--;
				}
			} else { 
				if (this->events[i].events & EPOLLIN) {
					Log::logger->log("CLIENT", DEBUG) << "There is something to read socket="<< this->events[i].data.fd <<endl;
					ClientDelay::read(this->events[i].data.fd);
					if (this->events[i].events & EPOLLRDHUP) {
						Log::logger->log("CLIENT", NOTICE) << "Server close the socket "<< this->events[i].data.fd <<" Duration="<<difftime(time(0), (*this->duration)[this->events[i].data.fd]) << " (sent: " <<count<<", waiting: "<< worker<<")"<<endl;
						::close(this->events[i].data.fd);
						worker--;
					}

				} else {
					if (this->events[i].events & EPOLLOUT) {
						Log::logger->log("CLIENT", DEBUG) << "Socket "<<count<< " successfully conected fd=" << events[i].data.fd <<endl;
						//this->add(socket);
						std::string request= "GET / HTTP/1.1\r\nHost: "+ this->dsthost +"\r\nConnection: close\r\nAccept: */*\r\n\r\n";
		  				int ws=write(events[i].data.fd, request.c_str(), request.length());
		       			if (ws<0) {
		            		Log::logger->log("CLIENT",ERROR) << "Can't write on socket: " << events[i].data.fd <<endl;
						}

					} else {
						Log::logger->log("CLIENT", ERROR) << "Epoll return more that 1 request : "<<n <<endl;
					}
				}
			}
		}
	}
	Log::logger->log("CLIENT", NOTICE) << "Press enter to quit"<<endl;
	string str;
	getline(cin, str);
}