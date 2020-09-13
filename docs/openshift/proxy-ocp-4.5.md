# Configuring a proxy for OCP 4.5 #

When applying proxy settings to an existing cluster, make sure not to block important traffic from the internet and do not block too early.   Meaning that configuring a proxy in an environment with limited internet access, and where the proxy configuration is not pushed from the start, for obvious reasons will create issues and is a bit of a chicken/egg issue. So first, I need to make sure that the nodes are all up and healthy, that the machineconfigpools are updated, and that the cluster can reach the internet. Then, I can configure a proxy. Then, I can block all traffic to the internet and rely on the proxy.

Here are the steps to set up a proxy and steps for validation, as well.

### Lab setup/clarification ###

In my lab, all traffic goes through my jumpserver, 192.168.123.1. All servers are on 192.168.123.0/24 and domain is example.com
~~~
[root@openshift-master-1 ~]# ip r | grep default
default via 192.168.123.1 dev ens3 proto dhcp metric 100 
~~~

So, on the jumpserver, I install squid and then block all direct traffic:
~~~
yum install squid -y
systemctl start squid
~~~

Testing the proxy:
~~~
[root@openshift-master-0 ~]# curl --proxy http://192.168.123.1:3128 http://www.httpbin.org/get
{
  "args": {}, 
  "headers": {
    "Accept": "*/*", 
    "Cache-Control": "max-age=259200", 
    "Host": "www.httpbin.org", 
    "If-Modified-Since": "Tue, 08 Sep 2020 15:33:44 GMT", 
    "User-Agent": "curl/7.61.1", 
    "X-Amzn-Trace-Id": "Root=1-5f57b2df-1a16ccc15b6a08f114114fc6"
  }, 
  "origin": "192.168.123.200, 66.187.232.131", 
  "url": "http://www.httpbin.org/get"
}
[root@openshift-master-0 ~]# curl --proxy http://192.168.123.1:3128 https://www.httpbin.org/get
{
  "args": {}, 
  "headers": {
    "Accept": "*/*", 
    "Host": "www.httpbin.org", 
    "User-Agent": "curl/7.61.1", 
    "X-Amzn-Trace-Id": "Root=1-5f57b2e7-49d5b86c15a39eaba3a017c1"
  }, 
  "origin": "66.187.232.131", 
  "url": "https://www.httpbin.org/get"
}
~~~

Before blocking all internet traffic, I must set up a proxy. It's chicken egg problem as the machineconfigoperator cannot configure the nodes without internet access (this cluster was installed with internet access, so the proxy needs to be configured first).

### Blocking traffic for demonstration purposes ###

Just a quick and ugly REJECT all in the FORWARD chain. Later on, we'll use a more coarse block as this one here is too extreme:
~~~
iptables -I FORWARD --src 192.168.123.0/24 --j REJECT
~~~

Starting from that moment on, direct traffic to the internet is completely blocked.
~~~
[root@openshift-jumpserver-0 ~]# oc debug node/openshift-master-0.example.com --image=registry.redhat.io/rhel8/support-tools
Starting pod/openshift-master-0examplecom-debug ...
To use host binaries, run `chroot /host`

Removing debug pod ...
error: Back-off pulling image "registry.redhat.io/rhel8/support-tools"
~~~

Now, when I conntect to the server, and try:
~~~
[root@openshift-master-1 ~]# timeout 10 curl  www.httpbin.org/get
[root@openshift-master-1 ~]# 
~~~

