### Building custom images with OpenShift ###

Note that with the below Dockerfile, we run into the caveat that the image requires to be run as user apache. So, if we are in the default project and are executing all of this as the admin user, we need to execute this as a prerequisite:
~~~
[root@master-2 ~]# oc adm policy add-scc-to-user anyuid -z default
scc "anyuid" added to: ["system:serviceaccount:default:default"]
~~~
> **Note:** That's from https://blog.openshift.com/jupyter-openshift-part-2-using-jupyter-project-images/

Otherwise, follow: 
* https://blog.openshift.com/jupyter-on-openshift-part-6-running-as-an-assigned-user-id/

Create the following file `buildconfig.yaml`:
~~~
apiVersion: image.openshift.io/v1
kind: ImageStream
metadata:
  creationTimestamp: null
  generation: 1
  name: fh
  selfLink: /apis/image.openshift.io/v1/namespaces/default/imagestreams/fh
spec:
  lookupPolicy:
    local: false
status:
  dockerImageRepository: docker-registry.default.svc:5000/fh
---
apiVersion: v1
data:
  run-apache.sh: |
    #!/bin/bash

    /usr/sbin/httpd $OPTIONS -DFOREGROUND
kind: ConfigMap
metadata:
  creationTimestamp: null
  name: run-apache
  selfLink: /api/v1/namespaces/default/configmaps/run-apache
---
apiVersion: v1
kind: BuildConfig
metadata:
  name: fh-build
spec:
  source:
    configMaps:
      - configMap:
          name: run-apache
    dockerfile: |
       FROM fedora
       EXPOSE 8080
       RUN yum install httpd -y
       RUN yum install tcpdump -y
       RUN yum install iproute -y
       RUN yum install procps-ng -y
       RUN echo "Apache" >> /var/www/html/index.html
       ADD run-apache.sh /usr/share/httpd/run-apache.sh
       RUN chown apache. /run/httpd/ -R
       RUN chmod -v +rx /usr/share/httpd/run-apache.sh
       RUN chown apache.  /usr/share/httpd/run-apache.sh
       RUN usermod apache -s /bin/bash
       RUN sed -i 's/Listen 80/Listen 8080/' /etc/httpd/conf/httpd.conf
       RUN chown apache. /etc/httpd/logs/ -R
       USER apache
       CMD ["/usr/share/httpd/run-apache.sh"]
  strategy:
    dockerStrategy:
      noCache: true
  output:
    to:
      kind: ImageStreamTag
      name: fh:latest
~~~

And apply it:
~~~
[root@master-2 ~]# oc apply -f buildconfig.yaml 
imagestream.image.openshift.io/fh created
configmap/run-apache created
buildconfig.build.openshift.io/fh-build created
[root@master-2 ~]# oc get configmap
NAME         DATA      AGE
run-apache   1         5s
[root@master-2 ~]# oc get bc
ocNAME       TYPE      FROM         LATEST
fh-build   Docker    Dockerfile   0
[root@master-2 ~]# oc get is
NAME      DOCKER REPO                                   TAGS      UPDATED
fh        docker-registry.default.svc:5000/default/fh   
~~~

Now, start the build:
~~~
[root@master-2 ~]# oc start-build fh-build --follow
build.build.openshift.io/fh-build-1 started
Step 1/18 : FROM fedora
 ---> 536f3995adeb
Step 2/18 : EXPOSE 8080
 ---> Running in 4db37c2a6dff
 ---> f82730dc3baa
Removing intermediate container 4db37c2a6dff
Step 3/18 : RUN yum install httpd -y
 ---> Running in cabcfaad7fb7

Fedora Modular 31 - x86_64                      729 kB/s | 5.2 MB     00:07    
Fedora Modular 31 - x86_64 - Updates            733 kB/s | 4.0 MB     00:05    
Fedora 31 - x86_64 - Updates                    1.6 MB/s |  22 MB     00:13    
Fedora 31 - x86_64                              4.9 MB/s |  71 MB     00:14    
Last metadata expiration check: 0:00:03 ago on Tue Mar 10 21:35:02 2020.
Dependencies resolved.
================================================================================
 Package                  Architecture Version              Repository     Size
