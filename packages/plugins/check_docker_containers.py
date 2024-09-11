#!/usr/bin/python3

# check_docker_containers.py:
# Check to check containters running in Docker
# Check needs a list of containers to check: ./check_docker_containers.py container1 container2 container3
# If adding parameter:
# -a Script will check all containers in Docker, omitting the list (which however must be provided)
# Script also have three multiple exclusive arguments, meaning they can not be used together.
# -l Returns a list of all running containers
# -s Returns a list of all stopped containers
# -v Verbose output
#
# Script author: Andreas Lindell <andreas.lindell@almondmonitor.com>
#

import docker
import sys
import argparse

def printf(format, *args):
   sys.stdout.write(format % args)

def getContainers():
   client = docker.from_env()
   containersReturn = []
   containers = client.containers.list(all=True) 
   for container in containers:
      containersReturn.append(container)
   return containersReturn

def isRunning(container_name):
   client = docker.from_env()
   container = client.containers.get(container_name)
   container_state = container.attrs['State']
   container_is_running = container_state['Status'] 
   if container_is_running == 'running':
      return True
   else:
      return False

if __name__ == '__main__':
   running = []
   stopped = []
   not_found = []
   parser = argparse.ArgumentParser(description='Check Docker container state')
   parser.add_argument('-a', '--all', action='store_true', required=False, help='Check all containers ignoring the list')
   parser.add_argument('-v', '--verbose', action='store_true', required=False, help='Verbose output')
   parser.add_argument('-l', '--list', action='store_true', required=False, help='List running containers')
   parser.add_argument('-s', '--stopped', action='store_true', required=False, help='List stopped containers')
   parser.add_argument('container_list', metavar='N', type=str, nargs='+', 
                    help='a list of containers')
   args = parser.parse_args()
   count_total = 0
   count_running = 0
   count_not_running = 0
   count_not_found = 0
   notFound = True
   v_running = " | Running:"
   v_not_found = ", Not found:"
   v_stopped = ", Stopped:"
   retVal = 0
   containers = getContainers()
   for container in containers:
      if (args.all or container.name in args.container_list):
         count_total += 1
         if (isRunning(container.id)):
            count_running += 1
            v_running += " " + container.name  
            running.append(container.name)
         else:
            retVal = 2
            v_stopped += " " + container.name
            count_not_running += 1
            stopped.append(container.name)
   for arg in args.container_list:
      for container in containers:
         if arg == container.name:
            notFound = False
      if notFound == True:
         v_not_found += " " + arg 
         not_found.append(arg)
         count_total += 1
         count_not_found += 1
         retVal = 1
      else:
         notFound = True
   retStr = "OK: "
   if retVal == 1:
      retStr = "WARNING: "
   if retVal == 2:
      retStr = "CRITICAL: "   
   if args.list:
       print(running)
   elif args.stopped:
      print(stopped)
   elif  args.verbose:
      printf(retStr + "Total containers: %i, Running: %i, Stopped: %i, Not found:%i" + v_running + v_stopped + v_not_found + "\n", count_total, count_running, count_not_running, count_not_found)
   else:
      printf (retStr + "Total containers: %i, Running: %i, Stopped: %i, Not found: %i\n", count_total, count_running, count_not_running, count_not_found)
   sys.exit(retVal)
