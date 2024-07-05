* Prereqs for running demo
  - Docker (Rancher Desktop)

* To run demo make sure you create a directory named almond_demo with a subfolder named data in your home catalogue.
  Run the following commands:
  cd ~
  mkdir -p almond_demo/data

* Finally start the demo stages running the shell scripts
  - 00_setup					Starts Redpanda and Redpanda Console (port 8080) for showing Almond Kafka producer option
  - 00_setup_optional_almond_on_redpanda	This script installs Almond on the Redpanda containers. Optional to run.
  - 01_stage1					Starts a demo application with a simple backend, a frontend web app (port 8075) and Redis used as "middleware"
						The backend (customapp_demo) run Almond on port 8076
  - 02_stage2					Starts a HowRU API to connect all sbove containers running on port 8180
  - 03_stage3					Starts Nagios Core runnnin on port 8180
  - 04_stage4					Starts Prometheus running on port 9090
  - 05_stage5_alternative_with_almond		Starts Grafana on port 3000. This option also installs Almond on the Grafana container.
  - 05_stage4_alternative_without_almond	As above but without Almond. Note! You use either of the script starting 05_. Do not execute both.
  - 06_stop					This script stops all containers started in the above script.

* Alternatively you run ./start_all.sh to run all the scripts at once to get the complete demo running.
  ./stop_all.sh will stop the demo from running.

Enjoy :)
