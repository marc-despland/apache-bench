#include "server.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>

#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <sstream>
#include <chrono>
#include <thread>

using namespace std::chrono;

#include "log.h"

#define MAXEVENTS 1024


Server::Server(unsigned int port) {
	this->events = (struct epoll_event *) calloc (MAXEVENTS, sizeof(struct epoll_event));
	this->pool = epoll_create1 (0);
	this->scenario=-1;
	this->port=port;
	this->state=new StateServer();
}


int Server::makeSocketNonBlocking(int socket) {
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



int Server::listen() {
	struct sockaddr_in server;
	this->socketfd=::socket(AF_INET,SOCK_STREAM,0);
	
	bzero(&server,sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_port=htons(this->port);
	if (::bind(this->socketfd,(struct sockaddr *)&server,sizeof(server))<0) {
		Log::logger->log("SERVER", DEBUG) << "Failed to bind the socket " << this->socketfd << " try to bind to " << this->port<<endl;
		return -1;
	}
	if (!Server::makeSocketNonBlocking(this->socketfd)) {
		Log::logger->log("SERVER", DEBUG) << "Failed to make listen socket non blocking " << this->socketfd << " bind to " << this->port<<endl;
		return -2;
	}
	if (::listen(this->socketfd,this->size)<0) {
		Log::logger->log("SERVER", DEBUG) << "Failed to listen on socket " << this->socketfd << " bind to " << this->port<<endl;
		return -3;
	}
	if (this->add(this->socketfd)!=1) {
		Log::logger->log("SERVER", DEBUG) << "Failed to add socket on pool " << this->socketfd << " bind to " << this->port<<endl;
		return -3;
	}
	Log::logger->log("SERVER", NOTICE) << "Listening on port " << this->port <<endl;
	return 1;
}


int Server::add(int socket) {
	struct epoll_event event;
	event.data.fd = socket;
  	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
  	int s = epoll_ctl (this->pool, EPOLL_CTL_ADD, socket, &event);
  	if (s == -1) {
  		Log::logger->log("SERVER", ERROR) << "Epoll failed to add socket "<< errno << " : " << strerror(errno) <<endl;
  		return -1;
  	}
  	return 1;
}

void Server::close(int socketfd, StateServer * state) {
	if (state->closewait>0) ::sleep(state->closewait);
	if (state->short_close_wait>0) usleep(state->short_close_wait);
	 Log::logger->log("SERVER",NOTICE) << "Send the close : " << socketfd <<endl;
	if (::close(socketfd)<0) {
	    Log::logger->log("SERVER",NOTICE) << "We have close the socket : " << socketfd << " error : " << errno << " " << strerror(errno)<<endl;
	}else{
	    Log::logger->log("SERVER",NOTICE) << "We have close the socket : " << socketfd <<endl;
	}
}


int Server::accept() {
	int clientfd;
	struct sockaddr_in client;
	socklen_t clientsize;
	bzero(&client,sizeof(client));
	clientsize=sizeof(client);
	do {
		bzero(&client,sizeof(client));
		clientsize=sizeof(client);
		clientfd = ::accept(this->socketfd,(struct sockaddr *)&client,&clientsize);
		if ((clientfd<0) && (errno==EAGAIN)) {
			Log::logger->log("SERVER",DEBUG) << "No more request waiting to be accepted"<<endl;
		} else  {
			Log::logger->log("SERVER",DEBUG) << "accept fd=" << clientfd << " error:" << errno<< " " << strerror(errno)<<endl;
		}
		if (clientfd>=0) {
			if (! Server::makeSocketNonBlocking(clientfd)) {
				return -1;
			} else {
				if (!this->add(clientfd)) {
					return -2;
				} else {
					if (this->state->send_bip) {
						milliseconds ms = duration_cast< milliseconds >(
    						system_clock::now().time_since_epoch()
						);
						std::stringstream tmp;
						tmp << "BIP:" << ms.count();
						std::string request= tmp.str();
						Log::logger->log("SERVER",NOTICE) << "Writing data on socket : " << clientfd <<endl;
	  					int ws=write(clientfd, request.c_str(), request.length());
	       				if (ws<0) {
	            			Log::logger->log("SERVER",ERROR) << "Can't write on socket: " << clientfd << " error : " << errno << " " << strerror(errno)<<endl;
	        			}
					}
					if (this->state->direct_close) {
						if (this->state->closewait>0) {
							std::thread closer(Server::close, clientfd, this->state);
							closer.detach();
						} else {
	        				Server::close(clientfd, this->state);
	        			}
					}
				}
			}
		} 
	} while ((clientfd>0) || (errno==EINTR)) ;
	//((clientfd<0) && ((errno==EINTR) || (errno!=EAGAIN)));
	return 1;
}

void Server::read(int socket, StateServer * state) {
	//if (state->readwait>0) sleep(state->readwait);
	int readerror=0;
	ssize_t count;
	do {
		char buf[512];
		count=::read (socket,  buf, sizeof buf);
		readerror=errno;
		buf[count]=0;
		if (count>0) Log::logger->log("SERVER",NOTICE) << "Reading  on socket "<<socket<< " "<<count << " bytes : " << buf<<endl;
		Log::logger->log("SERVER",DEBUG) << "Reading  on socket "<<socket<< " "<<count << " bytes with error " << readerror<< " : " << strerror(readerror)<<endl;
	} while ((readerror==0) && (count>0));
	/*if (((state->shutdown_done) || (state->not_closing_on_close_detected)) && (state->close_after_read)) {
		Log::logger->log("CLIENT",DEBUG) << "We close the socket after reading data	"<<endl;
		state->connected=false;
		::close(socket);
	}*/
}


void Server::run(int scenario) {
	switch (scenario) {
		case 0:
			//close without data
			this->state->direct_close=true;
		break;
		case 1:
			//send BIP then close
			this->state->send_bip=true;
			this->state->direct_close=true;
		break;
		case 2:
			//send BIP then close after 5s
			this->state->send_bip=true;
			this->state->direct_close=true;
			this->state->closewait=5;
		break;
		case 3:
			//Keep connection open, but close it on event EPOLLRDHUP
			this->state->send_bip=false;
			this->state->direct_close=false;
		break;
		case 4:
			//Keep connection open
			this->state->send_bip=false;
			this->state->direct_close=false;
			this->state->not_closing_on_close_detected=true;			
		break;		
		case 5:
			//Send a bip on close detection
			this->state->send_bip_on_close=true;
			this->state->not_closing_on_close_detected=true;
		break;
		case 6:
			this->state->is_an_http_server=true;
		break;
		case 7:
			this->state->is_an_http_server=true;
			this->state->http_close_after_response=true;
		break;
		case 8:
			this->state->is_an_http_server=true;
			this->state->http_keepalive=true;
		break;
		case 9:
			this->state->is_an_http_server=true;
			this->state->http_close_after_response=true;
			this->state->short_close_wait=200;
		break;
	}
	this->scenario=scenario;
	this->go=true;
	this->add(0);
	Log::logger->log("SERVER", NOTICE) << "!!! Press Enter to quit !!!" <<endl;
	if (this->listen()) {
		while (this->go) {
			free(this->events);
			this->events = (struct epoll_event *) calloc (MAXEVENTS, sizeof(struct epoll_event));
			int n = epoll_wait (this->pool, this->events, MAXEVENTS, -1);
			Log::logger->log("SERVER", DEBUG) << "Polling result : "<<n << " sockets" <<endl;
			for (int i = 0; i < n; i++) {
				/*if ((this->events[i].events & EPOLLERR) || (this->events[i].events & EPOLLHUP) || (!(this->events[i].events & EPOLLIN))) {
					Log::logger->log("SERVER", NOTICE) << "Socket event : EPOLLERR | EPOLLHUP | EPOLLIN => close" <<endl;
					::close(this->events[i].data.fd);
				} else {*/
					if (this->events[i].events & EPOLLIN) {
						Log::logger->log("SERVER", DEBUG) << "EPOLLIN : something to read" <<endl;
						if (this->socketfd==this->events[i].data.fd) {
							Log::logger->log("SERVER", NOTICE) << "Accepting new connections" <<endl;
							if (!this->accept()) {
								Log::logger->log("SERVER", ERROR) << "Failed to accept new connection" <<endl;
							}
						} else {
							if (this->events[i].data.fd==0) {
								this->go=false;
							} else {
								//we have data to read
								Server::read(this->events[i].data.fd, this->state);
								if ((this->state->is_an_http_server) && (!(this->events[i].events & EPOLLRDHUP))) {
									time_t rawtime;
		  							struct tm * timeinfo;
		  							char buffer [80]; 
		  							time (&rawtime);
		  							timeinfo = localtime (&rawtime);
		  							strftime (buffer,80,"Date: %a, %d %b %Y %X %Z",timeinfo);
		  							std::string response= "HTTP/1.1 200 OK\r\n"+ string(buffer) +"\r\nServer: fast\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: 13\r\n\r\n{ 'event':0 }";
		  							if ((this->state->http_keepalive) && !(this->events[i].events & EPOLLRDHUP)) {
		  								response= "HTTP/1.1 200 OK\r\n"+ string(buffer) +"\r\nServer: fast\r\nContent-Type: application/json\r\nConnection: keep-alive\r\nContent-Length: 13\r\n\r\n{ 'event':0 }";
		  							}
		  							int ws=write(this->events[i].data.fd, response.c_str(), response.length());
		                			if (ws<0) {
		                				Log::logger->log("SERVER",ERROR) << "Can't write on socket: " << this->events[i].data.fd <<endl;
		                			}

								}
								if (this->state->http_close_after_response) {
									if (this->state->short_close_wait>0) ::usleep(this->state->short_close_wait);
									Log::logger->log("SERVER", NOTICE) << "We close the socket after the response" <<endl;
									::close(this->events[i].data.fd);
								}
							}

						}
					}
					if (this->events[i].events & EPOLLHUP) {
						Log::logger->log("SERVER", NOTICE) << "EPOLLHUP : ??" <<endl;
					}
					if (this->events[i].events & EPOLLERR) {
						Log::logger->log("SERVER", NOTICE) << "EPOLLERR : error" <<endl;
					}
					if (this->events[i].events & EPOLLWAKEUP) {
						Log::logger->log("SERVER", NOTICE) << "EPOLLWAKEUP : wakeup ?" <<endl;
					}
					if (this->events[i].events & EPOLLRDHUP) {
						if (this->state->send_bip_on_close) {
							Log::logger->log("SERVER", NOTICE) << "We send a bip before closing" <<endl;
							milliseconds ms = duration_cast< milliseconds >(
    							system_clock::now().time_since_epoch()
							);
							std::stringstream tmp;
							tmp << "BIP:" << ms.count();
							std::string request= tmp.str();
							Log::logger->log("SERVER",NOTICE) << "Writing data on socket : " << this->events[i].data.fd <<endl;
	  						int ws=write(this->events[i].data.fd, request.c_str(), request.length());
	       					if (ws<0) {
	            				Log::logger->log("SERVER",ERROR) << "Can't write on socket: " << this->events[i].data.fd << " error : " << errno << " " << strerror(errno)<<endl;
	        				}
						}
						if (!this->state->not_closing_on_close_detected) {
							Log::logger->log("SERVER", NOTICE) << "Socket event : EPOLLRDHUP We detect the client has close the connection so we close it on our side" <<endl;
							::close(this->events[i].data.fd);
						} else {
							Log::logger->log("SERVER", NOTICE) << "Socket event : EPOLLRDHUP We detect the client has close the connection but we do nothing" <<endl;
						}
					} 
				//}
			}

		}
	}
	Log::logger->log("SERVER", NOTICE) << "Bye !!" <<endl;
}