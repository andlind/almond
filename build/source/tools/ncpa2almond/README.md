*******************************
*****     NCPA2ALMOND     *****
*******************************

1) Compile
   - Untar the source code
         tar xfvz ncpa2almond.tar.gz
   - Compile with
     cd ncpa2almond/build
     cmake ..
     cmake --build .

2) Run the program
   mkdir -p /opt/almond/data
   mkdir -p /var/log/almond
   ncpa2almond ../config.json   

3) IMPORTANT INFORMATION
   Even though the above shows how to run the program on Linux, the source code should run on Windows as well.

4) USAGE
   The name ncpa2almond is a little bit misleading since the product do not implement Almond functions.
   The program runs queries against NCPA, which is a requiement for the program to work.
   The queries are then transformed into Almond JSON output and Prometheus metrics and can be pushed to
   a HowRU proxy server to integrate nicely within the Almond Monitor family.
