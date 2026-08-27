#############################################
## Sample script consume Almond Kafka data ##
#############################################
try:
    from kafka import KafkaConsumer
except ImportError:
    print("Missing module 'kafka'. Please install it before running the script.")
    exit(1)
try:
    import json
except ImportError:
    print("Missing module 'json'. Please install it before running this script.")
    exit(1)
try:
    import lz4
except ImportError:
    print("Missing module 'lz4'. Please install it before running this script.")
    exit(1)

red = "\033[31m"
green = "\033[32m"
black = "\033[0;30m"
yellow = "\033[1;33m"
purple = "\033[0;35m"

consumer = KafkaConsumer(
   bootstrap_servers='localhost:19092',
   value_deserializer = lambda v: json.loads(v.decode('utf-8'), strict=False),
   auto_offset_reset = 'earliest'
)
consumer.subscribe(topics='almond_monitoring')
for message in consumer:
   value = message.value
   #print("Value is of type:", type(value))
   #for item in value.items():
   #   print (item)
   server = value.get("name")
   tag = value.get("tag")
   id = value.get("id")
   if (server == None):
      server = "Unknown"
   if (tag == None):
      tag = "None"
   if (id == None):
       id = "-1"
   data = value.get("data")
   plugin = data.get("pluginName")
   status = data.get("pluginStatus")
   # Ugly print
   if (server == "app01.demo.com"):
      server = "app01.demo.com\t"
   if (status == "OK"):
   	print(green + status + "     " + black + "\t\tServer: " + server + "\tTag: " + tag +" \tId: " + id + "\tOffset:" + str(message.offset) + "\tPlugin: " + plugin) 
   elif (status == "WARNING"):
        print(yellow + status + " " + black + "\tServer: " + server + "\tTag: " + tag +" \tId: " + id + "\tOffset:" + str(message.offset) + "\tPlugin: " + plugin)
   elif (status == "UNKNOWN"):
        print(purple + status + "\t" + black + "\tServer: " + server + "\tTag: " + tag +" \tId: " + id + "\tOffset:" + str(message.offset) + "\tPlugin: " + plugin)
   else:
        print(red + status + black + "\tServer: " + server + "\tTag: " + tag +" \tId: " + id + "\tOffset:" + str(message.offset) + "\tPlugin: " + plugin)
