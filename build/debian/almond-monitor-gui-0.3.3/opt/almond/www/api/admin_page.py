import subprocess
import json
import shutil
import os.path
from os import walk
from flask import Blueprint
from flask_httpauth import HTTPBasicAuth
from flask import render_template, request, make_response
from werkzeug.security import check_password_hash, generate_password_hash

admin_page = Blueprint('admin_page', __name__, template_folder='templates')

plugins = []
conf = []
api_conf = []
scheduler_conf = []
users = {}

enable_gui = True
standalone = True
jasonFile = '/opt/almond/data/monitor.json'
storeDir = '/opt/almond/data/metrics'
plugins_directory = '/opt/almond/plugins'
declaration_file = '/etc/almond/plugins.conf'
admin_user_file = '/etc/almond/users.conf'
almond_conf_file = '/etc/almond/almond.conf'
start_page = 'admin'
state_type='systemctl'

auth = HTTPBasicAuth()

def load_plugins():
    global plugins
    global declaration_file

    f = open(declaration_file)
    read_data = f.read()
    plugins = read_data.split("\n")
    plugins.pop()
    f.close()

def load_conf(isGlobal):
    global conf
    global almond_conf_file

    if (isGlobal):
        f = open(almond_conf_file)
    else:
        if os.path.isfile('/etc/almond/admin.conf'):
            f = open("/etc/almond/admin.conf", "r")
        elif os.path.isfile('/etc/almond/api.conf'):
            f = open("/etc/almond/api.conf", "r")
        else:
            f = open("/etc/almond/almond.conf", "r")
    read_data = f.read()
    conf = read_data.split("\n")
    conf.pop()
    f.close()

def load_scheduler_conf():
    global conf
    global scheduler_conf
    load_conf(True)
    this_conf = conf.copy()
    scheduler_conf = [x for x in this_conf if not 'api' in x]

def load_api_conf():
    global conf
    global api_conf

    prefixes = ('scheduler.', 'gardener.', 'data.', 'plugins.')
    load_conf(False)
    this_conf = conf.copy()
    api_conf = [x for x in this_conf if not x.startswith(prefixes)]

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

def read_conf():
    global standalone
    global enable_gui
    global jasonFile
    global storeDir
    global plugin_directory
    global declaration_file
    global admin_user_file
    global start_page
    global state_type
    global almond_conf_file

    admin_password = ''
    admin_user = ''

    load_conf(False)
    for x in conf:
        if (x.find('data') == 0):
            if (x.find('jsonFile') > 0):
                pos = x.find('=')
                json_file = x[pos+1:].rstrip()
            else:
                json_file = 'monitor.json'
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
                print ("data_dir")
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
                store_dire = x[pos+1:].rstrip()
            if (x.find('confFile') > 0):
                pos = x.find('=')
                almond_conf_file = x[pos+1:].rstrip()
        if (x.find('plugins') == 0):
            if (x.find('directory') > 0):
                pos = x.find('=')
                plugin_directory = x[pos+1:].rstrip()
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
    plugin_list.remove('utils.sh')
    plugin_list.remove('utils.pm')
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

def add_plugin_object(description, plugin, arguments, interval, active):
    global declaration_file

    write_active = "0"
    if (active):
        write_active = "1"
    # Check description
    write_str = description + ";" + plugin + " " + arguments + ";" + write_active + ";" + interval +  "\n"
    print (write_str)
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

@auth.verify_password
def verify_password(username, password):
    global admin_user_file
    #print (users.get(username))
    #print(users)
    #with open("/etc/almond/users.conf", "w") as f:
    #    f.write(json.dumps(users))
    if os.path.isfile(admin_user_file):
        #headers = {"Content-Type": "application/text"}
        #return make_response("Invalid userfile", 404, headers);
        users = json.load(open(admin_user_file))
    else:
        users = {}
    if username in users:
        return check_password_hash(users.get(username), password)
    return False

