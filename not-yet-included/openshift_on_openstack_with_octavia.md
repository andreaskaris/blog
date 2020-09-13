### Prerequisites ###

Install OpenShift on OpenStack. See: [https://github.com/andreaskaris/blog/blob/master/openshift_on_openstack_with_kuryr.md](https://github.com/andreaskaris/blog/blob/master/openshift_on_openstack_with_kuryr.md) for instructions. Do not use `networkType: Kuryr`, instead use the default `networkType:  OpenShiftSDN`.

### Upstream documentation ###

* [https://kubernetes.io/docs/concepts/cluster-administration/cloud-providers/#openstack](https://kubernetes.io/docs/concepts/cluster-administration/cloud-providers/#openstack)
* [https://kubernetes.io/docs/tasks/access-application-cluster/create-external-load-balancer/](https://kubernetes.io/docs/tasks/access-application-cluster/create-external-load-balancer/)

Looking at the first link, it's clear that one needs to modify `cloud.conf`:
~~~
cloud.conf

Kubernetes knows how to interact with OpenStack via the file cloud.conf. It is the file that will provide Kubernetes with credentials and location for the OpenStack auth endpoint. You can create a cloud.conf file by specifying the following details in it
Typical configuration

This is an example of a typical configuration that touches the values that most often need to be set. It points the provider at the OpenStack cloud’s Keystone endpoint, provides details for how to authenticate with it, and configures the load balancer:

[Global]
username=user
password=pass
auth-url=https://<keystone_ip>/identity/v3
tenant-id=c869168a828847f39f7f06edd7305637
domain-id=2a73b8f597c04551a0fdc8e95544be8a

[LoadBalancer]
subnet-id=6937f8fa-858d-4bc9-a3a5-18d2c957166a
~~~

Note that actually the following key/values need to be set:
~~~
[LoadBalancer]
use-octavia = true
subnet-id = <subnet ID of OpenShift worker nodes' IPs>
floating-network-id = <network ID of floating IPs for Octavia>
~~~

### Configuration ###

#### Looking at current configuration ####

When looking at the master nodes, can see that the cloud.conf configuration is stored in `/etc/kubernetes/cloud.conf` on the master nodes:
~~~
[root@osc-lsjmn-master-1 ~]# ps aux | grep [c]loud.conf
root        2824  8.6  0.7 2569452 244584 ?      Ssl  Dec23 882:56 /usr/bin/hyperkube kubelet --config=/etc/kubernetes/kubelet.conf --bootstrap-kubeconfig=/etc/kubernetes/kubeconfig --rotate-certificates --kubeconfig=/var/lib/kubelet/kubeconfig --container-runtime=remote --container-runtime-endpoint=/var/run/crio/crio.sock --node-labels=node-role.kubernetes.io/master,node.openshift.io/os_id=rhcos --minimum-container-ttl-duration=6m0s --cloud-provider=openstack --volume-plugin-dir=/etc/kubernetes/kubelet-plugins/volume/exec --cloud-config=/etc/kubernetes/cloud.conf --register-with-taints=node-role.kubernetes.io/master=:NoSchedule --v=3
~~~

And it looks like this:
~~~
[root@osc-lsjmn-master-1 ~]# cat /etc/kubernetes/cloud.conf
[Global]
secret-name = openstack-credentials
secret-namespace = kube-system
kubeconfig-path = /var/lib/kubelet/kubeconfig
~~~

Note that this configuration is written by a configmap:
~~~
[stack@undercloud-0 ~]$ kubectl get configmaps -n openshift-config cloud-provider-config -o yaml
apiVersion: v1
data:
  config: |
    [Global]
    secret-name = openstack-credentials
    secret-namespace = kube-system
    kubeconfig-path = /var/lib/kubelet/kubeconfig
    region = regionOne
kind: ConfigMap
metadata:
  creationTimestamp: "2019-12-23T18:36:14Z"
  name: cloud-provider-config
  namespace: openshift-config
  resourceVersion: "50"
  selfLink: /api/v1/namespaces/openshift-config/configmaps/cloud-provider-config
  uid: 19bfa1f7-25b3-11ea-b9b3-fa163e2d3404
~~~

And this pulls the following secret:
~~~
[stack@undercloud-0 ~]$ oc get secret -n kube-system openstack-credentials -o yaml
apiVersion: v1
data:
  clouds.conf: W0dsb2JhbF0KYXV0aC11cmwgICAgPSBodHRwOi8vMTcyLjE2LjAuMTA4OjUwMDAvL3YzCnVzZXJuYW1lICAgID0gYWRtaW4KcGFzc3dvcmQgICAgPSAielA0YmUydWtocENrajR6cVJmVWs4WGpRYiIKdGVuYW50LWlkICAgPSA5NDEwODEzMDMxOWE0MjhjYmI5OWI1OTg4ZThiYTBmMQp0ZW5hbnQtbmFtZSA9IGFkbWluCmRvbWFpbi1uYW1lID0gRGVmYXVsdApyZWdpb24gICAgICA9IHJlZ2lvbk9uZQoK
  clouds.yaml: Y2xvdWRzOgogIG9wZW5zdGFjazoKICAgIGF1dGg6CiAgICAgIGFwcGxpY2F0aW9uX2NyZWRlbnRpYWxfaWQ6ICIiCiAgICAgIGFwcGxpY2F0aW9uX2NyZWRlbnRpYWxfbmFtZTogIiIKICAgICAgYXBwbGljYXRpb25fY3JlZGVudGlhbF9zZWNyZXQ6ICIiCiAgICAgIGF1dGhfdXJsOiBodHRwOi8vMTcyLjE2LjAuMTA4OjUwMDAvL3YzCiAgICAgIGRlZmF1bHRfZG9tYWluOiAiIgogICAgICBkb21haW5faWQ6ICIiCiAgICAgIGRvbWFpbl9uYW1lOiAiIgogICAgICBwYXNzd29yZDogelA0YmUydWtocENrajR6cVJmVWs4WGpRYgogICAgICBwcm9qZWN0X2RvbWFpbl9pZDogIiIKICAgICAgcHJvamVjdF9kb21haW5fbmFtZTogIiIKICAgICAgcHJvamVjdF9pZDogOTQxMDgxMzAzMTlhNDI4Y2JiOTliNTk4OGU4YmEwZjEKICAgICAgcHJvamVjdF9uYW1lOiBhZG1pbgogICAgICB0b2tlbjogIiIKICAgICAgdXNlcl9kb21haW5faWQ6ICIiCiAgICAgIHVzZXJfZG9tYWluX25hbWU6IERlZmF1bHQKICAgICAgdXNlcl9pZDogIiIKICAgICAgdXNlcm5hbWU6IGFkbWluCiAgICBhdXRoX3R5cGU6ICIiCiAgICBjYWNlcnQ6ICIiCiAgICBjZXJ0OiAiIgogICAgY2xvdWQ6ICIiCiAgICBpZGVudGl0eV9hcGlfdmVyc2lvbjogIjMiCiAgICBrZXk6ICIiCiAgICBwcm9maWxlOiAiIgogICAgcmVnaW9uX25hbWU6IHJlZ2lvbk9uZQogICAgcmVnaW9uczogbnVsbAogICAgdmVyaWZ5OiB0cnVlCiAgICB2b2x1bWVfYXBpX3ZlcnNpb246ICIiCg==
kind: Secret
metadata:
  creationTimestamp: "2019-12-23T18:36:15Z"
  name: openstack-credentials
  namespace: kube-system
  resourceVersion: "54"
  selfLink: /api/v1/namespaces/kube-system/secrets/openstack-credentials
  uid: 1a2a1239-25b3-11ea-b9b3-fa163e2d3404
type: Opaque
~~~

Once can decode the base64 values to see the credentials that OpenShift / kubernetes uses to talk to the OpenStack cluster:
~~~
[stack@undercloud-0 ~]$ echo "W0dsb2JhbF0KYXV0aC11cmwgICAgPSBodHRwOi8vMTcyLjE2LjAuMTA4OjUwMDAvL3YzCnVzZXJuYW1lICAgID0gYWRtaW4KcGFzc3dvcmQgICAgPSAielA0YmUydWtocENrajR6cVJmVWs4WGpRYiIKdGVuYW50LWlkICAgPSA5NDEwODEzMDMxOWE0MjhjYmI5OWI1OTg4ZThiYTBmMQp0ZW5hbnQtbmFtZSA9IGFkbWluCmRvbWFpbi1uYW1lID0gRGVmYXVsdApyZWdpb24gICAgICA9IHJlZ2lvbk9uZQoK" | base64 -d
[Global]
auth-url    = http://172.16.0.108:5000//v3
username    = admin
password    = "zP4be2ukhpCkj4zqRfUk8XjQb"
tenant-id   = 94108130319a428cbb99b5988e8ba0f1
tenant-name = admin
domain-name = Default
region      = regionOne
~~~

~~~
[stack@undercloud-0 ~]$ echo "Y2xvdWRzOgogIG9wZW5zdGFjazoKICAgIGF1dGg6CiAgICAgIGFwcGxpY2F0aW9uX2NyZWRlbnRpYWxfaWQ6ICIiCiAgICAgIGFwcGxpY2F0aW9uX2NyZWRlbnRpYWxfbmFtZTogIiIKICAgICAgYXBwbGljYXRpb25fY3JlZGVudGlhbF9zZWNyZXQ6ICIiCiAgICAgIGF1dGhfdXJsOiBodHRwOi8vMTcyLjE2LjAuMTA4OjUwMDAvL3YzCiAgICAgIGRlZmF1bHRfZG9tYWluOiAiIgogICAgICBkb21haW5faWQ6ICIiCiAgICAgIGRvbWFpbl9uYW1lOiAiIgogICAgICBwYXNzd29yZDogelA0YmUydWtocENrajR6cVJmVWs4WGpRYgogICAgICBwcm9qZWN0X2RvbWFpbl9pZDogIiIKICAgICAgcHJvamVjdF9kb21haW5fbmFtZTogIiIKICAgICAgcHJvamVjdF9pZDogOTQxMDgxMzAzMTlhNDI4Y2JiOTliNTk4OGU4YmEwZjEKICAgICAgcHJvamVjdF9uYW1lOiBhZG1pbgogICAgICB0b2tlbjogIiIKICAgICAgdXNlcl9kb21haW5faWQ6ICIiCiAgICAgIHVzZXJfZG9tYWluX25hbWU6IERlZmF1bHQKICAgICAgdXNlcl9pZDogIiIKICAgICAgdXNlcm5hbWU6IGFkbWluCiAgICBhdXRoX3R5cGU6ICIiCiAgICBjYWNlcnQ6ICIiCiAgICBjZXJ0OiAiIgogICAgY2xvdWQ6ICIiCiAgICBpZGVudGl0eV9hcGlfdmVyc2lvbjogIjMiCiAgICBrZXk6ICIiCiAgICBwcm9maWxlOiAiIgogICAgcmVnaW9uX25hbWU6IHJlZ2lvbk9uZQogICAgcmVnaW9uczogbnVsbAogICAgdmVyaWZ5OiB0cnVlCiAgICB2b2x1bWVfYXBpX3ZlcnNpb246ICIiCg==" | base64 -d
clouds:
  openstack:
    auth:
      application_credential_id: ""
      application_credential_name: ""
      application_credential_secret: ""
      auth_url: http://172.16.0.108:5000//v3
      default_domain: ""
      domain_id: ""
      domain_name: ""
      password: zP4be2ukhpCkj4zqRfUk8XjQb
      project_domain_id: ""
      project_domain_name: ""
      project_id: 94108130319a428cbb99b5988e8ba0f1
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

#### Allowing OpenShift to talk to Octavia ####

Determine neutron networks. In this case, we use `provider1` for floating IPs and hence this network's UUID is used for the floating-network-id:
~~~
(overcloud) [stack@undercloud-0 ~]$ neutron net-list
neutron CLI is deprecated and will be removed in the future. Use openstack CLI instead.
+--------------------------------------+----------------------------------------------------+----------------------------------+-------------------------------------------------------+
| id                                   | name                                               | tenant_id                        | subnets                                               |
+--------------------------------------+----------------------------------------------------+----------------------------------+-------------------------------------------------------+
| 40e01c72-7e42-41fe-9bda-ddceb451af6f | private1                                           | 94108130319a428cbb99b5988e8ba0f1 | 5234769c-7c26-4510-afc3-a8ad3b656773 192.168.0.0/24   |
| 96e586a4-8600-49d4-8e2f-c7f2a6b48abe | provider1                                          | 94108130319a428cbb99b5988e8ba0f1 | 896af449-51de-4591-966d-e02bf7f2765f 172.16.0.0/24    |
| b34798e3-db86-4983-a896-41619f767842 | osc-lsjmn-openshift                                | 94108130319a428cbb99b5988e8ba0f1 | 1209b271-879e-45b1-9206-bae4f572bf5a 172.31.0.0/16    |
| b6209904-3d5e-4b2d-9618-42932983f00a | HA network tenant 94108130319a428cbb99b5988e8ba0f1 |                                  | 1a0bac7f-1760-4a44-a77a-dda6d3485f64 169.254.192.0/18 |
| c19374df-2450-42a4-8e6c-1c0b8ca8cf8a | lb-mgmt-net                                        | 94108130319a428cbb99b5988e8ba0f1 | 2556eee1-a52b-47a7-bb66-ecd69a6597c4 172.24.0.0/16    |
+--------------------------------------+----------------------------------------------------+----------------------------------+-------------------------------------------------------+
~~~

The `osc-<xxx>-nodes` subnet is the one that's used for the subnet-id: 
~~~
(overcloud) [stack@undercloud-0 ~]$ neutron subnet-list
neutron CLI is deprecated and will be removed in the future. Use openstack CLI instead.
+--------------------------------------+---------------------------------------------------+----------------------------------+------------------+------------------------------------------------------+
| id                                   | name                                              | tenant_id                        | cidr             | allocation_pools                                     |
+--------------------------------------+---------------------------------------------------+----------------------------------+------------------+------------------------------------------------------+
| 1209b271-879e-45b1-9206-bae4f572bf5a | osc-lsjmn-nodes                                   | 94108130319a428cbb99b5988e8ba0f1 | 172.31.0.0/16    | {"start": "172.31.0.10", "end": "172.31.62.128"}     |
| 1a0bac7f-1760-4a44-a77a-dda6d3485f64 | HA subnet tenant 94108130319a428cbb99b5988e8ba0f1 |                                  | 169.254.192.0/18 | {"start": "169.254.192.1", "end": "169.254.255.254"} |
| 2556eee1-a52b-47a7-bb66-ecd69a6597c4 | lb-mgmt-subnet                                    | 94108130319a428cbb99b5988e8ba0f1 | 172.24.0.0/16    | {"start": "172.24.0.2", "end": "172.24.255.254"}     |
| 5234769c-7c26-4510-afc3-a8ad3b656773 | private1-subnet                                   | 94108130319a428cbb99b5988e8ba0f1 | 192.168.0.0/24   | {"start": "192.168.0.100", "end": "192.168.0.150"}   |
| 896af449-51de-4591-966d-e02bf7f2765f | provider1-subnet                                  | 94108130319a428cbb99b5988e8ba0f1 | 172.16.0.0/24    | {"start": "172.16.0.100", "end": "172.16.0.150"}     |
+--------------------------------------+---------------------------------------------------+----------------------------------+------------------+------------------------------------------------------+
(overcloud) [stack@undercloud-0 ~]$ openstack server list --all
+--------------------------------------+----------------------------------------------+--------+----------------------------------------------------------+----------------------------------------+--------------+
| ID                                   | Name                                         | Status | Networks                                                 | Image                                  | Flavor       |
+--------------------------------------+----------------------------------------------+--------+----------------------------------------------------------+----------------------------------------+--------------+
| 462fae98-2874-46f9-b683-2925dbbea0a2 | osc-lsjmn-worker-6fx9d                       | ACTIVE | osc-lsjmn-openshift=172.31.0.35                          | rhcos                                  | m1.openshift |
| 4521bffe-9185-4213-9686-0888be71308c | osc-lsjmn-worker-rd9b5                       | ACTIVE | osc-lsjmn-openshift=172.31.0.17                          | rhcos                                  | m1.openshift |
| a76d2729-ba04-414f-b155-82d5836605c8 | osc-lsjmn-worker-nwl5k                       | ACTIVE | osc-lsjmn-openshift=172.31.0.38                          | rhcos                                  | m1.openshift |
| 63bda1e6-be94-483b-a0db-3b174f3613dc | osc-lsjmn-worker-vzn8n                       | ACTIVE | osc-lsjmn-openshift=172.31.0.15                          | rhcos                                  | m1.openshift |
| 2bca3bd9-acbe-4309-9495-0ee841a1c2fe | osc-lsjmn-master-0                           | ACTIVE | osc-lsjmn-openshift=172.31.0.30                          | rhcos                                  | m1.openshift |
| 3eaacb1e-fa5f-4315-90a3-5a5b95e19dc9 | osc-lsjmn-master-2                           | ACTIVE | osc-lsjmn-openshift=172.31.0.19                          | rhcos                                  | m1.openshift |
| 2a8dbde2-43e5-4b9a-a726-c2ba97ecc57d | osc-lsjmn-master-1                           | ACTIVE | osc-lsjmn-openshift=172.31.0.24                          | rhcos                                  | m1.openshift |
| d881c036-5c10-475b-a631-94d9eb97e7c2 | rhel-test                                    | ACTIVE | private1=192.168.0.107, 172.16.0.115                     | rhel                                   | m1.small     |
+--------------------------------------+----------------------------------------------+--------+----------------------------------------------------------+----------------------------------------+--------------+
~~~

Modify the configmap so that it looks like the following:
~~~
[stack@undercloud-0 ~]$ kubectl edit configmaps -n openshift-config cloud-provider-config 
[stack@undercloud-0 ~]$ kubectl get configmaps -n openshift-config cloud-provider-config -o yaml
apiVersion: v1
data:
  config: |
    [Global]
    secret-name = openstack-credentials
    secret-namespace = kube-system
    kubeconfig-path = /var/lib/kubelet/kubeconfig
    region = regionOne

    [LoadBalancer]
    use-octavia = true
    subnet-id = 1209b271-879e-45b1-9206-bae4f572bf5a
    floating-network-id = 96e586a4-8600-49d4-8e2f-c7f2a6b48abe
kind: ConfigMap
metadata:
  creationTimestamp: "2019-12-23T18:36:14Z"
  name: cloud-provider-config
  namespace: openshift-config
  resourceVersion: "4529388"
  selfLink: /api/v1/namespaces/openshift-config/configmaps/cloud-provider-config
  uid: 19bfa1f7-25b3-11ea-b9b3-fa163e2d3404
~~~

#### Opening security groups ####

The problem is that traffic will be blocked by neutron security groups which are too restrictive:
~~~
(overcloud) [stack@undercloud-0 ~]$ openstack security group list | grep osc-lsjmn-worker
| 0d51678b-60da-43fc-9562-2774f6bb8e01 | osc-lsjmn-worker                        |                        | 94108130319a428cbb99b5988e8ba0f1 |
(overcloud) [stack@undercloud-0 ~]$ openstack security group rule list | head -n2
+--------------------------------------+-------------+---------------+-------------+--------------------------------------+--------------------------------------+
| ID                                   | IP Protocol | IP Range      | Port Range  | Remote Security Group                | Security Group                       |
(overcloud) [stack@undercloud-0 ~]$ openstack security group rule list | grep 0d51678b-60da-43fc-9562-2774f6bb8e01
| 027ac684-407b-48d9-b9a4-3f525ac99027 | udp         | None          | 30000:32767 | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| 0925618e-dd15-444a-a270-d4dbb955c3f3 | udp         | None          | 9000:9999   | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d |
| 13ea21dd-3da6-4c2c-bf51-8ed56e3b775a | tcp         | None          | 10259:10259 | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d |
| 19fd9561-3b2e-41ea-8442-b4ba18ae1956 | tcp         | None          | 10250:10250 | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| 279b2431-ebd6-42b1-80eb-9de3936133cf | tcp         | 0.0.0.0/0     | 80:80       | None                                 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| 2a91945e-50ff-4889-a673-a7fe77218ce5 | tcp         | None          | 9000:9999   | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| 3a415612-38fd-492f-af45-b84683ca9138 | tcp         | 0.0.0.0/0     | 443:443     | None                                 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| 4cd696cd-1d6a-4151-ac8b-6fca1f3c7a7c | tcp         | None          | 9000:9999   | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| 6319ecf1-24c3-4843-8df5-5601d6eaade2 | None        | None          |             | None                                 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| 72fc968b-19f4-4698-99d0-a3a648880aac | icmp        | 0.0.0.0/0     |             | None                                 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| 7aefadc3-2de6-4e72-b03a-a3d3ba9f33fb | udp         | 172.31.0.0/16 | 5353:5353   | None                                 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| 88a70b23-ec17-4937-b041-588958b8cace | udp         | None          | 4789:4789   | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| 94f1fdef-e761-46fd-a95e-0202d2da4594 | tcp         | None          | 9000:9999   | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d |
| 99a56048-161c-4954-8e2e-548d0014ac34 | tcp         | None          | 30000:32767 | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| a54b2ce5-9847-4054-aef2-94b46853e612 | tcp         | None          | 10250:10250 | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| b500a240-b5da-40a4-9cfe-325229454d5e | udp         | None          | 6081:6081   | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| c93663ad-e80d-41dc-8251-c07cee958f41 | udp         | None          | 4789:4789   | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d |
| d0f18ecc-a2d3-44dd-b99e-1422f5c50e8e | tcp         | 0.0.0.0/0     | 22:22       | None                                 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| d4e43f8f-82e3-4fee-a298-a92991ad01c7 | vrrp        | 172.31.0.0/16 |             | None                                 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| d8064b0c-494e-48df-ad3e-e45b53916e83 | tcp         | None          | 10257:10257 | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d |
| d81946f9-0e8b-4648-83ca-b80cc0d363dc | tcp         | 172.31.0.0/16 | 1936:1936   | None                                 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| d9afe4e3-2a3e-45fe-b18a-20b994d2a0f0 | udp         | None          | 4789:4789   | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| e2e71c73-3d7f-4582-a73f-9564fecfc4b5 | udp         | None          | 6081:6081   | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| e80d679f-4c11-4862-bd1e-fe1bdc51082f | udp         | None          | 6081:6081   | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d |
| f3e5cfe4-6d57-43de-b7dc-78011dcaead7 | udp         | None          | 9000:9999   | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| f6d73f55-fba2-4a1e-b7ac-dc12b1a9bbec | udp         | None          | 9000:9999   | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| f750c6b5-5316-4015-9834-d746ef678218 | tcp         | None          | 10250:10250 | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d |
| faf4fd32-7124-4e1a-a6c4-47d9d1357631 | tcp         | None          | 6641:6642   | 0d51678b-60da-43fc-9562-2774f6bb8e01 | 0b628ed4-3444-42e9-82c8-c0aedd5e1b4d |
| fd754404-4be4-4180-aa78-55140e4acf59 | None        | None          |             | None                                 | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
~~~

Fix this with:
~~~
(overcloud) [stack@undercloud-0 ~]$ openstack security group rule create 0d51678b-60da-43fc-9562-2774f6bb8e01 --remote-ip 172.31.0.0/16 --dst-port 30000:32767 --protocol tcp
+-------------------+--------------------------------------+
| Field             | Value                                |
+-------------------+--------------------------------------+
| created_at        | 2019-12-30T22:59:49Z                 |
| description       |                                      |
| direction         | ingress                              |
| ether_type        | IPv4                                 |
| id                | 6a219d73-4250-47a1-8017-fa06b09629be |
| name              | None                                 |
| port_range_max    | 32767                                |
| port_range_min    | 30000                                |
| project_id        | 94108130319a428cbb99b5988e8ba0f1     |
| protocol          | tcp                                  |
| remote_group_id   | None                                 |
| remote_ip_prefix  | 172.31.0.0/16                        |
| revision_number   | 0                                    |
| security_group_id | 0d51678b-60da-43fc-9562-2774f6bb8e01 |
| updated_at        | 2019-12-30T22:59:49Z                 |
+-------------------+--------------------------------------+
~~~
> **Note:** Ports `30000:32767` are the dynamic range that's used internally by OpenShift / kubernetes. These ports will be used by Octavia as its members' ports. See below for further details.
> **Note:** `172.31.0.0/16` is the OpenShift subnet as can be seen in the `openstack subnet list` output.

#### Deploying a test nginx cluster and LoadBalancer #####

Now, deploy a test cluster:
~~~
kubectl apply -f https://k8s.io/examples/controllers/nginx-deployment.yaml
kubectl expose deployment nginx-deployment
kubectl edit svc nginx-deployment
~~~

Change the type to LoadBalancer:
~~~
  type: LoadBalancer
~~~

`oc get svc` will show the services's `EXTERNAL-IP` as `<Pending>` for a while. 
Regularly check with `oc describe svc <service name>` to make sure that there are no `ERROR` messages that would indicate issues talking to the Octavia LB.

#### Verifying test nginx cluster and LoadBalancer ####

After a while, kubernetes will show:
~~~
[stack@undercloud-0 ~]$ oc get svc
NAME               TYPE           CLUSTER-IP       EXTERNAL-IP                            PORT(S)        AGE
kubernetes         ClusterIP      172.30.0.1       <none>                                 443/TCP        7d4h
nginx-deployment   LoadBalancer   172.30.142.221   172.16.0.116                           80:31617/TCP   60m
openshift          ExternalName   <none>           kubernetes.default.svc.cluster.local   <none>         7d4h
[stack@undercloud-0 ~]$ 
~~~

And octavia will show:
~~~
(overcloud) [stack@undercloud-0 ~]$ openstack floating ip list | grep 172.16.0.116
| 91ff67b8-c96f-42fa-8739-e34256d1cb5c | 172.16.0.116        | 172.31.0.18      | ff85c407-977e-4593-9a86-1c444e60bfc8 | 96e586a4-8600-49d4-8e2f-c7f2a6b48abe | 94108130319a428cbb99b5988e8ba0f1 |
(overcloud) [stack@undercloud-0 ~]$ openstack loadbalancer list
openstack +--------------------------------------+----------------------------------+----------------------------------+-------------+---------------------+----------+
| id                                   | name                             | project_id                       | vip_address | provisioning_status | provider |
+--------------------------------------+----------------------------------+----------------------------------+-------------+---------------------+----------+
| 54276acf-826e-4d58-80e3-5e85c52b1cd5 | a002f20252b5011ea8656fa163e21cc4 | 94108130319a428cbb99b5988e8ba0f1 | 172.31.0.18 | ACTIVE              | octavia  |
+--------------------------------------+----------------------------------+----------------------------------+-------------+---------------------+----------+
(overcloud) [stack@undercloud-0 ~]$ openstack loadbalancer pool list
+--------------------------------------+-----------------------------------------+----------------------------------+---------------------+----------+--------------+----------------+
| id                                   | name                                    | project_id                       | provisioning_status | protocol | lb_algorithm | admin_state_up |
+--------------------------------------+-----------------------------------------+----------------------------------+---------------------+----------+--------------+----------------+
| 5da3f0d8-a8e6-4a50-8583-79ca872a7e0d | pool_a002f20252b5011ea8656fa163e21cc4_0 | 94108130319a428cbb99b5988e8ba0f1 | ACTIVE              | TCP      | ROUND_ROBIN  | True           |
+--------------------------------------+-----------------------------------------+----------------------------------+---------------------+----------+--------------+----------------+
(overcloud) [stack@undercloud-0 ~]$ openstack loadbalancer member list pool_a002f20252b5011ea8656fa163e21cc4_0
+--------------------------------------+------------------------------------------------------------------+----------------------------------+---------------------+-------------+---------------+------------------+--------+
| id                                   | name                                                             | project_id                       | provisioning_status | address     | protocol_port | operating_status | weight |
+--------------------------------------+------------------------------------------------------------------+----------------------------------+---------------------+-------------+---------------+------------------+--------+
| 0bf0a50c-5359-4a83-b6af-10e82a124d7c | member_a002f20252b5011ea8656fa163e21cc4_0_osc-lsjmn-worker-6fx9d | 94108130319a428cbb99b5988e8ba0f1 | ACTIVE              | 172.31.0.35 |         31617 | NO_MONITOR       |      1 |
| cca6a30c-dd0e-4ef1-baf5-17f7d3eae9df | member_a002f20252b5011ea8656fa163e21cc4_0_osc-lsjmn-worker-nwl5k | 94108130319a428cbb99b5988e8ba0f1 | ACTIVE              | 172.31.0.38 |         31617 | NO_MONITOR       |      1 |
| a031e0b4-e225-4fa4-bb3d-11abd98d4dec | member_a002f20252b5011ea8656fa163e21cc4_0_osc-lsjmn-worker-rd9b5 | 94108130319a428cbb99b5988e8ba0f1 | ACTIVE              | 172.31.0.17 |         31617 | NO_MONITOR       |      1 |
| 0099c8de-1e70-47af-952f-ae3af9299245 | member_a002f20252b5011ea8656fa163e21cc4_0_osc-lsjmn-worker-vzn8n | 94108130319a428cbb99b5988e8ba0f1 | ACTIVE              | 172.31.0.15 |         31617 | NO_MONITOR       |      1 |
+--------------------------------------+------------------------------------------------------------------+----------------------------------+---------------------+-------------+---------------+------------------+--------+
~~~

And looking at one of the worker nodes, we can see the following rules:
~~~
[root@osc-lsjmn-worker-6fx9d ~]# ip a | grep 172.31
    inet 172.31.0.35/16 brd 172.31.255.255 scope global dynamic noprefixroute ens3
    inet 172.31.0.7/16 scope global secondary ens3
[root@osc-lsjmn-worker-6fx9d ~]# iptables-save | grep 31617
-A KUBE-NODEPORTS -p tcp -m comment --comment "default/nginx-deployment:" -m tcp --dport 31617 -j KUBE-MARK-MASQ
-A KUBE-NODEPORTS -p tcp -m comment --comment "default/nginx-deployment:" -m tcp --dport 31617 -j KUBE-SVC-ECF5TUORC5E2ZCRD
[root@osc-lsjmn-worker-6fx9d ~]# iptables-save | grep KUBE-SVC-ECF5TUORC5E2ZCRD
:KUBE-SVC-ECF5TUORC5E2ZCRD - [0:0]
-A KUBE-SERVICES -d 172.30.142.221/32 -p tcp -m comment --comment "default/nginx-deployment: cluster IP" -m tcp --dport 80 -j KUBE-SVC-ECF5TUORC5E2ZCRD
-A KUBE-NODEPORTS -p tcp -m comment --comment "default/nginx-deployment:" -m tcp --dport 31617 -j KUBE-SVC-ECF5TUORC5E2ZCRD
-A KUBE-SVC-ECF5TUORC5E2ZCRD -m statistic --mode random --probability 0.33332999982 -j KUBE-SEP-OW7BD3OQ4R66AUN4
-A KUBE-SVC-ECF5TUORC5E2ZCRD -m statistic --mode random --probability 0.50000000000 -j KUBE-SEP-L4FZLDGYIO7C47A6
-A KUBE-SVC-ECF5TUORC5E2ZCRD -j KUBE-SEP-T7FINN27THN2R62N
-A KUBE-FW-ECF5TUORC5E2ZCRD -m comment --comment "default/nginx-deployment: loadbalancer IP" -j KUBE-SVC-ECF5TUORC5E2ZCRD
[root@osc-lsjmn-worker-6fx9d ~]# 
~~~

Once that's done, the loadbalancer will work:
~~~
(overcloud) [stack@undercloud-0 ~]$ curl 172.16.0.116
<!DOCTYPE html>
<html>
<head>
<title>Welcome to nginx!</title>
<style>
    body {
        width: 35em;
        margin: 0 auto;
        font-family: Tahoma, Verdana, Arial, sans-serif;
    }
</style>
</head>
<body>
<h1>Welcome to nginx!</h1>
<p>If you see this page, the nginx web server is successfully installed and
working. Further configuration is required.</p>

<p>For online documentation and support please refer to
<a href="http://nginx.org/">nginx.org</a>.<br/>
Commercial support is available at
<a href="http://nginx.com/">nginx.com</a>.</p>

<p><em>Thank you for using nginx.</em></p>
</body>
</html>
~~~
