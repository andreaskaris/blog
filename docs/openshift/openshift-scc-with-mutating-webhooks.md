## SCCs and mutating webhooks - a lesson learned

Recently, a partner of ours observed a strange behavior in their OpenShift cluster. For a specific application, this
partner needs the pod to run with the UID that was configured by the application container image:
~~~
$ cat application/Dockerfile 
FROM (...)
RUN useradd -u 1001 exampleuser
USER 1001
(...)
~~~

When their application pods run, the `.securityContext.runAsUser` field should not be set:
~~~
$ oc get pod application-pod -o jsonpath='{.spec.containers[?(@.name=="application-container")].securityContext.runAsUser}'
$
~~~

And the container should run with the user that was configured by the container image:
~~~
$ oc exec application-pod -- id
uid=1001(exampleuser) gid=1001(exampleuser) groups=1001(exampleuser)
~~~

In order to achieve this, the partner relies on the `RunAsAny` `RUNASUSER` strategy provided by either the
`anyuid` or the `privileged` SCC:
~~~
$ oc get scc | grep -E 'NAME|^anyuid|privileged'
NAME                              PRIV    CAPS         SELINUX     RUNASUSER          FSGROUP     SUPGROUP    PRIORITY     READONLYROOTFS   VOLUMES
anyuid                            false   <no value>   MustRunAs   RunAsAny           RunAsAny    RunAsAny    10           false            ["configMap","downwardAPI","emptyDir","persistentVolumeClaim","projected","secret"]
privileged                        true    ["*"]        RunAsAny    RunAsAny           RunAsAny    RunAsAny    <no value>   false            ["*"]
~~~

This partner is using a 3rd party Istio operator. Contrary to OpenShift ServiceMesh, upstream Istio requires elevated
privileges when injecting its sidecars to the application pods. 

When this partner followed the Istio documentation and bound the `anyuid` SCC to the namespace, the application pod's
non-istio containers would keep their configured `securityContext.runAsUser`, as expected:
~~~
$ oc adm policy add-scc-to-group anyuid system:serviceaccounts:istio-test
~~~

On the other hand, when the partner bound the `privileged` SCC to the namespace, the application pod could still spawn,
but the non-istio containers would show a `securityContext.runAsUser`. Due to this, the pods crash looped, as the
container image's application could not run with OpenShfit's enforced `runAsUser` UID.

Let's look at an example setup that simulates the partner's configuration. We install upstream Istio according to
the upsteam documentation:

* [https://istio.io/latest/docs/setup/getting-started/#download](https://istio.io/latest/docs/setup/getting-started/#download)
* [https://istio.io/latest/docs/setup/platform-setup/openshift/](https://istio.io/latest/docs/setup/platform-setup/openshift/)

We then create a test namespace for our application:
~~~
$ oc new-project istio-test
$ cat <<EOF | oc -n istio-test create -f -
apiVersion: "k8s.cni.cncf.io/v1"
kind: NetworkAttachmentDefinition
metadata:
  name: istio-cni
EOF
$ oc label namespace istio-test istio-injection=enabled
~~~

We bind the `anyuid` SCC to the ServiceAccount:
~~~
$ oc adm policy add-scc-to-group anyuid system:serviceaccounts:istio-test
~~~

And apply the following application:
~~~
$ cat <<'EOF' > fedora-test-sidecar-inject.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  labels:
    app: fedora-test-sidecar-inject
  name: fedora-test-sidecar-inject
spec:
  replicas: 1
  selector:
    matchLabels:
      deployment: fedora-test-sidecar-inject
  template:
    metadata:
      labels:
        deployment: fedora-test-sidecar-inject
      annotations:
        sidecar.istio.io/inject: 'true'
        sidecar.istio.io/proxyCPU: 50m
        sidecar.istio.io/proxyCPULimit: 2000m
        sidecar.istio.io/proxyMemory: 200Mi
        sidecar.istio.io/proxyMemoryLimit: 1Gi
        proxy.istio.io/config: '{ "terminationDrainDuration": 30s, "holdApplicationUntilProxyStarts": true }'
    spec:
      containers:
      - image: quay.io/akaris/fedora-test:uid
        imagePullPolicy: IfNotPresent
        name: fedora-test-sidecar-inject
EOF
$ oc apply -f fedora-test-sidecar-inject.yaml
~~~

As you can see below, only the `istio-proxy` has the `securityContext.runAsUser` field set, whereas the
`fedora-test-sidecar-inject` container does not and thus the container image's UID is honored:
~~~
$ oc get pods -o custom-columns="NAME:.metadata.name,SCC:.metadata.annotations.openshift\.io/scc,CONTAINERNAME:.spec.containers[*].name,RUNASUSER:.spec.containers[*].securityContext.runAsUser"
NAME                                         SCC          CONTAINERNAME                            RUNASUSER
fedora-test-sidecar-inject-d75986bbd-pbqbx   anyuid       istio-proxy,fedora-test-sidecar-inject   1337
$ oc exec fedora-test-sidecar-inject-d75986bbd-pbqbx -- id
uid=1001(exampleuser) gid=1001(exampleuser) groups=1001(exampleuser)
~~~

But when we redeploy the application with the `privileged` SCC, something interesting happens:
~~~
$ oc delete -f fedora-test-sidecar-inject.yaml
$ oc adm policy remove-scc-from-group anyuid system:serviceaccounts:istio-test
$ oc adm policy add-scc-to-group privileged system:serviceaccounts:istio-test
$ oc apply -f fedora-test-sidecar-inject.yaml
~~~

Note how the pod is bound to the `privileged` SCC, but the application container is now forced to run as a specific
user ID:
~~~
$ oc get pods -o custom-columns="NAME:.metadata.name,SCC:.metadata.annotations.openshift\.io/scc,CONTAINERNAME:.spec.containers[*].name,RUNASUSER:.spec.containers[*].securityContext.runAsUser"
NAME                                         SCC          CONTAINERNAME                            RUNASUSER
fedora-test-sidecar-inject-d75986bbd-8j6vt   privileged   istio-proxy,fedora-test-sidecar-inject   1337,1000780000
$ oc exec fedora-test-sidecar-inject-d75986bbd-8j6vt -- id
uid=1000780000(1000780000) gid=0(root) groups=0(root),1000780000
~~~

How is this possible? Both the `privileged` SCC as well as the `anyuid` SCC set `RUNASUSER` to `RunAsAny`, yet the two
SCCs seemingly behave differently. Why would the `privileged` SCC inject `securityContext.runAsUser` into the pods'
containers? Well, Istio uses mutating admission webhooks to achieve sidecar injection. And as it turns out, the
interaction between SCCs and mutating pod admission controllers can be quite complex.

In order to figure out what is going on, let's start by refreshing our memory about SCCs. Then, we will deploy our own
little mutating webhook and create a reproducer environment.

### SCC basics

#### SCC prioritization

It is important to understand OCP SCC prioritization: In OCP,  we can apply multiple SCCs to a ServiceAccount.
The default SCC for any serviceAccount is always `restricted`, even if you did not bind it explicitly to the
ServiceAccount. The matching goes by priority first, then most restrictive SCC policy, then by name.

This means that even when you assign the `privileged` SCC to the default ServiceAccount of your namespace, there will
always be an implicit `restricted` SCC with the same priority as the `privileged` SCC that OpenShift can fall back to:
~~~
oc adm policy add-scc-to-user privileged -z default
~~~

So even when you bind your ServiceAccount to the `privileged` SCC, the pod may actually use the `restricted` SCC, simply
because the pod is happy running in a more restrictive context.

Note that the `anyuid` SCC has a higher SCC priority, and thus it will always win against the `restricted` SCC.

For further details, see the [OpenShift documentation](https://docs.openshift.com/container-platform/4.10/authentication/managing-security-context-constraints.html#scc-prioritization_configuring-internal-oauth).

When deploying a pod, you can check which SCC it is actually running with using the following command:
~~~
oc get pods -o custom-columns="NAME:.metadata.name,SCC:.metadata.annotations.openshift\.io/scc"
~~~

#### SCC runAsUser strategies

There are 4 SCC `runAsUser` strategies, `MustRunAs`, `MustRunAsNonRoot`, `MustRunAsRange` and `RunAsAny`.
Further details can be found in the [OpenShift documentation](https://docs.openshift.com/container-platform/4.10/authentication/managing-security-context-constraints.html#authorization-SCC-strategies_configuring-internal-oauth):

* MustRunAs - Requires a runAsUser to be configured. Uses the configured runAsUser as the default. Validates against the configured runAsUser.
* MustRunAsNonRoot - Requires that the pod be submitted with a non-zero runAsUser or have the USER directive defined in the image. No default provided.
* MustRunAsRange - Requires minimum and maximum values to be defined if not using pre-allocated values. Uses the minimum as the default. Validates against the entire allowable range.
* RunAsAny - No default provided. Allows any runAsUser to be specified.

Neither `MustRunAsNonRoot` nor `RunAsAny` will create a `securityContext.runAsUser` setting on the pod/container, and
thus both will respect the container image's provided UID. `MustRunAsNonRoot` however requires a numeric UID to be
set inside the container image, though.

`MustRunAsRange` will enforce a UID from the project's range, and `MustRunAs` explicitly requires that the admin provide
a UID. So for both of these, `securityContext.runAsUser` will be explicitly set on the pod, one way or another.

Therefore, in theory, when you inspect a pod and it shows as being matched by an SCC with a `runAsUser` strategy of
`RunAsAny`, then OpenShift should never modify `securityContext.runAsUser`. An example of such an SCC is the
`privileged` SCC. On the other hand, if the SCC that matches the pod is `MustRunAsRange`, then a default value will be
chosen if `securityContext.runAsUser` was not set.
~~~
$ oc get scc | egrep -E 'NAME|privileged|restricted'
NAME                              PRIV    CAPS         SELINUX     RUNASUSER          FSGROUP     SUPGROUP    PRIORITY     READONLYROOTFS   VOLUMES
privileged                        true    ["*"]        RunAsAny    RunAsAny           RunAsAny    RunAsAny    <no value>   false            ["*"]
restricted                        false   <no value>   MustRunAs   MustRunAsRange     MustRunAs   RunAsAny    <no value>   false            ["configMap","downwardAPI","emptyDir","persistentVolumeClaim","projected","secret"]
~~~

For the curious, the relevant OpenShift code lives in the [apiserver-library-go](https://github.com/openshift/apiserver-library-go)
repository.
First of all, the SCC admission controller will only ever inject `securityContext.runAsUser` if the field is unset.
Otherwise, it will keep the currently set value. On the other hand, the code in `user/runasany.go`s `Generate()` will
always return `nil`, and thus result in a no-op. 

* Generate() is called here, but only if securityContext.runAsUser is unset for the pod or container:
[https://github.com/openshift/apiserver-library-go/blob/release-4.10/pkg/securitycontextconstraints/sccmatching/provider.go#L159](https://github.com/openshift/apiserver-library-go/blob/release-4.10/pkg/securitycontextconstraints/sccmatching/provider.go#L159)
* RunAsAny UID Generate() returns `nil` and no change will be made to the pod definition:
[https://github.com/openshift/apiserver-library-go/blob/release-4.10/pkg/securitycontextconstraints/user/runasany.go#L22](https://github.com/openshift/apiserver-library-go/blob/release-4.10/pkg/securitycontextconstraints/user/runasany.go#L22)
* Compare that to the MustRunAsRange UID Generate() which will return the first UID from the project's range:
[https://github.com/openshift/apiserver-library-go/blob/release-4.10/pkg/securitycontextconstraints/user/mustrunasrange.go#L37](https://github.com/openshift/apiserver-library-go/blob/release-4.10/pkg/securitycontextconstraints/user/mustrunasrange.go#L37)

### Putting SCC prioritization and SCC runAsUser strategies together

As mentioned earlier, given that the `privileged` and `restricted` SCC both have the same priority, a pod that does
not need any privileges from the `privileged` SCC will automatically fall back to using the `restricted` SCC. This is
because the `restricted` SCC is the most restrictive SCC that matches the pod.
When that happens, the pod will be subject to the `restricted` SCC's `MustRunAsRange` `runAsUser` strategy, and thus a
default UID will be set in `securityContext.runAsUser` if the field was unset at pod creation time. OpenShift's chosen
UID might then conflict with the container image's requirements to run with a specific UID.

The below examples use image `quay.io/akaris/fedora-test:uid` which was generated with the following Dockerfile:
~~~
$ cat Dockerfile 
FROM registry.fedoraproject.org/fedora:latest
RUN useradd -u 1001 exampleuser
USER 1001
RUN whoami
COPY script.sh /script.sh
CMD ["/bin/bash", "/script.sh"]
~~~
> You can find the repository with the Dockerfile and script here [https://github.com/andreaskaris/fedora-test](https://github.com/andreaskaris/fedora-test).

For all examples, the namespace's default ServiceAccount is bound to the `privileged` SCC:
~~~
$ oc adm policy add-scc-to-user privileged -z default
clusterrole.rbac.authorization.k8s.io/system:openshift:scc:privileged added: "default"
~~~

The following is the deployment definition for a pod that will be assigned the `restricted` SCC. Note that the pod
requires minimal privileges and thus upon creation will be mapped to the `restricted` SCC even though the ServiceAccount
is bound to the `privileged` SCC:
~~~
$ cat fedora-test.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  labels:
    app: fedora-test
  name: fedora-test
spec:
  replicas: 1
  selector:
    matchLabels:
      deployment: fedora-test
  template:
    metadata:
      labels:
        deployment: fedora-test
    spec:
      containers:
      - image: quay.io/akaris/fedora-test:uid
        imagePullPolicy: IfNotPresent
        name: fedora-test
~~~

The following is the deployment definition for a pod which will be assigned the `privileged` SCC. The pod's
`securityContext` adds some capabilities and thus does not fit into the restrictions that are imposed by the
`restrictive` SCC:
~~~
$ cat fedora-test-with-capabilities.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  labels:
    app: fedora-test-with-capabilities
  name: fedora-test-with-capabilities
spec:
  replicas: 1
  selector:
    matchLabels:
      deployment: fedora-test-with-capabilities
  template:
    metadata:
      labels:
        deployment: fedora-test-with-capabilities
    spec:
      containers:
      - image: quay.io/akaris/fedora-test:uid
        imagePullPolicy: IfNotPresent
        name: fedora-test-with-capabilities
        securityContext:
          capabilities:
            add:
              - SETFCAP
              - CAP_NET_RAW
              - CAP_NET_ADMIN
~~~

When applying both of these deployments, OpenShift will evaluate both the `restricted` and the `privileged` SCC. As
both have the same priority, the most restrictive SCC that allows the pod to spawn wins. If the `restrictive` SCC
matches the pod, then the `securityContext.runAsUser` field will be written by OpenShift, as well:
~~~
$ oc apply -f fedora-test.yaml
deployment.apps/fedora-test created
$ oc apply -f fedora-test-with-capabilities.yaml
deployment.apps/fedora-test-with-capabilities created
$ oc get pods -o custom-columns="NAME:.metadata.name,SCC:.metadata.annotations.openshift\.io/scc,RUNASUSER:.spec.containers[*].securityContext.runAsUser"
NAME                                             SCC          RUNASUSER
fedora-test-59985b9667-bg8kk                     restricted   1000770000
fedora-test-with-capabilities-5f667b698d-ppc5f   privileged   <none>
$ oc exec fedora-test-59985b9667-bg8kk -- id
uid=1000770000(1000770000) gid=0(root) groups=0(root),1000770000
$ oc exec fedora-test-with-capabilities-5f667b698d-ppc5f -- id
uid=1001(exampleuser) gid=1001(exampleuser) groups=1001(exampleuser)
~~~

###  Mutating webhooks

Mutating webhooks are part of Kubernetes' [Dynamic Admission Control](https://kubernetes.io/docs/reference/access-authn-authz/extensible-admission-controllers/).
A simple mutating admission webhook that we are going to use for some of the following examples can be found
[in this github repository](https://github.com/andreaskaris/webhook). The mutating webhook examines the annotations of
any pod upon creation. If the pod has an annotation `webhook/capabilities:` with a list of capabilities
`["SETFCAP","CAP_NET_RAW","CAP_NET_ADMIN"]` then the mutating webhook will copy these capabilities into
`securityContext.capabilities.add` of all containers:
~~~
apiVersion: apps/v1
kind: Deployment
(...)
spec:
(...)
  template:
    metadata:
      (...)
      annotations:
        webhook/capabilities: '["SETFCAP","CAP_NET_RAW","CAP_NET_ADMIN"]'
    spec:
      containers:
      - image: quay.io/akaris/fedora-test:uid
        imagePullPolicy: IfNotPresent
        name: fedora-test-with-capabilities
(...)
~~~

A pod from the deployment above will be rewritten by the mutating webhook to:
~~~
(...)
  spec:
    containers:
    - image: quay.io/akaris/fedora-test:uid
      imagePullPolicy: IfNotPresent
      name: fedora-test-with-capabilities
      securityContext:
        capabilities:
          add:
          - "SETFCAP"
          - "CAP_NET_RAW"
          - "CAP_NET_ADMIN"
(...)
~~~


### Putting SCC prioritization, SCC runAsUser strategies and mutating webhooks together

As a prerequisite for this section, we deployed the aforementioned mutating webhook. We also increased logging
for the openshift-apiserver and kubeapiserver to `TraceAll` as we will analyze the API server logs a bit later:
~~~
oc patch openshiftapiserver.operator/cluster --type=json -p '[{"op": "replace", "path": "/spec/logLevel", "value": "TraceAll" }]'
oc patch kubeapiserver.operator/cluster --type=json -p '[{"op": "replace", "path": "/spec/logLevel", "value": "TraceAll" }]'
~~~

With the mutating webhook in place, let's create the following deployment:
~~~
$ cat fedora-test-with-annotation.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  labels:
    app: fedora-test-with-annotation
  name: fedora-test-with-annotation
spec:
  replicas: 1
  selector:
    matchLabels:
      deployment: fedora-test-with-annotation
  template:
    metadata:
      labels:
        deployment: fedora-test-with-annotation
      annotations:
        webhook/capabilities: '["SETFCAP","CAP_NET_RAW","CAP_NET_ADMIN"]'
    spec:
      containers:
      - image: quay.io/akaris/fedora-test:uid
        imagePullPolicy: IfNotPresent
        name: fedora-test-with-annotation
~~~

As you can see, it does not define any `add` capabilities, but the webhook will actually inject them for us into the
container as soon as we create the deployment:
~~~
$ oc logs -n webhook -l deployment=webhook
(...)
{"level":"info","ts":"2023-02-13T21:01:59Z","msg":"Inspecting pod","namespace":"","name":"","ownerReferences":[{"apiVersion":"apps/v1","kind":"ReplicaSet","name":"fedora-test-with-annotation-6745dd89bf","uid":"3fc7311a-f380-4a3f-92b6-d43b2fb31da6","controller":true,"blockOwnerDeletion":true}],"labels":{"deployment":"fedora-test-with-annotation","pod-template-hash":"6745dd89bf"}}
{"level":"info","ts":"2023-02-13T21:01:59Z","msg":"Adding capabilities to pod","capabilities":"SETFCAP"}
{"level":"info","ts":"2023-02-13T21:01:59Z","msg":"Adding capabilities to pod","capabilities":"CAP_NET_RAW"}
{"level":"info","ts":"2023-02-13T21:01:59Z","msg":"Adding capabilities to pod","capabilities":"CAP_NET_ADMIN"}
(...)
~~~

After applying the deployment, we can see that the mutating webhook injected the capabilities that we requested:
~~~
apiVersion: v1
kind: Pod
metadata:
(...)
  name: fedora-test-with-annotation-6745dd89bf-t6kdf
  namespace: test
(...)
spec:
  containers:
  - image: quay.io/akaris/fedora-test:uid
    imagePullPolicy: IfNotPresent
    name: fedora-test-with-annotation
    resources: {}
    securityContext:
      capabilities:
        add:
        - SETFCAP
        - CAP_NET_RAW
        - CAP_NET_ADMIN
(...)
~~~

The new pod matches the `privileged` SCC, but it was also forced to run as a specific user ID:
~~~
$ oc get pods -o custom-columns="NAME:.metadata.name,SCC:.metadata.annotations.openshift\.io/scc,RUNASUSER:.spec.containers[*].securityContext.runAsUser"
NAME                                             SCC          RUNASUSER
fedora-test-59985b9667-bg8kk                     restricted   1000770000
fedora-test-with-annotation-6745dd89bf-t6kdf     privileged   1000770000
fedora-test-with-capabilities-5f667b698d-tbz4t   privileged   <none>
~~~

But this contradicts what we said earlier about the `privileged` SCC: it should not manipulate
`securityContext`.`runAsUser`. Interestingly, when the pod is created, it matches the `restricted` SCC which mutates
the pod's containers and adds `securityContext.runAsUser: <ID from project range>`.
Then, the mutating admission controller injects the new capabilities into the pod's containers. After this step, the pod
now requires to be `privileged` as it cannot run with the more restrictive set of rules from the `restricted` SCC.
Therefore, it is again sent through the SCC mutating admission controller where it is now assigned the `privileged` SCC. 

### TL;DR

In short, when the pod was created, it did not need to run with the `privileged` SCC and due to OpenShift's SCC matching
rules, it fell back to the `restricted` SCC. The `restricted` SCC mutated the pod's containers and added
`securityContext.runAsUser: <ID from project range>`. Now, Istio's mutating admission controller injected its containers
into the pod. The Istio containers require the pod to run with the `privileged` SCC, so after this step, the pod was
again sent through the SCC mutating admission controller where it was now assigned the `privileged` SCC. The pod
therefore showed up with the `privileged` SCC when inspecting it with `oc get pods`, but its containers were also
assigned a `securityContext.runAsUser` field that was actually mutated by the `restricted` SCC which had been applied to
the pod before the Istio mutation took place.