~~~
[root@openshift-master-1 ~]# curl --proxy http://192.168.123.1:3128 http://www.httpbin.org/get
{
  "args": {}, 
  "headers": {
    "Accept": "*/*", 
    "Cache-Control": "max-age=259200", 
    "Host": "www.httpbin.org", 
    "If-Modified-Since": "Tue, 08 Sep 2020 15:31:09 GMT", 
    "User-Agent": "curl/7.61.1", 
    "X-Amzn-Trace-Id": "Root=1-5f57a45c-5d80e17e71555d043de9db02"
  }, 
  "origin": "192.168.123.201, 66.187.232.131", 
  "url": "http://www.httpbin.org/get"
}
[root@openshift-master-1 ~]# curl --proxy http://192.168.123.1:3128 https://www.httpbin.org/get
{
  "args": {}, 
  "headers": {
    "Accept": "*/*", 
    "Host": "www.httpbin.org", 
    "User-Agent": "curl/7.61.1", 
    "X-Amzn-Trace-Id": "Root=1-5f57a45f-7ae7bbe010e4b5ae6635162e"
  }, 
  "origin": "66.187.232.131", 
  "url": "https://www.httpbin.org/get"
}
~~~

### Unblocking traffic ###

Reallow outbound connections as otherwise the cluster will fail:
~~~
iptables -D FORWARD --src 192.168.123.0/24 --j REJECT
~~~

### Verifying nodes and machineconfigpools ###

Make sure that nodes, machineconfigpools, machineconfigs are all o.k.:
~~~
[root@openshift-jumpserver-0 ~]# oc get nodes
NAME                             STATUS   ROLES     AGE    VERSION
openshift-master-0.example.com   Ready    master    4d1h   v1.18.3+2cf11e2
openshift-master-1.example.com   Ready    master    4d1h   v1.18.3+2cf11e2
openshift-master-2.example.com   Ready    master    4d1h   v1.18.3+2cf11e2
openshift-worker-0.example.com   Ready    worker    4d1h   v1.18.3+2cf11e2
openshift-worker-1.example.com   Ready    worker    4d1h   v1.18.3+2cf11e2
openshift-worker-2.example.com   Ready    elastic   4d1h   v1.18.3+2cf11e2
[root@openshift-jumpserver-0 ~]# oc get machineconfigpool
NAME      CONFIG                                              UPDATED   UPDATING   DEGRADED   MACHINECOUNT   READYMACHINECOUNT   UPDATEDMACHINECOUNT   DEGRADEDMACHINECOUNT   AGE
elastic   rendered-elastic-c3d458a557429fb1abeb6d1c09a69dfc   True      False      False      1              1                   1                     0                      26h
master    rendered-master-edd2a0d47b9511f4a90b6d79798ca16e    True      False      False      3              3                   3                     0                      4d1h
worker    rendered-worker-e5312f943ba9275872060e9753288916    True      False      False      2              2                   2                     0                      4d1h
[root@openshift-jumpserver-0 ~]# oc get machineconfig
NAME                                                        GENERATEDBYCONTROLLER                      IGNITIONVERSION   AGE
00-master                                                   f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             4d1h
00-worker                                                   f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             4d1h
01-master-container-runtime                                 f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             4d1h
01-master-kubelet                                           f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             4d1h
01-worker-container-runtime                                 f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             4d1h
01-worker-kubelet                                           f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             4d1h
50-elastic                                                                                             2.2.0             26h
99-master-367f0f30-d480-4fa4-86ae-21dc30d2b7ce-registries   f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             4d1h
99-master-ssh                                                                                          2.2.0             4d1h
99-worker-bb447ace-f8a6-4456-b49d-64acef50a333-registries   f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             4d1h
99-worker-ssh                                                                                          2.2.0             4d1h
rendered-elastic-c3d458a557429fb1abeb6d1c09a69dfc           f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             26h
rendered-master-edd2a0d47b9511f4a90b6d79798ca16e            f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             4d1h
rendered-worker-e5312f943ba9275872060e9753288916            f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             4d1h
~~~

And make sure that the used machineconfig does not contain proxy settings:
~~~
[root@openshift-jumpserver-0 ~]# oc get machineconfig rendered-master-edd2a0d47b9511f4a90b6d79798ca16e -o yaml | grep -i proxy
[root@openshift-jumpserver-0 ~]# 
~~~

### Configuring proxy in OCP 4.x ###

