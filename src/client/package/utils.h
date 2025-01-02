#pragma once

#include <socket_client.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <sstream>
#include <regex>

using namespace std;
using json = nlohmann::json;

// Function to read JSON data from a file
json read_json(const string& filename) {
    ifstream f(filename);
    json data;
    if (f.is_open()) {
        data = json::parse(f);
        // cout << "Read file successfully!" << endl;
    } else {
        cerr << "Failed to open file" << endl;
    }

    f.close();

    return data;
}

// Function to write JSON data to a file
void write_json(json data, const string& filename) {
    ofstream f(filename);

    if (f.is_open()) {
        f << data.dump(4);
        f.close();
        cout << "Write to file successfully!" << endl;
    } else {
        cerr << "Failed to open file" << endl;
    }

}

void receive_file(const string& file_path, int file_size, int client_socket) {
    char buffer[BUFFER_SIZE] = {0};

    ofstream ofile(file_path, ios::binary);

    if (!ofile.is_open()) {
        cerr << "Failed to open file" << endl;
        return;
    }

    // Receive the file in chunks
    int bytes_received = 0;
    int total_bytes_received = 0;

    while (total_bytes_received < file_size) {
        int bytes_to_receive = min(BUFFER_SIZE, file_size - total_bytes_received);
        // cout << "Need to receive " << bytes_to_receive << " bytes" << endl;
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, buffer, bytes_to_receive, 0);
        total_bytes_received += bytes_received;

        if (bytes_received < 0) {
            cerr << "Receive failed" << endl;
            break;
        }

        ofile.write(buffer, bytes_received);
        cout << "Total received " << total_bytes_received << " bytes" << endl;
    }

    ofile.close();
}

string getJsonField(const json& list, const string& field) {
    for (auto& item : list) {
        if (item.contains("name") && item["name"] == field) {
            return item["value"];
        }
    }

    return "";
}

unordered_map<string, string> getDevicesInNetwork() {
    unordered_map<string, string> macToIpMap;

    // Command to get the default network range
    string getNetworkCommand = "ip route | grep default | awk '{print $3}' | head -1";

    cout << "Determining the network range..." << endl;

    FILE* netFp = popen(getNetworkCommand.c_str(), "r");
    if (netFp == nullptr) {
        cerr << "Failed to determine the network range." << endl;
        return macToIpMap;
    }

    char netBuffer[1024];
    string network;
    if (fgets(netBuffer, sizeof(netBuffer), netFp) != nullptr) {
        network = netBuffer;
        network.erase(network.find_last_not_of(" \n\r\t") + 1); // Trim whitespace
    }
    pclose(netFp);

    if (network.empty()) {
        cerr << "Network range could not be determined." << endl;
        return macToIpMap;
    }

    // Convert the last byte of the IP address to 0
    network = network.substr(0, network.find_last_of(".")) + ".0";

    cout << "Network range: " << network << endl;
    string command = "sudo nmap -sn " + network + "/24 | grep 'Nmap scan report for' -A 3";

    cout << "Running nmap command to get devices in network..." << endl;

    FILE* fp = popen(command.c_str(), "r");
    if (fp == nullptr) {
        cerr << "Failed to run nmap command." << endl;
        return macToIpMap;
    }

    char buffer[1024];
    string output;

    // Read the output from nmap into a string
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        output += buffer;
    }
    fclose(fp);

    cout << "Devices MAC and IP addresses collected successfully!" << endl;

    // Regular expression to match IP and MAC addresses
    regex ip_regex("Nmap scan report for (\\S+)");
    regex mac_regex("MAC Address: ([0-9a-fA-F:]{17})");

    // Process the output to extract IP and MAC pairs
    smatch ip_match, mac_match;
    stringstream ss(output);
    string line;

    // Get the IP line
    while (getline(ss, line)) {
        if (regex_search(line, ip_match, ip_regex)) {
            string ip = ip_match[1];
            getline(ss, line); // Ignore the latency line
            getline(ss, line); // Get the MAC line
            if (regex_search(line, mac_match, mac_regex)) {
                string mac = mac_match[1];
                macToIpMap[mac] = ip; // Store MAC as key and IP as value
            }
        }
    }

    return macToIpMap;
}

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