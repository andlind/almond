#include <iostream>
#include <string>
#include <regex>
#include <iomanip> 
#include <unistd.h>
#include <curl/curl.h>

enum NagiosStatus {
    STATUS_OK       = 0,
    STATUS_WARNING  = 1,
    STATUS_CRITICAL = 2,
    STATUS_UNKNOWN  = 3
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

double extractMetric(const std::string& payload, const std::string& metricName, double defaultVal = -1.0) {
    std::regex expr(metricName + R"(\{[^\}]*\}\s+([0-9]+(?:\.[0-9]+)?))");
    std::smatch match;
    if (std::regex_search(payload, match, expr) && match.size() > 1) {
        return std::stod(match[1].str());
    }
    return defaultVal;
}

void printHelp(const char* progName) {
    std::cout << "Usage: " << progName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h            Print this help message" << std::endl;
    std::cout << "  -H <host>     Target host (default: localhost)" << std::endl;
    std::cout << "  -P <port>     Target port (default: 9099)" << std::endl;
    std::cout << "  -w <seconds>  Warning threshold for uptime (default: 300)" << std::endl;
    std::cout << "  -c <count>    Critical threshold for open file descriptors (default: 800)" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string host = "localhost";
    std::string port = "90";
    double uptime_warn = 300.0;
    double fds_crit = 800.0;
    int opt;
    CURL *curl;
    CURLcode res;
    std::string httpResponse;

    if (argc == 1) {
        printHelp(argv[0]);
        return STATUS_UNKNOWN;
    }
    while ((opt = getopt(argc, argv, "H:P:hw:c:")) != -1) {
        switch (opt) {
            case 'h':
                printHelp(argv[0]);
                return STATUS_OK; // Nagios plugins usually exit OK when help is explicitly requested
            case 'H':
                host = optarg;
                break;
            case 'P':
                try {
                    int p = std::stoi(optarg);
                    if (p < 1 || p > 65535) {
                        throw std::out_of_range("Port out of range");
                    }
                    port = optarg; // Validated, so keep the string for the URL
                } catch (const std::exception& e) {
                    std::cout << "UNKNOWN - Invalid port value for -P: '" << optarg << "'. Must be 1-65535." << std::endl;
                    return STATUS_UNKNOWN;
                }
                break;
            case 'w':
                try {
                    uptime_warn = std::stod(optarg);
                } catch (const std::exception& e) {
                    std::cout << "UNKNOWN - Invalid numeric value for -w: '" << optarg << "'" << std::endl;
                    return STATUS_UNKNOWN;
                }
                break;
            case 'c':
                try {
                    fds_crit = std::stod(optarg);
                } catch (const std::exception& e) {
                    std::cout << "UNKNOWN - Invalid numeric value for -c: '" << optarg << "'" << std::endl;
                    return STATUS_UNKNOWN;
                }
                break;
            default:
		printHelp(argv[0]);
                return STATUS_UNKNOWN;
	}
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) return STATUS_UNKNOWN;

    std::string url = "http://" + host + ":" + port;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"action\":\"almond\", \"name\":\"metrics\"}");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &httpResponse);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cout << "CRITICAL - HTTP Request failed" << std::endl;
        curl_easy_cleanup(curl);
        return STATUS_CRITICAL;
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    // 1. Primary Health Metrics
    double pid    = extractMetric(httpResponse, "almond_process_id");
    double uptime = extractMetric(httpResponse, "almond_process_uptime_seconds");
    double fds    = extractMetric(httpResponse, "almond_process_open_file_descriptors");
    double memory = extractMetric(httpResponse, "almond_process_resident_memory_bytes");

    // 2. Additional Performance Metrics
    double heap_alloc       = extractMetric(httpResponse, "almond_process_heap_allocated_bytes");
    double heap_total       = extractMetric(httpResponse, "almond_process_heap_total_bytes");
    double threads          = extractMetric(httpResponse, "almond_process_threads");
    double cpu_user         = extractMetric(httpResponse, "almond_process_cpu_user_seconds_total");
    double cpu_sys          = extractMetric(httpResponse, "almond_process_cpu_system_seconds_total");
    double io_read          = extractMetric(httpResponse, "almond_process_io_storage_read_bytes_total");
    double io_write         = extractMetric(httpResponse, "almond_process_io_storage_written_bytes_total");
    double page_faults      = extractMetric(httpResponse, "almond_process_major_page_faults_total");
    double context_switches = extractMetric(httpResponse, "almond_process_context_switches_involuntary_total");

    if (pid == -1.0) {
        std::cout << "CRITICAL - Could not parse metrics." << std::endl;
        return STATUS_CRITICAL;
    }

    NagiosStatus exitCode = STATUS_OK;
    std::string statusPrefix = "OK";

    if (uptime < uptime_warn) {
        exitCode = STATUS_WARNING;
        statusPrefix = "WARNING";
    }
    if (fds > fds_crit) {
        exitCode = STATUS_CRITICAL;
        statusPrefix = "CRITICAL";
    }

    // 3. Format Output
    std::cout << std::fixed << std::setprecision(0);
    std::cout << statusPrefix << " - Almond process active (PID: " << pid 
              << ", Uptime: " << uptime << "s, FDs: " << fds << ") "
              << "| "
              
              // Health Metrics
              << "uptime=" << uptime << "s;" << uptime_warn << ";;0; "
              << "fds=" << fds << ";;" << fds_crit << ";0; "
              << "rss=" << memory << "B;;;0; "
              
              // New Extended Metrics
              << "heap_alloc=" << heap_alloc << "B;;;0;" << heap_total << " "
              << "threads=" << threads << ";;;0; "
	      << std::setprecision(2)
              << "cpu_user=" << cpu_user << ";;;0; "
              << "cpu_sys=" << cpu_sys << ";;;0; "
              << std::setprecision(0)
              << "io_read=" << io_read << "c;;;0; "
              << "io_write=" << io_write << "c;;;0;" 
              << "page_faults=" << page_faults << "c;;;0;"
              << "context_switches=" << context_switches << "c;;;0;"
              << std::endl;

    return exitCode;
}
