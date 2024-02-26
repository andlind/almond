import json
import flask 
from flask import request, jsonify, render_template, redirect, url_for, send_from_directory, make_response
import os, os.path
import glob
import random
import socket
import logging
from venv import logger
from admin_page import admin_page

app = flask.Flask(__name__)
app.config["DEBUG"] = True
data = None
settings = None
bindPort = None
multi_server = False
multi_metrics = False
enable_file = False
enable_ssl = False
enable_gui = False
ausername = ''
apassword = ''
server_list_loaded = 0
server_list = []
file_found = 1
data_dir="/opt/almond/data"
file_name = ''
data_file = "/opt/almond/data/howru.json"
metrics_dir="/opt/almond/data/metrics"
full_metrics_file_name="monitor.metrics"
start_page = 'api'
ssl_certificate="/opt/almond/www/api/certificate.pem"
ssl_key="/opt/almond/www/api/certificate.key"
enable_scraper=False
aliases = []
valid_aliases = []

ok_quotes = ["I'm ok, thanks for asking!", "I'm all fine, hope you are too!", "I think I never felt better!", "I feel good, I knew that I would", "I feel happy from head to feet"]
warn_quotes = ["I'm so so", "I think someone should check me out", "Something is itching, scratch my back!", "I think I'm having a cold", "I'm not feeling all well"]
crit_quotes = ["I'm not fine", "I feel sick, please call the doctor", "Not good, please get a technical guru to check me out", "Code red, code red", "I have fever, an aspirin needed"]

current_version = '0.6.3'

app.secret_key = 'BAD_SECRET_KEY'
app.register_blueprint(admin_page)

def findPos(entry):
    pos = entry.find('=')
    return pos + 1

def load_aliases():
    global aliases, logger
    if os.path.isfile('/etc/almond/aliases.conf'):
        logger.info("Reading aliases from '/etc/almond/aliases'")
        df = open('/etc/almond/aliases.conf', 'rt')
        lines = df.read().split('\n')
        for l in lines:
            try:
                data = json.loads(l)
            except json.decoder.JSONDecodeError:
                return
            aliases.append(data)
            valid_aliases.append(data["alias"])

def load_conf():
    global bindPort, multi_server, multi_metrics, metrics_dir, enable_file, data_file, data_dir, enable_ssl, start_page, enable_gui, full_metrics_file_name
    global ssl_certificate, ssl_key, enable_scraper
    if os.path.isfile('/etc/almond/api.conf'):
        conf = open("/etc/almond/api.conf", "r")
    else:
        conf = open("/etc/almond/almond.conf", "r")
    for line in conf:
        if (line.find('data') == 0):
            if (line.find('jsonFile') > 0):
                json_file = line[findPos(line):].rstrip()
            if (line.find('metricsFile') > 0):
                full_metrics_file_name = line[findPos(line):].rstrip()
        if (line.find('api') == 0):
            if (line.find('Data') > 0):
                data_dir = line[findPos(line):].rstrip()
            if (line.find('metricsDir') > 0):
                metrics_dir = line[findPos(line):].rstrip()
            if (line.find('Port') > 0):
                port = line[findPos(line):]
                print (port)
                if (isinstance(int(port), int)):
                   bindPort = int(port)
                else:
                    bindPort = 80
            if (line.find('Server') > 0):
                multi_s = line[findPos(line)]
                if (isinstance(int(multi_s), int)):
                    if (int(multi_s) > 0):
                        multi_server = True
                    else:
                        multi_server = False
                else:
                    multi_server = False
            if (line.find('multiMetrics') > 0):
                multi_m = line[findPos(line)]
                if (isinstance(int(multi_m), int)):
                    if (int(multi_m) > 0):
                        multi_metrics = True
                    else:
                        multi_metrics = False
                else:
                    multi_metrics = False
            if (line.find('enableFile') > 0):
                bFile = line[findPos(line)]
                if (isinstance(int(bFile), int)):
                    if (int(bFile) > 0):
                        enable_file = True
                    else:
                        enable_file = False
                else:
                    enable_file = False
            if (line.find('SSL') > 0):
                useSSL = line[findPos(line)]
                if (isinstance(int(useSSL), int)):
                    if (int(useSSL) > 0):
                        enable_ssl = True
                    else:
                        enable_ssl = False
                else:
                    enable_ssl = False
            if (line.find('sslCertificate') > 0):
                ssl_certificate = line[findPos(line):].rstrip()
            if (line.find('sslKey') > 0):
                ssl_key = line[findPos(line):].rstrip()
            if (line.find('startPage') > 0):
                start_page = line[findPos(line):].rstrip()
            if (line.find('useGUI') > 0):
                bUseGui = line[findPos(line)]
                if (isinstance(int(bUseGui), int)):
                    if (int(bUseGui) > 0):
                        enable_gui = True
                    else:
                        enable_gui = False
                else:
                    enable_gui= False
            if (line.find('enableScraper') > 0):
                bScraperEnable = line[findPos(line)]
                if (isinstance(int(bScraperEnable), int)):
                    if (int(bScraperEnable) > 0):
                        enable_scraper = True
                    else:
                        enable_scraper = False
                else:
                    enable_scraper = False
            if (line.find('enableAliases') > 0):
                    bAliasesEnabled = line[findPos(line)]
                    if (isinstance(int(bAliasesEnabled), int)):
                        if (int(bAliasesEnabled) > 0):
                            load_aliases()
            if (line.find('adminuser') > 0):
                ausername = line[findPos(line):].rstrip()
            if (line.find('adminpassword') > 0):
                apassword = line[findPos(line):].rstrip()

    conf.close()
    data_file = data_dir 
    data_file = data_dir + "/" + json_file
    return bindPort

