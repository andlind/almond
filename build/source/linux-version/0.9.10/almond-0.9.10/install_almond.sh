#!/bin/bash
#  ##     ####     ##   ##   ## ##   ###  ##  ### ##  # 
#   ##     ##       ## ##   ##   ##    ## ##   ##  ## # 
# ## ##    ##      # ### #  ##   ##   # ## #   ##  ## # 
# ##  ##   ##      ## # ##  ##   ##   ## ##    ##  ## # 
# ## ###   ##      ##   ##  ##   ##   ##  ##   ##  ## # 
# ##  ##   ##  ##  ##   ##  ##   ##   ##  ##   ##  ## # 
####  ##  ### ###  ##   ##   ## ##   ###  ##  ### ##  #
#
#     Almond install script @Andreas_li@hotmail.com
#
source /etc/os-release
MAKE=/usr/bin/make
ubuntu=yes
echo "Check distro version"
if [[ "$ID" == "rocky" || "$ID_LIKE" == *"rocky"* ]]; then
	echo "This is Rocky Linux."
	ubuntu=false
    
# Check for Red Hat Enterprise Linux
elif [[ "$ID" == "rhel" || "$ID_LIKE" == *"rhel"* || "$ID_LIKE" == *"redhat"* ]]; then
	echo "This is Red Hat Enterprise Linux."
	ubuntu=false
else
	echo "Detected distro: $ID"
    	echo "Ubuntu compiled Nagios plugins will be used. Make sure they are compatible or you need to install nagios plugins and copy to /opt/almond/plugins."
fi
if ! $ubuntu; then
	echo "Installing Nagios plugins"
	yum install -y nagios-plugins-all python3-psutil
fi
echo "Setting up installation"
aclocal
autoreconf -fi
./configure --enable-avro --prefix=/opt/almond
if [ $? -ne 0 ] 
then
	echo "Error running configuration script"
	exit 2
fi
if test -f "$MAKE"; then
	echo "Running make"
else
	echo "Make is missing. Aborting."
	exit 2
fi
/usr/bin/make install
if [ $? -eq 0 ] 
then
	echo "Almond install successful"
else
	echo "Error running make"
	exit 2
fi
echo "Copy config files."
cp conf/plugins.conf /etc/almond/
cp conf/almond.conf /etc/almond/
cp conf/memalloc.conf /etc/almond/
cp conf/aliases.conf /etc/almond/
cp start_almond.sh /opt/almond/
cp gardener.py /opt/almond/
cp memalloc.alm /opt/almond/
echo "Copy plugin files needed."
if $ubuntu; then
	cp -r plugins /opt/almond/
else
        mkdir -p /opt/almond/plugins
	cp -p plugins/*.sh /opt/almond/plugins
	cp -p plugins/*.py /opt/almond/plugins
        cp -p plugins/*pl /opt/almond/plugins 
	ln -s /usr/lib64/nagios/plugins/* /opt/almond/plugins
fi
echo "Setting up log directory"
mkdir -p /var/log/almond
echo "Setting up command directory"
mkdir /opt/almond/api_cmd
echo "Installation complete"
exit 0
