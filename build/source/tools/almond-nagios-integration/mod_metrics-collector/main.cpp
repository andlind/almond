#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <csignal>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include "util.h"
#include "logger.h"
#define PORT 8075

using namespace std;

vector<string> url_config;
vector<string> metrics;
int server_fd;
struct sockaddr_in address;
bool bDataCollected = false;
bool is_ready = false;

void send_socket_message(int socket);
void close_socket();
void populateMetrics();

void signalHandler(int signum) {
        cout << "Interrupt signal (" << signum << ") received." << endl;
	LOGGER->Log("WARNING: Interrupt signal (%d) received", signum);
	switch (signum) {
		case SIGINT:
			cout << "Caught interrupt from command line." << endl;
			break;
		case SIGHUP:
			cout << "Received interactive signal to quit program." << endl;
			break;
		case SIGQUIT:
			cout << "A termination request was sent to the program." << endl;
			break;
		case SIGTERM:
			cout << "Sigterm sent to the program." << endl;
			break;
	}
	close_socket();
	CLogger::GetLogger()->Log("INFO: Exiting application");
	cout << "Good bye..." << endl;
	exit(signum);
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string convertCAtoString(char* a) {
	string s(a);
	return s;
}

int readConfiguration(string conf_file) {
	ifstream file;
	file.open(conf_file);
	if (file.fail()) {
		cout << "Could not open config file." << endl;
		CLogger::GetLogger()->Log("ERROR: Could not open config file. Missing?");
		return 1;
	}
	string line;
        url_config.clear();
	while (!file.eof()) {
		getline(file, line);
		url_config.push_back(line);
        }
	file.close();
	for (auto file_line : url_config)
		cout << file_line << endl;
	return 0;
}

string getMetrics(string url) {
	CURL *curl_handle;
	CURLcode res;
	string readBuffer;
	struct MemoryStruct chunk;
	int data_collected = 0;
  	
	chunk.memory = (char*)malloc(1);
  	chunk.size = 0;
	curl_handle = curl_easy_init();
	cout << "Connecting to: " << url << endl;
	LOGGER->Log("INFO: Connecting to '%s'", url.c_str());
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
   	 	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
		curl_easy_setopt(curl_handle, CURLOPT_HTTP09_ALLOWED, 1L);
    		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    		res = curl_easy_perform(curl_handle);
	}
	if (res != CURLE_OK) {
		fprintf(stderr, "error: %s\n", curl_easy_strerror(res));
		CLogger::GetLogger()->Log("ERROR: Could not connect to url.");
		data_collected--;
	}
	else {
		//printf("Size: %lu\n", (unsigned long)chunk.size);
		//printf("Data: %s\n", chunk.memory);
		readBuffer = convertCAtoString(chunk.memory);
		data_collected++;
	}
	cout << "data_collected=" << data_collected << endl;
	curl_easy_cleanup(curl_handle);
	free(chunk.memory);
	if (data_collected <= 0) {
		cout << "Could not collect any metrics." << endl;
		CLogger::GetLogger()->Log("WARNING: Could not collect any metrics.");
		bDataCollected = false;
	}
	else bDataCollected = true;
	cout << "getMetrics = " << bDataCollected << endl;
	LOGGER->Log("INFO: Metrics size is now: %d", metrics.size());
	return readBuffer;
}

void initSocket(int port) {
    	int opt = 1;
    	int addrlen = sizeof(address);
	cout << "Port is set to :" << port << endl;
	LOGGER->Log("INFO: Port is set number: %d", port);

	CLogger::GetLogger()->Log("INFO: Initializing socket.");
    	if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        	perror("socket failed");
		CLogger::GetLogger()->Log("ERROR: Socket failed.");
        	exit(EXIT_FAILURE);
    	}
    	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt))) {
        	perror("setsockopt");
		CLogger::GetLogger()->Log("ERROR: Setsockopt failed.");
        	exit(EXIT_FAILURE);
    	}
    	address.sin_family = AF_INET;
    	address.sin_addr.s_addr = INADDR_ANY;
	if (port == 0) {
		port = PORT;
	}
	address.sin_port = htons(port);
    	if (bind(server_fd, (struct sockaddr*)&address,sizeof(address))< 0) {
        	perror("bind failed");
		CLogger::GetLogger()->Log("ERROR: Bind failed.");
        	exit(EXIT_FAILURE);
    	}
	is_ready = true;
}

