**********************************
** Almond Nagios Integration    **
**********************************

You need to install python3-requests to run this script.

python3 integration.py -u url_to_collector -f path_to_file -c url_to_nagios_command_file -n command_to_check_nagios --reload (if you want to restart nagios) -x command_to_restart_nagios

The first to parameters are required, the others will default to:
-c /opt/nagios/etc/objects/commands.cfg
-n /opt/nagios/bin/nagios -v /opt/nagios/etc/nagios.cfg
-x /etc/rd.d/init.d/nagios reload

--reload will run the command provided by the -x parameter.

