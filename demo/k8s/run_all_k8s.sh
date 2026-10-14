#!/bin/bash
echo "Starting everything in demo"
./00_setup_k8s.sh
sleep 5
./00_setup_optional_almond_on_redpanda_k8s.sh
sleep 5
./01_stage1_k8s.sh
sleep 5
./02_stage2_k8s.sh
sleep 5
./03_stage3_k8s.sh
sleep 5
./04_stage4_k8s.sh
sleep 5
./05_stage5_k8s.sh
echo "Demo started."
echo "Run ./07_stop_all_k8s.sh to shutdown"
