#include "state.h"

State::State() {
	this->readwait=0;
	this->nbrequest=2;
	this->shutdown_done=false;
	this->close_after_read=false;
	this->use_shutdown=false;
	this->connected=false;
	this->not_closing_on_close_detected=false;
	this->close_after_connect=false;
	this->is_an_http_client=false;
	this->http_keepalive=false;
	this->http_close_after_response=false;

}