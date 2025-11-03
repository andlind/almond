import os
import time
import json
import logging
import subprocess
from logger_config import get_logger

# Set the timer interval in seconds
TIMER_INTERVAL = 30  # e.g., 30 seconds
CURRENT_VERSION = 0.1

is_syncing = False
proxy_current_update_time = -1
proxy_update_time = -1
proxy_server_count = 0
proxy_job_count = 0

logger = get_logger()

def get_proxy_data():
    global proxy_update_time, proxy_server_count, proxy_plugin_count

    result = subprocess.run(                        
        ['python3', 'howru_get_proxy_data.py'],
        capture_output=True,                   
        text=True                              
    )                                                       
    try:                                                    
        data = json.loads(result.stdout)                              
        if "error" in data:                                           
            print("Error from proxy script:", data["error"])          
            logger.warning("Could not get proxy data")
        else:                                                         
            #print("Proxy update time:", data["lastmodifiedtimestamp"])
            proxy_update_time = data["lastmodifiedtimestamp"]
            #print("Server/job data:", data["server_job_data"])        
            summary = data["server_job_data"]["summary"][0]
            print ("DEBUG: summary = ", summary)
            proxy_server_count = summary["servercount"]
            proxy_job_count = summary["plugincount"]
            print ("DEBUG: proxy_server_count = ", proxy_server_count)
            print ("DEBUG: proxy_job_count = ", proxy_job_count)
            logger.info("Extracted metadata from HowRU proxy")
    except json.JSONDecodeError:                                 
        print("Failed to parse proxy data output")      
        logger.warning("Failed to parse proxy data output.")

def main():
    logger = get_logger();
    logger.info('Starting Almond Zabbix integration (version:' + str(CURRENT_VERSION) + ')')
    logger.info('Initiating Zabbix sync')
    subprocess.run(['python3', 'zabbix_sync_init.py'])
    logger.info('Initiating Almond sync mechanism')
    subprocess.run(['python3', 'howru_get_proxy_data.py']) 
    is_syncing = False
    logger.info('Starting synchronization cycle')
    while True:
        print("Starting synchronization cycle...\n")
        if is_syncing:
            logger.info("Synching data.")
            nos = proxy_server_count
            noj = proxy_job_count
            print ("DEBUG: proxy_server_count = ", proxy_server_count)
            print ("DEBUG: proxy_job_count = ", proxy_job_count)
            print ("DEBUG: Running get_proxy_data")
            get_proxy_data()
            print ("DEBUG: proxy_server_count = ", proxy_server_count)
            print ("DEBUG: proxy_job_count = ", proxy_job_count) 
            if not proxy_current_update_time == proxy_update_time:
                logger.info("Config updates detected on proxy servers.")
                logger.info("We will not react upon this right now")
                proxy_current_update_time = proxy_update_time
            if (nos != proxy_server_count) or (noj != proxy_job_count):
                logger.info("Server or job change detected on proxy")
                logger.info("Let us dig...")
                subprocess.run(['python3', 'sync_mechanism.py'])
            subprocess.run(['python3', 'zabbix_sync_init.py'])
        else:
            print ("DEBUG: Init sync")
            get_proxy_data()
            proxy_current_update_time = proxy_update_time
        if not is_syncing:
            is_syncing = True
        print(f"Waiting {TIMER_INTERVAL} seconds before next cycle...\n")
        logger.info("Sleeping " +str(TIMER_INTERVAL) + " seconds.")
        is_syncing = True
        time.sleep(TIMER_INTERVAL)

if __name__ == '__main__':
    main()

