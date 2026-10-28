* Kubernetes stage 1 (Rancher Desktop)
  - Enable Kubernetes in Rancher Desktop and make sure kubectl uses the rancher-desktop context.
  - ./00_setup_k8s.sh deploys Redpanda and its Console to the redpanda namespace and starts local port-forwards:
    Console http://localhost:8080, Kafka localhost:19092, Schema Registry http://localhost:18081,
    HTTP Proxy http://localhost:18082, Admin API http://localhost:19644.
  - ./00_setup_optional_almond_on_redpanda_k8s.sh waits for Redpanda and Console, then installs Almond into those pods.
    It does not start or restart the other stages. The Redpanda, microservice, and HowRU workload manifests mount the same
    /var/lib/almond-demo/data directory at /opt/almond/data. The runtime Almond installation is lost if Redpanda or Console
    pods are recreated; rerun the script to restore it.
  - ./01_stage1_k8s.sh builds and deploys Redis, the backend, the web app, and a local-access proxy with an Almond monitoring sidecar to the microservice namespace; a service alias there
    keeps the existing Redpanda hostname working, while Redpanda advertises its cross-namespace DNS name to Kafka clients.
    These applications mount the Rancher Desktop node directory /var/lib/almond-demo/data. Kubernetes-specific app overrides
    are kept under k8s/overlays so the original Docker demo application sources remain unchanged. The local proxy forwards
    to the load-balanced webapp Service. Almond reads NGINX's localhost-only stub_status endpoint and pushes selected
    connection and request metrics to HowRU for Prometheus and Nagios; Prometheus does not scrape NGINX directly.
    Metric collection starts once stage 2's HowRU API is available. Stage 1 and stage 7 share the same Almond sidecar image;
    each supplies its own plugin list, hostname, and data-file prefix. Each build uses its immutable image ID as its Kubernetes
    tag so a rebuild cannot accidentally reuse a cached sidecar with the same local tag.
  - ./02_stage2_k8s.sh builds and deploys the HowRU API to the howru-proxy namespace, after applying the stage 1 manifest
    so both namespaces mount /var/lib/almond-demo/data at /opt/almond/data. It restarts port-forwards for stage 1 and starts
    a port-forward for HowRU at http://localhost:8085.
    The directory is inside the Rancher Desktop Kubernetes node, not the macOS host. The previous microservice/almond-data
    PVC is retained but no longer mounted; its contents are not copied automatically and remain available there for manual migration.
  - ./03_stage3_k8s.sh builds and deploys Nagios in the nagios namespace and forwards http://localhost:8180 to the Nagios UI.
    It requires stage 2 and configures the integration to call
    howru-api.howru-proxy.svc.cluster.local:80. Kubernetes namespace names use hyphens, so the existing namespace is
    howru-proxy (not howru_proxy). Nagios plugins are loaded from demo/nagios/plugins into a ConfigMap.
  - ./04_stage4_k8s.sh builds and deploys Prometheus with Almond in the prometheus namespace. The existing Prometheus
    image installs and starts Almond, and shares /opt/almond/data through the Rancher Desktop node hostPath. A root init container
    makes existing files and directories in the shared Almond and Prometheus storage writable by the non-root Prometheus process.
    Prometheus scrapes HowRU through howru-api.howru-proxy.svc.cluster.local:80/metrics and is available at http://localhost:9090.
    Prometheus time-series data is stored separately at /var/lib/almond-demo/prometheus; the permission setup is intended only
    for the local demo.
  - ./05_stage5_k8s.sh builds and deploys Grafana with Almond in the grafana namespace and provisions the Prometheus datasource using
    http://prometheus.prometheus.svc.cluster.local:9090. This is the URL to use in Grafana's Prometheus datasource:
    Grafana reaches Prometheus through Kubernetes Service DNS across namespaces, rather than Docker's http://prometheus:9090
    alias. Grafana is available in your browser at http://localhost:3000 (default credentials: admin/admin; Grafana may
    require a password change on first login). The Almond image uses the bundled Alpine ARM64 package and therefore
    targets linux/arm64.
  - ./06_stage6_k8s.sh adds a second web app pod through its own Deployment in the microservice namespace. Its pod joins
    the existing webapp Service, so in-cluster requests to webapp:8070 (or webapp.microservice.svc.cluster.local:8070 from
    another namespace) can be load-balanced across both app pods. It uses the same shared Almond data directory and
    Redis Service, while its Almond config keeps separate webapp02.json and webapp02.metrics files alongside the first
    instance's webapp01 files. It requires stage 1 and ensures there is one local port-forward to webapp-access on port
    8070 after deployment.
    Open http://localhost:8070 to access the in-cluster proxy, which forwards requests through the webapp Service to either
    pod. A direct kubectl port-forward to a Service selects a single pod, so the proxy is used to retain load balancing
    for local browser requests.
  - ./07_stage7_k8s.sh starts QuickPizza with an Almond sidecar in the quick-pizza namespace, using the same shared image as stage 1.
    The sidecar pushes JSON data
    every 60 seconds to HowRU at howru-api.howru-proxy.svc.cluster.local:80/receive; HowRU listens on port 80 and has
    file receiving enabled. Run stage 2 first so the HowRU Service is available.
  - Stage 1 starts background forwards for webapp port 8070,
    customapp port 8076, Almond API port 9909, and Redis port 6379, then exits.
  - Run stage 2 after stage 1, then stages 3–7 as needed. Open http://localhost:8070 in a browser.
    Use ./07_stop_k8s.sh to remove the demo workloads and stop the forwards.
    It removes the demo namespaces (including redpanda and its PVC/data), clears all contents from the shared Almond data
    directory and Prometheus data directory inside the Kubernetes node, and removes the demo Docker images after Kubernetes
    has finished deleting the workloads. It also removes webapp2's persisted Almond snapshot so Nagios does not briefly
    rediscover that optional instance on the next startup. Image removal is retried briefly if the container runtime is
    still releasing them.
  - To stop only Redpanda and Console while retaining their data, run:
    kubectl delete --ignore-not-found=true -n redpanda service/redpanda-headless service/redpanda statefulset/redpanda service/redpanda-console deployment/redpanda-console
    This command retains the Redpanda data PVC. The full ./07_stop_k8s.sh teardown deletes the redpanda namespace and its PVC,
    including its data.
  - The Redpanda Console can be opened using kubectl port-forward -n redpanda service/redpanda-console 8080:8080.
  - The Docker stage scripts and run_all.sh/stop_all.sh outside k8s remain part of the older Docker-based workflow and are not
    compatible with this Kubernetes setup.

Enjoy :)
