# Troubleshooting OpenShift on OpenStack worker creation #

See [https://bugzilla.redhat.com/show_bug.cgi?id=1785705](https://bugzilla.redhat.com/show_bug.cgi?id=1785705) for the bugzilla.

The following are steps which I executed in my lab to troubleshoot issues with worker creation for OpenShift on OpenStack.

~~~
(overcloud) [stack@undercloud-0 clouds]$ oc describe machine -n openshift-machine-api osc-c5r5c-worker-bgt9b
Name:         osc-c5r5c-worker-bgt9b
Namespace:    openshift-machine-api
Labels:       machine.openshift.io/cluster-api-cluster=osc-c5r5c
              machine.openshift.io/cluster-api-machine-role=worker
              machine.openshift.io/cluster-api-machine-type=worker
              machine.openshift.io/cluster-api-machineset=osc-c5r5c-worker
Annotations:  <none>
API Version:  machine.openshift.io/v1beta1
Kind:         Machine
Metadata:
  Creation Timestamp:  2019-12-20T10:23:14Z
  Finalizers:
    machine.machine.openshift.io
  Generate Name:  osc-c5r5c-worker-
  Generation:     1
  Owner References:
    API Version:           machine.openshift.io/v1beta1
    Block Owner Deletion:  true
    Controller:            true
    Kind:                  MachineSet
    Name:                  osc-c5r5c-worker
    UID:                   9077b6df-2312-11ea-9b6c-fa163e431263
  Resource Version:        3455
  Self Link:               /apis/machine.openshift.io/v1beta1/namespaces/openshift-machine-api/machines/osc-c5r5c-worker-bgt9b
  UID:                     bb0af782-2312-11ea-9b6c-fa163e431263
Spec:
  Metadata:
    Creation Timestamp:  <nil>
  Provider Spec:
    Value:
      API Version:  openstackproviderconfig.openshift.io/v1alpha1
      Cloud Name:   openstack
      Clouds Secret:
        Name:       openstack-cloud-credentials
        Namespace:  openshift-machine-api
      Flavor:       m1.openshift
      Image:        rhcos
      Kind:         OpenstackProviderSpec
      Metadata:
        Creation Timestamp:  <nil>
      Networks:
        Filter:
        Subnets:
          Filter:
            Name:  osc-c5r5c-nodes
            Tags:  openshiftClusterID=osc-c5r5c
      Security Groups:
        Filter:
        Name:  osc-c5r5c-worker
      Server Metadata:
        Name:                  osc-c5r5c-worker
        Openshift Cluster ID:  osc-c5r5c
      Tags:
        openshiftClusterID=osc-c5r5c
      Trunk:  true
      User Data Secret:
        Name:  worker-user-data
Events:        <none>
(overcloud) [stack@undercloud-0 clouds]$ oc get machine -n openshift-machine-api 
NAME                     STATE   TYPE   REGION   ZONE   AGE
osc-c5r5c-master-0                                      4h52m
osc-c5r5c-master-1                                      4h52m
osc-c5r5c-master-2                                      4h52m
osc-c5r5c-worker-bgt9b                                  4h51m
osc-c5r5c-worker-qphk7                                  4h51m
osc-c5r5c-worker-vs85h                                  4h51m
(overcloud) [stack@undercloud-0 clouds]$ oc get machineset -n openshift-machine-api 
NAME               DESIRED   CURRENT   READY   AVAILABLE   AGE
osc-c5r5c-worker   3         3                             4h54m
(overcloud) [stack@undercloud-0 clouds]$ 
~~~

~~~
(overcloud) [stack@undercloud-0 clouds]$ kubectl get machineset -n openshift-machine-api osc-c5r5c-worker -o yaml
apiVersion: machine.openshift.io/v1beta1
kind: MachineSet
metadata:
  creationTimestamp: "2019-12-20T10:22:02Z"
  generation: 1
  labels:
    machine.openshift.io/cluster-api-cluster: osc-c5r5c
    machine.openshift.io/cluster-api-machine-role: worker
    machine.openshift.io/cluster-api-machine-type: worker
  name: osc-c5r5c-worker
  namespace: openshift-machine-api
  resourceVersion: "3448"
  selfLink: /apis/machine.openshift.io/v1beta1/namespaces/openshift-machine-api/machinesets/osc-c5r5c-worker
  uid: 9077b6df-2312-11ea-9b6c-fa163e431263
spec:
  replicas: 3
  selector:
    matchLabels:
      machine.openshift.io/cluster-api-cluster: osc-c5r5c
      machine.openshift.io/cluster-api-machineset: osc-c5r5c-worker
  template:
    metadata:
      creationTimestamp: null
      labels:
        machine.openshift.io/cluster-api-cluster: osc-c5r5c
        machine.openshift.io/cluster-api-machine-role: worker
        machine.openshift.io/cluster-api-machine-type: worker
        machine.openshift.io/cluster-api-machineset: osc-c5r5c-worker
    spec:
      metadata:
        creationTimestamp: null
      providerSpec:
        value:
          apiVersion: openstackproviderconfig.openshift.io/v1alpha1
          cloudName: openstack
          cloudsSecret:
            name: openstack-cloud-credentials
            namespace: openshift-machine-api
          flavor: m1.openshift
          image: rhcos
          kind: OpenstackProviderSpec
          metadata:
            creationTimestamp: null
          networks:
          - filter: {}
            subnets:
            - filter:
                name: osc-c5r5c-nodes
                tags: openshiftClusterID=osc-c5r5c
          securityGroups:
          - filter: {}
            name: osc-c5r5c-worker
          serverMetadata:
            Name: osc-c5r5c-worker
            openshiftClusterID: osc-c5r5c
          tags:
          - openshiftClusterID=osc-c5r5c
          trunk: true
          userDataSecret:
            name: worker-user-data
status:
  fullyLabeledReplicas: 3
  observedGeneration: 1
  replicas: 3
~~~

~~~
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-tm7qc_b1f05af2-2312-11ea-9b6c-fa163e431263/machine-controller/0.log
(...)
2019-12-20T10:23:14.635223358+00:00 stderr F E1220 10:23:14.635191       1 controller.go:239] Failed to check if machine "osc-c5r5c-worker-bgt9b" exists: Error checking if instance exists (machine/actuator.go 346): 
2019-12-20T10:23:14.635223358+00:00 stderr F Error getting a new instance service from the machine (machine/actuator.go 467): Create providerClient err: You must provide exactly one of DomainID or DomainName in a Scope with ProjectName
2019-12-20T10:23:15.636135623+00:00 stderr F I1220 10:23:15.635874       1 controller.go:133] Reconciling Machine "osc-c5r5c-master-0"
2019-12-20T10:23:15.636215058+00:00 stderr F I1220 10:23:15.636192       1 controller.go:304] Machine "osc-c5r5c-master-0" in namespace "openshift-machine-api" doesn't specify "cluster.k8s.io/cluster-name" label, assuming nil cluster
2019-12-20T10:23:15.641348382+00:00 stderr F E1220 10:23:15.641317       1 controller.go:239] Failed to check if machine "osc-c5r5c-master-0" exists: Error checking if instance exists (machine/actuator.go 346): 
2019-12-20T10:23:15.641348382+00:00 stderr F Error getting a new instance service from the machine (machine/actuator.go 467): Create providerClient err: You must provide exactly one of DomainID or DomainName in a Scope with ProjectName
2019-12-20T10:23:16.641614350+00:00 stderr F I1220 10:23:16.641562       1 controller.go:133] Reconciling Machine "osc-c5r5c-master-1"
2019-12-20T10:23:16.641614350+00:00 stderr F I1220 10:23:16.641591       1 controller.go:304] Machine "osc-c5r5c-master-1" in namespace "openshift-machine-api" doesn't specify "cluster.k8s.io/cluster-name" label, assuming nil cluster
2019-12-20T10:23:16.647323379+00:00 stderr F E1220 10:23:16.647295       1 controller.go:239] Failed to check if machine "osc-c5r5c-master-1" exists: Error checking if instance exists (machine/actuator.go 346): 
2019-12-20T10:23:16.647323379+00:00 stderr F Error getting a new instance service from the machine (machine/actuator.go 467): Create providerClient err: You must provide exactly one of DomainID or DomainName in a Scope with ProjectName
2019-12-20T10:23:17.647630531+00:00 stderr F I1220 10:23:17.647587       1 controller.go:133] Reconciling Machine "osc-c5r5c-master-2"
2019-12-20T10:23:17.647695173+00:00 stderr F I1220 10:23:17.647677       1 controller.go:304] Machine "osc-c5r5c-master-2" in namespace "openshift-machine-api" doesn't specify "cluster.k8s.io/cluster-name" label, assuming nil cluster
2019-12-20T10:23:17.652613228+00:00 stderr F E1220 10:23:17.652551       1 controller.go:239] Failed to check if machine "osc-c5r5c-master-2" exists: Error checking if instance exists (machine/actuator.go 346): 
2019-12-20T10:23:17.652613228+00:00 stderr F Error getting a new instance service from the machine (machine/actuator.go 467): Create providerClient err: You must provide exactly one of DomainID or DomainName in a Scope with ProjectName
2019-12-20T10:23:18.652945792+00:00 stderr F I1220 10:23:18.652884       1 controller.go:133] Reconciling Machine "osc-c5r5c-worker-qphk7"
~~~

