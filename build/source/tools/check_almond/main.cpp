#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>

using namespace std;

class ProgramOptions {
	private:
		string host;
		int port;
		string command;
		string args;
		string token;
		int cid = -1;
		bool dry_run = false;
		bool passive = false;
		bool verbose = false;
	public:
		bool parseArgs(int argc, char* argv[]) {
			if (argc >=2 && (string(argv[1]) == "-h" || string(argv[1]) == "--help")) {
			       printHelp();
			       return false;
			}
		       			       
			bool hasHost = false;
			bool hasPort = false;
			bool hasCommand = false;
			bool hasArgs = false;
			bool hasToken = false;

			for (int i = 1; i < argc; i++) {
				if (string(argv[i]) == "-H" && i+1 < argc) {
					host = argv[++i];
					hasHost = true;
				}
				else if (string(argv[i]) == "-P" && i + 1 < argc) {
					port = stoi(argv[++i]);
					hasPort = true;
				}
				else if (string(argv[i]) == "-c" && i + 1 < argc) {
					command = argv[++i];
					hasCommand = true;
				}
				else if (string(argv[i]) == "-i" && i + 1 < argc) {
					cid = stoi(argv[++i]);
					hasCommand = true;
				}
				else if (string(argv[i]) == "-a" && i + 1 < argc) {
					args = argv[++i];
					if (args.length() > 2)
						hasArgs = true;
					else {
						printUsage();
					}
				}
				else if ((string(argv[i]) == "-t" || string(argv[i]) == "--token") && (i + 1 < argc)) {
					token = argv[++i];
					hasToken = true;
				}
				else if ((string(argv[i]) == "-p") || (string(argv[i]) == "--passive")) {
					passive = true;
					hasArgs = true;
				}
				else if ((string(argv[i]) == "-d") || (string(argv[i]) == "--dry-run")) {
					dry_run = true;
				}
				else if ((string(argv[i]) == "-v") || (string(argv[i]) == "--verbose")) {
					verbose = true;
				}
				else if ((string(argv[i]) == "-h") || (string(argv[i]) == "--help")) {
					printHelp();
					return false;
				}

			}
					
			if (!hasHost || !hasPort || !hasCommand || !hasArgs) {
				printUsage();
				return false;
			}

			if (!(passive) && !(hasToken)) {
                                printTokenInformation();
                                return false;
                        }
			return true;
		}

		void printUsage() {
			cout << "Usage ./check_almond -H <host> -P <port> -c <command> -i <command_id> -a \"<command_args>\" [--verbose] [--passive] [--dry-run] [--help]" << endl;
		}

		string getHostName () {
			return host;
		}

		int getPort() {
			return port;
		}

		int getCommandId() {
			return cid;
		}

		string getCommand() {
			return command;
		}

		string getCommandArgs() {
			return args;
		}

		string getToken() {
			return token;
		}

		bool getDryRun () {
			return dry_run;
		}

		bool getRunPassive() {
			return passive;
		}

		bool isVerbose() {
			return verbose;
		}

		void printTokenInformation() {
			cout << "check_almond (v.1 @Almond Monitor)" << endl << endl;
			cout << "In order to invoke an execution of a command with Almond API you need to supply a valid token." << endl;
			cout << "This is supplied with the argument -t/--token." << endl << endl;
			cout << "If you don´t have a valid token this can be created by the almond-token-generator found in the utilities directory on the" << endl;
			cout << "server where Almond in installed. (Usually /opt/almond/utilities/almond-token-generator)." << endl << endl;
			cout << "If you do not have access to creating a token you could consider running a passive check against Almond, which does not require a token." << endl;
			cout << "In this case you do not need to supply arguments, but insted use the parameter -p/--passive to this command." << endl;
		}	

		void printConfig() {
			cout << "check_almond (v.1 @Almond Monitor)" << endl << endl;
			cout << "Will try to invoke Almond API with the following parameters contributed:" << endl << endl;
			cout << "Host: " << host << endl;
			cout << "Port: " << port << endl;
			cout << "Command: " << command << endl;
			cout << "Arguments: " << args << endl;
			cout << "Passive: " << (passive ? "yes" : "no") << endl;
			cout << "Dry run: " << (dry_run ? "yes" : "no") << endl;
			if (!passive) {
				cout << "Token: " << token << endl;
			}
		}

