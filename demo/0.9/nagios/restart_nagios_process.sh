#!/bin/sh
# This is a sample shell script showing how you can submit the RESTART_PROGRAM command
# to Nagios. Adjust variables to fit your environment as necessary.

now=`date +%s`
commandfile='/opt/nagios/var/rw/nagios.cmd'

/bin/printf "[%lu] RESTART_PROGRAM\n" $now > $commandfile 
