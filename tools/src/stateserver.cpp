#include "stateserver.h"

StateServer::StateServer() {
	this->send_bip=false;
	this->direct_close=false;
	this->closewait=0;
	this->not_closing_on_close_detected=false;
	this->send_bip_on_close=false;
	this->is_an_http_server=false;
	this->http_keepalive=false;
	this->http_close_after_response=false;
	this->short_close_wait=0;

}