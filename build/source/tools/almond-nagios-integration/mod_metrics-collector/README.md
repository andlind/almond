*** THIS IS A MODIFIED VERSION OF METRICS-COLLECTOR ***
*******************************************************

1. BUILD THE PROXY
   - Prerequisites are g++, libjsoncpp-dev, libcurl-dev
   - Just run g++ -Wall main.cpp util.cpp logger.cpp -lcurl -ljsoncpp -o almond-collector

2. EDIT url.conf
   - Add the url(s) to the HowRU API:s you want to get data from, eg

   address_to_howru_api:port/api/showall

3. START almond-collector
   ./almond-collector [port]
