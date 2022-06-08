import json
import flask 
from flask import request, jsonify, render_template
import os, os.path
import glob

app = flask.Flask(__name__)
app.config["DEBUG"] = True
data = None
settings = None
bindPort = None
multi_server = False
enable_file = False
server_list_loaded = 0
server_list = []
data_dir="/opt/howru/www"
file_name = ''
data_file = "/opt/howru/www/monitor_data.json"

def load_conf():
    global bindPort, multi_server, enable_file, data_file
    conf = open("/etc/howru/howruc.conf", "r")
    for line in conf:
        if (line.find('data') == 0):
            if (line.find('jsonFile') > 0):
                pos = line.find('=')
                data_file = line[pos+1:].rstrip()
        if (line.find('api') == 0):
            if (line.find('Data') > 0):
                pos = line.find('=')
                data_dir = line[pos+1:]
            if (line.find('Port') > 0):
                pos = line.find('=')
                port = line[pos+1:]
                if (isinstance(int(port), int)):
                   bindPort = int(port)
                else:
                    bindPort = 80
            if (line.find('Server') > 0):
                pos = line.find('=')
                multi_s = line[pos+1]
                if (isinstance(int(multi_s), int)):
                    if (int(multi_s) > 0):
                        multi_server = True
                    else:
                        multi_server = False
                else:
                    multi_server = False
            if (line.find('File') > 0):
                pos = line.find('=')
                bFile = line[pos+1]
                if (isinstance(int(bFile), int)):
                    if (int(bFile) > 0):
                        enable_file = True
                    else:
                        enable_file = False
                else:
                    enable_file = False
    conf.close()
    return bindPort

def load_data():
    global data, data_dir, multi_server, enable_file, file_name, data_file
    os.chdir(data_dir)
    if (enable_file == True):
        if (len(file_name) > 5):
            if (os.path.exists(file_name)):
                count = 0
                f = open(file_name, "r")
                data = json.loads(f.read())
                f.close()
                return
            else:
                print ("Could not find serverfile ", file_name)
    if (multi_server):
       #print("Running in mode multi");
       count = 0 
       data = {
                  "server": [
                      ]
              }

       for file in glob.glob("*.json"):
            f = open(file, "r")
            data_set = json.loads(f.read())
            data["server"].append(data_set)
            f.close()
            count = count + 1
    else:
       #print("Running in single mode");
       #f = open("monitor_data.json", "r")
       f = open(data_file, "r")
       data = json.loads(f.read())
       f.close()
    os.chdir("/opt/howru/www/api")

def load_settings():
    global settings 
    f = open("/etc/howru/plugins.conf", "r")
    try:
        settings = f.readlines()
    finally:
        f.close()

def load_scheduler_settings():
    global settings
    f = open("/etc/howru/howruc.conf", "r")
    try:
        settings = f.readlines()
    finally:
        f.close()

def set_file_name():
    global file_name
    json_file = request.args.get("whichjson")
    if json_file is None:
        file_name = ''
    else:
        file_name = json_file + ".json"

@app.route('/', methods=['GET'])
def home():
    global data
    global server_list
    global server_list_loaded
    full_filename = '/static/howru.png'
    if (multi_server):
        if (server_list_loaded == 0):
            set_file_name()
            load_data()
            s_data = data["server"]
            for host in s_data:
                server_name = host["host"]["name"]
                server_list.append(server_name)
            server_list_loaded = 1
        return render_template("index_m.html", len = len (server_list), server_list = server_list, user_image = full_filename)
    else:
        return render_template("index.html", user_image = full_filename)

@app.route('/docs', methods=['GET'])
def documentation():
    full_filename = '/static/howru.png'
    return render_template("documentation.html", user_image = full_filename)
 
@app.route('/pluginSearch', methods=['GET'])
def search_plugin():
    full_filename = '/static/howru.png'
    if (multi_server):
        return render_template("plugin_query_mm.html", user_image = full_filename)
    else:
        return render_template("plugin_query.html", user_image = full_filename)

@app.route('/howru/monitoring/json', methods=['GET'])
def api_json():
   global data 
   set_file_name()
   load_data()
   return jsonify(data)

