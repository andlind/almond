import requests
import logging
from typing import Dict, List
from urllib.parse import urljoin
from logger_config import get_logger

logger = get_logger()

class ZabbixSyncError(Exception):
    """Custom exception for Zabbix sync errors"""
    pass

class ZabbixAPIClient:
    global logger
    def __init__(self, base_url: str, username: str, password: str):
        self.base_url = base_url.rstrip('/')
        self.username = username
        self.password = password
        self.session = requests.Session()
        self.auth_token = None

        # Configure logging
        #logging.basicConfig(level=logging.INFO)
        self.logger = logger

    def login(self) -> None:
        """Authenticate with Zabbix API using modern Bearer token approach"""
        try:
            response = self._api_request(
                "user.login",
                {"username": self.username, "password": self.password}
            )
            self.auth_token = response["result"]
            self.logger.info("Successfully authenticated with Zabbix API")
        except Exception as e:
            print("Zabbix API error:", str(e))
            logger.error("Zabbix API error: failed to authenticate")
            raise ZabbixSyncError(f"Authentication failed: {str(e)}")

    def _api_request(self, method: str, params: Dict) -> Dict:
        """Make a request to the Zabbix API"""
        headers = {
            "Content-Type": "application/json-rpc",
            "Authorization": f"Bearer {self.auth_token}" if self.auth_token else ""
        }

        payload = {
            "jsonrpc": "2.0",
            "method": method,
            "params": params,
            "id": 1
        }

        try:
            response = self.session.post(
                urljoin(self.base_url, "api_jsonrpc.php"),
                json=payload,
                headers=headers
            )
            response.raise_for_status()

            result = response.json()
            if "error" in result:
                print("Zabbix API error:", result["error"])
                logger.error("Zabbix API error:", result["error"])
                raise ZabbixSyncError(result["error"]["message"])

            logger.info("Executed method:" + method)
            return result

        except requests.RequestException as e:                    
            print("Zabbix API error:", result["error"])           
            logger.error("Zabbix API error:", result["error"])    
            raise ZabbixSyncError(f"API request failed: {str(e)}")
