import requests
import argparse
import sys
import shutil
import os
import subprocess

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
    this_url = "http://" + url
    response = requests.get(this_url, params=params, headers=headers)
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
    # TODO: Add check if service name and or command values changed as well
    # Currently we only check if the total number of services differ,
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

def check_for_howru_command(cfile):
    if os.path.isfile(cfile):
        with open(cfile, 'r') as file:
            content = file.read()
            if 'check_howru_service_by_id' in content:
                return True
            else: 
                return False
    else:
        print ("Could not find Nagios command file. Aborting.")
        sys.exit(1)

def add_howru_check_command(cfile):
    with open(cfile, "a") as command_file:
        command_file.write("################################################################################\n")
        command_file.write("#\n# HOWRU PLUGIN used by Almond Nagios Integration\n#\n")
        command_file.write("# Make sure you have the plugin located in $USER2 directory\n#\n")
        command_file.write("################################################################################\n\n")
        command_file.write("define command{\n\tcommand_name    check_howru_service_by_id\n")
        command_file.write("\tcommand_line    $USER2$/check_howru.py -H $ARG1$ -P $ARG2$ -A $ARG3$ -i $ARG4$ -e $ARG5$\n")
        command_file.write("}\n")

def check_nagios_config(command, conffile):
    command_sp = command.split()
    result = subprocess.run([command_sp[0], command_sp[1], command_sp[2]], stdout=subprocess.PIPE)
    if result.returncode != 0:
        print ("Nagios config has errors")
        print (result.stdout.decode('utf-8'))
        print ("Revert to old configuration")
        backupfile = conffile + ".bak"
        shutil.copyfile(backupfile, conffile)
    else:
        print (result.stdout.decode('utf-8'))
        print ("New config looks fine :)")

def reload_nagios(cmd):
    cmd_sp = cmd.split()
    result = subprocess.run([cmd_sp[0], cmd_sp[1]], stdout.subprocess.PIPE)
    if result.returncode == 0:
        print ("Nagios reloaded with new config.")
    else:
        print ("Nagios failed to reload...")
        print (result.stdout.decode('utf-8'))

def main():
    parser = argparse.ArgumentParser(description='Almond HowRU API Nagios integration')
    parser.add_argument('-u', '--url', type=str, required=True, help='Url to collector, for an example http://localhost:80')
    parser.add_argument('-f', '--file', type=str, required=True, help='Path to config file, for an example /opt/nagios/etc/conf.d/almond.cfg')
    parser.add_argument('-c', '--commands', type=str, required=False, default='/opt/nagios/etc/objects/commands.cfg')
    parser.add_argument('-n', '--nagioscheck', type=str, required=False, default="/opt/nagios/bin/nagios -v /opt/nagios/etc/nagios.cfg")
    parser.add_argument('-r', '--reload', action='store_true')
    parser.add_argument('-x', '--reloadcmd', type=str, required=False, default="/etc/rd.d/init.d/nagios reload")
    args = parser.parse_args()
    command_file = args.commands
    check_command = args.nagioscheck
    get_collector_host_list(args.url)
    get_inventory_host_list(args.file)
    compare_list(host_list, inventory_host_list, 0)
    compare_list(inventory_host_list, host_list, 1)
    if changes < 1:
        compare_services()
    if changes > 0:
        print ("Create new config")
        if not check_for_howru_command(command_file):
            add_howru_check_command(command_file)
        createNewConfig(args.file)
        check_nagios_config(check_command, args.file)
        if args.reload:
            reload_nagios(args.reloadcmd)
    else:
        print ("Nothing to do")

if __name__=="__main__":
    sys.exit(main())
