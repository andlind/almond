import requests
import re
import json
import urllib.parse
from typing import Dict, List, Optional
from urllib.parse import urljoin
from logger_config import get_logger
from zabbix_client import ZabbixAPIClient, ZabbixSyncError
from howru_client import HowruAPIClient

logger = get_logger()

howru_servers = []
zabbix_servers = []
howru_server_jobs = {}

def get_howru_data(client: HowruAPIClient):
    global howru_servers, howru_server_jobs
    servers_and_jobs = client.get_servers_and_jobs()
    howru_servers = list(servers_and_jobs.keys())
    howru_server_jobs = {
        server: [job["name"] for job in jobs]
        for server, jobs in servers_and_jobs.items()
    }

def get_zabbix_hosts(client: ZabbixAPIClient):
    global zabbix_servers

    group_response = client._api_request("hostgroup.get", {
        "output": ["groupid", "name"],
        "filter": {
            "name": ["Almond servers"]
        }
    })
    groups = group_response.get("result", [])
    if not groups:
        raise ValueError("Host group 'Linux servers' not found")
    group_id = groups[0]["groupid"]
    response = client._api_request("host.get",
        {
            "output": ["hostid", "host", "name"],
            "groupids": group_id
        }
    )
    zabbix_servers = [host["name"] for host in response["result"]]    
    print (zabbix_servers)

def get_zabbix_items(client: ZabbixAPIClient, host_name: str):
    host_response = client._api_request("host.get", {
        "output": ["hostid"],
        "filter": {
            "name": [host_name]
        }
    })
    hosts = host_response.get("result", [])
    if not hosts:
        raise ValueError(f"Host '{host_name}' not found")
    host_id = hosts[0]["hostid"]

    # Now, get all items associated with the host ID
    item_response = client._api_request("item.get", {
        "output": ["itemid", "name", "key_", "lastvalue"],
        "hostids": host_id
    })
    items = item_response.get("result", [])
    return items

    # Run initscript

def compare_registered_hosts(howru, zabbix):
    h_list = sorted(howru)
    z_list = sorted(zabbix)

    only_in_h = list(set(h_list) - set(z_list))
    only_in_z = list(set(z_list) - set(h_list))
    diff = list(set(h_list).symmetric_difference(set(z_list)))
    
    #print("Only in Howru:", only_in_h)
    #print("Only in Zabbix:", only_in_z)
    #print("Difference:", diff)
    for h in only_in_h:
        logger.info(f"Found server '{h}' which needs to be added to Zabbix")
    for z in only_in_z:
        logger.info(f"Server '{z}' needs to be removed from Zabbix")
    if diff:
        logger.warning("Found differences between servers in Zabbix and HowRU")    
    return only_in_z

def get_host_ids_by_names(client, hostnames):
    host_ids = []
    for name in hostnames:
        response = client._api_request("host.get", {
            "output": ["hostid"],
            "filter": {"host": [name]}
        })
        if response:
            host_ids.append(response[0]["hostid"])
    return host_ids


def delete_zabbix_hosts(client: ZabbixAPIClient, hostnames):
    host_ids = []

    for name in hostnames:
        response = client._api_request("host.get", {
            "output": ["hostid"],
            "filter": {"host": [name]}
        })

        # Correctly extract host IDs from response["result"]
        if isinstance(response, dict) and "result" in response:
            host_list = response["result"]
            host_ids.extend([host["hostid"] for host in host_list if "hostid" in host])
        else:
            logger.warning(f"Unexpected response format for host '{name}': {response}")

    if not host_ids:
        logger.info("Found no hosts to delete")
        return

    logger.info(f"Deleting hosts: {hostnames}")
    response = client._api_request("host.delete", host_ids)

    if isinstance(response, dict) and "result" in response:
        deleted_ids = response["result"]
        logger.info(f"Successfully deleted host IDs: {deleted_ids}")
    else:
        logger.warning(f"Unexpected delete response: {response}")

