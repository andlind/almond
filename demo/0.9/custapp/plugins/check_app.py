#!/usr/bin/python3
# Read data file and get "metrics"
import json
import time
import os
import stat
import argparse

global data_file

def file_age_in_seconds(fname):
    return time.time() - os.stat(fname)[stat.ST_MTIME]

def main():
    global data_file
    parser = argparse.ArgumentParser(description='Arguments for app check')
    parser.add_argument('--file', type=str, help='File where data is stored')
    parser.add_argument('--warning', type=int, help='File age warning')
    parser.add_argument('--critical', type=int, help='File age critical')
    args = parser.parse_args()


    data_file = args.file
    f = open(args.file)
    data = json.load(f)
    f.close()

    age = int(file_age_in_seconds(data_file))
    max = data["high"]
    min = data["low"]
    average = data["medium"]
    current = data["curval"]
    num = data["count"]
    percent = num/15 * 100
    retVal = 0
    retStr = ""

    if age > args.critical:
        retVal = 2
        retStr = "CRITICAL: App data is too old |"
    elif age > args.warning:
        retVal = 1
        retStr = "WARNING: App data is delayed |"
    else:
        retStr = "OK: App is producing data |"

    metrics = " average=" + str(round(average, 2)) + "; max=" + str(max) + "; min=" + str(min) + "; current=" + str(current) + "; percentage=" + str(round(percent, 2)) +"%; age=" + str(age)

    retStr = retStr + metrics
    print (retStr)
    return retVal
    
if __name__ == "__main__":
    main()
