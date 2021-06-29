## Get vs List API calls

Some confusion may arise when giving only `list` access to a service account.

As an example, create a test project, SA, role, rolebinding and a deployment:
~~~
oc new-project test-privs
oc create serviceaccount test-privs-sa
cat <<'EOF' | oc apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: test-privs-role
rules:
- apiGroups:
  - '*'
  resources:
  - '*'
  verbs:
  - list
EOF
oc policy add-role-to-user test-privs-role -z test-privs-sa --role-namespace="test-privs"

cat <<'EOF' > deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: fedora-deployment
  labels:
    app: fedora-deployment
spec:
  replicas: 1
  selector:
    matchLabels:
      app: fedora-pod
  template:
    metadata:
      labels:
        app: fedora-pod
    spec:
      serviceAccount: test-privs-sa
      serviceAccountName: test-privs-sa
      containers:
      - name: fedora
        image: fedora
        command:
          - bash
          - /entrypoint-scripts/entrypoint.sh
        imagePullPolicy: IfNotPresent
        volumeMounts:
        - name: entrypoint-scripts
          mountPath: /entrypoint-scripts
        - mountPath: /usr/local/bin
          name: usr-local-bin
      volumes:
        - name: entrypoint-scripts
          configMap:
            name: entrypoint-scripts
        - name: usr-local-bin
          emptyDir: {}
---
apiVersion: v1
data:
  entrypoint.sh: |
    #!/bin/bash

    curl -o /usr/local/bin/oc.tar.gz https://mirror.openshift.com/pub/openshift-v4/clients/ocp/stable-4.6/openshift-client-linux.tar.gz
    tar -xf /usr/local/bin/oc.tar.gz -C /usr/local/bin
    chmod +x /usr/local/bin/oc

    sleep infinity
kind: ConfigMap
metadata:
  name: entrypoint-scripts
EOF
oc apply -f deployment.yaml
~~~

Check logs:
~~~
oc logs -f -l app=fedora-pod
~~~

And when the `oc` client was installed and extracted, connect to the pod and check your SA and rights:
~~~
[root@openshift-jumpserver-0 ~]# oc rsh fedora-deployment-cc5c4748d-chsfr
sh-5.1$ 
sh-5.1$ oc whoami
system:serviceaccount:test-privs:test-privs-sa
sh-5.1$ oc auth can-i get pods   
no
sh-5.1$ oc auth can-i list pods
yes
~~~

With the above rights, the SA can list all resources of all types, but it cannot get them:
~~~
sh-5.1$ oc get pods
NAME                                READY   STATUS    RESTARTS   AGE
fedora-deployment-cc5c4748d-chsfr   1/1     Running   0          8m15s
sh-5.1$ oc get pod fedora-deployment-cc5c4748d-chsfr
Error from server (Forbidden): pods "fedora-deployment-cc5c4748d-chsfr" is forbidden: User "system:serviceaccount:test-privs:test-privs-sa" cannot get resource "pods" in API group "" in the namespace "test-privs"
~~~

