#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <curl/curl.h>
#include "./utils.h"

#define TOKEN_FILENAME "./json/token.json"
#define DATA_FILENAME "./json/data.json"

// Function to convert a string to base64 encoding, which is the content format required by Gmail API
typedef unsigned char uchar;
const string b = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
string base64_encode(const string& in) {
    string out;

    int val = 0, valb = -6;
    for (uchar c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(b[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(b[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

// Function to convert base64 encoding back to string
string base64_decode(const string& in) {
    string out;

    vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[b[i]] = i;

    int val = 0, valb = -8;
    for (uchar c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

// Callback function for libcurl to handle response
size_t write_callback(void* ptr, size_t size, size_t nmemb, string* data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}


// Function to update the access_token using the refresh_token
// Since the access_token expires after 1 hour, we need to update it periodically using the refresh_token
void update_token() {
    cout << TOKEN_FILENAME << endl;
    json old_data = read_json(TOKEN_FILENAME);

    // Setup cURL request to update access_token
    CURL *curl;
    CURLcode res;
    string response_string;

    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        string client_id = old_data["client_info"]["client_id"];               
        string client_secret = old_data["client_info"]["client_secret"];
        string refresh_token = old_data["token_info"]["refresh_token"];
        string refresh_token_url = "https://oauth2.googleapis.com/token";

        // Prepare the URL and POST fields
        string post_fields = "client_id=" + client_id +
                                  "&client_secret=" + client_secret +
                                  "&refresh_token=" + refresh_token +
                                  "&grant_type=refresh_token";

        // Set the URL for the POST request
        curl_easy_setopt(curl, CURLOPT_URL, refresh_token_url.c_str());
        
        // Set the POST fields for the request
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

        // Set the write callback to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        // Perform the request
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed at update: " << curl_easy_strerror(res) << endl;
        } else {
            // Log the response
            cout << "Response from server: " << response_string << endl;

            // When we get the new access_token, we update the "access_token" field in the JSON data 
            // and write it back to the file
            
            json new_data = json::parse(response_string);
            old_data["token_info"]["access_token"] = new_data["access_token"];

            write_json(old_data, TOKEN_FILENAME);
        }

        // Clean up
        curl_easy_cleanup(curl);
    }

    // Cleanup global curl environment
    curl_global_cleanup();
}

// Function to send an email using Gmail API (POST request)
// Since "smtp.gmail.com" requires app-specific password, which is only performed by real users, 
// we use cURL (with OAuth 2.0) to interact with Gmail API. It's the only way to send email programmatically in C++.
void send_mail(const string& from_email, const string& to_email, const string& subject, const json& body, const string& file_type, const string& file_path) {
    json data = read_json(TOKEN_FILENAME);

    // OAuth 2.0 Access Token
    string access_token = data["token_info"]["access_token"];

    // Sender and Receiver Emails
    // string from_email = data["body_info"]["from_email"];
    // string to_email = data["body_info"]["to_email"];

    // Gmail API URL to send the email
    string gmail_api_url = "https://gmail.googleapis.com/gmail/v1/users/me/messages/send";

    // MIME Boundary
    string boundary = "bG9uZ3RvaXNoZXJl";

    // Create the email content in MIME format
    stringstream email_content;

    // MIME Headers
    email_content << "From: " << from_email << "\n";
    email_content << "To: " << to_email << "\n";
    email_content << "Subject: " << subject << "\n";
    email_content << "MIME-Version: 1.0\n";
    email_content << "Content-Type: multipart/mixed; boundary=" << boundary << "\n\n";

    // Plain text email body (first part)
    email_content << "--" << boundary << "\n";
    email_content << "Content-Type: text/plain; charset=\"UTF-8\"\n\n";
    email_content << body.dump(4) << "\n\n";

    if (!file_type.empty()) {
        cout << "Attaching file: " << file_path << endl;
        // Attach the image file (second part)

        // Content-Type for the attachment
        unordered_map<string, string> content_types = {
            {"png", "image/png"},
            {"zip", "application/zip"},
            {"csv", "text/csv"},
        };

        ifstream file(file_path, ios::binary);
        if (!file.is_open()) {
            cerr << "Error: Could not open the file " << file_path << endl;
            return;
        }

        stringstream attachment_data;
        attachment_data << file.rdbuf();  // Read file content into a stringstream
        file.close();

        // Base64-encode the binary data
        string encoded_attachment = base64_encode(attachment_data.str());

        // Modify the filename's extension
        string filename = string(body["body"]["filename"]).substr(0, string(body["body"]["filename"]).find_last_of('.')) + "." + file_type;

        // Attach the file (MIME format for attachments)
        email_content << "--" << boundary << "\n";
        email_content << "Content-Type: " << content_types[file_type] << "; name=\"" << file_path << "\"\n";
        email_content << "Content-Transfer-Encoding: base64\n";
        email_content << "Content-Disposition: attachment; filename=\"" << filename << "\"\n\n";
        email_content << encoded_attachment << "\n\n";
    }

    // End the MIME message
    email_content << "--" << boundary << "--\n";

    // Encode the email content in base64 (for Gmail API)
    string base64_email = base64_encode(email_content.str());

    // Setup cURL request to send mail
    CURL* curl;
    CURLcode res;
    string response_string;

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Set the URL for the POST request
        curl_easy_setopt(curl, CURLOPT_URL, gmail_api_url.c_str());

        // Set headers for Authorization and Content-Type
        struct curl_slist* headers = NULL;
        string auth_header = "Authorization: Bearer " + access_token;
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        // Set the headers
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Prepare the JSON payload
        string json_data = "{\"raw\":\"" + base64_email + "\"}";

        // Set the payload for the POST request
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());

        // Set the write callback to handle the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        else {
            cout << "Response from Gmail API: " << response_string << endl;
        }

        // Clean up
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    // Cleanup global libcurl environment
    curl_global_cleanup();
}

// Function to fetch emails from Gmail API (GET request)
// Same as sending email, we use cURL (with OAuth 2.0) to fetch emails from client's Gmail account 
// (in this case, the account is "thanhlongpython@gmail.com"). The response will be a list of JSON objects, 
// each of which contains id and threadId of the email.
json fetch_mails(const string& email) {
    json data = read_json(TOKEN_FILENAME);

    // The URL endpoint for Gmail API. Some parameters are used to filter the emails.
    string url = "https://gmail.googleapis.com/gmail/v1/users/me/messages?q=from:" + email + "&labelIds=INBOX&maxResults=5";

    // OAuth 2.0 Bearer Access Token
    string access_token = data["token_info"]["access_token"];

    CURL* curl;
    CURLcode res;

    // The response will be stored in this string
    string response_string;

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Set the URL for the GET request
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Create the Authorization header
        string auth_header = "Authorization: Bearer " + access_token;
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        // Set the headers
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the callback function to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        // Perform the GET request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        } else {
            // Output the response
            cout << "[" << email << "] " << "Response: " << response_string.substr(0, 50) << "..." << endl;

            // The response will be processed later...
        }

        // Clean up
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    // Cleanup global libcurl environment
    curl_global_cleanup();

    return json::parse(response_string);
}


// Function to parse a specific email by its id
// The id is the unique identifier of the email, which is returned in the response of the fetch_mails() function.
// The response will be a JSON object containing a lot of information about the email, such as sender, receiver, subject, body, etc.
json parse_mail(const string& id) {
    json data = read_json(TOKEN_FILENAME);

    // The URL endpoint for Gmail API
    string url = "https://gmail.googleapis.com/gmail/v1/users/me/messages/" + id;

    // OAuth 2.0 Bearer Access Token
    string access_token = data["token_info"]["access_token"];

    CURL* curl;
    CURLcode res;

    // The response will be stored in this string
    string response_string;

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Set the URL for the GET request
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Create the Authorization header
        string auth_header = "Authorization: Bearer " + access_token;
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        // Set the headers
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the callback function to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        // Perform the GET request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        } else {
            // Output the response
            cout << "Response: " << response_string << endl;

            // The response will be processed later...
        }

        // Clean up
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    // Cleanup global libcurl environment
    curl_global_cleanup();

    return json::parse(response_string);
}

string getEpochTime() {
    auto now = chrono::system_clock::now();
    time_t now_c = chrono::system_clock::to_time_t(now);
    return to_string(now_c);
}

json generate_command(const string& email) {
    json mails = fetch_mails(email);
    json j = {
        {"status", "failed"},
        {"from", ""},
        {"time", ""},
        {"subject", ""},
        {"body", ""},
    };

    if (mails["messages"].size() > 0) {
        json email_data = read_json(DATA_FILENAME);
        json token_data = read_json(TOKEN_FILENAME);

        string latest_id = mails["messages"][0]["id"];
        string current_id = email_data[email]["latest_mail_id"];

        if (latest_id != current_id) {
            json mail = parse_mail(latest_id);

            string from = getJsonField(mail["payload"]["headers"], "From");
            string time = getJsonField(mail["payload"]["headers"], "Date");
            string subject = getJsonField(mail["payload"]["headers"], "Subject");
            string body = "";
            string attachment_data;

            // Check if the email has attachments
            if (mail["payload"]["parts"][0]["body"]["data"].is_null()) {
                json attachment_json_data = {
                    {"access_token", token_data["token_info"]["access_token"]},
                    {"email_id", latest_id},
                    {"attachment_id", mail["payload"]["parts"][1]["body"]["attachmentId"]},
                    {"attachment_name", mail["payload"]["parts"][1]["filename"]},
                };
                body = base64_decode(mail["payload"]["parts"][0]["parts"][0]["body"]["data"]);
                attachment_data = attachment_json_data.dump();
            
            // If the email has no attachments
            } else {
                body = base64_decode(mail["payload"]["parts"][0]["body"]["data"]);
            }

            j["status"] = "success";
            j["from"] = from;
            j["time"] = time;
            j["subject"] = subject;
            j["body"] = body;
            j["attachment"] = attachment_data;

            email_data[email]["latest_mail_id"] = latest_id;
            write_json(email_data, DATA_FILENAME);

            return j;
        }
    }
    
    return j;
}