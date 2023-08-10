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
MAKE=/usr/bin/make
echo "Setting up installation"
aclocal
autoreconf
./configure --prefix=/opt/almond 
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
cp start_almond.sh /opt/almond/
cp gardener.py /opt/almond/
echo "Copy plugin files needed."
cp -r plugins /opt/almond/
echo "Setting up log directory"
mkdir -p /var/log/almond
echo "Installation complete"