The difference in the API calls can be seen with a higher loglevel:
~~~
sh-5.1$ oc --loglevel=10 get pods 2>&1  | grep 'curl -k -v -XGET' | tail -1
I0629 12:41:09.168254     411 round_trippers.go:423] curl -k -v -XGET  -H "Accept: application/json;as=Table;v=v1;g=meta.k8s.io,application/json;as=Table;v=v1beta1;g=meta.k8s.io,application/json" -H "User-Agent: oc/4.6.0 (linux/amd64) kubernetes/5ab7b2b" -H "Authorization: Bearer eyJhbGciOiJSUzI1NiIsImtpZCI6IndSNGg0blNZWlNVWlZIUGVvaWUzN1pEdVd5c2JFZTQ1Mlk3bm1iMkt6LUEifQ.eyJpc3MiOiJrdWJlcm5ldGVzL3NlcnZpY2VhY2NvdW50Iiwia3ViZXJuZXRlcy5pby9zZXJ2aWNlYWNjb3VudC9uYW1lc3BhY2UiOiJ0ZXN0LXByaXZzIiwia3ViZXJuZXRlcy5pby9zZXJ2aWNlYWNjb3VudC9zZWNyZXQubmFtZSI6InRlc3QtcHJpdnMtc2EtdG9rZW4tNjVsZnIiLCJrdWJlcm5ldGVzLmlvL3NlcnZpY2VhY2NvdW50L3NlcnZpY2UtYWNjb3VudC5uYW1lIjoidGVzdC1wcml2cy1zYSIsImt1YmVybmV0ZXMuaW8vc2VydmljZWFjY291bnQvc2VydmljZS1hY2NvdW50LnVpZCI6ImFiMDU3MGIyLTM5YWQtNDAwNS1hZTY3LTBhYjg2YTdmNmIwOCIsInN1YiI6InN5c3RlbTpzZXJ2aWNlYWNjb3VudDp0ZXN0LXByaXZzOnRlc3QtcHJpdnMtc2EifQ.VoZcFEOfFuGr1XOXjq7d6Idq03aY6KVPTgXvWNy3ZEFtcAubsp8C-_0kjL2vzuKqspRV6PTuA1xjoGJ6EQGBbqkobPwkqvqmXflqnkhMcMDq9wdy6EZS5Wb-buUlAOYhmeODQKH5B4z8ervBUx46sOHiye_SMTXhHhmmnyFlKCwHd6RmvXu7lYYtDWrnQCjlE_EBOa3ZjfeY9GG1OY0lG-z5UlxUuwynoYAqe7o_FWIwte9mqmA4JsdyMA90GZ6XSomBFKlLpuKFF5RN-isO5Ksr9Da5YVGOTz08HPYj_xoNGdg5wpVZ7KFwbJUduodtXLVUlG0WROFE12EJaeNx5A" 'https://172.30.0.1:443/api/v1/namespaces/test-privs/pods?limit=500'
I0629 12:41:23.998393     430 round_trippers.go:423] curl -k -v -XGET  -H "Accept: application/json;as=Table;v=v1;g=meta.k8s.io,application/json;as=Table;v=v1beta1;g=meta.k8s.io,application/json" -H "User-Agent: oc/4.6.0 (linux/amd64) kubernetes/5ab7b2b" -H "Authorization: Bearer eyJhbGciOiJSUzI1NiIsImtpZCI6IndSNGg0blNZWlNVWlZIUGVvaWUzN1pEdVd5c2JFZTQ1Mlk3bm1iMkt6LUEifQ.eyJpc3MiOiJrdWJlcm5ldGVzL3NlcnZpY2VhY2NvdW50Iiwia3ViZXJuZXRlcy5pby9zZXJ2aWNlYWNjb3VudC9uYW1lc3BhY2UiOiJ0ZXN0LXByaXZzIiwia3ViZXJuZXRlcy5pby9zZXJ2aWNlYWNjb3VudC9zZWNyZXQubmFtZSI6InRlc3QtcHJpdnMtc2EtdG9rZW4tNjVsZnIiLCJrdWJlcm5ldGVzLmlvL3NlcnZpY2VhY2NvdW50L3NlcnZpY2UtYWNjb3VudC5uYW1lIjoidGVzdC1wcml2cy1zYSIsImt1YmVybmV0ZXMuaW8vc2VydmljZWFjY291bnQvc2VydmljZS1hY2NvdW50LnVpZCI6ImFiMDU3MGIyLTM5YWQtNDAwNS1hZTY3LTBhYjg2YTdmNmIwOCIsInN1YiI6InN5c3RlbTpzZXJ2aWNlYWNjb3VudDp0ZXN0LXByaXZzOnRlc3QtcHJpdnMtc2EifQ.VoZcFEOfFuGr1XOXjq7d6Idq03aY6KVPTgXvWNy3ZEFtcAubsp8C-_0kjL2vzuKqspRV6PTuA1xjoGJ6EQGBbqkobPwkqvqmXflqnkhMcMDq9wdy6EZS5Wb-buUlAOYhmeODQKH5B4z8ervBUx46sOHiye_SMTXhHhmmnyFlKCwHd6RmvXu7lYYtDWrnQCjlE_EBOa3ZjfeY9GG1OY0lG-z5UlxUuwynoYAqe7o_FWIwte9mqmA4JsdyMA90GZ6XSomBFKlLpuKFF5RN-isO5Ksr9Da5YVGOTz08HPYj_xoNGdg5wpVZ7KFwbJUduodtXLVUlG0WROFE12EJaeNx5A" 'https://172.30.0.1:443/api/v1/namespaces/test-privs/pods/fedora-deployment-cc5c4748d-chsfr'
~~~