def useCertificate():
    global enable_ssl
    return enable_ssl

def getCertificates():
    global ssl_certificate, ssl_key
    ret_val = "('" + ssl_certificate + "', '" + ssl_key + "')"
    return eval(ret_val)

def load_data():
    global data, data_dir, multi_server, enable_file, file_name, data_file, file_found, logger
    os.chdir(data_dir)
    if (enable_file == True):
        if (len(file_name) > 5):
            if (os.path.exists(file_name)):
                count = 0
                f = open(file_name, "r")
                data = json.loads(f.read())
                f.close()
                file_found = 1
                return
            else:
                print ("Could not find serverfile ", file_name)
                logger.warning("Could not find serverfile '" + file_name + "'")
                file_found = 0
    if (multi_server):
       #print("Running in mode multi");
       logger.info("Load data in multi_server mode")
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
       logger.info("Loading single node data.")
       if os.path.isfile(data_file):
           f = open(data_file, "r")
           data = json.loads(f.read())
           f.close()
       else:
           data = {}
    os.chdir("/opt/almond/www/api")

def load_settings():
    global settings, logger
    logger.info("Reading settings in '/etc/almond/plugins.conf'")
    f = open("/etc/almond/plugins.conf", "r")
    try:
        settings = f.readlines()
    finally:
        f.close()

def load_scheduler_settings():
    global settings, logger
    logger.info("Reading settings in '/etc/almond/almond.conf'")
    f = open("/etc/almond/almond.conf", "r")
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

def rand_quote(severity):
    ok_quotes = ["I'm ok, thanks for asking!", "I'm all fine, hope you are too!", "I think I never felt better!", "I feel good, I knew that I would", "I feel happy from head to feet"]
    warn_quotes = ["I'm so so", "I think someone should check me out", "Something is itching, scratch my back!", "I think I'm having a cold", "I'm not feeling all well"]
    crit_quotes = ["I'm not fine", "I feel sick, please call the doctor", "Not good, please get a technical guru to check me out", "Code red, code red", "I have fever, an aspirin needed"]
    unknown_quotes = ["I'm not completly sure to be honest", "Some things are unknown to me", "I'm not really certain", "I don´t actually no", "Not sure, maybe good - maybe not!"]
    if (severity == 0):
        return random.choice(ok_quotes)
    if (severity == 1):
        return random.choice(warn_quotes)
    if (severity == 2):
        return random.choice(crit_quotes)
    else:
        return random.choice(unknown_quotes)

