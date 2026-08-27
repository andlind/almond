import requests
import json
from urllib.parse import urljoin

class ZabbixSyncError(Exception):
    pass

class ZabbixAPIClient:
    def __init__(self, base_url: str, api_token: str):
        self.base_url = base_url.rstrip('/')
        self.api_token = api_token
        self.session = requests.Session()

    def _api_request(self, method: str, params: dict) -> dict:
        headers = {
            "Content-Type": "application/json",
            "Authorization": f"Bearer {self.api_token}"
        }
        payload = {
            "jsonrpc": "2.0",
            "method": method,
            "params": params,
            "id": 1
        }
        response = self.session.post(
            urljoin(self.base_url, "api_jsonrpc.php"),
            json=payload,
            headers=headers
        )
        response.raise_for_status()
        result = response.json()
        if "error" in result:
            raise ZabbixSyncError(result["error"]["message"])
        return result["result"]

def list_items_for_host(hostname: str):
    client = ZabbixAPIClient(
        base_url="http://zabbix-web:8888/zabbix",
        api_token="2d840b0650a729c7d74503381dbb5b1824e7c31e9e44214ce58ec8e20325cdd4"
    )

    # Step 1: Get host ID by matching hostname
    hosts = client._api_request("host.get", {
        "output": ["hostid", "host"]
    })

    host_id = None
    for host in hosts:
        if host["host"] == hostname:
            host_id = host["hostid"]
            break

    if not host_id:
        print(f"Host '{hostname}' not found.")
        return

    # Step 2: Get items for that host
    items = client._api_request("item.get", {
        "hostids": [host_id],
        "output": ["itemid", "name", "key_", "lastvalue", "lastclock"]
    })

    # Step 3: Print item details
    print(f"Items for host '{hostname}':")
    for item in items:
        print(f"- {item['name']} (key: {item['key_']}, last value: {item['lastvalue']}, last update: {item['lastclock']})")

def delete_unknown_items_for_host(hostname: str):
    client = ZabbixAPIClient(
        base_url="http://zabbix-web:8888/zabbix",
        api_token="2d840b0650a729c7d74503381dbb5b1824e7c31e9e44214ce58ec8e20325cdd4"
    )

    # Step 1: Get host ID
    hosts = client._api_request("host.get", {
        "output": ["hostid", "host"]
    })

    host_id = None
    for host in hosts:
        if host["host"] == hostname:
            host_id = host["hostid"]
            break

    if not host_id:
        print(f"Host '{hostname}' not found.")
        return

    # Step 2: Get all items for the host
    items = client._api_request("item.get", {
        "hostids": [host_id],
        "output": ["itemid", "name", "key_", "lastvalue"]
    })

    # Step 3: Filter items with "UNKNOWN" in lastvalue
    unknown_items = [
        item for item in items
        if "UNKNOWN" in item.get("lastvalue", "").upper()
    ]

    if not unknown_items:
        print("No UNKNOWN items found.")
        return

    # Step 4: Delete those items
    item_ids = [item["itemid"] for item in unknown_items]
    client._api_request("item.delete", item_ids)

    print(f"Deleted {len(item_ids)} UNKNOWN items from host '{hostname}'.")

# Example usage
if __name__ == "__main__":
    delete_unknown_items_for_host("redis01.demo.com")
    list_items_for_host("redis01.demo.com")

