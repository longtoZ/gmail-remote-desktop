#pragma once

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "user32.lib")

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <direct.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;
using namespace chrono;

typedef unsigned char uchar;
const string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Text format
const string RESET = "\033[0m";
const string BOLD = "\033[1m";
const string UNDERLINE = "\033[4m";

// Text color
const string GREEN = "\x1b[38;5;10m";
const string YELLOW = "\x1b[38;5;11m";
const string RED = "\x1b[38;5;9m";
const string WHITE = "\x1b[38;5;15m";

const string INFO_LOG = "INFO";
const string SUCCESS_LOG = "SUCCESS";
const string WARNING_LOG = "WARNING";
const string ERROR_LOG = "ERROR";

string get_current_time();
int get_file_size(const char* filename);
string get_file_path(const char* filename);
string trim(const string& str);
json convert_body_to_json(string body);
vector<string> split_string(const string& str, char delimiter);
string add_ending_backslash(string path);
string base64_encode(const string& in);
vector<unsigned char> gmail_base64_decode(const string& encoded_string);
size_t write_callback(void* ptr, size_t size, size_t nmemb, string* data);
void send_response_with_file(SOCKET server_socket, json response);
void sendFile(const string& filename, int client_socket);
string getProjectRootDirectory();
string toLowercase(const string& str);
void log(string message, string level = "INFO");