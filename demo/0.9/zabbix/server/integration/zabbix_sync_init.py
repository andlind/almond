import re
import requests
import json
import urllib.parse
from typing import Dict, List, Optional
from urllib.parse import urljoin
from logger_config import get_logger
from zabbix_client import ZabbixAPIClient, ZabbixSyncError
from howru_client import HowruAPIClient

ZABBIX_URL = "http://zabbix-web:8888/api_jsonrpc.php"
AUTH_TOKEN = "30cd9af27796854e45945f901d57cdef"

logger = get_logger()

def escape_zabbix_key_param(value):
    return value.replace("\\", "\\\\").replace(",", "\\,").replace(" ", "\\s")

def slugify(s):
    s = s.lower()
    s = re.sub(r'[^a-z0-9_-]+', '_', s)
    s = re.sub(r'_+', '_', s).strip('_')
    return s or 'item'

def sync_zabbix_hosts(zabbix_client: ZabbixAPIClient, 
                     howru_client: HowruAPIClient,
                     group_id: str) -> None:
    """Main synchronization function"""
    try:
        # Fetch data from HOWRU API
        servers_and_jobs = howru_client.get_servers_and_jobs()
        
        # Process each server
        for server_name, jobs in servers_and_jobs.items():
            host_exists = False
            host_id = None
            
            # Check if host exists
            try:
                result = zabbix_client._api_request(
                    "host.get",
                    {
                        "filter": {"host": [server_name]},
                        "output": ["hostid"]
                    }
                )
                if result["result"]:
                    host_id = result["result"][0]["hostid"]
                    host_exists = True
                    logger.info(f"Found existing host: {server_name}")
                    
            except ZabbixSyncError as e:
                logger.warning(f"Host lookup failed: {str(e)}")
            
            # Create hostgroup
            group_lookup = zabbix_client._api_request("hostgroup.get", {
                "filter": {"name": ["Almond servers"]},
                "output": ["groupid"]
            })
            print("Hostgroup.get response:", group_lookup)
            if isinstance(group_lookup.get("result"), list) and group_lookup["result"]:
                group_id = group_lookup["result"][0]["groupid"]
            else:
                group_create = zabbix_client._api_request("hostgroup.create", {
                    "name": "Almond servers"
                })
                print("hostgroup.create response:", group_create)
                if "groupids" in group_create["result"]:
                    group_id = group_create["result"]["groupids"][0]
                else:
                    logger.error("Failed to create host group 'Almond servers'.")
                    raise Exception("Failed to create host group 'Almond servers'")

            # Create host if it doesn't exist
            if not host_exists:
                create_params = {
                    "host": server_name,
                    "interfaces": [{
                        "type": 1,
                        "main": 1,
                        "useip": 1,
                        "ip": "127.0.0.1",
                        "dns": "",
                        "port": "10050"
                    }],
                    "groups": [{"groupid": group_id}],
                    "inventory_mode": 1
                }
                result = zabbix_client._api_request("host.create", create_params)
                host_id = result["result"]["hostids"][0]
                logger.info(f"Created new host: {server_name}")
            
            # Sync items for each job
            for job in jobs:
                plugin_name = job["name"]
                description = job["description"]
                
                # Create item with all required parameters
                item_name = f"{description} ({plugin_name})"
                create_item_params = {
                    "name": f"{description} ({plugin_name})",
                    "key_": f"check_howru_api.sh[{howru_client.base_url},{plugin_name},{server_name}]",
                    "hostid": host_id,
                    "type": 10,  # External check
                    "value_type": 1,  # Numeric (unsigned) is 3, 1 is text 
                    "interfaceid": 0,
                    "delay": "30s",  # Required parameter
                    "status": 0,     # Required parameter
                    "description": f"External check for {plugin_name} on {server_name}"
                }
                
                try:
                    # Check if item exists
                    result = zabbix_client._api_request(
                        "item.get",
                        {
                            "hostid": host_id,
                            "filter": {"key_": create_item_params["key_"]}
                        }
                    )
                     
                    if not result["result"]:
                        # Create new item
                        zabbix_client._api_request("item.create", create_item_params)
                        logging.info(f"Created item: {description} on {server_name}")
                        item_result =  zabbix_client._api_request("item.get", {
                            "hostids": host_id,
                            "search": {"name": item_name},   # or filter/search on key_
                            "output": ["itemid", "key_"]
                        })
                        #print("DEBUG: item_result:", item_result)
                        item = item_result["result"][0]
                        itemid = item["itemid"]

                        # Add trigger for the item
                        update_item_params = {
                            "itemid": itemid,
                            "value_type": 3,
                            "preprocessing": [
                                {
                                    "type": 5,
                                    "params": "^([0-3])\n\\1",
                                    "error_handler": 0
                                }
                            ]
                        }
                        try:
                            zabbix_client._api_request("item.update", update_item_params)
                            logger.info(f"Created preprocessor for item: {description} on {server_name}")
                        except ZabbixSyncError as e:
                            logger.error(f"Failed to sync item preprocessor for item {plugin_name}: {str(e)}")
                        try:
                            headers = {
                                "Content-Type": "application/json",
                                "Authorization": f"Bearer {AUTH_TOKEN}"
                            }

                            #expression = r"last(/app01.demo.com/check_howru_api.sh[http://host.docker.internal:8085,check_app,app01.demo.com])>0"
                            expression = f"last(/{server_name}/check_howru_api.sh[{howru_client.base_url},{plugin_name},{server_name}])>0"
                            description = f"Problem with {description} on {server_name}"

                            payload = {
                                "jsonrpc": "2.0",
                                "method": "trigger.create",
                                "params": {
                                    "description": description,
                                    "expression": expression,
                                    "priority": 4,
                                    "status": 0
                                },
                                "id": 1
                            }

                            response = requests.post(ZABBIX_URL, json=payload, headers=headers)
                            #print(response.json()) 
                        except ZabbixSyncError as e:
                            logger.error(f"Failed to send item trigger for item {plugin_name}: {str(e)}") 
                    else:
                        logger.info(f"Item exists: {description} on {server_name}")
                        
                except ZabbixSyncError as e:
                    logger.error(f"Failed to sync item {plugin_name}: {str(e)}")
        
    except Exception as e:
        logger.error(f"Sync failed: {str(e)}")
        raise

def main():
    """Main entry point"""
    config = {
        "ZABBIX_URL": "http://zabbix-web:8888",
        "ZABBIX_USER": "Admin",
        "ZABBIX_PASS": "zabbix",
        "HOWRU_API_ADDRESS": "http://host.docker.internal:8085",
        "ZABBIX_GROUP_ID": "2"
    }

    try:
        # Initialize clients
        zabbix_client = ZabbixAPIClient(
            base_url=config["ZABBIX_URL"],
            username=config["ZABBIX_USER"],
            password=config["ZABBIX_PASS"]
        )
                
        howru_client = HowruAPIClient(base_url=config["HOWRU_API_ADDRESS"])
        
        # Login to Zabbix
        zabbix_client.login()
        
        # Perform synchronization
        sync_zabbix_hosts(zabbix_client, howru_client, config["ZABBIX_GROUP_ID"])
        
    except Exception as e:
        logging.error(f"Synchronization failed: {str(e)}")
        raise

if __name__ == "__main__":
    main()
