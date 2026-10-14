from flask import Flask
from flask import render_template
import os
import redis

app = Flask(__name__)
def get_data():
    global high
    global low
    global average
    global current
    pool = redis.ConnectionPool(host=os.environ.get('REDIS_HOST', 'redis_demo'), port=6379, db=0)
    r = redis.Redis(connection_pool=pool)
    high = decode_value(r.get('ca_high'))
    low = decode_value(r.get('ca_low'))
    average = decode_value(r.get('ca_medium'))
    current = decode_value(r.get('ca_curval'))

def decode_value(value):
    return value.decode('utf-8') if value is not None else 'Waiting for data'

@app.route('/')
def show_app_data():
    global high
    global low
    global average
    global current
    get_data()
    return render_template('index.html', current=current, high=high, low=low, average=average)
