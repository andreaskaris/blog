### Documentation ###

* [https://access.redhat.com/documentation/en-us/openshift_container_platform/4.3/html/installing_on_openstack/installing-on-openstack](https://access.redhat.com/documentation/en-us/openshift_container_platform/4.3/html/installing_on_openstack/installing-on-openstack)

### Create jump server ###

OSC 4.3 download the rhcos image directly and uploads it into OSP. Therefore, if latency is high and/or throughput is low, it may be advisable to create a jump server in the cloud from where the OCP installer will be run.

~~~
source <rc file>
openstack server create --key-name akaris_id_rsa --flavor m1.small --image rhel-7.7-server-x86_64-latest --network provider_net_quicklab akaris_jump_server
openstack security group rule create akaris-jump-server --protocol tcp --dst-port 22
openstack server add security group akaris_jump_server akaris-jump-server                             
~~~

Connect to jump server:
~~~
ssh cloud-user@<jump server IP>
~~~

### Prepare jump server ###

~~~
subscription-manager register (...)
subscription-manager repos --enable=rhel-7-server-openstack-13-tools-rpms
yum install screen vim python2-openstackclient
~~~

Prepare directory structure:
~~~
[cloud-user@akaris-jump-server openshift]$ ls -l
total 427864
-rw-rw-r--. 1 cloud-user cloud-user       922 Jan 24 16:28 clouds.yaml
drwxrwxr-x. 2 cloud-user cloud-user        33 Jan 24 16:33 install_backup
drwxrwxr-x. 2 cloud-user cloud-user        63 Jan 24 16:42 install_config
-rw-rw-r--. 1 cloud-user cloud-user  27177324 Jan 24 16:32 oc-4.3.0-linux.tar.gz
-rwxr-xr-x. 1 cloud-user cloud-user 329097376 Jan 21 14:47 openshift-install
-rw-rw-r--. 1 cloud-user cloud-user  81833577 Jan 24 16:31 openshift-install-linux-4.3.0.tar.gz
-rw-rw-r--. 1 cloud-user cloud-user      2735 Jan 24 16:28 pull_secret.txt
~~~

Make sure to update `/etc/hosts`:
~~~
x.x.x.x api.akaris-osc.<cluster URL>
y.y.y.y application.apps.akaris-osc.<cluster URL>
y.y.y.y console-openshift-console.apps.akaris-osc.<cluster URL>
y.y.y.y oauth-openshift.apps.akaris-osc.<cluster URL>
y.y.y.y default-route-openshift-image-registry.apps.akaris-osc.<cluster URL>
~~~

### OpenShift installation ###

~~~
screen
./openshift-install create cluster --dir=install_config/ --log-level=debug
~~~

Note that image creation might take a long time:
~~~
DEBUG module.topology.openstack_networking_router_interface_v2.nodes_router_interface: Still creating... [10s elapsed]
DEBUG module.topology.openstack_networking_router_interface_v2.nodes_router_interface: Creation complete after 12s [id=fb346acf-ef2c-4475-bb0a-9314dbc50a51]
DEBUG module.topology.openstack_networking_floatingip_associate_v2.api_fip[0]: Creation complete after 6s [id=203817bd-29d6-43d0-94a7-f349f700f97e]
DEBUG module.topology.openstack_networking_trunk_v2.masters[0]: Creation complete after 5s [id=473262d2-dd37-4174-8c4e-af14c45f385d]
DEBUG module.topology.openstack_networking_trunk_v2.masters[2]: Creation complete after 6s [id=9b844e57-b68b-4edf-bdf9-678ebb9d43a3]
DEBUG module.topology.openstack_networking_trunk_v2.masters[1]: Creation complete after 6s [id=77ffbad8-a11d-4d05-b987-3a23706cba8e]
DEBUG module.bootstrap.openstack_networking_floatingip_v2.bootstrap_fip: Still creating... [10s elapsed]
DEBUG openstack_images_image_v2.base_image[0]: Still creating... [40s elapsed]
DEBUG module.bootstrap.openstack_networking_floatingip_v2.bootstrap_fip: Creation complete after 11s [id=b650343f-dd94-476b-8687-8861940a9698]
DEBUG openstack_images_image_v2.base_image[0]: Still creating... [50s elapsed]
DEBUG openstack_images_image_v2.base_image[0]: Still creating... [1m0s elapsed]
DEBUG openstack_images_image_v2.base_image[0]: Still creating... [1m10s elapsed]
DEBUG openstack_images_image_v2.base_image[0]: Still creating... [1m20s elapsed]
DEBUG openstack_images_image_v2.base_image[0]: Still creating... [1m30s elapsed]
DEBUG openstack_images_image_v2.base_image[0]: Still creating... [1m40s elapsed]
~~~
