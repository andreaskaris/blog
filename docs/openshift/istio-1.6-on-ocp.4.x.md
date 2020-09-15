# Istio 1.6 on OpenShfit 4.x #

How to install upstream istio 1.6 on OCP 4.x on AWS

## Resources ##

Instructions for upstream Istio with OpenShift:
[https://istio.io/latest/docs/setup/platform-setup/openshift/](https://istio.io/latest/docs/setup/platform-setup/openshift/)

I then installed istioctl:
[https://istio.io/latest/docs/ops/diagnostic-tools/istioctl/](https://istio.io/latest/docs/ops/diagnostic-tools/istioctl/)

And then *DO NOT* follow:
[https://istio.io/latest/docs/setup/install/standalone-operator/](https://istio.io/latest/docs/setup/install/standalone-operator/)

The standalone operator configuration does not work, so instead follow:
[https://istio.io/latest/docs/setup/additional-setup/cni/](https://istio.io/latest/docs/setup/additional-setup/cni/)

But be aware of:
[https://istio.io/latest/docs/setup/additional-setup/cni/#hosted-kubernetes-settings](https://istio.io/latest/docs/setup/additional-setup/cni/#hosted-kubernetes-settings)

Install the bookinfo application:
[https://istio.io/latest/docs/examples/bookinfo/](https://istio.io/latest/docs/examples/bookinfo/)

## Installation instructions ##

Make sure the cluster was correctly installed:
~~~
[akaris@linux upi]$ oc get clusterversion
NAME      VERSION   AVAILABLE   PROGRESSING   SINCE   STATUS
version   4.4.16    True        False         2d1h    Cluster version is 4.4.16
~~~

~~~
[akaris@linux upi]$ oc new-project istio-system
Now using project "istio-system" on server "https://api.akaris-upi.focused-solutions.support:6443".

You can add applications to this project with the 'new-app' command. For example, try:

    oc new-app django-psql-example

to build a new example application in Python. Or use kubectl to deploy a simple Kubernetes application:

    kubectl create deployment hello-node --image=gcr.io/hello-minikube-zero-install/hello-node
~~~

~~~
[akaris@linux upi]$ oc adm policy add-scc-to-group anyuid system:serviceaccounts:istio-system
securitycontextconstraints.security.openshift.io/anyuid added to groups: ["system:serviceaccounts:istio-system"]
~~~

Install istiocli:
~~~
[akaris@linux upi]$  curl -sL https://istio.io/downloadIstioctl | sh -
Downloading istioctl-1.6.8 from https://github.com/istio/istio/releases/download/1.6.8/istioctl-1.6.8-linux.tar.gz ... 
Failed. 

Trying with TARGET_ARCH. Downloading istioctl-1.6.8 from https://github.com/istio/istio/releases/download/1.6.8/istioctl-1.6.8-linux-amd64.tar.gz ...
istioctl-1.6.8-linux-amd64.tar.gz download complete!

Add the istioctl to your path with:
  export PATH=$PATH:$HOME/.istioctl/bin 

Begin the Istio pre-installation verification check by running:
	 istioctl verify-install 

Need more information? Visit https://istio.io/docs/reference/commands/istioctl/ 
[akaris@linux upi]$ export PATH=$PATH:$HOME/.istioctl/bin^C
[akaris@linux upi]$ export PATH=$PATH:$HOME/.istioctl/bin
[akaris@linux upi]$ istioctl verify-install
Error: could not load IstioOperator from cluster: the server could not find the requested resource.  Use --filename 
~~~

The above error is normal. Nothing was installed yet.

Follow [https://istio.io/latest/docs/setup/additional-setup/cni/](https://istio.io/latest/docs/setup/additional-setup/cni/):
~~~
$ cat <<EOF > istio-cni.yaml
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
spec:
  components:
    cni:
      enabled: true
  values:
    cni:
      excludeNamespaces:
       - istio-system
       - kube-system
      logLevel: info
EOF
~~~

For OpenShift, then follow: [https://istio.io/latest/docs/setup/additional-setup/cni/#hosted-kubernetes-settings](https://istio.io/latest/docs/setup/additional-setup/cni/#hosted-kubernetes-settings):

Therefore, run:
~~~
[akaris@linux upi]$ istioctl install -f istio-cni.yaml --set components.cni.namespace=kube-system --set values.cni.cniBinDir=/var/lib/cni/bin --set values.cni.cniConfDir=/etc/cni/multus/net.d --set values.cni.chained=false --set values.cni.cniConfFileName="istio-cni.conf" --set values.sidecarInjectorWebhook.injectedAnnotations."k8s\.v1\.cni\.cncf\.io/networks"=istio-cni
✔ Istio core installed                                                                                                
✔ Istiod installed                                                                                                    
✔ Addons installed                                                                                                    
✔ Ingress gateways installed                                                                                          
✔ CNI installed                                                                                                       
- Pruning removed resources                                                                                             Pruned object DaemonSet:istio-system:istio-cni-node.
  Pruned object ConfigMap:istio-system:istio-cni-config.
  Pruned object ServiceAccount:istio-system:istio-cni.
✔ Installation complete  
~~~

Verify the installation:
~~~
[akaris@linux upi]$ istioctl verify-install 
ClusterRole: istio-cni.default checked successfully
ClusterRole: istio-cni-repair-role.default checked successfully
ClusterRoleBinding: istio-cni.default checked successfully
ClusterRoleBinding: istio-cni-repair-rolebinding.default checked successfully
ConfigMap: istio-cni-config.kube-system checked successfully
DaemonSet: istio-cni-node.kube-system checked successfully
ServiceAccount: istio-cni.kube-system checked successfully
HorizontalPodAutoscaler: istio-ingressgateway.istio-system checked successfully
Deployment: istio-ingressgateway.istio-system checked successfully
PodDisruptionBudget: istio-ingressgateway.istio-system checked successfully
Role: istio-ingressgateway-sds.istio-system checked successfully
RoleBinding: istio-ingressgateway-sds.istio-system checked successfully
Service: istio-ingressgateway.istio-system checked successfully
ServiceAccount: istio-ingressgateway-service-account.istio-system checked successfully
ClusterRole: prometheus-istio-system.default checked successfully
ClusterRoleBinding: prometheus-istio-system.default checked successfully
ConfigMap: prometheus.istio-system checked successfully
Deployment: prometheus.istio-system checked successfully
Service: prometheus.istio-system checked successfully
ServiceAccount: prometheus.istio-system checked successfully
HorizontalPodAutoscaler: istiod.istio-system checked successfully
ConfigMap: istio.istio-system checked successfully
Deployment: istiod.istio-system checked successfully
ConfigMap: istio-sidecar-injector.istio-system checked successfully
MutatingWebhookConfiguration: istio-sidecar-injector.default checked successfully
PodDisruptionBudget: istiod.istio-system checked successfully
Service: istiod.istio-system checked successfully
EnvoyFilter: metadata-exchange-1.4.istio-system checked successfully
EnvoyFilter: stats-filter-1.4.istio-system checked successfully
EnvoyFilter: metadata-exchange-1.5.istio-system checked successfully
EnvoyFilter: tcp-metadata-exchange-1.5.istio-system checked successfully
EnvoyFilter: stats-filter-1.5.istio-system checked successfully
EnvoyFilter: tcp-stats-filter-1.5.istio-system checked successfully
EnvoyFilter: metadata-exchange-1.6.istio-system checked successfully
EnvoyFilter: tcp-metadata-exchange-1.6.istio-system checked successfully
EnvoyFilter: stats-filter-1.6.istio-system checked successfully
EnvoyFilter: tcp-stats-filter-1.6.istio-system checked successfully
ClusterRole: istiod-istio-system.default checked successfully
ClusterRole: istio-reader-istio-system.default checked successfully
ClusterRoleBinding: istio-reader-istio-system.default checked successfully
ClusterRoleBinding: istiod-pilot-istio-system.default checked successfully
ServiceAccount: istio-reader-service-account.istio-system checked successfully
ServiceAccount: istiod-service-account.istio-system checked successfully
ValidatingWebhookConfiguration: istiod-istio-system.default checked successfully
CustomResourceDefinition: httpapispecs.config.istio.io.default checked successfully
CustomResourceDefinition: httpapispecbindings.config.istio.io.default checked successfully
CustomResourceDefinition: quotaspecs.config.istio.io.default checked successfully
CustomResourceDefinition: quotaspecbindings.config.istio.io.default checked successfully
CustomResourceDefinition: destinationrules.networking.istio.io.default checked successfully
CustomResourceDefinition: envoyfilters.networking.istio.io.default checked successfully
CustomResourceDefinition: gateways.networking.istio.io.default checked successfully
CustomResourceDefinition: serviceentries.networking.istio.io.default checked successfully
CustomResourceDefinition: sidecars.networking.istio.io.default checked successfully
CustomResourceDefinition: virtualservices.networking.istio.io.default checked successfully
CustomResourceDefinition: workloadentries.networking.istio.io.default checked successfully
CustomResourceDefinition: attributemanifests.config.istio.io.default checked successfully
CustomResourceDefinition: handlers.config.istio.io.default checked successfully
CustomResourceDefinition: instances.config.istio.io.default checked successfully
CustomResourceDefinition: rules.config.istio.io.default checked successfully
CustomResourceDefinition: clusterrbacconfigs.rbac.istio.io.default checked successfully
CustomResourceDefinition: rbacconfigs.rbac.istio.io.default checked successfully
CustomResourceDefinition: serviceroles.rbac.istio.io.default checked successfully
CustomResourceDefinition: servicerolebindings.rbac.istio.io.default checked successfully
CustomResourceDefinition: authorizationpolicies.security.istio.io.default checked successfully
CustomResourceDefinition: peerauthentications.security.istio.io.default checked successfully
CustomResourceDefinition: requestauthentications.security.istio.io.default checked successfully
CustomResourceDefinition: adapters.config.istio.io.default checked successfully
CustomResourceDefinition: templates.config.istio.io.default checked successfully
CustomResourceDefinition: istiooperators.install.istio.io.default checked successfully
Checked 25 custom resource definitions
Checked 1 Istio Deployments
Istio is installed successfully
[akaris@linux upi]$ 
~~~

That will create the istio pods that are needed for the CNI plugin:
~~~
[akaris@linux upi]$ oc get pods -A | grep istio
istio-system                                            istio-ingressgateway-6c77d7f498-d58gx                                1/1     Running     0          33m
istio-system                                            istiod-58f84ffddc-r2bj9                                              1/1     Running     0          33m
istio-system                                            prometheus-5db67458fb-5m67n                                          2/2     Running     0          33m
kube-system                                             istio-cni-node-52ktn                                                 2/2     Running     0          56s
kube-system                                             istio-cni-node-794mr                                                 2/2     Running     0          56s
kube-system                                             istio-cni-node-gggvm                                                 2/2     Running     0          56s
kube-system                                             istio-cni-node-nd72d                                                 2/2     Running     0          56s
kube-system                                             istio-cni-node-p2w5f                                                 2/2     Running     0          56s
kube-system                                             istio-cni-node-vqjkb                                                 2/2     Running     0          56s
~~~
 
Create the bookinfo app according to [https://istio.io/latest/docs/examples/bookinfo/](https://istio.io/latest/docs/examples/bookinfo/):
~~~
[akaris@linux upi]$ oc new-project bookinfo
Now using project "bookinfo" on server "https://api.akaris-upi.focused-solutions.support:6443".

You can add applications to this project with the 'new-app' command. For example, try:

    oc new-app django-psql-example

to build a new example application in Python. Or use kubectl to deploy a simple Kubernetes application:

    kubectl create deployment hello-node --image=gcr.io/hello-minikube-zero-install/hello-node
~~~

Adjust SCCs according to [https://istio.io/latest/docs/setup/platform-setup/openshift/#privileged-security-context-constraints-for-application-sidecars](https://istio.io/latest/docs/setup/platform-setup/openshift/#privileged-security-context-constraints-for-application-sidecars):
~~~
The Istio sidecar injected into each application pod runs with user ID 1337, 
which is not allowed by default in OpenShift. To allow this user ID to be used, 
execute the following commands. Replace <target-namespace> with the appropriate namespace.
~~~

So execute:
~~~
[akaris@linux upi]$ oc adm policy add-scc-to-group privileged system:serviceaccounts:bookinfo
securitycontextconstraints.security.openshift.io/privileged added to groups: ["system:serviceaccounts:bookinfo"]
[akaris@linux upi]$ oc adm policy add-scc-to-group anyuid system:serviceaccounts:bookinfo
securitycontextconstraints.security.openshift.io/anyuid added to groups: ["system:serviceaccounts:bookinfo"]
~~~

Verify SCC configuration - in SCC 4.5, check `oc get clusterrolebindings | grep scc` instead.
~~~
[akaris@linux upi]$ oc get scc anyuid -o yaml | grep serv -C3
  type: RunAsAny
groups:
- system:cluster-admins
- system:serviceaccounts:istio-system
kind: SecurityContextConstraints
metadata:
  annotations:
~~~

Now, create the application:
~~~
oc apply -f https://raw.githubusercontent.com/istio/istio/release-1.6/samples/bookinfo/platform/kube/bookinfo.yaml
~~~

~~~
[akaris@linux upi]$ oc get pods
NAME                              READY   STATUS    RESTARTS   AGE
details-v1-5974b67c8-nd7kl        2/2     Running   0          83s
productpage-v1-64794f5db4-gl7c9   2/2     Running   0          83s
ratings-v1-c6cdf8d98-z987h        2/2     Running   0          83s
reviews-v1-7f6558b974-6z84q       2/2     Running   0          83s
reviews-v2-6cb6ccd848-l8s56       2/2     Running   0          83s
reviews-v3-cc56b578-9vvqv         2/2     Running   0          83s
~~~

Now, create the gateway:
~~~
[akaris@linux upi]$ oc apply -f https://raw.githubusercontent.com/istio/istio/release-1.6/samples/bookinfo/networking/bookinfo-gateway.yaml
gateway.networking.istio.io/bookinfo-gateway created
~~~

~~~
[akaris@linux upi]$ oc  get svc istio-ingressgateway -n istio-system
NAME                   TYPE           CLUSTER-IP     EXTERNAL-IP                                                               PORT(S)                                                      AGE
istio-ingressgateway   LoadBalancer   172.30.44.45   af0e3e39e3f544f55a59da6578212ff5-1911792852.eu-west-3.elb.amazonaws.com   15021:31538/TCP,80:31367/TCP,443:31276/TCP,15443:30398/TCP   47m
~~~

Get variables to connect to the gateway:
[https://istio.io/latest/docs/tasks/traffic-management/ingress/ingress-control/#determining-the-ingress-ip-and-ports](https://istio.io/latest/docs/tasks/traffic-management/ingress/ingress-control/#determining-the-ingress-ip-and-ports)

There's an issue in the instructions for the `INGRESS_HOST`. With AWS, it needs to be `.status.loadBalancer.ingress[0].hostname` and not `.status.loadBalancer.ingress[0].ip`:
~~~
[akaris@linux upi]$ export INGRESS_HOST=$(kubectl -n istio-system get service istio-ingressgateway -o jsonpath='{.status.loadBalancer.ingress[0].hostname}')
[akaris@linux upi]$ export INGRESS_PORT=$(kubectl -n istio-system get service istio-ingressgateway -o jsonpath='{.spec.ports[?(@.name=="http2")].port}')
[akaris@linux upi]$ export SECURE_INGRESS_PORT=$(kubectl -n istio-system get service istio-ingressgateway -o jsonpath='{.spec.ports[?(@.name=="https")].port}')
[akaris@linux upi]$ export TCP_INGRESS_PORT=$(kubectl -n istio-system get service istio-ingressgateway -o jsonpath='{.spec.ports[?(@.name=="tcp")].port}')
[akaris@linux upi]$ export GATEWAY_URL=$INGRESS_HOST:$INGRESS_PORT
~~~

Now, connect to the application with curl:
~~~
[akaris@linux upi]$ curl -s "http://${GATEWAY_URL}/productpage" | grep -o "<title>.*</title>"
<title>Simple Bookstore App</title>
~~~
