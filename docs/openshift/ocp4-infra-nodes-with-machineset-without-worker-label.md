# OCP 4.5 Infra nodes with MachineSets without worker label #

## How to create Infra nodes with Machinesets but without using the worker label ##

[https://docs.openshift.com/container-platform/4.5/machine_management/creating-infrastructure-machinesets.html](https://docs.openshift.com/container-platform/4.5/machine_management/creating-infrastructure-machinesets.html) states how to create infra nodes with MachineSets. However, the "inconvenience" is that these infra nodes will show up as "infra,worker". How can one not have the "worker" label?

The worker label is added by kubelet configuration as part of `--node-labels=node-role.kubernetes.io/worker,node.openshift.io/os_id=${ID}`. So this will require a custom systemd drop-in for the kubelet.service.

### Step 0: Switch to project openshift-machine-api ###

Switch to the `openshift-machine-api` project:
~~~
oc project openshift-machine-api
~~~

### Step 1: Create kubelet configuration drop-in for infra-node ###

To change booting node-labels a MachineConfig needs to be created as the labels are defined when the Kubelet is started with:
~~~
                --node-labels=node-role.kubernetes.io/worker,node.openshift.io/os_id=${ID} \
~~~

In order to create a new kubelet, create 03-infar-kubelet.yaml:
~~~
cat <<'EOF' > 03-infra-kubelet.yaml
apiVersion: machineconfiguration.openshift.io/v1
kind: MachineConfig
metadata:
  labels:
    machineconfiguration.openshift.io/role: infra
  name: 03-infra-kubelet
spec:
  config:
    ignition:
      config: {}
      security:
        tls: {}
      timeouts: {}
      version: 2.2.0
    systemd:
      units:
        - name: kubelet.service
          dropins:
            - name: 20-change-label.conf
              contents: |
                [Service]
                ExecStart=
                ExecStart=/usr/bin/hyperkube \
                    kubelet \
                      --config=/etc/kubernetes/kubelet.conf \
                      --bootstrap-kubeconfig=/etc/kubernetes/kubeconfig \
                      --kubeconfig=/var/lib/kubelet/kubeconfig \
                      --container-runtime=remote \
                      --container-runtime-endpoint=/var/run/crio/crio.sock \
                      --runtime-cgroups=/system.slice/crio.service \
                      --node-labels=node-role.kubernetes.io/infra,node.openshift.io/os_id=${ID} \
                      --minimum-container-ttl-duration=6m0s \
                      --volume-plugin-dir=/etc/kubernetes/kubelet-plugins/volume/exec \
                      --cloud-provider=aws \
                       \
                      --pod-infra-container-image=quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:dc0fe885d41cc4029caa3feacf71343806c81c8123abc91db90dc0e555fa5636 \
                      --v=${KUBELET_LOG_LEVEL}
EOF
~~~

~~~
$ oc apply -f 03-infra-kubelet.yaml
~~~

### Step 2: Create infra secret ###

Create new user data for infra worker. The following takes the userdata from worker nodes (from the worker-user-data secret) and replaces any occurrence of 'worker' with 'infra':
~~~
oc get secret -n openshift-machine-api worker-user-data -o yaml | awk '/userData:/ {print $2}' | base64 -d | sed 's/worker/infra/g' | base64 -w0 > userdata.base64 
~~~

Then, create a new secret:
~~~
cat << EOF > infra-user-data.yaml
apiVersion: v1
data:
  disableTemplating: dHJ1ZQo=
  userData: $(cat userdata.base64)  
kind: Secret
metadata:
  name: infra-user-data
  namespace: openshift-machine-api
type: Opaque
EOF
oc apply -f infra-user-data.yaml
~~~

Verify:
~~~
$ oc get secret -n openshift-machine-api infra-user-data -o yaml | awk '/userData:/ {print $2}' | base64 -d | jq '.ignition.config.append[0].source'
"https://<cluster URL>:22623/config/infra"
~~~

### Step 3: Create machine configuration for infra ###

Create a machine configuration pool. This machine configuration, for simplicity, will include all machine configurations for worker nodes, as well as for infra nodes. This is why it is important to name the machine configuration for the kubelet `03-infra-kubelet` (see above):
~~~
cat <<'EOF' > infra-mcp.yaml
apiVersion: machineconfiguration.openshift.io/v1
kind: MachineConfigPool
metadata:
  name: infra
spec:
  machineConfigSelector:
    matchExpressions:
    - key: machineconfiguration.openshift.io/role
      operator: In
      values:
      - worker
      - infra
  maxUnavailable: 1
  nodeSelector:
    matchLabels:
      node-role.kubernetes.io/infra: ""
  paused: false
EOF
~~~

Apply:
~~~
oc apply -f infra-mcp.yaml
~~~

### Step 4: Create a machineset for the new infra node ###

First, create a new machineset:
https://docs.openshift.com/container-platform/4.5/machine_management/creating-infrastructure-machinesets.html#machineset-yaml-aws_creating-infrastructure-machinesets

Look at an existing machineset for how to fill in some of the blank values:
~~~
[akaris@linux infra-nodes]$ oc get machinesets -A
NAMESPACE               NAME                              DESIRED   CURRENT   READY   AVAILABLE   AGE
openshift-machine-api   akaris2-4kpk4-worker-eu-west-3a   1         1         1       1           112m
openshift-machine-api   akaris2-4kpk4-worker-eu-west-3b   1         1         1       1           112m
openshift-machine-api   akaris2-4kpk4-worker-eu-west-3c   1         1         1       1           112m
~~~ 

~~~
$ oc get machineset -n openshift-machine-api akaris2-4kpk4-worker-eu-west-3a -o yaml > akaris2-4kpk4-infra-eu-west-3a.yaml
~~~

Clean up the file and modify as instructed in https://docs.openshift.com/container-platform/4.5/machine_management/creating-infrastructure-machinesets.html#machineset-yaml-aws_creating-infrastructure-machinesets

The example from the documentation will contain 'worker' in:
~~~
          iamInstanceProfile:
            id: <infrastructureID>-worker-profile 
~~~

~~~
          securityGroups:
            - filters:
                - name: tag:Name
                  values:
                    - <infrastructureID>-worker-sg 
~~~

~~~
          userDataSecret:
            name: worker-user-data
~~~

Make sure to change the userDataSecret to `infra-user-data`, contrary to what the documentation says:
~~~
          userDataSecret:
            name: infra-user-data
~~~

The MachineSet, after modification - in this case, 3 replicas are requested, right away:
~~~
$ cat akaris2-4kpk4-infra-eu-west-3a.yaml
apiVersion: machine.openshift.io/v1beta1
kind: MachineSet
metadata:
  labels:
    machine.openshift.io/cluster-api-cluster: akaris2-4kpk4
  name: akaris2-4kpk4-infra-eu-west-3a
  namespace: openshift-machine-api
spec:
  replicas: 3
  selector:
    matchLabels:
      machine.openshift.io/cluster-api-cluster: akaris2-4kpk4
      machine.openshift.io/cluster-api-machineset: akaris2-4kpk4-infra-eu-west-3a
  template:
    metadata:
      labels:
        machine.openshift.io/cluster-api-cluster: akaris2-4kpk4
        machine.openshift.io/cluster-api-machine-role: infra
        machine.openshift.io/cluster-api-machine-type: infra
        machine.openshift.io/cluster-api-machineset: akaris2-4kpk4-infra-eu-west-3a
    spec:
      metadata:
        labels:
          node-role.kubernetes.io/infra: ""
      providerSpec:
        value:
          ami:
            id: ami-(...)
          apiVersion: awsproviderconfig.openshift.io/v1beta1
          blockDevices:
          - ebs:
              iops: 0
              volumeSize: 120
              volumeType: gp2
          credentialsSecret:
            name: aws-cloud-credentials
          deviceIndex: 0
          iamInstanceProfile:
            id: akaris2-4kpk4-worker-profile
          instanceType: m5.large
          kind: AWSMachineProviderConfig
          metadata:
            creationTimestamp: null
          placement:
            availabilityZone: eu-west-3a
            region: eu-west-3
          publicIp: null
          securityGroups:
          - filters:
            - name: tag:Name
              values:
              - akaris2-4kpk4-worker-sg
          subnet:
            filters:
            - name: tag:Name
              values:
              - akaris2-4kpk4-private-eu-west-3a
          tags:
          - name: kubernetes.io/cluster/akaris2-4kpk4
            value: owned
          userDataSecret:
            name: infra-user-data
~~~

Apply the MachineSet:
~~~
$ oc apply -f akaris2-4kpk4-infra-eu-west-3a.yaml
~~~

### Step 5: Verify Machine creation ###

Monitor machine configuration - right after creation of the MachineSet:
~~~
[akaris@linux infra-nodes]$ oc get machineconfigpool
NAME     CONFIG                                             UPDATED   UPDATING   DEGRADED   MACHINECOUNT   READYMACHINECOUNT   UPDATEDMACHINECOUNT   DEGRADEDMACHINECOUNT   AGE
infra    rendered-infra-d63625ee2cdc4637cc156a1ebf25e926    True      False      False      0              0                   0                     0                      4m9s
master   rendered-master-8d007e81b9ebb00b4042722f2713f1ae   True      False      False      3              3                   3                     0                      116m
worker   rendered-worker-9d6647aa43752e0eb71a221dd8b7e32e   True      False      False      3              3                   3                     0                      116m
[akaris@linux infra-nodes]$ oc get machines
NAME                                    PHASE         TYPE        REGION      ZONE         AGE
akaris2-f74ht-infra-eu-west-3a-fctd5    Provisioned   m5.large    eu-west-3   eu-west-3a   49s
akaris2-f74ht-infra-eu-west-3a-xtpcv    Provisioned   m5.large    eu-west-3   eu-west-3a   49s
akaris2-f74ht-infra-eu-west-3a-zmvzh    Provisioned   m5.large    eu-west-3   eu-west-3a   49s
akaris2-f74ht-master-0                  Running       m5.xlarge   eu-west-3   eu-west-3a   119m
akaris2-f74ht-master-1                  Running       m5.xlarge   eu-west-3   eu-west-3b   119m
akaris2-f74ht-master-2                  Running       m5.xlarge   eu-west-3   eu-west-3c   119m
akaris2-f74ht-worker-eu-west-3a-4krrd   Running       m5.large    eu-west-3   eu-west-3a   111m
akaris2-f74ht-worker-eu-west-3b-59z6n   Running       m5.large    eu-west-3   eu-west-3b   111m
akaris2-f74ht-worker-eu-west-3c-chst9   Running       m5.large    eu-west-3   eu-west-3c   111m
[akaris@linux infra-nodes]$ oc get machineset
NAME                              DESIRED   CURRENT   READY   AVAILABLE   AGE
akaris2-f74ht-infra-eu-west-3a    3         3                             58s
akaris2-f74ht-worker-eu-west-3a   1         1         1       1           119m
akaris2-f74ht-worker-eu-west-3b   1         1         1       1           119m
akaris2-f74ht-worker-eu-west-3c   1         1         1       1           119m
[akaris@linux infra-nodes]$ oc get nodes
NAME                                         STATUS   ROLES    AGE    VERSION
ip-10-0-148-174.eu-west-3.compute.internal   Ready    worker   105m   v1.17.1+b83bc57
ip-10-0-159-35.eu-west-3.compute.internal    Ready    master   117m   v1.17.1+b83bc57
ip-10-0-168-64.eu-west-3.compute.internal    Ready    master   118m   v1.17.1+b83bc57
ip-10-0-191-238.eu-west-3.compute.internal   Ready    worker   105m   v1.17.1+b83bc57
ip-10-0-208-80.eu-west-3.compute.internal    Ready    master   117m   v1.17.1+b83bc57
ip-10-0-220-32.eu-west-3.compute.internal    Ready    worker   105m   v1.17.1+b83bc57
~~~

Wait:
~~~
[akaris@linux infra-nodes]$ oc get machineconfigpool
NAME     CONFIG                                             UPDATED   UPDATING   DEGRADED   MACHINECOUNT   READYMACHINECOUNT   UPDATEDMACHINECOUNT   DEGRADEDMACHINECOUNT   AGE
infra    rendered-infra-d63625ee2cdc4637cc156a1ebf25e926    True      False      False      3              3                   3                     0                      10m
master   rendered-master-8d007e81b9ebb00b4042722f2713f1ae   True      False      False      3              3                   3                     0                      122m
worker   rendered-worker-9d6647aa43752e0eb71a221dd8b7e32e   True      False      False      3              3                   3                     0                      122m
[akaris@linux infra-nodes]$ oc get machineset
NAME                              DESIRED   CURRENT   READY   AVAILABLE   AGE
akaris2-f74ht-infra-eu-west-3a    3         3         3       3           7m15s
akaris2-f74ht-worker-eu-west-3a   1         1         1       1           125m
akaris2-f74ht-worker-eu-west-3b   1         1         1       1           125m
akaris2-f74ht-worker-eu-west-3c   1         1         1       1           125m
[akaris@linux infra-nodes]$ oc get machines
NAME                                    PHASE     TYPE        REGION      ZONE         AGE
akaris2-f74ht-infra-eu-west-3a-fctd5    Running   m5.large    eu-west-3   eu-west-3a   7m17s
akaris2-f74ht-infra-eu-west-3a-xtpcv    Running   m5.large    eu-west-3   eu-west-3a   7m17s
akaris2-f74ht-infra-eu-west-3a-zmvzh    Running   m5.large    eu-west-3   eu-west-3a   7m17s
akaris2-f74ht-master-0                  Running   m5.xlarge   eu-west-3   eu-west-3a   125m
akaris2-f74ht-master-1                  Running   m5.xlarge   eu-west-3   eu-west-3b   125m
akaris2-f74ht-master-2                  Running   m5.xlarge   eu-west-3   eu-west-3c   125m
akaris2-f74ht-worker-eu-west-3a-4krrd   Running   m5.large    eu-west-3   eu-west-3a   117m
akaris2-f74ht-worker-eu-west-3b-59z6n   Running   m5.large    eu-west-3   eu-west-3b   117m
akaris2-f74ht-worker-eu-west-3c-chst9   Running   m5.large    eu-west-3   eu-west-3c   117m
[akaris@linux infra-nodes]$ oc get nodes
NAME                                         STATUS   ROLES    AGE     VERSION
ip-10-0-128-80.eu-west-3.compute.internal    Ready    infra    2m39s   v1.17.1+b83bc57
ip-10-0-145-35.eu-west-3.compute.internal    Ready    infra    2m43s   v1.17.1+b83bc57
ip-10-0-148-174.eu-west-3.compute.internal   Ready    worker   111m    v1.17.1+b83bc57
ip-10-0-159-154.eu-west-3.compute.internal   Ready    infra    2m7s    v1.17.1+b83bc57
ip-10-0-159-35.eu-west-3.compute.internal    Ready    master   124m    v1.17.1+b83bc57
ip-10-0-168-64.eu-west-3.compute.internal    Ready    master   124m    v1.17.1+b83bc57
ip-10-0-191-238.eu-west-3.compute.internal   Ready    worker   111m    v1.17.1+b83bc57
ip-10-0-208-80.eu-west-3.compute.internal    Ready    master   124m    v1.17.1+b83bc57
ip-10-0-220-32.eu-west-3.compute.internal    Ready    worker   111m    v1.17.1+b83bc57
~~~

### Step 7: Moving resources to the infra nodes ###

The following steps are from:
https://github.com/lbohnsac/OCP4/tree/master/infrastructure-node-setup

Also see steps from:
https://docs.openshift.com/container-platform/4.5/machine_management/creating-infrastructure-machinesets.html

#### Move the router bits ####

Move the internal routers to the infra nodes
~~~
oc patch ingresscontrollers.operator.openshift.io default -n openshift-ingress-operator --type=merge \
  --patch '{"spec":{"nodePlacement":{"nodeSelector":{"matchLabels":{"node-role.kubernetes.io/infra":""}}}}}'
~~~

Define 3 routers (default are 2!)
~~~
oc patch ingresscontrollers.operator.openshift.io default -n openshift-ingress-operator --type=merge \
  --patch '{"spec":{"replicas": 3}}'
~~~

#### Move the registry bits ####

Move the internal image registry to the infra nodes
~~~
oc patch configs.imageregistry.operator.openshift.io cluster --type=merge \
  --patch '{"spec":{"nodeSelector":{"node-role.kubernetes.io/infra": ""}}}'
~~~

Move the registry image pruner (from 4.4) to the infra nodes
~~~
oc patch imagepruners.imageregistry.operator.openshift.io cluster --type=merge \
  --patch '{"spec":{"nodeSelector": {"node-role.kubernetes.io/infra": ""}}}'
~~~

#### Move the logging bits ####

Move the Kibana pod to the infras
~~~
oc patch clusterloggings.logging.openshift.io instance -n openshift-logging --type=merge \
  --patch '{"spec":{"visualization":{"kibana":{"nodeSelector":{"node-role.kubernetes.io/infra":""}}}}}'
~~~

Move the curator pod to the infras
~~~
oc patch clusterloggings.logging.openshift.io instance -n openshift-logging --type=merge \
  --patch '{"spec":{"curation":{"curator":{"nodeSelector":{"node-role.kubernetes.io/infra":""}}}}}'
~~~

Move the elasticsearch pods to the infras
~~~
oc patch clusterloggings.logging.openshift.io instance -n openshift-logging --type=merge \
  --patch '{"spec":{"logStore":{"elasticsearch":{"nodeSelector":{"node-role.kubernetes.io/infra":""}}}}}'
~~~

### Issues ###

The inconvenience with this is that the ExecStart of the dropin is now "hardcoded" and will not be updated / changed by the installer upon upgrade.

### Step 6: Testing upgrades ###

~~~
[akaris@linux infra-nodes]$ oc get clusterversion
NAME      VERSION   AVAILABLE   PROGRESSING   SINCE   STATUS
version   4.4.16    True        False         123m    Cluster version is 4.4.16
[akaris@linux infra-nodes]$ oc adm upgrade
Cluster version is 4.4.16

No updates available. You may force an upgrade to a specific release image, but doing so may not be supported and result in downtime or data loss.

#############################
# set to 4.5-stable channel
#############################
[akaris@linux infra-nodes]$ oc edit clusterversion
clusterversion.config.openshift.io/version edited


[akaris@linux infra-nodes]$ oc adm upgrade
Cluster version is 4.4.16

Updates:

VERSION IMAGE
4.5.5   quay.io/openshift-release-dev/ocp-release@sha256:a58573e1c92f5258219022ec104ec254ded0a70370ee8ed2aceea52525639bd4
[akaris@linux infra-nodes]$ oc adm upgrade --to-latest
Updating to latest version 4.5.5
~~~

The upgrade succeeds:
~~~
[akaris@linux ipi]$ export KUBECONFIG=/home/akaris/cases/02729138/ipi/install-config/auth/kubeconfig
[akaris@linux ipi]$ oc get nodes
NAME                                         STATUS   ROLES    AGE   VERSION
ip-10-0-128-80.eu-west-3.compute.internal    Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-145-35.eu-west-3.compute.internal    Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-148-174.eu-west-3.compute.internal   Ready    worker   19h   v1.18.3+08c38ef
ip-10-0-159-154.eu-west-3.compute.internal   Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-159-35.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-168-64.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-191-238.eu-west-3.compute.internal   Ready    worker   19h   v1.18.3+08c38ef
ip-10-0-208-80.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-220-32.eu-west-3.compute.internal    Ready    worker   19h   v1.18.3+08c38ef
[akaris@linux ipi]$ oc get clusterversion
NAME      VERSION   AVAILABLE   PROGRESSING   SINCE   STATUS
version   4.5.5     True        False         16h     Cluster version is 4.5.5
[akaris@linux ipi]$ oc get machinesets
NAME                              DESIRED   CURRENT   READY   AVAILABLE   AGE
akaris2-f74ht-infra-eu-west-3a    3         3         3       3           17h
akaris2-f74ht-worker-eu-west-3a   1         1         1       1           19h
akaris2-f74ht-worker-eu-west-3b   1         1         1       1           19h
akaris2-f74ht-worker-eu-west-3c   1         1         1       1           19h
[akaris@linux ipi]$ oc get machines
NAME                                    PHASE     TYPE        REGION      ZONE         AGE
akaris2-f74ht-infra-eu-west-3a-fctd5    Running   m5.large    eu-west-3   eu-west-3a   17h
akaris2-f74ht-infra-eu-west-3a-xtpcv    Running   m5.large    eu-west-3   eu-west-3a   17h
akaris2-f74ht-infra-eu-west-3a-zmvzh    Running   m5.large    eu-west-3   eu-west-3a   17h
akaris2-f74ht-master-0                  Running   m5.xlarge   eu-west-3   eu-west-3a   19h
akaris2-f74ht-master-1                  Running   m5.xlarge   eu-west-3   eu-west-3b   19h
akaris2-f74ht-master-2                  Running   m5.xlarge   eu-west-3   eu-west-3c   19h
akaris2-f74ht-worker-eu-west-3a-4krrd   Running   m5.large    eu-west-3   eu-west-3a   19h
akaris2-f74ht-worker-eu-west-3b-59z6n   Running   m5.large    eu-west-3   eu-west-3b   19h
akaris2-f74ht-worker-eu-west-3c-chst9   Running   m5.large    eu-west-3   eu-west-3c   19h
[akaris@linux ipi]$ oc get machineconfigpool
NAME     CONFIG                                             UPDATED   UPDATING   DEGRADED   MACHINECOUNT   READYMACHINECOUNT   UPDATEDMACHINECOUNT   DEGRADEDMACHINECOUNT   AGE
infra    rendered-infra-aa17b9b35abf210691d268dd4c7449d3    True      False      False      3              3                   3                     0                      17h
master   rendered-master-2cc3abe4c49c6db04d9456744d0079f9   True      False      False      3              3                   3                     0                      19h
worker   rendered-worker-d65740ba72e305c96782a27b8255dd89   True      False      False      3              3                   3                     0                      19h
[akaris@linux ipi]$ oc get machineconfig
NAME                                                        GENERATEDBYCONTROLLER                      IGNITIONVERSION   AGE
00-master                                                   807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             19h
00-worker                                                   807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             19h
01-master-container-runtime                                 807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             19h
01-master-kubelet                                           807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             19h
01-worker-container-runtime                                 807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             19h
01-worker-kubelet                                           807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             19h
03-infra-kubelet                                                                                       2.2.0             17h
99-master-d32a1dcd-1b7b-4999-8919-3bd59c24fdae-registries   807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             19h
99-master-ssh                                                                                          2.2.0             19h
99-worker-f569c8d2-84df-4a5b-846a-9e090b9fb68d-registries   807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             19h
99-worker-ssh                                                                                          2.2.0             19h
rendered-infra-aa17b9b35abf210691d268dd4c7449d3             807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             16h
rendered-infra-d63625ee2cdc4637cc156a1ebf25e926             601c2285f497bf7c73d84737b9977a0e697cb86a   2.2.0             17h
rendered-master-2cc3abe4c49c6db04d9456744d0079f9            807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             16h
rendered-master-8d007e81b9ebb00b4042722f2713f1ae            601c2285f497bf7c73d84737b9977a0e697cb86a   2.2.0             19h
rendered-worker-9d6647aa43752e0eb71a221dd8b7e32e            601c2285f497bf7c73d84737b9977a0e697cb86a   2.2.0             19h
rendered-worker-d65740ba72e305c96782a27b8255dd89            807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             16h
[akaris@linux ipi]$ 
~~~

The one problem is the pod version on the infra nodes:
~~~
[akaris@linux ipi]$ oc get nodes
NAME                                         STATUS   ROLES    AGE   VERSION
ip-10-0-128-80.eu-west-3.compute.internal    Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-145-35.eu-west-3.compute.internal    Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-148-174.eu-west-3.compute.internal   Ready    worker   19h   v1.18.3+08c38ef
ip-10-0-159-154.eu-west-3.compute.internal   Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-159-35.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-168-64.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-191-238.eu-west-3.compute.internal   Ready    worker   19h   v1.18.3+08c38ef
ip-10-0-208-80.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-220-32.eu-west-3.compute.internal    Ready    worker   19h   v1.18.3+08c38ef
[akaris@linux ipi]$ oc debug node/ip-10-0-128-80.eu-west-3.compute.internal
Starting pod/ip-10-0-128-80eu-west-3computeinternal-debug ...
To use host binaries, run `chroot /host`
Pod IP: 10.0.128.80
If you don't see a command prompt, try pressing enter.
sh-4.2# chroot /host
sh-4.4# cat /etc/redhat-release 
Red Hat Enterprise Linux CoreOS release 4.5
sh-4.4# ps aux | grep kubelet
root        1422  3.3  2.0 1461696 161600 ?      Ssl  Aug19  33:46 kubelet --config=/etc/kubernetes/kubelet.conf --bootstrap-kubeconfig=/etc/kubernetes/kubeconfig --kubeconfig=/var/lib/kubelet/kubeconfig --container-runtime=remote --container-runtime-endpoint=/var/run/crio/crio.sock --runtime-cgroups=/system.slice/crio.service --node-labels=node-role.kubernetes.io/infra,node.openshift.io/os_id=rhcos --minimum-container-ttl-duration=6m0s --volume-plugin-dir=/etc/kubernetes/kubelet-plugins/volume/exec --cloud-provider=aws --pod-infra-container-image=quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:dc0fe885d41cc4029caa3feacf71343806c81c8123abc91db90dc0e555fa5636 --v=4
root     1232029  0.0  0.0   9180  1032 ?        S+   10:02   0:00 grep kubelet
sh-4.4# 
sh-4.4# grep pause /etc/crio/ -R
/etc/crio/seccomp.json:				"pause",
/etc/crio/crio.conf.d/00-default:pause_image = "quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:6e8bcc9c91267283f8b0edf1af0cb0d310c54d4f73905d9bcfb2a103a664fcb0"
/etc/crio/crio.conf.d/00-default:pause_image_auth_file = "/var/lib/kubelet/config.json"
/etc/crio/crio.conf.d/00-default:pause_command = "/usr/bin/pod"
sh-4.4# 
~~~

Compare that to the version on the worker nodes:
~~~
[akaris@linux ipi]$ oc get nodes
NAME                                         STATUS   ROLES    AGE   VERSION
ip-10-0-128-80.eu-west-3.compute.internal    Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-145-35.eu-west-3.compute.internal    Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-148-174.eu-west-3.compute.internal   Ready    worker   19h   v1.18.3+08c38ef
ip-10-0-159-154.eu-west-3.compute.internal   Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-159-35.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-168-64.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-191-238.eu-west-3.compute.internal   Ready    worker   19h   v1.18.3+08c38ef
ip-10-0-208-80.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-220-32.eu-west-3.compute.internal    Ready    worker   19h   v1.18.3+08c38ef
[akaris@linux ipi]$ oc debug node/ip-10-0-148-174.eu-west-3.compute.internal
Starting pod/ip-10-0-148-174eu-west-3computeinternal-debug ...
To use host binaries, run `chroot /host`
Pod IP: 10.0.148.174
If you don't see a command prompt, try pressing enter.
sh-4.2# chroot /host
sh-4.4# cat /etc/redhat-release
Red Hat Enterprise Linux CoreOS release 4.5
sh-4.4# ps aux | grep kubelet
root        1421  5.5  2.5 1454268 196984 ?      Ssl  Aug19  56:01 kubelet --config=/etc/kubernetes/kubelet.conf --bootstrap-kubeconfig=/etc/kubernetes/kubeconfig --kubeconfig=/var/lib/kubelet/kubeconfig --container-runtime=remote --container-runtime-endpoint=/var/run/crio/crio.sock --runtime-cgroups=/system.slice/crio.service --node-labels=node-role.kubernetes.io/worker,node.openshift.io/os_id=rhcos --minimum-container-ttl-duration=6m0s --volume-plugin-dir=/etc/kubernetes/kubelet-plugins/volume/exec --cloud-provider=aws --pod-infra-container-image=quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:6e8bcc9c91267283f8b0edf1af0cb0d310c54d4f73905d9bcfb2a103a664fcb0 --v=4
root     2138381  0.0  0.0   9180  1060 ?        S+   10:02   0:00 grep kubelet
sh-4.4# grep pause /etc/crio/ -R
/etc/crio/seccomp.json:				"pause",
/etc/crio/crio.conf.d/00-default:pause_image = "quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:6e8bcc9c91267283f8b0edf1af0cb0d310c54d4f73905d9bcfb2a103a664fcb0"
/etc/crio/crio.conf.d/00-default:pause_image_auth_file = "/var/lib/kubelet/config.json"
/etc/crio/crio.conf.d/00-default:pause_command = "/usr/bin/pod"
~~~

That `--pod-infra-container-image=` pod version is for the pause container; although as you can see above, the pause_image in crio.conf.d is always updated: 
[https://stackoverflow.com/questions/46630377/what-is-pod-infra-container-image-meant-for](https://stackoverflow.com/questions/46630377/what-is-pod-infra-container-image-meant-for)
~~~
The pause container, which image the --pod-infra-container flag selects, is used so that multiple containers can be launched in a pod, while sharing resources. It mostly does nothing, and unless you have a very good reason to replace it with something custom, you shouldn't. It mostly invokes the pause system call (hence its name) but it also performs the important function of having PID 1 and making sure no zombie processes are kept around.

An extremely complete article on the subject can be found here, from where I also shamelessly stole the following picture which illustrates where the pause container lives:
~~~

According to [https://github.com/kubernetes/kubernetes/pull/70603](https://github.com/kubernetes/kubernetes/pull/70603) (although that comment is from 2018):
~~~
The kubelet allows you to set --pod-infra-container-image
(also called PodSandboxImage in the kubelet config),
which can be a custom location to the "pause" image in the case
of Docker. Other CRIs are not supported.
~~~

And according to [https://github.com/kubernetes/kubernetes/issues/86081](https://github.com/kubernetes/kubernetes/issues/86081) (from November 2019):
~~~
Kubelet flag "pod-infra-container-image" is invalid when using CRI-O
~~~

And according to the kubelet help, this only affects docker and doesn't affect us:
~~~
sh-4.4# kubelet --help | grep infra
      --pod-infra-container-image string                                                                          The image whose network/ipc namespaces containers in each pod will use. This docker-specific flag only works when container-runtime is set to docker. (default "k8s.gcr.io/pause:3.2")
~~~

As an additional step just to bring the infra nodes in line with the workers and the rest of the cluster, we could update that by inspecting the current worker kubelet configuration and then modify this post-upgrade:
~~~
[akaris@linux ipi]$ oc get machineconfig 01-worker-kubelet -o yaml | grep pod-infra-container-image
                --pod-infra-container-image=quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:6e8bcc9c91267283f8b0edf1af0cb0d310c54d4f73905d9bcfb2a103a664fcb0 \
[akaris@linux ipi]$ oc get machineconfig 03-infra-kubelet -o yaml | grep pod-infra-container-image | grep -v apiVersion
                  --pod-infra-container-image=quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:dc0fe885d41cc4029caa3feacf71343806c81c8123abc91db90dc0e555fa5636 \
[akaris@linux ipi]$ oc edit machineconfig 03-infra-kubelet
machineconfig.machineconfiguration.openshift.io/03-infra-kubelet edited
[akaris@linux ipi]$ oc get machineconfig 03-infra-kubelet -o yaml | grep pod-infra-container-image | grep -v apiVersion
                  --pod-infra-container-image=quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:6e8bcc9c91267283f8b0edf1af0cb0d310c54d4f73905d9bcfb2a103a664fcb0 \
~~~

Which will then trigger a restart of the infra nodes:
~~~
[akaris@linux ipi]$ oc get machineconfigpool
NAME     CONFIG                                             UPDATED   UPDATING   DEGRADED   MACHINECOUNT   READYMACHINECOUNT   UPDATEDMACHINECOUNT   DEGRADEDMACHINECOUNT   AGE
infra    rendered-infra-aa17b9b35abf210691d268dd4c7449d3    False     True       False      3              0                   0                     0                      17h
master   rendered-master-2cc3abe4c49c6db04d9456744d0079f9   True      False      False      3              3                   3                     0                      19h
worker   rendered-worker-d65740ba72e305c96782a27b8255dd89   True      False      False      3              3                   3                     0                      19h
[akaris@linux ipi]$ oc get machineconfig | grep infra
03-infra-kubelet                                                                                       2.2.0             17h
rendered-infra-aa17b9b35abf210691d268dd4c7449d3             807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             17h
rendered-infra-d63625ee2cdc4637cc156a1ebf25e926             601c2285f497bf7c73d84737b9977a0e697cb86a   2.2.0             17h
rendered-infra-f3df01ebf7429a29582f3c46132fcf88             807abb900cf9976a1baad66eab17c6d76016e7b7   2.2.0             44s
[akaris@linux ipi]$ 
~~~

And then eventually:
~~~
[akaris@linux ipi]$ oc get nodes
NAME                                         STATUS   ROLES    AGE   VERSION
ip-10-0-128-80.eu-west-3.compute.internal    Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-145-35.eu-west-3.compute.internal    Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-148-174.eu-west-3.compute.internal   Ready    worker   19h   v1.18.3+08c38ef
ip-10-0-159-154.eu-west-3.compute.internal   Ready    infra    17h   v1.18.3+08c38ef
ip-10-0-159-35.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-168-64.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-191-238.eu-west-3.compute.internal   Ready    worker   19h   v1.18.3+08c38ef
ip-10-0-208-80.eu-west-3.compute.internal    Ready    master   19h   v1.18.3+08c38ef
ip-10-0-220-32.eu-west-3.compute.internal    Ready    worker   19h   v1.18.3+08c38ef
[akaris@linux ipi]$ oc get machineconfigpool
NAME     CONFIG                                             UPDATED   UPDATING   DEGRADED   MACHINECOUNT   READYMACHINECOUNT   UPDATEDMACHINECOUNT   DEGRADEDMACHINECOUNT   AGE
infra    rendered-infra-f3df01ebf7429a29582f3c46132fcf88    True      False      False      3              3                   3                     0                      18h
master   rendered-master-2cc3abe4c49c6db04d9456744d0079f9   True      False      False      3              3                   3                     0                      19h
worker   rendered-worker-d65740ba72e305c96782a27b8255dd89   True      False      False      3              3                   3                     0                      19h
[akaris@linux ipi]$ oc get machines
NAME                                    PHASE     TYPE        REGION      ZONE         AGE
akaris2-f74ht-infra-eu-west-3a-fctd5    Running   m5.large    eu-west-3   eu-west-3a   18h
akaris2-f74ht-infra-eu-west-3a-xtpcv    Running   m5.large    eu-west-3   eu-west-3a   18h
akaris2-f74ht-infra-eu-west-3a-zmvzh    Running   m5.large    eu-west-3   eu-west-3a   18h
akaris2-f74ht-master-0                  Running   m5.xlarge   eu-west-3   eu-west-3a   19h
akaris2-f74ht-master-1                  Running   m5.xlarge   eu-west-3   eu-west-3b   19h
akaris2-f74ht-master-2                  Running   m5.xlarge   eu-west-3   eu-west-3c   19h
akaris2-f74ht-worker-eu-west-3a-4krrd   Running   m5.large    eu-west-3   eu-west-3a   19h
akaris2-f74ht-worker-eu-west-3b-59z6n   Running   m5.large    eu-west-3   eu-west-3b   19h
akaris2-f74ht-worker-eu-west-3c-chst9   Running   m5.large    eu-west-3   eu-west-3c   19h
[akaris@linux ipi]$ oc debug node/ip-10-0-128-80.eu-west-3.compute.internal
Starting pod/ip-10-0-128-80eu-west-3computeinternal-debug ...
To use host binaries, run `chroot /host`
Pod IP: 10.0.128.80
If you don't see a command prompt, try pressing enter.
sh-4.2# chroot /host
sh-4.4# ps aux | grep kubelet
root        1411  3.5  1.8 1484684 147368 ?      Ssl  10:16   0:23 kubelet --config=/etc/kubernetes/kubelet.conf --bootstrap-kubeconfig=/etc/kubernetes/kubeconfig --kubeconfig=/var/lib/kubelet/kubeconfig --container-runtime=remote --container-runtime-endpoint=/var/run/crio/crio.sock --runtime-cgroups=/system.slice/crio.service --node-labels=node-role.kubernetes.io/infra,node.openshift.io/os_id=rhcos --minimum-container-ttl-duration=6m0s --volume-plugin-dir=/etc/kubernetes/kubelet-plugins/volume/exec --cloud-provider=aws --pod-infra-container-image=quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:6e8bcc9c91267283f8b0edf1af0cb0d310c54d4f73905d9bcfb2a103a664fcb0 --v=4
root       17471  0.0  0.0   9180  1052 ?        S+   10:27   0:00 grep kubelet
~~~

