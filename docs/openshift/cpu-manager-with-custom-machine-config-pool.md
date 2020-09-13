# CPU manager with custom MachineConfigPool in OCP 4.x #

## How to apply the CPU manager to only a subset of worker nodes ##

Create a custom MachineConfigPool named `worker-cpu-manager`.

`worker-cpu-manager.yaml`:
~~~
apiVersion: machineconfiguration.openshift.io/v1
kind: MachineConfigPool
metadata:
  name: worker-cpu-manager
  labels:
    custom-kubelet: cpumanager-enabled
spec:
  machineConfigSelector:
    matchExpressions:
      - {key: machineconfiguration.openshift.io/role, operator: In, values: [worker,worker-cpu-manager]}
  nodeSelector:
    matchLabels:
      node-role.kubernetes.io/worker-cpu-manager: ""
  paused: false
~~~

Apply the pool:
~~~
oc apply -f worker-cpu-manager.yaml
~~~

Create the `cpumanager-kubelet.yaml`:
~~~
apiVersion: machineconfiguration.openshift.io/v1
kind: KubeletConfig
metadata:
  name: cpumanager-enabled
spec:
  machineConfigPoolSelector:
    matchLabels:
      custom-kubelet: cpumanager-enabled
  kubeletConfig:
     cpuManagerPolicy: static
     cpuManagerReconcilePeriod: 5s
~~~

~~~
oc apply -f cpumanager-kubelet.yaml
~~~

Change worker node to `worker-cpu-manager` role:
~~~
oc label node <node> node-role.kubernetes.io/worker-cpu-manager=
~~~

Verify:
~~~
# oc get nodes
NAME                             STATUS   ROLES                       AGE     VERSION
openshift-master-0.example.com   Ready    master                      4h3m    v1.17.1+1aa1c48
openshift-master-1.example.com   Ready    master                      4h2m    v1.17.1+1aa1c48
openshift-master-2.example.com   Ready    master                      3h56m   v1.17.1+1aa1c48
openshift-worker-0.example.com   Ready    worker                      18m     v1.17.1+1aa1c48
openshift-worker-1.example.com   Ready    worker,worker-cpu-manager   18m     v1.17.1+1aa1c48
~~~

~~~
oc get machineconfig
(...)
rendered-worker-cpu-manager-bc48d7bf24df726f468b357482032845         8af4f709c4ba9c0afff3408ecc99c8fce61dd314   2.2.0             87s
rendered-worker-cpu-manager-ca0a09ddea41402490e1c39a138cd44e         8af4f709c4ba9c0afff3408ecc99c8fce61dd314   2.2.0             18m
~~~

~~~
[root@openshift-jumpserver-0 cpuman]# diff <(oc get machineconfig rendered-worker-cpu-manager-bc48d7bf24df726f468b357482032845 -o yaml)  <(oc get machineconfig rendered-worker-cpu-manager-ca0a09ddea41402490e1c39a138cd44e -o yaml)
6c6
<   creationTimestamp: "2020-06-26T16:40:44Z"
---
>   creationTimestamp: "2020-06-26T16:24:01Z"
8c8
<   name: rendered-worker-cpu-manager-bc48d7bf24df726f468b357482032845
---
>   name: rendered-worker-cpu-manager-ca0a09ddea41402490e1c39a138cd44e
16,18c16,18
<   resourceVersion: "451269"
<   selfLink: /apis/machineconfiguration.openshift.io/v1/machineconfigs/rendered-worker-cpu-manager-bc48d7bf24df726f468b357482032845
<   uid: bf950ef8-7270-4699-a113-dfe6c6c1d4fb
---
>   resourceVersion: "445466"
>   selfLink: /apis/machineconfiguration.openshift.io/v1/machineconfigs/rendered-worker-cpu-manager-ca0a09ddea41402490e1c39a138cd44e
>   uid: 74acdcd1-d195-48a5-8689-cecca137605c
163,168d162
<           verification: {}
<         filesystem: root
<         mode: 420
<         path: /etc/kubernetes/kubelet.conf
<       - contents:
<           source: data:text/plain,%7B%odcy%2(...)
~~~

Compare unmodified node to node with CPU manager:
~~~
[root@openshift-jumpserver-0 cpuman]# oc debug node/openshift-worker-0.example.com
Starting pod/openshift-worker-0examplecom-debug ...
To use host binaries, run `chroot /host`
Pod IP: 192.168.123.215
If you don't see a command prompt, try pressing enter.
sh-4.2# cat /host/etc/kubernetes/kubelet.conf | grep cpuManager
sh-4.2# exit
exit

Removing debug pod ...
~~~

~~~
[root@openshift-jumpserver-0 cpuman]# oc debug node/openshift-worker-1.example.com
Starting pod/openshift-worker-1examplecom-debug ...
To use host binaries, run `chroot /host`
Pod IP: 192.168.123.204
If you don't see a command prompt, try pressing enter.
sh-4.2# cat /host/etc/kubernetes/kubelet.conf | grep cpuManager
{"kind":"KubeletConfiguration","apiVersion":"kubelet.config.k8s.io/v1beta1","staticPodPath":"/etc/kubernetes/manifests","syncFrequency":"0s","fileCheckFrequency":"0s","httpCheckFrequency":"0s","rotateCertificates":true,"serverTLSBootstrap":true,"authentication":{"x509":{"clientCAFile":"/etc/kubernetes/kubelet-ca.crt"},"webhook":{"cacheTTL":"0s"},"anonymous":{"enabled":false}},"authorization":{"webhook":{"cacheAuthorizedTTL":"0s","cacheUnauthorizedTTL":"0s"}},"clusterDomain":"cluster.local","clusterDNS":["172.30.0.10"],"streamingConnectionIdleTimeout":"0s","nodeStatusUpdateFrequency":"0s","nodeStatusReportFrequency":"0s","imageMinimumGCAge":"0s","volumeStatsAggPeriod":"0s","systemCgroups":"/system.slice","cgroupRoot":"/","cgroupDriver":"systemd","cpuManagerPolicy":"static","cpuManagerReconcilePeriod":"5s","runtimeRequestTimeout":"0s","maxPods":250,"kubeAPIQPS":50,"kubeAPIBurst":100,"serializeImagePulls":false,"evictionPressureTransitionPeriod":"0s","featureGates":{"LegacyNodeRoleBehavior":false,"NodeDisruptionExclusion":true,"RotateKubeletServerCertificate":true,"SCTPSupport":true,"ServiceNodeExclusion":true,"SupportPodPidsLimit":true},"containerLogMaxSize":"50Mi","systemReserved":{"cpu":"500m","ephemeral-storage":"1Gi","memory":"1Gi"}}
sh-4.2# exit
exit

Removing debug pod ...
~~~

Spawn 2 pods on the same worker, one with and one without CPU manager enabled:
~~~
[root@openshift-jumpserver-0 cpuman]# cat cpumanager-pod.yaml 
apiVersion: v1
kind: Pod
metadata:
  name: cpumanager
spec:
  containers:
  - name: cpumanager
    image: gcr.io/google_containers/pause-amd64:3.0
    resources:
      requests:
        cpu: 1
        memory: "1G"
      limits:
        cpu: 1
        memory: "1G"
  nodeSelector:
    cpumanager: "true"
[root@openshift-jumpserver-0 cpuman]# cat non-cpumanager-pod.yaml 
apiVersion: v1
kind: Pod
metadata:
  name: noncpumanager
spec:
  containers:
  - name: cpumanager
    image: gcr.io/google_containers/pause-amd64:3.0
[root@openshift-jumpserver-0 cpuman]# oc get pods -o wide
NAME            READY   STATUS    RESTARTS   AGE     IP           NODE                             NOMINATED NODE   READINESS GATES
cpumanager      1/1     Running   0          11m     172.24.2.4   openshift-worker-1.example.com   <none>           <none>
noncpumanager   1/1     Running   0          8m10s   172.24.2.5   openshift-worker-1.example.com   <none>           <none>
~~~

