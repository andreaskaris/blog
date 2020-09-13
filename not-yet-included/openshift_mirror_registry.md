### Example for OpenShift offline installation with a mirrored registry ###

In the following, we will run a container registry without hostname (only IP address) and self-signed CA and certificate. We need a virtual machine that serves as the container registry and a host that serves as the OpenShift install server. 

I tested this with OpenShift 4.3 on OpenStack 13.

Follow the OpenShift documentation. The documentation isn't very detailed (at least at the moment), so I'm going through the documentation with an example deployment.
* https://access.redhat.com/documentation/en-us/openshift_container_platform/4.3/html/installing/installation-configuration#installing-restricted-networks-preparations

After creating a virtual machine that will serve as a registry, install podman and httpd-tools:
~~~
yum -y install podman httpd-tools
~~~

Now, generate a self-signed CA:
~~~
[root@mirror ~]# mkdir CA
[root@mirror ~]# cd CA
[root@mirror CA]# ll
total 0
[root@mirror CA]# openssl genrsa -des3 -out rootCA.key 4096
Generating RSA private key, 4096 bit long modulus
.++
....................................................++
e is 65537 (0x10001)
Enter pass phrase for rootCA.key:
140528188733328:error:28069065:lib(40):UI_set_result:result too small:ui_lib.c:831:You must type in 4 to 1023 characters
Enter pass phrase for rootCA.key:
140528188733328:error:28069065:lib(40):UI_set_result:result too small:ui_lib.c:831:You must type in 4 to 1023 characters
Enter pass phrase for rootCA.key:
140528188733328:error:28069065:lib(40):UI_set_result:result too small:ui_lib.c:831:You must type in 4 to 1023 characters
Enter pass phrase for rootCA.key:
140528188733328:error:28069065:lib(40):UI_set_result:result too small:ui_lib.c:831:You must type in 4 to 1023 characters
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
Country Name (2 letter code) [XX]:US
State or Province Name (full name) []:NC
Locality Name (eg, city) [Default City]:Raleigh
Organization Name (eg, company) [Default Company Ltd]:Acme.Inc
Organizational Unit Name (eg, section) []:
Common Name (eg, your name or your server's hostname) []:acme.root
Email Address []:   
[root@mirror CA]# ll
total 8
-rw-r--r--. 1 root root 1960 Feb  3 09:47 rootCA.crt
-rw-r--r--. 1 root root 3311 Feb  3 09:45 rootCA.key
~~~

Trust this rootCA on the mirror registry node and on the node where you are running the OpenShift installer:
~~~
cp rootCA.crt  /etc/pki/ca-trust/source/anchors/
update-ca-trust extract
~~~

Create a certificate signing request:
~~~
mkdir CA/certificates
cd CA/certificates
cat<<'EOF'>config 
[ req ]
distinguished_name = req_distinguished_name
prompt = no
req_extensions = v3_req

[ req_distinguished_name ]
C="US"
ST="NC"
L="Raleigh"
O="Acme Inc."
CN="10.10.181.198"
emailAddress="akaris@example.com"

[ v3_req ]

#basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = 10.10.181.198
IP.1 = 10.10.181.198
EOF
openssl genrsa -out domain.key 4096
openssl req -new -key domain.key -nodes -out domain.csr -config config
~~~

Verify the CSR:
~~~
[root@mirror certificates]# openssl req -in domain.csr -noout -text | grep -i dns
                DNS:10.10.181.198, IP Address:10.10.181.198
~~~

Sign your certificate with the rootCA key and force the SAN entries:
~~~
[root@mirror certificates]# openssl x509 -req -in domain.csr -CA ../rootCA.crt -CAkey ../rootCA.key -CAcreateserial -out domain.crt -days 3650 -sha256 -extensions v3_req -extfile config 
Signature ok
subject=/C=US/ST=NC/L=Raleigh/O=Acme Inc./CN=10.10.181.198/emailAddress=akaris@example.com
Getting CA Private Key
Enter pass phrase for ../rootCA.key:
~~~
See: 
* https://github.com/openssl/openssl/issues/6481
* https://security.stackexchange.com/questions/150078/missing-x509-extensions-with-an-openssl-generated-certificate


Then, trust the certificate and make sure that it passes verification and that it contains the SAN entries:
~~~
[root@mirror certificates]# openssl x509 -in domain.crt -noout -text | grep IP
                DNS:10.10.181.198, IP Address:10.10.181.198
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
[root@mirror ~]# mkdir -p /opt/registry/{auth,certs,data}
~~~

Copy the certificate into /opt/registry/certs as described in the documentation:
~~~
[root@mirror certificates]# cp domain.key  /opt/registry/certs/
[root@mirror certificates]# cp domain.crt  /opt/registry/certs/
~~~

Generate a password, e.g.:
~~~
[root@mirror ~]# htpasswd -bBc /opt/registry/auth/htpasswd root password
Adding password for user root
~~~

Run the containter with:
~~~
podman run --name mirror-registry \
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
[root@mirror ~]# openssl s_client -connect 10.10.181.198:5000
CONNECTED(00000003)
depth=0 C = OZ, ST = State of Oz, L = City of Oz, O = "ACME, Inc.", emailAddress = akaris@example.com, CN = 192.168.123.10
verify return:1
(...)
    Verify return code: 0 (ok)
---
~~~

And make sure that curl does not report an issue:
~~~
[root@mirror certificates]# curl -u root:password https://10.10.181.198:5000/v2/_catalog
{"repositories":[]}
~~~

Now, download your pull-secret from https://cloud.redhat.com/openshift/install/pull-secret. Modify the pull secret by adding the correct base64 encoded password:
~~~
[root@mirror certificates]# echo -n 'root:password' | base64 
cm9vdDpwYXNzd29yZA==
[root@mirror ~]# cat pull-secret-modified.txt 
(...)
        "10.10.181.198:5000": {
            "auth": "cm9vdDpwYXNzd29yZA==",
            "email": "akaris@example.com"
        },
        "registry.redhat.io": {
(...)
~~~

Get `$OCP_RELEASE` from https://quay.io/repository/openshift-release-dev/ocp-release?tag=latest&tab=tags and export required env files:
~~~
[root@mirror ~]#  env | egrep 'OCP|LOCAL|PRODUCT|RELEASE'
RELEASE_NAME=ocp-release
LOCAL_SECRET_JSON=/root/pull-secret-modified.txt
OCP_RELEASE=4.3.0-x86_64
PRODUCT_REPO=openshift-release-dev
LOCAL_REPOSITORY=ocp4/openshift4
LOCAL_REGISTRY=10.10.181.198:5000
~~~

The following command ...
~~~
oc adm -a ${LOCAL_SECRET_JSON} release mirror \
     --from=quay.io/${PRODUCT_REPO}/${RELEASE_NAME}:${OCP_RELEASE} \
     --to=${LOCAL_REGISTRY}/${LOCAL_REPOSITORY} \
     --to-release-image=${LOCAL_REGISTRY}/${LOCAL_REPOSITORY}:${OCP_RELEASE}
~~~

... should download the images:
~~~
[root@mirror ~]# oc adm -a ${LOCAL_SECRET_JSON} release mirror      --from=quay.io/${PRODUCT_REPO}/${RELEASE_NAME}:${OCP_RELEASE}      --to=${LOCAL_REGISTRY}/${LOCAL_REPOSITORY}      --to-release-image=${LOCAL_REGISTRY}/${LOCAL_REPOSITORY}:${OCP_RELEASE}
info: Mirroring 101 images to 192.168.123.10:5000/ocp4/openshift4 ...
10.10.181.198:5000/
  ocp4/openshift4
    blobs:
      quay.io/openshift-release-dev/ocp-v4.0-art-dev sha256:94eeef1238d5121c25ec4f2c77f44646910e36253dc08da798c6babf65ed9531 479B
      quay.io/openshift-release-dev/ocp-v4.0-art-dev sha256:0e8ea260d0262eac3725175d3d499ead6fd77cb1fa8272b3e665e8f64044fb89 1.499KiB
      quay.io/openshift-release-dev/ocp-v4.0-art-dev sha256:4fbc3bafa3d4400bb97a733c1fe12f2f99bf38b9d5b913d5034f29798739654d 1.585KiB
      quay.io/openshift-release-dev/ocp-v4.0-art-dev sha256:c4e552011c627e79a47d52ca6f0d0685f747f29f97b9a8f40f9c2b58b695f30a 1.608KiB
(...)
~~~

Once that's done, modify the install-config.yaml file on the installation server. Make sure to add `imageContentSources` (from the output of the last command) and also add the rootCA to the `additionalTrustBundle`:
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
  (...)
  oxmeuWzlWvcOCb4Usxa9m0rO95fa5Af2QoA4qxlvi7JoTypRR/zAskqHmcsaTfJN
  k4p3g3YK5u5tvbzERBTfl8bfbd/eIwBLNMDwmKk2z42bcP1OAAHHqCfChvc1Zasr
  TFVS2yQvfvvYSzW6tQE9UphVIiXeQhGhl7+TQ1wgoHRQL3pZRwxX9PrT
  -----END CERTIFICATE-----
(...)
~~~

By the way, the `additionalTrustBundle` step can be found here:
* https://access.redhat.com/documentation/en-us/openshift_container_platform/4.3/html/installing_on_bare_metal/installing-on-bare-metal#installation-initializing-manual_installing-restricted-networks-bare-metal

Also, make sure to modify the `pullSecret` section in install-config.yaml to include the credentials for the custom registry:
~~~
pullSecret: '{   
    "auths": {
(...)
        "10.10.181.198:5000": {
            "auth": "cm9vdDpwYXNzd29yZA==",
            "email": "akaris@example.com"
        },
(...)
}
}'
~~~

As a last step, generate the custom installer:
~~~
[root@mirror ~]# ls -al openshift-install
ls: cannot access openshift-install: No such file or directory
[root@mirror ~]# oc adm -a ${LOCAL_SECRET_JSON} release extract --command=openshift-install "${LOCAL_REGISTRY}/${LOCAL_REPOSITORY}:${OCP_RELEASE}"
[root@mirror ~]# ls -al openshift-install
-rwxr-xr-x. 1 root root 329097376 Jan 21 14:47 openshift-install
~~~

This installer must be copied to the installation server:
~~~
[cloud-user@akaris-jump-server openshift-private-registry]$ scp root@10.10.181.198:openshift-install .
~~~

And now, run the installation:
~~~
 ./openshift-install create cluster --dir=install-config/ --log-level=debug
~~~
