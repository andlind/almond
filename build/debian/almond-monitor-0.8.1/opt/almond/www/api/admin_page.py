import subprocess
import json
import shutil
import re
import os.path
import os
import socket
from os import walk
from flask import Blueprint
from flask_httpauth import HTTPBasicAuth
from flask import render_template, session, request, make_response, redirect
import matplotlib.pyplot as plt
from werkzeug.security import check_password_hash, generate_password_hash

admin_page = Blueprint('admin_page', __name__, template_folder='templates')

graph_written = 0
plugins = []
conf = []
api_conf = []
scheduler_conf = []
extra_conf = []
graph_names = {}
api_available_conf = ['api.bindPort', 'api.enableFile',' data.jsonFile', 'data.metricsFile', 'api.enableAliases', 'api.enableFile', 'api.enableScraper', 'api.dataDir', 'api.useSSL', 'api.sslCertificate', 'api.sslKey', 'api.startPage', 'api.useGUI', 'api.adminUser', 'api.adminPassword', 'api.userFile', 'api.stateType', 'api.multiMetrics', 'api.multiServer', 'scheduler.storeDir', 'scheduler.configFile', 'scheduler.dataDir', 'plugins.directory', 'plugins.declaration']
scheduler_available_conf = ['almond.api', 'almond.port', 'almond.standalone', 'data.jsonFile', 'data.saveOnExit', 'data.metricsFile', 'data.metricsOutputPrefix', 'plugins.directory', 'plugins.declaration', 'scheduler.confDir', 'scheduler.logDir', 'scheduler.logToStdout', 'scheduler.logPluginOutput', 'scheduler.storeResults', 'scheduler.format', 'scheduler.initSleepMs', 'scheduler.sleepMs', 'scheduler.tuneTimer', 'scheduler.tunerCycle', 'scheduler.tuneMaster', 'scheduler.dataDir', 'scheduler.storeDir', 'scheduler.hostName', 'scheduler.enableGardener', 'scheduler.gardenerScript', 'scheduler.gardenerRunInterval', 'scheduler.quickStart', 'scheduler.metricsOutputPrefix', 'scheduler.enableClearDataCache', 'scheduler.enableKafkaExport', 'scheduler.enableKafkaTag', 'scheduler.kafkaBrokers', 'scheduler.kafkaTopic', 'scheduler.kafkaTag', 'scheduler.enableKafkaSSL', 'scheduler.kafkaCACertificate', 'scheduler.kafkaProducerCertificate', 'scheduler.kafkaSSLKey', 'scheduler.clearDataCacheInterval', 'scheduler.dataCacheTimeFrame', 'gardener.CleanUpTime']

users = {}
current_version = '0.8.1'

