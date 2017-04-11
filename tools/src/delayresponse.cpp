#include "delayresponse.h"


DelayResponse::DelayResponse(int socketfd) {
	this->socketfd=socketfd;
	this->arrived=std::time(0);
}


int DelayResponse::socket() {
	return this->socketfd;
}
bool DelayResponse::ready(int delay) {
	std::time_t now=std::time(0);
	return (difftime(now, this->arrived)>=delay);
}
