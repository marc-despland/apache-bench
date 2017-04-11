#include <iostream>
#include "options.h"
#include "log.h"
#include "clientdelay.h"


int main(int argc, char **argv) {
	cout << " Starting"<<endl;
	Log::logger->setLevel(NOTICE);
	Options options(argv[0], "1.0.0", "Check simultaneous concurrent HTTP client");
	try {
		options.add('d', "debug", "Start on debug mode", false, false);
		options.add('t', "target", "Target host", true, true);
		options.add('c', "dstport", "Destination port on target", true, true);
		options.add('n', "nbrequest", "Number of requests to send", true, true);
		options.add('w', "worker", "nb worker", true, true);
	} catch(ExistingOptionException &e ) {
	}
	try {
		options.parse(argc, argv);
		if (options.get('d')->isAssign()) Log::logger->setLevel(DEBUG);
		ClientDelay * client=new ClientDelay(options.get("dstport")->asInt(), options.get("target")->asChars(),options.get("nbrequest")->asInt(),options.get("worker")->asInt());
		client->run();
		
	} catch (OptionsStopException &e) {
	} catch (UnknownOptionException &e) {
		cout << " Request unknown option"<<endl;
	}

}