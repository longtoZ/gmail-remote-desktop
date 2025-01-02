#include "./package/socket_thread.h"

#define USER_FILENAME "./json/users.json"

void clientThread(const string& ip_address, int port, const string& from_email, const string& to_email) {
    try {
        Client client(ip_address, port, from_email, to_email);
        client.connect_to_server();
        client.run();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}

int main() {
    json users = read_json(USER_FILENAME);

    // // Get the MAC to IP mapping for all devices in the network
    // unordered_map<string, string> macToIpMap = getDevicesInNetwork();

    // for (auto& [mac, ip] : macToIpMap) {
    //     cout << "MAC: " << mac << " -> IP: " << ip << endl;
    // }

    // // Update the IP address for each user
    // for (auto& [email, machine] : users.items()) {
    //     string mac_address = machine["MAC_ADDRESS"];
    //     if (macToIpMap.find(mac_address) != macToIpMap.end()) {
    //         machine["IP_ADDRESS"] = macToIpMap[mac_address];
    //     }
    // }

    // Start a client thread for each active user
    vector<thread> threads;
    for (auto& [email, machine] : users.items()) {
        if (machine["STATUS"] == "active") {
            string ip_address = machine["IP_ADDRESS"];
            int port = machine["PORT"];
            cout << "Starting client thread for " << email << " at " << ip_address << ":" << port << endl;
            threads.push_back(thread(clientThread, ip_address, port, "thanhlongpython@gmail.com", email));
        }
    }

    for (auto& thread : threads) {
        thread.join();
    }


    // g++ -I ./package/ ./package/*.cpp main.cpp -o main -lcurl

    return 0;
}