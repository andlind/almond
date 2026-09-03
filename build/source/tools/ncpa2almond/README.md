*******************************
*****     NCPA2ALMOND     *****
*******************************

1) Compile
   - Untar the source code
         tar xfvz ncpa2almond.tar.gz
   - Compile with
     cd ncpa2almond/
     ./build_all.sh

2) Run the program
   mkdir -p /opt/almond/data
   mkdir -p /var/log/almond
   ncpa2almond config.json   

3) USAGE
   The name ncpa2almond is a little bit misleading since the product do not implement Almond functions.
   The program runs queries against NCPA, which is a requiement for the program to work.
   The queries are then transformed into Almond JSON output and Prometheus metrics and can be pushed to
   a HowRU proxy server to integrate nicely within the Almond Monitor family.
   The primary usage for the program is the opportunity to move Windows Servers to the Almond stack, but
   there could of course be scenarios where you on Linux would like to run ncpa with an Almond integration.

4) VERSION AND NOTES
   The current build is labeled 0.9 and is tested in Linux and emulated Windows. There might be small
   bugg fixes for a 1.0 release, but there are currently no bugs found.
   Future version might also envision a Windows version more linked to Windows SDK for collector
   capabilities found in Almond but not implemented in ncpa2almond. Future builds however is not thought
   of having the API capabilities of Almond.
