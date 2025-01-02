#include "utils.h"

unordered_map<string, string> level_colors = {
	{"INFO", WHITE},
	{"SUCCESS", GREEN},
	{"WARNING", YELLOW},
	{"ERROR", RED}
};

// Function to get the current time in the format YYYYMMDD_HHMMSS
string get_current_time() {
	auto now = chrono::system_clock::now();
	time_t now_c = chrono::system_clock::to_time_t(now);
	tm timeinfo;
	localtime_s(&timeinfo, &now_c);

	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &timeinfo);
	return string(buffer);
}

// Function to get the size of a file (in bytes)
int get_file_size(const char* filename) {
	ifstream file(filename, ios::binary | ios::ate);
	return file.tellg();
}

// Function to get the full path of a file
string get_file_path(const char* filename) {
	char buffer[MAX_PATH];

	DWORD result = GetFullPathName(filename, MAX_PATH, buffer, NULL);

	if (result == 0) {
		log("Error getting full path of the file", ERROR_LOG);
		return "";
	}

	return string(buffer);
}

// Function to trim leading and trailing spaces from a string
string trim(const string& str) {
	// Find the index of the first non-space character
	size_t start = str.find_first_not_of(" \t\n\r\f\v");

	// If the string is empty or contains only spaces, return an empty string
	if (start == string::npos) {
		return "";
	}

	// Find the index of the last non-space character
	size_t end = str.find_last_not_of(" \t\n\r\f\v");

	// Return the trimmed substring
	return str.substr(start, end - start + 1);
}

// Function to convert a string to json (used for parsing the request body)
json convert_body_to_json(string body) {
	json data;

	stringstream ss(body);
	string line;

	while (getline(ss, line, '\n')) {
		if (line.empty()) continue;

		int pos = line.find(":");
		string key = trim(line.substr(0, pos));
		string value = trim(line.substr(pos + 1, line.length() - pos - 2));
		data[key] = value;
	}

	cout << data.dump() << endl;

	return data;
}

// Function to split a string based on a delimiter
vector<string> split_string(const string& str, char delimiter) {
	vector<string> result;
	stringstream ss(str);
	string item;

	while (getline(ss, item, delimiter)) {
		if (item.empty()) continue;

		result.push_back(trim(item));
	}

	return result;
}

// Function to add missing ending backslash to a path
string add_ending_backslash(string path) {
	return path[path.size() - 1] == '\\' ? path : path + "\\";
}

// Function to encode a string to base64
string base64_encode(const string& in) {
	string out;

	int val = 0, valb = -6;
	for (uchar c : in) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back(base64_chars[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6) out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
	while (out.size() % 4) out.push_back('=');
	return out;
}

// Function to handle URL-safe base64 decoding specifically for Gmail API. 
// Since Gmail API uses URL-safe base64 encoding, we need to convert it to standard base64 encoding before decoding it.
vector<unsigned char> gmail_base64_decode(const string& encoded_string) {
	// First, create a copy of the string that we can modify
	string modified_string = encoded_string;

	// Replace URL-safe characters with standard base64 characters
	replace(modified_string.begin(), modified_string.end(), '-', '+');
	replace(modified_string.begin(), modified_string.end(), '_', '/');

	// Add padding if necessary
	switch (modified_string.length() % 4) {
	case 0: break;                  // No padding needed
	case 2: modified_string += "=="; break; // Add two padding characters
	case 3: modified_string += "=";  break; // Add one padding character
	default: throw runtime_error("Invalid base64 length");
	}

	// Now decode the modified string
	vector<unsigned char> decoded;
	decoded.reserve(modified_string.length() * 3 / 4);

	int val = 0, valb = -8;
	for (unsigned char c : modified_string) {
		if (c == '=') break;  // Handle padding

		if (c >= 'A' && c <= 'Z')
			c = c - 'A';
		else if (c >= 'a' && c <= 'z')
			c = c - 'a' + 26;
		else if (c >= '0' && c <= '9')
			c = c - '0' + 52;
		else if (c == '+')
			c = 62;
		else if (c == '/')
			c = 63;
		else
			continue;  // Skip invalid characters

		val = (val << 6) + c;
		valb += 6;

		if (valb >= 0) {
			decoded.push_back(char((val >> valb) & 0xFF));
			valb -= 8;
		}
	}

	return decoded;
}

// Callback function for libcurl to handle response
size_t write_callback(void* ptr, size_t size, size_t nmemb, string* data) {
	data->append((char*)ptr, size * nmemb);
	return size * nmemb;
}

void send_response_with_file(SOCKET server_socket, json response) {
	const int BUFFER_SIZE = 8192;
	char buffer[BUFFER_SIZE];

	// Send the file to the client
	string path = response["body"]["path"];
	ifstream ifile(path, ios::binary);

	if (!ifile.is_open()) {
		log("Error opening file: " + path, ERROR_LOG);
		response["body"]["status"] = "error";
		return;
	}

	// Send a message to the client to indicate that the file is being sent
	memset(buffer, 0, BUFFER_SIZE);
	send(server_socket, response.dump().c_str(), BUFFER_SIZE, 0);

	// Read the file in chunks and send it to the client
	while (true) {
		// Make the buffer zero before reading
		memset(buffer, 0, BUFFER_SIZE);

		ifile.read(buffer, BUFFER_SIZE);
		int bytes_read = ifile.gcount();

		if (bytes_read <= 0) {
			cout << "End of file" << endl;
			break;
		}

		if (send(server_socket, buffer, bytes_read, 0) == SOCKET_ERROR) {
			cerr << "Send failed with error: " << WSAGetLastError() << endl;
			response["body"]["status"] = "error";
			break;
		}

		cout << "Sent " << bytes_read << " bytes" << endl;
	}

	cout << "File sent successfully" << endl;
	ifile.close();

}

// Function to send a file to the client using the given socket
void sendFile(const string& filename, int client_socket) {
	const int BUFFER_SIZE = 8192;
	char buffer[8192] = { 0 };
	ifstream ifile(filename, ios::binary);

	// Read the file in chunks and send it to the client
	while (true) {
		// Make the buffer zero before reading
		memset(buffer, 0, BUFFER_SIZE);

		ifile.read(buffer, BUFFER_SIZE);
		int bytes_read = ifile.gcount();

		if (bytes_read <= 0) {
			log("End of file", INFO_LOG);
			break;
		}

		if (send(client_socket, buffer, bytes_read, 0) == SOCKET_ERROR) {
			log("Send failed with error: " + to_string(WSAGetLastError()), ERROR_LOG);
			break;
		}

		log("Sent " + to_string(bytes_read) + " bytes", INFO_LOG);
	}

	log("File sent successfully", SUCCESS_LOG);
}

// Function to get the root directory of the project
string getProjectRootDirectory() {
	string fullPath = __FILE__;
	size_t pos = fullPath.find_last_of("\\/");
	if (pos != string::npos) {
		return fullPath.substr(0, pos);
	}
	return fullPath;
}

// Function to convert a string to lowercase
string toLowercase(const string& str) {
	string result = str;
	transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

void log(string message, string level) {
	string prefix;
	if (level == INFO_LOG) {
		prefix = "[ > ] : ";
	}
	else if (level == SUCCESS_LOG) {
		prefix = "[ + ] : ";
	}
	else if (level == WARNING_LOG) {
		prefix = "[ * ] : ";
	}
	else if (level == ERROR_LOG) {
		prefix = "[ ! ] : ";
	}

	cout << level_colors[level] << prefix << message << RESET << endl;
}