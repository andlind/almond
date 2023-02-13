import subprocess
import json
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

auth = HTTPBasicAuth()

def load_plugins():
    global plugins
    f = open("/etc/almond/plugins.conf")
    read_data = f.read()
    plugins = read_data.split("\n")
    plugins.pop()
    f.close()

def load_conf():
    global conf
    f = open("/etc/almond/almond.conf")
    read_data = f.read()
    conf = read_data.split("\n")
    conf.pop()
    f.close()

def load_scheduler_conf():
    global conf
    global scheduler_conf
    load_conf()
    scheduler_conf = [x for x in conf if not 'api' in x]

def load_api_conf():
    global conf
    global api_conf

    prefixes = ('scheduler.', 'gardener.', 'data.', 'plugins.')
    load_conf()
    this_conf = conf.copy()
    api_conf = [x for x in this_conf if not x.startswith(prefixes)]

def list_available_plugins():
    plugin_list = next(walk("/usr/local/nagios/libexec"), (None, None, []))[2]
    plugin_list.remove('utils.sh')
    plugin_list.remove('utils.pm')
    return plugin_list

def load_status_data():
    if os.path.isfile("/opt/almond/data/mon1_data.json"):
        f = open("/opt/almond/data/mon1_data.json", "r")
        data = json.loads(f.read())
        f.close()
    else:
        data = {
            "host": {
                "name":"tmauv03.test.svenskaspel.se"
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
    with open("/etc/almond/plugins.conf", "r+") as fp:
        lines = fp.readlines()
        del lines[int(id)+1]
        fp.seek(0)
        fp.truncate()
        fp.writelines(lines)

def update_plugin_object(id, t):
    with open("/etc/almond/plugins.conf", "r+") as fp:
        lines = fp.readlines()
        new_val = t + '\n'
        lines[int(id)+1] = new_val
        fp.seek(0)
        fp.truncate()
        fp.writelines(lines)

def add_plugin_object(description, plugin, arguments, interval, active):
    write_active = "0"
    if (active):
        write_active = "1"
    # Check description
    write_str = description + ";" + plugin + " " + arguments + ";" + write_active + ";" + interval +  "\n"
    print (write_str)
    f = open("/etc/almond/plugins.conf", "a")
    f.write(write_str)
    f.close()

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
    #print (users.get(username))
    #print(users)
    #with open("/etc/almond/users.conf", "w") as f:
    #    f.write(json.dumps(users))
    if os.path.isfile("/etc/almond/users.conf"):
        #headers = {"Content-Type": "application/text"}
        #return make_response("Invalid userfile", 404, headers);
        users = json.load(open("/etc/almond/users.conf"))
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

    username = ''
    password = ''
    image_file = '/static/almond_small.png'
    almond_avatar = '/static/almond_avatar.png'

    if not os.path.isfile("/etc/almond/users.conf"):
        headers = {"Content-Type": "application/text"}
        print ("Invalid userfile")
        return make_response("Invalid userfile", 404, headers);

    users = json.load(open("/etc/almond/users.conf"))
    username = users.get(username)
    password = ""
    if ('action_type' in request.form):
        action_type = request.form['action_type']
        if action_type == 'change_credentials':
            info = ''
            username = request.form['username']
            password = request.form['password']
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
                with open("/etc/almond/users.conf", "w") as f:
                    f.write(json.dumps(users))
                info = "Credentials updated."
            else:
                info = "Error updating credentials"
            return render_template('admin.html', info = info, username=username, passwd=password, logo_image=image_file, avatar=almond_avatar)
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
            with open("/etc/almond/almond.conf", "w") as fp:
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
            return render_template('admin.html', info=info, logo_image=image_file, avatar=almond_avatar)
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
            return_code = subprocess.call(["/bin/systemctl", "restart", "almond.service"])
            info = ""
            if (return_code == 0):
                info = "Object deleted and process Almond restarted"
            else:
                info = "Object deleted but could not start Almond. Wrong config?"
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
            print (plugin)
            print (test_this)
            print (arguments)
            print (interval)
            print (plugin_active)
            if (test_this == 'True'):
                plugin_cmd = "/usr/local/nagios/libexec/" + plugin
                plugin_args = arguments.strip()
                runcmd = plugin_cmd + " " + plugin_args
                print (runcmd)
                #out = check_output([plugin_cmd],[plugin_args])
                #out = check_output(["/usr/local/nagios/libexec/check_dummy"])
                try:
                    #out = subprocess.check_output([plugin_cmd, plugin_args],shell=True,stderr=subprocess.STDOUT)
                    out = subprocess.check_output([runcmd], shell=True, stderr=subprocess.STDOUT)
                    #out =  subprocess.check_output(["/usr/local/nagios/libexec/check_uptime2.sh 10 20"],shell=True,stderr=subprocess.STDOUT)
                except subprocess.CalledProcessError as e:
                    #raise RuntimeError("command '{}' return with error (code {}): {}".format(e.cmd, e.returncode, e.output))
                    out = e.output
                return render_template('testplugin.html', description=description.strip(), output=out.decode('UTF-8').strip(), plugin=plugin.strip(), args=plugin_args.strip(), active=plugin_active, interval=interval, user_image=image_file, avatar=almond_avatar)
            else:
                add_plugin_object(description.strip(), plugin.strip(), arguments.strip(), interval, plugin_active)
                return render_template('pluginadded.html', description=description, user_image=image_file, avatar=almond_avatar)
    if not ('page' in request.args):
        return render_template('admin.html', logo_image=image_file, username=username, password=password, avatar=almond_avatar)
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
    elif page == 'docs':
        return render_template('documentation.html', user_image=image_file) 
    else:
        return page