Verify CPU pinning for the pods' CPUs:
~~~
[root@openshift-jumpserver-0 cpuman]# oc debug node/openshift-worker-1.example.com
Starting pod/openshift-worker-1examplecom-debug ...
To use host binaries, run `chroot /host`
systemctlPod IP: 192.168.123.204
If you don't see a command prompt, try pressing enter.
sh-4.2# chroot /host
sh-4.4# ps aux | grep pause
root       39072  0.0  0.0   1028     4 ?        Ss   16:55   0:00 /pause
root       47750  0.0  0.0   1028     4 ?        Ss   16:58   0:00 /pause
root       64183  0.0  0.0   9180   960 ?        S+   17:03   0:00 grep pause
sh-4.4# cat /proc/39072/status | grep -i cpu
Cpus_allowed:	00,00100000
Cpus_allowed_list:	20
sh-4.4# cat /proc/47750/status | grep -i cpu
Cpus_allowed:	ff,ffefffff
Cpus_allowed_list:	0-19,21-39
~~~

Alternatively:
~~~
sh-4.4# systemctl status 39072
● crio-ff43bfb551a274eb9e9040510753db6271ef27c13c203fe69e87aad5f7d49f17.scope - libcontainer container ff43bfb551a274eb9e9040510753db6271ef27c13c203fe69e87aad5f7d49f17
   Loaded: loaded (/run/systemd/transient/crio-ff43bfb551a274eb9e9040510753db6271ef27c13c203fe69e87aad5f7d49f17.scope; transient)
Transient: yes
   Active: active (running) since Fri 2020-06-26 16:55:34 UTC; 15min ago
    Tasks: 1 (limit: 1024)
   Memory: 1.4M (limit: 953.6M)
      CPU: 33ms
   CGroup: /kubepods.slice/kubepods-podc405f7cf_b2d1_49c4_bd4c_09f01a0b8e2d.slice/crio-ff43bfb551a274eb9e9040510753db6271ef27c13c203fe69e87aad5f7d49f17.scope
           └─39072 /pause

Jun 26 16:55:34 openshift-worker-1.example.com systemd[1]: Started libcontainer container ff43bfb551a274eb9e9040510753db6271ef27c13c203fe69e87aad5f7d49f17.
sh-4.4# systemctl status 47750
● crio-473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260.scope - libcontainer container 473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260
   Loaded: loaded (/run/systemd/transient/crio-473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260.scope; transient)
Transient: yes
   Active: active (running) since Fri 2020-06-26 16:58:32 UTC; 13min ago
    Tasks: 1 (limit: 1024)
   Memory: 1.0M
      CPU: 35ms
   CGroup: /kubepods.slice/kubepods-besteffort.slice/kubepods-besteffort-pod3b4059d0_8030_461a_b192_b5820f6c1119.slice/crio-473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260.scope
           └─47750 /pause

Jun 26 16:58:32 openshift-worker-1.example.com systemd[1]: Started libcontainer container 473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260.
sh-4.4# cat /sys/fs/cgroup/   
blkio/            cpu,cpuacct/      cpuset/           freezer/          memory/           net_cls,net_prio/ perf_event/       rdma/             
cpu/              cpuacct/          devices/          hugetlb/          net_cls/          net_prio/         pids/             systemd/          
sh-4.4# cat /sys/fs/cgroup/cpuset//kubepods.slice/kubepods-podc405f7cf_b2d1_49c4_bd4c_09f01a0b8e2d.slice/crio-ff43bfb551a274eb9e9040510753db6271ef27c13c203fe69e87aad5f7d49f17.scope/cpuset.cpus
20
sh-4.4# cat /sys/fs/cgroup/cpuset//kubepods.slice/kubepods-besteffort.slice/kubepods-besteffort-pod3b4059d0_8030_461a_b192_b5820f6c1119.slice/crio-473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260.scope/cpuset.pus
0-19,21-39
~~~

### Exploring resource limits ###

Let's spawn more pods than the hypervisor has CPUs:
~~~
[root@openshift-jumpserver-0 cpuman]# cat cpumanager-pod-generated-name.yaml 
apiVersion: v1
kind: Pod
metadata:
  generateName: cpumanager-
spec:
  containers:
  - name: cpumanager
    image: gcr.io/google_containers/pause-amd64:3.0
    resources:
      requests:
        cpu: 1
        memory: "1G"
      limits:
        cpu: 1
        memory: "1G"
  nodeSelector:
    cpumanager: "true"
~~~

~~~
for i in {0..45}; do oc create -f cpumanager-pod-generated-name.yaml ; done
~~~

~~~
[root@openshift-jumpserver-0 cpuman]# oc get pods | grep Pending
cpumanager-8rtpt   0/1     Pending   0          4m23s
cpumanager-bw848   0/1     Pending   0          4m24s
cpumanager-cm2st   0/1     Pending   0          4m24s
cpumanager-cwj82   0/1     Pending   0          4m23s
cpumanager-r5krw   0/1     Pending   0          4m24s
[root@openshift-jumpserver-0 cpuman]# oc describe node openshift-worker-1.example.com
Name:               openshift-worker-1.example.com
Roles:              worker-cpu-manager
Labels:             beta.kubernetes.io/arch=amd64
                    beta.kubernetes.io/os=linux
                    cpumanager=true
                    kubernetes.io/arch=amd64
                    kubernetes.io/hostname=openshift-worker-1.example.com
                    kubernetes.io/os=linux
                    node-role.kubernetes.io/worker-cpu-manager=
                    node.openshift.io/os_id=rhcos
