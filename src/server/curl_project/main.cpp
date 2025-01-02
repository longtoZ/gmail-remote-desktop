#include "server.h"

int main() {
	Server server;

	if (!server.initialize()) {
		cout << "Failed to initialize server" << endl;
		return EXIT_FAILURE;
	}

	if (!server.bindSocket()) {
		cout << "Failed to bind socket" << endl;
		return EXIT_FAILURE;
	}

	server.listenAndAccept();

	return 0;
}