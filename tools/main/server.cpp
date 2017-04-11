#include <iostream>
#include "options.h"
#include "log.h"
#include "serverdelay.h"


int main(int argc, char **argv) {
	cout << " Starting"<<endl;
	Log::logger->setLevel(NOTICE);
	Options options(argv[0], "1.0.0", "HTTP 1.0 server with delayed response");
	try {
		options.add('d', "debug", "Start on debug mode", false, false);
		options.add('p', "port", "Port to listen to", true, true);
		options.add('s', "second", "Number of second to wait before sending the response", true, true);
	} catch(ExistingOptionException &e ) {
	}
	try {
		options.parse(argc, argv);
		if (options.get('d')->isAssign()) Log::logger->setLevel(DEBUG);
		ServerDelay * server=new ServerDelay(options.get("port")->asInt(),options.get("second")->asInt());
		server->run();
		
	} catch (OptionsStopException &e) {
	} catch (UnknownOptionException &e) {
		cout << " Request unknown option"<<endl;
	}

}