@app.route('/howru/monitoring/howareyou', methods=['GET'])
def api_howareyou():
    global data, multi_server
    set_file_name()
    load_data()
    ok = 0
    warn = 0
    crit = 0
    results = []
    unknown = 0
    res = "I am not sure"

    if (multi_server):
        results = {
            "server": [
                      ]
            }
        for serv in data['server']:
           name_o = serv['host']
           name =  name_o['name']
           obj = serv['monitoring']
           for status in obj:
               if (status['pluginStatusCode'] == "0"):
                   ok = ok + 1
               elif (status['pluginStatusCode'] == "1"):
                   warn = warn + 1
               elif (status['pluginStatusCode'] == "2"):
                   crit = crit +1
               else:
                   unknown = unknown +1
           if (crit > 0):
               res = "I'm not fine!"
               ret_code = 2
           else:
               if (warn > 0):
                   res = "I'm so so"
                   ret_code = 1
               else:
                   res = "I'm "
                   if (unknown > 0):
                       res = res + " not completly sure to be honest."
                       ret_code = 3
                   else:
                       res = res + " fine, thanks for asking!"
                       ret_code = 0

           server = [
                {  'name' : name,
                   'answer' : res,
                   'return_code' : ret_code,
                   'monitor_results':
                      {'ok': ok,
                       'warn' : warn,
                       'crit' : crit,
                       'unknown': unknown
                   }
               }
           ]
           
           results["server"].append(server);
           ok = 0
           warn = 0
           crit = 0
    else:
        obj = data['monitoring']
        for status in obj:
            if (status['pluginStatusCode'] == "0"):
                ok = ok + 1
            elif (status['pluginStatusCode'] == "1"):
                warn = warn + 1
            elif (status['pluginStatusCode'] == "2"):
                crit = crit +1
            else:
                unknown = unknown +1
        if (crit > 0):
            res = "I'm not fine!"
            ret_code = 2
        else:
            if (warn > 0):
                res = "I'm so so"
                ret_code = 1
            else:
                res = "I'm "
                if (unknown > 0):
                    res = res + " not completly sure to be honest."
                    ret_code = 3
                else:
                    res = res + " fine, thanks for asking!"
                    ret_code = 0

        results = [
            { 'answer' : res,
                'return_code' : ret_code,    
            'monitor_results':
                   {'ok': ok,
                     'warn' : warn,
                    'crit' : crit,
                    'unknown': unknown
                }  
            }
        ]

    return jsonify(results)

@app.route('/howru/monitoring/ok', methods=['GET'])
def api_show_oks():
    global data, multi_server
    set_file_name()
    load_data()

    results = []

    if (multi_server):
        serv = data['server']
        for s in serv:
            res_set = []
            res_set.append(s['host'])
            obj = s['monitoring']
            for i in obj:
               if (i['pluginStatusCode'] == "0"):
                   res_set.append(i)
            results.append(res_set);
    else:
        results.append(data['host'])
        obj = data['monitoring']
        for i in obj:
            if (i['pluginStatusCode'] == "0"):
                results.append(i)

    return jsonify(results)

@app.route('/howru/monitoring/warnings', methods=['GET'])
def api_show_warnings():
    global data, multi_server 
    set_file_name()
    load_data()
    results = []

    if (multi_server):
        serv = data['server']
        for s in serv:
            res_set = []
            res_set.append(s['host'])
            obj = s['monitoring']
            for i in obj:
               if (i['pluginStatusCode'] == "1"):
                   res_set.append(i)
            results.append(res_set);
    else:
        results.append(data['host'])
        obj = data['monitoring']
        for i in obj:
            if (i['pluginStatusCode'] == "1"):
                results.append(i)

    return jsonify(results)

@app.route('/howru/monitoring/criticals', methods=['GET'])
def api_show_criticals():
    global data, multi_server
    set_file_name()
    load_data()
    results = []

    if (multi_server):
        serv = data['server']
        for s in serv:
            res_set = []
            res_set.append(s['host'])
            obj = s['monitoring']
            for i in obj:
               if (i['pluginStatusCode'] == "2"):
                   res_set.append(i)
            results.append(res_set);
    else:
       results.append(data['host'])
       obj = data['monitoring']
       for i in obj:
           if (i['pluginStatusCode'] == "2"):
               results.append(i)

    return jsonify(results)

@app.route('/howru/monitoring/changes', methods=['GET'])
def api_show_changes():
    global data, multi_server
    set_file_name()
    load_data()
    results = []

    if (multi_server):
        serv = data['server']
        for s in serv:
            res_set = []
            res_set.append(s['host'])
            obj = s['monitoring']
            for i in obj:
               if (i['pluginStatusChanged'] == "1"):
                   res_set.append(i)
            results.append(res_set);
    else:
       results.append(data['host'])
       obj = data['monitoring']
       for i in obj:
           if (i['pluginStatusChanged'] == "1"):
               results.append(i)

    return jsonify(results)

