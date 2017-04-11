#ifndef _DELAYRESPONSE_H
#define _DELAYRESPONSE_H

#include <ctime>

class DelayResponse {
public:
	DelayResponse(int socketfd);
	int socket();
	bool ready(int delay);

protected:
	int socketfd;
	std::time_t arrived;
};

#endif