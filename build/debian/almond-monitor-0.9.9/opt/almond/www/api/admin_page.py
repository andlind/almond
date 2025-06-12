import subprocess
import json
import shutil
import re
import os.path
import os
import socket
import logging
from os import walk
from flask import Blueprint
from flask import current_app
from flask_httpauth import HTTPBasicAuth
from flask import render_template, session, request, make_response, redirect
import matplotlib.pyplot as plt
from werkzeug.security import check_password_hash, generate_password_hash
from collections import deque
from venv import logger
from auth2fa import auth_blueprint

admin_page = Blueprint('admin_page', __name__, template_folder='templates')

graph_written = 0
plugins = []
conf = []
api_conf = []
scheduler_conf = []
extra_conf = []
graph_names = {}
api_available_conf = ['api.adminUser', 'api.adminPassword', 'api.authType', 'api.bindPort', 'api.enableAliases', 'api.enableFile', 'api.enableScraper', 'api.dataDir','api.isContainer', 'api.multiMetrics', 'api.multiServer', 'api.sslCertificate', 'api.sslKey', 'api.startPage', 'api.stateType', 'api.useGUI', 'api.userFile', 'api.useSSL', 'api.wsgi', 'data.jsonFile', 'data.metricsFile', 'scheduler.storeDir', 'scheduler.configFile', 'scheduler.dataDir', 'plugins.directory', 'plugins.declaration']
scheduler_available_conf = ['almond.api', 'almond.port', 'almond.standalone', 'almond.useSSL', 'almond.certificate', 'almond.key', 'data.jsonFile', 'data.saveOnExit', 'data.metricsFile', 'data.metricsOutputPrefix', 'plugins.directory', 'plugins.declaration', 'scheduler.useTLS', 'scheduler.certificate', 'scheduler.key','scheduler.confDir', 'scheduler.logDir', 'scheduler.logToStdout', 'scheduler.logPluginOutput', 'scheduler.storeResults', 'scheduler.format', 'scheduler.initSleepMs', 'scheduler.sleepMs', 'scheduler.truncateLog', 'scheduler.truncateLogInterval', 'scheduler.tuneTimer', 'scheduler.tunerCycle', 'scheduler.tuneMaster', 'scheduler.dataDir', 'scheduler.storeDir', 'scheduler.hostName', 'scheduler.enableGardener', 'scheduler.gardenerScript', 'scheduler.gardenerRunInterval', 'scheduler.quickStart', 'scheduler.metricsOutputPrefix', 'scheduler.enableClearDataCache', 'scheduler.enableKafkaExport', 'scheduler.enableKafkaTag', 'scheduler.enableKafkaId', 'scheduler.kafkaStartId', 'scheduler.kafkaBrokers', 'scheduler.kafkaTopic', 'scheduler.kafkaTag', 'scheduler.enableKafkaSSL', 'scheduler.kafkaCACertificate', 'scheduler.kafkaProducerCertificate', 'scheduler.kafkaSSLKey', 'scheduler.clearDataCacheInterval', 'scheduler.dataCacheTimeFrame', 'scheduler.type', 'gardener.CleanUpTime']
users = {}
current_version = '0.9.9.6'

enable_gui = True
standalone = True
almond_api = False
is_container = False
logger_enabled = False
almond_port = 9909
jasonFile = '/opt/almond/data/monitor.json'
store_dir = '/opt/almond/data/metrics'
plugins_directory = '/opt/almond/plugins'
declaration_file = '/etc/almond/plugins.conf'
admin_user_file = '/etc/almond/users.conf'
almond_conf_file = '/etc/almond/almond.conf'
api_conf_file = '/etc/almond/almond.conf'
metrics_file_name = 'monitor.metrics'
start_page = 'admin'
state_type='systemctl'
user_secrets = {}

#auth = HTTPBasicAuth()

def load_plugins():
    global plugins
    global declaration_file

    f = open(declaration_file)
    read_data = f.read()
    plugins = read_data.split("\n")
    plugins.pop()
    header = plugins[0][1:]
    pos = header.find('<')
    header = plugins[0][1:pos]
    plugins[0] = header
    f.close()

def load_conf(isGlobal):
    global conf
    global extra_conf
    global almond_conf_file
    global api_conf_file

    if (isGlobal):
        f = open(almond_conf_file)
        read_data = f.read()
        conf = read_data.split("\n")
        f.close()
    else:
        conf_count = 0
        if os.path.isfile('/etc/almond/admin.conf'):
            f = open("/etc/almond/admin.conf", "r")
            read_data = f.read()
            conf = read_data.split("\n")
            f.close()
            api_conf_file = "/etc/almond/admin.conf"
            conf_count += 1
        if os.path.isfile('/etc/almond/api.conf'):
            f = open("/etc/almond/api.conf", "r")
            read_data = f.read()
            if conf_count > 0:
                extra_conf = read_data.split("\n")
                extra_conf = [i for i in extra_conf if i]
            else:
                conf = read_data.split("\n")
            f.close()
            if not 'admin' in api_conf_file:
                api_conf_file = "/etc/almond/api.conf"
            conf_count += 1
        f = open("/etc/almond/almond.conf", "r")
        read_data = f.read() 
        if conf_count == 0:
            conf = read_data.split("\n")
            f.close()
        else:
            gl_conf = read_data.split("\n")
            for x in gl_conf:
                if x not in extra_conf:
                    if not x == "":
                        extra_conf.append(x)
            f.close()
        if conf_count == 0:
            api_conf_file = "/etc/almond/almond.conf"
    conf.pop()

def load_scheduler_conf():
    global conf
    global scheduler_conf
    load_conf(True)
    this_conf = conf.copy()
    scheduler_conf = [x for x in this_conf if not 'api.' in x]

def load_api_conf():
    global conf
    global extra_conf
    global api_conf

    pop_list = []
    prefixes = ('almond.', 'scheduler.', 'gardener.', 'data.', 'plugins.')
    load_conf(False)
    this_conf = conf.copy()
    api_conf = [x for x in this_conf if not x.startswith(prefixes)]
    if extra_conf:
        that_conf = extra_conf.copy()
        extra_conf = [x for x in that_conf if not x.startswith(prefixes)]
        count = 0
        list_len = len(extra_conf)
        while count < list_len:
            pos = extra_conf[count].find('=')
            item = extra_conf[count][:pos]
            for x in api_conf:
                if item == x[:pos]:
                    pop_list.append(count)
            count += 1
        if pop_list:
            pop_list.sort(reverse=True)
            for x in pop_list:
                extra_conf.pop(x)

