#/usr/bin/python3
# Check array of apis:s for given servername
import sys
import argparse
import subprocess

def main():
    parser = argparse.ArgumentParser(description='Check howru.')
    parser.add_argument('-H', '--hosts', type=str, required=True, default='', help='Array of howru-api to query (required)')
    parser.add_argument('-A', '--api', type=str, required=True, help='Api to call (required)')
    parser.add_argument('-S', '--server', type=str, required=True, help='Server to look for (required)')
    parser.add_argument('-i', '--id', type=int, required=False, help='Plugin index for check to collect, put 0 if you pass by name (used by plugin api only)', default=0)
    parser.add_argument('-n', '--name', type=str, required=False, help='Name of service to check (used by plugin api only)', default='None')
    parser.add_argument('-f', '--file', type=str, required=False, help='File from where to read howru outputs (optional)', default='')
    parser.add_argument('-l', '--latencyaccepted', type=int, required=False, help='Latency accepted between Nagios and HowRU (optional)')
    parser.add_argument('-d', '--sid', type=int, required=False, help='Server id (optional)')
    parser.add_argument('-t', '--timeintervall', type=int, required=False, help='Time intervall used by change api (optional)', default=-1)
    args = parser.parse_args()
    api_servers = [str(item) for item in args.hosts.split(',')]
    api_call = args.api
    check_id = args.id
    check_name = args.name
    file_name = args.file
    servername = args.server
    accepted_latency = args.latencyaccepted
    retVal = 4
    retMes = ""

    for x in api_servers:
        #print (x)
        if ':' in x:
            pos = x.rfind(':')+1
            port = int(x[pos:])
            name = x[:pos-1]
        else:
            port = 80
            name = x

        cmd = "/usr/bin/python3 check_howru.py -H " + name + " -P " + str(port) + " -A " + api_call + " -e " + servername
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
        out, err = p.communicate()
        ret_code = p.wait()
        if (retVal > ret_code):
            retVal = ret_code
            if (retVal < 3):
                retMes = out.decode('utf-8')

    if (retVal == 3):
        print("ERROR: " + servername + " not found on any API server.")
        sys.exit(2)
    else:
        print(retMes)
        sys.exit(retVal)

if __name__== "__main__":
    sys.exit(main())
