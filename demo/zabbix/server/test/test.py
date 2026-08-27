import requests

ZABBIX_URL = "http://zabbix-web:8888/api_jsonrpc.php"
AUTH_TOKEN = "30cd9af27796854e45945f901d57cdef"

headers = {
    "Content-Type": "application/json",
    "Authorization": f"Bearer {AUTH_TOKEN}"
}

expression = r"last(/app01.demo.com/check_howru_api.sh[http://host.docker.internal:8085,check_load,app01.demo.com])>0"

payload = {
    "jsonrpc": "2.0",
    "method": "trigger.create",
    "params": {
        "description": "App load is not OK",
        "expression": expression,
        "priority": 4,
        "status": 0
    },
    "id": 1
}

response = requests.post(ZABBIX_URL, json=payload, headers=headers)
print(response.json())