def set_new_password(username, password):
    global admin_user_file

    # Ensure we remove leading/trailing whitespace
    username = username.strip()
    password = password.strip()

    print("DEBUG: Set password for '{}' as password '{}'".format(username, password))
    logger.info(session['user'] + " trying to set new password for user '" + username + "'.")

    # Validate input upfront
    if len(username) < 4 or len(password) < 6:
        info = "Error updating credentials"
        logger.warning("Failed updating password for user '" + username + "'.")
        return info

    # Merge all user entries into one dictionary
    user_data = {}
    with open(admin_user_file, 'r') as file:
        for line in file:
            try:
                # Each line should be a JSON object with a single key-value pair
                entry = json.loads(line.strip())
                user_data.update(entry)
            except json.JSONDecodeError:
                logger.error("JSON decoding failed for line: '{}'".format(line.strip()))
                continue

    # Debug: log the old password hash if it exists
    old_hash = user_data.get(username)
    logger.debug("OLD HASH for user '{}': {}".format(username, old_hash))

    # Generate new hash and update the user entry
    new_hash = generate_password_hash(password)
    logger.debug("NEW HASH for user '{}': {}".format(username, new_hash))
    user_data[username] = new_hash

    # Write all user credentials back to the file
    with open(admin_user_file, 'w') as file:
        for user, pwd_hash in user_data.items():
            file.write(json.dumps({user: pwd_hash}) + "\n")

    info = "Credentials updated."
    logger.info("Password updated for user '{}'.".format(username))
    return info

#def set_new_password(username, password):
#    global admin_user_file
#
#    lines = []
#    filtered_lines = []
#    print("DEBUG: Set password for %s as password %s" % (username, password))
#    logger.info(session['user'] + " trying to set new password for user '" + username + "'.")
#    update_credentials = True
#    if len(username.strip()) < 4:
#        # Username should contain at least 4 characters
#        update_credentials = False
#    if len(password.strip()) < 6:
#        # Password should contain at least 6 characters
#        update_credentials = False
#    #users = {
#    #    username: generate_password_hash(password.strip())
#    #}
#    if update_credentials:
#        # remove old first
#        with open(admin_user_file, 'r') as file:
#            lines = file.readlines()
#            #filtered_lines = [line for line in lines if username.lower() not in line.lower()]
#        for line in lines:
#            try:
#                user_data = json.loads(line.strip())
#                if username not in user_data.keys():
#                    filtered_lines.append(line.strip() + '\n')
#            except json.JSONDecodeError:
#                continue
#
#        with open(admin_user_file, 'w') as file:
#            #file.writelines(filtered_lines)
#            #file.write(json.dumps(users))
#            #file.writelines(line.rstrip('\n') + '\n' for line in filtered_lines) 
#            #file.write(json.dumps(users) + '\n') 
#            file.writelines(filtered_lines)
#            file.write(json.dumps({username: generate_password_hash(password.strip())}) + '\n')
#
#        info = "Credentials updated."
#        logger.info("Password updated for user '" + username + "'.")
#    else:
#        info = "Error updating credentials"
#        logger.warning("Failed updating password for user '" + username + "'.")    
#    return info

def delete_user_entries():
    new_lines = []
    if os.path.isfile('/etc/almond/admin.conf'):
        f = open("/etc/almond/admin.conf", "r+")
    elif os.path.isfile('/etc/almond/api.conf'):
        f = open("/etc/almond/api.conf", "r+")
    else:
        f = open("/etc/almond/almond.conf", "r+")
    lines = f.readlines()
    for line in lines:
        if ("adminPassword" in line):
            print ("Delete admin password from config.")
        elif ("adminUser" in line):
            print ("Delete admin usernanme from config.");
        else:
            new_lines.append(line)
    f.seek(0)
    f.truncate()
    f.writelines(new_lines)
    f.close()

def rewrite_config(conf, newlines):
    new_lines = []
    f = open(conf, "r+")
    lines = f.readlines()
    for line in lines:
        o_pos = line.find('=')
        o_val = line[o_pos+1:]
        new_line = ""
        for new in newlines:
            pos = new.find('=')
            namestr = new[:pos]
            if namestr in line:
                new_line = new
        if not new_line == "":
            new_lines.append(new_line + '\n')
        else:
            new_lines.append(line)
    for line in newlines:
        has_addition = True
        item = line.split('=')
        for confline in lines:
            confitem = confline.split('=')
            if item[0] == confitem[0]:
                has_addition = False
        if has_addition:
            new_lines.append(line + '\n')
    new_lines.sort()
    f.seek(0)
    f.truncate()
    f.writelines(new_lines)
    f.close()
    return_list = []
    for element in new_lines:
        return_list.append(element.strip())
    return return_list

def read_conf():
    global standalone
    global enable_gui
    global jasonFile
    global store_dir
    global plugins_directory
    global declaration_file
    global admin_user_file
    global start_page
    global state_type
    global almond_conf_file
    global almond_api
    global metrics_file_name

    admin_password = ''
    admin_user = ''
    json_file = 'monitor.json'

    load_conf(False)
    for x in conf:
        if (x.find('almond') == 0):
            if (x.find('api') > 0):
                pos = x.find('=')
                api_enabled = x[pos+1]
                if (isinstance(int(api_enabled), int)):
                    if (int(api_enabled) > 0):
                        almond_api = True
                    else:
                        almond_api = False
                else:
                    almond_api = False
            if (x.find('port') > 0):
                pos = x.find('=')
                alport = x[pos+1]
                if (isinstance(int(alport), int)):
                    if (int(alport) > 0):
                        almond_port = int(alport)
                    else:
                        almond_port = 9909
                else:
                    almond_port = 9909
        if (x.find('data') == 0):
            if (x.find('jsonFile') > 0):
                pos = x.find('=')
                json_file = x[pos+1:].rstrip()
            if (x.find('metricsFile') > 0):
                pos = x.find('=')
                metrics_file_name = x[pos+1:].rstrip()
        if (x.find('api') == 0):
            if (x.find('multiServer') > 0):
                pos = x.find('=')
                multi = x[pos+1]
                if (isinstance(int(multi), int)):
                    if (int(multi) > 0):
                        standalone = False
                    else:
                        standalone = True
                else:
                    standalone = False
            if (x.find('dataDir') > 0):
                pos = x.find('=')
                data_dir = x[pos+1:].rstrip()
            if (x.find('useGUI') > 0):
                pos = x.find('=')
                usegui = x[pos+1]
                if (isinstance(int(usegui), int)):
                    if (int(usegui) > 0):
                        enable_gui = True
                    else:
                        enable_gui = False
                else:
                    enable_gui = False
            if (x.find('adminUser') > 0):
                pos = x.find('=')
                admin_user = x[pos+1:].rstrip()
            if (x.find('adminPassword') > 0):
                pos = x.find('=')
                admin_password = x[pos+1:].rstrip()
            if (x.find('userFile') > 0):
                pos = x.find('=')
                admin_user_file = x[pos+1:].rstrip()
            if (x.find('startPage') > 0):
                pos = x.find('=')
                start_page = x[pos+1:].rstrip()
            if (x.find('stateType') > 0):
                pos = x.find('=')
                state_type = x[pos+1:].rstrip()
        if (x.find('scheduler') == 0):
            if (x.find('storeDir') > 0):
                pos = x.find('=')
                store_dir = x[pos+1:].rstrip()
            if (x.find('confFile') > 0):
                pos = x.find('=')
                almond_conf_file = x[pos+1:].rstrip()
        if (x.find('plugins') == 0):
            if (x.find('directory') > 0):
                pos = x.find('=')
                plugins_directory = x[pos+1:].rstrip()
            if (x.find('declarations') > 0):
                pos = x.find('=')
                delcaration_file = x[pos+1:].rstrip()

    jasonFile = data_dir + '/' + json_file
    if (len(admin_user) > 0) and (len(admin_password) > 4):
        set_new_password(admin_user, admin_password)
        delete_user_entries()