@app.route('/howru/monitoring/plugin', methods=['GET'])
def api_show_plugin():
    global data, multi_server
    set_file_name()
    load_data()
    do_start = True
    info = []
    name = ""
    server = ""
    server_found = 0
    id = -1
    id_count = 0
    results = []

    if (multi_server):
        if 'server' in request.args:
            server = request.args['server']
        if 'name' in request.args:
            name = request.args['name']
        elif 'id' in request.args:
            id = int(request.args['id'])
 
        for x in data['server']:
            this_server = x['host']['name']
            if (this_server == server):
                print ("server found")
                server_found = 1
                s_name = {
                    'name': this_server
                    }
                results.append(s_name)
                if ((len(name) < 1) and (id < 0)):
                    do_start = False
                    info = [
                        { 'returnCode' :'2',
                            'monitoring':
                                {'info': 'No id or name provided for plugin'
                            }
                        }
                    ]
                    results.append(info)
                    return jsonify(results)
                monitor_obj = x['monitoring']
                for i in monitor_obj:
                    if ((id_count == id) or (name == i['pluginName'])):
                        results.append(i)
                        break
                    id_count = id_count + 1
        if (server_found == 0):
            s_j = "server_search: " + server
            results.append(s_j)
            info = {
                    'info': 'Server not found in api'
                }
            results.append(info)
            return jsonify(results)
        return jsonify(results)

    results.append(data['host'])
    if 'name' in request.args:
        name = request.args['name']
    elif 'id' in request.args:
        id = int(request.args['id'])
    else:
        do_start = False
        info = [
           { 'returnCode' :'2',
                'monitoring':
                   {'info': 'No id or name provided for plugin'
                   }
           }
        ]
        results.append(info)
    if (do_start):
       obj = data['monitoring']
       for i in obj:
           if ((id_count == id) or (name == i['pluginName'])):
               results.append(i)
               break
           id_count = id_count + 1

    return jsonify(results)

@app.route('/howru/monitoring/servers', methods=['GET'])
def api_show_server():
    global data
    global server_list
    id = -1
    results = []
    set_file_name()
    load_data()

    if 'id' in request.args:
        id = int(request.args['id'])
    if (id == -1 ):
        if 'host' in request.args:
            # Search host
            server_name = request.args['host']
            s_data = data["server"]
            for host in s_data:
                if (server_name == host["host"]["name"]):
                    server_data = host
                    results.append(server_data)
        else:
            results = [
                    { 'returnCode' : '2',
                        'servers':
                            {
                             'info': 'No server id provided'
                            }
                    }
            ]
    else:
        # results by id
        server_name = server_list[id]
        s_data = data["server"]
        count = 0
        for host in s_data:
            if (count == id):
                server_name = host["host"]["name"]
                server_data = host
            count = count + 1
        results.append(server_data)

    
    return jsonify(results)

@app.route('/howru/settings/plugins', methods=['GET'])
def api_show_settings():
    global settings
    load_settings()
    results = []
    #results = settings
    for s in settings:
        if (s[0] != '#'):
            s_arr = s.split(';')
            pos = s_arr[0].find(']')
            p_name = s_arr[0][1:pos]
            p_des = s_arr[0][pos+2:]
            p_info = [
                    { 'pluginName' : p_name,
                        'settings':
                        {'description': p_des,
                         'command': s_arr[1],
                         'active': s_arr[2],
                         'interval': s_arr[3].rstrip()
                        }
               }
            ]
            results.append(p_info)

    return jsonify(results)

@app.route('/howru/settings/scheduler', methods=['GET'])
def api_show_scheduler_settings():
    global settings
    load_scheduler_settings()
    results = []

    #results = settings
    for s in settings:
        s.rstrip()
        s_arr = s.split('=')
        pos = s_arr[0].find('.')
        configType = s_arr[0][:pos]
        configName = s_arr[0][pos+1:]
        pos = s.find('=')
        configValue = s[pos+1:].rstrip()
        if (len(configType) > 2):
            s_info = [
                    { 'configType': configType,
                      'name': configName,
                      'value': configValue
                    }
            ]
            results.append(s_info)

    return jsonify(results)

if __name__ == '__main__':
    use_port = load_conf()
    app.run(host='0.0.0.0', port=use_port)