================================================================================
Installing:
 httpd                    x86_64       2.4.41-12.fc31       updates       1.4 M
Installing dependencies:
 apr                      x86_64       1.7.0-2.fc31         fedora        124 k
 apr-util                 x86_64       1.6.1-11.fc31        fedora         98 k
 fedora-logos-httpd       noarch       30.0.2-3.fc31        fedora         16 k
 httpd-filesystem         noarch       2.4.41-12.fc31       updates        15 k
 httpd-tools              x86_64       2.4.41-12.fc31       updates        84 k
 mailcap                  noarch       2.1.48-6.fc31        fedora         31 k
 mod_http2                x86_64       1.15.3-2.fc31        fedora        158 k
Installing weak dependencies:
 apr-util-bdb             x86_64       1.6.1-11.fc31        fedora         13 k
 apr-util-openssl         x86_64       1.6.1-11.fc31        fedora         16 k

Transaction Summary
================================================================================
Install  10 Packages

Total download size: 1.9 M
Installed size: 5.9 M
Downloading Packages:
(1/10): httpd-filesystem-2.4.41-12.fc31.noarch.  15 kB/s |  15 kB     00:01    
(2/10): httpd-tools-2.4.41-12.fc31.x86_64.rpm    57 kB/s |  84 kB     00:01    
(3/10): apr-util-1.6.1-11.fc31.x86_64.rpm        59 kB/s |  98 kB     00:01    
(4/10): apr-1.7.0-2.fc31.x86_64.rpm              57 kB/s | 124 kB     00:02    
(5/10): apr-util-bdb-1.6.1-11.fc31.x86_64.rpm    56 kB/s |  13 kB     00:00    
(6/10): apr-util-openssl-1.6.1-11.fc31.x86_64.r  66 kB/s |  16 kB     00:00    
(7/10): httpd-2.4.41-12.fc31.x86_64.rpm         401 kB/s | 1.4 MB     00:03    
(8/10): fedora-logos-httpd-30.0.2-3.fc31.noarch  68 kB/s |  16 kB     00:00    
(9/10): mailcap-2.1.48-6.fc31.noarch.rpm        131 kB/s |  31 kB     00:00    
(10/10): mod_http2-1.15.3-2.fc31.x86_64.rpm      96 kB/s | 158 kB     00:01    
--------------------------------------------------------------------------------
Total                                           312 kB/s | 1.9 MB     00:06     
Running transaction check
Transaction check succeeded.
Running transaction test
Transaction test succeeded.
Running transaction
  Preparing        :                                                        1/1 
  Installing       : apr-1.7.0-2.fc31.x86_64                               1/10 
  Installing       : apr-util-bdb-1.6.1-11.fc31.x86_64                     2/10 
  Installing       : apr-util-openssl-1.6.1-11.fc31.x86_64                 3/10 
  Installing       : apr-util-1.6.1-11.fc31.x86_64                         4/10 
  Installing       : httpd-tools-2.4.41-12.fc31.x86_64                     5/10 
  Installing       : mailcap-2.1.48-6.fc31.noarch                          6/10 
  Installing       : fedora-logos-httpd-30.0.2-3.fc31.noarch               7/10 
  Running scriptlet: httpd-filesystem-2.4.41-12.fc31.noarch                8/10 
  Installing       : httpd-filesystem-2.4.41-12.fc31.noarch                8/10 
  Installing       : mod_http2-1.15.3-2.fc31.x86_64                        9/10 
  Installing       : httpd-2.4.41-12.fc31.x86_64                          10/10 
  Running scriptlet: httpd-2.4.41-12.fc31.x86_64                          10/10 
  Verifying        : httpd-2.4.41-12.fc31.x86_64                           1/10 
  Verifying        : httpd-filesystem-2.4.41-12.fc31.noarch                2/10 
  Verifying        : httpd-tools-2.4.41-12.fc31.x86_64                     3/10 
  Verifying        : apr-1.7.0-2.fc31.x86_64                               4/10 
  Verifying        : apr-util-1.6.1-11.fc31.x86_64                         5/10 
  Verifying        : apr-util-bdb-1.6.1-11.fc31.x86_64                     6/10 
  Verifying        : apr-util-openssl-1.6.1-11.fc31.x86_64                 7/10 
  Verifying        : fedora-logos-httpd-30.0.2-3.fc31.noarch               8/10 
  Verifying        : mailcap-2.1.48-6.fc31.noarch                          9/10 
  Verifying        : mod_http2-1.15.3-2.fc31.x86_64                       10/10 