@admin_page.route('/almond/admin', methods=['GET', 'POST'])
@auth.login_required
def index():
    global plugins
    global scheduler_conf
    global api_conf
    global users
    global enable_gui
    global standalone
    global admin_user_file
    global almond_conf_file
    
    if not enable_gui:
        return render_template("403.html")

    read_conf()
    username = ''
    password = ''
    image_file = '/static/almond_small.png'
    almond_avatar = '/static/almond_avatar.png'

    if not os.path.isfile(admin_user_file):
        headers = {"Content-Type": "application/text"}
        print ("Invalid userfile")
        return make_response("Invalid userfile", 404, headers);

    users = json.load(open(admin_user_file))
    username = users.get(username)
    password = ""
    if ('action_type' in request.form):
        action_type = request.form['action_type']
        if action_type == 'change_credentials':
            info = ''
            username = request.form['username']
            password = request.form['password']
            #update_credentials = True
            #if len(username.strip()) < 4:
            #    # Username should contain at least 4 characters
            #    update_credentials = False
            #if len(password.strip()) < 6:
            #    # Password should contain at least 6 characters
            #    update_credentials = False
            #users = {
            #        username: generate_password_hash(password.strip())
            #}
            #if update_credentials:
            #    with open(admin_user_file, "w") as f:
            #        f.write(json.dumps(users))
            #    info = "Credentials updated."
            #else:
            #    info = "Error updating credentials"
            set_new_password(username.strip(), password.strip())
            howru_state = check_service_state("howru-api")
            almond_state = check_service_state("almond")
            return render_template('admin.html', info = info, username=username, passwd=password, logo_image=image_file, avatar=almond_avatar, almond_state=almond_state, howru_state=howru_state)
        if action_type == 'plugins':
            #form_data = request.form
            #for key in form_data:
            #    print("Key is type:", type(key))
            #    print (key)
            #    print ("Data is type:", type(form_data[key]))
            #    print (form_data[key])
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
            else:
                print ("None")
            return action_type
        if action_type == 'scheduler':
            config = request.form['config']
            write_conf = config.rstrip().split('\n')
            load_api_conf()
            with open(almond_conf_file, "w") as fp:
                for item in api_conf:
                    if (len(item) > 5):
                        fp.write("%s\n" % item)
                for item in write_conf:
                    fp.write("%s\n" % item.strip('\r'))
                fp.write('\n')
            return render_template('conf.html', conf = write_conf, info="Config was rewritten", user_image=image_file, avatar=almond_avatar)
        if action_type == 'restart_almond':
            return_code = subprocess.call(["/bin/systemctl", "restart", "almond.service"])
            info = ""
            if (return_code == 0):
                info = "Process Almond restarted"
            else:
                info = "Could not start Almond. Wrong config?"
            howru_state = check_service_state("howru-api")
            almond_state = check_service_state("almond")
            return render_template('admin.html', info=info, logo_image=image_file, avatar=almond_avatar, almond_state=almond_state, howru_state=howru_state)
        if action_type == 'restart_scheduler':
            return_code = subprocess.call(["/bin/systemctl", "restart", "almond.service"])
            info = ""
            if (return_code == 0):
                info = "Process Almond restarted"
            else:
                info = "Could not start Almond. Wrong config?"
            #status = subprocess.check_output("/bin/systemctl show -p ActiveState --value almond.service")
            #print(status)
            return render_template('conf.html', conf = scheduler_conf, info=info, user_image=image_file, avatar=almond_avatar)
        if action_type == "api":
            config = request.form['config']
            write_config = config.rstrip().split('\n')
            load_scheduler_conf()
            with open("/etc/almond/almond.conf", "w") as fp:
                for item in write_config:
                    fp.write("%s\n" % item.strip('\r'))
                for item in scheduler_conf:
                    if (len(item) > 5):
                        fp.write("%s\n" % item)
            return render_template('howruconf.html', conf=write_config, info="Config was rewritten", user_image=image_file, avatar=almond_avatar)
        if action_type == "restart_api":
            return "You need to run systemctl restart howru-api.service"
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
                plugin_cmd = "/usr/local/nagios/libexec/" + plugin
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
    if not ('page' in request.args):
        howru_state = check_service_state("howru-api")
        almond_state = check_service_state("almond")
        return render_template('admin.html', logo_image=image_file, username=username, password=password, avatar=almond_avatar, almond_state=almond_state, howru_state=howru_state)
    page = request.args.get('page')
    if page == 'plugins':
        available_plugins = list_available_plugins()
        load_plugins()
        return render_template('plugins.html', plugins_loaded = plugins, plugins_available = available_plugins, user_image=image_file, avatar=almond_avatar)
    elif page == 'almond':
        load_scheduler_conf()
        return render_template('conf.html', conf = scheduler_conf, user_image=image_file, avatar=almond_avatar)
    elif page == 'howru':
        load_api_conf()
        return render_template('howruconf.html', conf = api_conf, user_image=image_file, avatar=almond_avatar)
    elif page == 'status':
        this_data = load_status_data()
        image_name = '/static/almond_small.png'
        hostname = this_data['host']['name']
        monitoring = this_data['monitoring']
        return render_template('status_admin.html', user_image=image_file, server=hostname, monitoring=monitoring, avatar=almond_avatar)
    elif page == 'api':
        return render_template('api.html', logo_image=image_file, avatar=almond_avatar)
    elif page == 'docs':
        return render_template('documentation_a.html', user_image=image_file) 
    else:
        return page
