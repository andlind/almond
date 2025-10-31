mport requests
import json
import urllib.parse
from typing import Dict, List, Optional
from urllib.parse import urljoin
from logger_config import get_logger
from zabbix_client import ZabbixAPIClient, ZabbixSyncError
from howru_client import HowruAPIClient

logger = get_logger()
