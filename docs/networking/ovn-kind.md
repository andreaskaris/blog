# OVN kind and the 

## Sources

Based on:

* [https://github.com/ovn-org/ovn-kubernetes/blob/master/docs/kind.md](https://github.com/ovn-org/ovn-kubernetes/blob/master/docs/kind.md)

* [https://github.com/ovn-org/ovn-kubernetes](https://github.com/ovn-org/ovn-kubernetes)

And most instructions are from my colleagues Tim Rozet and Robin Cernin.

## Prerequisites

Spawn a RHEL 7.8 or above virtual machine and install:
~~~
yum install docker -y
yum install python3 -y
yum install git -y
yum install python3-pip -y
python3 -m pip install j2cli
sudo update-alternatives --install /usr/bin/pip pip /usr/bin/pip3 1
curl -O https://golang.org/dl/go1.14.6.linux-amd64.tar.gz -L
tar -C /usr/local -xzf go1.14.6.linux-amd64.tar.gz
cat <<'EOF' | tee /etc/profile.d/go.sh
#!/bin/bash

export PATH=$PATH:/usr/local/go/bin
EOF
chmod +x /etc/profile.d/go.sh
source  /etc/profile.d/go.sh
~~~

Download kind:
~~~
curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.8.1/kind-linux-amd64
chmod +x ./kind
~~~
> For further details, see https://kind.sigs.k8s.io/docs/user/quick-start/

Download ovn-kubernetes:
~~~
git clone https://github.com/ovn-org/ovn-kubernetes
cd ovn-kubernetes/contrib/
~~~

Now, run:
~~~
./kind.sh -ho
~~~

Wait until it finishes.

Now, go to the home folder and generate kubeconfig:
~~~
cd ~
kind get kubeconfig --name ovn > .kube/config
~~~

Verify:
~~~
 kubectl  get nodes
NAME                STATUS   ROLES    AGE   VERSION
ovn-control-plane   Ready    master   15m   v1.18.2
ovn-worker          Ready    <none>   14m   v1.18.2
ovn-worker2         Ready    <none>   14m   v1.18.2
~~~

Get your Worker node IPs:
~~~
# kubectl  get nodes -o wide
NAME                STATUS   ROLES    AGE   VERSION   INTERNAL-IP   EXTERNAL-IP   OS-IMAGE           KERNEL-VERSION                 CONTAINER-RUNTIME
ovn-control-plane   Ready    master   15m   v1.18.2   172.18.0.4    <none>        Ubuntu 20.04 LTS   4.18.0-193.13.2.el8_2.x86_64   containerd://1.3.3-14-g449e9269
ovn-worker          Ready    <none>   15m   v1.18.2   172.18.0.3    <none>        Ubuntu 20.04 LTS   4.18.0-193.13.2.el8_2.x86_64   containerd://1.3.3-14-g449e9269
ovn-worker2         Ready    <none>   15m   v1.18.2   172.18.0.2    <none>        Ubuntu 20.04 LTS   4.18.0-193.13.2.el8_2.x86_64   containerd://1.3.3-14-g449e9269
~~~

Get the worker pod subnets:
~~~
[root@kind ~]# kubectl get nodes | tail -n +2 | awk '{print $1}' | while read node; do echo === $node === ; kubectl describe node $node | grep PodCIDR ; done
=== ovn-control-plane ===
PodCIDR:                      10.244.0.0/24
PodCIDRs:                     10.244.0.0/24
=== ovn-worker ===
PodCIDR:                      10.244.1.0/24
PodCIDRs:                     10.244.1.0/24
=== ovn-worker2 ===
PodCIDR:                      10.244.2.0/24
PodCIDRs:                     10.244.2.0/24
~~~


Create CentOS to act as GW:
~~~
docker run --name gw -d --privileged --network kind -it centos /bin/bash
~~~

Now connecto to the GW node:
~~~
docker exec -it gw /bin/bash
~~~

If your worker node IPs and pod subnets are different from what was listed above, make sure to adjust the following.
It's important to route traffic for the node's pod subnet via the node's internal IP.
~~~
cat <<'EOF' | tee vxlan.sh
#!/bin/bash

echo "VIP"
ip add a 9.0.0.1 dev lo

echo "ovn-workers"
# https://joejulian.name/post/how-to-configure-linux-vxlans-with-multiple-unicast-endpoints/
ip link add vxlan0 type vxlan dev eth0 id 4097 dstport 4789       
bridge fdb append to 00:00:00:00:00:00 dst 172.18.0.3 dev vxlan0
bridge fdb append to 00:00:00:00:00:00 dst 172.18.0.2 dev vxlan0
ip route add 10.244.0.0/16 dev vxlan0
EOF
chmod +x vxlan.sh
./vxlan.sh
~~~

Now create a namespace with annotations like:
~~~
apiVersion: v1
kind: Namespace
metadata:
  name: exgw2
  annotations:
    k8s.ovn.org/hybrid-overlay-external-gw: 9.0.0.1
    k8s.ovn.org/hybrid-overlay-vtep: 172.18.0.5 #replace this with your simulated  ip
~~~

Execute:
~~~
kubectl apply -f file.yaml
~~~

Now create pods on each worker node in the namespace:
~~~
cat <<'EOF' | oc apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: dnsutils
  namespace: exgw2
spec:
  containers:
  - name: dnsutils01
    image: gcr.io/kubernetes-e2e-test-images/dnsutils:1.3
    command:
      - sleep
      - "3600"
    imagePullPolicy: IfNotPresent
  restartPolicy: Always
  nodeName: ovn-worker
---
apiVersion: v1
kind: Pod
metadata:
  name: dnsutils02
  namespace: exgw2
spec:
  containers:
  - name: dnsutils
    image: gcr.io/kubernetes-e2e-test-images/dnsutils:1.3
    command:
      - sleep
      - "3600"
    imagePullPolicy: IfNotPresent
  restartPolicy: Always
  nodeName: ovn-worker2
EOF
~~~

Execute:
~~~
[root@kind ~]# kubectl  get pods -n exgw2 -o wide
NAME         READY   STATUS    RESTARTS   AGE   IP           NODE          NOMINATED NODE   READINESS GATES
dnsutils     1/1     Running   0          20m   10.244.0.7   ovn-worker    <none>           <none>
dnsutils02   1/1     Running   0          20m   10.244.2.6   ovn-worker2   <none>           <none>
~~~

Verify from the pods:
~~~
[root@kind ~]# kubectl exec -it dnsutils02 /bin/sh
kubectl exec [POD] [COMMAND] is DEPRECATED and will be removed in a future version. Use kubectl kubectl exec [POD] -- [COMMAND] instead.
/ # ping 9.0.0.1
PING 9.0.0.1 (9.0.0.1): 56 data bytes
64 bytes from 9.0.0.1: seq=0 ttl=64 time=0.998 ms
^C
--- 9.0.0.1 ping statistics ---
1 packets transmitted, 1 packets received, 0% packet loss
round-trip min/avg/max = 0.998/0.998/0.998 ms
/ # 
~~~

~~~
[root@kind ~]# kubectl exec -it dnsutils /bin/sh
kubectl exec [POD] [COMMAND] is DEPRECATED and will be removed in a future version. Use kubectl kubectl exec [POD] -- [COMMAND] instead.
/ # ping 9.0.0.1
PING 9.0.0.1 (9.0.0.1): 56 data bytes
64 bytes from 9.0.0.1: seq=0 ttl=64 time=1.686 ms
64 bytes from 9.0.0.1: seq=1 ttl=64 time=0.667 ms
^C
--- 9.0.0.1 ping statistics ---
2 packets transmitted, 2 packets received, 0% packet loss
round-trip min/avg/max = 0.667/1.176/1.686 ms
/ # 
~~~

Verify the HybridOverlay is enabled in NS:
~~~
# kubectl get ns exgw2 -o yaml
apiVersion: v1
kind: Namespace
metadata:
  annotations:
    k8s.ovn.org/hybrid-overlay-external-gw: 9.0.0.1
    k8s.ovn.org/hybrid-overlay-vtep: 172.18.0.5
~~~