@app.route('/', methods=['GET'])
def home():
    global data
    global server_list
    global server_list_loaded
    global data_file
    global enable_gui
    global logger

    full_filename = '/static/howru.png'
    almond_image = '/static/almond_small.png'
    if not enable_gui:
        return render_template("403.html")
    if (start_page == 'status'):
        return api_show_status()
    if (start_page == 'metrics'):
        return api_show_metrics()
    if (start_page == 'admin'):
        return redirect("/almond/admin") 
    os.chdir(data_dir)
    if (multi_server):
        if (server_list_loaded == 0):
            set_file_name()
            load_data()
            s_data = data["server"]
            for host in s_data:
                server_name = host["host"]["name"]
                server_list.append(server_name)
            server_list_loaded = 1
        logger.info("Render index template.")
        if (len(server_list) > 0):
            return render_template("index_m.html", len = len (server_list), server_list = server_list, user_image = full_filename)
        else:
            return render_template("index_e.html", user_image = almond_image)
        server_list_loaded = 0
        server_list.clear()
    else:
        if enable_gui:
            logger.info("Render index template")
            if (os.path.exists(data_file)):
                return render_template("index.html", user_image = full_filename)
            else:
                return render_template("index_w.html", user_image = almond_image)
        else:
            logger.info("No HTML GUI is enabled.")
            return "No GUI enabled"
    os.chdir('/opt/almond/www/api')

@app.route('/docs', methods=['GET'])
def documentation():
    full_filename = '/static/howru.png'
    if not enable_gui:
        return render_template("403.html")
    return render_template("documentation.html", user_image = full_filename)
 
@app.route('/pluginSearch', methods=['GET'])
def search_plugin():
    full_filename = '/static/howru.png'
    if not enable_gui:
        return render_template("403.html")
    if (multi_server):
        return render_template("plugin_query_mm.html", user_image = full_filename)
    else:
        return render_template("plugin_query.html", user_image = full_filename)

@app.route('/api/all', methods=['GET'])
@app.route('/api/json', methods=['GET'])
@app.route('/api/showall', methods=['GET'])
@app.route('/howru/api/showall', methods=['GET'])
@app.route('/howru/api/all', methods=['GET'])
@app.route('/howru/api/json', methods=['GET'])
def api_json(response = True):
   global data
   server_found = 0
   set_file_name()
   load_data()
   if not ('server' in request.args): 
       if (response):
           return jsonify(data)
       else:
           return data
   else:
       servername = request.args['server']
       for serv in data['server']:
           name_o = serv['host']
           name = name_o['name']
           if (name == servername):
               result = serv
               server_found = 1
       if (server_found == 1):
           if (response):
               return jsonify(result)
           else:
               return result
       else:
           result = [
                    { 'returnCode' :'3',
                       'monitoring':
                               {'info': 'Server not found'
                            }
                        }
                    ]
           headers = {"Content-Type": "application/json"}
           return make_response(jsonify(result), 200, headers)

@app.route('/monitoring/json', methods=['GET'])
@app.route('/howru/monitoring/json', methods=['GET'])
def api_old_json():
    if not ('server' in request.args):
        return redirect(url_for('api_json'))
    else:
        servername = request.args['server']
        return redirect(url_for('api_json', server=servername))