Now, configure the proxy:
~~~
[root@openshift-jumpserver-0 ~]# oc edit proxy/cluster
proxy.config.openshift.io/cluster edited
[root@openshift-jumpserver-0 ~]# oc get proxy/cluster -o yaml
apiVersion: config.openshift.io/v1
kind: Proxy
metadata:
  creationTimestamp: "2020-09-04T15:01:16Z"
  generation: 2
  managedFields:
  - apiVersion: config.openshift.io/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:spec:
        .: {}
        f:trustedCA:
          .: {}
          f:name: {}
      f:status: {}
    manager: cluster-bootstrap
    operation: Update
    time: "2020-09-04T15:01:17Z"
  - apiVersion: config.openshift.io/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:spec:
        f:httpProxy: {}
        f:httpsProxy: {}
        f:noProxy: {}
        f:readinessEndpoints: {}
    manager: oc
    operation: Update
    time: "2020-09-08T15:37:36Z"
  name: cluster
  resourceVersion: "1934446"
  selfLink: /apis/config.openshift.io/v1/proxies/cluster
  uid: 9917e11d-6bc7-460a-8d5c-6c2bcf191f6e
spec:
  httpProxy: http://192.168.123.1:3128
  httpsProxy: http://192.168.123.1:3128
  noProxy: example.com,172.16.0.0/12,10.0.0.0/16,192.168.0.0/16
  readinessEndpoints:
  - http://www.google.com
  - https://www.google.com
  trustedCA:
    name: ""
status: {}
~~~

It's of utmost important to configure the noProxy correctly. Otherwise, the openshift-api server might not be able to function any more later down the road, for example with bad noProxy, after all steps completed, one might see:
~~~
oc logs -n openshift-kube-apiserver kube-apiserver-openshift-master-1.example.com
(...)
43: i/o timeout', Header: map[Content-Type:[text/plain; charset=utf-8] X-Content-Type-Options:[nosniff]]
I0908 17:44:54.369013       1 controller.go:127] OpenAPI AggregationController: action for item v1.project.openshift.io: Rate Limited Requeue.
E0908 17:45:24.376826       1 controller.go:114] loading OpenAPI spec for "v1.route.openshift.io" failed with: failed to retrieve openAPI spec, http error: ResponseCode: 503, Body: Error trying to reach service: 'dial tcp 172.25.0.12:8443: i/o timeout', Header: map[Content-Type:[text/plain; charset=utf-8] X-Content-Type-Options:[nosniff]]
~~~

This will create machineconfigs with the proxy settings:
~~~
[root@openshift-jumpserver-0 ~]# oc get machineconfig | grep 1m
rendered-elastic-862e04ea44d8822c0343fcd1ec4a1fcb           f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             1m
rendered-master-0ac69c11e1e16fcd06a8ea95e2b605af            f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             1m
rendered-worker-de2ff42fcefa2bef931fa94091019990            f6ec58e7b69f4fc1eb2297c2734b0470a581f378   2.2.0             1m
~~~

~~~
[root@openshift-jumpserver-0 ~]# oc describe machineconfigpool master | grep -i 'All nodes are updating to'
    Message:               All nodes are updating to rendered-master-0ac69c11e1e16fcd06a8ea95e2b605af
~~~

If you inspect these machineconfigs, you can see that they will push changes to the environment configuration, to crio, etc.:
~~~
[root@openshift-jumpserver-0 ~]# oc get machineconfig rendered-master-0ac69c11e1e16fcd06a8ea95e2b605af -o yaml | grep -i proxy -C20
(...) 
~~~

