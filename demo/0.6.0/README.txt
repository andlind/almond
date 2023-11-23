* Prereqs for running demo
  - Docker (Rancher Desktop)

* To run demo make sure you create a directory named almond_demo with a subfolder named data in your home catalogue.
  Run the following commands:
  cd ~
  mkdir -p almond_data/data

* Finally start the demo stages running the shell scripts
  - stage1	starts a application sceleton with an app, a database and a webgui.
		Each component also run Almond to monitor itself.
                The app component also runs a local Almond Admin on local port 80.
  - stage2	This stage starts a howru-api exposing monitoring data and metrics 
                from all the containers in stage1, on local port 8080
  - stage3	This stage starts a container running Nagios on local port 8180.
                The Nagios installation have config to show data from all containers
                started above,
  - stage4	This stage starts a Prometheus instance that scrapes data from the
                API started in stage2 on port 9090
  - stage5	This stage starts a Grafana server which can be connected to the 
                Prometheus server started in stage4. Grafana runs on local port 3000.
                NOTE! The Grafana server has no prebuilt dashboards and config. You
                need to add the Prometheus instance started in stage4 as a source:
                http://prometheus:9090
                After adding the source you can play and build own dashboards and
                panels with the metrics coming from the HowRU API.

* You could also run everything at once with
  - start.sh

* When finished exploring the demo you can run stop to stop all containers and deleting them.
  - stop.sh
