#!/bin/bash
export FLASK_APP=app.py
/usr/bin/nohup /usr/bin/flask run --host=0.0.0.0 --port=80 
