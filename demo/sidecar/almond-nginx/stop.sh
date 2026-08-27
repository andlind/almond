#!/bin/bash
kubectl delete pod nginx-almond --ignore-not-found
kubectl delete svc nginx-service almond-api almond-howru --ignore-not-found
kubectl delete configmap nginx-config almond-plugins almond-config almond-supervisor --ignore-not-found
kubectl delete configmap almond-nginx-scripts --ignore-not-found
echo "Pod removed"
echo "Killing possible zoombies"
pkill -f "port-forward pod/nginx-almond"
echo "Remove portforward log file"
rm -f portforward.log