Installed:
  apr-1.7.0-2.fc31.x86_64                 apr-util-1.6.1-11.fc31.x86_64        
  apr-util-bdb-1.6.1-11.fc31.x86_64       apr-util-openssl-1.6.1-11.fc31.x86_64
  fedora-logos-httpd-30.0.2-3.fc31.noarch httpd-2.4.41-12.fc31.x86_64          
  httpd-filesystem-2.4.41-12.fc31.noarch  httpd-tools-2.4.41-12.fc31.x86_64    
  mailcap-2.1.48-6.fc31.noarch            mod_http2-1.15.3-2.fc31.x86_64       

Complete!
 ---> 88867ed9b8a4
Removing intermediate container cabcfaad7fb7
Step 4/18 : RUN yum install tcpdump -y
 ---> Running in 361782c1902a

Last metadata expiration check: 0:01:53 ago on Tue Mar 10 21:35:02 2020.
Dependencies resolved.
================================================================================
 Package         Architecture   Version                   Repository       Size
================================================================================
Installing:
 tcpdump         x86_64         14:4.9.3-1.fc31           updates         446 k

Transaction Summary
================================================================================
Install  1 Package

Total download size: 446 k
Installed size: 1.2 M
Downloading Packages:
tcpdump-4.9.3-1.fc31.x86_64.rpm                 217 kB/s | 446 kB     00:02    
--------------------------------------------------------------------------------
Total                                           144 kB/s | 446 kB     00:03     
Running transaction check
Transaction check succeeded.
Running transaction test
Transaction test succeeded.
Running transaction
  Preparing        :                                                        1/1 
  Running scriptlet: tcpdump-14:4.9.3-1.fc31.x86_64                         1/1 
  Installing       : tcpdump-14:4.9.3-1.fc31.x86_64                         1/1 
  Running scriptlet: tcpdump-14:4.9.3-1.fc31.x86_64                         1/1 
  Verifying        : tcpdump-14:4.9.3-1.fc31.x86_64                         1/1 

Installed:
  tcpdump-14:4.9.3-1.fc31.x86_64                                                

Complete!
 ---> 42d0e7726ddc
Removing intermediate container 361782c1902a
Step 5/18 : RUN yum install iproute -y
 ---> Running in 38fbda75678e

Last metadata expiration check: 0:02:42 ago on Tue Mar 10 21:35:02 2020.
Dependencies resolved.
================================================================================
 Package               Architecture  Version               Repository      Size
================================================================================
Installing:
 iproute               x86_64        5.4.0-1.fc31          updates        640 k
Installing dependencies:
 libmnl                x86_64        1.0.4-10.fc31         fedora          28 k
 linux-atm-libs        x86_64        2.5.1-25.fc31         fedora          38 k
 psmisc                x86_64        23.3-2.fc31           updates        160 k
Installing weak dependencies:
 iproute-tc            x86_64        5.4.0-1.fc31          updates        408 k

Transaction Summary
================================================================================
Install  5 Packages