def list_available_plugins():
    global plugins_directory

    #plugin_list = next(walk("/usr/local/nagios/libexec"), (None, None, []))[2]
    plugin_list = next(walk(plugins_directory), (None, None,  []))[2]
    if 'utils.sh' in plugin_list:
        plugin_list.remove('utils.sh')
    if 'utils.pm' in plugin_list:
        plugin_list.remove('utils.pm')
    plugin_list.sort()
    return plugin_list

def get_status(this_data):
    global jasonFile

    hostname = socket.getfqdn()
    if not os.path.isfile(jasonFile):
        resultString = hostname + ";4;" + "No data file"
        return resultString
    ret_code = 0
    num_of_oks = 0
    num_of_warnings = 0
    num_of_criticals = 0
    num_of_unknowns = 0
    mon_obj = this_data.get("monitoring")
    for obj in mon_obj:
        status_code = int(obj.get('pluginStatusCode'))
        if (status_code > ret_code):
            if (ret_code < 3):
                ret_code = status_code
        if (status_code == 0):
            num_of_oks += 1
        elif (status_code == 1):
            num_of_warnings += 1
        elif (status_code == 2):
            num_of_criticals += 1
        elif (status_code < 0 and status_code < 2):
            num_of_unknowns += 1
        else:
            print ("Could not parse status code")
    resultString = hostname + ";" + str(ret_code) + ";" + str(num_of_oks) + ";" + str(num_of_warnings) + ";" + str(num_of_criticals) + ";" + str(num_of_unknowns)
    return resultString

def get_infostr(data):
    infostr = ""
    d_array = data.split(';')
    #for index, x in enumerate(d_array):
    #    print(f"Position {index}: {x}")
    s_code = d_array[1]
    if (len(d_array) > 2):
        tot_checks = int(d_array[2]) + int(d_array[3]) + int(d_array[4]) + int(d_array[5])
        infostr = d_array[0] + " has run " + str(tot_checks) + " checks. " + str(d_array[2]) + " where ok, " + str(d_array[3]) + " where warnings, " + str(d_array[4]) + " where criticals and " + str(d_array[5]) + " where unknown."
    else:
        infostr = d_array[2]
    return infostr

def load_status_data():
    global jasonFile

    if os.path.isfile(jasonFile):
        f = open(jasonFile, "r")
        data = json.loads(f.read())
        f.close()
    else:
        data = {
            "host": {
                "name":"almond01.domain.com"
        },
        "monitoring": [
            {
                "name": "Almond reservice is stopped or restarting.",
                "pluginName": "Not loaded"
            }
          ]
        }    
    return data

def delete_plugin_object(id):
    global declaration_file

    with open(declaration_file, "r+") as fp:
        lines = fp.readlines()
        del lines[int(id)+1]
        fp.seek(0)
        fp.truncate()
        fp.writelines(lines)

def update_plugin_object(id, t):
    global declaration_file

    with open(declaration_file, "r+") as fp:
        lines = fp.readlines()
        new_val = t.strip() + '\n'
        lines[int(id)+1] = new_val
        fp.seek(0)
        fp.truncate()
        fp.writelines(lines)

def execute_plugin_object(id):
    # Check if almond is binding and on which port
    global almond_api
    global almond_port
    global is_container

    totalsent = 0
    is_container = current_app.config['IS_CONTAINER']

    if (almond_api):
        #if in container
        #container_ip = socket.gethostbyname(socket.gethostname())
        if is_container == 'true':
            container_ip = socket.gethostbyname(socket.gethostname())
        clientSocket = None
        try:
            clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            clientSocket.settimeout(30)
        except socket.error as e:
            print ("Error creating socket: %s" % e)
            return 2;
        try:
            #if in container
            if is_container == 'true':
                clientSocket.connect((container_ip, almond_port))
            else:
                clientSocket.connect(("127.0.0.1",almond_port))
        except socket.gaierror as e:
            print ("Address-related error connecting to server: %s" % e)
            return 1;
        except socket.error as e:
            print ("Connection error: %s" % e)
            return 2;
        data = f'{{"action":"execute", "id":"{id}", "token":"xtw%p15899764887938680313afghk"}}'
        try:
            clientSocket.send(data.encode())
        except socket.error as e:
            print ("Error sending data: %s" % e) 
            return 2;
        try:
            retVal = clientSocket.recv(1024)
        except socket.error as e:
            print ("Error receiving data: %s" % e)
            return 1;
        finally:
            if clientSocket:
                clientSocket.close()
        if not len(retVal):
            print ("No retVal len\n")
        # Return value
        #print(retVal.decode())
        return 0;
    else:
        printf("Almond api is not enabled.")
        return 1;

def add_plugin_object(description, plugin, arguments, interval, active):
    global declaration_file

    write_active = "0"
    if (active):
        write_active = "1"
    # Check description
    write_str = description + ";" + plugin + " " + arguments + ";" + write_active + ";" + interval +  "\n"
    f = open(declaration_file, "a")
    f.write(write_str)
    f.close()

def check_service_state(service):
    global state_type

    retArr = []
    runcmd = "/usr/bin/supervisorctl status " + service
    if (state_type == "supervisorctl"):
        runcmd = "/usr/bin/supervisorctl status " + service
    elif (state_type == "systemctl"):
        runcmd = "/bin/systemctl status " + service
    else:
        runcmd = ""
    if not (runcmd == ""):
        p = subprocess.Popen(runcmd, stdout=subprocess.PIPE, shell=True)
        (output, err) = p.communicate()
        p_status = p.wait()
        retArr.append(str(p_status))
        retArr.append(output.decode("utf-8"))
    else:
        retArr.append("3")
        retArr.append("No information available")
    return retArr

def compare_lists(list1, list2):
    return_list = []
    for element in list1:
        if element not in list2:
            return_list.append(element)
    return return_list

def get_logs(dir):
    logfiles = next(walk(dir), (None, None, []))[2]
    return logfiles

