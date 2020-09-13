# How Security Context Constraints (SCCs) work in OpenShift #

How SCCs work in OpenShift by example. The following tests were run in Red Hat OpenShift Platform 3.11. Depending on the version in use, the cluster's behavior may be slightly different.

## Impact of the default restricted SCC ##

The following example spawns a statefulset with a pod as user 65000. This user falls not into the `supplemental-groups` of the project and can hence not be used by the `restricted` SCC. The deployment will fail:
~~~
$ oc describe project test  | grep scc
			openshift.io/sa.scc.mcs=s0:c10,c0
			openshift.io/sa.scc.supplemental-groups=1000090000/10000
			openshift.io/sa.scc.uid-range=1000090000/10000
~~~

Storage class is a local provisioner with 4 100MB local mounts, set up with [https://github.com/andreaskaris/kubernetes/tree/master/localvolume](https://github.com/andreaskaris/kubernetes/tree/master/localvolume):
~~~
$ oc get storageclass
NAME              PROVISIONER                    AGE
local-loopbacks   kubernetes.io/no-provisioner   3d
$ oc get pv
NAME                CAPACITY   ACCESS MODES   RECLAIM POLICY   STATUS      CLAIM     STORAGECLASS      REASON    AGE
local-pv-18dc9ec0   92Mi       RWO            Delete           Available             local-loopbacks             36s
local-pv-89b778f1   92Mi       RWO            Delete           Available             local-loopbacks             19m
local-pv-9e67e7d6   92Mi       RWO            Delete           Available             local-loopbacks             28m
local-pv-b6a7cd37   92Mi       RWO            Delete           Available             local-loopbacks             3d
~~~

`statefulset.yaml`:
~~~
kind: Service
apiVersion: v1
metadata:
  name: "test"
spec:
  clusterIP: None
  # the list of ports that are exposed by this service
  ports:
    - name: http
      port: 80
  # will route traffic to pods having labels matching this selector
  selector:
    name: "test"
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: test
spec:
  selector:
    matchLabels:
       app: test
  serviceName: test
  replicas: 1
  template:
    metadata:
      labels:
        app: test
    spec:
      securityContext:
        runAsUser: 65000
      containers:
        - image: gcr.io/google_containers/busybox
          command:
            - "/bin/sh"
            - "-c"
            - "while true; do date; sleep 1; done"
          name: busybox
          volumeMounts:
            - name: vol
              mountPath: /mnt
  volumeClaimTemplates:
    - metadata:
        name: vol
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: local-loopbacks
        resources:
          requests:
            storage: 90Mi
~~~

~~~
$ oc apply -f statefulset.yaml
~~~

OpenShift's admission controller will complain that the range is not correct:
~~~
$ oc get pods
No resources found.
$ oc describe statefulset test | tail
  StorageClass:  local-loopbacks
  Labels:        <none>
  Annotations:   <none>
  Capacity:      90Mi
  Access Modes:  [ReadWriteOnce]
Events:
  Type     Reason            Age                 From                    Message
  ----     ------            ----                ----                    -------
  Normal   SuccessfulCreate  40s                 statefulset-controller  create Claim vol-test-0 Pod test-0 in StatefulSet test success
  Warning  FailedCreate      19s (x14 over 40s)  statefulset-controller  create Pod test-0 in StatefulSet test failed error: pods "test-0" is forbidden: unable to validate against any security context constraint: [spec.containers[0].securityContext.securityContext.runAsUser: Invalid value: 65000: must be in the ranges: [1000090000, 1000099999]]
~~~

## Fixing failed validation of security context constraints ##

There are 3 solutions to this. Either:

* create a new SCC (or modify the restricted policy which is not recommended) or 
* modify the runAsUser field to run the pod as a user inside range `1000090000, 1000099999` or 
* change the namespace's `openshift.io/sa.scc.uid-range`.

### Resetting the lab ###

First, delete the statefulSet, PVC, etc. Continue once all is deleted:
~~~
$ oc get statefulset
No resources found.
$ oc get pods
No resources found.
$ oc get pvc
No resources found.
~~~

### Changing the namespace's openshift.io/sa.scc.uid-range ###

Let's change the namespace's `uid-range`:
~~~
$ oc edit namespace test
namespace/test edited
$ oc describe project test  | grep scc
			openshift.io/sa.scc.mcs=s0:c10,c0
			openshift.io/sa.scc.supplemental-groups=1000090000/10000
			openshift.io/sa.scc.uid-range=65000/1000
~~~

~~~
$ oc apply -f statefulset.yaml
service/test created
statefulset.apps/test created
~~~

~~~
$ oc get statefulset
NAME      DESIRED   CURRENT   AGE
test      1         1         24s
$ oc get pods
NAME      READY     STATUS    RESTARTS   AGE
test-0    1/1       Running   0          26s
$ oc get pvc
NAME         STATUS    VOLUME              CAPACITY   ACCESS MODES   STORAGECLASS      AGE
vol-test-0   Bound     local-pv-9e67e7d6   92Mi       RWO            local-loopbacks   30s
~~~

The pod was assigned to the restricted SCC:
~~~
$ oc get pod test-0 -o yaml | grep scc
    openshift.io/scc: restricted
~~~

This is because the restricted SCC is the default. If other SCCs were assigned to the default serviceaccount of the project, then SCC priorization would come into play. 

For more about SCC priorization, see: [https://docs.openshift.com/container-platform/3.11/architecture/additional_concepts/authorization.html#scc-prioritization](https://docs.openshift.com/container-platform/3.11/architecture/additional_concepts/authorization.html#scc-prioritization)

On the worker that the pod runs at, verify the container's and processes' UID:
~~~
[root@node-0 ~]# docker ps  | grep test
47b2d7df970c        gcr.io/google_containers/busybox@sha256:d8d3bc2c183ed2f9f10e7258f84971202325ee6011ba137112e01e30f206de67                          "/bin/sh -c 'while..."   About a minute ago   Up About a minute                       k8s_busybox_test-0_test_6dfbf91e-d5a1-11ea-a312-fa163e8752f1_0
8b3ef77e9213        registry.redhat.io/openshift3/ose-pod:v3.11.248                                                                                   "/usr/bin/pod"           About a minute ago   Up About a minute                       k8s_POD_test-0_test_6dfbf91e-d5a1-11ea-a312-fa163e8752f1_0
[root@node-0 ~]# docker inspect 47b2d7df970c | grep -i user
            "UsernsMode": "",
            "User": "65000",
[root@node-0 ~]# ps aux | grep 'date; sleep 1'
65000      5172  0.0  0.0   3164   352 ?        Ss   11:53   0:00 /bin/sh -c while true; do date; sleep 1; done
root       6616  0.0  0.0 112808   972 pts/0    R+   11:55   0:00 grep --color=auto date; sleep 1
~~~

Also note that the FsGroupID was set automatically for the filesystem:
~~~
[root@node-0 ~]# ls /mnt/local-storage/loopbacks/ -al
total 8
drwxr-xr-x. 6 root root         42 Jul 30 13:30 .
drwxr-xr-x. 3 root root         23 Jul 30 13:28 ..
drwxrwsr-x. 2 root root       1024 Aug  3 10:06 a
drwxrwsr-x. 2 root root       1024 Aug  3 09:55 b
drwxrwsr-x. 2 root 1000090000 1024 Aug  3 09:55 c
drwxr-xr-x. 3 root root       1024 Jul 30 13:30 d
~~~

And it was assigned as GID to the user inside the pod:
~~~
$ oc rsh test-0
/ $ id
uid=65000 gid=0(root) groups=1000090000
~~~

### Resetting the lab ###

Delete the statefulSet, PVC, etc again. Continue once all is deleted:
~~~
$ oc get statefulset
No resources found.
$ oc get pods
No resources found.
$ oc get pvc
No resources found.
~~~

### Creating a custom SCC

Let's now modify `pod.spec.securityContext` and set fsGroup:
~~~
kind: Service
apiVersion: v1
metadata:
  name: "test"
spec:
  clusterIP: None
  # the list of ports that are exposed by this service
  ports:
    - name: http
      port: 80
  # will route traffic to pods having labels matching this selector
  selector:
    name: "test"
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: test
spec:
  selector:
    matchLabels:
       app: test
  serviceName: test
  replicas: 1
  template:
    metadata:
      labels:
        app: test
    spec:
      securityContext:
        runAsUser: 65000
        fsGroup: 65000
      containers:
        - image: gcr.io/google_containers/busybox
          command:
            - "/bin/sh"
            - "-c"
            - "while true; do date; sleep 1; done"
          name: busybox
          volumeMounts:
            - name: vol
              mountPath: /mnt
  volumeClaimTemplates:
    - metadata:
        name: vol
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: local-loopbacks
        resources:
          requests:
            storage: 90Mi
~~~

This of course must fail:
~~~
 oc apply -f statefulset.yaml 
service/test created
statefulset.apps/test created
$ oc describe statefulset test | tail
  StorageClass:  local-loopbacks
  Labels:        <none>
  Annotations:   <none>
  Capacity:      90Mi
  Access Modes:  [ReadWriteOnce]
Events:
  Type     Reason            Age               From                    Message
  ----     ------            ----              ----                    -------
  Normal   SuccessfulCreate  7s                statefulset-controller  create Claim vol-test-0 Pod test-0 in StatefulSet test success
  Warning  FailedCreate      2s (x11 over 7s)  statefulset-controller  create Pod test-0 in StatefulSet test failed error: pods "test-0" is forbidden: unable to validate against any security context constraint: [fsGroup: Invalid value: []int64{65000}: 65000 is not an allowed group]
~~~

Let's create a custom SCC to change this. The following group was created by inspecting the `restricted` SCC with `oc get scc restricted -o yaml` and then by changing the name, the fsGroup ranges and by removing some of the metadata.

`restricted-fsgroup.yaml`:
~~~
allowHostDirVolumePlugin: false
allowHostIPC: false
allowHostNetwork: false
allowHostPID: false
allowHostPorts: false
allowPrivilegeEscalation: true
allowPrivilegedContainer: false
allowedCapabilities: null
apiVersion: security.openshift.io/v1
defaultAddCapabilities: null
fsGroup:
  type: MustRunAs
  ranges:
    - min: 65000
      max: 66000
kind: SecurityContextConstraints
metadata:
  name: restricted-fsgroup
# priority: 10  # it's possible to prioritize this SCC - we are not doing this here
readOnlyRootFilesystem: false
requiredDropCapabilities:
- KILL
- MKNOD
- SETUID
- SETGID
runAsUser:
  type: MustRunAsRange
seLinuxContext:
  type: MustRunAs
supplementalGroups:
  type: RunAsAny
users: []
volumes:
- configMap
- downwardAPI
- emptyDir
- persistentVolumeClaim
- projected
- secret
~~~

~~~
$ oc apply -f restricted-fsgroup.yaml 
securitycontextconstraints.security.openshift.io/restricted-fsgroup created
$ oc get scc
NAME                 PRIV      CAPS      SELINUX     RUNASUSER          FSGROUP     SUPGROUP    PRIORITY   READONLYROOTFS   VOLUMES
anyuid               false     []        MustRunAs   RunAsAny           RunAsAny    RunAsAny    10         false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
hostaccess           false     []        MustRunAs   MustRunAsRange     MustRunAs   RunAsAny    <none>     false            [configMap downwardAPI emptyDir hostPath persistentVolumeClaim projected secret]
hostmount-anyuid     false     []        MustRunAs   RunAsAny           RunAsAny    RunAsAny    <none>     false            [configMap downwardAPI emptyDir hostPath nfs persistentVolumeClaim projected secret]
hostnetwork          false     []        MustRunAs   MustRunAsRange     MustRunAs   MustRunAs   <none>     false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
kube-state-metrics   false     []        RunAsAny    RunAsAny           RunAsAny    RunAsAny    <none>     false            [*]
node-exporter        false     []        RunAsAny    RunAsAny           RunAsAny    RunAsAny    <none>     false            [*]
nonroot              false     []        MustRunAs   MustRunAsNonRoot   RunAsAny    RunAsAny    <none>     false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
privileged           true      [*]       RunAsAny    RunAsAny           RunAsAny    RunAsAny    <none>     false            [*]
restricted           false     []        MustRunAs   MustRunAsRange     MustRunAs   RunAsAny    <none>     false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
restricted-fsgroup   false     []        MustRunAs   MustRunAsRange     MustRunAs   RunAsAny    <none>     false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
~~~

Add this restricted-fsgroup to the default service account's SCCs:
~~~
$ oc adm policy add-scc-to-user restricted-fsgroup -z default
scc "restricted-fsgroup" added to: ["system:serviceaccount:test:default"]
$ oc get scc restricted-fsgroup -o yaml | grep users -A1
(...)
users:
- system:serviceaccount:test:default
(...)
~~~

Delete the statefulSet, PVC, etc again. Continue once all is deleted:
~~~
$ oc get statefulset
No resources found.
$ oc get pods
No resources found.
$ oc get pvc
No resources found.
~~~

The statefulSet can now spawn the pod and the pod is assigned to SCC `restricted-fsgroup`:
~~~
$ oc apply -f statefulset.yaml
$ oc get pods
NAME      READY     STATUS    RESTARTS   AGE
test-0    1/1       Running   0          17s
$ oc describe statefulset test | tail
  StorageClass:  local-loopbacks
  Labels:        <none>
  Annotations:   <none>
  Capacity:      90Mi
  Access Modes:  [ReadWriteOnce]
Events:
  Type    Reason            Age   From                    Message
  ----    ------            ----  ----                    -------
  Normal  SuccessfulCreate  30s   statefulset-controller  create Claim vol-test-0 Pod test-0 in StatefulSet test success
  Normal  SuccessfulCreate  30s   statefulset-controller  create Pod test-0 in StatefulSet test successful
$ oc get pod test-0 -o yaml | grep scc
    openshift.io/scc: restricted-fsgroup
~~~

The pod now runs with group ID 65000:
~~~
$ oc rsh test-0
/ $ id
uid=65000 gid=0(root) groups=65000
~~~

And that matches the group ID of the mount's directory:
~~~
[root@node-0 ~]# ls /mnt/local-storage/loopbacks/ -al
total 8
drwxr-xr-x. 6 root root    42 Jul 30 13:30 .
drwxr-xr-x. 3 root root    23 Jul 30 13:28 ..
drwxrwsr-x. 2 root root  1024 Aug  3 10:06 a
drwxrwsr-x. 2 root 65000 1024 Aug  3 09:55 b
drwxrwsr-x. 2 root root  1024 Aug  3 09:55 c
drwxr-xr-x. 3 root root  1024 Jul 30 13:30 d
~~~

## SCC matching and priority ##

Keep in mind that both SCCs are active at the same time - and will be selected depending on which SCC is matched by the requested pod.

### Matching based on allowed security context ###

Let's change `fsGroup: 65000` to `fsGroup: 1000090000`:
~~~
kind: Service
apiVersion: v1
metadata:
  name: "test"
spec:
  clusterIP: None
  # the list of ports that are exposed by this service
  ports:
    - name: http
      port: 80
  # will route traffic to pods having labels matching this selector
  selector:
    name: "test"
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: test
spec:
  selector:
    matchLabels:
       app: test
  serviceName: test
  replicas: 1
  template:
    metadata:
      labels:
        app: test
    spec:
      securityContext:
        runAsUser: 65000
        fsGroup: 1000090000
      containers:
        - image: gcr.io/google_containers/busybox
          command:
            - "/bin/sh"
            - "-c"
            - "while true; do date; sleep 1; done"
          name: busybox
          volumeMounts:
            - name: vol
              mountPath: /mnt
  volumeClaimTemplates:
    - metadata:
        name: vol
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: local-loopbacks
        resources:
          requests:
            storage: 90Mi
~~~

Reset the lab again, then deploy the modified deployment.

Now, the pod's SCC is `restricted` because the requested GID only matches the restricted but not the restricted-fsgroup SCC:
~~~
$ oc apply -f statefulset.yaml
service/test created
$ oc get pod test-0 -o yaml | grep scc
    openshift.io/scc: restricted
~~~

### Name as tiebreaker ###

Now, what happens if we remove the request for a specific fsGroup from the pod?
~~~
kind: Service
apiVersion: v1
metadata:
  name: "test"
spec:
  clusterIP: None
  # the list of ports that are exposed by this service
  ports:
    - name: http
      port: 80
  # will route traffic to pods having labels matching this selector
  selector:
    name: "test"
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: test
spec:
  selector:
    matchLabels:
       app: test
  serviceName: test
  replicas: 1
  template:
    metadata:
      labels:
        app: test
    spec:
      securityContext:
        runAsUser: 65000
      containers:
        - image: gcr.io/google_containers/busybox
          command:
            - "/bin/sh"
            - "-c"
            - "while true; do date; sleep 1; done"
          name: busybox
          volumeMounts:
            - name: vol
              mountPath: /mnt
  volumeClaimTemplates:
    - metadata:
        name: vol
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: local-loopbacks
        resources:
          requests:
            storage: 90Mi
~~~

Reset the lab again, then apply the statefulset.

~~~
$ oc apply -f statefulset.yaml 
service/test created
statefulset.apps/test created
$ oc get pod test-0 -o yaml | grep scc
    openshift.io/scc: restricted
~~~

~~~
$ oc rsh test-0
/ $ id
uid=65000 gid=0(root) groups=1000090000
~~~

~~~
[root@node-0 ~]# ls /mnt/local-storage/loopbacks/ -al
total 8
drwxr-xr-x. 6 root root         42 Jul 30 13:30 .
drwxr-xr-x. 3 root root         23 Jul 30 13:28 ..
drwxrwsr-x. 2 root 1000090000 1024 Aug  3 10:06 a
drwxrwsr-x. 2 root root       1024 Aug  3 09:55 b
drwxrwsr-x. 2 root root       1024 Aug  3 09:55 c
drwxr-xr-x. 3 root root       1024 Jul 30 13:30 d
~~~

Both SCCs have the same priority, are equally restrictive. So the tiebreaker is the name:

* [https://docs.openshift.com/container-platform/3.11/architecture/additional_concepts/authorization.html#scc-prioritization](https://docs.openshift.com/container-platform/3.11/architecture/additional_concepts/authorization.html#scc-prioritization)
~~~
    Highest priority first, nil is considered a 0 priority

    If priorities are equal, the SCCs will be sorted from most restrictive to least restrictive

    If both priorities and restrictions are equal the SCCs will be sorted by name
~~~

Let's suppose this had a different name:
~~~
$ oc delete -f restricted-fsgroup.yaml
securitycontextconstraints.security.openshift.io "restricted-fsgroup" deleted
$ oc apply -f fsgroup.yaml 
securitycontextconstraints.security.openshift.io/fsgroup created
$ oc adm policy add-scc-to-user fsgroup -z default
scc "fsgroup" added to: ["system:serviceaccount:test:default"]
$ oc get scc
NAME                 PRIV      CAPS      SELINUX     RUNASUSER          FSGROUP     SUPGROUP    PRIORITY   READONLYROOTFS   VOLUMES
anyuid               false     []        MustRunAs   RunAsAny           RunAsAny    RunAsAny    10         false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
fsgroup              false     []        MustRunAs   MustRunAsRange     MustRunAs   RunAsAny    <none>     false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
hostaccess           false     []        MustRunAs   MustRunAsRange     MustRunAs   RunAsAny    <none>     false            [configMap downwardAPI emptyDir hostPath persistentVolumeClaim projected secret]
hostmount-anyuid     false     []        MustRunAs   RunAsAny           RunAsAny    RunAsAny    <none>     false            [configMap downwardAPI emptyDir hostPath nfs persistentVolumeClaim projected secret]
hostnetwork          false     []        MustRunAs   MustRunAsRange     MustRunAs   MustRunAs   <none>     false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
kube-state-metrics   false     []        RunAsAny    RunAsAny           RunAsAny    RunAsAny    <none>     false            [*]
node-exporter        false     []        RunAsAny    RunAsAny           RunAsAny    RunAsAny    <none>     false            [*]
nonroot              false     []        MustRunAs   MustRunAsNonRoot   RunAsAny    RunAsAny    <none>     false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
privileged           true      [*]       RunAsAny    RunAsAny           RunAsAny    RunAsAny    <none>     false            [*]
restricted           false     []        MustRunAs   MustRunAsRange     MustRunAs   RunAsAny    <none>     false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
~~~

Then we'd use the `fsgroup` SCC:
~~~
$ oc apply -f statefulset.yaml 
service/test created
statefulset.apps/test created
$ oc get pod test-0 -o yaml | grep scc
    openshift.io/scc: fsgroup
~~~

~~~
$ oc rsh test-0
id
/ $ id
uid=65000 gid=0(root) groups=65000
~~~

~~~
[root@node-0 ~]# ls /mnt/local-storage/loopbacks/ -al
total 8
drwxr-xr-x. 6 root root         42 Jul 30 13:30 .
drwxr-xr-x. 3 root root         23 Jul 30 13:28 ..
drwxrwsr-x. 2 root      65000 1024 Aug  3 10:06 a
drwxrwsr-x. 2 root root       1024 Aug  3 09:55 b
drwxrwsr-x. 2 root root       1024 Aug  3 09:55 c
drwxr-xr-x. 3 root root       1024 Jul 30 13:30 d
~~~

### Using the priority field as a tiebreaker ###

And is there a way to prioritize our policy without changing the name? Yes, with the priority field. Change the SCC's name again to `restricted-fsgroup`. So based on the name, it would lose against `restricted`. But also set the priority to `1` which is higher than `restricted`s priority of `0`:
~~~
$ cat restricted-fsgroup.yaml 
allowHostDirVolumePlugin: false
allowHostIPC: false
allowHostNetwork: false
allowHostPID: false
allowHostPorts: false
allowPrivilegeEscalation: true
allowPrivilegedContainer: false
allowedCapabilities: null
apiVersion: security.openshift.io/v1
defaultAddCapabilities: null
fsGroup:
  type: MustRunAs
  ranges:
    - min: 65000
      max: 66000
kind: SecurityContextConstraints
metadata:
  name: restricted-fsgroup
readOnlyRootFilesystem: false
requiredDropCapabilities:
- KILL
- MKNOD
- SETUID
- SETGID
runAsUser:
  type: MustRunAsRange
seLinuxContext:
  type: MustRunAs
supplementalGroups:
  type: RunAsAny
users: []
volumes:
- configMap
- downwardAPI
- emptyDir
- persistentVolumeClaim
- projected
- secret
priority: 1
~~~

After applying this, we see:
~~~
$ oc get scc
NAME                 PRIV      CAPS      SELINUX     RUNASUSER          FSGROUP     SUPGROUP    PRIORITY   READONLYROOTFS   VOLUMES
anyuid               false     []        MustRunAs   RunAsAny           RunAsAny    RunAsAny    10         false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
hostaccess           false     []        MustRunAs   MustRunAsRange     MustRunAs   RunAsAny    <none>     false            [configMap downwardAPI emptyDir hostPath persistentVolumeClaim projected secret]
hostmount-anyuid     false     []        MustRunAs   RunAsAny           RunAsAny    RunAsAny    <none>     false            [configMap downwardAPI emptyDir hostPath nfs persistentVolumeClaim projected secret]
hostnetwork          false     []        MustRunAs   MustRunAsRange     MustRunAs   MustRunAs   <none>     false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
kube-state-metrics   false     []        RunAsAny    RunAsAny           RunAsAny    RunAsAny    <none>     false            [*]
node-exporter        false     []        RunAsAny    RunAsAny           RunAsAny    RunAsAny    <none>     false            [*]
nonroot              false     []        MustRunAs   MustRunAsNonRoot   RunAsAny    RunAsAny    <none>     false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
privileged           true      [*]       RunAsAny    RunAsAny           RunAsAny    RunAsAny    <none>     false            [*]
restricted           false     []        MustRunAs   MustRunAsRange     MustRunAs   RunAsAny    <none>     false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
restricted-fsgroup   false     []        MustRunAs   MustRunAsRange     MustRunAs   RunAsAny    1          false            [configMap downwardAPI emptyDir persistentVolumeClaim projected secret]
~~~

~~~
$ oc get pod test-0 -o yaml | grep scc
    openshift.io/scc: restricted-fsgroup
~~~

### Demonstrating that priority only comes into play if SCCs overlap ###

But should we decide that we not like  a fsGroup within 65000-65999, we can request the namespace's default and still use `restricted`, even though we changed the priority:

~~~
kind: Service
apiVersion: v1
metadata:
  name: "test"
spec:
  clusterIP: None
  # the list of ports that are exposed by this service
  ports:
    - name: http
      port: 80
  # will route traffic to pods having labels matching this selector
  selector:
    name: "test"
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: test
spec:
  selector:
    matchLabels:
       app: test
  serviceName: test
  replicas: 1
  template:
    metadata:
      labels:
        app: test
    spec:
      securityContext:
        runAsUser: 65000
        fsGroup: 1000090000
      containers:
        - image: gcr.io/google_containers/busybox
          command:
            - "/bin/sh"
            - "-c"
            - "while true; do date; sleep 1; done"
          name: busybox
          volumeMounts:
            - name: vol
              mountPath: /mnt
  volumeClaimTemplates:
    - metadata:
        name: vol
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: local-loopbacks
        resources:
          requests:
            storage: 90Mi
~~~

See this:
~~~
$ oc apply -f statefulset.yaml 
service/test created
statefulset.apps/test created
$ oc get pod test-0 -o yaml | grep scc
    openshift.io/scc: restricted
~~~

## Summary ##

Lessons learned: we can apply multiple SCCs to a ServiceAccount. The default SCC is always `restricted`. The matching goes by priority first, then most restrictive policy, then by name. And if a policy does not match a specific request, another one might "jump in" and "help out". 


## Resources ##

* [https://www.openshift.com/blog/managing-sccs-in-openshift]([https://www.openshift.com/blog/managing-sccs-in-openshift)
* [https://docs.openshift.com/container-platform/3.11/admin_guide/manage_scc.html]([https://docs.openshift.com/container-platform/3.11/admin_guide/manage_scc.html)
* [https://docs.openshift.com/container-platform/3.11/install_config/persistent_storage/pod_security_context.html]([https://docs.openshift.com/container-platform/3.11/install_config/persistent_storage/pod_security_context.html)