Looking at the secret:
~~~
(overcloud) [stack@undercloud-0 clouds]$ kubectl get secrets -n openshift-machine-api openstack-cloud-credentials -o yaml
apiVersion: v1
data:
  clouds.yaml: Y2xvdWRzOgogIG9wZW5zdGFjazoKICAgIGF1dGg6CiAgICAgIGFwcGxpY2F0aW9uX2NyZWRlbnRpYWxfaWQ6ICIiCiAgICAgIGFwcGxpY2F0aW9uX2NyZWRlbnRpYWxfbmFtZTogIiIKICAgICAgYXBwbGljYXRpb25fY3JlZGVudGlhbF9zZWNyZXQ6ICIiCiAgICAgIGF1dGhfdXJsOiBodHRwOi8vMTcyLjE2LjAuMTMwOjUwMDAvL3YzCiAgICAgIGRlZmF1bHRfZG9tYWluOiAiIgogICAgICBkb21haW5faWQ6ICIiCiAgICAgIGRvbWFpbl9uYW1lOiAiIgogICAgICBwYXNzd29yZDogelA0YmUydWtocENrajR6cVJmVWs4WGpRYgogICAgICBwcm9qZWN0X2RvbWFpbl9pZDogIiIKICAgICAgcHJvamVjdF9kb21haW5fbmFtZTogIiIKICAgICAgcHJvamVjdF9pZDogIiIKICAgICAgcHJvamVjdF9uYW1lOiBhZG1pbgogICAgICB0b2tlbjogIiIKICAgICAgdXNlcl9kb21haW5faWQ6ICIiCiAgICAgIHVzZXJfZG9tYWluX25hbWU6IERlZmF1bHQKICAgICAgdXNlcl9pZDogIiIKICAgICAgdXNlcm5hbWU6IGFkbWluCiAgICBhdXRoX3R5cGU6ICIiCiAgICBjYWNlcnQ6ICIiCiAgICBjZXJ0OiAiIgogICAgY2xvdWQ6ICIiCiAgICBpZGVudGl0eV9hcGlfdmVyc2lvbjogIjMiCiAgICBrZXk6ICIiCiAgICBwcm9maWxlOiAiIgogICAgcmVnaW9uX25hbWU6IHJlZ2lvbk9uZQogICAgcmVnaW9uczogbnVsbAogICAgdmVyaWZ5OiB0cnVlCiAgICB2b2x1bWVfYXBpX3ZlcnNpb246ICIiCg==