void createSocket(int server_fd) {
	int new_socket, valread;
	char buffer[1024] = { 0 };
	int addrlen = sizeof(address);

	cout << "Listen" << endl;
	CLogger::GetLogger()->Log("INFO: Listening");
    	if (listen(server_fd, 3) < 0) {
        	perror("listen");
		CLogger::GetLogger()->Log("ERROR: Failed listening...");
		is_ready = false;
        	exit(EXIT_FAILURE);
    	}
    	if ((new_socket = accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen)) < 0) {
        	perror("accept");
		CLogger::GetLogger()->Log("ERROR: Failed accepting request.");
		is_ready = false;
        	exit(EXIT_FAILURE);
    	}
    	valread = read(new_socket, buffer, 1024);
	string sBuff = convertCAtoString(buffer);
	size_t found = sBuff.find("User-Agent:");
	found += 11;
	string sMes = sBuff.substr(found);
	int stop = sMes.find_first_of("\n");
	sMes = sMes.substr(0, stop);
	LOGGER->Log("INFO: Request received. Agent used:  %s", sMes.c_str());
    	printf("%s\n", buffer);
	send_socket_message(new_socket);
    	close(new_socket);
}

void send_socket_message(int socket) {
	char info[95] = "HTTP/1.1 200 OK\nContent-Type:application/txt\nContent-Length: 17\n\nNo metrics found\n";
	if (readConfiguration("urls.conf") == 0) {
                populateMetrics();
        }
	if (bDataCollected) {
		string header = "HTTP/1.1 200 OK\nContent-Type:application/txt\nContent-Length: ";
		string response = "\n\n";
		int response_length = 0;
		cout << "Metrics size is = " << metrics.size() << endl;
		LOGGER->Log("INFO: Metrics size is: %d", metrics.size());
		for (unsigned int i = 0; i < metrics.size(); i++) {
			response = response + metrics[i];
			response_length += metrics[i].length();
		}
		string message = header + to_string(response_length) + response;
		send(socket, message.c_str(), strlen(message.c_str()), 0);
	}
	else {
		cout << "data_collected = " << bDataCollected << endl;
		send(socket, info, strlen(info), 0);
	}
	metrics.clear();
	cout << "Metrics sent." << endl;
	CLogger::GetLogger()->Log("INFO: Metrics sent as requested.");
	LOGGER->Log("INFO: Metrics length is now %d", metrics.size());
}

void close_socket() {
	CLogger::GetLogger()->Log("INFO: Closing socket.");
	shutdown(server_fd, SHUT_RDWR);
}

void populateMetrics() {
	unsigned int vecSize = url_config.size();
	LOGGER->Log("INFO: Number of urls to query is %d", vecSize-1);
	metrics.clear();
	for (unsigned int i = 0; i < vecSize-1; i++) {
		cout << "Url config " << i << ": " << url_config[i] << endl;
		string source = "        \"source\": \"";
		size_t pos = url_config[i].find("/");
		source.append(url_config[i].substr(0, pos));
		source.append("\",\n");
		cout << source;
		metrics.push_back(getMetrics(url_config[i]));
		Json::Value root;
		Json::Reader reader;
		bool parsingSuccessful = reader.parse(metrics[0], root);
		if (!parsingSuccessful)
		{
			CLogger::GetLogger()->Log("WARNING: Could not parse metrics to insert source.");
		}
		else {
			vector<size_t> positions;
			size_t pos = metrics[0].find("host", 0);
			int count = 0;
			while (pos != string::npos)
			{
				positions.push_back(pos);
				if (count > 0) 
					metrics[0].insert(pos+10, source);
				int len = source.length() + 10;
				pos = metrics[0].find("\"host", pos+len);
				count++;
			}
			size_t fpos = metrics[0].find("\"host")+10;
			metrics[0].insert(fpos, source);
		}
	}
	cout << "Metrics reloaded." << endl;
	CLogger::GetLogger()->Log("INFO: Metrics reloaded.");
	LOGGER->Log("INFO: Metrics size = %d", metrics.size());
}


int main(int argc, char** argv)
{
  int use_port = 0;
  
  try {
	  CLogger::GetLogger()->Log("INFO: Application started");
  }
  catch (...) {
	  cout << "Could not initiate logger." << endl;
  } 
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGHUP, signalHandler);
  signal(SIGQUIT, signalHandler);
  
  if (readConfiguration("urls.conf") == 0) {
	  populateMetrics();
  }
  if (argc > 1) {	
	LOGGER->Log("INFO: Use port is now: %d", use_port);
  	if (atoi(argv[1])) {
  		use_port = (unsigned short)strtoul(argv[1], NULL, 0);
		cout << "Use port changed to :" << use_port << endl;
		LOGGER->Log("INFO: Use port changed to: %d", use_port);
	}
	else {
		cout << "atoi says no" << endl;
		CLogger::GetLogger()->Log("WARNING: Atoi says no");
	}
	cout << "use_port=" << use_port << endl;
	LOGGER->Log("INFO: Port is set to: %d", use_port);
  }
  initSocket(use_port);
  while (is_ready) {
  	createSocket(server_fd);
  }
  close_socket();
  return 0;
}