@app.route('/api/status', methods=['GET']
@app.route('/api/summary', methods=['GET'])
@app.route('/api/howru', methods=['GET'])
@app.route('/api/howareyou', methods=['GET'])
@app.route('/howru/api/howareyou', methods=['GET'])
def api_howareyou(response=True):
    global data, multi_server, file_found
    set_file_name()
    load_data()
    ok = 0
    warn = 0
    crit = 0
    results = []
    unknown = 0
    res = "I am not sure"
    servername = ""
    
    if (multi_server):
        results = {
            "server": [
                      ]
            }

        if ('server' in request.args):
            servername = request.args['server']
        if (len(servername) > 2 ):
            for serv in data['server']:
                name_o = serv['host']
                name = name_o['name']
                if (name == servername):
                    mon = serv['monitoring']
                    for status in mon:
                        if (status['pluginStatusCode'] == "0"):
                            ok = ok + 1
                        elif (status['pluginStatusCode'] == "1"):
                            warn = warn + 1
                        elif (status['pluginStatusCode'] == "2"):
                            crit = crit + 1
                        else:
                            unknown = unknown + 1
                    if (crit > 0):
                        ret_code = 2
                    else:
                        if (warn > 0):
                            ret_code = 1
                        else:
                            if (unknown > 0):
                                ret_code = 3
                            else:
                                ret_code = 0
                    res = rand_quote(ret_code)
                    result = [
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

                    if (response):
                        return jsonify(result)
                    else:
                        return result
            
        if (len(file_name) > 2):
            if (file_found == 0):
                res = "File not found"
                ret_code = 3
                result = [
                        {    'name': file_name,
                             'answer': res,
                             'return_code': ret_code
                        }
                ]
                return jsonify(result)
            name_o = data['host']
            name = name_o['name']
            obj = data['monitoring']
            for status in obj:
                if (status['pluginStatusCode'] == "0"):
                    ok = ok + 1
                elif (status['pluginStatusCode'] == "1"):
                    warn = warn + 1
                elif (status['pluginStatusCode'] == "2"):
                    crit = crit + 1
                else:
                    unknown = unknown + 1
            if (crit > 0):
                ret_code = 2
            else:
                if (warn > 0):
                    ret_code = 1
                else:
                    if (unknown > 0):
                        ret_code = 3
                    else:
                        ret_code = 0
            res = rand_quote(ret_code)
            server = {  'name' : name,
                        'answer' : res,
                        'return_code' : ret_code,
                        'monitor_results':
                           {'ok': ok,
                            'warn' : warn,
                            'crit' : crit,
                            'unknown': unknown
                        }
                    }
            results["server"].append(server)
        else:
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
                   ret_code = 2
               else:
                   if (warn > 0):
                       ret_code = 1
                   else:
                       if (unknown > 0):
                           ret_code = 3
                       else:
                           ret_code = 0
               res = rand_quote(ret_code)
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
               unknown = 0
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
            ret_code = 2
        else:
            if (warn > 0):
                ret_code = 1
            else:
                if (unknown > 0):
                    ret_code = 3
                else:
                    ret_code = 0
        res = rand_quote(ret_code)
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
    if (response):
        return jsonify(results)
    else:
        return results

@app.route('/monitoring/howareyou', methods=['GET'])
@app.route('/howru/monitoring/howareyou', methods=['GET'])
def api_old_howareyou():
    if ('server' in request.args):
        servername = request.args['server']
        return_redirect(url_for('api_howareyou', server=servername))
    else:
        return redirect(url_for('api_howareyou'))

@app.route('/api/oks', methods=['GET'])
@app.route('/api/ok', methods=['GET'])
@app.route('/howru/api/ok', methods=['GET'])
def api_show_oks():
    global data, multi_server
    set_file_name()
    load_data()
    multi_set = 0
    servername = ""
    name_is_set = False
    name_found = False

    results = []

    if (multi_server):
        try:
            host = data['host']
        except:
            multi_set = 1
    if (multi_set > 0):
        serv = data['server']
        for s in serv:
            res_set = []
            if ('server' in request.args):
                servername = request.args['server']
                if (len(servername) > 2):
                    name_is_set = True
            if (name_is_set == True):
                 if (s['host']['name'] == servername):
                    res_set.append(s['host'])
                    obj = s['monitoring']
                    for i in obj:
                        if (i['pluginStatusCode'] == "0"):
                            res_set.append(i)
                    results.append(res_set)
                    server_found = True
                    return jsonify(results)
                 else:
                     server_found = False
            else:
                res_set.append(s['host'])
                obj = s['monitoring']
                for i in obj:
                    if (i['pluginStatusCode'] == "0"):
                        res_set.append(i)
                results.append(res_set);
        if (name_is_set == True and server_found == False):
            results = [
                { 'returnCode' :'3',
                   'monitoring':
                           {'info': 'Server not found'
                        }
                    }
                ]
    else:
        results.append(data['host'])
        obj = data['monitoring']
        for i in obj:
            if (i['pluginStatusCode'] == "0"):
                results.append(i)

    return jsonify(results)

@app.route('/monitoring/ok', methods=['GET'])
@app.route('/howru/monitoring/ok', methods=['GET'])
def api_old_ok():
    if ('server' in request.args):
        servername = request.args['server']
        return redirect(url_for('api_show_oks', server=servername))
    else:
        return redirect(url_for('api_show_oks'))

@app.route('/api/warnings', methods=['GET'])
@app.route('/howru/api/warnings', methods=['GET'])
def api_show_warnings():
    global data, multi_server 
    set_file_name()
    load_data()
    multi_set = 0
    results = []
    servername = ""
    name_is_set = False
    name_found = False

    if (multi_server):
        try:
            host = data['host']
        except:
             multi_set = 1
    if (multi_set > 0):
        serv = data['server']
        for s in serv:
            res_set = []
            if ('server' in request.args):
                servername = request.args['server']
                if (len(servername) > 2):
                    name_is_set = True
            if (name_is_set == True):
                if (s['host']['name'] == servername):
                    res_set.append(s['host'])
                    obj = s['monitoring']
                    for i in obj:
                        if (i['pluginStatusCode'] == "1"):
                            res_set.append(i)
                    results.append(res_set)
                    server_found = True
                    return jsonify(results)
                else:
                    server_found = False
            else:
                res_set.append(s['host'])
                obj = s['monitoring']
                for i in obj:
                   if (i['pluginStatusCode'] == "1"):
                       res_set.append(i)
                results.append(res_set);
        if (name_is_set == True and server_found == False):
            results = [
                    { 'returnCode' :'3',
                       'monitoring':
                               {'info': 'Server not found'
                            }
                        }
                    ]
    else:
        results.append(data['host'])
        obj = data['monitoring']
        for i in obj:
            if (i['pluginStatusCode'] == "1"):
                results.append(i)

    return jsonify(results)

@app.route('/monitoring/warnings', methods=['GET'])
@app.route('/howru/monitoring/warnings', methods=['GET'])
def api_old_warnings():
    if ('server' in request.args):
        servername = request.args['server']
        return redirect(url_for('api_show_warnings', server=servername))
    else:
        return redirect(url_for('api_show_warnings'))

@app.route('/api/criticals', methods=['GET'])
@app.route('/howru/api/criticals', methods=['GET'])
def api_show_criticals():
    global data, multi_server
    set_file_name()
    load_data()
    multi_set = 0
    results = []
    servername = ""
    name_is_set = 0
    name_found = 0

    if (multi_server):
        try:
            host = data['host']
        except:
            multi_set = 1
    if (multi_set > 0):
        serv = data['server']
        for s in serv:
            res_set = []
            if ('server' in request.args):
                servername = request.args['server']
                if (len(servername) > 2):
                    name_is_set = 1
            if (name_is_set > 0):
                if (s['host']['name'] == servername):
                    res_set.append(s['host'])
                    obj = s['monitoring']
                    for i in obj:
                        if (i['pluginStatusCode'] == "2"):
                            res_set.append(i)
                    results.append(res_set)
                    name_found = 1
                    return jsonify(results) 
                else:
                    name_found = 0
            else:
                res_set.append(s['host'])
                obj = s['monitoring']
                for i in obj:
                    if (i['pluginStatusCode'] == "2"):
                        res_set.append(i)
                results.append(res_set);
        if (name_is_set > 0 and name_found == 0):
            result = [
                    { 'returnCode' :'3',
                       'monitoring':
                               {'info': 'Server not found'
                            }
                        }
                    ]
            return jsonify(result)
    else:
       results.append(data['host'])
       obj = data['monitoring']
       for i in obj:
           if (i['pluginStatusCode'] == "2"):
               results.append(i)

    return jsonify(results)

@app.route('/monitoring/criticals', methods=['GET'])
@app.route('/howru/monitoring/criticals', methods=['GET'])
def api_old_criticals():
    if ('server' in request.args):
        servername = request.args['server']
        return redirect(url_for('api_show_criticals', server=servername))
    else:
        return redirect(url_for('api_show_criticals'))

@app.route('/api/changes', methods=['GET'])
@app.route('/howru/api/changes', methods=['GET'])
def api_show_changes():
    global data, multi_server
    set_file_name()
    load_data()
    multi_set = 0
    results = []

    if (multi_server):
        try:
            host = data['host']
        except:
            multi_set = 1
    if (multi_set > 0):
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

@app.route('/monitoring/changes', methods=['GET'])
@app.route('/howru/monitoring/changes', methods=['GET'])
def api_old_changes():
    return redirect(url_for('api_show_changes'))

@app.route('/api/search', methods=['GET'])
@app.route('/api/querysearch', methods=['GET'])
@app.route('howru/api/search', methods=['GET']):
    return api_show_plugin(1)

@app.route('/api/query', methods=['GET'])
@app.route('/api/plugin', methods=['GET'])
@app.route('/howru/api/plugin', methods=['GET'])
def api_show_plugin(search=0):
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
        if (search == 0):
            if 'server' in request.args:
                server = request.args['server']
            if 'name' in request.args:
                name = request.args['name']
            elif 'id' in request.args:
                id = int(request.args['id'])
        else:
            if 'server' in request.args:
                server = request.args['server']
            if 'name' in request.args:
                name = request.args('name')
            elif 'query' in request.args:
                name = request.args('query')
            elif 'find' in request.args:
                name = request.args('find')
            if (name == ""):
                if 'id' in request.args:
                    id = int(request.args['id']) 
        
        for x in data['server']:
            this_server = x['host']['name']
            if (this_server == server):
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
                    if (search == 0):
                        if ((id_count == id) or (name == i['pluginName'])):
                            results.append(i)
                            break
                    else:
                        if ((id_count == id) or (name in i['name']) or (name in i['pluginName'])):
                            results.append(i)
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

    # else
    results.append(data['host'])
    if (search == 0):
        if 'name' in request.args:
            name = request.args['name']
        elif 'id' in request.args:
            id = int(request.args['id'])
        else:
            do_start = False
    else:
        if 'find' in request.args:
            name = request.args['find']
        elif 'query' in request.args:
            name = request.args['query']
        elif 'name' in request.args:
            name = request.args['name']
        elif 'id' in request.args:
            id = int(request.args['id'])
        else:
            do_start = False
    if (do_start):
       obj = data['monitoring']
       for i in obj:
           if (search == 0):
               if ((id_count == id) or (name == i['pluginName'])):
                   results.append(i)
                   break
           else:
               if ((name in i['name']) or (name in i['pluginName'])):
                   results.append(i)
               elif (id_count == id):
                   results.append(i)
                   break
           id_count = id_count + 1
    else:
        info = [
           { 'returnCode' :'2',
                'monitoring':
                   {'info': 'No id or name provided for plugin'
                   }
           }
        ]
        results.append(info)
    return jsonify(results)

@app.route('/monitoring/plugin', methods=['GET'])
@app.route('/howru/monitoring/plugin', methods=['GET'])
def api_old_plugin():
    if ('server' in request.args):
        servername = request.args['server']
        if ('name' in request.args):
            plugin_name = request.args['name']
            return redirect(url_for('api_show_plugin', server=servername, name=plugin_name))
        if ('id' in request.args):
            this_id = request.args['id']
            return redirect(url_for('api_show_plugin', server=servername, id=this_id))
        return redirect(url_for('api_show_plugin', server=servername))
    if ('name' in request.args):
        name = request.args['name']
        return redirect(url_for('api_show_plugin', name=name))
    elif ('id' in request.args):
        this_id = int(request.args['id'])
        return redirect(url_for('api_show_plugin', id=this_id))
    else:
        return redirect(url_for('api_show_plugin'))

@app.route('/api/servers', methods=['GET'])
@app.route('/howru/api/servers', methods=['GET'])
def api_show_server():
    global data
    global server_list
    global server_list_loaded
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
        # BUG: server_list loaded in home, must be loaded here if not
        if (server_list_loaded == 0):
            try:
                s_data = data["server"]
            except:
                results = [
                        { 'returnCode' : '3',
                            'servers':
                               { 
                                'info': 'Server api not enabled in single mode'
                               }
                        }
                ]
                return jsonify(results)
            for host in s_data:
                server_name = host["host"]["name"]
                server_list.append(server_name)
            server_list_loaded = 1
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

@app.route('/monitoring/servers', methods=['GET'])
@app.route('/howru/monitoring/servers', methods=['GET'])
def api_old_servers():
    if ('id' in request.args):
        return redirect(url_for('api_show_servers', id=request.args['id']))
    elif ('host' in request.args):
        return redirect(url_for('api_show_servers', host=request.args['host']))
    else:
        return redirect(url_for('api_show_servers'))

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

@app.route('/howru/metrics', methods=['GET'])
def api_show_metric_lists():
    global metrics_dir
    global full_metrics_file_name
    full_filename = '/static/howru.png'
    metrics_list = []
    #onlyfiles = [f for f in os.listdir(metrics_dir) if os.path.isfile(join(metrics_dir, f))]
    for f in os.listdir(metrics_dir):
        if (f == full_metrics_file_name):
            f = "Current metrics"
        metrics_list.append(f)
    metrics_list.sort()
    if not enable_gui:
        return render_template("403.html")
    else:
        return render_template("metrics.html", user_image = full_filename, metrics_list=metrics_list)

@app.route('/howru/show_metrics', methods=['GET'])
def api_show_metrics():
    global metrics_dir, full_metrics_file_name
    metric_selection = ''
    file_name = ''
    return_list = []
    if not enable_gui:
        return render_template("403.html")
    if 'metric' in request.args:
        metric_selection = request.args['metric']
        if metric_selection == '-1':
            return redirect(url_for('api_show_metric_lists'))
        else:
            file_name = metrics_dir + '/' +  metric_selection
            if metric_selection == 'Current metrics':
                file_name = metrics_dir + '/' + full_metrics_file_name
            with open(file_name) as f:
                return_list = f.readlines()
                if not metric_selection == 'Current metrics':
                    return_list.reverse()
                f.close()
    else:
        #Error: no metric selection
        return redirect(url_for('api_show_metric_lists'))
    #return jsonify(return_list)
    return render_template("show_metrics.html", b_lines=return_list)

@app.route('/monitoring/status', methods=['GET'])
@app.route('/howru/monitoring/status', methods=['GET'])
def api_show_status():
    global multi_server
    global data_file
    almond_image = '/static/almond_small.png'
    load_data()
    if not os.path.isfile(data_file):
        return render_template("index_w.html", user_image = almond_image)
    this_data = api_howareyou(False)
    full_filename = '/static/howru.png'
    image_icon = '/static/green.png'

    if not enable_gui:
        return render_template("403.html")

    if (multi_server):
        servers = [] 
        icons = ['/static/green.png', '/static/yellow.png', '/static/red.png']
        server_data = this_data['server']
        for server in server_data:
            servers.append(server[0])
        servers_sorted = sorted(servers, key=lambda n: n['name'])
        return render_template("status_mm.html", user_image = full_filename, servers = servers_sorted, icons = icons)
    else:
        if not os.path.isfile(data_file):
            return "No data file"
        hostname = socket.getfqdn()
        ret_code = this_data[0]['return_code']
        if (ret_code == 2):
            image_icon = '/static/red.png'
        elif (ret_code == 1):
            image_icon = '/static/yellow.png'
        else:
            image_icon = '/static/green.png'
        mon_res = this_data[0]['monitor_results']
        num_of_ok = mon_res['ok']
        num_of_warnings = mon_res['warn']
        num_of_criticals = mon_res['crit']
        num_of_unknown = mon_res['unknown']
        # only in json mode -> show server status
    return render_template("status.html", user_image = full_filename, server = hostname, icon = image_icon, oks = num_of_ok, warnings = num_of_warnings, criticals = num_of_criticals, unknown = num_of_unknown)

@app.route('/monitoring/details', methods=['GET', 'POST'])
@app.route('/howru/monitoring/details', methods=['GET', 'POST'])
def api_show_details():
    global multi_server
    global logger
    this_data = api_json(False)
    full_filename = '/static/howru.png'

    if not enable_gui:
        return render_template("403.html")
    
    if (multi_server):
        monitoring = []
        server = request.args['name']
        monitoring_data = this_data['server']
        for obj in monitoring_data:
            logger.info("Object hostname is: " + obj['host']['name'])
            #print (obj['host']['name'])
            if (obj['host']['name'] == server):
                #print ("Found server: " + server)
                logger.info("Found server '" + server + "'")
                #print (obj['monitoring'])
                monitoring = obj['monitoring']
                #print ("Monitoring is of type:", type(monitoring))
                logger.debug("Monitoring is of type:", type(monitoring))
                break
        if len(monitoring) == 0:
            logger.warning("No monitoring data found for server '" + server + "'")  
            return "No monitoring data found for server: " + server
        logger.info("Rendering details template.")
        return render_template("details.html", user_image=full_filename, server=server, monitoring=monitoring)
    else:
        hostname = this_data['host']['name']
        monitoring = this_data['monitoring']
        return render_template("details.html", user_image=full_filename, server=hostname, monitoring=monitoring)

@app.route('/monitoring/graph', methods=['GET', 'POST'])
@app.route('/howru/monitoring/graph', methods=['GET', 'POST'])
def api_show_graph():
    full_filename = '/static/howru.png'
    current_plot = "check_memory"
    x_axis =[10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
    data = [22.35, 21.15, 35.42, 88.12, 27.95, 28.35, 42, 75.33, 93.12, 25.87]
    plt.plot(x_axis, data)
    plt.title('Memory usage')
    plt.xlabel('Memory percentage')
    plt.ylabel('Memory used')
    plt.savefig('static/charts/chart.png')
    return render_template("graph.html", user_image = full_filename, name="Memory usage", url="/static/charts/chart.png")

@app.route('/metrics', methods=['GET'])
def api_prometheus_export():
    global enable_scraper, multi_metrics, metrics_dir, full_metrics_file_name
    current_metrics_files = []
    ret_list = []
    if multi_metrics:
        for file in os.listdir(metrics_dir):
            if file.endswith('.metrics'):
                file_name = metrics_dir + '/' + file
                current_metrics_files.append(file_name)
    else:
        file_name = metrics_dir + '/' + full_metrics_file_name
        current_metrics_files.append(file_name)
    #print (current_metrics_files)
    for file_name in current_metrics_files:
        #print (file_name)
        logger.info("Generating metrics from file '" + file_name + "'")
        if os.path.isfile(file_name):
            with open(file_name) as f:
                return_list = f.readlines()
                f.close()
                print (return_list)
            if enable_scraper:
                ret_val = []
                for line in return_list:
                    line = line.strip()
                    last_char = line[len(line)-1]
                    if (last_char == '%'):
                        line = line[:-1]
                        last_char = line[len(line)-1]
                    if (line.find('(') !=  -1):
                        line = line.replace("(", "_")
                        line = line.replace(")", "")
                    while (last_char.isalpha()):
                        line = line[:-1]
                        last_char = line[len(line)-1]
                    # Check if a time thing
                    time_thing = line[-8:].strip()
                    if (time_thing.find(':') != -1):
                        time_in_secs = 0
                        time_vals = time_thing.split(':')
                        time_in_secs = int(time_vals[0])*3600 + int(time_vals[1])*60 + int(time_vals[2])
                        line = line[:-8] + ' ' + str(time_in_secs)
                    line = line + '\n'
                    ret_val.append(line)
                print (ret_val)
                return_list = ret_val.copy()
        ret_list.extend(return_list)
    response = app.response_class(response=ret_list, status=200, mimetype='application/txt')
    return response
    # only in prometheus mode -> return metrics file
    return redirect(url_for('api_show_metric_lists'))

#@app.route('/howru/monitoring/graph', methods=['GET'])
#def get_graph():
#    x = [1,2,3]
#    # corresponding y axis values
#    y = [2,4,1]
#
#    # plotting the points 
#    plt.plot(x, y)
#
#    # naming the x axis
#    plt.xlabel('x - axis')
#    # naming the y axis
#    plt.ylabel('y - axis')
#
#    # giving a title to my graph
#    plt.title('My first graph!')
#
#    # function to show the plot
#    #return(plt.show())
#    fig = Figure()
#    ax = fig.subplots()
#    ax.plot([1, 2])
#    # Save it to a temporary buffer.
#    buf = BytesIO()
#    #fig.savefig(buf, format="png")
#    plt.savefig(buf, format="png")
#    #  plt.savefig('/static/images/new_plot2.png')
#    return render_template('graph.html', name='new_plot', url='/static/images/new_plot.png')
#    # Embed the result in the html output.
#    #data = base64.b64encode(buf.getbuffer()).decode("ascii")
#    #return f"<img src='data:image/png;base64,{data}'/>"

@app.route('/get-file/<path:path>', methods=['GET'])
def get_files(path):
    DOWNLOAD_DIRECTORY = "/opt/almond/data"
    try:
        return send_from_directory(DOWNLOAD_DIRECTORY, path, as_attachment=True)
    except FileNotFoundError:
        abort(404)

@app.route('/api/<string:Alias>', methods=['GET'])
def api_show_alias(Alias):
    global aliases, valid_aliases, logger

    if Alias in valid_aliases:
        logger.info("Collect data for alias '" + Alias + "'")
        this_id = valid_aliases.index(Alias)
        #return redirect(url_for('api_show_plugin', id=this_id))
        return api_show_plugin(0, id=this_id)
    else:
        logger.warning("Asked to collect data from unknown alias '" + Alias + "'")
        err_res = {"howru-version":current_version, "alias": Alias, "result":"Unknown api call. Call or alias does not exists."}
        return jsonify(err_res)

def main():
    global logger
    global current_version
    logging.basicConfig(filename='/var/log/almond/howru.log', filemode='a', format='%(asctime)s | %(levelname)s: %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    logger.info('Starting howru api (version:' + current_version + ')')
    use_port = load_conf()
    logger.info("Configuration read.")
    use_ssl = useCertificate()
    context = getCertificates()
    if (use_ssl):
        logger.info("Running application in ssl_context")
        app.run(host='0.0.0.0', port=use_port, ssl_context=context, threaded=True)
    else:
        logger.info("Running application without encryption")
        app.run(host='0.0.0.0', port=use_port)

if __name__ == '__main__':
    main()
