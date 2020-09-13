### Spawning OpenShift on OSP 13 ###

Use the official guide: 
[https://access.redhat.com/documentation/en-us/openshift_container_platform/4.2/html/installing/installing-on-openstack](https://access.redhat.com/documentation/en-us/openshift_container_platform/4.2/html/installing/installing-on-openstack)

#### Prerequisites ####

At time of this writing, the guide isn't entirely exact with regards to several prerequisites.

Create working directory:
~~~
mkdir clouds
cd clouds
~~~

Generate clouds.yaml:
* [https://github.com/andreaskaris/blog/blob/master/using_clouds_yaml.md](https://github.com/andreaskaris/blog/blob/master/using_clouds_yaml.md)

* [https://access.redhat.com/documentation/en-us/openshift_container_platform/4.2/html/installing/installing-on-openstack](https://access.redhat.com/documentation/en-us/openshift_container_platform/4.2/html/installing/installing-on-openstack)

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
openstack quota set --secgroups -1 --secgroup-rules -1 --cores -1 --ram -1 --gigabytes -1 admin
~~~

Also if it's necessary to spawn a higher number of workers:
~~~
nova quota-class-update default --instances -1 --ram -1 --cores -1
~~~

Create flavor:
~~~
openstack --os-cloud openstack flavor create --disk 25 --ram 32768 --vcpus 8 m1.openshift
openstack flavor set --property hw:cpu_policy=dedicated  --property hw:mem_page_size=large m1.openshift
~~~

Download image and create it:
[https://access.redhat.com/downloads/content/290/ver=4.2/rhel---8/4.2.1/x86_64/product-software](https://access.redhat.com/downloads/content/290/ver=4.2/rhel---8/4.2.1/x86_64/product-software)

Note that there's only one coreos image which is based on RHCOREOS 8. Download that. At time of this writing, the file ending is .qcow2, but
it's really a .qcow2.gz. Hence, rename the file to .gz and extract:
~~~
mv rhcos-4.2.0-x86_64-openstack.qcow2{,.gz}
gunzip rhcos-4.2.0-x86_64-openstack.qcow2.gz
~~~

Create the following directory structure:
~~~
[stack@host clouds]$ ll
total 2247156
-rw-rw-r--. 1 stack stack        322 Oct 30 12:42 clouds.yaml
drwxrwxr-x. 4 stack stack        242 Nov  4 07:45 install-config
drwxrwxr-x. 2 stack stack         48 Oct 30 12:24 openshift-client
-rw-rw-r--. 1 stack stack   24533950 Oct 30 12:23 openshift-client-linux-4.2.0.tar.gz
-rwxr-xr-x. 1 stack stack  293887936 Oct 10 17:49 openshift-install
-rw-rw-r--. 1 stack stack   71492736 Oct 30 12:23 openshift-install-linux-4.2.0.tar.gz
-rw-r--r--. 1 stack stack        706 Oct 10 17:49 README.md
-rw-rw-r--. 1 stack stack 1911160832 Oct 30 11:33 rhcos-4.2.0-x86_64-openstack.qcow2
~~~

#### Deleting and recreating a cluster when something went wrong ####
~~~
./openshift-install destroy cluster  --dir install-config/  --log-level=info
rm -f install-config/terraform.tfstate 
rm -f install-config/.openshift_install.log 
cp ~/backup/install-config.yaml install-config/
./openshift-install create cluster --dir=install-config/ --log-level=info
~~~

#### Troubleshooting ####

How to access the cluster when masters and workers are up?
~~~
(overcloud) [stack@undercloud-0 clouds]$ grep api /etc/hosts
172.16.0.231 api.openshift.example.com
~~~

~~~
(overcloud) [stack@undercloud-0 clouds]$ openstack --os-cloud openstack server list
+--------------------------------------+------------------------------+--------+-------------------------------------+-------+--------------+
| ID                                   | Name                         | Status | Networks                            | Image | Flavor       |
+--------------------------------------+------------------------------+--------+-------------------------------------+-------+--------------+
| b0b7c0fe-b32a-47de-aef0-2b5655bd3441 | openshift-ns98d-worker-w4djg | ACTIVE | openshift-ns98d-openshift=10.0.0.22 | rhcos | m1.openshift |
| 1087c10a-9f65-484b-8ade-b13637d9887c | openshift-ns98d-worker-9tkbt | ACTIVE | openshift-ns98d-openshift=10.0.0.25 | rhcos | m1.openshift |
| a8cf9a89-896c-4172-b591-f5b9fa0696a8 | openshift-ns98d-worker-7ktvw | ACTIVE | openshift-ns98d-openshift=10.0.0.26 | rhcos | m1.openshift |
| 303dabd2-e42e-4fa7-af8a-c82de43926ae | openshift-ns98d-master-2     | ACTIVE | openshift-ns98d-openshift=10.0.0.41 | rhcos | m1.openshift |
| c84a1425-6ddb-4d4f-893e-cb8de242b2a0 | openshift-ns98d-master-0     | ACTIVE | openshift-ns98d-openshift=10.0.0.20 | rhcos | m1.openshift |
| d7030165-26a5-463c-b663-41119d0d3ee3 | openshift-ns98d-master-1     | ACTIVE | openshift-ns98d-openshift=10.0.0.16 | rhcos | m1.openshift |
+--------------------------------------+------------------------------+--------+-------------------------------------+-------+--------------+
~~~

~~~
(undercloud) [stack@undercloud-0 ~]$ eval $(ssh-agent)
Agent pid 29891
(undercloud) [stack@undercloud-0 ~]$ ssh-add ~/.ssh/id_rsa
Identity added: /home/stack/.ssh/id_rsa (/home/stack/.ssh/id_rsa)
(undercloud) [stack@undercloud-0 ~]$ ssh core@api.openshift.example.com -A
Warning: Permanently added 'api.openshift.example.com,172.16.0.231' (ECDSA) to the list of known hosts.
Red Hat Enterprise Linux CoreOS 42.80.20191010.0
WARNING: Direct SSH access to machines is not recommended.

---
Last login: Mon Nov  4 16:23:16 2019 from 172.16.0.84
[systemd]
Failed Units: 8
  crio-06678432c466dbdb9cfa51bfa4c0cb57befa547c3c98431316a04631f508ce23.scope
  crio-4f7846e6fd60e2f6da52c6a183b7501a88dd3ab71906fd596013ec6d6c40d8af.scope
  crio-5d6be1d80417068e898a3b0cb4a61e99ee7720c9e1e6d25a890b8b837ae58331.scope
  crio-5fc3f586741d4e11858755d125148834e151deca7812f5ee5a33b44e03725984.scope
  crio-b11bf4e4c9b8fbdb6f6df870d146c74be0cf3a39a444b4f410c7d17080f589b3.scope
  crio-be2df06f51061d76c5fc64ec3fd63f15d21fbe8cf21fc590757caefbe9f12dc8.scope
  crio-c9bff014902ed038cf3406dd08ff2098a3db932b9fcbff578861787cf6e547aa.scope
  crio-e17e1746877e470ec60c87373b958d84b12d4278545fea800f1e899b0ab15b83.scope
[core@openshift-ns98d-master-1 ~]$ 
[core@openshift-ns98d-master-1 ~]$ ssh core@10.0.0.22
Red Hat Enterprise Linux CoreOS 42.80.20191010.0
WARNING: Direct SSH access to machines is not recommended.

---

[systemd]
Failed Units: 1
  multipathd.service
[core@openshift-ns98d-worker-w4djg ~]$ 
~~~
