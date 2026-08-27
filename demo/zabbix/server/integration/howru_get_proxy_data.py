import json
from howru_client import HowruAPIClient
from logger_config import get_logger

logger = get_logger()

def main():
    config = {                          
        "HOWRU_API_ADDRESS": "http://host.docker.internal:8085",
    }                                  
                 
    try:
        client = HowruAPIClient(base_url=config["HOWRU_API_ADDRESS"])

        update_time = client.get_proxy_config_update_time()
        server_job_data = client.get_server_and_job_count()

        result = {
            "lastmodifiedtimestamp": update_time,
            "server_job_data": server_job_data
        }

        print(json.dumps(result))  # 👈 Output as JSON to stdout

    except Exception as e:
        logger.error("Failed to fetch proxy data")
        logger.error(str(e))
        print(json.dumps({"error": str(e)}))

if __name__ == "__main__":
    main()