		void printHelp() {
			cout << endl << "check_almond (v.1 @Almond Monitor)" << endl << endl;
			printUsage();
			cout << endl;
			cout << "-H\t\tHostname to query [value: string]" << endl;
			cout << "-P\t\tPort used by Almond on host [value: integer]" << endl;
			cout << "-c\t\tThe command to be executed by Almond [value: string][example: check_memory]" << endl;
			cout << "-i\t\tCommand id to be executed by Almond, used insted of -c [value: integer]" << endl;
			cout << "-a\t\tArguments to be passed to command [value: string][example: \"-W 85% -C 95%\"]" << endl;
			cout << "-t\t\tToken needed to execute command against Almond API [value:string]" << endl;
			cout << "-p\t\tDo not execute command, just get Almonds latest retun values [default value: false][optional]" << endl;
			cout << "-d\t\tDry run, run the command but do not add the result to Almond process [default value: false][optional]" << endl;
			cout << "-h\t\tPrint help" << endl << endl;
		}
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
    	return size * nmemb;
}

bool isValidJson(const std::string& jsonStr) {
   	Json::Value jsonData;
    	Json::Reader jsonReader;
    	return jsonReader.parse(jsonStr, jsonData);
}

string curlPostRequest(const string& url, const string& jsonData) {
	CURL *curl;
	CURLcode res;
	string readBuffer;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L); 
		
		struct curl_slist *headers = NULL;
        	headers = curl_slist_append(headers, "Content-Type: application/json");
        	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		res = curl_easy_perform(curl);
        	if (res != CURLE_OK)
               		throw std::runtime_error(curl_easy_strerror(res));

        	curl_easy_cleanup(curl);
        	curl_slist_free_all(headers);
	}
	curl_global_cleanup();
    	return readBuffer;
}

int main(int argc, char* argv[]) {
	string json_string = "";
	string response = "";
	string url;
	Json::Value root;
        Json::Reader reader;
	bool runPassive = false;
	ProgramOptions options;
	int retCode = 0;

	if (!options.parseArgs(argc, argv)) {
		//std::cerr << "Usage ./check_almond -H <host> -P <port> [--dry-run]" << endl;
		return -1;
	}
	if (options.isVerbose())
		options.printConfig();
	runPassive = options.getRunPassive();
	if (runPassive) {
		json_string = "{\"action\":\"read\", \"";
	}
	else {
		json_string = "{\"action\":\"execute\", \"";
	}
	int commandId = options.getCommandId();
	if (commandId != -1) {
		json_string += "id\":\"" + to_string(commandId) + "\", ";
	}
	else {
		json_string += "name\":\"" + options.getCommand() + "\", ";
	}
	if (runPassive) {
		json_string += "\"flags\":\"verbose\"}";
	}
	else {
		json_string += "\"args\":\"" + options.getCommandArgs() + "\", \"flags\":\"verbose\", ";
		json_string += "\"token\":\"" + options.getToken() + "\"}";
	}
	url = options.getHostName() + ":" + to_string(options.getPort());   
	if (!isValidJson(json_string)) {
		throw std::runtime_error("Invalid JSON format");
		return -1;
	}
	try {
		response = curlPostRequest(url, json_string);
	}
	catch (const exception& e) {
		std::cerr << "Request failed: " << e.what() << endl;
		return -2;
	}
	if (!reader.parse(response, root)) {
        	cout << "Error parsing json" << endl;
                return -2;
        }
	if (runPassive) {
		string sCode = root["pluginStatusCode"].asString();
		retCode = stoi(sCode);
		string output = root["pluginOutput"].asString();
		cout << output << endl;
	}
	else {
		string sCode = root["result"]["pluginStatusCode"].asString();
		retCode = stoi(sCode);
		string output = root["result"]["pluginOutput"].asString();
		cout << output << endl;
	}

	return retCode;
}
