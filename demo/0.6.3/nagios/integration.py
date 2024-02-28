import requests
import argparse
import sys
import shutil
import schedule
import time
import functools

host_list = []
inventory_host_list = []
changes = 0
global inventory
global server_list

file_header = "##############################################################################\n# HOWRU.CFG - CREATED BY ALMOND HOWRU API TO NAGIOS INTEGRATION\n#\n#\n"
file_header = file_header + "##############################################################################\n#\n## HOST DEFINITION\n#\n#\n###############################################################################\n"
file_header = file_header + "###############################################################################\n\n# Define a hosts for the local machine\n\n"
service_header = "\n\n###############################################################################\n###############################################################################\n#\n"
service_header = service_header + "# HOST GROUP DEFINITION\n#\n###############################################################################\n###############################################################################\n"
service_header = service_header + "\n# Define an optional hostgroup for machines imported by Almond integration\n\ndefine hostgroup{\n\thostgroup_name  almond-servers ; The name of the hostgroup\n\t"
service_header = service_header + "alias           Almond Servers ; Long name of the group\n"

headers = {
    'User-Agent': 'Almond-Nagios-Integration(v1.0)',
    'Accept': 'application/json, text/plain, */*',
    # 'Accept-Encoding': 'gzip, deflate, br',
    'Connection': 'keep-alive',
    'Sec-Fetch-Dest': 'empty',
    'Sec-Fetch-Mode': 'cors',
    'Sec-Fetch-Site': 'same-site',
    'Pragma': 'no-cache',
    'Cache-Control': 'no-cache',
}

params = {
    'assetclass': 'stocks',
}

def get_collector_host_list(url):
    global host_list
    global server_list
    print (url)
    response = requests.get(url, params=params, headers=headers)
    data = response.json()
    server_list = data.get('server')
    for x in server_list:
        this_host_name = x['host']['name']
        if this_host_name not in host_list:
            host_list.append(this_host_name)
    host_list.sort()

def get_inventory_host_list(path):
    global inventory_host_list
    global inventory
    print (path)
    with open(path) as inventory_file:
        inventory = inventory_file.readlines()
        for line in inventory:
            if 'host_name' in line:
                host_data = line.split()
                host_name = host_data[1]
                if (host_name not in inventory_host_list):
                    inventory_host_list.append(host_data[1])
    inventory_host_list.sort()

def compare_list(list1, list2, set_order):
    global changes
    s = set(list1)
    temp = [x for x in list2 if x not in s]
    if len(temp) > 0:
        if set_order == 0:
            print ("These server names are found on Nagios but should not be in use.")
        else:
            print ("These server names are found and should be added to Nagios configuration")
        print (temp)
        changes += len(temp)

def compare_services():
    global inventory
    global server_list
    global changes
    num_service_checks = 0
    inventory_service_list = []
    
    for linenum, line in enumerate(inventory):
        if 'define service{' in line:
            inventory_service_list.append(inventory[linenum:linenum+6])

    print ("Number of service checks in inventory:", len(inventory_service_list))
    for x in server_list:
        for y in x['monitoring']:
            num_service_checks += 1

    print("Number of service checks in api: " + str(num_service_checks))
    if (len(inventory_service_list) > num_service_checks):
        changes = changes + len(inventory_service_list) - num_service_checks
    if (num_service_checks > len(inventory_service_list)):
        changes = changes + num_service_checks - len(inventory_service_list)

def createNewConfig(file):
    global host_list
    global server_list
    global file_header
    
    backup_file = file + ".bak"
    shutil.copyfile(file, backup_file)
    f = open(file, "w")
    f.write(file_header)
    for x in host_list:
        f.write("\ndefine host{\n")
        f.write("\tuse                     linux-server            ; Name of host template to use\n")
        f.write("\thost_name               ")
        f.write(x)
        f.write("\n\talias                   ")
        f.write(x)
        f.write("\n\taddress                 127.0.0.1\n")
        f.write("}\n")
    f.write(service_header)
    f.write("\tmembers         ")
    for y in range(len(host_list)):
        f.write(host_list[y])
        if (y < len(host_list)-1):
            f.write(",")
        else:
            f.write("; Comma separated list of hosts that belong to this group\n}")
    f.write("\n\n###############################################################################\n")
    f.write("###############################################################################\n")
    f.write("#\n# SERVICE DEFINITIONS\n#\n###############################################################################\n")
    f.write("###############################################################################\n\n")
    for x in server_list:
        count = 0
        host = x['host']['name']
        # Source 0 is source url and Source 1 is port
        source = x['host']['source'].split(':')
        services = x['monitoring']
        for y in services:
            f.write("define service{\n\tuse                             local-service,graphed-service         ; Name of service template to use\n\t")
            f.write("host_name                       ")
            f.write(host)
            f.write("\n\tservice_description             ")
            f.write(y['pluginName'])
            f.write("\n\tcheck_command                   check_howru_service_by_id!")
            f.write(source[0])
            f.write("!")
            f.write(source[1])
            f.write("!plugin!")
            count = count + 1
            f.write(str(count))
            f.write("!")
            f.write(host)
            f.write("\n}\n\n")
    f.close()

def do_integrate(url,file):
    global host_list
    global inventory_list

    get_collector_host_list(url)
    get_inventory_host_list(file)
    compare_list(host_list, inventory_host_list, 0)
    compare_list(inventory_host_list, host_list, 1)
    if changes < 1:
        compare_services()
    if changes > 0:
        print ("Create new config")
        createNewConfig(file)
    else:
        print ("Nothing to do")

def main():
    parser = argparse.ArgumentParser(description='Almond HowRU API Nagios integration')
    parser.add_argument('-u', '--url', type=str, required=True, help='Url to collector, for an example http://localhost:80')
    parser.add_argument('-f', '--file', type=str, required=True, help='Path to config file, for an example /opt/nagios/etc/conf.d/almond.cfg')
    parser.add_argument('-s', '--sleep', type=int, required=False, default=5, help='Number of minutes between integrations. Default is 5')
    args = parser.parse_args()
    #get_collector_host_list(args.url)
    #get_inventory_host_list(args.file)
    #compare_list(host_list, inventory_host_list, 0)
    #compare_list(inventory_host_list, host_list, 1)
    #if changes < 1:
    #    compare_services()
    #if changes > 0:
    #    print ("Create new config")
    #    createNewConfig(args.file)
    #else:
    #    print ("Nothing to do")
    schedule.every(args.sleep).minutes.do(functools.partial(do_integrate, args.url, args.file))
    while True:
        schedule.run_pending()
        time.sleep(1)

if __name__=="__main__":
    sys.exit(main())
