import glob
import random
import socket
import logging
import json
import flask
import time
import threading
import subprocess
import os, os.path
from venv import logger
from howrug import app

enable_ssl = False
enable_mods = False
ssl_certificate="/opt/almond/www/api/certificate.pem"
ssl_key="/opt/almond/www/api/certificate.key"
almond_conf_file="/etc/almond/almond.conf"
howru_conf_file="/etc/almond/api.conf"
sleep_time = 10
mods_list = []

current_version = "0.9.7 Gunicorn"

def useCertificate():
    global enable_ssl
    return enable_ssl

def getCertificates():
    global ssl_certificate, ssl_key
    ret_val = "('" + ssl_certificate + "', '" + ssl_key + "')"
    return eval(ret_val)

def load_conf():
    global enable_ssl, ssl_certificate, ssl_key, enable_mods, mods_list
    config = {}
    if os.path.isfile('/etc/almond/api.conf'):
        with open("/etc/almond/api.conf", "r") as conf:
            for line in conf:
                key, value = parse_line(line)
                config[key] = value
    else:
        try:
           with open("/etc/almond/almond.conf", "r") as conf:
               for line in conf:
                   key, value = parse_line(line)
                   config[key] = value
        except OSError:
            logger.error("Could not load configuration file. It does not seem to exist!")
            print ("Could not open configutation file.")
            return 80
   
    bindPort = int(config.get('api.bindPort', 80))
    enable_ssl = bool(int(config.get('api.enableSSL', 0)))
    enable_mods = bool(int(config.get('api.enableMods', 0)))
    mods_list = config.get('api.activeMods', '').split(',')
    if config.get('api.sslCertificate') is not None:
        ssl_certificate = config.get('api.sslCertificate', '/opt/almond/www/api/certificate.pem')
    if config.get('api.sslKey') is not None:
        ssl_key = config.get('api.sslKey', '/opt/almond/www/api/certificate.key')

    return bindPort

def check_config():
    """Background thread to monitor the configuration file."""
    global stop_background_thread
    global almond_conf_file
    global howru_conf_file
    global sleep_time
    global logger
    use_api_conf = False
    logger.info("[Check_conf_thread] Starting...")
    howru_last_modified = 0
    threshold = 5
    almond_last_modified = os.path.getmtime(almond_conf_file)
    if (os.path.isfile(howru_conf_file)):
        use_api_conf = True
        howru_last_modified = os.path.getmtime(howru_conf_file)
    while not stop_background_thread:
        change_detected = False
        logger.info("[Check_conf_thread] Check for configuration changes in almond.conf")
        try:
            current_modified = os.path.getmtime(almond_conf_file)
            if abs(current_modified != almond_last_modified) > threshold:
                print("Config change detected. Reload config.")
                logger.info("Change of configuration detected.")
                change_detected = True
                almond_last_modified = current_modified
        except Exception as e:
            logger.error(f"Failed to access {almond_conf_file}: {e}")

        if use_api_conf:
            logger.info("[Check_conf_thread] Check for configuration changes in api.conf")
            try:
                current_modified = os.path.getmtime(howru_conf_file)
                if abs(current_modified != howru_last_modified) > threshold:
                    logger.info("[Check_conf_thread] Change in api.conf detected")
                    change_detected = True
                    howru_last_modified = current_modified
            except:
                logger.error(f"Failed to access {howru_conf_file}: {e}")

        if change_detected:
            logger.info("[Check_conf_thread] Reload configurations")
            change_detected = False
        time.sleep(sleep_time)

def run_mods():
    global stop_background_thread
    global logger
    global sleep_time
    global mods_list

    mods_dir = '/opt/almond/www/api/mods/enabled'

    while not stop_background_thread:
        for x in mods_list:
            c_file = mods_dir + x
            if (os.path.isfile(c_file)):
                logger.info('Found mod ' + x)
                try:
                    mod = subprocess.Popen(c_file, stdout=subprocess.PIPE)
                    output, _ = mod.communicate()
                    rc = mod.returncode
                    if rc != 0:
                        logger.critical(f'Mod {x} returned error: {output.decode()}.')
                    else:
                        logger.info(f'Mod {x} has run successfully.')

                except FileNotFoundError:
                    logger.error('Could not find mod{x}.')
                except Exception as e:
                    logger.exception('An error occurred while running mod {x}: {str(e)}')
        time.sleep(sleep_time)

def gc_main():
    global enable_ssl, enable_mods, ssl_certificate, ssl_key, current_version
    logging.basicConfig(filename='/var/log/almond/howru.log', filemode='a', format='%(asctime)s | %(levelname)s: %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    handler = logging.FileHandler('/var/log/almond/howru.log')
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    handler.setFormatter(formatter)
    app.logger.addHandler(handler)
    app.logger.setLevel(logging.DEBUG)
    app.logger.info('Starting howru api (version:' + current_version + ')')
    use_port = load_conf()
    app.logger.info("Configuration read.")
    use_ssl = useCertificate()
    context = getCertificates()
    tCheck = threading.Thread(target=check_config, daemon=True)
    tCheck.start()
    if enable_mods:
        logger.info('Mods are enabled.')
        logger.info('Looking for mods to be executed.')
        tMods = threading.Thread(target=run_mods, daemon=True)
        tMods.start()
    try:
        while True:
            if (app_started == False):
                app.logger.info("Starting application")
                if (use_ssl):
                    app.logger.info("Running application in ssl_context")
                    app.run(host='0.0.0.0', port=use_port, ssl_context=context, threaded=True)
                else:
                    app.logger.info("Running application without encryption")
                    app.run(host='0.0.0.0', port=use_port)
                app_started = True
            time.sleep(sleep_time)
    except (KeyboardInterrupt,SystemExit):
        app.logger.info("Caught info to stop program")
        stop_background_thread = True
        app.logger.info("Stopping thread checking for configurations changes.")
        time.sleep(1)
        app.logger.info("Main thread exits now. Goodbye :)")
    app.run()

if __name__ == "__main__":
    gc_main()
