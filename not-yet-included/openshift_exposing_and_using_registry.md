### Disclaimer ###

This is just a quick and dirty way of using the internal registry within OpenShift 4.2. Not suitable for production environments.

### Documentation ###

* [https://docs.openshift.com/container-platform/4.2/registry/securing-exposing-registry.html](https://docs.openshift.com/container-platform/4.2/registry/securing-exposing-registry.html)
* [https://docs.openshift.com/container-platform/4.2/registry/accessing-the-registry.html](https://docs.openshift.com/container-platform/4.2/registry/accessing-the-registry.html)

### Accessing the registry ###

Exposing with default routes:
~~~
oc patch configs.imageregistry.operator.openshift.io/cluster --patch '{"spec":{"defaultRoute":true}}' --type=merge
HOST=$(oc get route default-route -n openshift-image-registry --template='{{ .spec.host }}')
~~~

If in a lab environment and not using DNS servers, modify `/etc/hosts` on the client and push:
~~~
x.x.x.x default-route-openshift-image-registry.apps.<cluster URL>
~~~

First, login to OpenShift:
~~~
oc login -u kubeadmin -p <password from install log>
~~~

When using the kubeadmin user, login as follows:
~~~
podman login -u kubeadmin -p $(oc whoami -t) --tls-verify=false $HOST 
~~~

Otherwise:
~~~
podman login -u $(oc whoami) -p $(oc whoami -t) --tls-verify=false $HOST 
~~~

### Pushing images to the registry ###

Build a custom image, e.g.:
~~~
mkdir custom-image
cd custom-image
cat<<'EOF'>Dockerfile
FROM fedora
RUN yum install tcpdump iproute iputils -y
EOF
buildah bud -t fedora-custom:1.0 .
~~~

Tag the image correctly and push to get registry:
~~~
podman tag localhost/fedora-custom:1.0 $HOST/openshift/fedora-custom:1.0
podman push  --tls-verify=false $HOST/openshift/fedora-custom:1.0
~~~

### Viewing logs ###

Verify registry logs with:
~~~
oc logs -n openshift-image-registry deployments/image-registry | grep fedora-custom
~~~

### Launching a deployment with the custom image ###

Launch a custom deployment from the custom image. Note that the internal registry name must be used:
~~~
cat<<'EOF'>fedora-custom-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: fedora-custom-deployment
  labels:
    app: fedora-custom-deployment
spec:
  replicas: 3
  selector:
    matchLabels:
      app: fedora-custom-pod
  template:
    metadata:
      labels:
        app: fedora-custom-pod
    spec:
      containers:
      - name: fedora-custom
        image: image-registry.openshift-image-registry.svc:5000/openshift/fedora-custom:1.0
        command:
          - sleep
          - infinity
        imagePullPolicy: IfNotPresent
EOF
oc apply -f fedora-custom-deployment.yaml
~~~
