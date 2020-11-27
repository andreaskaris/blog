# Setting up a private registry

#### Prerequisites

After creating a virtual machine that will serve as a registry, install podman and httpd-tools:
~~~
[root@mirror ~]# sudo yum -y install podman httpd-tools
~~~

#### Generating certificate authority and registry certificates

Now, generate a self-signed CA:
~~~
[root@mirror ~]# mkdir CA
[root@mirror ~]# cd CA
[root@mirror CA]# openssl genrsa -out rootCA.key 4096
(...)
[root@mirror CA]# openssl req -x509 -new -nodes -key rootCA.key -sha256 -days 10240 -out rootCA.crt -subj "/C=CA/ST=Arctica/L=Northpole/O=Acme Inc/OU=DevOps/CN=www.example.com/emailAddress=dev@www.example.com"
(...)
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

#### Starting the registry

You have 2 options here. Password protect your registry, or don't.

##### Password protection

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

##### No password protection

Simply run the containter with:
~~~
sudo podman run --name mirror-registry \
  -p 5000:5000 -v /opt/registry/data:/var/lib/registry:z      \
  -v /opt/registry/certs:/certs:z      \
  -e REGISTRY_HTTP_TLS_CERTIFICATE=/certs/domain.crt      \
  -e REGISTRY_HTTP_TLS_KEY=/certs/domain.key      \
  -d docker.io/library/registry:2
~~~

##### Generating a systemd service for the registry

Generate a service file so that the container autostarts:
~~~
[root@kind certificates]# podman generate systemd --name mirror-registry > /etc/systemd/system/mirror-registry-container.service
[root@kind certificates]# systemctl daemon-reload
[root@kind certificates]# systemctl enable --now mirror-registry-container
Created symlink /etc/systemd/system/multi-user.target.wants/mirror-registry-container.service → /etc/systemd/system/mirror-registry-container.service.
Created symlink /etc/systemd/system/default.target.wants/mirror-registry-container.service → /etc/systemd/system/mirror-registry-container.service.
~~~

#### Configuring the firewall

Open the firewall with iptables or firewall-cmd as describe in the documentation. In my case, I'm not running a firewall so this step is not needed.

#### Verification 

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

