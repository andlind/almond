#!/usr/bin/python

# TODO:
# Latency check on plugin data
# File to use whichjson in file mode

import sys
import os
import json
import argparse
import datetime
import urllib.request
import urllib.parse

def calculateTimeDiff(now, then):
    duration = now - then
    return duration.total_seconds()

def parse_json(jsonObject, api, ac):
    global return_code
    global return_message
    global check_name
    try:
    	json_object = json.loads(jsonObject)
    except json.decoder.JSONDecodeError:
        return_message = "UNKNOWN: Could not load json. Wrong parameters?"
        return_code = 3
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
            result_code = 0
            return_code = 0
            count = 0
            for x in json_object:
                server_name = x[0]['name']
                length = len(x)
                if (length > 1):
                    count = 0
                    if (api == 'ok'):
                        if (ac < 30):
                            result_message = "OK: " + str(length-1) + " checks are ok on " + server_name + "\n"
                            result_code = 0
                        else:
                            # check latency
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
                                        result_code = 1
                                        result_message = result_message + " (" + data['name'] + " has latency.) "
                            if (return_code == 0):
                                result_message = "OK: " + result_message + "\n"
                            else:
                                result_message = "WARNING: " + result_message + "\n"
                    else:
                        if (api == 'criticals'):
                            return_code = 2
                        else:
                            return_code = 1
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
                all_results = all_results + result_message
                if (result_code > return_code):
                    return_code = result_code
            if (len(all_results) > 5):
                return_message = all_results.strip()
        else:
            server_name = json_object[0]['name']
            length = len(json_object)
            if (length > 1):
                count = 0
                if (api == 'ok'):
                    return_message = "OK: " + str(length-1) + " checks are ok on " + server_name
                    return_code = 0
                else:
                    return_code = 2
                    return_message = "CRITICAL: " + str(length-1) + " checks on " + server_name + " is not ok ( "
                    for data in json_object:
                        count = count +1
                        if (count > 1):
                            return_message = return_message + data['name'] + " "
                    return_message = return_message + ")"
            else:
                if (api == 'ok'):
                    return_code = 2
                    return_message = "CRITICAL: No positive monitoring objects found on server " + server_name
                else:
                    return_code = 0
                    return_message = "OK: No " + api + " found on server " + server_name

    elif (api == 'howareyou'):
        try:
            num_servers = len(json_object['server'])
        except TypeError:
            num_servers = 0
        if (num_servers > 0):
            return_code = 0
            result_info = ""
            for data in json_object['server']:
                result_code = int(data[0]['return_code'])
                if (result_code > return_code):
                    return_code = result_code
                if (result_code > 0):
                    result_info = result_info + data[0]['name'] + " (has " + str(data[0]['monitor_results']['crit']) + " criticals and " + str(data[0]['monitor_results']['warn']) + " warnings) "
            if (return_code == 0):
                return_message = "OK: All " + str(num_servers) + " servers are fine."
            if (return_code == 1):
                return_message = "WARNING: " + result_info
            if (return_code > 1):
                return_message = "CRITICAL: " + result_info
        else:
            return_code = int(json_object[0]['return_code'])
            if (return_code == 0):
                return_message = "OK: " + json_object[0]['answer']
            elif (return_code == 1):
                return_message = "WARNING: " + json_object[0]['answer'] + " " + str(json_object[0]['monitor_results']['warn']) + " warning(s)." 
            elif (return_code == 2):
                return_message = "CRITICAL: " + json_object[0]['answer'] + " " + str(json_object[0]['monitor_results']['crit']) + " critical alert(s)."

    elif (api == 'changes'):
        return_code = 3
        return_message = "INFO: This api call is not implemented yet for this service check."

    elif (api == 'plugin'):
        # Here is work to be done. Not working in multimode
        server_name = json_object[0]['name']
        if (len(json_object) == 1):
            return_code = 3
            return_message = "UNKNOWN: '" + check_name + "' not found."
            return
        output = json_object[1]['pluginOutput']
        return_code = int(json_object[1]['pluginStatusCode'])
        if (return_code == 0):
            return_message = "OK: " + output
        elif (return_code == 1):
            return_message = "WARNING: " + output
        elif (return_code == 2):
            return_message = "CRITICAL: " + output
        else:
            return_code = "UNKNOWN: " + output

    elif (api == 'json'):
        # use this to parse the oldest timestamp?
        return_code = 0
        return_message = "OK! HowRU is running"
    else:
        return_code = 3
        return_message = "UNKNOWN! Could not understand API call"
   
def main():
    global return_code
    global return_message
    global check_name
    parser = argparse.ArgumentParser(description='Check howru.')
    parser.add_argument('-s', '--server', type=str, required=True, help='Server to query')
    parser.add_argument('-a', '--api', type=str, required=True, help='Api to call')
    parser.add_argument('-i', '--id', type=int, required=False, help='Index for check to collect, put 0 if you pass by name', default=0)
    parser.add_argument('-n', '--name', type=str, required=False, help='Name of service to check', default='None')
    parser.add_argument('-f', '--file', type=str, required=False, help='File from where to read howru outputs', default='monitor_data')
    parser.add_argument('-l', '--latencyaccepted', type=int, required=False, help='Latency accepted between Nagios and HowRU')
    args = parser.parse_args()
    api_server = args.server
    api_call = args.api
    check_id = args.id
    check_name = args.name
    file_name = args.file
    accepted_latency = args.latencyaccepted
    
    if accepted_latency is None:
        latency = 0
    else:
        latency = accepted_latency

    url = 'http://' + api_server + '/howru/monitoring/' + api_call
    
    if (check_id == 0):
        if (check_name == 'None'):
            if (api_call == 'plugin'):
                return_code = 3
                print ("No id or name is specified.")
                sys.exit(return_code)
        else:
            url = url + "?name=" + check_name
    else:
        url = url + "?id=" + str(check_id)

    return_code = 0
    try:
    	f = urllib.request.urlopen(url)
    except:
        return_code = 3
        return_message = "UNKNOWN: " + api_call + " not recognized on " + api_server

    if (return_code == 0):
    	jblob = f.read().decode('utf-8')
    	parse_json(jblob, api_call, latency) 

    print (return_message)
    sys.exit(return_code)

if __name__== "__main__":
    sys.exit(main())