def set_graph_names():
    global plugins, graph_names

    graph_names = {}
    load_plugins()
    for plugin in plugins:
        this_name = ''
        this_val = ''
        try:
           end_pos = plugin.index(']')
        except ValueError:
            end_pos = -1
        if end_pos > 0:
            this_val = plugin[1:end_pos].strip()
            if not 'service_name' in this_val:
                pos = plugin.find(';')
                this_name = plugin[end_pos+1:pos].strip()
                graph_names[this_name] = this_val

def get_graph_data(name):
    global graph_names

    # enabled?
    # where?
    uptime_percentage = 0.0
    count = 0
    oks = 0
    set_graph_names()
    data_name = graph_names[name];
    filename = '/opt/almond/data/metrics/' + data_name;
    if os.path.isfile(filename):
        graph_file = open(filename, 'r')
        d_dates = []
        d_lines = []
        for line in graph_file:
            count += 1
            if 'OK' in line:
                oks += 1
            l_data = line.split("|")
            d_data = {}
            if (len(l_data) > 1):
                d_key = ''
                d_value = ''
                l_date = l_data[0].split(",")[0]
                d_dates.append(l_date)
                l_stats = l_data[1]
                data_lines = l_stats.split("=")
                if not data_lines[0].isnumeric():
                    d_key = data_lines[0]
                    d_val = data_lines[1].split(";")[0]
                    d_float = re.findall(r'\d+\.\d+', d_val)
                    try:
                        d_data[d_key] = d_float[0]
                    except IndexError:
                        d_int = re.sub('[^0-9]','', d_val)
                        if not d_int.isnumeric():
                            d_int = "0"
                        d_data[d_key] = d_int
                    d_lines.append(d_data)
            else:
                # Does not have metrics
                l_date = l_data[0].split(",")[0]
                try:
                    l_output = l_data[0].split(",")[2]
                except IndexError as error:
                    logger.error("List index of of range: %s", error)
                    l_output = "UNKOWN: Howru internal error"
                l_output = l_output.lower().strip()
                if ('ok') in l_output:
                    ret_val = 0
                elif ('warning') in l_output:
                    ret_val = 1
                elif ('critical') in l_output:
                    ret_val = 2
                else:
                    ret_val = -1
                d_dates.append(l_date)
                d_data['returnValue'] = ret_val
                d_lines.append(d_data)
    else:
        print ("Not found")
    uptime_percentage = round(oks/count * 100, 2)
    return d_dates, d_lines, uptime_percentage

