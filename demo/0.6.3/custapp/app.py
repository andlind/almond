#!/usr/bin/python3
import json
import random
import time
import os
import redis

global high
global low
global medium
global curval
global numbers
global datafile
global r

def init():
    global high
    global low
    global medium
    global curval
    global total
    global numbers
    global r
    high = 0
    low = 0
    medium = 0
    curval = 0
    total = 0
    numbers = 0
    os.chdir('/custapp/')
    pool = redis.ConnectionPool(host='redis_demo', port=6379, db=0)
    r = redis.Redis(connection_pool=pool)

def writeData():
    data = {}
    data["high"] = high
    data["low"] = low
    data["medium"] = medium
    data["curval"] = curval
    data["count"] = numbers

    with open ("/custapp/data.txt", "w") as data_file:
        data_file.write(json.dumps(data))
    
    r.set("ca_high", high)
    r.set("ca_low", low)
    r.set("ca_medium", medium)
    r.set("ca_curval", curval)
    r.set("ca_count", numbers)     

def calculate():
    global high
    global low
    global medium
    global total
    global curval
    global numbers

    if numbers == 0:
        high = curval
        low = curval
    if curval > high:
        high = curval
    if curval < low:
        low = curval
    total = total + curval
    numbers = numbers + 1
    medium = total / numbers

    writeData()

    if numbers > 15:
        init()
    
def getNumber():
    global curval
    r = random.randint(0,999)
    curval = r
    calculate()
    time.sleep(15)

def main():
    init()
    while (True):
        getNumber()

if __name__ == "__main__":
    main()
