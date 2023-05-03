## DedicatedServiceMonitors in Red Hat OpenShift Monitoring

### Introduction

By default, OpenShift's Prometheus stack will pull pod CPU and memory usage from kubelet's `/metrics/cadvisor` endpoint, ignoring the kubelet provided timestamps.
Prometheus can be configured to export another `ServiceMonitor` which will read from kubelet's `/metrics/resource` endpoint, in addition to the default metrics that it is colleting.
This configuration knob / feature is called `DedicatedServiceMonitors`. In addition to exposing these new metrics via Prometheus, the feature will also reconfigure the Prometheus Adapter which will use and expose
the new values instead of the default ones. Therefore, anything using the PodMetrics API will use the metrics exposed by `/metrics/resource` instead of the default cadvisor metrics. As an example, `oc adm top pod`
or the HorizontalPodAutoscaler will use this.

The feature can be enabled by modifying and/or creating the `openshift-monitoring/cluster-monitoring-config` ConfigMap:

~~~
$ cat cm.yaml 
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-monitoring-config
  namespace: openshift-monitoring
data:
  config.yaml: |
(...)
    k8sPrometheusAdapter:
      dedicatedServiceMonitors:
        enabled: true
(...)
~~~

Here is the documentation for this option:

~~~
When enabled is set to true, the Cluster Monitoring Operator (CMO) deploys a dedicated Service Monitor that exposes the kubelet /metrics/resource endpoint. This Service Monitor sets honorTimestamps: true and only keeps metrics that are relevant for the pod resource queries of Prometheus Adapter. Additionally, Prometheus Adapter is configured to use these dedicated metrics. Overall, this feature improves the consistency of Prometheus Adapter-based CPU usage measurements used by, for example, the oc adm top pod command or the Horizontal Pod Autoscaler.
~~~
> [https://docs.openshift.com/container-platform/4.12/monitoring/config-map-reference-for-the-cluster-monitoring-operator.html#dedicatedservicemonitors](https://docs.openshift.com/container-platform/4.12/monitoring/config-map-reference-for-the-cluster-monitoring-operator.html#dedicatedservicemonitors)

And `honor_timestamps` is described here:

~~~
# honor_timestamps controls whether Prometheus respects the timestamps present
# in scraped data.
#
# If honor_timestamps is set to "true", the timestamps of the metrics exposed
# by the target will be used.
#
# If honor_timestamps is set to "false", the timestamps of the metrics exposed
# by the target will be ignored.
[ honor_timestamps: <boolean> | default = true ]
~~~
> [https://prometheus.io/docs/prometheus/latest/configuration/configuration/#scrape_config](https://prometheus.io/docs/prometheus/latest/configuration/configuration/#scrape_config)

More info about `ServiceMonitors` can be found here: [https://github.com/prometheus-operator/prometheus-operator/blob/main/Documentation/user-guides/getting-started.md](https://github.com/prometheus-operator/prometheus-operator/blob/main/Documentation/user-guides/getting-started.md)

### Comparison to default settings

Let's begin with a quick look at the default settings. You can use this section to come back to if you want to compare
the `DedicatedServiceMonitors` feature to the defaults.

Prometheus normally only exposes pod CPU and memory usage via Kubelet's `/metrics/cadvisor` endpoint:

~~~
- job_name: serviceMonitor/openshift-monitoring/kubelet/1
  honor_labels: true
  honor_timestamps: false
  kubernetes_sd_configs:
  - role: endpoints
    namespaces:
      names:
      - kube-system
  scrape_interval: 1m0s
  scrape_timeout: 30s
  metrics_path: /metrics/cadvisor
(...)
~~~

We can manually query those metrics via:

~~~
$ oc get ep -n kube-system
NAME      ENDPOINTS                                                    AGE
kubelet   192.168.18.18:10250,192.168.18.18:10255,192.168.18.18:4194   36h
~~~

~~~
$ TOKEN=$(kubectl get secrets -n openshift-cluster-version -o jsonpath="{.items[?(@.metadata.annotations['kubernetes\.io/service-account\.name']=='default')].data.token}" | base64 --decode)
$ curl -k -H "Authorization: Bearer $TOKEN" https://192.168.18.18:10250/metrics/cadvisor 2>/dev/null | grep container_cpu_usage_seconds_total | grep cluster-autoscaler-operator-647cbf4d9d-cljtq | head -1
container_cpu_usage_seconds_total{container="",cpu="total",id="/kubepods.slice/kubepods-burstable.slice/kubepods-burstable-pod1b2c56f1_ff15_4ab7_8ae8_7161cdfb59d4.slice",image="",name="",namespace="openshift-machine-api",pod="cluster-autoscaler-operator-647cbf4d9d-cljtq"} 113.00285771 1683109361193
~~~

The default configuration for the Prometheus Adapter is:

~~~
sh-4.4$ cat /etc/adapter/config.yaml
"resourceRules":
  "cpu":
    "containerLabel": "container"
    "containerQuery": |
      sum by (<<.GroupBy>>) (
        irate (
            container_cpu_usage_seconds_total{<<.LabelMatchers>>,container!="",pod!=""}[4m]
        )
      )
    "nodeQuery": |
      sum by (<<.GroupBy>>) (
        1 - irate(
          node_cpu_seconds_total{mode="idle"}[4m]
        )
        * on(namespace, pod) group_left(node) (
          node_namespace_pod:kube_pod_info:{<<.LabelMatchers>>}
        )
      )
      or sum by (<<.GroupBy>>) (
        1 - irate(
          windows_cpu_time_total{mode="idle", job="windows-exporter",<<.LabelMatchers>>}[4m]
        )
      )
    "resources":
      "overrides":
        "namespace":
          "resource": "namespace"
        "node":
          "resource": "node"
        "pod":
          "resource": "pod"
  "memory":
    "containerLabel": "container"
    "containerQuery": |
      sum by (<<.GroupBy>>) (
        container_memory_working_set_bytes{<<.LabelMatchers>>,container!="",pod!=""}
      )
    "nodeQuery": |
      sum by (<<.GroupBy>>) (
        node_memory_MemTotal_bytes{job="node-exporter",<<.LabelMatchers>>}
        -
        node_memory_MemAvailable_bytes{job="node-exporter",<<.LabelMatchers>>}
      )
      or sum by (<<.GroupBy>>) (
        windows_cs_physical_memory_bytes{job="windows-exporter",<<.LabelMatchers>>}
        -
        windows_memory_available_bytes{job="windows-exporter",<<.LabelMatchers>>}
      )
    "resources":
      "overrides":
        "instance":
          "resource": "node"
        "namespace":
          "resource": "namespace"
        "pod":
          "resource": "pod"
  "window": "5m"
~~~

Therefore, anything (such as PodMetrics and the HPA) querying the Prometheus Adapter will actually read `container_cpu_usage_seconds_total`
and `container_memory_working_set_bytes` metrics from Prometheus via the adapter.

### Enabling DedicatedServiceMonitors in Red Hat OpenShift Monitoring

The feature can be enabled by modifying and/or creating the `openshift-monitoring/cluster-monitoring-config` ConfigMap:

~~~
$ cat cm.yaml 
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-monitoring-config
  namespace: openshift-monitoring
data:
  config.yaml: |
(...)
    k8sPrometheusAdapter:
      dedicatedServiceMonitors:
        enabled: true
(...)
~~~

After enabling `DedicatedServiceMonitors`, the Prometheus Adapters will restart and Prometheus will be reconfigured. Metrics from Kubelet's `/metrics/resource` endpoint will be exposed via the Prometheus Adapter instead of the default cadvisor metrics.


### New data source: Kubelet `/metrics/resource` CPU and memory usage

As an example and for reference, here is some raw data from the `/metrics/resource` endpoint of kubelet. This is the data that shall be exposed by Prometheus Adapter:

~~~
$ TOKEN=$(kubectl get secrets -n openshift-cluster-version -o jsonpath="{.items[?(@.metadata.annotations['kubernetes\.io/service-account\.name']=='default')].data.token}" | base64 --decode)
$ curl -k -H "Authorization: Bearer $TOKEN" https://192.168.18.18:10250/metrics/resource 2>/dev/null | grep cpu | grep container | head
# HELP container_cpu_usage_seconds_total [ALPHA] Cumulative cpu time consumed by the container in core-seconds
# TYPE container_cpu_usage_seconds_total counter
container_cpu_usage_seconds_total{container="alertmanager",namespace="openshift-monitoring",pod="alertmanager-main-0"} 15.375475724 1683108596685
container_cpu_usage_seconds_total{container="alertmanager-proxy",namespace="openshift-monitoring",pod="alertmanager-main-0"} 89.31764639 1683108603252
container_cpu_usage_seconds_total{container="authentication-operator",namespace="openshift-authentication-operator",pod="authentication-operator-65f78f5bc6-kgvzl"} 1623.985942455 1683108603509
container_cpu_usage_seconds_total{container="baremetal-kube-rbac-proxy",namespace="openshift-machine-api",pod="cluster-baremetal-operator-856f996786-dndrv"} 3.205377967 1683108585181
container_cpu_usage_seconds_total{container="catalog-operator",namespace="openshift-operator-lifecycle-manager",pod="catalog-operator-756ccdbd48-srjg7"} 308.34872386 1683108597029
container_cpu_usage_seconds_total{container="check-endpoints",namespace="openshift-network-diagnostics",pod="network-check-source-746dd6c885-fldk4"} 287.449872369 1683108592751
container_cpu_usage_seconds_total{container="cloud-credential-operator",namespace="openshift-cloud-credential-operator",pod="cloud-credential-operator-6ffc47fc7f-zt4zw"} 173.850633838 1683108606042
container_cpu_usage_seconds_total{container="cluster-autoscaler-operator",namespace="openshift-machine-api",pod="cluster-autoscaler-operator-647cbf4d9d-cljtq"} 101.632360728 1683108601519
~~~

### ServiceMonitor kubelet-resource-metrics

The Prometheus configuration can be read from `/etc/prometheus/config_out/prometheus.env.yaml` in the `prometheus-k8s-[01]` pods. A new `ServiceMonitor` will be configured there:

~~~
$ oc exec -n openshift-monitoring prometheus-k8s-0  -- cat /etc/prometheus/config_out/prometheus.env.yaml | less
(...)
- job_name: serviceMonitor/openshift-monitoring/kubelet-resource-metrics/0
  honor_labels: true
  honor_timestamps: true                      # <----------- this is set
  kubernetes_sd_configs:
  - role: endpoints
    namespaces:
      names:
      - kube-system
  scrape_interval: 1m0s
  scrape_timeout: 30s
  metrics_path: /metrics/resource            # <------------- read from kubelet /metrics/resource instead of /metrics/cadvisor
  scheme: https
(...)
  metric_relabel_configs:
  - source_labels:
    - __name__
    regex: container_cpu_usage_seconds_total|container_memory_working_set_bytes|scrape_error
    action: keep
  - source_labels:
    - __name__
    target_label: __name__
    replacement: pa_$1
    action: replace
(...)
~~~

We can also see this `ServiceMonitor` via the API:

~~~
$ oc get servicemonitor -n openshift-monitoring kubelet-resource-metrics -o yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  creationTimestamp: "2023-05-03T10:03:00Z"
  generation: 1
  labels:
    app.kubernetes.io/name: kubelet
    app.kubernetes.io/part-of: openshift-monitoring
    k8s-app: kubelet
  name: kubelet-resource-metrics
  namespace: openshift-monitoring
  resourceVersion: "520043"
  uid: 57bfa99b-ce69-475d-9dbc-26fbf329a380
spec:
  endpoints:
  - bearerTokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
    bearerTokenSecret:
      key: ""
    honorLabels: true
    honorTimestamps: true
    interval: 1m0s
    metricRelabelings:
    - action: keep
      regex: container_cpu_usage_seconds_total|container_memory_working_set_bytes|scrape_error
      sourceLabels:
      - __name__
    - action: replace
      replacement: pa_$1
      sourceLabels:
      - __name__
      targetLabel: __name__
    path: /metrics/resource
    port: https-metrics
    relabelings:
    - action: replace
      sourceLabels:
      - __metrics_path__
      targetLabel: metrics_path
    scheme: https
    scrapeTimeout: 30s
    tlsConfig:
      ca: {}
      caFile: /etc/prometheus/configmaps/kubelet-serving-ca-bundle/ca-bundle.crt
      cert: {}
      certFile: /etc/prometheus/secrets/metrics-client-certs/tls.crt
      keyFile: /etc/prometheus/secrets/metrics-client-certs/tls.key
  jobLabel: k8s-app
  namespaceSelector:
    matchNames:
    - kube-system
  selector:
    matchLabels:
      k8s-app: kubelet
~~~

The `ServiceMonitor` targets the kubelet endpoints and uses the `https-metrics` port, thus `10250`:

~~~
$ oc get ep -n kube-system
NAME      ENDPOINTS                                                    AGE
kubelet   192.168.18.18:10250,192.168.18.18:10255,192.168.18.18:4194   36h
~~~

Prometheus renames the data that it gets from kubelet `/metrics/resource` and prefixes it with `pa_` (see the `ServiceMonitor` above).

### Querying metrics from the new ServiceMonitor

We can now query the raw Prometheus data for the new `ServiceMonitor` ([https://access.redhat.com/articles/4894261](https://access.redhat.com/articles/4894261)):

~~~
$  oc exec -c prometheus -n openshift-monitoring prometheus-k8s-0 -- curl --data-urlencode "query=pa_container_cpu_usage_seconds_total{namespace=\"openshift-cluster-version\"}[10m]" http://localhost:9090/api/v1/query 2>/dev/null  | jq '.'
{
  "status": "success",
  "data": {
    "resultType": "matrix",
    "result": [
      {
        "metric": {
          "__name__": "pa_container_cpu_usage_seconds_total",
          "container": "cluster-version-operator",
          "endpoint": "https-metrics",
          "instance": "192.168.18.18:10250",
          "job": "kubelet",
          "metrics_path": "/metrics/resource",
          "namespace": "openshift-cluster-version",
          "node": "sno.workload.bos2.lab",
          "pod": "cluster-version-operator-6c8b455fbb-sdrzq",
          "service": "kubelet"
        },
        "values": [
          [
            1683115420.506,
            "633.245698023"
          ],
          [
            1683115470.283,
            "633.46500069"
          ],
          [
            1683115542.317,
            "634.925027571"
          ],
          [
            1683115602.590,
            "634.9471959"
          ],
          [
            1683115656.951,
            "635.03250882"
          ],
          [
            1683115722.646,
            "635.057353948"
          ],
          [
            1683115769.333,
            "636.538756124"
          ],
          [
            1683115841.953,
            "636.725988239"
          ],
          [
            1683115897.743,
            "636.81144008"
          ],
          [
            1683115947.750,
            "636.825680846"
          ]
        ]
      }
    ]
  }
}
~~~

Compare that to the default `ServiceMonitor` that's querying cadvisor:

~~~
$  oc exec -c prometheus -n openshift-monitoring prometheus-k8s-0 -- curl --data-urlencode "query=container_cpu_usage_seconds_total{namespace=\"openshift-cluster-version\"}[10m]" http://localhost:9090/api/v1/query 2>/dev/null  | jq '.'
{
  "status": "success",
  "data": {
    "resultType": "matrix",
    "result": [
      {
        "metric": {
          "__name__": "container_cpu_usage_seconds_total",
          "container": "cluster-version-operator",
          "cpu": "total",
          "endpoint": "https-metrics",
          "id": "/kubepods.slice/kubepods-burstable.slice/kubepods-burstable-pod8e6d67fd_14e5_4f43_b99c_e9899676f981.slice/crio-c8e8734a6f3ccdf5f4fe5b402b576620a9e06be6af377ffdeab2e87fbdca2f5a.scope",
          "image": "quay.io/openshift-release-dev/ocp-release@sha256:4c5a7e26d707780be6466ddc9591865beb2e3baa5556432d23e8d57966a2dd18",
          "instance": "192.168.18.18:10250",
          "job": "kubelet",
          "metrics_path": "/metrics/cadvisor",
          "name": "k8s_cluster-version-operator_cluster-version-operator-6c8b455fbb-sdrzq_openshift-cluster-version_8e6d67fd-14e5-4f43-b99c-e9899676f981_1",
          "namespace": "openshift-cluster-version",
          "node": "sno.workload.bos2.lab",
          "pod": "cluster-version-operator-6c8b455fbb-sdrzq",
          "service": "kubelet"
        },
        "values": [
          [
            1683115453.677,
            "633.249080752"
          ],
          [
            1683115513.677,
            "634.912051604"
          ],
          [
            1683115573.677,
            "634.935097642"
          ],
          [
            1683115633.677,
            "634.968498889"
          ],
          [
            1683115693.677,
            "635.044346907"
          ],
          [
            1683115753.677,
            "635.402776476"
          ],
          [
            1683115813.677,
            "636.706505035"
          ],
          [
            1683115873.677,
            "636.732014919"
          ],
          [
            1683115933.677,
            "636.820754267"
          ],
          [
            1683115993.677,
            "636.837443227"
          ]
        ]
      },
      {
        "metric": {
          "__name__": "container_cpu_usage_seconds_total",
          "cpu": "total",
          "endpoint": "https-metrics",
          "id": "/kubepods.slice/kubepods-burstable.slice/kubepods-burstable-pod8e6d67fd_14e5_4f43_b99c_e9899676f981.slice",
          "instance": "192.168.18.18:10250",
          "job": "kubelet",
          "metrics_path": "/metrics/cadvisor",
          "namespace": "openshift-cluster-version",
          "node": "sno.workload.bos2.lab",
          "pod": "cluster-version-operator-6c8b455fbb-sdrzq",
          "service": "kubelet"
        },
        "values": [
          [
            1683115453.677,
            "670.476601522"
          ],
          [
            1683115513.677,
            "672.183884154"
          ],
          [
            1683115573.677,
            "672.258874213"
          ],
          [
            1683115633.677,
            "672.299388874"
          ],
          [
            1683115693.677,
            "672.368442138"
          ],
          [
            1683115753.677,
            "673.132759516"
          ],
          [
            1683115813.677,
            "674.139684623"
          ],
          [
            1683115873.677,
            "674.187607886"
          ],
          [
            1683115933.677,
            "674.250701464"
          ],
          [
            1683115993.677,
            "674.278054253"
          ]
        ]
      }
    ]
  }
}
~~~

And sure enough, when the `DedicatedServiceMonitors` (/metrics/resource) endpoint is disabled, the `pa_.*` entries are missing:

~~~
$ oc exec -c prometheus -n openshift-monitoring prometheus-k8s-0 -- curl --data-urlencode "query=pa_container_cpu_usage_seconds_total[1m]" http://localhost:9090/api/v1/query 2>/dev/null | jq '.'
{
  "status": "success",
  "data": {
    "resultType": "matrix",
    "result": []
  }
}
~~~

Samples are collected every minute, but with the `irate` function we can get the rate per second between the last 2 samples of the series ([https://prometheus.io/docs/prometheus/latest/querying/functions/#irate](https://prometheus.io/docs/prometheus/latest/querying/functions/#irate)).

For example, we can generate load in the cluster-version pod:

~~~
$ oc rsh -n openshift-cluster-version                          cluster-version-operator-6c8b455fbb-sdrzq
sh-4.4$ dd if=/dev/zero of=/dev/null
~~~

And then wait a bit and run:

~~~
$  oc exec -c prometheus -n openshift-monitoring prometheus-k8s-0 -- curl --data-urlencode "query=irate(pa_container_cpu_usage_seconds_total{namespace=\"openshift-cluster-version\"}[10m])" http://localhost:9090/api/v1/query 2>/dev/null  | jq '.'
{
  "status": "success",
  "data": {
    "resultType": "vector",
    "result": [
      {
        "metric": {
          "container": "cluster-version-operator",
          "endpoint": "https-metrics",
          "instance": "192.168.18.18:10250",
          "job": "kubelet",
          "metrics_path": "/metrics/resource",
          "namespace": "openshift-cluster-version",
          "node": "sno.workload.bos2.lab",
          "pod": "cluster-version-operator-6c8b455fbb-sdrzq",
          "service": "kubelet"
        },
        "value": [
          1683116762.162,
          "0.9984198379632914"
        ]
      }
    ]
  }
}
$  oc exec -c prometheus -n openshift-monitoring prometheus-k8s-0 -- curl --data-urlencode "query=irate(container_cpu_usage_seconds_total{namespace=\"openshift-cluster-version\"}[10m])" http://localhost:9090/api/v1/query 2>/dev/null  | jq '.'
{
  "status": "success",
  "data": {
    "resultType": "vector",
    "result": [
      {
        "metric": {
          "container": "cluster-version-operator",
          "cpu": "total",
          "endpoint": "https-metrics",
          "id": "/kubepods.slice/kubepods-burstable.slice/kubepods-burstable-pod8e6d67fd_14e5_4f43_b99c_e9899676f981.slice/crio-c8e8734a6f3ccdf5f4fe5b402b576620a9e06be6af377ffdeab2e87fbdca2f5a.scope",
          "image": "quay.io/openshift-release-dev/ocp-release@sha256:4c5a7e26d707780be6466ddc9591865beb2e3baa5556432d23e8d57966a2dd18",
          "instance": "192.168.18.18:10250",
          "job": "kubelet",
          "metrics_path": "/metrics/cadvisor",
          "name": "k8s_cluster-version-operator_cluster-version-operator-6c8b455fbb-sdrzq_openshift-cluster-version_8e6d67fd-14e5-4f43-b99c-e9899676f981_1",
          "namespace": "openshift-cluster-version",
          "node": "sno.workload.bos2.lab",
          "pod": "cluster-version-operator-6c8b455fbb-sdrzq",
          "service": "kubelet"
        },
        "value": [
          1683116764.877,
          "0.8565954120166661"
        ]
      },
      {
        "metric": {
          "cpu": "total",
          "endpoint": "https-metrics",
          "id": "/kubepods.slice/kubepods-burstable.slice/kubepods-burstable-pod8e6d67fd_14e5_4f43_b99c_e9899676f981.slice",
          "instance": "192.168.18.18:10250",
          "job": "kubelet",
          "metrics_path": "/metrics/cadvisor",
          "namespace": "openshift-cluster-version",
          "node": "sno.workload.bos2.lab",
          "pod": "cluster-version-operator-6c8b455fbb-sdrzq",
          "service": "kubelet"
        },
        "value": [
          1683116764.877,
          "0.9803661877999995"
        ]
      }
    ]
  }
}
~~~

### Prometheus Adapter reconfiguration

The Prometheus Adapter then reads and uses the new data that's exposed by Prometheus:

~~~
$ oc exec -n openshift-monitoring prometheus-adapter-57598cb4db-pqj2w -- cat /etc/adapter/config.yaml
"resourceRules":
  "cpu":
    "containerLabel": "container"
    "containerQuery": |
      sum by (<<.GroupBy>>) (
        irate (
            pa_container_cpu_usage_seconds_total{<<.LabelMatchers>>,container!="",pod!=""}[4m]
        )
      )
    "nodeQuery": |
      sum by (<<.GroupBy>>) (
        1 - irate(
          node_cpu_seconds_total{mode="idle"}[4m]
        )
        * on(namespace, pod) group_left(node) (
          node_namespace_pod:kube_pod_info:{<<.LabelMatchers>>}
        )
      )
      or sum by (<<.GroupBy>>) (
        1 - irate(
          windows_cpu_time_total{mode="idle", job="windows-exporter",<<.LabelMatchers>>}[4m]
        )
      )
    "resources":
      "overrides":
        "namespace":
          "resource": "namespace"
        "node":
          "resource": "node"
        "pod":
          "resource": "pod"
  "memory":
    "containerLabel": "container"
    "containerQuery": |
      sum by (<<.GroupBy>>) (
        pa_container_memory_working_set_bytes{<<.LabelMatchers>>,container!="",pod!=""}
      )
    "nodeQuery": |
      sum by (<<.GroupBy>>) (
        node_memory_MemTotal_bytes{job="node-exporter",<<.LabelMatchers>>}
        -
        node_memory_MemAvailable_bytes{job="node-exporter",<<.LabelMatchers>>}
      )
      or sum by (<<.GroupBy>>) (
        windows_cs_physical_memory_bytes{job="windows-exporter",<<.LabelMatchers>>}
        -
        windows_memory_available_bytes{job="windows-exporter",<<.LabelMatchers>>}
      )
    "resources":
      "overrides":
        "instance":
          "resource": "node"
        "namespace":
          "resource": "namespace"
        "pod":
          "resource": "pod"
  "window": "5m"
~~~

And this data in turn ends up in the `PodMetrics` and `NodeMetrics`:

~~~
$ oc get podmetrics -n openshift-machine-api | grep cluster-autoscaler
cluster-autoscaler-operator-647cbf4d9d-cljtq         0     74508Ki   5m0s
~~~

This data is also used by `oc adm top pod` and the HPA:

~~~
$ oc adm top pod -n openshift-cluster-version
NAME                                        CPU(cores)   MEMORY(bytes)   
cluster-version-operator-6c8b455fbb-sdrzq   997m         151Mi 
~~~

