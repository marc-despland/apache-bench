#ifndef _STATESERVER_H
#define _STATESERVER_H


class StateServer {
public:
	StateServer();
	bool send_bip;
	bool direct_close;
	int closewait;
	bool not_closing_on_close_detected;
	bool send_bip_on_close;
	bool is_an_http_server;
	bool http_keepalive;
	bool http_close_after_response;
	int short_close_wait;

};

#endif