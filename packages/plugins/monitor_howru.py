#!/usr/bin/python3
import json
import sys
import argparse
import datetime
import urllib.request
import urllib.parse

def get_metrics(jsonObject, api, ac):
    # metrics = {
    #     'status': 1,  # 1 for OK, 0 for problem
    #     'memory_usage': 45.67,
    #     'response_time': 0.123,
    #     'errors': 0
    # }
    metrics = {}
    try:
        json_object = json.loads(jsonObject)
    except json.decoder.JSONDecodeError as e:
        print(f"JSON decode error: {str(e)}")
        print(f"Raw JSON string:\n{jsonObject}")
        metrics.update({
            'status': 0,
            'errors': 1,
            'message': 'Could not load json'
        })
        return json.dumps(metrics)
    current_time = datetime.datetime.now()
    non_active_checks = 0
    current_time = datetime.datetime.now()
    non_active_checks = 0
    if (api == 'criticals' or api == 'ok' or api == 'warnings'):
        num_servers = 1
        try:
            server_name = json_object[0]['name']
        except:
            num_servers = 2
        if (num_servers > 1):
            return_message = ""
            result_message = ""
            all_results = ""
            result_code = 1
            return_code = 1
            count = 0
            for x in json_object:
                server_name = x[0]['name']
                length = len(x)
                if (length > 1):
                    count = 0
                    if (api == 'ok'):
                        if (ac < 30):
                            result_message = "OK: " + str(length-1) + " checks are ok on " + server_name + "\n"
                            result_code = 1
                        else:
                            result_message = str(length-1) + " checks are ok on " + server_name
                            for data in x:
                                count = count + 1
                                if (count > 1):
                                    try:
                                        date_time_obj = datetime.datetime.strptime(data['lastRun'], '%Y-%m-%d %H:%M:%S')
                                    except ValueError:
                                        date_time_obj = datetime.datetime.now()
                                        #print (data['name'] + " is not active.\n")
                                        non_active_checks = non_active_checks + 1
                                    if (calculateTimeDiff(current_time, date_time_obj) > ac):
                                        result_code = 0
                                        result_message = result_message + " (" + data['name'] + " has latency.) "
                            if (return_code == 1):
                                result_message = "OK: " + result_message + "\n"
                            else:
                                result_message = "WARNING: " + result_message + "\n"
                    else:
                        return_code = 0
                        if (len(return_message) < 1):
                            return_message = "CRITICAL: " + str(length-1) + " checks on " + server_name + " is not ok. ( "
                        else:
                            return_message = return_message + " | " +  str(length-1) + " checks on " + server_name + " is not ok. ( "
                        for data in x:
                            count = count + 1
                            if (count > 1):
                                return_message = return_message + data['name'] + " "
                        return_message = return_message + ")"
                        if (ac > 30):
                           latency_message = " - "
                           latency_found = 0
                           count = 0
                           for data in x:
                               count = count + 1
                               if (count > 1):
                                   try:
                                       date_time_obj = datetime.datetime.strptime(data['lastRun'], '%Y-%m-%d %H:%M:%S')
                                   except ValueError:
                                       date_time_obj = datetime.datetime.now()
                                   if (calculateTimeDiff(current_time, date_time_obj) > ac):
                                       latency_found  = 1
                                       latency_message = latency_message + " (" + data['name'] + " has latency.) "
                           if (latency_found != 0):
                               return_message = return_message + latency_message
                else:
                    result_message = "OK:"
                    if (api == 'warnings' or api == 'criticals'):
                        result_message = result_message + " No alerts on " + server_name + "\n"
                all_results = all_results + result_message
                if (result_code < return_code):
                    return_code = result_code
            if (len(all_results) > 5):
                return_message = all_results.strip()
        else:
            server_name = json_object[0]['name']
            length = len(json_object)
            result_code = 1
            if (length > 1):
                count = 0
                if (api == 'ok'):
                    errors = 0
                    if (ac > 30):
                        result_message = str(length-1) + " checks are ok on " + server_name
                        for data in json_object:
                            count = count + 1
                            if (count > 1):
                                try:
                                    date_time_obj = datetime.datetime.strptime(data['lastRun'], '%Y-%m-%d %H:%M:%S')
                                except ValueError:
                                    date_time_obj = datetime.datetime.now()
                                    non_active_checks = non_active_checks + 1
                                if (calculateTimeDiff(current_time, date_time_obj) > ac):
                                    result_code = 0
                                    errors += 1
                                    result_message = result_message + " (" + data['name'] + " has latency.) "
                        if (result_code == 1):
                            return_message = "OK " + result_message
                            return_code = result_code
                        else:
                            return_message = "WARNING: " + result_message
                            return_code = result_code
                    else:
                        return_message = "OK: " + str(length-1) + " checks are ok on " + server_name
                        return_code = 1
                    metrics.update({
                        'status': return_code,
                        'errors': errors,
                        'name': server_name,
                        'ok': str(length-1)
                    })
                else:
                    result_message = ""
                    return_code = 1
                    errors = 0
                    return_message = "CRITICAL: " + str(length-1) + " checks on " + server_name + " is not ok ( "
                    if (str(length-1) > 0):
                        return_code = 0
                        errors = lenght-1
                    for data in json_object:
                        count = count +1
                        if (count > 1):
                            return_message = return_message + data['name'] + " "
                            if (ac > 30):
                                try:
                                    date_time_obj = datetime.datetime.strptime(data['lastRun'], '%Y-%m-%d %H:%M:%S')
                                except ValueError:
                                    date_time_obj = datetime.datetime.now()
                                    non_active_checks = non_active_checks + 1
                                if (calculateTimeDiff(current_time, date_time_obj) > ac):
                                    if (len(result_message) < 2):
                                        result_message = " | "
                                    result_message = result_message + " (" + data['name'] + " has latency.) "
                    return_message = return_message + ") " + result_message
                    metrics.update({
                        'status': return_code,
                        'errors': errors,
                        'not_ok': str(length-1),
                        'message': return_message
                    })
            else:
                if (api == 'ok'):
                    return_code = 0
                    return_message = "CRITICAL: No positive monitoring objects found on server " + server_name
                else:
                    return_code = 0
                    return_message = "OK: No " + api + " found on server " + server_name
                metrics.update({
                    'status': return_code,
                    'errors': 1,
                    'message': return_message
                })
            return json.dumps(metrics)
    elif (api == 'howareyou'):
        mserv = 0
        m_standalone = 0
        bad_servers = 0
        try:
            num_servers = len(json_object['server'])
        except TypeError:
            num_servers = 0
        if (num_servers > 1):
            return_code = 1
            result_info = ""
            for data in json_object['server']:
                result_code = int(data[0]['return_code'])
                if (result_code == 0):
                    result_code = 1
                if (result_code < return_code):
                    return_code = result_code
                if (result_code != 1):
                    result_info = result_info + data[0]['name'] + " (has " + str(data[0]['monitor_results']['crit']) + " criticals and " + str(data[0]['monitor_results']['warn']) + " warnings) "
                    bad_servers += 1
            if (return_code == 1):
                return_message = "OK: All " + str(num_servers) + " servers are fine."
                metrics.update({
                    'status': return_code,
                    'errors': 0,
                    'message': return_message,
                    'servers_ok' : num_servers,
                    'servers_not_ok': bad_servers   
                })
            if (return_code != 1):
                return_message = "CRITICAL: " + result_info
                metrics.update({
                    'status': return_code,
                    'errors': 1,
                    'message': return_message,
                    'servers_ok' : num_servers - bad_servers,
                    'servers_not_ok': bad_servers
                })
            return json.dumps(metrics)
        else:
            try:
                return_code = int(json_object[0]['return_code'])
            except:
                mserv = 1
            if (mserv == 0):
                errors = 0
                if (return_code == 0):
                    return_code = 1 
                    return_message = "OK: " + json_object[0]['answer']
                else:
                    return_code = 0
                    errors += 1
                    return_message = "CRITICAL: " + json_object[0]['answer'] + " " + str(json_object[0]['monitor_results']['crit']) + " critical alert(s)."
                metrics.update({
                    'status': return_code,
                    'errors': errors,
                    'message': return_message,
                    'criticals' : str(json_object[0]['monitor_results']['critical']),
                    'warnings': str(json_object[0]['monitor_results']['warning']),
                    'ok': str(json_object[0]['monitor_results']['ok'])
                })
            else:
                errors = 0
                try:
                    return_code = int(json_object['server'][0]['return_code'])
                except:
                    return_code = int(json_object['server'][0][0]['return_code'])
                    m_standalone = 1
                if (m_standalone == 0):
                    answer = json_object['server'][0]['answer']
                    warn = json_object['server'][0]['monitor_results']['warning']
                    crit = json_object['server'][0]['monitor_results']['critical']
                    ok = json_object['server'][0]['monitor_results']['ok']
                if (m_standalone == 1):
                    answer = json_object['server'][0][0]['answer']
                    warn = json_object['server'][0][0]['monitor_results']['warning']
                    crit = json_object['server'][0][0]['monitor_results']['critical']
                    crit = json_object['server'][0][0]['monitor_results']['ok']
                if (return_code == 0):
                    return_code = 1
                    return_message = "OK: " + answer
                else:
                    return_code = 0
                    errors += 1
                    return_message = "CRITICAL: " + answer + " " + str(crit) + " critical alert(s)."
                metrics.update({
                    'status': return_code,
                    'errors': errors,
                    'message': return_message,
                    'criticals': crit,
                    'warnings': warn,
                    'ok': ok
                })
                return json.dumps(metrics)
    elif (api == 'plugin'):
        # Here is work to be done. Not working in multimode
        is_multi_server = False
        try:
            server_name = json_object[0]['name']
        except:
             is_multi_server = True
        if (is_multi_server):
            print (json_object)
            return_message = "Under construction"
            return_code = 3
        else:
            #print (server_name)
            if (len(json_object) == 1):
                return_message = "UNKNOWN: '" + check_name + "' not found."
                metrics.update({
                    'status': 0,
                    'errors': 1,
                    'message': return_message
                })
                return json.dumps(metrics)
            output = json_object[1]['pluginOutput']
            p_metrics = output.partition('|')[2]
            print (p_metrics)
            return_code = int(json_object[1]['pluginStatusCode'])
            if (return_code == 0):
                return_message = "OK: " + output
                metrics.update({
                    'status': 1 ,
                    'errors': 0,
                    'message': return_message
                })
                return json.dumps(metrics)
            else:
                return_message = "CRITICAL: " + output
                metrics.update({
                    'status': 0,
                    'errors': 1,
                    'message': return_message
                })
            return json.dumps(metrics)

    elif (api == 'json'):
        # use this to parse the oldest timestamp?
        return_code = 1
        errors = 0
        return_message = "OK! HowRU is running"
    else:
        return_code = 0
        errors = 1
        return_message = "UNKNOWN! Could not understand API call"
    metrics.update({
        'status': return_code,
        'errors': errors,
        'message': return_message
    })

    return json.dumps(metrics)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Check howru.')
    parser.add_argument('-H', '--host', type=str, required=True, help='Server to query (required)')
    parser.add_argument('-A', '--api', type=str, required=True, help='Api to call (required)')
    parser.add_argument('-i', '--id', type=int, required=False, help='Plugin index for check to collect, put 0 if you pass by name (used by plugin api only)', default=0)
    parser.add_argument('-n', '--name', type=str, required=False, help='Name of service to check (used by plugin api only)', default='None')
    parser.add_argument('-f', '--file', type=str, required=False, help='File from where to read howru outputs (optional)', default='')
    parser.add_argument('-l', '--latencyaccepted', type=int, required=False, help='Latency accepted between Zabbix and HowRU (optional)')
    parser.add_argument('-d', '--sid', type=int, required=False, help='Server id (optional)')
    parser.add_argument('-e', '--servername', type=str, required=False,help='Name of server provided by the API (optional)', default='')
    parser.add_argument('-t', '--timeintervall', type=int, required=False, help='Time intervall used by change api (optional)', default=-1)
    parser.add_argument('-P', '--port', type=int, required=True, default=80, help='Port used by API, default is 80')
    args = parser.parse_args()
    api_server = args.host
    api_call = args.api
    check_id = args.id
    check_name = args.name
    file_name = args.file
    servername = args.servername
    accepted_latency = args.latencyaccepted
    port = args.port
    url = 'http://' + api_server + ":" + str(port) + '/api/' + api_call

    if accepted_latency is None:
        latency = 0         
    else:           
        latency = accepted_latency

    if (check_id == 0):
        if (check_name == 'None'):
            if (api_call == 'plugin'):
                return_code = 0
                print ("No id or name is specified.")
                sys.exit(return_code)
        else:
            url = url + "?name=" + check_name
    else:
        check_id = check_id -1
        url = url + "?id=" + str(check_id)

    if (len(file_name) > 2):
        if (url.find('?') < 0):
            url = url + "?whichjson=" + file_name
        else:
            url = url + "&whichjson=" + file_name

    if (len(servername) > 2):
        if (url.find('?') < 0):
            url = url + "?server="
        else:
            url = url + "&server="
        url = url + servername

    return_code = 1
    print (url)
    try:
        f = urllib.request.urlopen(url)
    except:
        return_message = "UNKNOWN: " + api_call + " not recognized on " + api_server
        err_mes = {
            'status': 1,
            'errors': 1,
            'message': return_message
        }
        print(json.dumps(err_mes))
        sys.exit(3)

    try:
        jblob = f.read().decode('utf-8')
        #print (jblob)
        print(get_metrics(jblob, api_call, latency))
    except Exception as e:
        print(f"Error: {str(e)}")
        sys.exit(1)
