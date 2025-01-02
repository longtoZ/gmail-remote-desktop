#pragma once

#include "utils.h"

class Machine {
private:
	int client_socket;
	json requestFromClient;
	const int BUFFER_SIZE = 8192;
	char buffer[8192] = { 0 };

    void sendMessageOnce(string cmd, int status, string filename = "");

    // Shut down, restart and sleep device
    void restartDeviceHelper(int& sleepTime);

    void shutdownDeviceHelper(int& sleepTime);

    void sleepDeviceHelper(int& sleepTime);

public:
    Machine(int client_socket) : client_socket(client_socket) {}

    void restartDevice(json requestFromClient);

    void shutdownDevice(json requestFromClient);

    void sleepDevice(json requestFromClient);

};