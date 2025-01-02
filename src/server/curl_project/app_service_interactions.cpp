#include "app_service_interactions.h"

// Function to sort apps based on a given criterion and order
void AppService::sortApps(vector<App>& apps, const string& sortCriterion, const string& order) {
	// Define comparators for each sort criterion
	unordered_map<string, function<bool(const App&, const App&)>> comparators = {
		{"memory", [](const App& a, const App& b) { return a.memory < b.memory; }},
		{"processCount", [](const App& a, const App& b) { return a.processCount < b.processCount; }},
		{"processName", [](const App& a, const App& b) { return a.processName < b.processName; }},
		{"status", [](const App& a, const App& b) { return a.status < b.status; }},
		{"company", [](const App& a, const App& b) { return a.company < b.company; }},
		{"path", [](const App& a, const App& b) { return a.path < b.path; }}
	};

	// Check if the sortCriterion exists in the map
	auto it = comparators.find(sortCriterion);
	if (it != comparators.end()) {
		auto& comparator = it->second;

		// Apply the order (ascending or descending)
		sort(apps.begin(), apps.end(), comparator);
		if (order == "asc") {
		}
		else {
			reverse(apps.begin(), apps.end());
		}
	}
	else {
		log("Invalid sort keyword: " + sortCriterion, ERROR_LOG);
	}
}

// Function to sort services based on a given criterion and order
void AppService::sortServices(vector<Service>& services, const string& sortCriterion, const string& order) {
	// Define comparators for each sort criterion
	unordered_map<string, function<bool(const Service&, const Service&)>> comparators = {
		{"memory", [](const Service& a, const Service& b) { return a.memory < b.memory; }},
		{"name", [](const Service& a, const Service& b) { return a.name < b.name; }},
		{"status", [](const Service& a, const Service& b) { return a.status < b.status; }},
		{"startMode", [](const Service& a, const Service& b) { return a.startMode < b.startMode; }},
		{"path", [](const Service& a, const Service& b) { return a.path < b.path; }}
	};

	// Check if the sortCriterion exists in the map
	auto it = comparators.find(sortCriterion);
	if (it != comparators.end()) {
		auto& comparator = it->second;

		// Apply the order (ascending or descending)
		sort(services.begin(), services.end(), comparator);
		if (order == "asc") {
		}
		else {
			reverse(services.begin(), services.end());
		}
	}
	else {
		log("Invalid sort keyword: " + sortCriterion, ERROR_LOG);
	}
}

// Function to return a vector of the output string of the Get-Process command
vector<App> AppService::parseAppOutput(const string& output) {
	vector<App> apps;
	istringstream stream(output);
	string line;

	App currentApp;
	bool isNewApp = false;

	while (getline(stream, line)) {
		// Skip empty lines
		if (line.empty()) continue;

		// Find the position of the colon
		size_t colonPos = line.find(':');
		if (colonPos == string::npos) continue;

		// Extract field name and value
		string fieldName = line.substr(0, colonPos);
		string value = line.substr(colonPos + 1);

		// Trim whitespace from field name and value
		fieldName.erase(0, fieldName.find_first_not_of(" \t"));
		fieldName.erase(fieldName.find_last_not_of(" \t") + 1);
		value.erase(0, value.find_first_not_of(" \t"));
		value.erase(value.find_last_not_of(" \t") + 1);

		if (fieldName == "processName") {
			// If we were already processing an app, save it
			if (isNewApp) {
				apps.push_back(currentApp);
			}
			currentApp = App();
			currentApp.processName = value;
			currentApp.status = "Running";
			isNewApp = true;
		}
		else if (fieldName == "Company") {
			currentApp.company = value;
		}
		else if (fieldName == "ProcessCount") {
			currentApp.processCount = stoi(value);
		}
		else if (fieldName == "Memory") {
			currentApp.memory = stod(value);
		}
		else if (fieldName == "cpuUsage") {
			currentApp.memory = stod(value);
		}
		else if (fieldName == "Path") {
			currentApp.path = value;
		}
	}

	if (isNewApp) {
		apps.push_back(currentApp);
	}

	return apps;
}