kind: Secret
metadata:
  annotations:
    cloudcredential.openshift.io/credentials-request: openshift-cloud-credential-operator/openshift-machine-api-openstack
  creationTimestamp: "2019-12-20T10:23:00Z"
  name: openstack-cloud-credentials
  namespace: openshift-machine-api
  resourceVersion: "2525"
  selfLink: /api/v1/namespaces/openshift-machine-api/secrets/openstack-cloud-credentials
  uid: b2bded52-2312-11ea-9b6c-fa163e431263
type: Opaque
~~~

~~~
(overcloud) [stack@undercloud-0 clouds]$ base64 -d <(echo 'Y2xvdWRzOgogIG9wZW5zdGFjazoKICAgIGF1dGg6CiAgICAgIGFwcGxpY2F0aW9uX2NyZWRlbnRpYWxfaWQ6ICIiCiAgICAgIGFwcGxpY2F0aW9uX2NyZWRlbnRpYWxfbmFtZTogIiIKICAgICAgYXBwbGljYXRpb25fY3JlZGVudGlhbF9zZWNyZXQ6ICIiCiAgICAgIGF1dGhfdXJsOiBodHRwOi8vMTcyLjE2LjAuMTMwOjUwMDAvL3YzCiAgICAgIGRlZmF1bHRfZG9tYWluOiAiIgogICAgICBkb21haW5faWQ6ICIiCiAgICAgIGRvbWFpbl9uYW1lOiAiIgogICAgICBwYXNzd29yZDogelA0YmUydWtocENrajR6cVJmVWs4WGpRYgogICAgICBwcm9qZWN0X2RvbWFpbl9pZDogIiIKICAgICAgcHJvamVjdF9kb21haW5fbmFtZTogIiIKICAgICAgcHJvamVjdF9pZDogIiIKICAgICAgcHJvamVjdF9uYW1lOiBhZG1pbgogICAgICB0b2tlbjogIiIKICAgICAgdXNlcl9kb21haW5faWQ6ICIiCiAgICAgIHVzZXJfZG9tYWluX25hbWU6IERlZmF1bHQKICAgICAgdXNlcl9pZDogIiIKICAgICAgdXNlcm5hbWU6IGFkbWluCiAgICBhdXRoX3R5cGU6ICIiCiAgICBjYWNlcnQ6ICIiCiAgICBjZXJ0OiAiIgogICAgY2xvdWQ6ICIiCiAgICBpZGVudGl0eV9hcGlfdmVyc2lvbjogIjMiCiAgICBrZXk6ICIiCiAgICBwcm9maWxlOiAiIgogICAgcmVnaW9uX25hbWU6IHJlZ2lvbk9uZQogICAgcmVnaW9uczogbnVsbAogICAgdmVyaWZ5OiB0cnVlCiAgICB2b2x1bWVfYXBpX3ZlcnNpb246ICIiCg==')
clouds:
  openstack:
    auth:
      application_credential_id: ""
      application_credential_name: ""
      application_credential_secret: ""
      auth_url: http://172.16.0.130:5000//v3
      default_domain: ""
      domain_id: ""
      domain_name: ""
      password: zP4be2ukhpCkj4zqRfUk8XjQb
      project_domain_id: ""
      project_domain_name: ""
      project_id: ""
      project_name: admin
      token: ""
      user_domain_id: ""
      user_domain_name: Default
      user_id: ""
      username: admin
    auth_type: ""
    cacert: ""
    cert: ""
    cloud: ""
    identity_api_version: "3"
    key: ""
    profile: ""
    region_name: regionOne
    regions: null
    verify: true
    volume_api_version: ""