def restart_api():
    p = subprocess.Popen(['/opt/almond/www/api/rs.sh'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

#def requires_auth():
#    def wrapper(f):
#        @wraps(f)
#        def decorated(*args, **kwargs):
#            if 'auth' not in flask.session:
#                return unauthorized_abort()
#            else:
#                if flask.session['first_login']:
#                    return f(*args, **kwargs)
#                else:
#                    return flask.render_template('password.html')
#        return decorated
#    return wrapper

#user = 'admin'
#pw = 'admin'
#
#users = {
#        user: generate_password_hash(pw)
#}

#@auth.verify_password
#def verify_password(username, password):
#    global admin_user_file
#    if os.path.isfile(admin_user_file):
#        users = json.load(open(admin_user_file))
#    else:
#        users = {}
#    if username in users:
#        return check_password_hash(users.get(username), password)
#    return False

def verify_password(username, password):
    global admin_user_file, users
    users = {}
    print("DEBUG: Verify password for %s with password %s" % (username, password))
    if os.path.isfile(admin_user_file):
        with open(admin_user_file, 'r') as f:
            for line_num, line in enumerate(f, 1):
                print ("DEBUG: ", line.strip())
                try:
                    user_data = json.loads(line.strip())
                    print("DEBUG: ", user_data)
                    #username = list(user_data.keys())[0]
                    #users[username] = user_data[username]
                    for user_key, hash_value in user_data.items():
                        users[user_key] = hash_value
                except json.JSONDecodeError as e:
                    print(f"Warning: Invalid JSON format at line {line_num}: {str(e)}")
                    continue
    else:
        users = {}
    if username in users:
        return check_password_hash(users.get(username), password)
    return False

@admin_page.route('/almond/admin', methods=['GET', 'POST'])
#@auth.login_required
def index():
    global plugins
    global scheduler_conf
    global api_conf
    global extra_conf
    global users
    global enable_gui
    global standalone
    global almond_api
    global admin_user_file
    global almond_conf_file
    global api_conf_file
    global store_dir
    global metrics_file_name
    global current_version
    global plugins_directory
    global graph_written
    global logger
    global logger_enabled

    if not logger_enabled:
        logging.basicConfig(filename='/var/log/almond/howru.log', filemode='a', format='%(asctime)s | %(levelname)s: %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
        logger = logging.getLogger()
        logger.setLevel(logging.DEBUG)
        logger.info('Logger for  admin (version:' + current_version + ') enabled')
        logger_enabled = True
    use_port = load_conf(True)
    
    if not enable_gui:
        return render_template("403.html")

    read_conf()
    username = ''
    password = ''
    image_file = '/static/almond_small.png'
    logon_img = '/static/almond.png'
    almond_avatar = '/static/almond_avatar.png'

    if not os.path.isfile(admin_user_file):
        headers = {"Content-Type": "application/text"}
        print ("Invalid userfile")
        logger.warning("Invalid userfile")
        return make_response("Invalid userfile", 404, headers);

    #users = json.load(open(admin_user_file))
    #with open(admin_user_file, 'r') as f:
    #    for line_num, line in enumerate(f, 1):
    #        print ("DEBUG: ", line.strip())
    #        try:
    #            user_data = json.loads(line.strip())
    #            print("DEBUG: ", user_data)
    #            username = list(user_data.keys())[0]
    #            users[username] = user_data[username]
    #        except json.JSONDecodeError as e:
    #            print(f"Warning: Invalid JSON format at line {line_num}: {str(e)}")
    #            continue
    #username = users.get(username)
    #password = ""
    if ('action_type' in request.form):
        if 'login' in session:
            session['login'] = 'true'
            session['user'] = session['user']
        else:
            if (request.form['action_type'] == "create_session"):
                print ("Login")
                logger.info("Creating admin login session")
            else:
                a_auth_type = current_app.config['AUTH_TYPE']
                print("DEBUG: a_auth_type = ", a_auth_type)
                if (a_auth_type == "2fa"):
                    return render_template('login_fa.html', logon_image=logon_img)
                elif (a_auth_type == "basic"):
                    return render_template('login_a.html', logon_image=logon_img)
                else:
                    logger.warning("Could not get auth_type. Returning to basic")
                    return render_template('login_a.html', logon_image=logon_img)
        action_type = request.form['action_type']
        if action_type == "create_session":
            print ("DEBUG: Create session");
            username = request.form['uname']
            password = request.form['psw']
            print ("DEBUG: User/Pwd = %s | %s " % (username, password))
            if verify_password(username.strip(), password.strip()):
                logger.info("User " + username.strip() + " logged in to new session")
                session['login'] = 'true'
                session['user'] = username.strip()
            else:
                logger.warning("Failed login for user " + username.strip())
                session['login'] = 'false'
                session.pop('login', None)
                session.pop('user', None)
        if action_type == 'change_credentials':
            info = ''
            username = request.form['username']
            password = request.form['password']
            set_new_password(username.strip(), password.strip())
            howru_state = check_service_state("howru")
            almond_state = check_service_state("almond")
            data = load_status_data()
            info_data = get_status(data)
            status = get_infostr(info_data)
            logger.info("Rendering admin.html")
            return render_template('admin.html', info = info, version=current_version, username=username, passwd=password, logo_image=image_file, avatar=almond_avatar, almond_state=almond_state, howru_state=howru_state, status=status)
        if action_type == 'plugins':
            if 'delete_line' in request.form:
                line_id = request.form['delete_line']
                logger.info("Rendering deleteplugin.html")
                return render_template('deleteplugin.html', user_image=image_file, line=line_id, avatar=almond_avatar)
            elif 'edit_line' in request.form:
                line_id = request.form['edit_line']
                plugin_text = request.form['edit_text']
                logger.info("Rendering editplugin.html")
                return render_template('editplugin.html', user_image=image_file, line=line_id, text=plugin_text.strip(), avatar=almond_avatar)
            elif 'add_line' in request.form:
                plugin_name = request.form['installed_plugins']
                logger.info("Rendering addplugin.html")
                return render_template('addplugin.html', user_image=image_file, plugin=plugin_name, avatar=almond_avatar)
            elif 'execute' in request.form:
                plugin_id = request.form['plugin_id']
                command = request.form['execute']
                logger.info("Rendering execute.html")
                return render_template('execute.html', user_image=image_file, plugin_id=plugin_id, execute=command, avatar=almond_avatar)
            else:
                print ("None")
                logger.warning("Not having any plugin action type to take care of")
            return action_type
        if action_type == 'scheduler':
            update_lines = []
            write_conf = []
            for key, val in request.form.items():
                if not key == "action_type":
                    line = key + "=" + val
                    update_lines.append(line)
            if update_lines:
                write_conf=rewrite_config(almond_conf_file, update_lines)
            else:
                write_conf=scheduler_conf.copy()
            logger.info("Rendering edit.html")
            return render_template('conf.html', conf = write_conf, info="Config was rewritten", user_image=image_file, avatar=almond_avatar)
        if action_type == 'restart_almond':
            logger.info("Received action type 'restart almond'")
            if state_type == "systemctl":
                return_code = subprocess.call(["/bin/systemctl", "restart", "almond.service"])
            elif state_type == "supervisorctl":
                return_code = subprocess.call(["/usr/bin/supervisorctl", "restart", "almond"])
            else:
                return_code = 3
            info = ""
            if (return_code == 0):
                info = "Process Almond restarted"
                logger.info(info)
            else:
                info = "Could not start Almond. Wrong config?"
                logger.warning(info)
            howru_state = check_service_state("howru")
            almond_state = check_service_state("almond")
            data = load_status_data()
            info_data = get_status(data)
            status = get_infostr(info_data)
            logger.info("Render template admin.html")
            return render_template('admin.html', version=current_version, info=info, logo_image=image_file, avatar=almond_avatar, almond_state=almond_state, howru_state=howru_state, status=status)
        if action_type == 'restart_scheduler':
            logger.info("Received action type 'restart scheduler'")
            if state_type == "systemctl":
                return_code = subprocess.call(["/bin/systemctl", "restart", "almond.service"])
            elif state_type == "supervisorctl":
                return_code = subprocess.call(["/usr/bin/supervisorctl", "restart", "almond"])
            else:
                return_code = 3
            info = ""
            if (return_code == 0):
                info = "Process Almond restarted"
                logger.info(info)
            else:
                info = "Could not start Almond. Wrong config?"
                logger.warning(info)
            logger.info("Rendering template conf.html")
            return render_template('conf.html', conf = scheduler_conf, info=info, user_image=image_file, avatar=almond_avatar)
        if action_type == "execute_plugin":
            logger.info("Received action type 'execute plugin'")
            info = ""
            pid = request.form['plugin_id']
            exr = execute_plugin_object(pid)
            if (exr == 0):
                info = "Plugin execution sent to Almond."
                logger.info(info)
            elif (exr == 1):
                info = "Could not execute. Configuration is not correct."
                logger.warning(info)
            elif (exr == 2):
                info = "Could not execute. Socket error."
                logger.warning(info)
            else:
                info = "Something went wrong?"
                logger.warning(info)
            logger.info("Rendering template plugins.html")
            return render_template('plugins.html', plugins_loaded = plugins, plugins_available = list_available_plugins(), user_image=image_file, avatar=almond_avatar, info=info)
        if action_type == "api":
            logger.info("Received action type 'api'")
            update_lines = []
            write_conf = []
            move_value = False
            for key, val in request.form.items():
                if not key == "action_type":
                    if (val == 'true'):
                        move_value = True
                    if not (val == 'false' or val == 'true'):
                        line = key + "=" + val
                        update_lines.append(line)
            if standalone:
                info = "Config was rewritten"
                logger.info(info)
            else:
                info = "Config rewritten. Note! API is running in multimode, but this config will only apply to the local server."
                logger.info(info)
            if move_value:
                form_keys = []
                form_vals = []
                for x in update_lines:
                    item = x.split('=')
                    form_keys.append(item[0])
                    form_vals.append(item[1])
                logger.info("Rendering template confirm_move.html")
                return render_template('confirm_move.html', keys=form_keys, vals=form_vals, user_image=image_file, avatar=almond_avatar)
            if update_lines:
                write_conf = rewrite_config(api_conf_file, update_lines)
            else:
                write_conf = api_conf.copy()
            logger.info("Rendering template howruconf.html")
            return render_template('howruconf.html', conf=write_conf, user_image=image_file, avatar=almond_avatar, info=info)
        if action_type == "restart_api":
            logger.info("Received action_type 'restart_api'")
            #return "You need to run systemctl restart howru-api.service"
            restart_api() 
            logger.info("Rendering template restart.html")
            return render_template('restart.html', user_image=image_file)
        if action_type == "deleteplugin":
            logger.info("Received action type 'deleteplugin'")
            line_id = request.form['line']
            delete_plugin_object(line_id)
            #return_code = subprocess.call(["/bin/systemctl", "restart", "almond.service"])
            info = "Object deleted. Almond process reloads."
            logger.info(info)
            #if (return_code == 0):
            #    info = "Object deleted and process Almond restarted"
            #else:
            #    info = "Object deleted but could not start Almond. Wrong config?"
            load_plugins()
            logger.info("Rendering template plugins.html")
            return render_template('plugins.html', plugins_loaded = plugins, plugins_available = list_available_plugins(), user_image=image_file, avatar=almond_avatar, info=info)
        if action_type == "updateplugin":
            logger.info("Received action type 'update plugin'")
            line_id = request.form['line']
            plugin_text = request.form['plugin']
            update_plugin_object(line_id, plugin_text)
            load_plugins()
            logger.info("Rendering template plugins.html")
            return render_template('plugins.html', plugins_loaded = plugins, plugins_available = list_available_plugins(), user_image=image_file, avatar=almond_avatar, info="Object updated")
        if (action_type == "addplugin"):
            logger.info("Received action type 'add plugin'")
            plugin_active = True
            description = request.form["description"]
            plugin = request.form["plugin_name"]
            test_this = request.form["test_plugin"]
            arguments = request.form["arguments"]
            interval = request.form["intervall"]
            if 'active' in request.form:
                plugin_active = True
            else:
                plugin_active = False
            if (test_this == 'True'):
                plugin_cmd = plugins_directory + '/' + plugin
                plugin_args = arguments.strip()
                runcmd = plugin_cmd + " " + plugin_args
                try:
                    out = subprocess.check_output([runcmd], shell=True, stderr=subprocess.STDOUT)
                except subprocess.CalledProcessError as e:
                    #raise RuntimeError("command '{}' return with error (code {}): {}".format(e.cmd, e.returncode, e.output))
                    out = e.output
                    logger.warning("CalledProcessError: " + out.decode('utf-8'))
                logger.info("Rendering template testplugin.html")
                return render_template('testplugin.html', description=description.strip(), output=out.decode('UTF-8').strip(), plugin=plugin.strip(), args=plugin_args.strip(), active=plugin_active, interval=interval, user_image=image_file, avatar=almond_avatar)
            else:
                add_plugin_object(description.strip(), plugin.strip(), arguments.strip(), interval, plugin_active)
                logger.info("Rendering template pluginadded.html")
                return render_template('pluginadded.html', description=description, user_image=image_file, avatar=almond_avatar)
        if (action_type == "install"):
            logger.info("Received action type 'install'")
            logger.info("Rendering template installplugin.html")
            return render_template('installplugin.html', user_image=image_file, avatar=almond_avatar) 
        if (action_type == "add_conf"):
            logger.info("Received action type 'add conf'")
            config_name = request.form['add_conf_value']
            config_type = request.form['config_type']
            if 'add_value' in request.form:
                config_value = request.form['add_value']
                if config_value == '':
                    config_value = 'None'
            else:
                config_value = 'None'
            if config_value == 'None':
                logger.info("Rendering template add_conf.html")
                return render_template('add_conf.html', item=config_name,config=config_type, user_image=image_file, avatar=almond_avatar)
            else:
                if config_type == 'api':
                    url = "/almond/admin?page=howru&add_item=" + config_name + "&item_value=" + config_value
                else:
                    url = "/almond/admin?page=almond&add_item=" + config_name + "&item_value=" + config_value
                logger.info("Redirect url: " + url)
                return redirect(url)
        if (action_type == "addconf"):
            logger.info("Received action type 'addconf'")
            config_type = request.form['config_type']
            config_name = request.form['conf_item']
            config_value = request.form['conf_value']
            config_file = ""
            if config_type == "api":
                config_file = api_conf_file
            elif config_type == "almond":
                config_file = almod_conf_file
            if not config_file == "":
                if not (len(config_value) == 0):
                    line = config_name.strip() + "=" + config_value.strip()
                    # You need to check if entry exists
                    #file = open(config_file, "a")
                    #print (line)
                    #file.write(line)
                    write_conf = rewrite_config(config_file, line)
                    info = "Line added to configuration"
                    logger.info(info)
                else:
                    info = "Missing config value. Did not write to file."
                    logger.warning(info)
            else:
                info = "Error writing to file. Missing information."
                logger.warning(info)
            #return redirect("/almond/admin?page=howru")
            logger.info("Rendering template howruconf.html")
            return render_template('howruconf.html', conf=write_conf, user_image=image_file, avatar=almond_avatar, info=info)
                
        if (action_type == "upload_plugin"):
            logger.info("Received action type 'upload_plugin'")
            white_list = ['sh', 'csh', 'ksh', 'py', 'pl', 'rb']
            f = request.files['filename']
            upload_name = f.filename.split('.')
            if upload_name[1] in white_list:
                print ("OK")
                f.save(f.filename)
                dest = "/opt/almond/plugins/" + f.filename
                shutil.move(f.filename, dest)
                #os.chmod(dest, stat.S_IXUSR | stat.S_I)
                os.chmod(dest, 0o750)
                logger.info(f.filename + " uploaded successfully")
                return render_template('upload.html', user_image=image_file, info=f.filename + ' uploaded successfully', avatar=almond_avatar)
            else:
                logger.info(f.filename + " was not recognized as a valid script file")
                return render_template('upload.html', user_image=image_file, info=f.filename + ' was not recognized as a valid script file.', avatar=almond_avatar)
        if (action_type == "show_metrics"):
            logger.info("Received action_type 'show metrics'")
            is_metrics = False
            metric_selection = ''
            file_name = ''
            return_list = []
            if not enable_gui:
                return render_template("403.html")
            metric_selection = request.form['metric']
            if not metric_selection == '-1':
                is_metrics = True
                file_name = store_dir + '/' +  metric_selection
                if metric_selection == 'Current metrics':
                    file_name = store_dir + '/' + metrics_file_name
                with open(file_name) as f:
                    return_list = f.readlines()
                    if not metric_selection == 'Current metrics':
                        return_list.reverse()
                    f.close()
            if is_metrics:
                logger.info("Rendering template show_metrics_a.html")
                return render_template("show_metrics_a.html", file=metric_selection, user_image=image_file, avatar=almond_avatar,  b_lines=return_list)
            else:
                logger.info("Rendering template show_metrics.html")
                return render_template("show_metrics.html", user_image=image_file, avatar=almond_avatar)
        if (action_type == "show_log"):
            logger.info("Received action_type 'show_log'")
            if not enable_gui:
                return render_template("403.html")
            log_selection = request.form['logfile']
            if not log_selection == -1:
                file_name = '/var/log/almond/' + log_selection
                try:
                    limit = int(request.form['limit'])
                except ValueError:
                    limit = 100
                except KeyError:
                    limit = 100
                with open(file_name, 'r') as file:
                    lines = deque(file, maxlen=limit)
                    file.close()
                log = list(lines)
                log.reverse()
                logger.info("Rendering template show_log_a.html")
                return render_template("show_log_a.html", log_name=log_selection, log=log, logo_image=image_file, avatar=almond_avatar)
            else:
                return render_template("show_log_a.html", log_name="None", log="No file selected", logo_image=image_file, avatar=almond_avatar)

        if (action_type == "actionapi"):
            logger.info("Received action_type 'actionapi'")
            action_id = int(request.form["action_id"])
            action_str = "{\"action\":"
            flags_str = "\"flags\":\""
            if (action_id == 1 or action_id == 2 or action_id == 3):
                name = request.form["name"]
                flags = request.form["flags"]
                if (flags == "1"):
                    flags_str += "verbose\""
                elif (flags == "3"):
                    flags_str += "all\""
                else:
                    flags_str += "none\""
                if (action_id == 1):
                    action_str += "\"read\""
                elif (action_id == 2):
                    action_str += "\"run\""
                else:
                    action_str += "\"runread\""
                action_str += ", \"id\":\"" + name + "\", " + flags_str
                if (action_id > 1):
                    token = request.form["token"]
                    action_str += ", \"token\":\"" + token + "\"}"
                else:
                    action_str += "}"
            elif (action_id == 4):
                action_str = "{\"action\":\"metrics\", \"name\":\"get_metrics\"}"
            elif (action_id == 5):
                variable = request.form["variable"]
                action_str += "\"getvar\", \"name\":\"" + variable + "\"}"
            elif (action_id == 6 or action_id == 7):
                if (action_id == 6):
                    action_str += "\"enable\""
                else:
                    action_str += "\"disable\""
                feature = request.form["function"]
                token = request.form["token"]
                action_str += ", \"name\":\"" + feature + "\", \"token\":\"" + token + "\"}"
            elif (action_id == 8):
                variable = request.form["variable"]
                value = request.form["value"]
                token = request.form["token"]
                action_str += "\"setvar\", \"name\":\"" + variable + "\", \"value\":\"" + value + "\", \"token\":\"" + token + "\"}"
            elif (action_id == 10):
                flags = "all"
                action_str += "\"read\", \"name\":\"check_all\", \"flags\":\"" + flags + "\"}"
            else:
                print ("Action id error")
            if (almond_api):
                try:
                    clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                except socket.error as e:
                    print ("Error creating socket: %s" % e)
                    retVal = "\"connection_error\" : \"Error creating socket \"}"
                    return render_template('actionapi.html', user_image=image_file, data=retVal, errors=1, avatar=almond_avatar)
                try:
                    clientSocket.connect(("127.0.0.1",almond_port))
                except socket.gaierror as e:
                    print ("Address-related error connecting to server: %s" % e)
                    retVal = "\"connection_error\" : \"Address-related error connecting to server.\"}"
                    return render_template('actionapi.html', user_image=image_file, data=retVal, errors=1, avatar=almond_avatar)
                except socket.error as e:
                    print ("Connection error: %s" % e)
                    retVal = "\"connection_error\" : \"Socket connection error.\"}"
                    return render_template('actionapi.html', user_image=image_file, data=retVal, errors=1, avatar=almond_avatar)
                try:
                    clientSocket.send(action_str.encode())
                except socket.error as e:
                    print ("Error sending data: %s" % e)
                    retVal = "\"connection_error\" : \"Error sending data.\"}"
                    return render_template('actionapi.html', user_image=image_file, data=retVal, errors=1, avatar=almond_avatar)
                try:
                    retVal = clientSocket.recv(8000)
                except socket.error as e:
                    print ("Error receiving data: %s" % e)
                    retVal = "\"connection_error\" : \"Error receiving data.\"}"
                    return render_template('actionapi.html', user_image=image_file, data=retVal, errors=1, avatar=almond_avatar)
                if not len(retVal):
                    print ("No retVal len\n")
                    retVal = "\connection_error\" : \"Empty return on socket.\"}"
                    return render_template('actionapi.html', user_image=image_file, data=retVal, errors=1, avatar=almond_avatar)
                #print(retVal.decode())
            else:
                printf("Almond api is not enabled.")
                retVal = "{\"almond_message\":\"Almond API is not enabled.\"}"
                return render_template('actionapi.html', user_image=image_file, data=retVal, errors=1, avatar=almond_avatar)
            data = retVal.decode("utf-8").strip()
            pos = data.find('Content-Length:')
            text = data[pos+16:]
            newline = text.find("\n")
            newtext = text[newline+1:].strip()
            return render_template('actionapi.html', user_image=image_file, data=newtext, errors=0, avatar=almond_avatar)

    if not ('page' in request.args):
        print("Session")
        logger.info("Checking session page")
        if 'login' in session:
            session['login'] = 'true'
            session['user'] = session['user']
            howru_state = check_service_state("howru")
            almond_state = check_service_state("almond")
            data = load_status_data()
            info_data = get_status(data)
            info = get_infostr(info_data)
            logger.info("Rendering template admin.html")
            return render_template('admin.html', version=current_version, logo_image=image_file, username=username, password=password, avatar=almond_avatar, almond_state=almond_state, howru_state=howru_state, status=info)
            #return render_template('status_admin.html', version=current_version, user_image=image_file, server=hostname, monitoring=monitoring, avatar=almond_avatar, info=info)
        else:
            a_auth_type = current_app.config['AUTH_TYPE']
            print("DEBUG: a_auth_type = ", a_auth_type)
            if (a_auth_type == "2fa"):
                #logger.info("Rendering template login_fa.html")
                #return render_template('login_fa.html', logon_image=logon_img)
                return redirect('/almond/admin/login')
            elif (a_auth_type == "basic"):
                logger.info("Rendering template login_a.html")
                return render_template('login_a.html', logon_image=logon_img)
            else:
                logger.warning("Could not get auth_type. Returning to basic")
                return render_template('login_a.html', logon_image=logon_img)
    else:
        logger.info("Checking if session is alive")
        page = request.args.get('page')
        if 'login' in session:
            session['login'] = 'true'
            session['user'] = session['user']
        else:
            logger.info("No login in session. Rendering template login_a.html")
            return render_template('login_a.html', logon_image=logon_img) 
    # page = request.args.get('page')
    if page == 'login':
        almond_img = '/static/almond.png'
        logger.info("Rendering template login_a.html")
        return render_template('login_a.html', logon_image=almond_img)
    if page == 'plugins':
        available_plugins = list_available_plugins()
        load_plugins()
        if standalone:
            info = ""
        else:
            info = "The API is running in multimode, but Plugins is only shown for the server where the API is running."
            logger.info(info)
        logger.info("Rendering template plugins.html")
        return render_template('plugins.html', plugins_loaded = plugins, plugins_available = available_plugins, user_image=image_file, avatar=almond_avatar, info=info)
    elif page == 'almond':
        load_scheduler_conf()
        if standalone:
            info = ""
        else:
            info = "Note! API is in multimode, but this configuration is for the local server only."
            logger.info(info)
        item_names = []
        item_values = []
        for item in scheduler_conf:
            pos = item.find('=')
            item_names.append(item[:pos])
            item_values.append(item[pos+1:])
        available_conf = compare_lists(scheduler_available_conf, item_names)
        if not available_conf:
            available_conf.append('None')
        add_item = request.args.get('add_item')
        add_item_value = request.args.get('item_value')
        if not add_item == None:
            item_names.append(add_item.strip())
            item_values.append(add_item_value.strip())
            available_conf.remove(add_item.strip())
            if len(available_conf) == 0:
                available_conf.append('None')
        logger.info("Rendering template conf_a.html")
        return render_template('conf_a.html', item_names=item_names, item_values=item_values,conf=scheduler_conf, aconf=available_conf, avatar=almond_avatar, info=info, user_image=image_file)   
    elif page == 'howru':
        load_api_conf()
        if standalone:
            info = ""
        else:
            info = "Note! API is in multimode, but this configuration is only applied to the local server."
            logger.info(info)
        item_names = []
        item_values = []
        for item in api_conf:
            pos = item.find('=')
            item_names.append(item[:pos])
            item_values.append(item[pos+1:])
        available_conf = compare_lists(api_available_conf, item_names)
        if not available_conf:
            available_conf.append('None')
        add_names = []
        add_values = []
        if extra_conf:
            for item in extra_conf:
                pos = item.find('=')
                add_names.append(item[:pos])
                add_values.append(item[pos+1:])
        available_conf = compare_lists(available_conf, add_names)
        add_item = request.args.get('add_item')
        add_item_value = request.args.get('item_value')
        if not add_item == None:
            item_names.append(add_item.strip())
            item_values.append(add_item_value.strip())
            available_conf.remove(add_item.strip())
        logger.info("Rendering template howruconf_a.html")
        return render_template('howruconf_a.html', item_names=item_names, item_values=item_values, add_names=add_names, add_values=add_values, conf = api_conf, aconf=available_conf, user_image=image_file, avatar=almond_avatar, info=info)
    elif page == 'status':
        set_graph_names()
        this_data = load_status_data()
        image_name = '/static/almond_small.png'
        hostname = this_data['host']['name']
        monitoring = this_data['monitoring']
        info = ''
        if not standalone:
            info = "The API is running in multimode but status will only show info for single node"
            logger.info(info)
        logger.info("Rendering template status_admin.html")
        return render_template('status_admin.html', version=current_version, user_image=image_file, server=hostname, monitoring=monitoring, avatar=almond_avatar, info=info)
    elif page == 'api':
        load_plugins()
        item_names = []
        for x in plugins:
            pos = x.find(';')
            item = x[0:pos]
            pos = item.find(' ');
            item_name = item[pos+1:]
            item_names.append(item_name.strip())
        item_names.pop(0)
        amount = len(item_names)
        logger.info("Rendering template api.html")
        return render_template('api.html', logo_image=image_file, avatar=almond_avatar, amount=amount, plugins=item_names)
    elif page == 'action':
        load_plugins()
        item_names = []
        for x in plugins:
            pos = x.find(';')
            item = x[0:pos]
            pos = item.find(' ');
            item_name = item[pos+1:]
            item_names.append(item_name.strip())
        item_names.pop(0)
        action = request.args.get("aid")
        logger.info("Rendering template action.html")
        return render_template('action.html', logo_image=image_file, avatar=almond_avatar, plugins=item_names, action=action)
    elif page == 'docs':
        logger.info("Rendering template documentation_a.html")
        return render_template('documentation_a.html', user_image=image_file, avatar=almond_avatar) 
    elif page == 'logs':
        logs_list = get_logs('/var/log/almond/')
        logger.info("Rendering template logs.html")
        return render_template('logs.html', logo_image=image_file, avatar=almond_avatar, logfiles=logs_list)
    elif page == 'metrics':
        metrics_list = []
        for f in os.listdir(store_dir):
            if (f == metrics_file_name):
                f = "Current metrics"
            metrics_list.append(f)
        metrics_list.sort()
        if not standalone:
            info = "Note! API is running in multimode, but Metrics will only be shown for the local server."
            logger.info(info)
        else:
            info = ""
        logger.info("Rendering template metrics_a.html")
        return render_template('metrics_a.html', user_image=image_file, avatar=almond_avatar, metrics_list=metrics_list, info=info)
    elif page == 'graph':
        logger.info("Rendering new graph")
        key_list = []
        key_vals = []
        graph_name = request.args.get("name")
        plot_dates, plot_data, uptime = get_graph_data(graph_name)
        for data in plot_data:
            for key, value in data.items():
                key_list.append(key)
                key_vals.append(float(value))
        while (len(key_vals) > 40):
            div_num = len(key_vals) / 40;
            if (div_num <= 2):
                del key_vals[::2]
                del plot_dates[::2]
            else:
                #aggregate on div_num
                key_len = len(key_vals) -1
                agg_keys = []
                agg_dates = []
                while (key_len > 0):
                    x = key_len - int(div_num)
                    agg_part = key_vals[x:key_len]
                    date_part = plot_dates[x:key_len]
                    date_middle_index = int((len(date_part) -1)/2)
                    if (date_middle_index > 0):
                        strip_date = date_part[date_middle_index][4:]
                        strip_date = strip_date[:len(strip_date)-8]
                        agg_dates.append(strip_date)
                        agg_sum = sum(agg_part) / len(agg_part)
                        agg_keys.append(round(agg_sum, 3))
                    key_len = x
                agg_keys.reverse()
                agg_dates.reverse()
                key_vals = agg_keys
                plot_dates = agg_dates
        cur_dir = os.getcwd()
        if not cur_dir == '/opt/almond/www/api':
            os.chdir('/opt/almond/www/api')
        while graph_written < 2:
            plt.plot(plot_dates, key_vals, color="white")
            plt.rcParams['axes.facecolor'] = '#491c0f'
            plt.gcf().autofmt_xdate()
            plt.title(graph_name)
            plt.xlabel('Timestamp')
            plt.ylabel(key_list[0])
            plt.xticks(fontsize=6, rotation=90, ha='right')
            graph_file_name = graph_name
            graph_file_name.replace(" ", "")
            graph_file_name.replace("/", "_")
            save_name = 'static/charts/' + graph_file_name + '.png'
            save_name = 'static/charts/graph.png'
            plt.savefig(save_name)
            plt.clf()
            graph_written += 1
        save_name = '/' + save_name
        cur_dir = os.getcwd()
        if not (os.getcwd() == cur_dir):
            os.chdir(cur_dir)
        graph_written = 1
        logger.info("Rendering graph.html")
        return render_template("graph.html", user_image = image_file, name="Trend chart", url=save_name, uptime=str(uptime))
    elif page == 'logout':
        almond_img = '/static/almond.png'
        a_auth_type = current_app.config['AUTH_TYPE']
        logger.info("User " + session['user'] + " logged out.")
        session.pop('login', None)
        session.pop('user', None)
        if (a_auth_type == "2fa"):
            session.pop('authenticated', None)
            logger.info("Rendering template login_fa.html")
            return render_template('login_fa.html', logon_image=almond_img)
        else:
            logger.info("Rendering template login_a.html")
            return render_template('login_a.html', logon_image=almond_img)
    else:
        return page