def delete_zabbix_items_and_triggers(client: ZabbixAPIClient, item_names):     
    item_ids = []                                                              
    trigger_ids = []                                                      
                                                                          
    for name in item_names:                                               
        # Get item details                                                
        item_response = client._api_request("item.get", {                 
            "output": ["itemid", "key_", "hostid"],                       
            "filter": {"name": [name]}                                    
        })                                                                
                                                                          
        logger.debug(f"item_response for '{name}': {item_response} (type: {type(item_response)})")
        item_result = item_response.get("result", [])                                                
        if not item_result:                                                                         
            logger.warning(f"No items found for '{name}'")                                          
            continue                                                                                
                                                                                                    
        for item in item_result:                                                                    
            item_ids.append(item["itemid"])                                                         
            key = item["key_"]                                                                      
            hostid = item["hostid"]                                                                 
                                                                                                    
            # Get triggers referencing this item's key                                              
            trigger_response = client._api_request("trigger.get", {                                
                "output": ["triggerid", "expression"],                                              
                "filter": {"hostid": hostid},                                                       
                "expandExpression": True                                                            
            })                                                                                      
                                                                                                    
            for trigger in trigger_response.get("result", []):                                     
                if key in trigger["expression"]:                                                    
                    trigger_ids.append(trigger["triggerid"])                                        
                                                                                                    
    if trigger_ids:                                                                                
        logger.info(f"Deleting triggers: {trigger_ids}")                                            
        client._api_request("trigger.delete", trigger_ids)                                          
                                                                                                    
    if item_ids:                                                                                    
        logger.info(f"Deleting items: {item_names}")                                                
        response = client._api_request("item.delete", item_ids)                                     
        deleted_ids = response.get("result", [])                                                    
        logger.info(f"Successfully deleted item IDs: {deleted_ids}")                                
    else:                                                                                           
        logger.info("Found no items to delete")                                                     

def main():                       
    """Main entry point"""
    # Get Zabbix items, delete if extra on Zabbix 
    # Run initscript
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
        get_howru_data(howru_client)
                                                       
        # Login to Zabbix            
        zabbix_client.login()
        get_zabbix_hosts(zabbix_client)
        remove_hosts = compare_registered_hosts(howru_servers, zabbix_servers)
        print (remove_hosts)
        if (remove_hosts):
            delete_zabbix_hosts(zabbix_client, remove_hosts)
        zabbix_server_items = {
            server: [item["name"] for item in get_zabbix_items(zabbix_client, server)]
                for server in howru_server_jobs.keys()
        }
        for server in howru_server_jobs:
            howru_jobs = set(howru_server_jobs[server]) 
            zabbix_items_raw = set(zabbix_server_items.get(server, []))  
            #zabbix_items = {re.search(r'\((.*?)\)', item).group(1) for item in zabbix_items_raw if re.search(r'\((.*?)\)', item)}
            zabbix_items_lookup = {
                re.search(r'\((.*?)\)', item).group(1): item
                for item in zabbix_items_raw
                if re.search(r'\((.*?)\)', item)
            }
            zabbix_items = set(zabbix_items_lookup.keys()) 
            #print("DEBUG: howru_jobs = ", howru_jobs)
            #print("DEBUG: zabbix_items = ", zabbix_items)
            extra_in_zabbix = zabbix_items - howru_jobs
            extra_items_for_deletion = {zabbix_items_lookup[job] for job in extra_in_zabbix} 
            #print("DEBUG extra_items_for_deletion = ", extra_items_for_deletion)
            print(f"\n🔍 Server: {server}")
            print(f"➕ Extra items in Zabbix: {extra_in_zabbix}")                                               
            delete_zabbix_items_and_triggers(zabbix_client, extra_items_for_deletion)

    except Exception as e:                                                     
        logger.error(f"Synchronization failed: {str(e)}")
        raise     
                                                                 
if __name__ == "__main__":                              
    main()             