// Function to get running apps
string AppService::getRunningApps() {
	string command =
		"powershell.exe -Command"
		" \"& {"
		"Get-Process | Where-Object { $_.SessionId -ne 0 } |"
		"Group-Object processName |"
		"ForEach-Object {"
		"   $firstProcess = $_.Group | Select-Object -First 1;"
		"   [PSCustomObject]@{"
		"       processName = $_.Name;"
		"       Company = $firstProcess.Company;"
		"       ProcessCount = $_.Count;"
		"       Path = $firstProcess.Path;"
		"       Memory = [Math]::Round(($_.Group | Measure-Object -Property WorkingSet -Sum).Sum / 1MB, 2)"
		"   }"
		"} |"
		"Sort-Object memory -Descending |"
		"Select-Object processName, Company, ProcessCount, Memory, Path"
		"}\"";

	FILE* pipe = _popen(command.c_str(), "r");

	if (!pipe) {
		return "Error: Failed to open pipe\n";
	}

	string result;
	char buffer[1024];

	while (!feof(pipe)) {
		if (fgets(buffer, 1024, pipe) != NULL) {
			result += buffer;
		}
	}

	_pclose(pipe);

	return result;
}

// Function to return a vector of the output string of the Get-WmiObject command
vector<Service> AppService::parseServiceOutput(const string& output) {
	vector<Service> services;
	istringstream stream(output);
	string line;

	Service currentService;
	bool isNewService = false;

	while (getline(stream, line)) {
		// Skip empty lines
		if (line.empty()) continue;

		// Find the position of the colon
		size_t colonPos = line.find(':');
		if (colonPos == string::npos) continue;

		// Extract field name and value
		string fieldName = line.substr(0, colonPos);
		string value = line.substr(colonPos + 1);

		// Trim whitespace from field name and value
		fieldName.erase(0, fieldName.find_first_not_of(" \t"));
		fieldName.erase(fieldName.find_last_not_of(" \t") + 1);
		value.erase(0, value.find_first_not_of(" \t"));
		value.erase(value.find_last_not_of(" \t") + 1);

		if (fieldName == "Name") {
			// If we were already processing an app, save it
			if (isNewService) {
				services.push_back(currentService);
			}
			currentService = Service();
			currentService.name = value;
			isNewService = true;
		}
		else if (fieldName == "Status") {
			currentService.status = value;
		}
		else if (fieldName == "StartMode") {
			currentService.startMode = value;
		}
		else if (fieldName == "Memory") {
			currentService.memory = stod(value);
		}
		else if (fieldName == "PathName") {
			currentService.path = value;
		}
	}

	if (isNewService) {
		services.push_back(currentService);
	}

	return services;
}

// Function to get running/stopped services
string AppService::getServices(string status) {
	string filter;

	if (status == "running") {
		filter = "Where-Object { $_.State -eq 'Running' } |";
	}
	else if (status == "stopped") {
		filter = "Where-Object { $_.State -ne 'Running' } |";
	}

	string command =
		"powershell.exe -Command"
		" \"& {"
		"Get-WmiObject -Class Win32_Service | " + filter + " ForEach-Object { "
		"$process = Get-Process -Id $_.ProcessId -ErrorAction SilentlyContinue; "
		"[PSCustomObject]@{ "
		"Name = $_.Name; "
		"Status = $_.State; "
		"StartMode = $_.StartMode; "
		"PathName = $_.PathName; "
		"Memory = if ($process) { [Math]::Round($process.WorkingSet64 / 1MB, 2) } else { 'N/A' }; "
		"} } | Select-Object Name, Status, StartMode, Memory, PathName"
		"}\"";


	FILE* pipe = _popen(command.c_str(), "r");

	if (!pipe) {
		return "Error: Failed to open pipe\n";
	}

	string result;
	char buffer[1024];

	while (!feof(pipe)) {
		if (fgets(buffer, 1024, pipe) != NULL) {
			result += buffer;
		}
	}

	_pclose(pipe);

	return result;
}

