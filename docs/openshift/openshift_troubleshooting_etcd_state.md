# Troubleshooting etcd state #

### Troubleshooting steps ###

~~~
[akaris@linux 00000001]$ oc get pods -A | grep etcd-member
openshift-etcd                                          etcd-member-cluste-xxxxx-m-1.c.akaris-00000001.internal                2/2     Running     0          3h49m
openshift-etcd                                          etcd-member-cluste-xxxxx-m-2.c.akaris-00000001.internal                2/2     Running     0          3h49m
[akaris@linux 00000001]$ oc exec -n openshift-etcd -it etcd-member-cluste-xxxxx-m-1.c.akaris-00000001.internal /bin/bash
Defaulting container name to etcd-member.
Use 'oc describe pod/etcd-member-cluste-xxxxx-m-1.c.akaris-00000001.internal -n openshift-etcd' to see all of the containers in this pod.
[root@cluste-xxxxx-m-1 /]# export ETCDCTL_API=3 ETCDCTL_CACERT=/etc/ssl/etcd/ca.crt ETCDCTL_CERT=$(find /etc/ssl/ -name *peer*crt) ETCDCTL_KEY=$(find /etc/ssl/ -name *peer*key)
[root@cluste-xxxxx-m-1 /]#  etcdctl member list -w table
+------------------+---------+---------------------------------------------------------+-------------------------------------------------+-----------------------+
|        ID        | STATUS  |                          NAME                           |                   PEER ADDRS                    |     CLIENT ADDRS      |
+------------------+---------+---------------------------------------------------------+-------------------------------------------------+-----------------------+
| 8338e0f35d1ea667 | started | etcd-member-cluste-xxxxx-m-0.c.akaris-00000001.internal | https://etcd-0.cluster.ocptest.example.net:2380 | https://10.0.0.5:2379 |
| 87cd63e5849290c1 | started | etcd-member-cluste-xxxxx-m-2.c.akaris-00000001.internal | https://etcd-2.cluster.ocptest.example.net:2380 | https://10.0.0.3:2379 |
| f2a4a192cecf2103 | started | etcd-member-cluste-xxxxx-m-1.c.akaris-00000001.internal | https://etcd-1.cluster.ocptest.example.net:2380 | https://10.0.0.4:2379 |
+------------------+---------+---------------------------------------------------------+-------------------------------------------------+-----------------------+
[root@cluste-xxxxx-m-1 /]# etcdctl endpoint status --endpoints=$(etcdctl member list | cut -d, -f5 | sed -e 's/ //g' | paste -sd ',') --write-out table
{"level":"warn","ts":"2020-02-18T12:46:55.693Z","caller":"clientv3/retry_interceptor.go:61","msg":"retrying of unary invoker failed","target":"passthrough:///https://10.0.0.5:2379","attempt":0,"error":"rpc error: code = DeadlineExceeded desc = context deadline exceeded"}
Failed to get the status of endpoint https://10.0.0.5:2379 (context deadline exceeded)
+-----------------------+------------------+---------+---------+-----------+-----------+------------+
|       ENDPOINT        |        ID        | VERSION | DB SIZE | IS LEADER | RAFT TERM | RAFT INDEX |
+-----------------------+------------------+---------+---------+-----------+-----------+------------+
| https://10.0.0.3:2379 | 87cd63e5849290c1 |  3.3.17 |   69 MB |     false |        11 |      88235 |
| https://10.0.0.4:2379 | f2a4a192cecf2103 |  3.3.17 |   68 MB |      true |        11 |      88235 |
+-----------------------+------------------+---------+---------+-----------+-----------+------------+
[root@cluste-xxxxx-m-1 /]# etcdctl endpoint health --endpoints=$(etcdctl member list | cut -d, -f5 | sed -e 's/ //g' | paste -sd ',') --write-out table
{"level":"warn","ts":"2020-02-18T12:47:08.048Z","caller":"clientv3/retry_interceptor.go:61","msg":"retrying of unary invoker failed","target":"endpoint://client-01c6d062-363d-44cf-b92b-3e5356ed2579/10.0.0.5:2379","attempt":0,"error":"rpc error: code = DeadlineExceeded desc = context deadline exceeded"}
+-----------------------+--------+--------------+---------------------------+
|       ENDPOINT        | HEALTH |     TOOK     |           ERROR           |
+-----------------------+--------+--------------+---------------------------+
| https://10.0.0.4:2379 |   true |  23.353459ms |                           |
| https://10.0.0.3:2379 |   true |  22.503798ms |                           |
| https://10.0.0.5:2379 |  false | 5.000398701s | context deadline exceeded |
+-----------------------+--------+--------------+---------------------------+
Error: unhealthy cluster
[root@cluste-xxxxx-m-1 /]# etcdctl alarm list
[root@cluste-xxxxx-m-1 /]# 
~~~

### Resources ###

* [https://rancher.com/docs/rancher/v2.x/en/troubleshooting/kubernetes-components/etcd/#etcd-cluster-and-connectivity-checks](https://rancher.com/docs/rancher/v2.x/en/troubleshooting/kubernetes-components/etcd/#etcd-cluster-and-connectivity-checks)
