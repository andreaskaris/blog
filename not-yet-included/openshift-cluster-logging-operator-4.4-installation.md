~~~
cat <<'EOF' > clo-instance-modified.yaml
apiVersion: "logging.openshift.io/v1"
kind: "ClusterLogging"
metadata:
  name: "instance" 
  namespace: "openshift-logging"
spec:
  managementState: "Managed"  
  logStore:
    type: "elasticsearch"  
    retentionPolicy: 
      application:
        maxAge: 1d
      infra:
        maxAge: 7d
      audit:
        maxAge: 7d
    elasticsearch:
      nodeCount: 3 
      storage:
        storageClassName: "standard" 
        size: 5G
      redundancyPolicy: "FullRedundancy"
      resources: 
        limits:
          memory: "16Gi"
        requests:
          cpu: "1"
          memory: "16Gi"
  visualization:
    type: "kibana"  
    kibana:
      replicas: 2
      resources:  
        limits:
          memory: 1Gi
        requests:
          cpu: 500m
          memory: 1Gi
  curation:
    type: "curator"
    curator:
      resources:
        limits:
          memory: 200Mi
        requests:
          cpu: 200m
          memory: 200Mi
      schedule: "*/5 * * * *"
  collection:
    logs:
      type: "fluentd"  
      fluentd:
        resources:
          limits:
            memory: 200Mi
          requests:
            cpu: 200m
            memory: 200Mi
EOF
cat <<'EOF' > clo-instance.yaml
apiVersion: "logging.openshift.io/v1"
kind: "ClusterLogging"
metadata:
  name: "instance" 
  namespace: "openshift-logging"
spec:
  managementState: "Managed"  
  logStore:
    type: "elasticsearch"  
    retentionPolicy: 
      application:
        maxAge: 1d
      infra:
        maxAge: 7d
      audit:
        maxAge: 7d
    elasticsearch:
      nodeCount: 3 
      storage:
        storageClassName: "standard" 
        size: 5G
      redundancyPolicy: "SingleRedundancy"
  visualization:
    type: "kibana"  
    kibana:
      replicas: 1
  curation:
    type: "curator"
    curator:
      schedule: "30 3 * * *" 
  collection:
    logs:
      type: "fluentd"  
      fluentd: {}
EOF
cat <<'EOF' > clo-namespace.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: openshift-logging
  annotations:
    openshift.io/node-selector: ""
  labels:
    openshift.io/cluster-monitoring: "true"
EOF
cat <<'EOF' > clo-og.yaml
apiVersion: operators.coreos.com/v1
kind: OperatorGroup
metadata:
  name: cluster-logging
  namespace: openshift-logging 
spec:
  targetNamespaces:
  - openshift-logging 
EOF
cat <<'EOF' > clo-sub.yaml
apiVersion: operators.coreos.com/v1alpha1
kind: Subscription
metadata:
  name: cluster-logging
  namespace: openshift-logging 
spec:
  channel: "4.4" 
  name: cluster-logging
  source: redhat-operators 
  sourceNamespace: openshift-marketplace
EOF
cat <<'EOF' > eo-namespace.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: openshift-operators-redhat 
  annotations:
    openshift.io/node-selector: ""
  labels:
    openshift.io/cluster-monitoring: "true" 
EOF
cat <<'EOF' > eo-og.yaml
apiVersion: operators.coreos.com/v1
kind: OperatorGroup
metadata:
  name: openshift-operators-redhat
  namespace: openshift-operators-redhat 
spec: {}
EOF
cat <<'EOF' > eo-sub.yaml
apiVersion: operators.coreos.com/v1alpha1
kind: Subscription
metadata:
  name: "elasticsearch-operator"
  namespace: "openshift-operators-redhat" 
spec:
  channel: "4.4" 
  installPlanApproval: "Automatic"
  source: "redhat-operators" 
  sourceNamespace: "openshift-marketplace"
  name: "elasticsearch-operator"
EOF
~~~

Install:
~~~
oc apply -f eo-namespace.yaml 
oc create -f clo-namespace.yaml
oc create -f eo-og.yaml
oc create -f eo-sub.yaml
oc create -f clo-og.yaml
oc create -f clo-sub.yaml
sleep 600 ; oc create -f clo-instance.yaml
sleep 120
~~~

Verification at various stages:
~~~
oc project openshift-logging
oc get csv
oc get pods
oc get deployments
oc get ds
pod=$(oc get pods | grep elasticsearch | awk '{print $1}' | head -1)
oc exec -n openshift-logging -c elasticsearch $pod -- es_cluster_health | grep status
oc exec -n openshift-logging -c elasticsearch $pod -- indices
oc get kibana kibana -o json
oc get cronjob -o name
oc get pvc
oc get pv
~~~
