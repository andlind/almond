#!/usr/bin/python3

#### Author : Taha Ali
#### Created: 09/01/2015
#### Contact: tahazohair@gmail.com
###
##
# Summary: this is script or program calculates how much total memory is available.
# it also takes in account for buffers and Caches. Defaults are set at 85% for warning and 90% for critical
##
###
#### Modified by: Andreas Lindell
#### Changed: 04/02/2023
#### Contact: Andreas_li@hotmail.com
###
##
# Change: Modified for python3 and return value modified
##
###

import sys
import optparse

__author__ = 'bindas'

# nagios exit status
OK = 0
WARN = 1
CRIT = 2
UNKNOWN = 3

# Parameters optioning
parser = optparse.OptionParser(
    usage="%prog -c <critical_value> -w <warning_value> \nAuthor:\t\tTaha Ali \nContact:\ttahazohair@gmail.com",
    prog='check_mem.py',
    version='1.0',
)
parser.add_option('-c', '--critical',
                  dest="critical",
                  default="90",
                  type="float",
                  help="critical value in percentage, Default set at 90%",
                  )
parser.add_option('-w', '--warning',
                  dest="warning",
                  default="85",
                  type="float",
                  help="warning value in percentage, Default set at 85%",
                  )
(options, args) = parser.parse_args()

# get data from parameters optioning and feed it into warning and critical variables
critical = options.critical
warning = options.warning

with open('/proc/meminfo', 'r') as memfile:
    entry = {}
    for line in memfile:
        line = line.strip()
        line = line.replace('kB', '')
        if not line: break
        name, value = map(str.strip, line.split(':'))
        entry[name] = value

# get and calculate free memory value
free = int(entry['MemFree']) + int(entry['Buffers']) + int(entry['Cached'])
# get total memory value
total = int(entry['MemTotal'])


def mem_load():
    # change to percentage
    use_percent = 100 - ((free * 100) / total)
    if use_percent >= critical:
        print ("WARNING: Memory (ram) is critical. Memory used is {:.2f}% | memory_used={:.5f};{:.0f};{:.0f}".format(use_percent,use_percent,warning,critical))
        sys.exit(CRIT)
    elif warning <= use_percent < critical:
        print ("WARNING: Memory (ram) is {:.2f}% | memory_used={:.5f};{:.0f};{:.0f}".format(use_percent,use_percent,warning,critical))
        sys.exit(WARN)
    elif use_percent < warning:
        #print ("OK: memory is only", use_percent, '%', "used | memory_used=",use_percent,';;')
        print ("OK: Memory used is {:.2f}% | memory_used={:.5f};{:.0f};{:.0f}".format(use_percent,use_percent,warning,critical))
        sys.exit(OK)
    else:
        print ("UNKNOWN: unable to process free memory\n", use_percent)
        sys.exit(UNKNOWN)


mem_load()
