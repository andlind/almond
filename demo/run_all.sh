#!/bin/bash
echo "Starting everything in demo"
./00_setup.sh
./00_setup_optional_almond_on_redpanda.sh
./01_stage1.sh
./02_stage2.sh
./03_stage3.sh
./04_stage4.sh
./05_stage5_alternative_with_almond.sh
echo "Demo started."
echo "Run ./stop_all.sh to shutdown"
