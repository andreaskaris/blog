## CPU isolation in Red Hat OpenShift Container Platform

Two complementary features allow admins to partition the node's CPUs according to their needs. The first is
the Kubernetes `Static CPU Manager` feature. The second is the OpenShift only `Management workload partitioning` feature.

### Static CPU manager

[The static CPU manager](https://kubernetes.io/docs/tasks/administer-cluster/cpu-management-policies/#static-policy)
partitions the node into system reserved CPUs (kubelet and system services) and non system
reserved CPUs:

```
+----------------------+------------------------------+
| system reserved CPUs | non system reserved CPUs     |
+----------------------+------------------------------+
```

In order to achieve such separation, administrators can configure a
[PerformanceProfile](https://docs.openshift.com/container-platform/4.14/scalability_and_performance/cnf-create-performance-profiles.html)

```
apiVersion: performance.openshift.io/v2
kind: PerformanceProfile
metadata:
  name: performance
spec:
  cpu:
    isolated: 20-111
    reserved: 0-19
(...)
```

The Node Tuning Operator will then push the following kubelet configuration, in addition to configuring operating
system wide CPU isolation with systemd configuration and kernel command line parameters:
```
# grep -E '"cpuManagerPolicy"|"reservedSystemCPUs"' /etc/kubernetes/kubelet.conf
  "cpuManagerPolicy": "static",
  "reservedSystemCPUs": "0-19",
```

With the static CPU manager in place, normal pods will run in the shared CPU pool which initially spans all CPUs:
```
+----------------------+------------------------------+
| system reserved CPUs | non system reserved CPUs     |
+----------------------+------------------------------+
| shared pool          | shared pool                  |
+----------------------+------------------------------+
```

As exclusive CPUs are requested (by QoS Guaranteed pods with integer number CPU requests), CPUs are taken in ascending
order from the non system reserved shared pool and they are moved to the list of exclusive CPUs:
```
+----------------------+------------------------------+
| system reserved CPUs | non system reserved CPUs     |
+----------------------+------------------------------+
| shared pool          | exclusive CPUs | shared pool |
+----------------------+------------------------------+
                         ------------->
```

Eventually, if all available CPUs are reserved, the system will consist of a shared pool which consists of all system
reserved CPUs and exclusive CPUs which take up all non system reserved CPUs:
```
+----------------------+------------------------------+
| system reserved CPUs | non system reserved CPUs     |
+----------------------+------------------------------+
| shared pool          | exclusive CPUs               |
+----------------------+------------------------------+
```

#### Pods with mixed exclusive and shared containers

With the static CPU manager, it is possible to run pods which are QoS guaranteed and yet not all of ttheir containers
need to have exclusive CPU requests. Take the following example:

```
apiVersion: apps/v1
kind: Deployment
metadata:
  creationTimestamp: null
  labels:
    app: fedora-minimal
  name: fedora-minimal
spec:
  replicas: 1
  selector:
    matchLabels:
      app: fedora-minimal
  strategy: {}
  template:
    metadata:
      creationTimestamp: null
      labels:
        app: fedora-minimal
    spec:
      containers:
      - image: registry.fedoraproject.org/fedora-minimal:latest
        name: fedora-minimal-exclusive
        command:
          - /bin/sleep
          - "12345"
        resources:
          requests:
            memory: "128Mi"
            cpu: "2"
          limits:
            memory: "128Mi"
            cpu: "2"
      - image: registry.fedoraproject.org/fedora-minimal:latest
        name: fedora-minimal-shared
        command:
          - /bin/sleep
          - "54321"
        resources:
          requests:
            memory: "128Mi"
            cpu: "1200m"
          limits:
            memory: "128Mi"
            cpu: "1200m"
```

Because CPU and memory requests are the same for each container, the pod is of QoS class guaranteed after deployment:

```
$ oc get pods --output=custom-columns="NAME:.metadata.name,STATUS:.status.qosClass"
NAME                              STATUS
fedora-minimal-78cc494dd8-rxsvn   Guaranteed
```

The container with integer CPU requests will reserve CPUs from the exclusive set:

```
$ oc exec fedora-minimal-78cc494dd8-rxsvn -c fedora-minimal-exclusive -- grep -i cpus /proc/self/status
Cpus_allowed:	00000080,00000080
Cpus_allowed_list:	7,39
```

Whereas the container with non-integer CPU requests will use the shared CPU pool:

```
$ oc exec fedora-minimal-78cc494dd8-rxsvn -c fedora-minimal-shared -- grep -i cpus /proc/self/status
Cpus_allowed:	ffffff43,ffffff43
Cpus_allowed_list:	0-1,6,8-33,38,40-63
```

### Management workload partitioning

In small clusters, especially in Single Node deployments, administrators might want additional control over how their
nodes are partitioned. Management workload partitioning allows management workloads in OpenShift clusters to run
exclusively on the system reserved CPUs. This way, these workloads can be isolated away from non-management
workloads which will run exclusively on non system reserved CPUs.

```
+----------------------+------------------------------+
| system reserved CPUs | non system reserved CPUs     |
+----------------------+------------------------------+
| management workloads | exclusive CPUs | shared pool |
+----------------------+------------------------------+
```

According to the enhancement proposal for
[Management workload partitioning](https://github.com/openshift/enhancements/blob/master/enhancements/workload-partitioning/management-workload-partitioning.md#high-level-end-to-end-workflow):

> We want to define "management workloads" in a flexible way. (...) "management workloads" include all OpenShift core
components necessary to run the cluster, any add-on operators necessary to make up the "platform" as defined by telco
customers, and operators or other components from third-party vendors that the customer deems as management rather than
operational.  It is important to note that not all of those components will be delivered as part of the OpenShift
payload and some may be written by the customer or by vendors who are not our partners. Therefore, while this feature
will be released as Tech Preview initially with limited formal support, the APIs described are not internal or otherwise
private to OpenShift.

At time of this writing, management workload partitioning can only be enabled during cluster installation by setting
`cpuPartitioningMode: AllNodes` in the `install-config.yaml` file. See
[the workload partitioning documentation](https://docs.openshift.com/container-platform/4.14/scalability_and_performance/enabling-workload-partitioning.html)
for further details. For further details behind the reasoning, you can also have a look at the enhancement proposal for
[Wide Availability Workload Partitioning](https://github.com/openshift/enhancements/blob/master/enhancements/workload-partitioning/wide-availability-workload-partitioning.md).

After cluster installation, create a `PerformanceProfile`, the same as mentioned above:
```
apiVersion: performance.openshift.io/v2
kind: PerformanceProfile
metadata:
  name: performance
spec:
  cpu:
    isolated: 20-111
    reserved: 0-19
(...)
```

The `PerformanceProfile` will create the exact same system CPU isolation, and it will configure the kubelet to use the
static CPU manager as in the earlier example. However, it will also configure the API server, the kubelet and crio
for workload partitioning.

Once the cluster is installed with `cpuPartitioningMode: AllNodes` and configured for CPU isolation with a
`PerformanceProfile`, for a pod to be scheduled to the system reserved CPUs, 2 requirements must be fulfilled:

* The namespace must be annotated by an admin or a privileged user with `workload.openshift.io/allowed: management`.
  This requirement makes sure that normal users cannot enable the feature without administrator consent.
* A pod must opt in to being a management workload via annotation
`target.workload.openshift.io/management: {"effect": "PreferredDuringScheduling"}`.

