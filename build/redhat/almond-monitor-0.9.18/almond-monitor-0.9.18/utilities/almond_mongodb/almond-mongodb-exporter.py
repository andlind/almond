import pymongo
import os
import schedule
import time
import json
import logging
from datetime import datetime

REQUIRED_KEYS = ['mongodb_uri', 'database', 'collection', 'json_file_path']
last_mtime = None

def validate_config(config):
    missing = [key for key in REQUIRED_KEYS if key not in config or not config[key]]
    if missing:
        raise ValueError(f"Missing or empty config keys: {missing}")
    try:
        test_client = pymongo.MongoClient(config['mongodb_uri'], serverSelectionTimeoutMS=3000)
        test_client.server_info()  # Forces connection
    except Exception as e:
        raise ConnectionError(f"Cannot connect to MongoDB: {e}")
    if not os.path.isfile(config['json_file_path']):
        raise FileNotFoundError(f"JSON file not found: {config['json_file_path']}")

def insert_document(json_file_path, collection, last_mtime):
    try:
        current_mtime = os.path.getmtime(json_file_path)
        if current_mtime == last_mtime:
            logging.info("File unchanged. Skipping insert.")
            return
        last_mtime[0] = current_mtime

        with open(json_file_path, 'r') as f:
            doc = json.load(f)
            doc['ingested_at'] = datetime.utcnow().isoformat()
            collection.insert_one(doc)
            logging.info(f"Inserted document for host: {doc['host']['name']}")
        print(f"Inserted document for host: {doc['host']['name']}")
    except Exception as e:
        logging.error(f"Error inserting document: {e}")
        print(f"Error inserting document: {e}")

def main():
    logging.basicConfig(
        filename='almond_mongodb.log',
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(message)s'
    )
    # Load config
    with open('config.json', 'r') as config_file:
        config = json.load(config_file)
    validate_config(config)
    logging.info(f"Configuration read.")

    client = pymongo.MongoClient(config['mongodb_uri'])
    db = client[config['database']]
    collection = db[config['collection']]
    json_file_path = config['json_file_path']
    last_mtime = [None] 
    
    logging.info(f"Starting scheduled transfer of Almond data to mongodb.")
    schedule.every(1).minutes.do(insert_document, json_file_path, collection, last_mtime)

    while True:
        schedule.run_pending()
        time.sleep(1)

if __main()__ == "__main__":
    main()
