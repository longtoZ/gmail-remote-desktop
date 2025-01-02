#include "machine_interactions.h"

void Machine::sendMessageOnce(string cmd, int status, string filename) {
    json reply;
    reply["cmd"] = cmd;

    if (cmd == "listas") {
        int size = 0;
        ifstream input(getProjectRootDirectory() + "\\LIST_RECORD\\" + filename, ios::ate);
        size = input.tellg();
        input.close();
        reply["body"]["filename"] = filename;
        reply["body"]["path"] = getProjectRootDirectory() + "\\LIST_RECORD\\" + filename;
        reply["body"]["bytes"] = size;
    }

    reply["body"]["error_code"] = to_string(status);
    reply["body"]["status"] = status == 0 ? "success" : "failed";
    string data = reply.dump();
    cout << reply.dump(4) << endl;
    send(client_socket, data.c_str(), data.length(), 0);
}

// Shut down, restart and sleep device
void Machine::restartDeviceHelper(int& sleepTime) {
    string cmd = "powershell -Command \"Start-Sleep -Seconds " + to_string(sleepTime) + ";Restart-Computer -Force\"";
    int result = system(cmd.c_str());
}

void Machine::shutdownDeviceHelper(int& sleepTime) {
    //active shutdown
    string cmd = "powershell -Command \"Start-Sleep -Seconds " + to_string(sleepTime) + ";Stop-Computer -Force\"";
    int result = system(cmd.c_str());
}

void Machine::sleepDeviceHelper(int& sleepTime) {
    Sleep(sleepTime * 1000);
    //active sleep 
    string cmd = "rundll32.exe powrprof.dll,SetSuspendState 0,1,0";
    int result = system(cmd.c_str());
}

void Machine::restartDevice(json requestFromClient) {
    this->requestFromClient = requestFromClient;
    int status = 1;
    // send a message to confirm server recvievied request
    sendMessageOnce("restart", status);
    int sleepTime = stoi(requestFromClient["interval"].get<string>());
    // Start the function Restart Server
    restartDeviceHelper(sleepTime);
}

void Machine::shutdownDevice(json requestFromClient) {
    this->requestFromClient = requestFromClient;
    int status = 1;
    // send a message to confirm server recvievied request
    sendMessageOnce("shutdown", status);
	int sleepTime = stoi(requestFromClient["interval"].get<string>());
    cout << sleepTime << endl;
    shutdownDeviceHelper(sleepTime);
}

void Machine::sleepDevice(json requestFromClient) {
    this->requestFromClient = requestFromClient;
    int status = system("powercfg -h off");
    // send a message to confirm server recvievied request
    sendMessageOnce("sleep", status == 0 ? 1 : -303);
    int sleepTime = stoi(requestFromClient["interval"].get<string>());
    sleepDeviceHelper(sleepTime);
}