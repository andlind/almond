CHECK_ALMOND
A Nagios check to make calls against Almond API

Build requirements
You need to install jsoncpp and libcurl headers to compile the source
 
BUILD
g++ -Wall main.cpp -o check_almond -I /usr/include/jsoncpp/ -lcurl -ljsoncpp

