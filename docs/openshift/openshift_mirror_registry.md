# OpenShfit mirror registry setup

In the following, we will run a container registry without hostname (only IP address) and self-signed CA and certificate. We need a virtual machine that serves as the container registry and a host that serves as the OpenShift install server. These can both be on the same machine, to simplify things further.

All steps below clarify and should give an easy walkthrough of the OpenShift restricted network installation guide. 

Follow the OpenShift documentation. The documentation isn't very detailed (at least at the moment), so I'm going through the documentation with an example deployment.

* [https://docs.openshift.com/container-platform/4.5/installing/install_config/installing-restricted-networks-preparations.html](https://docs.openshift.com/container-platform/4.5/installing/install_config/installing-restricted-networks-preparations.html)

## Setting up a private the registry server 

After creating a virtual machine that will serve as a registry, install podman and httpd-tools:
~~~
[root@mirror ~]# sudo yum -y install podman httpd-tools
~~~

Now, generate a self-signed CA:
~~~
[root@mirror ~]# mkdir CA
[root@mirror ~]# cd CA
[root@mirror CA]# openssl genrsa -des3 -out rootCA.key 4096
Generating RSA private key, 4096 bit long modulus
.++
....................................................++
e is 65537 (0x10001)
Enter pass phrase for rootCA.key:
Verifying - Enter pass phrase for rootCA.key:
[root@mirror CA]# ll
total 4
-rw-r--r--. 1 root root 3311 Feb  3 09:45 rootCA.key
[root@mirror CA]# openssl req -x509 -new -nodes -key rootCA.key -sha256 -days 10240 -out rootCA.crt
Enter pass phrase for rootCA.key:
You are about to be asked to enter information that will be incorporated
into your certificate request.
What you are about to enter is what is called a Distinguished Name or a DN.
There are quite a few fields but you can leave some blank
For some fields there will be a default value,
If you enter '.', the field will be left blank.
-----
Country Name (2 letter code) [XX]:DE
State or Province Name (full name) []:NRW
Locality Name (eg, city) [Default City]:Acme.Inc
Organization Name (eg, company) [Default Company Ltd]:
Organizational Unit Name (eg, section) []:acme.root
Common Name (eg, your name or your server's hostname) []:
Email Address []:
[root@mirror CA]# ll
total 8
-rw-r--r--. 1 root root 1960 Feb  3 09:47 rootCA.crt
-rw-r--r--. 1 root root 3311 Feb  3 09:45 rootCA.key
~~~

Trust this rootCA on the mirror registry node and on the node where you are running the OpenShift installer:
~~~
sudo cp rootCA.crt  /etc/pki/ca-trust/source/anchors/
sudo update-ca-trust extract
~~~

Create a certificate signing request:
~~~
mkdir certificates
cd certificates
cat<<'EOF'>config 
[ req ]
distinguished_name = req_distinguished_name
prompt = no
req_extensions = v3_req

[ req_distinguished_name ]
C="DE"
ST="NRW"
L="Dusseldorf"
O="Acme Inc."
CN="192.0.2.100"
emailAddress="akaris@example.com"

[ v3_req ]

#basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = 192.0.2.100
IP.1 = 192.0.2.100
DNS.2 = 192.0.3.100
IP.2 = 192.0.3.100
EOF
openssl genrsa -out domain.key 4096
openssl req -new -key domain.key -nodes -out domain.csr -config config
~~~

Verify the CSR:
~~~
[root@mirror certificates]#  openssl req -in domain.csr -noout -text | grep -i dns
                DNS:192.0.2.100, IP Address:192.0.2.100, DNS:192.0.3.100, IP Address:192.0.3.100
~~~

Sign your certificate with the rootCA key and force the SAN entries:
~~~
[root@mirror certificates]# openssl x509 -req -in domain.csr -CA ../rootCA.crt -CAkey ../rootCA.key -CAcreateserial -out domain.crt -days 3650 -sha256 -extensions v3_req -extfile config 
Signature ok
subject=C = DE, ST = NRW, L = Dusseldorf, O = Acme Inc., CN = 192.0.2.100, emailAddress = akaris@example.com
Getting CA Private Key
Enter pass phrase for ../rootCA.key:
~~~
See: 
* [https://github.com/openssl/openssl/issues/6481](https://github.com/openssl/openssl/issues/6481)
* [https://security.stackexchange.com/questions/150078/missing-x509-extensions-with-an-openssl-generated-certificate](https://security.stackexchange.com/questions/150078/missing-x509-extensions-with-an-openssl-generated-certificate)

Then, trust the certificate and make sure that it passes verification and that it contains the SAN entries:
~~~
[root@mirror certificates]# openssl x509 -in domain.crt -noout -text | grep IP
                DNS:192.0.2.100, IP Address:192.0.2.100, DNS:192.0.3.100, IP Address:192.0.3.100
[root@mirror certificates]# openssl verify -verbose domain.crt
domain.crt: OK
~~~

In case of issues with certificate verification, verify the cert directly against the root CA file to determine the failure domain:
~~~
[root@mirror certificates]# openssl verify -CAfile ../rootCA.crt domain.crt 
domain.crt: OK
~~~

Generate directories for the registry:
~~~
sudo mkdir -p /opt/registry/{auth,certs,data}
~~~

Copy the certificate into /opt/registry/certs as described in the documentation:
~~~
sudo cp domain.key  /opt/registry/certs/
sudo cp domain.crt  /opt/registry/certs/
~~~

Generate a username and password and store in an htpasswd file for authentication with the registry, e.g. `root` / `password`:
~~~
sudo htpasswd -bBc /opt/registry/auth/htpasswd root password
~~~

Run the containter with:
~~~
sudo podman run --name mirror-registry \
  -p 5000:5000 -v /opt/registry/data:/var/lib/registry:z      \
  -v /opt/registry/auth:/auth:z      -e "REGISTRY_AUTH=htpasswd"      \
  -e "REGISTRY_AUTH_HTPASSWD_REALM=Registry Realm"      \
  -e REGISTRY_AUTH_HTPASSWD_PATH=/auth/htpasswd      \
  -v /opt/registry/certs:/certs:z      \
  -e REGISTRY_HTTP_TLS_CERTIFICATE=/certs/domain.crt      \
  -e REGISTRY_HTTP_TLS_KEY=/certs/domain.key      \
  -d docker.io/library/registry:2
~~~

Open the firewall with iptables or firewall-cmd as describe in the documentation. In my case, I'm not running a firewall so this step is not needed.

Then, verify:
~~~
[root@mirror ~]# openssl s_client -connect 192.0.2.100:5000
CONNECTED(00000003)
Can't use SSL_get_servername
depth=1 C = DE, ST = NRW, L = Acme.Inc, O = Default Company Ltd, OU = acme.root
verify return:1
depth=0 C = DE, ST = NRW, L = Dusseldorf, O = Acme Inc., CN = 192.0.2.100, emailAddress = akaris@example.com
verify return:1
---
Certificate chain
 0 s:C = DE, ST = NRW, L = Dusseldorf, O = Acme Inc., CN = 192.0.2.100, emailAddress = akaris@example.com
   i:C = DE, ST = NRW, L = Acme.Inc, O = Default Company Ltd, OU = acme.root
---
(...)
Verification: OK
(...)
~~~

And make sure that curl does not report an issue:
~~~
[root@mirror certificates]# curl -u root:password https://192.0.2.100:5000/v2/_catalog
{"repositories":[]}
~~~

If using a separate installation server, make sure that the same verification works from there.

## Mirroring OpenShift container images into private registry and preparing openshift-install

Connect to the installation server and follow steps from the installation guide:
[https://docs.openshift.com/container-platform/4.5/installing/install_config/installing-restricted-networks-preparations.html](https://docs.openshift.com/container-platform/4.5/installing/install_config/installing-restricted-networks-preparations.html)

### Merging the pull secrets 

Download your pull-secret from [https://cloud.redhat.com/openshift/install/pull-secret](https://cloud.redhat.com/openshift/install/pull-secret).

At time of this writing, a bug makes this procedure a bit more clumsy: [https://bugzilla.redhat.com/show_bug.cgi?id=1866588](https://bugzilla.redhat.com/show_bug.cgi?id=1866588)

~~~
$ oc registry login --to ./pull-secret.json --registry "192.0.2.100:5000" --auth-basic=root:password
error: Missing or incomplete configuration info.  Please point to an existing, complete config file:


  1. Via the command-line flag --kubeconfig
  2. Via the KUBECONFIG environment variable
  3. In your home directory as ~/.kube/config

To view or setup config directly use the 'config' command.
~~~

Instead, either follow:
[https://docs.openshift.com/container-platform/4.2/installing/install_config/installing-restricted-networks-preparations.html#installation-adding-registry-pull-secret_installing-restricted-networks-preparations](https://docs.openshift.com/container-platform/4.2/installing/install_config/installing-restricted-networks-preparations.html#installation-adding-registry-pull-secret_installing-restricted-networks-preparations)

Or, more elegantly, create a pull secret with podman:
~~~
$ podman login -u root -p password --authfile pull-secret-podman.json https://192.0.2.100:5000
WARNING! Using --password via the cli is insecure. Please consider using --password-stdin
Login Succeeded!
$ cat pull-secret-podman.json
{
	"auths": {
		"192.0.2.100:5000": {
			"auth": "cm9vdDpwYXNzd29yZA=="
		}
	}
~~~

Install jq
~~~
sudo yum install jq -y
~~~

Then, merge both secrets:
~~~
jq -c --argjson var "$(jq .auths pull-secret-podman.json)" '.auths += $var' pull-secret.json > pull-secret-merged.json
~~~

### Mirroring the container images

Get `$OCP_RELEASE` from [https://quay.io/repository/openshift-release-dev/ocp-release?tag=latest&tab=tags](https://quay.io/repository/openshift-release-dev/ocp-release?tag=latest&tab=tags) and export required env variables, e.g.:
~~~
[root@mirror ~]#  env | egrep 'OCP|LOCAL|PRODUCT|RELEASE'
export RELEASE_NAME=ocp-release
export LOCAL_SECRET_JSON=pull-secret-merged.json
export OCP_RELEASE=4.5.9
export PRODUCT_REPO=openshift-release-dev
export LOCAL_REPOSITORY=ocp4/openshift4
export LOCAL_REGISTRY=192.0.2.100:5000
export ARCHITECTURE=x86_64
export REMOVABLE_MEDIA_PATH=removable_media
~~~

If you want to simulate the removable media steps, create a directory to simulate removable media and export the variable:
~~~
export REMOVABLE_MEDIA_PATH=removable_media
mkdir removable_media
~~~

Now, continue with the rest of the instructions from [https://docs.openshift.com/container-platform/4.5/installing/install_config/installing-restricted-networks-preparations.html#installation-mirror-repository_installing-restricted-networks-preparations](https://docs.openshift.com/container-platform/4.5/installing/install_config/installing-restricted-networks-preparations.html#installation-mirror-repository_installing-restricted-networks-preparations)

For example:
~~~
$ oc adm -a ${LOCAL_SECRET_JSON} release mirror      --from=quay.io/${PRODUCT_REPO}/${RELEASE_NAME}:${OCP_RELEASE}-${ARCHITECTURE}      --to=${LOCAL_REGISTRY}/${LOCAL_REPOSITORY}      --to-release-image=${LOCAL_REGISTRY}/${LOCAL_REPOSITORY}:${OCP_RELEASE}-${ARCHITECTURE}
~~~

After mirroring the images, the following instructions will be presented:
~~~
(...)
Success
Update image:  192.0.2.100:5000/ocp4/openshift4:4.5.9-x86_64
Mirror prefix: 192.0.2.100:5000/ocp4/openshift4

To use the new mirrored repository to install, add the following section to the install-config.yaml:

imageContentSources:
- mirrors:
  - 192.0.2.100:5000/ocp4/openshift4
  source: quay.io/openshift-release-dev/ocp-release
- mirrors:
  - 192.0.2.100:5000/ocp4/openshift4
  source: quay.io/openshift-release-dev/ocp-v4.0-art-dev


To use the new mirrored repository for upgrades, use the following to create an ImageContentSourcePolicy:

apiVersion: operator.openshift.io/v1alpha1
kind: ImageContentSourcePolicy
metadata:
  name: example
spec:
  repositoryDigestMirrors:
  - mirrors:
    - 192.0.2.100:5000/ocp4/openshift4
    source: quay.io/openshift-release-dev/ocp-release
  - mirrors:
    - 192.0.2.100:5000/ocp4/openshift4
    source: quay.io/openshift-release-dev/ocp-v4.0-art-dev
~~~

Make sure to save these instructions for later.

Verify contents of the local registry:
~~~
$ curl -u root:password https://192.0.2.100:5000/v2/_catalog | jq
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100    37  100    37    0     0   1121      0 --:--:-- --:--:-- --:--:--  1121
{
  "repositories": [
    "ocp4/openshift4"
  ]
}
$ curl -u root:password https://192.0.2.100:5000/v2/ocp4/openshift4/tags/list | jq
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100  3241  100  3241    0     0   102k      0 --:--:-- --:--:-- --:--:--  102k
{
  "name": "ocp4/openshift4",
  "tags": [
    "4.5.9-kube-storage-version-migrator",
    "4.5.9-csi-snapshot-controller",
    "4.5.9-cluster-kube-apiserver-operator",
    "4.5.9-grafana",
    "4.5.9-cluster-update-keys",
    "4.5.9-hyperkube",
    "4.5.9-container-networking-plugins",
    "4.5.9-etcd",
    "4.5.9-baremetal-operator",
    "4.5.9-jenkins-agent-nodejs",
    "4.5.9-cluster-monitoring-operator",
    "4.5.9-installer",
    "4.5.9-aws-machine-controllers",
    "4.5.9-kube-etcd-signer-server",
    "4.5.9-kuryr-controller",
    "4.5.9-oauth-server",
    "4.5.9-cluster-authentication-operator",
    "4.5.9-operator-lifecycle-manager",
    "4.5.9-haproxy-router",
    "4.5.9-cluster-node-tuning-operator",
    "4.5.9-openshift-state-metrics",
    "4.5.9-prom-label-proxy",
    "4.5.9-cluster-kube-scheduler-operator",
    "4.5.9-machine-os-content",
    "4.5.9-cli-artifacts",
    "4.5.9-cluster-kube-controller-manager-operator",
    "4.5.9-libvirt-machine-controllers",
    "4.5.9-console-operator",
    "4.5.9-prometheus-operator",
    "4.5.9-openshift-apiserver",
    "4.5.9-x86_64",
    "4.5.9-console",
    "4.5.9-operator-marketplace",
    "4.5.9-cluster-etcd-operator",
    "4.5.9-k8s-prometheus-adapter",
    "4.5.9-ironic-inspector",
    "4.5.9-kube-rbac-proxy",
    "4.5.9-machine-config-operator",
    "4.5.9-ovirt-machine-controllers",
    "4.5.9-cluster-dns-operator",
    "4.5.9-aws-pod-identity-webhook",
    "4.5.9-ironic-machine-os-downloader",
    "4.5.9-cluster-csi-snapshot-controller-operator",
    "4.5.9-cluster-image-registry-operator",
    "4.5.9-kube-proxy",
    "4.5.9-cluster-autoscaler-operator",
    "4.5.9-multus-whereabouts-ipam-cni",
    "4.5.9-cluster-bootstrap",
    "4.5.9-insights-operator",
    "4.5.9-keepalived-ipfailover",
    "4.5.9-machine-api-operator",
    "4.5.9-azure-machine-controllers",
    "4.5.9-cluster-policy-controller",
    "4.5.9-cluster-machine-approver",
    "4.5.9-deployer",
    "4.5.9-installer-artifacts",
    "4.5.9-gcp-machine-controllers",
    "4.5.9-service-ca-operator",
    "4.5.9-prometheus-config-reloader",
    "4.5.9-oauth-proxy",
    "4.5.9-ovn-kubernetes",
    "4.5.9-prometheus",
    "4.5.9-sdn",
    "4.5.9-ironic-hardware-inventory-recorder",
    "4.5.9-baremetal-runtimecfg",
    "4.5.9-baremetal-installer",
    "4.5.9-ironic-ipa-downloader",
    "4.5.9-operator-registry",
    "4.5.9-pod",
    "4.5.9-multus-cni",
    "4.5.9-cloud-credential-operator",
    "4.5.9-kuryr-cni",
    "4.5.9-cluster-kube-storage-version-migrator-operator",
    "4.5.9-docker-builder",
    "4.5.9-telemeter",
    "4.5.9-tools",
    "4.5.9-docker-registry",
    "4.5.9-baremetal-machine-controllers",
    "4.5.9-tests",
    "4.5.9-cluster-svcat-controller-manager-operator",
    "4.5.9-cluster-ingress-operator",
    "4.5.9-kube-client-agent",
    "4.5.9-cli",
    "4.5.9-ironic-static-ip-manager",
    "4.5.9-jenkins",
    "4.5.9-cluster-node-tuned",
    "4.5.9-cluster-storage-operator",
    "4.5.9-prometheus-node-exporter",
    "4.5.9-openstack-machine-controllers",
    "4.5.9-cluster-openshift-apiserver-operator",
    "4.5.9-ironic",
    "4.5.9-cluster-version-operator",
    "4.5.9-local-storage-static-provisioner",
    "4.5.9-mdns-publisher",
    "4.5.9-multus-route-override-cni",
    "4.5.9-thanos",
    "4.5.9-cluster-config-operator",
    "4.5.9-jenkins-agent-maven",
    "4.5.9-openshift-controller-manager",
    "4.5.9-prometheus-alertmanager",
    "4.5.9-cluster-autoscaler",
    "4.5.9-configmap-reloader",
    "4.5.9-multus-admission-controller",
    "4.5.9-kube-state-metrics",
    "4.5.9-coredns",
    "4.5.9-cluster-openshift-controller-manager-operator",
    "4.5.9-cluster-samples-operator",
    "4.5.9-cluster-svcat-apiserver-operator",
    "4.5.9-must-gather",
    "4.5.9-cluster-network-operator"
  ]
}
~~~

Now, extract the new `openshift-install` command:
~~~
$ oc adm -a ${LOCAL_SECRET_JSON} release extract --command=openshift-install "${LOCAL_REGISTRY}/${LOCAL_REPOSITORY}:${OCP_RELEASE}-${ARCHITECTURE}
~~~

The resulting file can be found in the current working directory:
~~~
$ ls openshift-install
openshift-install
~~~

## Installing a disconnected cluster

Once the new openshift-install client was created, continue with the actual cluster installation.

For example, on AWS, use [https://docs.openshift.com/container-platform/4.5/installing/installing_aws/installing-restricted-networks-aws.html#installing-restricted-networks-aws](https://docs.openshift.com/container-platform/4.5/installing/installing_aws/installing-restricted-networks-aws.html#installing-restricted-networks-aws)

Create and then modify the `install-config.yaml` file on the installation server. Make sure to add `imageContentSources` (from the output of the last command) and also add the rootCA to the `additionalTrustBundle`:
~~~
cat install-config.yaml
(...)
imageContentSources:
- mirrors:
  - 10.10.181.198:5000/ocp4/openshift4
  source: quay.io/openshift-release-dev/ocp-release
- mirrors:
  - 10.10.181.198:5000/ocp4/openshift4
  source: quay.io/openshift-release-dev/ocp-v4.0-art-dev
additionalTrustBundle: |
  -----BEGIN CERTIFICATE-----
  MIIFxjCCA66gAwIBAgIJAOpJf+VRkKHUMA0GCSqGSIb3DQEBCwUAMIGKMQswCQYD
  VQQGEwJVUzEXMBUGA1UECAwOTm9ydGggQ2Fyb2xpbmExEDAOBgNVBAcMB1JhbGVp
  (... contents of CA/rootCA.crt ...)
  oxmeuWzlWvcOCb4Usxa9m0rO95fa5Af2QoA4qxlvi7JoTypRR/zAskqHmcsaTfJN
  k4p3g3YK5u5tvbzERBTfl8bfbd/eIwBLNMDwmKk2z42bcP1OAAHHqCfChvc1Zasr
  TFVS2yQvfvvYSzW6tQE9UphVIiXeQhGhl7+TQ1wgoHRQL3pZRwxX9PrT
  -----END CERTIFICATE-----
(...)
~~~

Also, make sure to modify the `pullSecret` section in install-config.yaml to include the credentials for the custom registry:
~~~
pullSecret: '(... contents of pull-secret-merged.json ...)'
~~~

Steps for modifying `install-config.yaml` can be found in [https://docs.openshift.com/container-platform/4.5/installing/installing_aws/installing-restricted-networks-aws.html#installation-generate-aws-user-infra-install-config_installing-restricted-networks-aws](https://docs.openshift.com/container-platform/4.5/installing/installing_aws/installing-restricted-networks-aws.html#installation-generate-aws-user-infra-install-config_installing-restricted-networks-aws)

Follow any further steps from the restricted installation documentation. Finally, run the installation. Use the custom `openshift-install` binary that was generated earlier:
~~~
 ./openshift-install create cluster --dir=install-config/ --log-level=debug
~~~
