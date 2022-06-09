#!/usr/bin/python

import sys
import os
import json
import argparse


def load_file(json_file):
    global jsonObject
    with open(json_file) as jsonFile:
        jsonObject = json.load(jsonFile)
        jsonFile.close()

def parse_json(check_id, check_name):
    global server_name
    global return_code
    server_name = jsonObject['host']['name']
    if (check_id > 0):
        check_id = check_id - 1
        retObj = jsonObject['monitoring'][check_id]
    else:
        n_id = -1 
        count = 0
        for name in jsonObject['monitoring']:
            if (name['name'] == check_name):
                n_id = count
                break
            else:
                count = count + 1
        if (n_id < 0):
            print "No check named '" + check_name + "' found."
            sys.exit(3)
        retObj = jsonObject['monitoring'][n_id]
    return_code = int(retObj['pluginStatusCode'])
    return retObj['pluginOutput'] + "; HowRU last run: " + retObj['lastRun']

def  main():
    global return_code;
    parser = argparse.ArgumentParser(description='Check howru.')
    parser.add_argument('-i', '--id', type=int, required=True, help='Index for check to collect, put 0 if you pass by name', default=0)
    parser.add_argument('-n', '--name', type=str, required=False, help='Name of service to check', default='None')
    parser.add_argument('-f', '--file', type=str, required=False, help='File from where to read howru outputs', default='/opt/howru/www/monitor_data.json')
    args = parser.parse_args()
    check_id = args.id
    check_name = args.name
    file_name = args.file

    if (check_id == 0):
        if (check_name == 'None'):
            return_code = 3
            print "No id or name is specified."
            sys.exit(return_code)

    if (os.path.isfile(file_name) == False):
        return_code = 3
        print "Could not find howru file '" + file_name + "'."
        sys.exit(return_code)

    load_file(file_name)
    print parse_json(check_id, check_name)

    sys.exit(return_code)

if __name__== "__main__":
    sys.exit(main())
    

    
