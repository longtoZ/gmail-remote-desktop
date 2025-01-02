#include <iostream>             // For input/output operations
#include <sys/socket.h>         // For socket(), connect(), send(), recv()
#include <arpa/inet.h>          // For sockaddr_in and inet_pton()
#include <unistd.h>             // For close()
#include <cstring>              // For memset()
#include <thread>               // For std::this_thread::sleep_for()     
#include <fstream>

#define BUFFER_SIZE 8192

using namespace std;