Total download size: 1.2 M
Installed size: 3.4 M
Downloading Packages:
(1/5): psmisc-23.3-2.fc31.x86_64.rpm             96 kB/s | 160 kB     00:01    
(2/5): iproute-tc-5.4.0-1.fc31.x86_64.rpm       167 kB/s | 408 kB     00:02    
(3/5): iproute-5.4.0-1.fc31.x86_64.rpm          258 kB/s | 640 kB     00:02    
(4/5): libmnl-1.0.4-10.fc31.x86_64.rpm           19 kB/s |  28 kB     00:01    
(5/5): linux-atm-libs-2.5.1-25.fc31.x86_64.rpm   31 kB/s |  38 kB     00:01    
--------------------------------------------------------------------------------
Total                                           221 kB/s | 1.2 MB     00:05     
Running transaction check
Transaction check succeeded.
Running transaction test
Transaction test succeeded.
Running transaction
  Preparing        :                                                        1/1 
  Installing       : libmnl-1.0.4-10.fc31.x86_64                            1/5 
  Installing       : linux-atm-libs-2.5.1-25.fc31.x86_64                    2/5 
  Installing       : psmisc-23.3-2.fc31.x86_64                              3/5 
  Installing       : iproute-tc-5.4.0-1.fc31.x86_64                         4/5 
  Installing       : iproute-5.4.0-1.fc31.x86_64                            5/5 
  Running scriptlet: iproute-5.4.0-1.fc31.x86_64                            5/5 
  Verifying        : iproute-5.4.0-1.fc31.x86_64                            1/5 
  Verifying        : iproute-tc-5.4.0-1.fc31.x86_64                         2/5 
  Verifying        : psmisc-23.3-2.fc31.x86_64                              3/5 
  Verifying        : libmnl-1.0.4-10.fc31.x86_64                            4/5 
  Verifying        : linux-atm-libs-2.5.1-25.fc31.x86_64                    5/5 

Installed:
  iproute-5.4.0-1.fc31.x86_64        iproute-tc-5.4.0-1.fc31.x86_64            
  libmnl-1.0.4-10.fc31.x86_64        linux-atm-libs-2.5.1-25.fc31.x86_64       
  psmisc-23.3-2.fc31.x86_64         

Complete!
 ---> 21549d4c64c0
Removing intermediate container 38fbda75678e
Step 6/18 : RUN yum install procps-ng -y
 ---> Running in d435bba8f834

Last metadata expiration check: 0:03:29 ago on Tue Mar 10 21:35:02 2020.
Dependencies resolved.
================================================================================
 Package           Architecture   Version                  Repository      Size
================================================================================
Installing:
 procps-ng         x86_64         3.3.15-6.fc31            fedora         326 k

Transaction Summary
================================================================================
Install  1 Package

Total download size: 326 k
Installed size: 966 k
Downloading Packages:
procps-ng-3.3.15-6.fc31.x86_64.rpm              126 kB/s | 326 kB     00:02    
--------------------------------------------------------------------------------
Total                                            87 kB/s | 326 kB     00:03     
Running transaction check
Transaction check succeeded.
Running transaction test
Transaction test succeeded.
Running transaction
  Preparing        :                                                        1/1 
  Installing       : procps-ng-3.3.15-6.fc31.x86_64                         1/1 
  Running scriptlet: procps-ng-3.3.15-6.fc31.x86_64                         1/1 
  Verifying        : procps-ng-3.3.15-6.fc31.x86_64                         1/1 

Installed:
  procps-ng-3.3.15-6.fc31.x86_64                                                

Complete!
 ---> 951a4cb399c5
Removing intermediate container d435bba8f834
Step 7/18 : RUN echo "Apache" >> /var/www/html/index.html
 ---> Running in b6c65c93e5fa

 ---> 35e10a5d00de
Removing intermediate container b6c65c93e5fa
Step 8/18 : ADD run-apache.sh /usr/share/httpd/run-apache.sh
 ---> d1fef6e7b9ab
Removing intermediate container 408f20999a38
Step 9/18 : RUN chown apache. /run/httpd/ -R
 ---> Running in 607c708100d6

 ---> 0be64aed67dc
Removing intermediate container 607c708100d6
Step 10/18 : RUN chmod -v +rx /usr/share/httpd/run-apache.sh
 ---> Running in 0f5f0790daa8

mode of '/usr/share/httpd/run-apache.sh' changed from 0600 (rw-------) to 0755 (rwxr-xr-x)
 ---> 50e2716f2ff5
Removing intermediate container 0f5f0790daa8
Step 11/18 : RUN chown apache.  /usr/share/httpd/run-apache.sh
 ---> Running in 14bf2ae5b2db

 ---> 506b6f95e9d4
