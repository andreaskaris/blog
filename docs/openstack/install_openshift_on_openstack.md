### Spawning OpenShift on OSP 13 ###

Use the official guide: 
[https://access.redhat.com/documentation/en-us/openshift_container_platform/4.5/html/installing/installing-on-openstack](https://access.redhat.com/documentation/en-us/openshift_container_platform/4.5/html/installing/installing-on-openstack)

#### Prerequisites ####

Create working directory:
~~~
mkdir clouds
cd clouds
~~~

Generate clouds.yaml:
* [https://andreaskaris.github.io/blog/openstack/using_clouds_yaml/](https://andreaskaris.github.io/blog/openstack/using_clouds_yaml/)

Update quotas:
~~~
project=$(openstack project list | awk '/admin/ {print $2}')
openstack --os-cloud openstack quota set \
  --secgroups 20 --secgroup-rules 120 --ports 200 --routers 40 \
  --ram 307200 --cores 100 --gigabytes 500 \
  $project
~~~

Or consider disabling quotas altogether:
~~~
openstack quota set --secgroups -1 --secgroup-rules -1 --cores -1 --ram -1 --gigabytes -1 $project
~~~

Also if it's necessary to spawn a higher number of workers:
~~~
nova quota-class-update default --instances -1 --ram -1 --cores -1
~~~

Create flavor:
~~~
openstack --os-cloud openstack flavor create --disk 25 --ram 32768 --vcpus 8 m1.openshift
# If using hugepages for instances:
# openstack flavor set --property hw:cpu_policy=dedicated  --property hw:mem_page_size=large m1.openshift
~~~

Download the installer and client, for example:
~~~
curl -O https://mirror.openshift.com/pub/openshift-v4/clients/ocp/latest-4.5/openshift-install-linux.tar.gz
curl -O https://mirror.openshift.com/pub/openshift-v4/clients/ocp/latest-4.5/openshift-client-linux.tar.gz
sudo tar -xf openshift-install-linux.tar.gz -C /usr/local/bin
sudo tar -xf openshift-client-linux.tar.gz -C /usr/local/bin
~~~

Create the pull-secret in `pull-secret.txt`:
~~~
cat <<'EOF' > pull-secret.txt
(... pull-secret here ...)
EOF
~~~

Create the install-config:
~~~
openshift-install create install-config --dir=install-config
~~~

Make sure to update `/etc/hosts` with the API endpoint (`api.<cluster name>.<domain>`):
~~~
(overcloud) [stack@undercloud-0 clouds]$ grep api /etc/hosts
172.16.0.231 api.cluster.example.com
~~~


#### Deleting and recreating a cluster when something went wrong ####
~~~
openshift-install destroy cluster  --dir install-config/  --log-level=info
rm -Rf install-config
cp -a install-backup installconfig
cp -a install-backup install-config/
openshift-install create cluster --dir=install-config/ --log-level=info
~~~
