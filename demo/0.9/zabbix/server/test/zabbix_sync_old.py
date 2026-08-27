import requests
import json

# === CONFIGURATION ===
ZABBIX_URL = "http://zabbix-web:8080/api_jsonrpc.php"
ZABBIX_USER = "Admin"
ZABBIX_PASS = "zabbix"
CHECK_SCRIPT = "check_howru_api.sh"
HOWRU_API_ADDRESS = "http://host.docker.internal:8085"
ZABBIX_GROUP_ID = "2"  # Default Linux servers group

# === ZABBIX API WRAPPER ===
def zabbix_api(method, params, auth=None, id=1):
    payload = {
        "jsonrpc": "2.0",
        "method": method,
        "params": params,
        "id": id
    }
    if auth:
        payload["auth"] = auth
    response = requests.post(ZABBIX_URL, json=payload).json()
    if "error" in response:
        raise Exception(f"Zabbix API error: {response['error']}")
    return response["result"]

# === AUTHENTICATE ===
auth_token = zabbix_api("user.login", {
    "user": ZABBIX_USER,
    "password": ZABBIX_PASS
})

# === FETCH SERVERS AND JOBS FROM PROXY API ===
servers = requests.get(f"{HOWRU_API_ADDRESS}/api/listservers").json()
jobs_data = requests.get(f"{HOWRU_API_ADDRESS}/api/listjobs").json()

# === SYNC HOSTS AND ITEMS ===
for entry in jobs_data:
    server = entry["server"]
    jobs = entry["jobs"]

    # --- Ensure host exists ---
    host_resp = zabbix_api("host.get", {
        "filter": {"host": [server]},
        "output": ["hostid"]
    }, auth_token)

    if host_resp:
        hostid = host_resp[0]["hostid"]
    else:
        host_create = zabbix_api("host.create", {
            "host": server,
            "interfaces": [{
                "type": 1,
                "main": 1,
                "useip": 1,
                "ip": "127.0.0.1",
                "dns": "",
                "port": "10050"
            }],
            "groups": [{"groupid": ZABBIX_GROUP_ID}]
        }, auth_token)
        hostid = host_create["hostids"][0]

    # --- Create items for each job ---
    for job in jobs:
        plugin_name = job["name"]
        description = job["description"]
        item_key = f'{CHECK_SCRIPT}["{HOWRU_API_ADDRESS}","{plugin_name}","{server}"]'

        # Check if item already exists
        existing = zabbix_api("item.get", {
            "hostid": hostid,
            "filter": {"key_": item_key}
        }, auth_token)

        if not existing:
            zabbix_api("item.create", {
                "name": f"{description} ({plugin_name})",
                "key_": item_key,
                "hostid": hostid,
                "type": 10,  # External check
                "value_type": 1,  # Text
                "interfaceid": 0
            }, auth_token)
            print(f"✔ Created item: {description} on {server}")
        else:
            print(f"✓ Item already exists: {description} on {server}")

