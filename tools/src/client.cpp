#include "client.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>

#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include "log.h"

#define MAXEVENTS 1024


Client::Client(unsigned int source_port, unsigned int dst_port, string host) {
	this->events = (struct epoll_event *) calloc (MAXEVENTS, sizeof(struct epoll_event));
	this->pool = epoll_create1 (0);
	this->scenario=-1;
	this->srcport=source_port;
	this->dstport=dst_port;
	this->dsthost=host;
	this->state=new State();

}


int Client::makeSocketNonBlocking(int socket) {
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



int Client::connect() {
	struct sockaddr_in server;
	long hostaddress;
	long srcaddress;
	struct hostent *serverent;
	struct epoll_event *events;

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
	source.sin_port = htons(this->srcport);
	source.sin_family = AF_INET;
	if (::bind(socketfd, (struct sockaddr *)&(source), sizeof(struct sockaddr_in))<0) {
		Log::logger->log("CLIENT",ERROR) << "Can't bind : " << strerror(errno) <<endl;
		return -3;
	}

	if (::connect( socketfd,(struct sockaddr *)&(server), sizeof(server)) < 0 ) {
		Log::logger->log("CLIENT",ERROR) << "Can't connect : " << strerror(errno) <<endl;
		return -4;
	}

	Log::logger->log("CLIENT", NOTICE) << "Conected from " << this->srcport << " to " << this->dsthost << " : " << this->dstport <<endl;
	return socketfd;
}


int Client::add(int socket) {
	if (! Client::makeSocketNonBlocking(socket)) {
		return -1;
	} 
	struct epoll_event event;
	event.data.fd = socket;
  	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
  	int s = epoll_ctl (this->pool, EPOLL_CTL_ADD, socket, &event);
  	if (s == -1) {
  		Log::logger->log("CLIENT", ERROR) << "Epoll failed to add socket "<< errno << " : " << strerror(errno) <<endl;
  		return -2;
  	}
  	return 1;
}

void Client::read(int socket, State * state) {
	if (state->readwait>0) sleep(state->readwait);
	int readerror=0;
	ssize_t count;
	do {
		char buf[512];
		count=::read (socket,  buf, sizeof buf);
		readerror=errno;
		buf[count]=0;
		if (count>0) Log::logger->log("CLIENT",NOTICE) << "Reading  on socket "<<socket<< " "<<count << " bytes : " << buf<<endl;
		if (errno==EBADF) Log::logger->log("CLIENT",ERROR) << "Can't read, the socket has been destroy !!! "<<endl;
		Log::logger->log("CLIENT",DEBUG) << "Reading  on socket "<<socket<< " "<<count << " bytes with error " << readerror<< " : " << strerror(readerror)<<endl;
	} while ((readerror==0) && (count>0));
	if (((state->shutdown_done) || (state->not_closing_on_close_detected)) && (state->close_after_read)) {
		Log::logger->log("CLIENT",DEBUG) << "We close the socket after reading data	"<<endl;
		state->connected=false;
		::close(socket);
	}
}


void Client::run(int scenario) {
	switch (scenario) {
		case 0:
		break;
		case 1:
			this->state->readwait=10;
		break;
		case 2:
			this->state->use_shutdown=true;
			this->state->readwait=10;
		break;
		case 3:
			this->state->use_shutdown=true;
			this->state->readwait=10;
		break;
		case 4:
			this->state->use_shutdown=true;
			this->state->readwait=10;
			this->state->close_after_read=true;
		break;
		case 5:
			this->state->not_closing_on_close_detected=true;
			this->state->readwait=10;
			this->state->close_after_read=true;
		break;
		case 6:
			this->state->close_after_connect=true;
		break;
		case 7:
			this->state->not_closing_on_close_detected=true;
			this->state->close_after_read=true;
		break;
		case 8:
			this->state->nbrequest=1;
			this->state->is_an_http_client=true;
		break;
		case 9:
			this->state->nbrequest=1;
			this->state->is_an_http_client=true;
			this->state->http_close_after_response=true;
		break;
		case 10:
			this->state->nbrequest=1;
			this->state->is_an_http_client=true;
			this->state->http_keepalive=true;
		break;
	}

	for (int i=0; i<this->state->nbrequest; i++) {
		if ((scenario==3) && (i==1)) sleep(15);
		int socket=this->connect();
		this->state->shutdown_done=false;
		if (socket>0) {
			Log::logger->log("CLIENT", NOTICE) << "Socket "<<i<< " successfully conected fd=" << socket <<endl;
			this->add(socket);
			this->state->connected=true;
			std::thread * reader;
			if (this->state->close_after_connect) {
				this->state->connected=false;
				Log::logger->log("CLIENT", NOTICE) << "We decide to close the socket" <<endl;
				::close(socket);
			}
			if (this->state->is_an_http_client) {
				std::string request= "GET / HTTP/1.1\r\nHost: "+ this->dsthost +"\r\nConnection: close\r\nAccept: */*\r\n\r\n";
				if (this->state->http_keepalive) {
					request= "GET / HTTP/1.1\r\nHost: "+ this->dsthost +"\r\nConnection: keep-alive\r\nAccept: */*\r\n\r\n";
				}
	  			int ws=write(socket, request.c_str(), request.length());
	            if (ws<0) {
	            	Log::logger->log("CLIENT",ERROR) << "Can't write on socket: " << socket <<endl;
	            }
			}


			while (this->state->connected) {
				bzero(this->events, MAXEVENTS * sizeof(struct epoll_event));
				Log::logger->log("CLIENT", DEBUG) << "Start wainting for event"<<endl;
				int n = epoll_wait (this->pool, this->events, MAXEVENTS, -1);
				Log::logger->log("CLIENT", DEBUG) <<  "We received " << n << " events" <<endl;
				if (n==1) {
					if (this->events[0].data.fd==socket) {
						if ((this->events[0].events & EPOLLERR) || (this->events[0].events & EPOLLHUP) || (!(this->events[0].events & EPOLLIN))) {
							if (this->events[0].events & EPOLLHUP) {
								Log::logger->log("CLIENT", NOTICE) << "Hang up happened on the associated file descriptor." <<endl;
								this->state->connected=false;
							}
							if (this->events[0].events & EPOLLERR) {
								Log::logger->log("CLIENT", NOTICE) << "Socket event : EPOLLERR " <<endl;
								this->state->connected=false;
								::close(this->events[0].data.fd);
							}
						} else { 
							if (this->events[0].events & EPOLLIN) {
								Log::logger->log("CLIENT", NOTICE) << "There is something to read socket="<< this->events[0].data.fd <<endl;
								if (this->state->readwait>0) {
									int fd=this->events[0].data.fd;
									reader=new std::thread(Client::read, fd, this->state);
								} else {
									Client::read(this->events[0].data.fd, this->state);
								}
							}
							if (this->events[0].events & EPOLLRDHUP) {
								Log::logger->log("CLIENT", NOTICE) << "Client detect the server has close the connection socket="<< this->events[0].data.fd <<endl;
								if (this->state->use_shutdown) {
									this->state->shutdown_done=true;
									if (!this->state->close_after_read) this->state->connected=false;
									::shutdown(this->events[0].data.fd,SHUT_WR);
								} else {
									this->state->connected=false;
									if (!this->state->not_closing_on_close_detected) ::close(this->events[0].data.fd);
								}
								
							}
						}
					} else {
						Log::logger->log("CLIENT", ERROR) << "Not our socket, strange" <<endl;		
					}
				} else {
					Log::logger->log("CLIENT", ERROR) << "Epoll return more that 1 request : "<<n <<endl;
				}
			}
			if (this->state->readwait>0) {
				reader->join();
			}
		} else {
			Log::logger->log("CLIENT", NOTICE) << "Failed to connect socket "<<i<<  " error " << errno<< " : " << strerror(errno)<<endl;
		}
	}
	Log::logger->log("CLIENT", NOTICE) << "Press enter to quit"<<endl;
	string str;
	getline(cin, str);
}