Annotations:        k8s.ovn.org/l3-gateway-config:
                      {"default":{"mode":"local","interface-id":"br-local_openshift-worker-1.example.com","mac-address":"8e:c7:71:4d:0f:49","ip-addresses":["169...
                    k8s.ovn.org/node-chassis-id: 5a8880a6-5b50-4be5-9d84-f195bbc306a2
                    k8s.ovn.org/node-join-subnets: {"default":"100.64.3.0/29"}
                    k8s.ovn.org/node-mgmt-port-mac-address: 9a:b7:cd:ab:bf:b9
                    k8s.ovn.org/node-subnets: {"default":"172.24.2.0/23"}
                    machineconfiguration.openshift.io/currentConfig: rendered-worker-cpu-manager-bc48d7bf24df726f468b357482032845
                    machineconfiguration.openshift.io/desiredConfig: rendered-worker-cpu-manager-bc48d7bf24df726f468b357482032845
                    machineconfiguration.openshift.io/reason: 
                    machineconfiguration.openshift.io/state: Done
                    volumes.kubernetes.io/controller-managed-attach-detach: true
CreationTimestamp:  Thu, 25 Jun 2020 12:03:55 -0400
Taints:             <none>
Unschedulable:      false
Lease:
  HolderIdentity:  openshift-worker-1.example.com
  AcquireTime:     <unset>
  RenewTime:       Fri, 26 Jun 2020 14:14:05 -0400
Conditions:
  Type             Status  LastHeartbeatTime                 LastTransitionTime                Reason                       Message
  ----             ------  -----------------                 ------------------                ------                       -------
  MemoryPressure   False   Fri, 26 Jun 2020 14:10:56 -0400   Fri, 26 Jun 2020 12:43:22 -0400   KubeletHasSufficientMemory   kubelet has sufficient memory available
  DiskPressure     False   Fri, 26 Jun 2020 14:10:56 -0400   Fri, 26 Jun 2020 12:43:22 -0400   KubeletHasNoDiskPressure     kubelet has no disk pressure
  PIDPressure      False   Fri, 26 Jun 2020 14:10:56 -0400   Fri, 26 Jun 2020 12:43:22 -0400   KubeletHasSufficientPID      kubelet has sufficient PID available
  Ready            True    Fri, 26 Jun 2020 14:10:56 -0400   Fri, 26 Jun 2020 12:43:33 -0400   KubeletReady                 kubelet is posting ready status
Addresses:
  InternalIP:  192.168.123.204
  Hostname:    openshift-worker-1.example.com
Capacity:
  cpu:                40
  ephemeral-storage:  584946668Ki
  hugepages-1Gi:      0
  hugepages-2Mi:      0
  memory:             131924236Ki
  pods:               250
Allocatable:
  cpu:                39500m
  ephemeral-storage:  538013106513
  hugepages-1Gi:      0
  hugepages-2Mi:      0
  memory:             130773260Ki
  pods:               250
System Info:
  Machine ID:                             21668e85e1264ac78ea115b2fe79408e
  System UUID:                            4c4c4544-004b-5a10-8050-cac04f484832
  Boot ID:                                f20ee17e-226f-4f34-b9ce-965043215c2d
  Kernel Version:                         4.18.0-147.8.1.el8_1.x86_64
  OS Image:                               Red Hat Enterprise Linux CoreOS 44.81.202005250830-0 (Ootpa)
  Operating System:                       linux
  Architecture:                           amd64
  Container Runtime Version:              cri-o://1.17.4-12.dev.rhaos4.4.git2be4d9c.el8
  Kubelet Version:                        v1.17.1
  Kube-Proxy Version:                     v1.17.1
Non-terminated Pods:                      (49 in total)
  Namespace                               Name                                    CPU Requests  CPU Limits  Memory Requests  Memory Limits  AGE
  ---------                               ----                                    ------------  ----------  ---------------  -------------  ---
  default                                 cpumanager                              1 (2%)        1 (2%)      1G (0%)          1G (0%)        78m
  default                                 cpumanager-2548b                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        7m41s
  default                                 cpumanager-44nxg                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m29s
  default                                 cpumanager-5zx5v                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m30s
  default                                 cpumanager-72spc                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m28s
  default                                 cpumanager-9754w                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m29s
  default                                 cpumanager-cff7t                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        8m6s
  default                                 cpumanager-djhz9                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m27s
  default                                 cpumanager-dl7lf                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        7m42s
  default                                 cpumanager-dnz7k                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m28s
  default                                 cpumanager-drhf8                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m27s
  default                                 cpumanager-ff7h5                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m30s
  default                                 cpumanager-fh77s                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        7m40s
  default                                 cpumanager-fsptg                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m30s
  default                                 cpumanager-h74fq                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        7m38s
  default                                 cpumanager-hb72w                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m28s
  default                                 cpumanager-hbhz2                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m29s
  default                                 cpumanager-j7smp                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m28s
  default                                 cpumanager-jlwwn                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m30s
  default                                 cpumanager-jmdfk                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m30s
  default                                 cpumanager-kvfkp                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        7m45s
  default                                 cpumanager-kx4ft                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m28s
  default                                 cpumanager-kz6zg                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        7m43s
  default                                 cpumanager-m4qrn                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m31s
  default                                 cpumanager-mzqqs                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m30s
  default                                 cpumanager-n2tt5                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m28s
  default                                 cpumanager-n79js                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m31s
  default                                 cpumanager-qwjkf                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m27s
  default                                 cpumanager-sn658                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m29s
  default                                 cpumanager-tc7q6                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m29s
  default                                 cpumanager-vss6b                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m29s
  default                                 cpumanager-w846n                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m31s
  default                                 cpumanager-wchfq                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m30s
  default                                 cpumanager-whwxr                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m28s
  default                                 cpumanager-wkhw8                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m31s
  default                                 cpumanager-wtr5z                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m31s
  default                                 cpumanager-z7zgl                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m28s
  default                                 cpumanager-zckx8                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m30s
  default                                 cpumanager-zsnwv                        1 (2%)        1 (2%)      1G (0%)          1G (0%)        4m27s
  default                                 noncpumanager                           0 (0%)        0 (0%)      0 (0%)           0 (0%)         75m
  openshift-cluster-node-tuning-operator  tuned-tkhrp                             10m (0%)      0 (0%)      50Mi (0%)        0 (0%)         26h
  openshift-dns                           dns-default-q8hww                       110m (0%)     0 (0%)      70Mi (0%)        512Mi (0%)     26h
  openshift-image-registry                node-ca-sfjgq                           10m (0%)      0 (0%)      10Mi (0%)        0 (0%)         26h
  openshift-machine-config-operator       machine-config-daemon-8nw2s             40m (0%)      0 (0%)      100Mi (0%)       0 (0%)         26h
  openshift-marketplace                   certified-operators-74d989c4dd-w6l7f    10m (0%)      0 (0%)      100Mi (0%)       0 (0%)         50m
  openshift-monitoring                    node-exporter-nlgr5                     9m (0%)       0 (0%)      210Mi (0%)       0 (0%)         26h
  openshift-multus                        multus-w2sgc                            10m (0%)      0 (0%)      150Mi (0%)       0 (0%)         26h
  openshift-ovn-kubernetes                ovnkube-node-pg57x                      200m (0%)     0 (0%)      600Mi (0%)       0 (0%)         26h
  openshift-ovn-kubernetes                ovs-node-ktvtm                          100m (0%)     0 (0%)      300Mi (0%)       0 (0%)         26h
Allocated resources:
  (Total limits may be over 100 percent, i.e., overcommitted.)
  Resource           Requests           Limits
  --------           --------           ------
  cpu                39499m (99%)       39 (98%)
  memory             40667235840 (30%)  39536870912 (29%)
  ephemeral-storage  0 (0%)             0 (0%)
Events:
  Type     Reason                   Age                  From                                     Message
  ----     ------                   ----                 ----                                     -------
  Normal   NodeNotSchedulable       110m                 kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeNotSchedulable
  Normal   Starting                 106m                 kubelet, openshift-worker-1.example.com  Starting kubelet.
  Normal   NodeHasSufficientMemory  106m (x2 over 106m)  kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeHasSufficientMemory
  Normal   NodeHasNoDiskPressure    106m (x2 over 106m)  kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeHasNoDiskPressure
  Normal   NodeHasSufficientPID     106m (x2 over 106m)  kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeHasSufficientPID
  Warning  Rebooted                 106m                 kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com has been rebooted, boot id: 7df19e71-4ebc-4a66-94e8-1e475d11e095
  Normal   NodeNotReady             106m                 kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeNotReady
  Normal   NodeNotSchedulable       106m                 kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeNotSchedulable
  Normal   NodeAllocatableEnforced  106m                 kubelet, openshift-worker-1.example.com  Updated Node Allocatable limit across pods
  Normal   NodeReady                106m                 kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeReady
  Normal   NodeSchedulable          100m                 kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeSchedulable
  Normal   Starting                 90m                  kubelet, openshift-worker-1.example.com  Starting kubelet.
  Normal   NodeHasSufficientMemory  90m (x2 over 90m)    kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeHasSufficientMemory
  Normal   NodeHasNoDiskPressure    90m (x2 over 90m)    kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeHasNoDiskPressure
  Normal   NodeHasSufficientPID     90m (x2 over 90m)    kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeHasSufficientPID
  Warning  Rebooted                 90m                  kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com has been rebooted, boot id: f20ee17e-226f-4f34-b9ce-965043215c2d
  Normal   NodeNotReady             90m                  kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeNotReady
  Normal   NodeNotSchedulable       90m                  kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeNotSchedulable
  Normal   NodeAllocatableEnforced  90m                  kubelet, openshift-worker-1.example.com  Updated Node Allocatable limit across pods
  Normal   NodeReady                90m                  kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeReady
  Normal   NodeSchedulable          85m                  kubelet, openshift-worker-1.example.com  Node openshift-worker-1.example.com status is now: NodeSchedulable
~~~

Verify processes and cgroup limits on the worker node:
~~~
[root@openshift-jumpserver-0 cpuman]# oc debug node/openshift-worker-1.example.com
Starting pod/openshift-worker-1examplecom-debug ...
To use host binaries, run `chroot /host`
Pod IP: 192.168.123.204
If you don't see a command prompt, try pressing enter.
sh-4.2# chroot /host
sh-4.4# ps aux | grep pause
root       39072  0.0  0.0   1028     4 ?        Ss   16:55   0:00 /pause
root       47750  0.0  0.0   1028     4 ?        Ss   16:58   0:00 /pause
root      274649  0.0  0.0   1028     4 ?        Ss   18:06   0:00 /pause
root      276349  0.0  0.0   1028     4 ?        Ss   18:06   0:00 /pause
root      276526  0.0  0.0   1028     4 ?        Ss   18:06   0:00 /pause
root      276685  0.0  0.0   1028     4 ?        Ss   18:06   0:00 /pause
root      276824  0.0  0.0   1028     4 ?        Ss   18:06   0:00 /pause
root      276988  0.0  0.0   1028     4 ?        Ss   18:06   0:00 /pause
root      277279  0.0  0.0   1028     4 ?        Ss   18:06   0:00 /pause
root      291552  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      291674  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      291783  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      291900  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      292072  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      292145  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      292191  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      292638  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      292639  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      292693  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      292941  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      292949  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      292971  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      292986  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      293300  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      293422  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      293528  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      293615  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      293678  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      293823  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      293909  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      293951  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      293965  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      293983  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      294087  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      294164  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      294184  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      294239  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      294330  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      294352  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      294426  0.0  0.0   1028     4 ?        Ss   18:09   0:00 /pause
root      301832  0.0  0.0   9180   964 ?        R+   18:11   0:00 grep pause
sh-4.4# systemctl status 47750
● crio-473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260.scope - libcontainer container 473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260
   Loaded: loaded (/run/systemd/transient/crio-473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260.scope; transient)
Transient: yes
   Active: active (running) since Fri 2020-06-26 16:58:32 UTC; 1h 12min ago
    Tasks: 1 (limit: 1024)
   Memory: 1.0M
      CPU: 35ms
   CGroup: /kubepods.slice/kubepods-besteffort.slice/kubepods-besteffort-pod3b4059d0_8030_461a_b192_b5820f6c1119.slice/crio-473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260.scope
           └─47750 /pause

Jun 26 16:58:32 openshift-worker-1.example.com systemd[1]: Started libcontainer container 473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260.
sh-4.4# cat /sys/fs/cgroup/cpuset/kubepods.slice/kubepods-besteffort.slice/kubepods-besteffort-pod3b4059d0_8030_461a_b192_b5820f6c1119.slice/crio-473fd6bdac78596e0dbcf0d33dc11e11285725b4cf4e55410ce70dbdf088d260.scope/cpuset.cpus
0
sh-4.4# exit
exit
sh-4.2# exit
exit

Removing debug pod ...
~~~

Default host limits:
~~~
sh-4.4# cat /etc/kubernetes/kubelet.conf | jq '.systemReserved'
{
  "cpu": "500m",
  "ephemeral-storage": "1Gi",
  "memory": "1Gi"
}
~~~

### Applying custom limits ###

Let's now reserve 10 CPUs for the host.

`cpumanager-kubeletconfig.yaml`:
~~~
apiVersion: machineconfiguration.openshift.io/v1
kind: KubeletConfig
metadata:
  name: cpumanager-enabled
spec:
  machineConfigPoolSelector:
    matchLabels:
      custom-kubelet: cpumanager-enabled
  kubeletConfig:
     cpuManagerPolicy: static
     cpuManagerReconcilePeriod: 5s
     systemReserved:
       cpu: "10"
       memory: "1Gi"
       ephemeral-storage: "10Gi"
~~~

~~~
[root@openshift-jumpserver-0 cpuman]# oc apply -f cpumanager-kubeletconfig.yaml 
kubeletconfig.machineconfiguration.openshift.io/cpumanager-enabled configured
[root@openshift-jumpserver-0 cpuman]# oc get -o yaml -f cpumanager-kubeletconfig.yaml 
apiVersion: machineconfiguration.openshift.io/v1
kind: KubeletConfig
metadata:
  annotations:
    kubectl.kubernetes.io/last-applied-configuration: |
      {"apiVersion":"machineconfiguration.openshift.io/v1","kind":"KubeletConfig","metadata":{"annotations":{},"name":"cpumanager-enabled"},"spec":{"kubeletConfig":{"cpuManagerPolicy":"static","cpuManagerReconcilePeriod":"5s","systemReserved":{"cpu":"10","ephemeral-storage":"10Gi","memory":"1Gi"}},"machineConfigPoolSelector":{"matchLabels":{"custom-kubelet":"cpumanager-enabled"}}}}
  creationTimestamp: "2020-06-26T18:29:07Z"
  finalizers:
  - b5ac419d-573d-428c-adc1-f7b5bedb27e3
  - 21fa7ce4-49fa-470d-b4ed-f0972dbdd040
  generation: 2
  name: cpumanager-enabled
  resourceVersion: "484854"
  selfLink: /apis/machineconfiguration.openshift.io/v1/kubeletconfigs/cpumanager-enabled
  uid: 75d54d68-7b96-4b9e-8c9a-db3820b2629b
spec:
  kubeletConfig:
    cpuManagerPolicy: static
    cpuManagerReconcilePeriod: 5s
    systemReserved:
      cpu: "10"
      ephemeral-storage: 10Gi
      memory: 1Gi
  machineConfigPoolSelector:
    matchLabels:
      custom-kubelet: cpumanager-enabled
status:
  conditions:
  - lastTransitionTime: "2020-06-26T18:29:07Z"
    message: Success
    status: "True"
    type: Success
  - lastTransitionTime: "2020-06-26T18:33:14Z"
    message: Success
    status: "True"
    type: Success
[root@openshift-jumpserver-0 cpuman]# oc get nodes
NAME                             STATUS                     ROLES                AGE   VERSION
openshift-master-0.example.com   Ready                      master               27h   v1.17.1
openshift-master-1.example.com   Ready                      master               27h   v1.17.1
openshift-master-2.example.com   Ready                      master               27h   v1.17.1
openshift-worker-0.example.com   Ready                      worker               26h   v1.17.1
openshift-worker-1.example.com   Ready,SchedulingDisabled   worker-cpu-manager   26h   v1.17.1
[root@openshift-jumpserver-0 cpuman]# oc get machineconfig | grep worker-cpu-manager
99-worker-cpu-manager-ab32b145-ada3-4f96-adf9-1fb8388ba183-kubelet   8af4f709c4ba9c0afff3408ecc99c8fce61dd314   2.2.0             4m42s
rendered-worker-cpu-manager-171f3675f101028b058ffe27d6344bb2         8af4f709c4ba9c0afff3408ecc99c8fce61dd314   2.2.0             30s
rendered-worker-cpu-manager-bc48d7bf24df726f468b357482032845         8af4f709c4ba9c0afff3408ecc99c8fce61dd314   2.2.0             113m
rendered-worker-cpu-manager-ca0a09ddea41402490e1c39a138cd44e         8af4f709c4ba9c0afff3408ecc99c8fce61dd314   2.2.0             129m
~~~

~~~
[root@openshift-jumpserver-0 cpuman]# for i in {0..35}; do oc create -f cpumanager-pod-generated-name.yaml ; done
pod/cpumanager-msf8c created
pod/cpumanager-2kwws created
pod/cpumanager-7mx5k created
pod/cpumanager-bq7sf created
pod/cpumanager-86kkl created
pod/cpumanager-z7h5f created
pod/cpumanager-c5pd4 created
pod/cpumanager-sr6jt created
pod/cpumanager-dp7ck created
pod/cpumanager-n4ffv created
pod/cpumanager-5q77h created
pod/cpumanager-zxbj2 created
pod/cpumanager-7f8fw created
pod/cpumanager-j5kdw created
pod/cpumanager-fhfmw created
pod/cpumanager-hz5r8 created
pod/cpumanager-44xst created
pod/cpumanager-r6h98 created
pod/cpumanager-mjsfq created
pod/cpumanager-dh897 created
pod/cpumanager-s445q created
pod/cpumanager-brwcz created
pod/cpumanager-mnbrg created
pod/cpumanager-56vmv created
pod/cpumanager-kt5fg created
pod/cpumanager-fscpd created
pod/cpumanager-hsjzn created
pod/cpumanager-cvsth created
pod/cpumanager-j7wlv created
pod/cpumanager-z87tp created
pod/cpumanager-xggjm created
pod/cpumanager-jg646 created
pod/cpumanager-ml7kg created
pod/cpumanager-qwhjm created
pod/cpumanager-m6jw8 created
pod/cpumanager-f8qsh created
[root@openshift-jumpserver-0 cpuman]# oc get pods | grep Runn
cpumanager-2kwws   1/1     Running   0          15s
cpumanager-44xst   1/1     Running   0          13s
cpumanager-56vmv   1/1     Running   0          12s
cpumanager-5q77h   1/1     Running   0          14s
cpumanager-7f8fw   1/1     Running   0          14s
cpumanager-7mx5k   1/1     Running   0          15s
cpumanager-86kkl   1/1     Running   0          15s
cpumanager-8rtpt   1/1     Running   0          32m
cpumanager-bq7sf   1/1     Running   0          15s
cpumanager-brwcz   1/1     Running   0          13s
cpumanager-bw848   1/1     Running   0          32m
cpumanager-c5pd4   1/1     Running   0          14s
cpumanager-cm2st   1/1     Running   0          32m
cpumanager-cwj82   1/1     Running   0          32m
cpumanager-dh897   1/1     Running   0          13s
cpumanager-dp7ck   1/1     Running   0          14s
cpumanager-fhfmw   1/1     Running   0          13s
cpumanager-hz5r8   1/1     Running   0          13s
cpumanager-j5kdw   1/1     Running   0          14s
cpumanager-mjsfq   1/1     Running   0          13s
cpumanager-mnbrg   1/1     Running   0          12s
cpumanager-msf8c   1/1     Running   0          15s
cpumanager-n4ffv   1/1     Running   0          14s
cpumanager-r5krw   1/1     Running   0          32m
cpumanager-r6h98   1/1     Running   0          13s
cpumanager-s445q   1/1     Running   0          13s
cpumanager-sr6jt   1/1     Running   0          14s
cpumanager-z7h5f   1/1     Running   0          15s
cpumanager-zxbj2   1/1     Running   0          14s
[root@openshift-jumpserver-0 cpuman]# oc get pods | grep Runn | wc -l
29
~~~

Verify host limits:
~~~
sh-4.4# cat /etc/kubernetes/kubelet.conf | jq '.systemReserved'
{
  "cpu": "10",
  "ephemeral-storage": "10Gi",
  "memory": "1Gi"
}
~~~

## Reserving 0 CPUs for the host ##

This will not work - `cpumanager-kubeletconfig.yaml`:
~~~
apiVersion: machineconfiguration.openshift.io/v1
kind: KubeletConfig
metadata:
  name: cpumanager-enabled
spec:
  machineConfigPoolSelector:
    matchLabels:
      custom-kubelet: cpumanager-enabled
  kubeletConfig:
     cpuManagerPolicy: static
     cpuManagerReconcilePeriod: 5s
     systemReserved:
       cpu: "0"
       memory: "1Gi"
       ephemeral-storage: "10Gi"
~~~


Upon worker restart, the kubelet will not be able to start, reporting:
~~~
[root@openshift-worker-1 ~]# Jun 26 19:05:10 openshift-worker-1.example.com systemd[1]: Stopped Kubernetes Kubelet.
Jun 26 19:05:10 openshift-worker-1.example.com systemd[1]: kubelet.service: Consumed 115ms CPU time
Jun 26 19:05:10 openshift-worker-1.example.com systemd[1]: Starting Kubernetes Kubelet...
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: Flag --minimum-container-ttl-duration has been deprecated, Use --eviction-hard or --eviction-soft instead. Will be removed in a future version.
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421694    5492 flags.go:33] FLAG: --add-dir-header="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421726    5492 flags.go:33] FLAG: --address="0.0.0.0"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421741    5492 flags.go:33] FLAG: --allowed-unsafe-sysctls="[]"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421748    5492 flags.go:33] FLAG: --alsologtostderr="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421752    5492 flags.go:33] FLAG: --anonymous-auth="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421756    5492 flags.go:33] FLAG: --application-metrics-count-limit="100"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421759    5492 flags.go:33] FLAG: --authentication-token-webhook="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421763    5492 flags.go:33] FLAG: --authentication-token-webhook-cache-ttl="2m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421767    5492 flags.go:33] FLAG: --authorization-mode="AlwaysAllow"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421771    5492 flags.go:33] FLAG: --authorization-webhook-cache-authorized-ttl="5m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421775    5492 flags.go:33] FLAG: --authorization-webhook-cache-unauthorized-ttl="30s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421778    5492 flags.go:33] FLAG: --azure-container-registry-config=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421781    5492 flags.go:33] FLAG: --boot-id-file="/proc/sys/kernel/random/boot_id"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421787    5492 flags.go:33] FLAG: --bootstrap-checkpoint-path=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421790    5492 flags.go:33] FLAG: --bootstrap-kubeconfig="/etc/kubernetes/kubeconfig"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421793    5492 flags.go:33] FLAG: --cert-dir="/var/lib/kubelet/pki"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421797    5492 flags.go:33] FLAG: --cgroup-driver="cgroupfs"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421800    5492 flags.go:33] FLAG: --cgroup-root=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421803    5492 flags.go:33] FLAG: --cgroups-per-qos="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421806    5492 flags.go:33] FLAG: --chaos-chance="0"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421811    5492 flags.go:33] FLAG: --client-ca-file=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421814    5492 flags.go:33] FLAG: --cloud-config=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421817    5492 flags.go:33] FLAG: --cloud-provider=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421820    5492 flags.go:33] FLAG: --cluster-dns="[]"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421824    5492 flags.go:33] FLAG: --cluster-domain=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421828    5492 flags.go:33] FLAG: --cni-bin-dir="/opt/cni/bin"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421831    5492 flags.go:33] FLAG: --cni-cache-dir="/var/lib/cni/cache"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421834    5492 flags.go:33] FLAG: --cni-conf-dir="/etc/cni/net.d"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421838    5492 flags.go:33] FLAG: --config="/etc/kubernetes/kubelet.conf"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421842    5492 flags.go:33] FLAG: --container-hints="/etc/cadvisor/container_hints.json"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421845    5492 flags.go:33] FLAG: --container-log-max-files="5"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421849    5492 flags.go:33] FLAG: --container-log-max-size="10Mi"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421852    5492 flags.go:33] FLAG: --container-runtime="remote"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421865    5492 flags.go:33] FLAG: --container-runtime-endpoint="/var/run/crio/crio.sock"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421868    5492 flags.go:33] FLAG: --containerd="/run/containerd/containerd.sock"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421872    5492 flags.go:33] FLAG: --contention-profiling="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421876    5492 flags.go:33] FLAG: --cpu-cfs-quota="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421879    5492 flags.go:33] FLAG: --cpu-cfs-quota-period="100ms"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421883    5492 flags.go:33] FLAG: --cpu-manager-policy="none"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421886    5492 flags.go:33] FLAG: --cpu-manager-reconcile-period="10s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421889    5492 flags.go:33] FLAG: --docker="unix:///var/run/docker.sock"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421893    5492 flags.go:33] FLAG: --docker-endpoint="unix:///var/run/docker.sock"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421896    5492 flags.go:33] FLAG: --docker-env-metadata-whitelist=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421899    5492 flags.go:33] FLAG: --docker-only="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421902    5492 flags.go:33] FLAG: --docker-root="/var/lib/docker"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421905    5492 flags.go:33] FLAG: --docker-tls="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421908    5492 flags.go:33] FLAG: --docker-tls-ca="ca.pem"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421911    5492 flags.go:33] FLAG: --docker-tls-cert="cert.pem"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421914    5492 flags.go:33] FLAG: --docker-tls-key="key.pem"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421917    5492 flags.go:33] FLAG: --dynamic-config-dir=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421921    5492 flags.go:33] FLAG: --enable-cadvisor-json-endpoints="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421924    5492 flags.go:33] FLAG: --enable-controller-attach-detach="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421929    5492 flags.go:33] FLAG: --enable-debugging-handlers="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421932    5492 flags.go:33] FLAG: --enable-load-reader="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421951    5492 flags.go:33] FLAG: --enable-server="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421954    5492 flags.go:33] FLAG: --enforce-node-allocatable="[pods]"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421958    5492 flags.go:33] FLAG: --event-burst="10"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421961    5492 flags.go:33] FLAG: --event-qps="5"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421963    5492 flags.go:33] FLAG: --event-storage-age-limit="default=0"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421966    5492 flags.go:33] FLAG: --event-storage-event-limit="default=0"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421969    5492 flags.go:33] FLAG: --eviction-hard="imagefs.available<15%,memory.available<100Mi,nodefs.available<10%,nodefs.inodesFree<5%"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421979    5492 flags.go:33] FLAG: --eviction-max-pod-grace-period="0"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421982    5492 flags.go:33] FLAG: --eviction-minimum-reclaim=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421986    5492 flags.go:33] FLAG: --eviction-pressure-transition-period="5m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.421990    5492 flags.go:33] FLAG: --eviction-soft=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422000    5492 flags.go:33] FLAG: --eviction-soft-grace-period=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422003    5492 flags.go:33] FLAG: --exit-on-lock-contention="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422006    5492 flags.go:33] FLAG: --experimental-allocatable-ignore-eviction="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422009    5492 flags.go:33] FLAG: --experimental-bootstrap-kubeconfig="/etc/kubernetes/kubeconfig"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422012    5492 flags.go:33] FLAG: --experimental-check-node-capabilities-before-mount="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422015    5492 flags.go:33] FLAG: --experimental-dockershim="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422018    5492 flags.go:33] FLAG: --experimental-dockershim-root-directory="/var/lib/dockershim"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422021    5492 flags.go:33] FLAG: --experimental-kernel-memcg-notification="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422024    5492 flags.go:33] FLAG: --experimental-mounter-path=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422027    5492 flags.go:33] FLAG: --fail-swap-on="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422030    5492 flags.go:33] FLAG: --feature-gates=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422034    5492 flags.go:33] FLAG: --file-check-frequency="20s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422037    5492 flags.go:33] FLAG: --global-housekeeping-interval="1m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422040    5492 flags.go:33] FLAG: --hairpin-mode="promiscuous-bridge"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422043    5492 flags.go:33] FLAG: --healthz-bind-address="127.0.0.1"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422046    5492 flags.go:33] FLAG: --healthz-port="10248"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422049    5492 flags.go:33] FLAG: --help="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422052    5492 flags.go:33] FLAG: --hostname-override=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422055    5492 flags.go:33] FLAG: --housekeeping-interval="10s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422058    5492 flags.go:33] FLAG: --http-check-frequency="20s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422060    5492 flags.go:33] FLAG: --image-gc-high-threshold="85"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422063    5492 flags.go:33] FLAG: --image-gc-low-threshold="80"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422066    5492 flags.go:33] FLAG: --image-pull-progress-deadline="1m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422069    5492 flags.go:33] FLAG: --image-service-endpoint=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422072    5492 flags.go:33] FLAG: --iptables-drop-bit="15"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422075    5492 flags.go:33] FLAG: --iptables-masquerade-bit="14"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422078    5492 flags.go:33] FLAG: --keep-terminated-pod-volumes="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422081    5492 flags.go:33] FLAG: --kube-api-burst="10"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422084    5492 flags.go:33] FLAG: --kube-api-content-type="application/vnd.kubernetes.protobuf"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422087    5492 flags.go:33] FLAG: --kube-api-qps="5"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422090    5492 flags.go:33] FLAG: --kube-reserved=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422093    5492 flags.go:33] FLAG: --kube-reserved-cgroup=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422103    5492 flags.go:33] FLAG: --kubeconfig="/var/lib/kubelet/kubeconfig"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422106    5492 flags.go:33] FLAG: --kubelet-cgroups=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422109    5492 flags.go:33] FLAG: --lock-file=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422112    5492 flags.go:33] FLAG: --log-backtrace-at=":0"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422115    5492 flags.go:33] FLAG: --log-cadvisor-usage="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422118    5492 flags.go:33] FLAG: --log-dir=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422121    5492 flags.go:33] FLAG: --log-file=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422124    5492 flags.go:33] FLAG: --log-file-max-size="1800"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422127    5492 flags.go:33] FLAG: --log-flush-frequency="5s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422130    5492 flags.go:33] FLAG: --logtostderr="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422133    5492 flags.go:33] FLAG: --machine-id-file="/etc/machine-id,/var/lib/dbus/machine-id"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422136    5492 flags.go:33] FLAG: --make-iptables-util-chains="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422139    5492 flags.go:33] FLAG: --manifest-url=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422142    5492 flags.go:33] FLAG: --manifest-url-header=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422146    5492 flags.go:33] FLAG: --master-service-namespace="default"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422149    5492 flags.go:33] FLAG: --max-open-files="1000000"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422153    5492 flags.go:33] FLAG: --max-pods="110"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422156    5492 flags.go:33] FLAG: --maximum-dead-containers="-1"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422159    5492 flags.go:33] FLAG: --maximum-dead-containers-per-container="1"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422162    5492 flags.go:33] FLAG: --minimum-container-ttl-duration="6m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422165    5492 flags.go:33] FLAG: --minimum-image-ttl-duration="2m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422168    5492 flags.go:33] FLAG: --network-plugin=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422170    5492 flags.go:33] FLAG: --network-plugin-mtu="0"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422173    5492 flags.go:33] FLAG: --node-ip=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422176    5492 flags.go:33] FLAG: --node-labels="node-role.kubernetes.io/worker=,node.openshift.io/os_id=rhcos"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422182    5492 flags.go:33] FLAG: --node-status-max-images="50"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422185    5492 flags.go:33] FLAG: --node-status-update-frequency="10s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422188    5492 flags.go:33] FLAG: --non-masquerade-cidr="10.0.0.0/8"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422191    5492 flags.go:33] FLAG: --oom-score-adj="-999"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422194    5492 flags.go:33] FLAG: --pod-cidr=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422196    5492 flags.go:33] FLAG: --pod-infra-container-image="k8s.gcr.io/pause:3.1"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422200    5492 flags.go:33] FLAG: --pod-manifest-path=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422211    5492 flags.go:33] FLAG: --pod-max-pids="-1"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422214    5492 flags.go:33] FLAG: --pods-per-core="0"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422217    5492 flags.go:33] FLAG: --port="10250"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422219    5492 flags.go:33] FLAG: --protect-kernel-defaults="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422222    5492 flags.go:33] FLAG: --provider-id=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422225    5492 flags.go:33] FLAG: --qos-reserved=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422228    5492 flags.go:33] FLAG: --read-only-port="10255"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422231    5492 flags.go:33] FLAG: --really-crash-for-testing="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422234    5492 flags.go:33] FLAG: --redirect-container-streaming="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422237    5492 flags.go:33] FLAG: --register-node="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422240    5492 flags.go:33] FLAG: --register-schedulable="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422243    5492 flags.go:33] FLAG: --register-with-taints=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422246    5492 flags.go:33] FLAG: --registry-burst="10"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422249    5492 flags.go:33] FLAG: --registry-qps="5"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422252    5492 flags.go:33] FLAG: --reserved-cpus=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422255    5492 flags.go:33] FLAG: --resolv-conf="/etc/resolv.conf"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422258    5492 flags.go:33] FLAG: --root-dir="/var/lib/kubelet"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422261    5492 flags.go:33] FLAG: --rotate-certificates="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422264    5492 flags.go:33] FLAG: --rotate-server-certificates="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422266    5492 flags.go:33] FLAG: --runonce="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422269    5492 flags.go:33] FLAG: --runtime-cgroups="/system.slice/crio.service"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422272    5492 flags.go:33] FLAG: --runtime-request-timeout="2m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422275    5492 flags.go:33] FLAG: --seccomp-profile-root="/var/lib/kubelet/seccomp"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422278    5492 flags.go:33] FLAG: --serialize-image-pulls="true"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422281    5492 flags.go:33] FLAG: --skip-headers="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422284    5492 flags.go:33] FLAG: --skip-log-headers="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422287    5492 flags.go:33] FLAG: --stderrthreshold="2"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422290    5492 flags.go:33] FLAG: --storage-driver-buffer-duration="1m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422293    5492 flags.go:33] FLAG: --storage-driver-db="cadvisor"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422296    5492 flags.go:33] FLAG: --storage-driver-host="localhost:8086"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422299    5492 flags.go:33] FLAG: --storage-driver-password="root"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422301    5492 flags.go:33] FLAG: --storage-driver-secure="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422312    5492 flags.go:33] FLAG: --storage-driver-table="stats"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422315    5492 flags.go:33] FLAG: --storage-driver-user="root"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422318    5492 flags.go:33] FLAG: --streaming-connection-idle-timeout="4h0m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422321    5492 flags.go:33] FLAG: --sync-frequency="1m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422324    5492 flags.go:33] FLAG: --system-cgroups=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422326    5492 flags.go:33] FLAG: --system-reserved=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422329    5492 flags.go:33] FLAG: --system-reserved-cgroup=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422333    5492 flags.go:33] FLAG: --tls-cert-file=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422336    5492 flags.go:33] FLAG: --tls-cipher-suites="[]"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422340    5492 flags.go:33] FLAG: --tls-min-version=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422343    5492 flags.go:33] FLAG: --tls-private-key-file=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422345    5492 flags.go:33] FLAG: --topology-manager-policy="none"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422349    5492 flags.go:33] FLAG: --v="3"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422352    5492 flags.go:33] FLAG: --version="false"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422361    5492 flags.go:33] FLAG: --vmodule=""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422365    5492 flags.go:33] FLAG: --volume-plugin-dir="/etc/kubernetes/kubelet-plugins/volume/exec"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422369    5492 flags.go:33] FLAG: --volume-stats-agg-period="1m0s"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.422394    5492 feature_gate.go:244] feature gates: &{map[]}
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: Flag --minimum-container-ttl-duration has been deprecated, Use --eviction-hard or --eviction-soft instead. Will be removed in a future version.
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.423859    5492 feature_gate.go:244] feature gates: &{map[LegacyNodeRoleBehavior:false NodeDisruptionExclusion:true RotateKubeletServerCertificate:true SCTPSupport:true ServiceNodeExclusion:true SupportPodPidsLimit:true]}
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.423940    5492 feature_gate.go:244] feature gates: &{map[LegacyNodeRoleBehavior:false NodeDisruptionExclusion:true RotateKubeletServerCertificate:true SCTPSupport:true ServiceNodeExclusion:true SupportPodPidsLimit:true]}
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.433150    5492 mount_linux.go:168] Detected OS with systemd
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.433271    5492 server.go:424] Version: v1.17.1
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.433323    5492 feature_gate.go:244] feature gates: &{map[LegacyNodeRoleBehavior:false NodeDisruptionExclusion:true RotateKubeletServerCertificate:true SCTPSupport:true ServiceNodeExclusion:true SupportPodPidsLimit:true]}
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.433385    5492 feature_gate.go:244] feature gates: &{map[LegacyNodeRoleBehavior:false NodeDisruptionExclusion:true RotateKubeletServerCertificate:true SCTPSupport:true ServiceNodeExclusion:true SupportPodPidsLimit:true]}
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.433491    5492 plugins.go:100] No cloud provider specified.
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.433510    5492 server.go:540] No cloud provider specified: "" from the config file: ""
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.433519    5492 server.go:830] Client rotation is on, will bootstrap in background
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.445886    5492 bootstrap.go:84] Current kubeconfig file contents are still valid, no bootstrap necessary
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.445957    5492 certificate_store.go:129] Loading cert/key pair from "/var/lib/kubelet/pki/kubelet-client-current.pem".
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.446355    5492 server.go:857] Starting client certificate rotation.
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.446367    5492 certificate_manager.go:285] Certificate rotation is enabled.
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.446467    5492 certificate_manager.go:556] Certificate expiration is 2020-07-26 09:59:31 +0000 UTC, rotation deadline is 2020-07-20 19:25:44.002114354 +0000 UTC
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.446497    5492 certificate_manager.go:291] Waiting 576h20m33.555623075s for next certificate rotation
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.447060    5492 dynamic_cafile_content.go:129] Loaded a new CA Bundle and Verifier for "client-ca-bundle::/etc/kubernetes/kubelet-ca.crt"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.447171    5492 dynamic_cafile_content.go:167] Starting client-ca-bundle::/etc/kubernetes/kubelet-ca.crt
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.447357    5492 manager.go:146] cAdvisor running in container: "/sys/fs/cgroup/cpu,cpuacct/system.slice/kubelet.service"
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.448446    5492 fs.go:125] Filesystem UUIDs: map[00000000-0000-4000-a000-000000000002:/dev/sda4 1ccc3ac6-b59d-46bb-9850-229cd8b4e007:/dev/dm-0 676b3306-2850-478c-a817-3f723d49377d:/dev/sda1 D802-CD71:/dev/sda2]
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.448470    5492 fs.go:126] Filesystem partitions: map[/dev/mapper/coreos-luks-root-nocrypt:{mountpoint:/var major:253 minor:0 fsType:xfs blockSize:0} /dev/sda1:{mountpoint:/boot major:8 minor:1 fsType:ext4 blockSize:0} /dev/shm:{mountpoint:/dev/shm major:0 minor:22 fsType:tmpfs blockSize:0} /run:{mountpoint:/run major:0 minor:24 fsType:tmpfs blockSize:0} /run/user/1000:{mountpoint:/run/user/1000 major:0 minor:44 fsType:tmpfs blockSize:0} /sys/fs/cgroup:{mountpoint:/sys/fs/cgroup major:0 minor:25 fsType:tmpfs blockSize:0}]
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.453331    5492 manager.go:193] Machine: {NumCores:40 CpuFrequency:3100000 MemoryCapacity:135090417664 HugePages:[{PageSize:1048576 NumPages:0} {PageSize:2048 NumPages:0}] MachineID:21668e85e1264ac78ea115b2fe79408e SystemUUID:4c4c4544-004b-5a10-8050-cac04f484832 BootID:16f2699c-8a76-444a-8f56-e32057152db4 Filesystems:[{Device:/run/user/1000 DeviceMajor:0 DeviceMinor:44 Capacity:13509038080 Type:vfs Inodes:16490529 HasInodes:true} {Device:/dev/shm DeviceMajor:0 DeviceMinor:22 Capacity:67545206784 Type:vfs Inodes:16490529 HasInodes:true} {Device:/run DeviceMajor:0 DeviceMinor:24 Capacity:67545206784 Type:vfs Inodes:16490529 HasInodes:true} {Device:/sys/fs/cgroup DeviceMajor:0 DeviceMinor:25 Capacity:67545206784 Type:vfs Inodes:16490529 HasInodes:true} {Device:/dev/mapper/coreos-luks-root-nocrypt DeviceMajor:253 DeviceMinor:0 Capacity:598985388032 Type:vfs Inodes:292478400 HasInodes:true} {Device:/dev/sda1 DeviceMajor:8 DeviceMinor:1 Capacity:381549568 Type:vfs Inodes:98304 HasInodes:true}] DiskMap:map[253:0:{Name:dm-0 Major:253 Minor:0 Size:598995877376 Scheduler:none} 8:0:{Name:sda Major:8 Minor:0 Size:599550590976 Scheduler:mq-deadline}] NetworkDevices:[{Name:eno1 MacAddress:18:66:da:9f:b1:0a Speed:1000 Mtu:1500} {Name:eno2 MacAddress:18:66:da:9f:b1:0b Speed:-1 Mtu:1500} {Name:eno3 MacAddress:18:66:da:9f:b1:0c Speed:-1 Mtu:1500} {Name:eno4 MacAddress:18:66:da:9f:b1:0d Speed:1000 Mtu:1500} {Name:enp4s0f0 MacAddress:a0:36:9f:e5:e9:fc Speed:10000 Mtu:1500} {Name:enp4s0f1 MacAddress:a0:36:9f:e5:e9:fe Speed:10000 Mtu:1500} {Name:enp5s0f0 MacAddress:a0:36:9f:e5:e2:a8 Speed:10000 Mtu:1500} {Name:enp5s0f1 MacAddress:a0:36:9f:e5:e2:aa Speed:10000 Mtu:1500}] Topology:[{Id:0 Memory:67476070400 HugePages:[{PageSize:1048576 NumPages:0} {PageSize:2048 NumPages:0}] Cores:[{Id:0 Threads:[0 20] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:1 Threads:[2 22] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:2 Threads:[4 24] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:3 Threads:[6 26] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:4 Threads:[8 28] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:8 Threads:[10 30] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:9 Threads:[12 32] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:10 Threads:[14 34] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:11 Threads:[16 36] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:12 Threads:[18 38] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]}] Caches:[{Size:26214400 Type:Unified Level:3}]} {Id:1 Memory:67614347264 HugePages:[{PageSize:1048576 NumPages:0} {PageSize:2048 NumPages:0}] Cores:[{Id:0 Threads:[1 21] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:1 Threads:[3 23] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:2 Threads:[5 25] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:3 Threads:[7 27] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:4 Threads:[9 29] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:8 Threads:[11 31] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:9 Threads:[13 33] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:10 Threads:[15 35] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:11 Threads:[17 37] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]} {Id:12 Threads:[19 39] Caches:[{Size:32768 Type:Data Level:1} {Size:32768 Type:Instruction Level:1} {Size:262144 Type:Unified Level:2}]}] Caches:[{Size:26214400 Type:Unified Level:3}]}] CloudProvider:Unknown InstanceType:Unknown InstanceID:None}
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.453643    5492 manager.go:199] Version: {KernelVersion:4.18.0-147.8.1.el8_1.x86_64 ContainerOsVersion:Red Hat Enterprise Linux CoreOS 44.81.202005250830-0 (Ootpa) DockerVersion:Unknown DockerAPIVersion:Unknown CadvisorVersion: CadvisorRevision:}
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.454069    5492 container_manager_linux.go:265] container manager verified user specified cgroup-root exists: []
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.454102    5492 container_manager_linux.go:270] Creating Container Manager object based on Node Config: {RuntimeCgroupsName:/system.slice/crio.service SystemCgroupsName:/system.slice KubeletCgroupsName: ContainerRuntime:remote CgroupsPerQOS:true CgroupRoot:/ CgroupDriver:systemd KubeletRootDir:/var/lib/kubelet ProtectKernelDefaults:false NodeAllocatableConfig:{KubeReservedCgroupName: SystemReservedCgroupName: ReservedSystemCPUs: EnforceNodeAllocatable:map[pods:{}] KubeReserved:map[] SystemReserved:map[cpu:{i:{value:0 scale:0} d:{Dec:<nil>} s:0 Format:DecimalSI} ephemeral-storage:{i:{value:10737418240 scale:0} d:{Dec:<nil>} s:10Gi Format:BinarySI} memory:{i:{value:1073741824 scale:0} d:{Dec:<nil>} s:1Gi Format:BinarySI}] HardEvictionThresholds:[{Signal:memory.available Operator:LessThan Value:{Quantity:100Mi Percentage:0} GracePeriod:0s MinReclaim:<nil>} {Signal:nodefs.available Operator:LessThan Value:{Quantity:<nil> Percentage:0.1} GracePeriod:0s MinReclaim:<nil>} {Signal:nodefs.inodesFree Operator:LessThan Value:{Quantity:<nil> Percentage:0.05} GracePeriod:0s MinReclaim:<nil>} {Signal:imagefs.available Operator:LessThan Value:{Quantity:<nil> Percentage:0.15} GracePeriod:0s MinReclaim:<nil>}]} QOSReserved:map[] ExperimentalCPUManagerPolicy:static ExperimentalCPUManagerReconcilePeriod:5s ExperimentalPodPidsLimit:-1 EnforceCPULimits:true CPUCFSQuotaPeriod:100ms ExperimentalTopologyManagerPolicy:none}
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.454183    5492 fake_topology_manager.go:29] [fake topologymanager] NewFakeManager
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.454189    5492 container_manager_linux.go:305] Creating device plugin manager: true
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.454201    5492 manager.go:126] Creating Device Plugin manager at /var/lib/kubelet/device-plugins/kubelet.sock
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.454215    5492 fake_topology_manager.go:39] [fake topologymanager] AddHintProvider HintProvider:  &{kubelet.sock /var/lib/kubelet/device-plugins/ map[] {0 0} <nil> {{} [0 0 0]} 0x1b64c30 0x72dc9c8 0x1b65500 map[] map[] map[] map[] map[] 0xc000853ce0 [0 1] 0x72dc9c8}
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: I0626 19:05:10.454252    5492 cpu_manager.go:131] [cpumanager] detected CPU topology: &{40 20 2 map[0:{0 0 0} 1:{1 1 1} 2:{0 0 2} 3:{1 1 3} 4:{0 0 4} 5:{1 1 5} 6:{0 0 6} 7:{1 1 7} 8:{0 0 8} 9:{1 1 9} 10:{0 0 10} 11:{1 1 11} 12:{0 0 12} 13:{1 1 13} 14:{0 0 14} 15:{1 1 15} 16:{0 0 16} 17:{1 1 17} 18:{0 0 18} 19:{1 1 19} 20:{0 0 0} 21:{1 1 1} 22:{0 0 2} 23:{1 1 3} 24:{0 0 4} 25:{1 1 5} 26:{0 0 6} 27:{1 1 7} 28:{0 0 8} 29:{1 1 9} 30:{0 0 10} 31:{1 1 11} 32:{0 0 12} 33:{1 1 13} 34:{0 0 14} 35:{1 1 15} 36:{0 0 16} 37:{1 1 17} 38:{0 0 18} 39:{1 1 19}]}
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: E0626 19:05:10.454300    5492 container_manager_linux.go:329] failed to initialize cpu manager: [cpumanager] unable to determine reserved CPU resources for static policy
Jun 26 19:05:10 openshift-worker-1.example.com hyperkube[5492]: F0626 19:05:10.454309    5492 server.go:273] failed to run Kubelet: [cpumanager] unable to determine reserved CPU resources for static policy
Jun 26 19:05:10 openshift-worker-1.example.com systemd[1]: kubelet.service: Main process exited, code=exited, status=255/n/a
Jun 26 19:05:10 openshift-worker-1.example.com systemd[1]: kubelet.service: Failed with result 'exit-code'.
Jun 26 19:05:10 openshift-worker-1.example.com systemd[1]: Failed to start Kubernetes Kubelet.
Jun 26 19:05:10 openshift-worker-1.example.com systemd[1]: kubelet.service: Consumed 117ms CPU time
~~~

And the node will never become `Ready` in the node list.

## References ##
* [https://www.redhat.com/en/blog/openshift-container-platform-4-how-does-machine-config-pool-work](https://www.redhat.com/en/blog/openshift-container-platform-4-how-does-machine-config-pool-work)
* [https://docs.openshift.com/container-platform/4.4/scalability_and_performance/using-cpu-manager.html](https://docs.openshift.com/container-platform/4.4/scalability_and_performance/using-cpu-manager.html)
* [https://kubernetes.io/docs/tasks/administer-cluster/cpu-management-policies/](https://kubernetes.io/docs/tasks/administer-cluster/cpu-management-policies/)