Wait a bit as all nodes need to restart with the new configuration:
~~~
[root@openshift-jumpserver-0 ~]# oc get nodes
NAME                             STATUS                     ROLES     AGE    VERSION
openshift-master-0.example.com   Ready                      master    4d2h   v1.18.3+2cf11e2
openshift-master-1.example.com   NotReady                   master    4d2h   v1.18.3+2cf11e2
openshift-master-2.example.com   Ready                      master    4d2h   v1.18.3+2cf11e2
openshift-worker-0.example.com   Ready                      worker    4d1h   v1.18.3+2cf11e2
openshift-worker-1.example.com   Ready,SchedulingDisabled   worker    4d1h   v1.18.3+2cf11e2
openshift-worker-2.example.com   Ready                      elastic   4d1h   v1.18.3+2cf11e2
[root@openshift-jumpserver-0 ~]# oc get machineconfigpool
NAME      CONFIG                                              UPDATED   UPDATING   DEGRADED   MACHINECOUNT   READYMACHINECOUNT   UPDATEDMACHINECOUNT   DEGRADEDMACHINECOUNT   AGE
elastic   rendered-elastic-862e04ea44d8822c0343fcd1ec4a1fcb   True      False      False      1              1                   1                     0                      26h
master    rendered-master-edd2a0d47b9511f4a90b6d79798ca16e    False     True       False      3              1                   1                     0                      4d1h
worker    rendered-worker-e5312f943ba9275872060e9753288916    False     True       False      2              1                   1                     0                      4d1h
~~~

#### Verification ####

The environment should stabilize:
~~~
[root@openshift-jumpserver-0 ~]# oc get nodes
NAME                             STATUS   ROLES     AGE    VERSION
openshift-master-0.example.com   Ready    master    4d2h   v1.18.3+2cf11e2
openshift-master-1.example.com   Ready    master    4d2h   v1.18.3+2cf11e2
openshift-master-2.example.com   Ready    master    4d2h   v1.18.3+2cf11e2
openshift-worker-0.example.com   Ready    worker    4d2h   v1.18.3+2cf11e2
openshift-worker-1.example.com   Ready    worker    4d2h   v1.18.3+2cf11e2
openshift-worker-2.example.com   Ready    elastic   4d2h   v1.18.3+2cf11e2
[root@openshift-jumpserver-0 ~]# oc get machineconfigpool
NAME      CONFIG                                              UPDATED   UPDATING   DEGRADED   MACHINECOUNT   READYMACHINECOUNT   UPDATEDMACHINECOUNT   DEGRADEDMACHINECOUNT   AGE
elastic   rendered-elastic-862e04ea44d8822c0343fcd1ec4a1fcb   True      False      False      1              1                   1                     0                      27h
master    rendered-master-0ac69c11e1e16fcd06a8ea95e2b605af    True      False      False      3              3                   3                     0                      4d2h
worker    rendered-worker-de2ff42fcefa2bef931fa94091019990    True      False      False      2              2                   2                     0                      4d2h
~~~

