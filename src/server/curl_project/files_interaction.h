#pragma once

#include "utils.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <array>
#include <string>
#include <windows.h>
#include <direct.h>
#include <curl/curl.h>

class Files {
private:
	int server_socket;
	const int BUFFER_SIZE = 8192;
	char buffer[8192] = { 0 };

	// Function to convert forward slashes to backslashes (since Windows uses backslashes in file paths)
	string convert_to_backslashes(string str);

	// Function to convert /Date(1620000000000)/ to a readable date format
	string convert_date_format(string date);

	// Function to convert the length of a file (in bytes) to a human-readable format
	string convert_file_size(long long length);

	// Function to determine an item is a file or a folder
	string get_item_type(bool psiscontainer);

	string get_mode_type(string mode);

public:
	Files(int server_socket) {
		this->server_socket = server_socket;
	}

	// Function to create a file
	json create_file(const char* filename);

	// Function to create a folder
	json create_folder(const char* foldername);

	// Function to move a file
	json move_file(const char* old_filename, const char* new_filename);

	// Function to move a folder
	json move_folder(const char* old_foldername, const char* new_foldername);

	// Function to copy a file
	json copy_file(const char* old_filename, const char* new_filename);

	// Function to copy a folder by executing shell command instead of using C++ code
	json copy_folder(const char* old_foldername, const char* new_foldername);

	json delete_file(const char* filename);

	// Function to delete a folder (using the same method as copying a folder)
	json delete_folder(const char* foldername);

	// Function to write the JSON content to a CSV file
	bool save_contents_to_csv(const char* filename, const string& result);

	// Function to list the contents of a folder
	void list_contents(const char* foldername);

	// Function to save an attachment to a file
	json save_attachment(const string& attachment, const string& path);

	// Function to send a local file to the client
	void get_files(vector<string>& paths);
};
