#pragma once

#include "utils.h"
#include <thread>
#include <atomic>
#include <opencv2/opencv.hpp>

class WebcamThread {
private:
    thread worker;
    json requestFromClient;
    atomic<bool> webcam_running{ false };
    int client_socket;
    const int BUFFER_SIZE = 8192;
    char buffer[8192] = { 0 };
    string folder_path;
	int interval = 1;

	// Function to send a message to the client
    void sendMessage(json message);

	// Function to compress a folder as a zip file
    bool zipFolder(string folder_path);

	// Function to create a reply message for the client
    json creatReplyMessageByWebcam(int status);

	// Function to create a folder for saving the webcam images/videos
    string creatFolderForSaving();

	// Function to capture a single photo from the webcam
    void webcamCapture();

	// Function to create a new thread that captures multiple photos from the webcam
    void webcamWorkerThread();

	// Function to create a new thread that captures multiple videos from the webcam
    void videoWorkerThread();

public:
	WebcamThread(int client_socket) : client_socket(client_socket) {
	}

    // Setter for interval
    void setInterval(int interval);

	// Function to start the webcam capture process
    void startCapture(json request, const string& type);

	// Function to start the webcam video recording process
    void startVideo(json request);

	// Function to stop the webcam capture/video recording process
    void stop();

	~WebcamThread() {
		stop();
	}

};