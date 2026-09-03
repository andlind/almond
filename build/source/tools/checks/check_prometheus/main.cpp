#include <iostream>
#include <string>
#include <regex>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <cxxopts.hpp>

bool isValidUrl(const std::string& url) {
    // Basic regex check for http:// or https:// prefix
    const std::regex url_pattern(R"(^https?://.+)");
    return std::regex_match(url, url_pattern);
}

int main(int argc, char* argv[]) {
    cxxopts::Options options("check_prometheus", "Prometheus metric status checker");

    options.add_options()
        ("H,url", "Prometheus URL", cxxopts::value<std::string>()->default_value("http://localhost:9090"))
        ("q,query", "PromQL query", cxxopts::value<std::string>())
        ("w,warning", "Warning threshold", cxxopts::value<double>()->default_value("0.15"))
        ("c,critical", "Critical threshold", cxxopts::value<double>()->default_value("0.10"))
        ("h,help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << "\n";
        return 0;
    }

    if (!result.count("query")) {
        std::cerr << "UNKNOWN: Missing required argument: --query (-q)\n";
        std::cout << options.help() << "\n";
        return 3;
    }

    std::string url = result["url"].as<std::string>();
    std::string query = result["query"].as<std::string>();
    double warning = result["warning"].as<double>();
    double critical = result["critical"].as<double>();

    if (!isValidUrl(url)) {
        std::cerr << "UNKNOWN: Invalid URL format provided: " << url << "\n";
        return 3;
    }

    std::string api_url = url + "/api/v1/query";
    auto response = cpr::Get(cpr::Url{api_url}, cpr::Parameters{{"query", query}});

    if (response.status_code != 200) {
        std::cerr << "CRITICAL: Failed to connect to Prometheus (HTTP " << response.status_code << ")\n";
        return 2;
    }

    try {
        auto json_data = nlohmann::json::parse(response.text);
        auto results = json_data["data"]["result"];
        
        if (results.empty()) {
            std::cout << "UNKNOWN: Query returned no results.\n";
            return 3;
        }

        std::string val_str = results[0]["value"][1];
        double value = std::stod(val_str);

        if (value <= critical) {
            std::cout << "CRITICAL: Metric value is " << value << " (Threshold <= " << critical << ")\n";
            return 2;
        } else if (value <= warning) {
            std::cout << "WARNING: Metric value is " << value << " (Threshold <= " << warning << ")\n";
            return 1;
        } else {
            std::cout << "OK: Metric value is " << value << "\n";
            return 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "UNKNOWN: Error parsing response: " << e.what() << "\n";
        return 3;
    }
}
