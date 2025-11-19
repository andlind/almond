import socket
import json
import re
import os
import sys
import argparse
import signal

def load_json(path):
    if not os.path.exists(path):
        print(f"Error: JSON file '{path}' not found.")
        sys.exit(1)
    try:
        with open(path) as f:
            return json.load(f)
    except json.JSONDecodeError:
        print(f"Error: Failed to parse JSON file '{path}': {e}")
        sys.exit(1)

def handle_client(conn, data, plugins):
    try:
        conn.sendall(f"# munin node at {data['host']['name']}\n".encode())
        while True:
            raw = conn.recv(1024)
            if not raw:
                break
            cmd = raw.decode("ascii", errors="ignore").strip()
            if not cmd:
                continue            
            if cmd == "list":
                conn.sendall((" ".join(plugins.keys()) + "\n").encode())
            elif cmd.startswith("config "):
                name = cmd.split()[1]
                plugin = plugins[name]
                conn.sendall(f"graph_title {plugin['pluginName']}\n".encode())
                conn.sendall(f"{name}.label {plugin['pluginName']}\n".encode())
                conn.sendall(".\n".encode())
            elif cmd.startswith("fetch "):
                name = cmd.split()[1]
                plugin = plugins[name]
                # Extract perfdata after '|'
                if "|" in plugin["pluginOutput"]:
                    perfdata = plugin["pluginOutput"].split("|",1)[1]
                    # Example: rta=0.086ms;100.000;500.000;0;
                    for item in perfdata.split():
                        k,v = item.split("=")[0], item.split("=")[1].split(";")[0]
                        v_clean = re.sub(r"[^0-9.\-]", "", v)
                        conn.sendall(f"{name}_{k}.value {v_clean}\n".encode())
                else:
                    v_clean = re.sub(r"[^0-9.\-]", "", plugin["pluginStatusCode"])
                    conn.sendall(f"{name}.value {v_clean}\n".encode())
                conn.sendall(".\n".encode())
            elif cmd == "quit":
                break
    except Exception as e:
        print(f"Client error: {e}")
    finally:
        conn.close()

def main():
    global s, json_path, data, plugins
    s = None
    json_path = None
    data = None
    plugins = None
    parser = argparse.ArgumentParser(description="Munin node serving JSON monitoring data")
    parser.add_argument("jsonfile", help="Path to JSON file with monitoring data")
    args = parser.parse_args()

    json_path = args.jsonfile

    data = load_json(json_path)
    plugins = {m["name"]: m for m in data["monitoring"]}

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.bind(("0.0.0.0", 4949))
        s.listen(5)
    except OSError as e:
        print(f"Error: Could not bind to port 4949: {e}")
        sys.exit(1)

    print(f"Munin node running on port 4949, using {args.jsonfile}")
    
    def handle_sigterm(signum, frame):
        print("\nAlmond munin client stopped (SIGTERM)")
        if s:
            s.close()
        sys.exit(0)
    def handle_sighup(signum, frame):
        print("\nReloading JSON file (SIGHUP)")
        try:
            new_data = load_json(json_path)
            new_plugins = {m["name"]: m for m in data["monitoring"]}
            globals()["data"] = new_data
            globals()["plugins"] = new_plugins
            print("Reload successful")
        except Exception as e:
            print(f"Reload failed: {e}")

    signal.signal(signal.SIGTERM, handle_sigterm)
    signal.signal(signal.SIGHUP, handle_sighup)

    try:
        while True:
            conn, addr = s.accept()
            handle_client(conn, data, plugins)
    except KeyboardInterrupt:
        print("\nAlmond munin client stopped")
    finally:
        s.close()

if __name__ == "__main__":
    main()

