### Understanding the Horizontal Pod Autoscaler (HPA)

The basics for Horiontal Pod Autoscaler (HPA) are well explained in:

* [https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/#support-for-metrics-apis](https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/#support-for-metrics-apis)
* [https://docs.openshift.com/container-platform/4.6/nodes/pods/nodes-pods-autoscaling.html](https://docs.openshift.com/container-platform/4.6/nodes/pods/nodes-pods-autoscaling.html)

For OpenShift 4.x, consider that there is a bug with the `v1` API for autoscaling. However, the `v2beta2` API works fine:

* [https://access.redhat.com/solutions/5428871](https://access.redhat.com/solutions/5428871)

#### Getting the examples

Clone the following repository. Then, go to directory `hpa-test`:
~~~
git clone https://github.com/andreaskaris/kubernetes-tools.git
cd kubernetes/hpa-test
~~~

You will find all required resources for the following steps. See `REAME.md` in case you want to build your own `hpa-tester` image.

#### Deploying an HPA tester deployment

First, deploy the HPA-Tester deployment. The deployment runs 2 applications which simulate a cumulative load.  Meaning that `COMBINED_CPU_MS` and `COMBINED_MEMORY_MB` will be divided by the number of replicas for the deployment and each pod will then individually run that fraction of memory and CPU load. This check happens every `SLEEP_TIME` seconds.

Deploy the deployment in a dedicated namespace with:
~~~
oc new-project hpa-test || oc project hpa-test
oc apply -f role-list-deployments-pods.yaml
oc apply -f role-binding-list-deployments-pods.yaml
oc apply -f deployment-hpa-tester.yaml
~~~

If you change the replica count, the `entrypoint.sh` will make sure to distribute the memory MB and CPU ms between the number of replicas.

#### Scaling basics based on the average CPU scaler metric

For the most basic example, `.spec.template.spec.containers[0].resources` is:
~~~
        resources:
          requests:
            cpu: "1000m"
            memory: "1024Mi"
~~~

The Horizontal Pod Autoscaler will by default compare a pod's actual CPU usage to the pod's requested values:
[https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/](https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/)
~~~
For per-pod resource metrics (like CPU), the controller fetches the metrics from the resource metrics API for each Pod targeted by the HorizontalPodAutoscaler. Then, if a target utilization value is set, the controller calculates the utilization value as a percentage of the equivalent resource request on the containers in each Pod. If a target raw value is set, the raw metric values are used directly. The controller then takes the mean of the utilization or the raw value (depending on the type of target specified) across all targeted Pods, and produces a ratio used to scale the number of desired replicas.

Please note that if some of the Pod's containers do not have the relevant resource request set, CPU utilization for the Pod will not be defined and the autoscaler will not take any action for that metric. See the algorithm details section below for more information about how the autoscaling algorithm works.
~~~

That also means that it is required to set resource requests for the containers.

~~~
[root@openshift-jumpserver-0 hpa-test]# oc get pods
NAME                          READY   STATUS    RESTARTS   AGE
hpa-tester-7bff6856bd-wgmct   1/1     Running   0          3m2s
[root@openshift-jumpserver-0 hpa-test]# oc get podmetrics
NAME                          CPU     MEMORY      WINDOW
hpa-tester-7bff6856bd-wgmct   1611m   4218724Ki   5m0s
~~~

~~~
[root@openshift-jumpserver-0 hpa-test]# oc describe pod hpa-tester-7bff6856bd-wgmct | grep Requests -A2
    Requests:
      cpu:     1
      memory:  1Gi
~~~

When starting with the above configuration, the current CPU utilization for the pod is 1600ms / 1000ms (a request of one full CPU unit, and a cumulative load of 1600 ms), meaning the load is 160%

Now, deploy the following HPA resource:
~~~
apiVersion: autoscaling/v2beta2
kind: HorizontalPodAutoscaler
metadata:
  annotations:
  name: hpa-tester
  namespace: test
spec:
  maxReplicas: 20
  minReplicas: 1
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: hpa-tester
  metrics: 
  - type: Resource
    resource:
      name: cpu 
      target:
        type: Utilization
        averageUtilization: 60
~~~

The target load is 60%.

Looking at the algorithm data for this simple example:
~~~
desiredReplicas = ceil[currentReplicas * ( currentMetricValue / desiredMetricValue )]
~~~

That means `ceil[1 * ( 160% / 60 % )] = 3`

Let's test this:
~~~
[root@openshift-jumpserver-0 hpa-test]# oc apply -f hpa.yaml 
horizontalpodautoscaler.autoscaling/hpa-tester created
[root@openshift-jumpserver-0 hpa-test]# oc get hpa
NAME         REFERENCE               TARGETS         MINPODS   MAXPODS   REPLICAS   AGE
hpa-tester   Deployment/hpa-tester   <unknown>/60%   1         20        0          3s
~~~

After a while, we will see that the HPA scaled the deployment to 3 pods:
~~~
[root@openshift-jumpserver-0 hpa-test]# oc get hpa
NAME         REFERENCE               TARGETS   MINPODS   MAXPODS   REPLICAS   AGE
hpa-tester   Deployment/hpa-tester   55%/60%   1         20        3          3m34s
[root@openshift-jumpserver-0 hpa-test]# oc get pods
NAME                          READY   STATUS    RESTARTS   AGE
hpa-tester-7bff6856bd-c5hfj   1/1     Running   0          3m20s
hpa-tester-7bff6856bd-lk2pv   1/1     Running   0          3m20s
hpa-tester-7bff6856bd-wgmct   1/1     Running   0          10m
[root@openshift-jumpserver-0 hpa-test]# oc get podmetrics
NAME                          CPU    MEMORY      WINDOW
hpa-tester-7bff6856bd-c5hfj   543m   1413984Ki   5m0s
hpa-tester-7bff6856bd-lk2pv   533m   1439692Ki   5m0s
hpa-tester-7bff6856bd-wgmct   547m   1447412Ki   5m0s
~~~

#### Scaling based on absolute CPU values

It is also possible to scale for a specific CPU value. First, scale back the deployment to 1 replica or delete and recreate it. Then, apply the following `HorizontalPodAutoscaler`:
~~~
apiVersion: autoscaling/v2beta2
kind: HorizontalPodAutoscaler
metadata:
  annotations:
  name: hpa-tester
  namespace: test
spec:
  maxReplicas: 20
  minReplicas: 1
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: hpa-tester
  metrics: 
  - type: Resource
    resource:
      name: cpu
      target:
        type: AverageValue
        averageValue: 200m
~~~

And wait for 5 minutes. The result will be 9 pods - ceil [(1600ms + overhead) / 800ms]:
~~~
[root@openshift-jumpserver-0 hpa-test]# oc get hpa
NAME         REFERENCE               TARGETS     MINPODS   MAXPODS   REPLICAS   AGE
hpa-tester   Deployment/hpa-tester   199m/200m   1         20        9          6m51s
[root@openshift-jumpserver-0 hpa-test]# oc get pods
NAME                          READY   STATUS    RESTARTS   AGE
hpa-tester-7bff6856bd-2dnjn   1/1     Running   0          2m22s
hpa-tester-7bff6856bd-5pf46   1/1     Running   0          7m16s
hpa-tester-7bff6856bd-bx49l   1/1     Running   0          2m7s
hpa-tester-7bff6856bd-d487n   1/1     Running   0          2m22s
hpa-tester-7bff6856bd-h2kd8   1/1     Running   0          112s
hpa-tester-7bff6856bd-mmk7c   1/1     Running   0          2m22s
hpa-tester-7bff6856bd-n48nj   1/1     Running   0          2m7s
hpa-tester-7bff6856bd-qhjtl   1/1     Running   0          2m7s
hpa-tester-7bff6856bd-wx8r8   1/1     Running   0          2m7s
[root@openshift-jumpserver-0 hpa-test]# oc get podmetrics
NAME                          CPU    MEMORY     WINDOW
hpa-tester-7bff6856bd-2dnjn   192m   478724Ki   5m0s
hpa-tester-7bff6856bd-5pf46   199m   509580Ki   5m0s
hpa-tester-7bff6856bd-bx49l   194m   507920Ki   5m0s
hpa-tester-7bff6856bd-d487n   189m   507564Ki   5m0s
hpa-tester-7bff6856bd-h2kd8   190m   503232Ki   5m0s
hpa-tester-7bff6856bd-mmk7c   200m   503428Ki   5m0s
hpa-tester-7bff6856bd-n48nj   202m   506848Ki   5m0s
hpa-tester-7bff6856bd-qhjtl   197m   477332Ki   5m0s
hpa-tester-7bff6856bd-wx8r8   194m   479528Ki   5m0s
~~~

#### Scaling based on memory utiliation

It is also possible to scale for a specific memory utiliation. First, scale back the deployment to 1 replica or delete and recreate i
t. Then, apply the following `HorizontalPodAutoscaler`:
~~~
apiVersion: autoscaling/v2beta2
kind: HorizontalPodAutoscaler
metadata:
  annotations:
  name: hpa-tester
  namespace: test
spec:
  maxReplicas: 20
  minReplicas: 1
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: hpa-tester
  metrics:
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 100
~~~

Apply the HPA and verify:
~~~
[root@openshift-jumpserver-0 hpa-test]# oc apply -f hpa.yaml 
oc ghorizontalpodautoscaler.autoscaling/hpa-tester created
[root@openshift-jumpserver-0 hpa-test]# oc get hpa
NAME         REFERENCE               TARGETS          MINPODS   MAXPODS   REPLICAS   AGE
hpa-tester   Deployment/hpa-tester   <unknown>/100%   1         20        0          2s
[root@openshift-jumpserver-0 hpa-test]# oc describe hpa hpa-tester
Name:                                                     hpa-tester
Namespace:                                                test
Labels:                                                   <none>
Annotations:                                              <none>
CreationTimestamp:                                        Mon, 01 Mar 2021 10:56:02 +0000
Reference:                                                Deployment/hpa-tester
Metrics:                                                  ( current / target )
  resource memory on pods  (as a percentage of request):  <unknown> / 100%
Min replicas:                                             1
Max replicas:                                             20
Deployment pods:                                          0 current / 0 desired
Events:                                                   <none>
~~~

And wait for 5 minutes. With 0 overhead inside the pods, the result be 4 pods - 4096 MB (cumulative Memory) / 1024 MB (request container spec) = 400%, then 400% / 100% = 4). Given that the cumulative memory utilization will be a bit higher than 4096 MB, HPA will scale out to 5 pods:
~~~
[root@openshift-jumpserver-0 hpa-test]# oc get hpa
NAME         REFERENCE               TARGETS    MINPODS   MAXPODS   REPLICAS   AGE
hpa-tester   Deployment/hpa-tester   83%/100%   1         20        5          4m19s
[root@openshift-jumpserver-0 hpa-test]# oc get pods
NAME                          READY   STATUS    RESTARTS   AGE
hpa-tester-7bff6856bd-4dxgb   1/1     Running   0          4m6s
hpa-tester-7bff6856bd-7r4r8   1/1     Running   0          3m51s
hpa-tester-7bff6856bd-h45s7   1/1     Running   0          5m23s
hpa-tester-7bff6856bd-nxp29   1/1     Running   0          4m6s
hpa-tester-7bff6856bd-trqv9   1/1     Running   0          4m6s
[root@openshift-jumpserver-0 hpa-test]# oc get podmetrics
NAME                          CPU    MEMORY     WINDOW
hpa-tester-7bff6856bd-4dxgb   338m   878064Ki   5m0s
hpa-tester-7bff6856bd-7r4r8   347m   886932Ki   5m0s
hpa-tester-7bff6856bd-h45s7   338m   854892Ki   5m0s
hpa-tester-7bff6856bd-nxp29   337m   880600Ki   5m0s
hpa-tester-7bff6856bd-trqv9   342m   878492Ki   5m0s
~~~

#### Scaling based on absolute memory values

It is also possible to scale for a specific CPU value. First, scale back the deployment to 1 replica or delete and recreate i
t. Then, apply the following `HorizontalPodAutoscaler`:
~~~
apiVersion: autoscaling/v2beta2
kind: HorizontalPodAutoscaler
metadata:
  annotations:
  name: hpa-tester
  namespace: test
spec:
  maxReplicas: 20
  minReplicas: 1
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: hpa-tester
  metrics:
  - type: Resource
    resource:
      name: memory
      target:
        type: AverageValue
        averageValue: "2048Mi"
~~~

Verify after deploying this HPA:
~~~
[root@openshift-jumpserver-0 hpa-test]# oc get hpa
NAME         REFERENCE               TARGETS         MINPODS   MAXPODS   REPLICAS   AGE
hpa-tester   Deployment/hpa-tester   <unknown>/2Gi   1         20        0          3s
[root@openshift-jumpserver-0 hpa-test]# oc describe hpa hpa-tester
Name:                       hpa-tester
Namespace:                  test
Labels:                     <none>
Annotations:                <none>
CreationTimestamp:          Mon, 01 Mar 2021 11:01:49 +0000
Reference:                  Deployment/hpa-tester
Metrics:                    ( current / target )
  resource memory on pods:  <unknown> / 2Gi
Min replicas:               1
Max replicas:               20
Deployment pods:            0 current / 0 desired
Events:                     <none>
~~~

And wait for 5 minutes. The result will be 3 pods ceil[(4096+overhead MB / 2048 MB)]:
~~~
[root@openshift-jumpserver-0 hpa-test]# oc get hpa
NAME         REFERENCE               TARGETS              MINPODS   MAXPODS   REPLICAS   AGE
hpa-tester   Deployment/hpa-tester   1476487850666m/2Gi   1         20        3          16m
[root@openshift-jumpserver-0 hpa-test]# oc get pods
NAME                          READY   STATUS    RESTARTS   AGE
hpa-tester-7bff6856bd-6mvbv   1/1     Running   0          16m
hpa-tester-7bff6856bd-mpjvb   1/1     Running   0          17m
hpa-tester-7bff6856bd-t5xc4   1/1     Running   0          16m
[root@openshift-jumpserver-0 hpa-test]# oc get podmetrics
NAME                          CPU    MEMORY      WINDOW
hpa-tester-7bff6856bd-6mvbv   552m   1446040Ki   5m0s
hpa-tester-7bff6856bd-mpjvb   542m   1425156Ki   5m0s
hpa-tester-7bff6856bd-t5xc4   557m   1449152Ki   5m0s
~~~

### Combining CPU and memory metrics

It is also possible to combine CPU and memory targets. For example:
~~~
apiVersion: autoscaling/v2beta2
kind: HorizontalPodAutoscaler
metadata:
  annotations:
  name: hpa-tester
  namespace: test
spec:
  maxReplicas: 20
  minReplicas: 1
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: hpa-tester
  metrics: 
  - type: Resource
    resource:
      name: memory
      target:
        type: AverageValue
        averageValue: "2048Mi"
  - type: Resource
    resource:
      name: cpu
      target:
        type: AverageValue
        averageValue: "200m"
~~~

Verification:
~~~
[root@openshift-jumpserver-0 hpa-test]# oc get hpa
NAME         REFERENCE               TARGETS                         MINPODS   MAXPODS   REPLICAS   AGE
hpa-tester   Deployment/hpa-tester   <unknown>/2Gi, <unknown>/200m   1         20        0          7s
~~~

Wait for a short while and the memory metrics will already lead to a scale-out to 3 pods:
~~~
[root@openshift-jumpserver-0 hpa-test]# oc get hpa
NAME         REFERENCE               TARGETS                          MINPODS   MAXPODS   REPLICAS   AGE
hpa-tester   Deployment/hpa-tester   4344119296/2Gi, <unknown>/200m   1         20        3          48s
[root@openshift-jumpserver-0 hpa-test]# oc describe hpa
Name:                       hpa-tester
Namespace:                  test
Labels:                     <none>
Annotations:                <none>
CreationTimestamp:          Mon, 01 Mar 2021 11:41:03 +0000
Reference:                  Deployment/hpa-tester
Metrics:                    ( current / target )
  resource memory on pods:  4343648256 / 2Gi
  resource cpu on pods:     <unknown> / 200m
Min replicas:               1
Max replicas:               20
Deployment pods:            1 current / 3 desired
Conditions:
  Type            Status  Reason              Message
  ----            ------  ------              -------
  AbleToScale     True    SucceededRescale    the HPA controller was able to update the target scale to 3
  ScalingActive   True    ValidMetricFound    the HPA was able to successfully calculate a replica count from memory resource
  ScalingLimited  False   DesiredWithinRange  the desired count is within the acceptable range
Events:
  Type     Reason                   Age   From                       Message
  ----     ------                   ----  ----                       -------
  Warning  FailedGetResourceMetric  11s   horizontal-pod-autoscaler  did not receive metrics for any ready pods
  Normal   SuccessfulRescale        10s   horizontal-pod-autoscaler  New size: 3; reason: memory resource above target
[root@openshift-jumpserver-0 hpa-test]# oc get pods
NAME                          READY   STATUS    RESTARTS   AGE
hpa-tester-7bff6856bd-4mlh6   1/1     Running   0          57s
hpa-tester-7bff6856bd-c7862   1/1     Running   0          2m8s
hpa-tester-7bff6856bd-vm27z   1/1     Running   0          57s
~~~

It will take a bit longer for the environment to calculate the CPU averages but eventually a second scale-out step will follow to 9 pods:
~~~
[root@openshift-jumpserver-0 hpa-test]# oc get hpa
NAME         REFERENCE               TARGETS                        MINPODS   MAXPODS   REPLICAS   AGE
hpa-tester   Deployment/hpa-tester   500336867555m/2Gi, 190m/200m   1         20        9          6m37s
[root@openshift-jumpserver-0 hpa-test]# oc get pods
NAME                          READY   STATUS    RESTARTS   AGE
hpa-tester-7bff6856bd-4mlh6   1/1     Running   0          6m28s
hpa-tester-7bff6856bd-6c968   1/1     Running   0          86s
hpa-tester-7bff6856bd-c7862   1/1     Running   0          7m39s
hpa-tester-7bff6856bd-cj8dz   1/1     Running   0          86s
hpa-tester-7bff6856bd-ngp4l   1/1     Running   0          102s
hpa-tester-7bff6856bd-nwx7r   1/1     Running   0          101s
hpa-tester-7bff6856bd-tg6vb   1/1     Running   0          86s
hpa-tester-7bff6856bd-vm27z   1/1     Running   0          6m28s
hpa-tester-7bff6856bd-xfptr   1/1     Running   0          101s
[root@openshift-jumpserver-0 hpa-test]# oc get podmetrics
NAME                          CPU    MEMORY     WINDOW
hpa-tester-7bff6856bd-4mlh6   199m   508864Ki   5m0s
hpa-tester-7bff6856bd-6c968   185m   478548Ki   5m0s
hpa-tester-7bff6856bd-c7862   181m   484504Ki   5m0s
hpa-tester-7bff6856bd-cj8dz   193m   476976Ki   5m0s
hpa-tester-7bff6856bd-ngp4l   194m   477744Ki   5m0s
hpa-tester-7bff6856bd-nwx7r   195m   507352Ki   5m0s
hpa-tester-7bff6856bd-tg6vb   191m   478620Ki   5m0s
hpa-tester-7bff6856bd-vm27z   181m   484528Ki   5m0s
hpa-tester-7bff6856bd-xfptr   209m   505692Ki   5m0s
~~~

### Custom application metrics for auto-scaling in OpenShift

* [https://docs.openshift.com/container-platform/4.6/monitoring/exposing-custom-application-metrics-for-autoscaling.html](https://docs.openshift.com/container-platform/4.6/monitoring/exposing-custom-application-metrics-for-autoscaling.html)