// Function to save apps to a CSV file
void AppService::saveAppsToCSV(const vector<App>& apps, const string& filename) {
	ofstream file(filename);

	if (!file.is_open()) {
		log("Error: Could not open file " + filename, ERROR_LOG);
		return;
	}

	file << "processName,Status,Company,ProcessCount,Memory,Path" << endl;

	for (const App& app : apps) {
		file << app.processName << ","
			<< app.status << ","
			<< app.company << ","
			<< app.processCount << ","
			<< app.memory << ","
			<< app.path << endl;
	}

	file.close();
	log("Apps saved to: " + filename, SUCCESS_LOG);
}

// Function to save services to a CSV file
void AppService::saveServicesToCSV(const vector<Service>& services, const string& filename) {
	ofstream file(filename);

	if (!file.is_open()) {
		log("Error: Could not open file " + filename, ERROR_LOG);
		return;
	}

	file << "Name,Status,StartMode,Memory,Path" << endl;

	for (const Service& service : services) {
		file << service.name << ","
			<< service.status << ","
			<< service.startMode << ","
			<< service.memory << ","
			<< service.path << endl;
	}

	file.close();
	log("Services saved to: " + filename, SUCCESS_LOG);
}

// Function to get all executable paths and save them to a CSV file
void AppService::getAllExecutablePaths(string filename) {
	string command = "powershell -Command \""
		"Get-ChildItem "
		"-Path 'C:\\Program Files', 'C:\\Program Files (x86)', 'C:\\Windows', \"C:\\Users\\$env:USERNAME\\AppData\", 'D:\\' "
		"-Recurse "
		"-Filter *.exe "
		"-ErrorAction SilentlyContinue | "
		"Select-Object "
		"@{Name=\\\"Name\\\"; Expression={$_.Name}}, "
		"@{Name=\\\"Path\\\"; Expression={$_.FullName}} | "
		"Export-Csv "
		"-Path '" + filename + "' "
		"-NoTypeInformation "
		"-Force\"";

	int result = system(command.c_str());

	if (result == 0) {
		log("Executable paths saved to: " + filename, SUCCESS_LOG);
	}
	else {
		log("Error saving executable paths", ERROR_LOG);
	}
}

// Function to fill the processPaths map
void AppService::loadMap(string filename) {
	ifstream file(filename);

	// Check if the file exists
	if (!file.good()) {
		log("Error: File " + filename + " does not exist. Creating a new one.", ERROR_LOG);
		getAllExecutablePaths(filename);
		file.open(filename);
	}

	if (!file.is_open()) {
		log("Error: Could not open file " + filename, ERROR_LOG);
		return;
	}

	string line;
	string processName;
	string path;

	// Skip the header line
	getline(file, line);

	while (getline(file, line)) {
		processName = toLowercase(line.substr(1, line.find(",") - 6));
		path = line.substr(line.find(",") + 1);

		processPaths[processName].push_back(path);

	}

	file.close();

	log("Map loaded successfully", SUCCESS_LOG);
}

// Function to start an app using its executable path
void AppService::startProcess(string name) {
	// Start the app using the first path that works
	cout << "Starting app: " << name << " - " << processPaths[name].size() << " paths" << endl;

	for (const string& path : processPaths[name]) {
		string command = "powershell -Command \"Start-Process -FilePath '" + path + "'\"";
		cout << command << endl;

		try {
			int result = system(command.c_str());

			if (result == 0) {
				log("App started successfully: " + path, SUCCESS_LOG);
				//break;
			}
			else {
				log("App could not be started: " + path, ERROR_LOG);
			}
		}
		catch (exception e) {
			log("Error starting app: " + path, ERROR_LOG);
		}

	}

}

