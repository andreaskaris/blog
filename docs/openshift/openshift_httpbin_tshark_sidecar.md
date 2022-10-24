# AlertManager #

## Configuring Alertmanager with webhooks and httpbin container with tshark sidecar as a consumer ##

### Summary ###

The following describe a setup *on OCP 3.11* with:

* a container running httpbin and a sidecar running tshark and filtering for incoming http requests and logging them
* configuration of Alertmanager so that it sends alerts via webhook to httpbin
* loading cluster with high number of pods
* analyzing generated alarms

### Prerequisites ###

Make sure that ocntainers can run as any uid:
~~~
# oc adm policy add-scc-to-user anyuid -z default
scc "anyuid" added to: ["system:serviceaccount:default:default"]
~~~

### OpenShift httpbin with tshark sidecar ###

The following allows us to see any incoming requests to httpbin but to filter out httpbin's answers.

Prerequisites:
~~~
# oc adm policy add-scc-to-user anyuid -z default
scc "anyuid" added to: ["system:serviceaccount:default:default"]
~~~

Create file `httpbin.yaml`:
~~~
apiVersion: route.openshift.io/v1
kind: Route
metadata:
  labels:
    app: httpbin-deploymentconfig
  name: httpbin-service
spec:
  host: httpbin.apps.akaris2.lab.pnq2.cee.redhat.com
  port:
    targetPort: 80
  to:
    kind: Service
    name: httpbin-service
    weight: 100
  wildcardPolicy: None
---
apiVersion: v1
kind: Service
metadata:
  name: httpbin-service
  labels:
    app: httpbin-deploymentconfig
spec:
  selector:
    app: httpbin-pod
  ports:
    - protocol: TCP
      port: 80
      targetPort: 80
---
apiVersion: v1
kind: DeploymentConfig
metadata:
  name: httpbin-deploymentconfig
  labels:
    app: httpbin-deploymentconfig
spec:
  replicas: 1
  selector:
    app: httpbin-pod
  template:
    metadata:
      labels:
        app: httpbin-pod
    spec:
      containers:
      - name: tshark
        image: danielguerra/alpine-tshark 
        command:
          - "tshark" 
          - "-i" 
          - "eth0" 
          - "-Y" 
          - "http" 
          - "-V"  
          - "dst" 
          - "port"
          - "80"
      - name: httpbin
        image: kennethreitz/httpbin
        imagePullPolicy: Always
        command:
        - "gunicorn"
        - "-b"
        - "0.0.0.0:80"
        - "httpbin:app"
        - "-k"
        - "gevent"
        - "--capture-output"
        - "--error-logfile"
        - "-"
        - "--access-logfile"
        - "-"
        - "--access-logformat"
        - "'%(h)s %(t)s %(r)s %(s)s Host: %({Host}i)s} Header-i: %({Header}i)s Header-o: %({Header}o)s'"
~~~

Apply config:
~~~
oc apply -f httpbin.yaml 
~~~

