#pragma once

#include "utils.h"

struct App {
	string processName;
	string status;
	string company;
	int processCount;
	float memory;
	string path;
};

struct Service {
	string name;
	string status;
	string startMode;
	float memory;
	string path;
};

class AppService {
private:
	int client_socket;
	const int BUFFER_SIZE = 8192;
	char buffer[8192] = { 0 };
	unordered_map<string, vector<string>> processPaths;

	// Function to sort apps based on a given criterion and order
	void sortApps(vector<App>& apps, const string& sortCriterion, const string& order);

	// Function to sort services based on a given criterion and order
	void sortServices(vector<Service>& services, const string& sortCriterion, const string& order);

	// Function to return a vector of the output string of the Get-Process command
	vector<App> parseAppOutput(const string& output);

	// Function to get running apps
	string getRunningApps();

	// Function to return a vector of the output string of the Get-WmiObject command
	vector<Service> parseServiceOutput(const string& output);

	// Function to get running/stopped services
	string getServices(string status);

	// Function to save apps to a CSV file
	void saveAppsToCSV(const vector<App>& apps, const string& filename);

	// Function to save services to a CSV file
	void saveServicesToCSV(const vector<Service>& services, const string& filename);

	// Function to get all executable paths and save them to a CSV file
	void getAllExecutablePaths(string filename);

	// Function to fill the processPaths map
	void loadMap(string filename);

	// Function to start an app using its executable path
	void startProcess(string name);

	// Function to stop an app using its process name
	void stopProcess(string name);

public:
	AppService(int client_socket) : client_socket(client_socket) {
		loadMap("ExecutablePaths.csv");
	}

	void listApps(json& params);

	void listServices(json& params);

	void list(json& params);

	void start(json& params);

	void stop(json& params);
};