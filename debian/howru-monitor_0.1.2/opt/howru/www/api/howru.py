import json
import flask
from flask import request, jsonify, render_template
import os

app = flask.Flask(__name__)
app.config["DEBUG"] = True
data = None

def load_data():
    global data
    f = open("../monitor_data.json", "r")
    data = json.loads(f.read())
    f.close()

@app.route('/', methods=['GET'])
def home():
    full_filename = '/static/howru.png'
    return render_template("index.html", user_image = full_filename)

@app.route('/docs', methods=['GET'])
def documentation():
    full_filename = '/static/howru.png'
    return render_template("documentation.html", user_image = full_filename)

@app.route('/api/v1/howru/monitoring/json', methods=['GET'])
def api_json():
   global data
   load_data()
   return jsonify(data)

@app.route('/api/v1/howru/monitoring/howareyou', methods=['GET'])
def api_howareyou():
    global data
    load_data()
    ok = 0
    warn = 0
    crit = 0
    results = []
    unknown = 0
    res = "I am not sure"

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
    else:
        if (warn > 0):
            res = "I'm so so"
        else:
            res = "I'm "
            if (unknown > 0):
                res = res + " not completly sure to be honest."
            else:
                res = res + " fine, thanks for asking!"

    results = [
       { 'answer' : res,
           'monitor_results':
               {'ok': ok,
                 'warn' : warn,
                 'crit' : crit,
                 'unknown': unknown
               }
       }
    ]

    return jsonify(results)

@app.route('/api/v1/howru/monitoring/ok', methods=['GET'])
def api_show_oks():
    global data
    load_data()
    results = []

    results.append(data['host'])
    obj = data['monitoring']
    for i in obj:
        if (i['pluginStatusCode'] == "0"):
            results.append(i)

    return jsonify(results)

@app.route('/api/v1/howru/monitoring/warnings', methods=['GET'])
def api_show_warnings():
    global data
    load_data()
    results = []

    results.append(data['host'])
    obj = data['monitoring']
    for i in obj:
        if (i['pluginStatusCode'] == "1"):
            results.append(i)

    return jsonify(results)

@app.route('/api/v1/howru/monitoring/criticals', methods=['GET'])
def api_show_criticals():
    global data
    load_data()
    results = []

    results.append(data['host'])
    obj = data['monitoring']
    for i in obj:
        if (i['pluginStatusCode'] == "2"):
            results.append(i)

    return jsonify(results)

app.run(host='0.0.0.0', port=80)