Connect to the nodes and make sure that the proxy configuration was pushed: 
~~~
[root@openshift-master-0 ~]# grep -i proxy /etc/systemd -R
/etc/systemd/system/multi-user.target.wants/machine-config-daemon-firstboot.service:Environment=HTTP_PROXY=http://192.168.123.1:3128
/etc/systemd/system/multi-user.target.wants/machine-config-daemon-firstboot.service:Environment=HTTPS_PROXY=http://192.168.123.1:3128
/etc/systemd/system/multi-user.target.wants/machine-config-daemon-firstboot.service:Environment=NO_PROXY=.cluster.local,.svc,10.0.0.0/16,127.0.0.1,172.24.0.0/14,172.30.0.0/16,192.168.123.0/24,api-int.cluster.example.com,etcd-0.cluster.example.com,etcd-1.cluster.example.com,etcd-2.cluster.example.com,example.com,localhost
/etc/systemd/system/machine-config-daemon-host.service.d/10-mco-default-env.conf:Environment=HTTP_PROXY=http://192.168.123.1:3128
/etc/systemd/system/machine-config-daemon-host.service.d/10-mco-default-env.conf:Environment=HTTPS_PROXY=http://192.168.123.1:3128
/etc/systemd/system/machine-config-daemon-host.service.d/10-mco-default-env.conf:Environment=NO_PROXY=.cluster.local,.svc,10.0.0.0/16,127.0.0.1,172.24.0.0/14,172.30.0.0/16,192.168.123.0/24,api-int.cluster.example.com,etcd-0.cluster.example.com,etcd-1.cluster.example.com,etcd-2.cluster.example.com,example.com,localhost
/etc/systemd/system/kubelet.service.requires/machine-config-daemon-firstboot.service:Environment=HTTP_PROXY=http://192.168.123.1:3128
/etc/systemd/system/kubelet.service.requires/machine-config-daemon-firstboot.service:Environment=HTTPS_PROXY=http://192.168.123.1:3128
/etc/systemd/system/kubelet.service.requires/machine-config-daemon-firstboot.service:Environment=NO_PROXY=.cluster.local,.svc,10.0.0.0/16,127.0.0.1,172.24.0.0/14,172.30.0.0/16,192.168.123.0/24,api-int.cluster.example.com,etcd-0.cluster.example.com,etcd-1.cluster.example.com,etcd-2.cluster.example.com,example.com,localhost
/etc/systemd/system/crio.service.d/10-mco-default-env.conf:Environment=HTTP_PROXY=http://192.168.123.1:3128
/etc/systemd/system/crio.service.d/10-mco-default-env.conf:Environment=HTTPS_PROXY=http://192.168.123.1:3128
/etc/systemd/system/crio.service.d/10-mco-default-env.conf:Environment=NO_PROXY=.cluster.local,.svc,10.0.0.0/16,127.0.0.1,172.24.0.0/14,172.30.0.0/16,192.168.123.0/24,api-int.cluster.example.com,etcd-0.cluster.example.com,etcd-1.cluster.example.com,etcd-2.cluster.example.com,example.com,localhost
/etc/systemd/system/kubelet.service.d/10-mco-default-env.conf:Environment=HTTP_PROXY=http://192.168.123.1:3128
/etc/systemd/system/kubelet.service.d/10-mco-default-env.conf:Environment=HTTPS_PROXY=http://192.168.123.1:3128
/etc/systemd/system/kubelet.service.d/10-mco-default-env.conf:Environment=NO_PROXY=.cluster.local,.svc,10.0.0.0/16,127.0.0.1,172.24.0.0/14,172.30.0.0/16,192.168.123.0/24,api-int.cluster.example.com,etcd-0.cluster.example.com,etcd-1.cluster.example.com,etcd-2.cluster.example.com,example.com,localhost
/etc/systemd/system/pivot.service.d/10-mco-default-env.conf:Environment=HTTP_PROXY=http://192.168.123.1:3128
/etc/systemd/system/pivot.service.d/10-mco-default-env.conf:Environment=HTTPS_PROXY=http://192.168.123.1:3128
/etc/systemd/system/pivot.service.d/10-mco-default-env.conf:Environment=NO_PROXY=.cluster.local,.svc,10.0.0.0/16,127.0.0.1,172.24.0.0/14,172.30.0.0/16,192.168.123.0/24,api-int.cluster.example.com,etcd-0.cluster.example.com,etcd-1.cluster.example.com,etcd-2.cluster.example.com,example.com,localhost
/etc/systemd/system/machine-config-daemon-firstboot.service:Environment=HTTP_PROXY=http://192.168.123.1:3128
/etc/systemd/system/machine-config-daemon-firstboot.service:Environment=HTTPS_PROXY=http://192.168.123.1:3128
/etc/systemd/system/machine-config-daemon-firstboot.service:Environment=NO_PROXY=.cluster.local,.svc,10.0.0.0/16,127.0.0.1,172.24.0.0/14,172.30.0.0/16,192.168.123.0/24,api-int.cluster.example.com,etcd-0.cluster.example.com,etcd-1.cluster.example.com,etcd-2.cluster.example.com,example.com,localhost
/etc/systemd/system/crio.service.requires/machine-config-daemon-firstboot.service:Environment=HTTP_PROXY=http://192.168.123.1:3128
/etc/systemd/system/crio.service.requires/machine-config-daemon-firstboot.service:Environment=HTTPS_PROXY=http://192.168.123.1:3128
/etc/systemd/system/crio.service.requires/machine-config-daemon-firstboot.service:Environment=NO_PROXY=.cluster.local,.svc,10.0.0.0/16,127.0.0.1,172.24.0.0/14,172.30.0.0/16,192.168.123.0/24,api-int.cluster.example.com,etcd-0.cluster.example.com,etcd-1.cluster.example.com,etcd-2.cluster.example.com,example.com,localhost
~~~

