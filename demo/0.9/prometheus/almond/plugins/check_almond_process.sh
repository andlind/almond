#!/bin/bash

# Run your command and store the count in a variable
COUNT=$(pgrep -f almond | wc -l)

if [ "$COUNT" -gt 0 ]; then
    echo "OK - $COUNT almond processes running."
    exit 0
else
    echo "CRITICAL - No almond processes found!"
    exit 2
fi