Removing intermediate container 14bf2ae5b2db
Step 12/18 : RUN usermod apache -s /bin/bash
 ---> Running in 4df46121305b

 ---> a6ddc2b4646b
Removing intermediate container 4df46121305b
Step 13/18 : RUN sed -i 's/Listen 80/Listen 8080/' /etc/httpd/conf/httpd.conf
 ---> Running in 91eff6b4a550

 ---> 8dd2adb40976
Removing intermediate container 91eff6b4a550
Step 14/18 : RUN chown apache. /etc/httpd/logs/ -R
 ---> Running in e08957abbebc

 ---> ad1c7829a76f
Removing intermediate container e08957abbebc
Step 15/18 : USER apache
 ---> Running in daacadbb8527
 ---> 19696cc0c7cc
Removing intermediate container daacadbb8527
Step 16/18 : CMD /usr/share/httpd/run-apache.sh
 ---> Running in 572378ff8b5f
 ---> 14f30ed483b8
Removing intermediate container 572378ff8b5f
Step 17/18 : ENV "OPENSHIFT_BUILD_NAME" "fh-build-1" "OPENSHIFT_BUILD_NAMESPACE" "default"
 ---> Running in 38160ae2c287
 ---> b9caed661e59
Removing intermediate container 38160ae2c287
Step 18/18 : LABEL "io.openshift.build.name" "fh-build-1" "io.openshift.build.namespace" "default"
 ---> Running in a23d9f647e89
 ---> f34898ea8473
Removing intermediate container a23d9f647e89
Successfully built f34898ea8473

Pushing image docker-registry.default.svc:5000/default/fh:latest ...
Pushed 0/13 layers, 15% complete
Pushed 1/13 layers, 31% complete
Pushed 2/13 layers, 31% complete
Pushed 3/13 layers, 31% complete
Pushed 4/13 layers, 31% complete
Pushed 5/13 layers, 38% complete
Pushed 6/13 layers, 48% complete
Pushed 7/13 layers, 64% complete
Pushed 8/13 layers, 71% complete
Pushed 9/13 layers, 80% complete
Pushed 10/13 layers, 79% complete
Pushed 11/13 layers, 86% complete
Pushed 12/13 layers, 92% complete
Pushed 13/13 layers, 100% complete
Push successful
~~~

Verify the build and imagestream:
~~~
[root@master-2 ~]# oc get is
oNAME      DOCKER REPO                                   TAGS      UPDATED
fh        docker-registry.default.svc:5000/default/fh   latest    About a minute ago
[root@master-2 ~]# oc get bc
oNAME       TYPE      FROM         LATEST
fh-build   Docker    Dockerfile   1
[root@master-2 ~]# oc get builds
NAME         TYPE      FROM         STATUS     STARTED         DURATION
fh-build-1   Docker    Dockerfile   Complete   7 minutes ago   5m55s
[root@master-2 ~]# oc describe is fh
Name:			fh
Namespace:		default
Created:		7 minutes ago
Labels:			<none>
Annotations:		kubectl.kubernetes.io/last-applied-configuration={"apiVersion":"image.openshift.io/v1","kind":"ImageStream","metadata":{"annotations":{},"creationTimestamp":null,"generation":1,"name":"fh","namespace":"default","selfLink":"/apis/image.openshift.io/v1/namespaces/default/imagestreams/fh"},"spec":{"lookupPolicy":{"local":false}},"status":{"dockerImageRepository":"docker-registry.default.svc:5000/fh"}}
			
Docker Pull Spec:	docker-registry.default.svc:5000/default/fh
Image Lookup:		local=false
Unique Images:		1
Tags:			1

latest
  no spec tag

  * docker-registry.default.svc:5000/default/fh@sha256:770559f9e958c6c1d0dd91f8ff64f10f2e1fc8538c337682b95f855f8a5a123e
      About a minute ago
~~~

### Using the image stream in a new-app ###

