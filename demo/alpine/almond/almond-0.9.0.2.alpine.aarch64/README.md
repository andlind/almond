## Almond built from source in Alpine aarch64 environment ##
1. Install dependecies
	- apk add --no-cache sysstat bash python3 py3-psutil procps busybox iputils json-c librdkafka
2. Edit plugins.conf and almond.conf to your needs
3. Test and add additional plugins if needed.
4. Run the install script
5. /opt/almond/start_almond.sh