However, with the `list` API call, the ServiceAccount can still see all properties of an object:
~~~
sh-5.1$ oc get pods -o yaml
apiVersion: v1
items:
- apiVersion: v1
  kind: Pod
  metadata:
    annotations:
      k8s.ovn.org/pod-networks: '{"default":{"ip_addresses":["172.25.2.121/23"],"mac_address":"0a:58:ac:19:02:79","gateway_ips":["172.25.2.1"],"ip_address":"172.25.2.121/23","gateway_ip":"172.25.2.1"}}'
      k8s.v1.cni.cncf.io/network-status: |-
        [{
            "name": "",
            "interface": "eth0",
            "ips": [
                "172.25.2.121"
            ],
            "mac": "0a:58:ac:19:02:79",
            "default": true,
            "dns": {}
        }]
      k8s.v1.cni.cncf.io/networks-status: |-
        [{
            "name": "",
            "interface": "eth0",
            "ips": [
                "172.25.2.121"
            ],
            "mac": "0a:58:ac:19:02:79",
            "default": true,
            "dns": {}
        }]
      openshift.io/scc: restricted
    creationTimestamp: "2021-06-29T12:31:58Z"
    generateName: fedora-deployment-cc5c4748d-
    labels:
      app: fedora-pod
      pod-template-hash: cc5c4748d
    managedFields:
    - apiVersion: v1
      fieldsType: FieldsV1
      fieldsV1:
        f:metadata:
          f:annotations:
            f:k8s.ovn.org/pod-networks: {}
      manager: ovnkube
      operation: Update
      time: "2021-06-29T12:31:57Z"
    - apiVersion: v1
      fieldsType: FieldsV1
      fieldsV1:
        f:metadata:
          f:generateName: {}
          f:labels:
            .: {}
            f:app: {}
            f:pod-template-hash: {}
          f:ownerReferences:
            .: {}
            k:{"uid":"e1cef632-15a0-4ce5-9566-87c7299a30c1"}:
              .: {}
              f:apiVersion: {}
              f:blockOwnerDeletion: {}
              f:controller: {}
              f:kind: {}
              f:name: {}
              f:uid: {}
        f:spec:
          f:containers:
            k:{"name":"fedora"}:
              .: {}
              f:command: {}
              f:image: {}
              f:imagePullPolicy: {}
              f:name: {}
              f:resources: {}
              f:terminationMessagePath: {}
              f:terminationMessagePolicy: {}
              f:volumeMounts:
                .: {}
                k:{"mountPath":"/entrypoint-scripts"}:
                  .: {}
                  f:mountPath: {}
                  f:name: {}
                k:{"mountPath":"/usr/local/bin"}:
                  .: {}
                  f:mountPath: {}
                  f:name: {}
          f:dnsPolicy: {}
          f:enableServiceLinks: {}
          f:restartPolicy: {}
          f:schedulerName: {}
          f:securityContext:
            .: {}
            f:fsGroup: {}
            f:seLinuxOptions:
              f:level: {}
          f:serviceAccount: {}
          f:serviceAccountName: {}
          f:terminationGracePeriodSeconds: {}
          f:volumes:
            .: {}
            k:{"name":"entrypoint-scripts"}:
              .: {}
              f:configMap:
                .: {}
                f:defaultMode: {}
                f:name: {}
              f:name: {}
            k:{"name":"usr-local-bin"}:
              .: {}
              f:emptyDir: {}
              f:name: {}
      manager: kube-controller-manager
      operation: Update
      time: "2021-06-29T12:31:58Z"
    - apiVersion: v1
      fieldsType: FieldsV1
      fieldsV1:
        f:status:
          f:conditions:
            k:{"type":"ContainersReady"}:
              .: {}
              f:lastProbeTime: {}
              f:lastTransitionTime: {}
              f:status: {}
              f:type: {}
            k:{"type":"Initialized"}:
              .: {}
              f:lastProbeTime: {}
              f:lastTransitionTime: {}
              f:status: {}
              f:type: {}
            k:{"type":"Ready"}:
              .: {}
              f:lastProbeTime: {}
              f:lastTransitionTime: {}
              f:status: {}
              f:type: {}
          f:containerStatuses: {}
          f:hostIP: {}
          f:phase: {}
          f:podIP: {}
          f:podIPs:
            .: {}
            k:{"ip":"172.25.2.121"}:
              .: {}
              f:ip: {}
          f:startTime: {}
      manager: kubelet
      operation: Update
      time: "2021-06-29T12:32:01Z"
    - apiVersion: v1
      fieldsType: FieldsV1
      fieldsV1:
        f:metadata:
          f:annotations:
            f:k8s.v1.cni.cncf.io/network-status: {}
            f:k8s.v1.cni.cncf.io/networks-status: {}
      manager: multus
      operation: Update
      time: "2021-06-29T12:32:01Z"
    name: fedora-deployment-cc5c4748d-chsfr
    namespace: test-privs
    ownerReferences:
    - apiVersion: apps/v1
      blockOwnerDeletion: true
      controller: true
      kind: ReplicaSet
      name: fedora-deployment-cc5c4748d
      uid: e1cef632-15a0-4ce5-9566-87c7299a30c1
    resourceVersion: "9513628"
    selfLink: /api/v1/namespaces/test-privs/pods/fedora-deployment-cc5c4748d-chsfr
    uid: af723b92-897a-4d8e-8bd8-4656f229c12c
  spec:
    containers:
    - command:
      - bash
      - /entrypoint-scripts/entrypoint.sh
      image: fedora
      imagePullPolicy: IfNotPresent
      name: fedora
      resources: {}
      securityContext:
        capabilities:
          drop:
          - KILL
          - MKNOD
          - SETGID
          - SETUID
        runAsUser: 1000670000
      terminationMessagePath: /dev/termination-log
      terminationMessagePolicy: File
      volumeMounts:
      - mountPath: /entrypoint-scripts
        name: entrypoint-scripts
      - mountPath: /usr/local/bin
        name: usr-local-bin
      - mountPath: /var/run/secrets/kubernetes.io/serviceaccount
        name: test-privs-sa-token-65lfr
        readOnly: true
    dnsPolicy: ClusterFirst
    enableServiceLinks: true
    imagePullSecrets:
    - name: test-privs-sa-dockercfg-zt88r
    nodeName: openshift-worker-1
    preemptionPolicy: PreemptLowerPriority
    priority: 0
    restartPolicy: Always
    schedulerName: default-scheduler
    securityContext:
      fsGroup: 1000670000
      seLinuxOptions:
        level: s0:c26,c10
    serviceAccount: test-privs-sa
    serviceAccountName: test-privs-sa
    terminationGracePeriodSeconds: 30
    tolerations:
    - effect: NoExecute
      key: node.kubernetes.io/not-ready
      operator: Exists
      tolerationSeconds: 300
    - effect: NoExecute
      key: node.kubernetes.io/unreachable
      operator: Exists
      tolerationSeconds: 300
    volumes:
    - configMap:
        defaultMode: 420
        name: entrypoint-scripts
      name: entrypoint-scripts
    - emptyDir: {}
      name: usr-local-bin
    - name: test-privs-sa-token-65lfr
      secret:
        defaultMode: 420
        secretName: test-privs-sa-token-65lfr
  status:
    conditions:
    - lastProbeTime: null
      lastTransitionTime: "2021-06-29T12:33:23Z"
      status: "True"
      type: Initialized
    - lastProbeTime: null
      lastTransitionTime: "2021-06-29T12:33:28Z"
      status: "True"
      type: Ready
    - lastProbeTime: null
      lastTransitionTime: "2021-06-29T12:33:28Z"
      status: "True"
      type: ContainersReady
    - lastProbeTime: null
      lastTransitionTime: "2021-06-29T12:31:58Z"
      status: "True"
      type: PodScheduled
    containerStatuses:
    - containerID: cri-o://1cda3e9c2f68926aa300d30dbcac0c8c6a30f9af7c3d825798844fe811a72422
      image: docker.io/library/fedora:latest
      imageID: docker.io/library/fedora@sha256:6c8b3dea130cfa28babcb4d73450ed64f962e5464ab191bc0539487daf25d533
      lastState: {}
      name: fedora
      ready: true
      restartCount: 0
      started: true
      state:
        running:
          startedAt: "2021-06-29T12:33:27Z"
    hostIP: 192.168.123.221
    phase: Running
    podIP: 172.25.2.121
    podIPs:
    - ip: 172.25.2.121
    qosClass: BestEffort
    startTime: "2021-06-29T12:33:23Z"
kind: List
metadata:
  resourceVersion: ""
  selfLink: ""
~~~

This means that the `list` verb gives full access to all resources of permitted types.

This is as designed and is implied in:

* [https://kubernetes.io/docs/reference/generated/kubernetes-api/v1.19/](https://kubernetes.io/docs/reference/generated/kubernetes-api/v1.19/)

~~~
Read

Reads come in 3 forms: Get, List and Watch:

    Get: Get will retrieve a specific resource object by name.
    List: List will retrieve all resource objects of a specific type within a namespace, and the results can be restricted to resources matching a selector query.
    List All Namespaces: Like List but retrieves resources across all namespaces.
    Watch: Watch will stream results for an object(s) as it is updated. Similar to a callback, watch is used to respond to resource changes.
~~~
