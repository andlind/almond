#!/usr/bin/python3
#
# This plugin is just for a demo
# It is not intended to be used in production
#
import sys
word = "GET"
ok_requests = 0
error_requests = 0
ret_code = 0
with open('/var/log/webapp.log') as fp:
    lines = fp.readlines()
    for line in lines:
        if line.find(word) != -1:
            if line.find('200'):
                ok_requests += 1
            else:
                ret_code = 1
                error_requests += 1
    total_reqs = ok_requests + error_requests
    if (error_requests == 0):
        error_percentage = 0.0
    else:
        error_percentage = float(round(error_requests/total_reqs * 100, 2))
    if (error_percentage > 5):
        ret_code = 2

return_message = ""
if (ret_code == 0):
    return_message = "OK All " + str(total_reqs) + " requests are fine. |"
elif (ret_code == 1):
    return_message = "WARNING There are request errors. | "
else:
    return_message = "CRITICAL " + str(error_percentage) + "% of all requests are bad. | "
metrics = " total=" + str(float(total_reqs)) + " error_percentage="+ str(error_percentage) + " request_errors=" + str(float(error_requests)) + " fine_requests=" + str(float(ok_requests))
return_message = return_message + metrics
print(return_message)
sys.exit(ret_code)