enable_gui = True
standalone = True
almond_api = False
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

    update_credentials = True
    if len(username.strip()) < 4:
        # Username should contain at least 4 characters
        update_credentials = False
    if len(password.strip()) < 6:
        # Password should contain at least 6 characters
        update_credentials = False
    users = {
        username: generate_password_hash(password.strip())
    }
    if update_credentials:
        with open(admin_user_file, "w") as f:
            f.write(json.dumps(users))
        info = "Credentials updated."
    else:
        info = "Error updating credentials"
    print ("New password credentials set")
    return info

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

    totalsent = 0

    if (almond_api):
        try:
            clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        except socket.error as e:
            print ("Error creating socket: %s" % e)
            return 2;
        try:
            clientSocket.connect(("127.0.0.1",almond_port))
        except socket.gaierror as e:
            print ("Address-related error connecting to server: %s" % e)
            return 1;
        except socket.error as e:
            print ("Connection error: %s" % e)
            return 2;
        data = '{"action":"execute", "id":"' + str(id) + '"}'
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
                l_output = l_data[0].split(",")[2]
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
def verify_password(username, password):
    global admin_user_file
    if os.path.isfile(admin_user_file):
        users = json.load(open(admin_user_file))
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
        return make_response("Invalid userfile", 404, headers);

    users = json.load(open(admin_user_file))
    username = users.get(username)
    password = ""
    if ('action_type' in request.form):
        if 'login' in session:
            session['login'] = 'true'
        else:
            if (request.form['action_type'] == "create_session"):
                print ("Login")
            else:
                return render_template('login_a.html', logon_image=logon_img)
        action_type = request.form['action_type']
        if action_type == "create_session":
            username = request.form['uname']
            password = request.form['psw']
            if verify_password(username.strip(), password.strip()):
                session['login'] = 'true'
        if action_type == 'change_credentials':
            info = ''
            username = request.form['username']
            password = request.form['password']
            set_new_password(username.strip(), password.strip())
            howru_state = check_service_state("howru")
            almond_state = check_service_state("almond")
            return render_template('admin.html', info = info, version=current_version, username=username, passwd=password, logo_image=image_file, avatar=almond_avatar, almond_state=almond_state, howru_state=howru_state)
        if action_type == 'plugins':
            if 'delete_line' in request.form:
                line_id = request.form['delete_line']
                return render_template('deleteplugin.html', user_image=image_file, line=line_id, avatar=almond_avatar)
            elif 'edit_line' in request.form:
                line_id = request.form['edit_line']
                plugin_text = request.form['edit_text']
                return render_template('editplugin.html', user_image=image_file, line=line_id, text=plugin_text.strip(), avatar=almond_avatar)
            elif 'add_line' in request.form:
                plugin_name = request.form['installed_plugins']
                return render_template('addplugin.html', user_image=image_file, plugin=plugin_name, avatar=almond_avatar)
            elif 'execute' in request.form:
                plugin_id = request.form['plugin_id']
                command = request.form['execute']
                return render_template('execute.html', user_image=image_file, plugin_id=plugin_id, execute=command, avatar=almond_avatar)
            else:
                print ("None")
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
            return render_template('conf.html', conf = write_conf, info="Config was rewritten", user_image=image_file, avatar=almond_avatar)
        if action_type == 'restart_almond':
            if state_type == "systemctl":
                return_code = subprocess.call(["/bin/systemctl", "restart", "almond.service"])
            elif state_type == "supervisorctl":
                return_code = subprocess.call(["/usr/bin/supervisorctl", "restart", "almond"])
            else:
                return_code = 3
            info = ""
            if (return_code == 0):
                info = "Process Almond restarted"
            else:
                info = "Could not start Almond. Wrong config?"
            howru_state = check_service_state("howru")
            almond_state = check_service_state("almond")
            return render_template('admin.html', version=current_version, info=info, logo_image=image_file, avatar=almond_avatar, almond_state=almond_state, howru_state=howru_state)
        if action_type == 'restart_scheduler':
            if state_type == "systemctl":
                return_code = subprocess.call(["/bin/systemctl", "restart", "almond.service"])
            elif state_type == "supervisorctl":
                return_code = subprocess.call(["/usr/bin/supervisorctl", "restart", "almond"])
            else:
                return_code = 3
            info = ""
            if (return_code == 0):
                info = "Process Almond restarted"
            else:
                info = "Could not start Almond. Wrong config?"
            return render_template('conf.html', conf = scheduler_conf, info=info, user_image=image_file, avatar=almond_avatar)
        if action_type == "execute_plugin":
            info = ""
            pid = request.form['plugin_id']
            exr = execute_plugin_object(pid)
            if (exr == 0):
                info = "Plugin execution sent to Almond."
            elif (exr == 1):
                info = "Could not execute. Configuration is not correct."
            elif (exr == 2):
                info = "Could not execute. Socket error."
            else:
                info = "Something went wrong?"
            return render_template('plugins.html', plugins_loaded = plugins, plugins_available = list_available_plugins(), user_image=image_file, avatar=almond_avatar, info=info)
        if action_type == "api":
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
            else:
                info = "Config rewritten. Note! API is running in multimode, but this config will only apply to the local server."
            if move_value:
                form_keys = []
                form_vals = []
                for x in update_lines:
                    item = x.split('=')
                    form_keys.append(item[0])
                    form_vals.append(item[1])
                return render_template('confirm_move.html', keys=form_keys, vals=form_vals, user_image=image_file, avatar=almond_avatar)
            if update_lines:
                write_conf = rewrite_config(api_conf_file, update_lines)
            else:
                write_conf = api_conf.copy()
            return render_template('howruconf.html', conf=write_conf, user_image=image_file, avatar=almond_avatar, info=info)
        if action_type == "restart_api":
            #return "You need to run systemctl restart howru-api.service"
            restart_api() 
            return render_template('restart.html', user_image=image_file)
        if action_type == "deleteplugin":
            line_id = request.form['line']
            delete_plugin_object(line_id)
            #return_code = subprocess.call(["/bin/systemctl", "restart", "almond.service"])
            info = "Object deleted. Almond process reloads."
            #if (return_code == 0):
            #    info = "Object deleted and process Almond restarted"
            #else:
            #    info = "Object deleted but could not start Almond. Wrong config?"
            load_plugins()
            return render_template('plugins.html', plugins_loaded = plugins, plugins_available = list_available_plugins(), user_image=image_file, avatar=almond_avatar, info=info)
        if action_type == "updateplugin":
            line_id = request.form['line']
            plugin_text = request.form['plugin']
            update_plugin_object(line_id, plugin_text)
            load_plugins()
            return render_template('plugins.html', plugins_loaded = plugins, plugins_available = list_available_plugins(), user_image=image_file, avatar=almond_avatar, info="Object updated")
        if (action_type == "addplugin"):
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
                return render_template('testplugin.html', description=description.strip(), output=out.decode('UTF-8').strip(), plugin=plugin.strip(), args=plugin_args.strip(), active=plugin_active, interval=interval, user_image=image_file, avatar=almond_avatar)
            else:
                add_plugin_object(description.strip(), plugin.strip(), arguments.strip(), interval, plugin_active)
                return render_template('pluginadded.html', description=description, user_image=image_file, avatar=almond_avatar)
        if (action_type == "install"):
            return render_template('installplugin.html', user_image=image_file, avatar=almond_avatar) 
        if (action_type == "add_conf"):
            config_name = request.form['add_conf_value']
            config_type = request.form['config_type']
            if 'add_value' in request.form:
                config_value = request.form['add_value']
                if config_value == '':
                    config_value = 'None'
            else:
                config_value = 'None'
            if config_value == 'None':
                return render_template('add_conf.html', item=config_name,config=config_type, user_image=image_file, avatar=almond_avatar)
            else:
                if config_type == 'api':
                    url = "/almond/admin?page=howru&add_item=" + config_name + "&item_value=" + config_value
                else:
                    url = "/almond/admin?page=almond&add_item=" + config_name + "&item_value=" + config_value
                return redirect(url)
        if (action_type == "addconf"):
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
                else:
                    info = "Missing config value. Did not write to file."
            else:
                info = "Error writing to file. Missing information."
            #return redirect("/almond/admin?page=howru")
            return render_template('howruconf.html', conf=write_conf, user_image=image_file, avatar=almond_avatar, info=info)
                
        if (action_type == "upload_plugin"):
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
                return render_template('upload.html', user_image=image_file, info=f.filename + ' uploaded successfully', avatar=almond_avatar)
            else:
                return render_template('upload.html', user_image=image_file, info=f.filename + ' was not recognized as a valid script file.', avatar=almond_avatar)
        if (action_type == "show_metrics"):
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
                return render_template("show_metrics_a.html", file=metric_selection, user_image=image_file, avatar=almond_avatar,  b_lines=return_list)
            else:
                return render_template("show_metrics.html", user_image=image_file, avatar=almond_avatar)
    if not ('page' in request.args):
        print("Session")
        if 'login' in session:
            session['login'] = 'true'
            howru_state = check_service_state("howru")
            almond_state = check_service_state("almond")
            return render_template('admin.html', version=current_version, logo_image=image_file, username=username, password=password, avatar=almond_avatar, almond_state=almond_state, howru_state=howru_state)
        else:
            return render_template('login_a.html', logon_image=logon_img)
    else:
        page = request.args.get('page')
        if 'login' in session:
            session['login'] = 'true'
        else:
            return render_template('login_a.html', logon_image=logon_img) 
    # page = request.args.get('page')
    if page == 'login':
        almond_img = '/static/almond.png'
        return render_template('login_a.html', logon_image=almond_img)
    if page == 'plugins':
        available_plugins = list_available_plugins()
        load_plugins()
        if standalone:
            info = ""
        else:
            info = "The API is running in multimode, but Plugins is only shown for the server where the API is running."
        return render_template('plugins.html', plugins_loaded = plugins, plugins_available = available_plugins, user_image=image_file, avatar=almond_avatar, info=info)
    elif page == 'almond':
        load_scheduler_conf()
        if standalone:
            info = ""
        else:
            info = "Note! API is in multimode, but this configuration is for the local server only."
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
        return render_template('conf_a.html', item_names=item_names, item_values=item_values,conf=scheduler_conf, aconf=available_conf, avatar=almond_avatar, info=info, user_image=image_file)   
    elif page == 'howru':
        load_api_conf()
        if standalone:
            info = ""
        else:
            info = "Note! API is in multimode, but this configuration is only applied to the local server."
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
        return render_template('api.html', logo_image=image_file, avatar=almond_avatar, amount=amount, plugins=item_names)
    elif page == 'docs':
        return render_template('documentation_a.html', user_image=image_file, avatar=almond_avatar) 
    elif page == 'metrics':
        metrics_list = []
        for f in os.listdir(store_dir):
            if (f == metrics_file_name):
                f = "Current metrics"
            metrics_list.append(f)
        metrics_list.sort()
        if not standalone:
            info = "Note! API is running in multimode, but Metrics will only be shown for the local server."
        else:
            info = ""
        return render_template('metrics_a.html', user_image=image_file, avatar=almond_avatar, metrics_list=metrics_list, info=info)
    elif page == 'graph':
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
        return render_template("graph.html", user_image = image_file, name="Trend chart", url=save_name, uptime=str(uptime))
    elif page == 'logout':
        almond_img = '/static/almond.png'
        session.pop('login', None)
        return render_template('login_a.html', logon_image=almond_img)
    else:
        return page