Get the pod name and loolk at the pod's logs for container `tshark`:
~~~
[root@master-0 ~]# oc get pods -l app=httpbin-pod
NAME                               READY     STATUS    RESTARTS   AGE
httpbin-deploymentconfig-8-tgmvn   2/2       Running   0          3m
[root@master-0 ~]# oc logs httpbin-deploymentconfig-8-tgmvn -c tshark  -f
Capturing on 'eth0'
Frame 4: 535 bytes on wire (4280 bits), 535 bytes captured (4280 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Mar 11, 2020 12:17:13.290037158 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1583929033.290037158 seconds
    [Time delta from previous captured frame: 0.000002253 seconds]
    [Time delta from previous displayed frame: 0.000000000 seconds]
    [Time since reference or first frame: 36.739477011 seconds]
    Frame Number: 4
    Frame Length: 535 bytes (4280 bits)
    Capture Length: 535 bytes (4280 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:tcp:http:urlencoded-form]
Ethernet II, Src: 7a:9c:fa:d2:07:d8 (7a:9c:fa:d2:07:d8), Dst: 0a:58:0a:80:00:0c (0a:58:0a:80:00:0c)
    Destination: 0a:58:0a:80:00:0c (0a:58:0a:80:00:0c)
        Address: 0a:58:0a:80:00:0c (0a:58:0a:80:00:0c)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: 7a:9c:fa:d2:07:d8 (7a:9c:fa:d2:07:d8)
        Address: 7a:9c:fa:d2:07:d8 (7a:9c:fa:d2:07:d8)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 10.130.0.1, Dst: 10.128.0.12
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Total Length: 521
    Identification: 0xdfdf (57311)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: TCP (6)
    Header checksum: 0x4401 [validation disabled]
    [Header checksum status: Unverified]
    Source: 10.130.0.1
    Destination: 10.128.0.12
Transmission Control Protocol, Src Port: 38288, Dst Port: 80, Seq: 1, Ack: 1, Len: 469
    Source Port: 38288
    Destination Port: 80
    [Stream index: 1]
    [TCP Segment Len: 469]
    Sequence number: 1    (relative sequence number)
    [Next sequence number: 470    (relative sequence number)]
    Acknowledgment number: 1    (relative ack number)
    1000 .... = Header Length: 32 bytes (8)
    Flags: 0x018 (PSH, ACK)
        000. .... .... = Reserved: Not set
        ...0 .... .... = Nonce: Not set
        .... 0... .... = Congestion Window Reduced (CWR): Not set
        .... .0.. .... = ECN-Echo: Not set
        .... ..0. .... = Urgent: Not set
        .... ...1 .... = Acknowledgment: Set
        .... .... 1... = Push: Set
        .... .... .0.. = Reset: Not set
        .... .... ..0. = Syn: Not set
        .... .... ...0 = Fin: Not set
        [TCP Flags: ·······AP···]
    Window size value: 221
    [Calculated window size: 28288]
    [Window size scaling factor: 128]
    Checksum: 0xd9c6 [unverified]
    [Checksum Status: Unverified]
    Urgent pointer: 0
    Options: (12 bytes), No-Operation (NOP), No-Operation (NOP), Timestamps
        TCP Option - No-Operation (NOP)
            Kind: No-Operation (1)
        TCP Option - No-Operation (NOP)
            Kind: No-Operation (1)
        TCP Option - Timestamps: TSval 44637623, TSecr 44644920
            Kind: Time Stamp Option (8)
            Length: 10
            Timestamp value: 44637623
            Timestamp echo reply: 44644920
    [SEQ/ACK analysis]
        [iRTT: 0.001410475 seconds]
        [Bytes in flight: 470]
        [Bytes sent since last PSH flag: 469]
    TCP payload (469 bytes)
Hypertext Transfer Protocol
    POST /post HTTP/1.1\r\n
        [Expert Info (Chat/Sequence): POST /post HTTP/1.1\r\n]
            [POST /post HTTP/1.1\r\n]
            [Severity level: Chat]
            [Group: Sequence]
        Request Method: POST
        Request URI: /post
        Request Version: HTTP/1.1
~~~

### Configuring Alertmanager to send webhooks to httpbin pod ###

Prerequisites:
* https://docs.openshift.com/container-platform/3.11/install_config/prometheus_cluster_monitoring.html

In the following, replace `myuser` with the user who shall log into alertmanager:
~~~
$ oc adm policy add-cluster-role-to-user cluster-monitoring-view myuser
cluster role "cluster-monitoring-view" added: "myuser"
~~~

We can use the above to tell alertmanager to use httpbin as its web hook:
~~~
$ oc get routes -n openshift-monitoring
NAME                HOST/PORT                                                                     PATH      SERVICES            PORT      TERMINATION   WILDCARD
alertmanager-main   alertmanager-main-openshift-monitoring.apps.akaris2.lab.pnq2.cee.redhat.com             alertmanager-main   web       reencrypt     None
grafana             grafana-openshift-monitoring.apps.akaris2.lab.pnq2.cee.redhat.com                       grafana             https     reencrypt     None
prometheus-k8s      prometheus-k8s-openshift-monitoring.apps.akaris2.lab.pnq2.cee.redhat.com                prometheus-k8s      web       reencrypt     None
~~~

Now, access:
~~~
https://alertmanager-main-openshift-monitoring.apps.akaris2.lab.pnq2.cee.redhat.com
~~~

The status page will show the current alertmanager configuration.

The following Red Hat knowledge base solution shows how to update the alertmanager config: https://access.redhat.com/solutions/3804781

Create file: `~/group_vars/OSEv3.yml`:
~~~
openshift_cluster_monitoring_operator_alertmanager_config: |+
  global:
    resolve_timeout: 2m
  route:
    group_wait: 5s
    group_interval: 10s
    repeat_interval: 20s
    receiver: default
    routes:
    - match:
        alertname: DeadMansSwitch
      repeat_interval: 30s
      receiver: deadmansswitch
    - match:
        alertname: DeadMansSwitch
      repeat_interval: 30s
      receiver: wh
    - match:
        alertname: '*'
      repeat_interval: 2m
      receiver: wh
    - match:
        severity: critical
      receiver: wh
    - match:
        severity: warning
      receiver: wh
    - match:
        alertname: KubeAPILatencyHigh
      receiver: wh
  receivers:
  - name: default
  - name: deadmansswitch
  - name: wh
    webhook_configs:
      - url: "http://httpbin.apps.akaris2.lab.pnq2.cee.redhat.com/anything"
~~~

And run:
~~~
ansible-playbook -i hosts openshift-ansible/playbooks/openshift-monitoring/config.yml -e="openshift_cluster_monitoring_operator_install=true"
~~~

Verify:
~~~
$ oc get secret -n openshift-monitoring alertmanager-main -o yaml | awk '/alertmanager.yaml:/ {print $NF}' | base64 -d
global:
  resolve_timeout: 2m
route:
  group_wait: 5s
  group_interval: 10s
  repeat_interval: 20s
  receiver: default
  routes:
  - match:
      alertname: DeadMansSwitch
    repeat_interval: 30s
    receiver: deadmansswitch
  - match:
      alertname: DeadMansSwitch
    repeat_interval: 30s
    receiver: wh
  - match:
      alertname: '*'
    repeat_interval: 2m
    receiver: wh
  - match:
      severity: critical
    receiver: wh
  - match:
      severity: warning
    receiver: wh
  - match:
      alertname: KubeAPILatencyHigh
    receiver: wh
receivers:
- name: default
- name: deadmansswitch
- name: wh
  webhook_configs:
    - url: "http://httpbin.apps.akaris2.lab.pnq2.cee.redhat.com/anything"
~~~

Restart pods:
~~~
$ oc delete pods --selector=app=alertmanager -n openshift-monitoring
pod "alertmanager-main-0" deleted
pod "alertmanager-main-1" deleted
pod "alertmanager-main-2" deleted
~~~

And check in the web interface of alertmanager to make sure that the new configuration shows up.

### Loading the cluster ###

An easy way to generate an alert in a small lab is to trigger alert `KubeletTooManyPods`. Go to prometheus and check its configuration:
~~~
alert: KubeletTooManyPods
expr: kubelet_running_pod_count{job="kubelet"}
  > 250 * 0.9
for: 15m
labels:
  severity: warning
annotations:
  message: Kubelet {{ $labels.instance }} is running {{ $value }} Pods, close to the
    limit of 250.
~~~

Then, create the following busybox deployment with a number of pods that exceeds this number, e.g.:
`busybox.yaml`:
~~~
apiVersion: apps/v1
kind: Deployment
metadata:
  name: busybox-deployment
  labels:
    app: busybox-deployment
spec:
  replicas: 500
  selector:
    matchLabels:
      app: busybox-pod
  template:
    metadata:
      labels:
        app: busybox-pod
    spec:
      containers:
      - name: busybox
        image: busybox
        command:
          - sleep
          - infinity
        imagePullPolicy: IfNotPresent
~~~

~~~
oc apply -f busybox.yaml
~~~

The cluster will need some time to create those pods and it'll take 15 minutes for the alarm to fire. So take a coffee and come back later. Once the alarm fires in prometheus, go to alertmanager and make sure that it shows there, too.

Among others, Alertmanager should show:
~~~
alertname="KubeletTooManyPods"
16:06:32, 2020-03-11
message:	Kubelet 10.74.176.204:10250 is running 250 Pods, close to the limit of 250.
severity="warning"
service="kubelet"prometheus="openshift-monitoring/k8s"namespace="kube-system"job="kubelet"instance="10.74.176.204:10250"endpoint="https-metrics"
~~~

Now, it's time to go back to the httpbin pod.

### Monitoring incoming webhook reuests ###

Get the pod name:
~~~
# oc get pods | grep httpbin
httpbin-deploymentconfig-8-8crvh     2/2       Running   0          1h
~~~

And check the logs of the tshark container which will show a verbose packet capture of HTTP with a destination port of 80 (so we are not capturing the response):
~~~
# oc logs httpbin-deploymentconfig-8-8crvh -c tshark | tail -n 400
(...)
Frame 1708: 5220 bytes on wire (41760 bits), 5220 bytes captured (41760 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
(...)
Ethernet II, Src: ... (...), Dst: ... (...)
(...)
Internet Protocol Version 4, Src: ..., Dst: ...
(...)
Transmission Control Protocol, Src Port: 41606, Dst Port: 80, Seq: 1, Ack: 1, Len: 5154
(...)
Hypertext Transfer Protocol
    POST /anything HTTP/1.1\r\n
        [Expert Info (Chat/Sequence): POST /anything HTTP/1.1\r\n]
            [POST /anything HTTP/1.1\r\n]
            [Severity level: Chat]
            [Group: Sequence]
        Request Method: POST
        Request URI: /anything
        Request Version: HTTP/1.1
    User-Agent: Alertmanager/0.15.2\r\n
    Content-Length: 4743\r\n
        [Content length: 4743]
    Content-Type: application/json\r\n
(...)
JavaScript Object Notation: application/json
    Object
        Member Key: receiver
            String value: wh
            Key: receiver
        Member Key: status
            String value: firing
            Key: status
        Member Key: alerts
            Array
                Object
                    Member Key: status
                        String value: firing
                        Key: status
                    Member Key: labels
                        Object
                            Member Key: alertname
                                String value: KubeDaemonSetRolloutStuck
                                Key: alertname
                            Member Key: cluster
                                String value: openshift.akaris2.lab.pnq2.cee.redhat.com
                                Key: cluster
                            Member Key: daemonset
                                String value: node-exporter
                                Key: daemonset
                            Member Key: endpoint
                                String value: https-main
                                Key: endpoint
                            Member Key: instance
                                String value: ...:8443
                                Key: instance
                            Member Key: job
                                String value: kube-state-metrics
                                Key: job
                            Member Key: namespace
                                String value: openshift-monitoring
                                Key: namespace
                            Member Key: pod
                                String value: kube-state-metrics-6f4c658bcc-v57b6
                                Key: pod
                            Member Key: prometheus
                                String value: openshift-monitoring/k8s
                                Key: prometheus
                            Member Key: service
                                String value: kube-state-metrics
                                Key: service
                            Member Key: severity
                                String value: critical
                                Key: severity
                        Key: labels
                    Member Key: annotations
                        Object
                            Member Key: message
                                String value: Only 66.66666666666666% of desired pods scheduled and ready for daemon set openshift-monitoring/node-exporter
                                Key: message
                        Key: annotations
                    Member Key: startsAt
                        String value: 2020-03-11T16:07:40.59085788Z
                        Key: startsAt
                    Member Key: endsAt
                        String value: 0001-01-01T00:00:00Z
                        Key: endsAt
                    Member Key: generatorURL
                        String value [truncated]: https://prometheus-k8s-openshift-monitoring.apps.akaris2.lab.pnq2.cee.redhat.com/graph?g0.expr=kube_daemonset_status_number_ready%7Bjob%3D%22kube-state-metrics%22%2Cnamespace%3D~%22%28openshift-.%2A%7Ckube-.%2A%7C
                        Key: generatorURL
                Object
                    Member Key: status
                        String value: firing
                        Key: status
                    Member Key: labels
                        Object
                            Member Key: alertname
                                String value: KubeDaemonSetRolloutStuck
                                Key: alertname
                            Member Key: cluster
                                String value: openshift.akaris2.lab.pnq2.cee.redhat.com
                                Key: cluster
                            Member Key: daemonset
                                String value: ovs
                                Key: daemonset
                            Member Key: endpoint
                                String value: https-main
                                Key: endpoint
                            Member Key: instance
                                String value: ...:8443
                                Key: instance
                            Member Key: job
                                String value: kube-state-metrics
                                Key: job
                            Member Key: namespace
                                String value: openshift-sdn
                                Key: namespace
                            Member Key: pod
                                String value: kube-state-metrics-6f4c658bcc-v57b6
                                Key: pod
                            Member Key: prometheus
                                String value: openshift-monitoring/k8s
                                Key: prometheus
                            Member Key: service
                                String value: kube-state-metrics
                                Key: service
                            Member Key: severity
                                String value: critical
                                Key: severity
                        Key: labels
                    Member Key: annotations
                        Object
                            Member Key: message
                                String value: Only 66.66666666666666% of desired pods scheduled and ready for daemon set openshift-sdn/ovs
                                Key: message
                        Key: annotations
                    Member Key: startsAt
                        String value: 2020-03-11T16:07:40.59085788Z
                        Key: startsAt
                    Member Key: endsAt
                        String value: 0001-01-01T00:00:00Z
                        Key: endsAt
                    Member Key: generatorURL
                        String value [truncated]: https://prometheus-k8s-openshift-monitoring.apps.akaris2.lab.pnq2.cee.redhat.com/graph?g0.expr=kube_daemonset_status_number_ready%7Bjob%3D%22kube-state-metrics%22%2Cnamespace%3D~%22%28openshift-.%2A%7Ckube-.%2A%7C
                        Key: generatorURL
                Object
                    Member Key: status
                        String value: firing
                        Key: status
                    Member Key: labels
                        Object
                            Member Key: alertname
                                String value: KubeDaemonSetRolloutStuck
                                Key: alertname
                            Member Key: cluster
                                String value: openshift.akaris2.lab.pnq2.cee.redhat.com
                                Key: cluster
                            Member Key: daemonset
                                String value: sdn
                                Key: daemonset
                            Member Key: endpoint
                                String value: https-main
                                Key: endpoint
                            Member Key: instance
                                String value: ...:8443
                                Key: instance
                            Member Key: job
                                String value: kube-state-metrics
                                Key: job
                            Member Key: namespace
                                String value: openshift-sdn
                                Key: namespace
                            Member Key: pod
                                String value: kube-state-metrics-6f4c658bcc-v57b6
                                Key: pod
                            Member Key: prometheus
                                String value: openshift-monitoring/k8s
                                Key: prometheus
                            Member Key: service
                                String value: kube-state-metrics
                                Key: service
                            Member Key: severity
                                String value: critical
                                Key: severity
                        Key: labels
                    Member Key: annotations
                        Object
                            Member Key: message
                                String value: Only 66.66666666666666% of desired pods scheduled and ready for daemon set openshift-sdn/sdn
                                Key: message
                        Key: annotations
                    Member Key: startsAt
                        String value: 2020-03-11T16:07:40.59085788Z
                        Key: startsAt
                    Member Key: endsAt
                        String value: 0001-01-01T00:00:00Z
                        Key: endsAt
                    Member Key: generatorURL
                        String value [truncated]: https://prometheus-k8s-openshift-monitoring.apps.akaris2.lab.pnq2.cee.redhat.com/graph?g0.expr=kube_daemonset_status_number_ready%7Bjob%3D%22kube-state-metrics%22%2Cnamespace%3D~%22%28openshift-.%2A%7Ckube-.%2A%7C
                        Key: generatorURL
                Object
                    Member Key: status
                        String value: firing
                        Key: status
                    Member Key: labels
                        Object
                            Member Key: alertname
                                String value: KubeDaemonSetRolloutStuck
                                Key: alertname
                            Member Key: cluster
                                String value: openshift.akaris2.lab.pnq2.cee.redhat.com
                                Key: cluster
                            Member Key: daemonset
                                String value: sync
                                Key: daemonset
                            Member Key: endpoint
                                String value: https-main
                                Key: endpoint
                            Member Key: instance
                                String value: ...:8443
                                Key: instance
                            Member Key: job
                                String value: kube-state-metrics
                                Key: job
                            Member Key: namespace
                                String value: openshift-node
                                Key: namespace
                            Member Key: pod
                                String value: kube-state-metrics-6f4c658bcc-v57b6
                                Key: pod
                            Member Key: prometheus
                                String value: openshift-monitoring/k8s
                                Key: prometheus
                            Member Key: service
                                String value: kube-state-metrics
                                Key: service
                            Member Key: severity
                                String value: critical
                                Key: severity
                        Key: labels
                    Member Key: annotations
                        Object
                            Member Key: message
                                String value: Only 66.66666666666666% of desired pods scheduled and ready for daemon set openshift-node/sync
                                Key: message
                        Key: annotations
                    Member Key: startsAt
                        String value: 2020-03-11T16:07:40.59085788Z
                        Key: startsAt
                    Member Key: endsAt
                        String value: 0001-01-01T00:00:00Z
                        Key: endsAt
                    Member Key: generatorURL
                        String value [truncated]: https://prometheus-k8s-openshift-monitoring.apps.akaris2.lab.pnq2.cee.redhat.com/graph?g0.expr=kube_daemonset_status_number_ready%7Bjob%3D%22kube-state-metrics%22%2Cnamespace%3D~%22%28openshift-.%2A%7Ckube-.%2A%7C
                        Key: generatorURL
            Key: alerts
        Member Key: groupLabels
            Object
            Key: groupLabels
        Member Key: commonLabels
            Object
                Member Key: alertname
                    String value: KubeDaemonSetRolloutStuck
                    Key: alertname
                Member Key: cluster
                    String value: openshift.akaris2.lab.pnq2.cee.redhat.com
                    Key: cluster
                Member Key: endpoint
                    String value: https-main
                    Key: endpoint
                Member Key: instance
                    String value: ...:8443
                    Key: instance
                Member Key: job
                    String value: kube-state-metrics
                    Key: job
                Member Key: pod
                    String value: kube-state-metrics-6f4c658bcc-v57b6
                    Key: pod
                Member Key: prometheus
                    String value: openshift-monitoring/k8s
                    Key: prometheus
                Member Key: service
                    String value: kube-state-metrics
                    Key: service
                Member Key: severity
                    String value: critical
                    Key: severity
            Key: commonLabels
~~~
