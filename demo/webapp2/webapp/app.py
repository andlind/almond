from flask import Flask
from flask import render_template
import redis

app = Flask(__name__)
def get_data():
    global high
    global low
    global average
    global current
    pool = redis.ConnectionPool(host='redis_demo', port=6379, db=0)
    r = redis.Redis(connection_pool=pool)
    high = r.get('ca_high').decode('utf-8')
    low = r.get('ca_low').decode('utf-8')
    average = r.get('ca_medium').decode('utf-8')
    current = r.get('ca_curval').decode('utf-8')

@app.route('/')
def show_app_data():
    global high
    global low
    global average
    global current
    get_data()
    return render_template('index.html', current=current, high=high, low=low, average=average)
