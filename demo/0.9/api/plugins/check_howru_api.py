#!/usr/bin/python3
import subprocess

p = subprocess.Popen("supervisorctl status howru", stdout=subprocess.PIPE, shell=True)
(output, err) = p.communicate()
exit_code = p.wait()

ret_str = output.decode("utf-8").strip()
ret_data = ret_str.split()
if exit_code == 0:
    returstr = "OK: "
    returstr = returstr + ret_data[0] + ' ' + ret_data[1] + " | " + ret_data[2] + "=" + ret_data[3][:-1] + " " + ret_data[4] + "=" + ret_data[5]
else:
    returstr = "CRITICAL: " + ret_data[0] + ' ' + ret_data[1] + " Down since: " + ret_data[2] + ' ' + ret_data[3] + ' ' + ret_data[4] + ' ' + ret_data[5]
    returstr = returstr + " | pid=-1 uptime=0:00:00"
print(returstr)
if (exit_code > 0):
    exit(2)
else:
    exit(exit_code)