// Function to stop an app using its process name
void AppService::stopProcess(string name) {
	// Stop the app using the first path that works
	for (const string& path : processPaths[name]) {
		// Stop an app using admin privileges
		cout << "Stopping app: " << path << endl;
		string command = "powershell -Command \"Get-Process | Where-Object { $_.Path -eq '" + path + "' } | Stop-Process -Force\"";
		int result = system(command.c_str());

		if (result == 0) {
			log("App stopped successfully: " + path, SUCCESS_LOG);
			//break;
		}
		else {
			log("App could not be stopped: " + path, ERROR_LOG);
		}
	}

}

void AppService::listApps(json& params) {
	string sortCriterion = params.contains("sort") ? params["sort"] : "";
	string order = params.contains("order") ? params["order"] : "";
	vector<App> apps = parseAppOutput(getRunningApps());

	if (sortCriterion != "") {
		sortApps(apps, sortCriterion, order);
	}

	string filename = "apps_" + get_current_time() + ".csv";
	saveAppsToCSV(apps, filename);

	json response = {
		{"cmd", "listas"},
		{"body", {
			{"status", "sending"},
			{"filename", string(filename)},
			{"path", get_file_path(filename.c_str())},
			{"bytes", get_file_size(filename.c_str())},
		}}
	};

	string responseString = response.dump();
	memset(buffer, 0, BUFFER_SIZE);
	send(client_socket, responseString.c_str(), BUFFER_SIZE, 0);

	sendFile(filename, client_socket);
	log("Apps sent successfully!", SUCCESS_LOG);
}

void AppService::listServices(json& params) {
	string status = params.contains("status") ? params["status"] : "";
	string sortCriterion = params.contains("sort") ? params["sort"] : "";
	string order = params.contains("order") ? params["order"] : "";

	vector<Service> services = parseServiceOutput(getServices(status));

	if (sortCriterion != "") {
		sortServices(services, sortCriterion, order);
	}

	string filename = "services_" + get_current_time() + ".csv";
	saveServicesToCSV(services, filename);

	json response = {
		{"cmd", "listas"},
		{"body", {
			{"status", "sending"},
			{"filename", string(filename)},
			{"path", get_file_path(filename.c_str())},
			{"bytes", get_file_size(filename.c_str())},
		}}
	};

	string responseString = response.dump();
	memset(buffer, 0, BUFFER_SIZE);
	send(client_socket, responseString.c_str(), BUFFER_SIZE, 0);

	sendFile(filename, client_socket);
	log("Services sent successfully!", SUCCESS_LOG);
}

void AppService::list(json& params) {
	if (params["type"] == "app") {
		listApps(params);
	}
	else if (params["type"] == "service") {
		listServices(params);
	}
	else {
		log("Invalid type: " + params["type"], ERROR_LOG);
	}
}

void AppService::start(json& params) {
	vector<string> processNames = split_string(params["names"], ',');

	for (const string& processName : processNames) {
		startProcess(toLowercase(processName));
	}

	json response = {
		{"cmd", "startas"},
		{"body", {
			{"status", "success"}
		}}
	};

	string responseString = response.dump();
	memset(buffer, 0, BUFFER_SIZE);
	cout << responseString << endl;
	send(client_socket, responseString.c_str(), BUFFER_SIZE, 0);
}

void AppService::stop(json& params) {
	vector<string> processNames = split_string(params["names"], ',');

	for (const string& processName : processNames) {
		stopProcess(toLowercase(processName));
	}

	json response = {
		{"cmd", "stopas"},
		{"body", {
			{"status", "success"}
		}}
	};

	string responseString = response.dump();
	memset(buffer, 0, BUFFER_SIZE);
	send(client_socket, responseString.c_str(), BUFFER_SIZE, 0);
}