Now, use the imagestream in a deployment. Inspect this with a dry-run:
~~~
[root@master-2 ~]# oc new-app fh  --name fh  --dry-run -o yaml
apiVersion: v1
items:
- apiVersion: apps.openshift.io/v1
  kind: DeploymentConfig
  metadata:
    annotations:
      openshift.io/generated-by: OpenShiftNewApp
    creationTimestamp: null
    labels:
      app: fh
    name: fh
  spec:
    replicas: 1
    selector:
      app: fh
      deploymentconfig: fh
    strategy:
      resources: {}
    template:
      metadata:
        annotations:
          openshift.io/generated-by: OpenShiftNewApp
        creationTimestamp: null
        labels:
          app: fh
          deploymentconfig: fh
      spec:
        containers:
        - image: docker-registry.default.svc:5000/default/fh:latest
          name: fh
          ports:
          - containerPort: 80
            protocol: TCP
          resources: {}
    test: false
    triggers:
    - type: ConfigChange
    - imageChangeParams:
        automatic: true
        containerNames:
        - fh
        from:
          kind: ImageStreamTag
          name: fh:latest
          namespace: default
      type: ImageChange
  status:
    availableReplicas: 0
    latestVersion: 0
    observedGeneration: 0
    replicas: 0
    unavailableReplicas: 0
    updatedReplicas: 0
- apiVersion: v1
  kind: Service
  metadata:
    annotations:
      openshift.io/generated-by: OpenShiftNewApp
    creationTimestamp: null
    labels:
      app: fh
    name: fh
  spec:
    ports:
    - name: 80-tcp
      port: 80
      protocol: TCP
      targetPort: 80
    selector:
      app: fh
      deploymentconfig: fh
  status:
    loadBalancer: {}
kind: List
metadata: {}
~~~

Either use these resource manually or simply apply the app:
~~~
oc new-app fh  --name fh 
~~~

### Using the image stream in a custom deployment ###

With old school kubernetes deployments, this would look like this:
`fh.yaml`:
~~~
apiVersion: route.openshift.io/v1
kind: Route
metadata:
  labels:
    app: fh-deploymentconfig
  name: fh-service
spec:
  host: fh.apps.akaris.lab.pnq2.cee.redhat.com
  port:
    targetPort: 8080
  to:
    kind: Service
    name: fh-service
    weight: 100
  wildcardPolicy: None
---
apiVersion: v1
kind: Service
metadata:
  name: fh-service
  labels:
    app: fh-deploymentconfig
spec:
  selector:
    app: fh-pod
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
---
apiVersion: v1
kind: DeploymentConfig
metadata:
  name: fh-deploymentconfig
  labels:
    app: fh-deploymentconfig
spec:
  replicas: 1
  selector:
    app: fh-pod
  template:
    metadata:
      labels:
        app: fh-pod
    spec:
      containers:
      - name: fh
        image: docker-registry.default.svc:5000/default/fh:latest
        imagePullPolicy: Always
~~~

~~~
oc apply -f fh.yaml
~~~

Verification:
~~~
sleep 60
[root@master-2 ~]# curl http://fh.apps.akaris.lab.pnq2.cee.redhat.com/
Apache
~~~


### Resources ###

* https://docs.openshift.com/container-platform/3.11/dev_guide/builds/build_inputs.html#dockerfile-source
* https://docs.openshift.com/container-platform/3.11/dev_guide/builds/basic_build_operations.html
* https://lists.openshift.redhat.com/openshift-archives/users/2017-September/msg00031.html
* https://docs.openshift.com/container-platform/3.11/dev_guide/builds/basic_build_operations.html
* https://kb.novaordis.com/index.php/OpenShift_Image_and_ImageStream_Operations
* https://docs.openshift.com/dedicated/3/dev_guide/builds/build_inputs.html#using-secrets-during-build
* https://cloudowski.com/articles/why-managing-container-images-on-openshift-is-better-than-on-kubernetes/
* https://access.redhat.com/solutions/3635491
* https://blog.openshift.com/jupyter-on-openshift-part-6-running-as-an-assigned-user-id/
* https://blog.openshift.com/jupyter-openshift-part-2-using-jupyter-project-images/
