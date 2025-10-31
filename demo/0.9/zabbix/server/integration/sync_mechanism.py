import requests
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

def get_zabbix_items(client: ZabbixAPIClient):
    return

    # Get HowRU data
    # Get Zabbix hosts
    # Compare hostlists, delete if extra on Zabbix
    # Get Zabbix items, delete if extra on Zabbix
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

#def delete_zabbix_hosts(client: ZabbixAPIClient, hostnames):
#    host_ids = []                                                              
#    for name in hostnames:                                                     
#        response = client._api_request("host.get", {                           
#            "output": ["hostid"],                                              
#            "filter": {"host": [name]}                                         
#        })                                                                     
#        if response:                                                           
#            host_ids.append(response[0]["hostid"])             
#    if not host_ids:
#        logger.info("Found no hosts to delete")
#        return
#    response = client._api_request("host.delete", host_ids)
#    logger.info(f"Deleted hosts with IDs: {response}")

def delete_zabbix_hosts(client: ZabbixAPIClient, hostnames):
    host_ids = []
    for name in hostnames:
        response = client._api_request("host.get", {
            "output": ["hostid"],
            "filter": {"host": [name]}
        })
        if response:
            host_ids.extend([host["hostid"] for host in response])

    if not host_ids:
        logger.info("Found no hosts to delete")
        return

    logger.info(f"Deleting hosts: {hostnames}")
    response = client._api_request("host.delete", host_ids)
    deleted_ids = response.get("result", [])
    logger.info(f"Successfully deleted host IDs: {deleted_ids}")

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

                                                          
    except Exception as e:                                                     
        logger.error(f"Synchronization failed: {str(e)}")
        raise     
                                                                 
if __name__ == "__main__":                              
    main()             