### Blocking traffic again ###

Now, once the cluster is stable and the proxy settings were pushed, REJECT unwanted traffic again. For example:
~~~
[root@openshift-jumpserver-0 ~]# iptables -I FORWARD --src 192.168.123.0/24  -p tcp --dport 80 --j REJECT
[root@openshift-jumpserver-0 ~]# iptables -I FORWARD --src 192.168.123.0/24 --dst 10.0.0.0/8,192.168.0.0/16,172.16.0.0/12   -p tcp --dport 80 --j ACCEPT
[root@openshift-jumpserver-0 ~]# iptables -I FORWARD --src 192.168.123.0/24  -p tcp --dport 443 --j REJECT
[root@openshift-jumpserver-0 ~]# iptables -I FORWARD --src 192.168.123.0/24 --dst 10.0.0.0/8,192.168.0.0/16,172.16.0.0/12   -p tcp --dport 443 --j ACCEPT
~~~

~~~
[root@openshift-jumpserver-0 ~]# iptables -L FORWARD -nv
Chain FORWARD (policy ACCEPT 4004K packets, 16G bytes)
 pkts bytes target     prot opt in     out     source               destination         
    0     0 ACCEPT     tcp  --  *      *       192.168.123.0/24     172.16.0.0/12        tcp dpt:443
    0     0 ACCEPT     tcp  --  *      *       192.168.123.0/24     192.168.0.0/16       tcp dpt:443
    0     0 ACCEPT     tcp  --  *      *       192.168.123.0/24     10.0.0.0/8           tcp dpt:443
    0     0 REJECT     tcp  --  *      *       192.168.123.0/24     0.0.0.0/0            tcp dpt:443 reject-with icmp-port-unreachable
    0     0 ACCEPT     tcp  --  *      *       192.168.123.0/24     172.16.0.0/12        tcp dpt:80
    0     0 ACCEPT     tcp  --  *      *       192.168.123.0/24     192.168.0.0/16       tcp dpt:80
    0     0 ACCEPT     tcp  --  *      *       192.168.123.0/24     10.0.0.0/8           tcp dpt:80
    0     0 REJECT     tcp  --  *      *       192.168.123.0/24     0.0.0.0/0            tcp dpt:80 reject-with icmp-port-unreachable
  13M   18G CNI-FORWARD  all  --  *      *       0.0.0.0/0            0.0.0.0/0            /* CNI firewall plugin rules */
4889K  420M ACCEPT     all  --  eth0   *       192.168.123.0/24     0.0.0.0/0 
~~~

Verify from a host:
~~~
[root@openshift-master-0 ~]# curl https://www.httpbin.org/get
curl: (7) Failed to connect to www.httpbin.org port 443: Connection refused
[root@openshift-master-0 ~]# 
[root@openshift-master-0 ~]# curl --proxy http://192.168.123.1:3128 https://www.httpbin.org/get
{
  "args": {}, 
  "headers": {
    "Accept": "*/*", 
    "Host": "www.httpbin.org", 
    "User-Agent": "curl/7.61.1", 
    "X-Amzn-Trace-Id": "Root=1-5f57d689-5ebe776493157066d0909100"
  }, 
  "origin": "66.187.232.131", 
  "url": "https://www.httpbin.org/get"
}
~~~

Make sure that you can run `oc debug/node`:
~~~
[root@openshift-jumpserver-0 ~]#  oc debug node/openshift-master-0.example.com 
Starting pod/openshift-master-0examplecom-debug ...
To use host binaries, run `chroot /host`
Pod IP: 192.168.123.200
If you don't see a command prompt, try pressing enter.
sh-4.2# 
sh-4.2# exit
exit

