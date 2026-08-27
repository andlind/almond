from urllib.parse import urljoin
import requests
import json
from typing import Dict, List
from logger_config import get_logger
from zabbix_client import ZabbixSyncError

logger = get_logger()

class HowruAPIClient:                                                              
    def __init__(self, base_url: str):                                             
        self.base_url = base_url.rstrip('/')                                       
        self.session = requests.Session()                                          

    def get_proxy_config_update_time(self) ->int:
        try:
            response = self._get("/api/unixupdatetime")
            logger.info("Fetched last updatetime from HowRU proxy")
            return response.get("lastmodifiedtimestamp", -1)           
        except Exception as e:
            logger.error("Failed to fetch update time from HowRU proxy")
            logger.error(str(e))
            raise ZabbixSyncError(f"Failed to fetch HOWRU API data: {str(e)}")

    def get_server_and_job_count(self) -> Dict[str, List[Dict]]:
        """Fetch servers and jobs from HOWRU API"""
        try:
            servers_response = self._get("/api/countservers")
            jobs_response = self._get("/api/countjobs")

            # Combine server and job data
            combined_data = {
                "summary": [
                    {
                        "servercount": servers_response.get("servercount", 0),
                        "plugincount": jobs_response.get("plugincount", 0)
                    }
                ]
            }
            
            logger.info("Fetched server and job count from HowRU proxy")
            return combined_data

        except Exception as e:
            logger.error("Failed to fetch Howru data. Error:")
            logger.error(str(e))
            raise ZabbixSyncError(f"Failed to fetch HOWRU API data: {str(e)}")
                                                                                   
    def get_servers_and_jobs(self) -> Dict[str, List[Dict]]:                       
        """Fetch servers and jobs from HOWRU API"""                                
        try:                                                                       
            servers_response = self._get("/api/listservers")                       
            jobs_response = self._get("/api/listjobs")                             
                                                                                   
            # Combine server and job data                                          
            combined_data = {}                                                     
            for job_entry in jobs_response:                                        
                server = job_entry["server"]                                       
                if server not in combined_data:                                    
                    combined_data[server] = []                                     
                combined_data[server].extend(job_entry["jobs"])                    
                                                                  
            logger.info("Fetched server and jobs from HowRU proxy")
            return combined_data                                   
                                                                   
        except Exception as e:                                     
            logger.error("Failed to fetch Howru data. Error:")     
            logger.error(str(e))                                   
            raise ZabbixSyncError(f"Failed to fetch HOWRU API data: {str(e)}")
                                                                              
    def _get(self, endpoint: str) -> Dict:                                    
        """Make GET request to HOWRU API"""                                   
        response = self.session.get(urljoin(self.base_url, endpoint))         
        response.raise_for_status()                                           
        return response.json()            