~~~


~~~
(overcloud) [stack@undercloud-0 clouds]$ cat clouds.yaml 
clouds:
  overcloud:
    auth:
      auth_url: http://172.16.0.130:5000//v3
      username: "admin"
      password: zP4be2ukhpCkj4zqRfUk8XjQb
      project_name: "admin"
      user_domain_name: "Default"
    region_name: "regionOne"
    interface: "public"
    identity_api_version: 3
~~~




The problem is in the generated secret:
~~~
Error getting a new instance service from the machine (machine/actuator.go 467): Create providerClient err: You must provide exactly one of DomainID or DomainName in a Scope with ProjectName
~~~

[https://github.com/terraform-providers/terraform-provider-openstack/issues/267](https://github.com/terraform-providers/terraform-provider-openstack/issues/267)


Whereas when I check here: [https://egallen.com/openshift-42-on-openstack-13-gpu/](https://egallen.com/openshift-42-on-openstack-13-gpu/)  This blog article uses project_id, too.
~~~

clouds:
  openstack:
    auth:
      auth_url: http://192.168.168.54:5000/v3
      username: "admin"
      password: XXXXXXXXXXXXXX
      project_id: XXXXXXXXX
      project_name: "admin"
      user_domain_name: "Default"
    region_name: "regionOne"
    interface: "public"
    identity_api_version: 3
~~~

However, my clouds.yaml file is actually completely correct:
~~~
[stack@undercloud-0 clouds]$ openstack --os-cloud overcloud token issue
+------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Field      | Value                                                                                                                                                                                   |
+------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| expires    | 2019-12-20T16:24:40+0000                                                                                                                                                                |
| id         | gAAAAABd_Oe4sLSSjwQiCPFhVK9PUFBehqVXbj-r96GdFvRieT51YZQUdm5lc5ic5VKYRFPg4jhPat4ZIdyow1QL-vZnxSK8MUAqUMQnc6xjs80JD-ibCNIg1Gac14Idp1CGIutsaUMS-Ms33LDgEw32S2qomv7LRUCLVcEBwrqwYLHXYE2ohyk |
| project_id | 1bb14f515f0945a4891fe3fa2372a795                                                                                                                                                        |
| user_id    | 0d3f5ab158c64c11b57d58c76d9675f0                                                                                                                                                        |
+------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
~~~

I then tried with:
~~~
clouds:
  openstack:
    auth:
      auth_url: http://172.16.0.130:5000//v3
      username: "admin"
      password: zP4be2ukhpCkj4zqRfUk8XjQb
      project_id: 1bb14f515f0945a4891fe3fa2372a795
      project_name: "admin"
      user_domain_name: "Default"
    region_name: "regionOne"
    interface: "public"
    identity_api_version: 3
~~~
> Note that I made 2 changes: project_id which I got from `openstack project list | grep admin` ; and I renamed the cloud credential to name `openstack`.

This actually worked:
~~~
(overcloud) [stack@undercloud-0 ~]$ openstack server list
+--------------------------------------+------------------------+--------+-------------------------------------------------------------------------+-------+--------------+
| ID                                   | Name                   | Status | Networks                                                                | Image | Flavor       |
+--------------------------------------+------------------------+--------+-------------------------------------------------------------------------+-------+--------------+
| 6726f1d5-977f-48da-9554-acf710d459fe | osc-6gzh2-worker-shw2w | ACTIVE | osc-6gzh2-openshift=172.31.0.28                                         | rhcos | m1.openshift |
| 37af5d51-bc08-4002-bb5b-380061d9efbf | osc-6gzh2-master-0     | ACTIVE | osc-6gzh2-openshift=172.31.0.24                                         | rhcos | m1.openshift |
| 4b52d095-e174-47ea-a699-85286f899f67 | osc-6gzh2-master-1     | ACTIVE | osc-6gzh2-openshift=172.31.0.16                                         | rhcos | m1.openshift |
| 73bb2012-d9ff-46e4-8e62-c6d723ca5711 | osc-6gzh2-master-2     | ACTIVE | osc-6gzh2-openshift=172.31.0.15                                         | rhcos | m1.openshift |
| 53e2da7f-1398-45d3-94e3-0008d8e209d9 | rhel-test1             | ACTIVE | private1=2000:192:168:0:f816:3eff:fe13:275, 192.168.0.113, 172.16.0.110 | rhel  | m1.small     |
+--------------------------------------+------------------------+--------+-------------------------------------------------------------------------+-------+--------------+
~~~

The cluster deployed:
~~~
(overcloud) [stack@undercloud-0 clouds]$ ./openshift-install create cluster --dir=install-config/ --log-level=info
INFO Consuming "Install Config" from target directory 
INFO Creating infrastructure resources...         
INFO Waiting up to 30m0s for the Kubernetes API at https://api.osc.redhat.local:6443... 
INFO API v1.14.6+17b1cc6 up                       
INFO Waiting up to 30m0s for bootstrapping to complete... 
INFO Destroying the bootstrap resources...        
INFO Waiting up to 30m0s for the cluster at https://api.osc.redhat.local:6443 to initialize... 
INFO Waiting up to 10m0s for the openshift-console route to be created... 
INFO Install complete!                            
INFO To access the cluster as the system:admin user when using 'oc', run 'export KUBECONFIG=/home/stack/clouds/install-config/auth/kubeconfig' 
INFO Access the OpenShift web-console here: https://console-openshift-console.apps.osc.redhat.local 
INFO Login to the console with user: kubeadmin, password: vFKnm-d43eY-aMMBB-52xcU 
~~~

Now, there was an issue with 2 of the machines:
~~~
(overcloud) [stack@undercloud-0 clouds]$ oc get machineset -A
NAMESPACE               NAME               DESIRED   CURRENT   READY   AVAILABLE   AGE
openshift-machine-api   osc-6gzh2-worker   3         3         1       1           97m
(overcloud) [stack@undercloud-0 clouds]$ oc get machine -A
NAMESPACE               NAME                     STATE    TYPE           REGION      ZONE   AGE
openshift-machine-api   osc-6gzh2-master-0       ACTIVE   m1.openshift   regionOne   nova   97m
openshift-machine-api   osc-6gzh2-master-1       ACTIVE   m1.openshift   regionOne   nova   97m
openshift-machine-api   osc-6gzh2-master-2       ACTIVE   m1.openshift   regionOne   nova   97m
openshift-machine-api   osc-6gzh2-worker-5d627   ERROR                                      96m
openshift-machine-api   osc-6gzh2-worker-j6j78   ERROR                                      96m
openshift-machine-api   osc-6gzh2-worker-shw2w   ACTIVE   m1.openshift   regionOne   nova   96m
(overcloud) [stack@undercloud-0 clouds]$ oc get machineconfig -A
NAME                                                        GENERATEDBYCONTROLLER                      IGNITIONVERSION   CREATED
00-master                                                   d780d197a9c5848ba786982c0c4aaa7487297046   2.2.0             96m
00-worker                                                   d780d197a9c5848ba786982c0c4aaa7487297046   2.2.0             96m
01-master-container-runtime                                 d780d197a9c5848ba786982c0c4aaa7487297046   2.2.0             96m
01-master-kubelet                                           d780d197a9c5848ba786982c0c4aaa7487297046   2.2.0             96m
01-worker-container-runtime                                 d780d197a9c5848ba786982c0c4aaa7487297046   2.2.0             96m
01-worker-kubelet                                           d780d197a9c5848ba786982c0c4aaa7487297046   2.2.0             96m
99-master-51c90b92-2340-11ea-baab-fa163e272495-registries   d780d197a9c5848ba786982c0c4aaa7487297046   2.2.0             96m
99-master-ssh                                                                                          2.2.0             96m
99-worker-51ca001b-2340-11ea-baab-fa163e272495-registries   d780d197a9c5848ba786982c0c4aaa7487297046   2.2.0             96m
99-worker-ssh                                                                                          2.2.0             96m
rendered-master-8e766b361a127ddfebde5802f36b90ce            d780d197a9c5848ba786982c0c4aaa7487297046   2.2.0             96m
rendered-worker-74267811bebab086ede643b9c9b2ba66            d780d197a9c5848ba786982c0c4aaa7487297046   2.2.0             96m
~~~

And it fails with:
~~~
(overcloud) [stack@undercloud-0 clouds]$ oc describe machine -n openshift-machine-api osc-6gzh2-worker-5d627 | tail
        Openshift Cluster ID:  osc-6gzh2
      Tags:
        openshiftClusterID=osc-6gzh2
      Trunk:  true
      User Data Secret:
        Name:  worker-user-data
Events:
  Type     Reason        Age                   From                  Message
  ----     ------        ----                  ----                  -------
  Warning  FailedCreate  2m30s (x23 over 95m)  openstack_controller  CreateError
(overcloud) [stack@undercloud-0 clouds]$ 
~~~

~~~
vercloud) [stack@undercloud-0 clouds]$ oc get events -n openshift-machine-api
LAST SEEN   TYPE      REASON              OBJECT                                              MESSAGE
95m         Normal    Scheduled           pod/cluster-autoscaler-operator-65dfcc75bb-gv8mg    Successfully assigned openshift-machine-api/cluster-autoscaler-operator-65dfcc75bb-gv8mg to osc-6gzh2-master-1
95m         Warning   FailedMount         pod/cluster-autoscaler-operator-65dfcc75bb-gv8mg    MountVolume.SetUp failed for volume "cert" : couldn't propagate object cache: timed out waiting for the condition
95m         Warning   FailedMount         pod/cluster-autoscaler-operator-65dfcc75bb-gv8mg    MountVolume.SetUp failed for volume "cluster-autoscaler-operator-token-gtb48" : couldn't propagate object cache: timed out waiting for the condition
95m         Warning   FailedMount         pod/cluster-autoscaler-operator-65dfcc75bb-gv8mg    MountVolume.SetUp failed for volume "ca-cert" : couldn't propagate object cache: timed out waiting for the condition
94m         Normal    Pulling             pod/cluster-autoscaler-operator-65dfcc75bb-gv8mg    Pulling image "quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:e6ea0a063874ff9169ef1e0e58c6399a42e163c49321018fa34c838faec99cb4"
94m         Normal    Pulled              pod/cluster-autoscaler-operator-65dfcc75bb-gv8mg    Successfully pulled image "quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:e6ea0a063874ff9169ef1e0e58c6399a42e163c49321018fa34c838faec99cb4"
94m         Normal    Created             pod/cluster-autoscaler-operator-65dfcc75bb-gv8mg    Created container cluster-autoscaler-operator
94m         Normal    Started             pod/cluster-autoscaler-operator-65dfcc75bb-gv8mg    Started container cluster-autoscaler-operator
95m         Normal    SuccessfulCreate    replicaset/cluster-autoscaler-operator-65dfcc75bb   Created pod: cluster-autoscaler-operator-65dfcc75bb-gv8mg
94m         Normal    LeaderElection      configmap/cluster-autoscaler-operator-leader        cluster-autoscaler-operator-65dfcc75bb-gv8mg_40da4183-2341-11ea-8630-0a58ac150025 became leader
95m         Normal    ScalingReplicaSet   deployment/cluster-autoscaler-operator              Scaled up replica set cluster-autoscaler-operator-65dfcc75bb to 1
101m        Normal    Scheduled           pod/machine-api-controllers-f64b7f7b8-xb7qn         Successfully assigned openshift-machine-api/machine-api-controllers-f64b7f7b8-xb7qn to osc-6gzh2-master-2
101m        Normal    Pulling             pod/machine-api-controllers-f64b7f7b8-xb7qn         Pulling image "quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:fad6250c0e717515d4caf35d6e6e006f7154e2a7bd9d4fec73f0540e155e3119"
101m        Normal    Pulled              pod/machine-api-controllers-f64b7f7b8-xb7qn         Successfully pulled image "quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:fad6250c0e717515d4caf35d6e6e006f7154e2a7bd9d4fec73f0540e155e3119"
101m        Normal    Created             pod/machine-api-controllers-f64b7f7b8-xb7qn         Created container controller-manager
101m        Normal    Started             pod/machine-api-controllers-f64b7f7b8-xb7qn         Started container controller-manager
101m        Normal    Pulled              pod/machine-api-controllers-f64b7f7b8-xb7qn         Container image "quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:fad6250c0e717515d4caf35d6e6e006f7154e2a7bd9d4fec73f0540e155e3119" already present on machine
101m        Normal    Created             pod/machine-api-controllers-f64b7f7b8-xb7qn         Created container machine-controller
101m        Normal    Started             pod/machine-api-controllers-f64b7f7b8-xb7qn         Started container machine-controller
101m        Normal    Pulled              pod/machine-api-controllers-f64b7f7b8-xb7qn         Container image "quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8f1a9c01710f09ad1d1c105cbc4f4ff88d7c0f5916a628f9414d3a7905fbced8" already present on machine
101m        Normal    Created             pod/machine-api-controllers-f64b7f7b8-xb7qn         Created container nodelink-controller
101m        Normal    Started             pod/machine-api-controllers-f64b7f7b8-xb7qn         Started container nodelink-controller
101m        Normal    SuccessfulCreate    replicaset/machine-api-controllers-f64b7f7b8        Created pod: machine-api-controllers-f64b7f7b8-xb7qn
101m        Normal    ScalingReplicaSet   deployment/machine-api-controllers                  Scaled up replica set machine-api-controllers-f64b7f7b8 to 1
101m        Warning   FailedScheduling    pod/machine-api-operator-655d94c8fd-6vrqg           0/3 nodes are available: 3 node(s) had taints that the pod didn't tolerate.
101m        Normal    Scheduled           pod/machine-api-operator-655d94c8fd-6vrqg           Successfully assigned openshift-machine-api/machine-api-operator-655d94c8fd-6vrqg to osc-6gzh2-master-2
101m        Normal    Pulling             pod/machine-api-operator-655d94c8fd-6vrqg           Pulling image "quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8f1a9c01710f09ad1d1c105cbc4f4ff88d7c0f5916a628f9414d3a7905fbced8"
101m        Normal    Pulled              pod/machine-api-operator-655d94c8fd-6vrqg           Successfully pulled image "quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8f1a9c01710f09ad1d1c105cbc4f4ff88d7c0f5916a628f9414d3a7905fbced8"
101m        Normal    Created             pod/machine-api-operator-655d94c8fd-6vrqg           Created container machine-api-operator
101m        Normal    Started             pod/machine-api-operator-655d94c8fd-6vrqg           Started container machine-api-operator
102m        Normal    SuccessfulCreate    replicaset/machine-api-operator-655d94c8fd          Created pod: machine-api-operator-655d94c8fd-6vrqg
102m        Normal    ScalingReplicaSet   deployment/machine-api-operator                     Scaled up replica set machine-api-operator-655d94c8fd to 1
6m55s       Warning   FailedCreate        machine/osc-6gzh2-worker-5d627                      CreateError
7m29s       Warning   FailedCreate        machine/osc-6gzh2-worker-j6j78                      CreateError
98m         Warning   FailedCreate        machine/osc-6gzh2-worker-shw2w                      CreateError
97m         Normal    Created             machine/osc-6gzh2-worker-shw2w                      Created Machine osc-6gzh2-worker-shw2w
~~~


I connected to master 2 via master 1:
~~~
[stack@undercloud-0 ~]$ eval $(ssh-agent)
[stack@undercloud-0 ~]$ ssh-add  ~/.ssh/id_rsa
[stack@undercloud-0 ~]$ ssh core@api.osc.redhat.local -A
[stack@undercloud-0 ~]$ ssh core@api.osc.redhat.local -A
Warning: Permanently added 'api.osc.redhat.local,172.16.0.108' (ECDSA) to the list of known hosts.
Red Hat Enterprise Linux CoreOS 42.81.20191203.0
WARNING: Direct SSH access to machines is not recommended.

---
Last login: Fri Dec 20 17:27:11 2019 from 172.16.0.65
[core@osc-6gzh2-master-1 ~]$ ssh core@osc-6gzh2-master-0
Red Hat Enterprise Linux CoreOS 42.81.20191203.0
WARNING: Direct SSH access to machines is not recommended.

---
Last login: Fri Dec 20 17:27:13 2019 from 172.31.0.16
sud[core@osc-6gzh2-master-0 ~]$ sudo -i
[root@osc-6gzh2-master-0 ~]# 
~~~

And then got:
~~~
[root@osc-6gzh2-master-2 ~]# grep osc-6gzh2-worker-5d627 /var/log -R
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:49:41.304368389+00:00 stderr F I1220 15:49:41.304339       1 controller.go:133] Reconciling Machine "osc-6gzh2-worker-5d627"
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:49:41.304368389+00:00 stderr F I1220 15:49:41.304356       1 controller.go:304] Machine "osc-6gzh2-worker-5d627" in namespace "openshift-machine-api" doesn't specify "cluster.k8s.io/cluster-name" label, assuming nil cluster
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:50:37.383723384+00:00 stderr F I1220 15:50:37.383397       1 controller.go:133] Reconciling Machine "osc-6gzh2-worker-5d627"
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:50:37.383835414+00:00 stderr F I1220 15:50:37.383814       1 controller.go:304] Machine "osc-6gzh2-worker-5d627" in namespace "openshift-machine-api" doesn't specify "cluster.k8s.io/cluster-name" label, assuming nil cluster
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:50:38.552995792+00:00 stderr F I1220 15:50:38.552954       1 controller.go:253] Reconciling machine object osc-6gzh2-worker-5d627 triggers idempotent create.
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:50:52.700244622+00:00 stderr F E1220 15:50:52.698880       1 actuator.go:470] Machine error osc-6gzh2-worker-5d627: error creating Openstack instance: Create new server err: Request forbidden: [POST http://172.16.0.130:8774/v2.1/servers], error message: {"forbidden": {"message": "Quota exceeded for ram: Requested 32768, but already used 132096 of 153600 ram", "code": 403}}
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:50:52.700244622+00:00 stderr F W1220 15:50:52.698926       1 controller.go:255] Failed to create machine "osc-6gzh2-worker-5d627": error creating Openstack instance: Create new server err: Request forbidden: [POST http://172.16.0.130:8774/v2.1/servers], error message: {"forbidden": {"message": "Quota exceeded for ram: Requested 32768, but already used 132096 of 153600 ram", "code": 403}}
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:51:38.126016932+00:00 stderr F I1220 15:51:38.125931       1 controller.go:133] Reconciling Machine "osc-6gzh2-worker-5d627"
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:51:38.126016932+00:00 stderr F I1220 15:51:38.125971       1 controller.go:304] Machine "osc-6gzh2-worker-5d627" in namespace "openshift-machine-api" doesn't specify "cluster.k8s.io/cluster-name" label, assuming nil cluster
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:51:39.076618133+00:00 stderr F I1220 15:51:39.076540       1 controller.go:253] Reconciling machine object osc-6gzh2-worker-5d627 triggers idempotent create.
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:51:48.625264976+00:00 stderr F E1220 15:51:48.625215       1 actuator.go:470] Machine error osc-6gzh2-worker-5d627: error creating Openstack instance: Create new server err: Request forbidden: [POST http://172.16.0.130:8774/v2.1/servers], error message: {"forbidden": {"message": "Quota exceeded for ram: Requested 32768, but already used 132096 of 153600 ram", "code": 403}}
/var/log/pods/openshift-machine-api_machine-api-controllers-f64b7f7b8-xb7qn_4d35a879-2340-11ea-baab-fa163e272495/machine-controller/0.log:2019-12-20T15:51:48.625264976+00:00 stderr F W1220 15:51:48.625239       1 controller.go:255] Failed to create machine "osc-6gzh2-worker-5d627": error creating Openstack instance: Create new server err: Request forbidden: [POST http://172.16.0.130:8774/v2.1/servers], error message: {"forbidden": {"message": "Quota exceeded for ram: Requested 32768, but already used 132096 of 153600 ram", "code": 403}}
~~~