Removing debug pod ...
[root@openshift-jumpserver-0 ~]#  oc debug node/openshift-master-1.example.com 
Starting pod/openshift-master-1examplecom-debug ...
To use host binaries, run `chroot /host`
Pod IP: 192.168.123.201
If you don't see a command prompt, try pressing enter.
sh-4.2# exit
exit

Removing debug pod ...
[root@openshift-jumpserver-0 ~]#  oc debug node/openshift-master-2.example.com 
Starting pod/openshift-master-2examplecom-debug ...
To use host binaries, run `chroot /host`
Pod IP: 192.168.123.202
If you don't see a command prompt, try pressing enter.
sh-4.2# exit
exit

Removing debug pod ...
~~~

Make sure that you can run `oc adm must-gather`:
~~~
[root@openshift-jumpserver-0 ~]# oc adm must-gather
[must-gather      ] OUT Using must-gather plugin-in image: quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4dc41732afbccb4d882234749ac6031d8ec15715230df8058ec0ad7ccee31a94
[must-gather      ] OUT namespace/openshift-must-gather-tnj99 created
[must-gather      ] OUT clusterrolebinding.rbac.authorization.k8s.io/must-gather-bll5p created
[must-gather      ] OUT pod for plug-in image quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4dc41732afbccb4d882234749ac6031d8ec15715230df8058ec0ad7ccee31a94 created
[must-gather-fj4qr] POD Wrote inspect data to must-gather.
[must-gather-fj4qr] POD Gathering data for ns/openshift-cluster-version...
[must-gather-fj4qr] POD Wrote inspect data to must-gather.
[must-gather-fj4qr] POD Gathering data for ns/openshift-config...
[must-gather-fj4qr] POD Gathering data for ns/openshift-config-managed...
[must-gather-fj4qr] POD Gathering data for ns/openshift-authentication...
[must-gather-fj4qr] POD Gathering data for ns/openshift-authentication-operator...
[must-gather-fj4qr] POD Gathering data for ns/openshift-ingress...
[must-gather-fj4qr] POD Gathering data for ns/openshift-cloud-credential-operator...
[must-gather-fj4qr] POD Gathering data for ns/openshift-machine-api...
[must-gather-fj4qr] POD Gathering data for ns/openshift-config-operator...
[must-gather-fj4qr] POD Gathering data for ns/openshift-console-operator...
[must-gather-fj4qr] POD Gathering data for ns/openshift-console...
[must-gather-fj4qr] POD Gathering data for ns/openshift-cluster-storage-operator...
[must-gather-fj4qr] POD Gathering data for ns/openshift-dns-operator...
[must-gather-fj4qr] POD Gathering data for ns/openshift-dns...
[must-gather-fj4qr] POD Gathering data for ns/openshift-etcd-operator...
[must-gather-fj4qr] POD Gathering data for ns/openshift-etcd...
[must-gather-fj4qr] POD Gathering data for ns/openshift-image-registry...
[must-gather-fj4qr] POD Gathering data for ns/openshift-ingress-operator...
[must-gather-fj4qr] POD Gathering data for ns/openshift-insights...
(...)
~~~

Make sure that you can create a deployment:
~~~
[root@openshift-jumpserver-0 ~]# cat fedora.yaml 
apiVersion: apps/v1
kind: Deployment
metadata:
  name: fedora-deployment-user
  labels:
    app: fedora-deployment-user
spec:
  replicas: 3
  selector:
    matchLabels:
      app: fedora-deployment-user
  template:
    metadata:
      labels:
        app: fedora-deployment-user
    spec:
      containers:
      - name: fedora
        image: fedora
        command:
          - sleep
          - infinity
        imagePullPolicy: Always
~~~

While applying the deployment, run tshark to verify:
~~~

~~~

~~~
[root@openshift-jumpserver-0 ~]# oc get pods
NAME                                      READY   STATUS    RESTARTS   AGE
fedora-deployment-user-5df6fb4b4b-4nznr   1/1     Running   0          42s
fedora-deployment-user-5df6fb4b4b-dnpbn   1/1     Running   0          42s
fedora-deployment-user-5df6fb4b4b-zb64c   1/1     Running   0          42s
~~~

