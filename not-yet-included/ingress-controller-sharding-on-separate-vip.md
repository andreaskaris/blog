* It is important to know that each node can only host one ingress router. You will need a specific role for each INGRESS type and different INGRESS types cannot be hosted on the same worker node. This is due to the particularities of the `HostNetwork` network type.

* The following example uses the OpenStack IPI but the vSphere IPI should behave similarly.

### Step 1: Scale out by 2 more worker nodes ###

Scale out the worker role. This step might have to be improved.
~~~
[cloud-user@akaris-jump-server cluster-logging]$ oc scale -n openshift-machine-api --replicas=5 machineset akaris-osc-6klvk-worker
machineset.machine.openshift.io/akaris-osc-6klvk-worker scaled
[cloud-user@akaris-jump-server cluster-logging]$ 
~~~

~~~
[cloud-user@akaris-jump-server openshift]$ oc get nodes
NAME                            STATUS   ROLES    AGE     VERSION
akaris-osc-6klvk-master-0       Ready    master   5h18m   v1.18.3+012b3ec
akaris-osc-6klvk-master-1       Ready    master   5h18m   v1.18.3+012b3ec
akaris-osc-6klvk-master-2       Ready    master   5h19m   v1.18.3+012b3ec
akaris-osc-6klvk-worker-g4q6z   Ready    worker   5h4m    v1.18.3+012b3ec
akaris-osc-6klvk-worker-hp2hz   Ready    worker   2m12s   v1.18.3+012b3ec
akaris-osc-6klvk-worker-l726s   Ready    worker   5h1m    v1.18.3+012b3ec
akaris-osc-6klvk-worker-lp2df   Ready    worker   5h6m    v1.18.3+012b3ec
akaris-osc-6klvk-worker-zhs67   Ready    worker   2m57s   v1.18.3+012b3ec
~~~

### Step 2: Add additional interface to additional nodes ###

This step will be executed within the underlying infrastructure provider. In this example case, OpenShift is hosted on top of OpenStack.

Make sure that the subnet runs a DHCP server:
~~~
[cloud-user@akaris-jump-server openshift]$ openstack subnet set --dhcp 41c073c0-5b6a-4639-9023-115ce3d0e378
[cloud-user@akaris-jump-server openshift]$ openstack subnet show 41c073c0-5b6a-4639-9023-115ce3d0e378
+-------------------+--------------------------------------+
| Field             | Value                                |
+-------------------+--------------------------------------+
| allocation_pools  | 172.31.254.2-172.31.254.254          |
| cidr              | 172.31.254.0/24                      |
| created_at        | 2020-01-15T17:13:39Z                 |
| description       |                                      |
| dns_nameservers   |                                      |
| enable_dhcp       | True                                 |
| gateway_ip        | 172.31.254.1                         |
| host_routes       |                                      |
| id                | 41c073c0-5b6a-4639-9023-115ce3d0e378 |
| ip_version        | 4                                    |
| ipv6_address_mode | None                                 |
| ipv6_ra_mode      | None                                 |
| name              | akaris-backend-subnet                |
| network_id        | aa68c208-b6c9-402e-b508-1b9d82f3baa4 |
| prefix_length     | None                                 |
| project_id        | ea78dddee3e240c7af1779b417c64618     |
| revision_number   | 1                                    |
| segment_id        | None                                 |
| service_types     |                                      |
| subnetpool_id     | None                                 |
| tags              |                                      |
| updated_at        | 2020-08-11T15:19:17Z                 |
+-------------------+--------------------------------------+
~~~

And create a router that can host floating IPs:
~~~
openstack router create akaris-backend
openstack router set --external-gateway 5cd089f9-8ed2-46bc-8ea7-4e1cdb5262ba akaris-backend
openstack router add subnet akaris-backend akaris-backend-subnet
~~~

~~~
[cloud-user@akaris-jump-server openshift]$ openstack port create --network aa68c208-b6c9-402e-b508-1b9d82f3baa4 akaris-osc-6klvk-worker-hp2hz-akaris-backend
+-----------------------+------------------------------------------------------------------------------+
| Field                 | Value                                                                        |
+-----------------------+------------------------------------------------------------------------------+
| admin_state_up        | UP                                                                           |
| allowed_address_pairs |                                                                              |
| binding_host_id       | None                                                                         |
| binding_profile       | None                                                                         |
| binding_vif_details   | None                                                                         |
| binding_vif_type      | None                                                                         |
| binding_vnic_type     | normal                                                                       |
| created_at            | 2020-08-11T14:47:17Z                                                         |
| data_plane_status     | None                                                                         |
| description           |                                                                              |
| device_id             |                                                                              |
| device_owner          |                                                                              |
| dns_assignment        | None                                                                         |
| dns_name              | None                                                                         |
| extra_dhcp_opts       |                                                                              |
| fixed_ips             | ip_address='172.31.254.48', subnet_id='41c073c0-5b6a-4639-9023-115ce3d0e378' |
| id                    | 8e737c31-0450-40d0-a5e0-54871a735428                                         |
| ip_address            | None                                                                         |
| mac_address           | fa:16:3e:07:66:12                                                            |
| name                  | akaris-osc-6klvk-worker-hp2hz-akaris-backend                                 |
| network_id            | aa68c208-b6c9-402e-b508-1b9d82f3baa4                                         |
| option_name           | None                                                                         |
| option_value          | None                                                                         |
| port_security_enabled | True                                                                         |
| project_id            | ea78dddee3e240c7af1779b417c64618                                             |
| qos_policy_id         | None                                                                         |
| revision_number       | 6                                                                            |
| security_group_ids    | ae5a722f-aeea-4acd-8260-a04f5dceb624                                         |
| status                | DOWN                                                                         |
| subnet_id             | None                                                                         |
| tags                  |                                                                              |
| trunk_details         | None                                                                         |
| updated_at            | 2020-08-11T14:47:17Z                                                         |
+-----------------------+------------------------------------------------------------------------------+
[cloud-user@akaris-jump-server openshift]$ openstack port create --network aa68c208-b6c9-402e-b508-1b9d82f3baa4 akaris-osc-6klvk-worker-zhs67-akaris-backend
+-----------------------+-------------------------------------------------------------------------------+
| Field                 | Value                                                                         |
+-----------------------+-------------------------------------------------------------------------------+
| admin_state_up        | UP                                                                            |
| allowed_address_pairs |                                                                               |
| binding_host_id       | None                                                                          |
| binding_profile       | None                                                                          |
| binding_vif_details   | None                                                                          |
| binding_vif_type      | None                                                                          |
| binding_vnic_type     | normal                                                                        |
| created_at            | 2020-08-11T14:47:40Z                                                          |
| data_plane_status     | None                                                                          |
| description           |                                                                               |
| device_id             |                                                                               |
| device_owner          |                                                                               |
| dns_assignment        | None                                                                          |
| dns_name              | None                                                                          |
| extra_dhcp_opts       |                                                                               |
| fixed_ips             | ip_address='172.31.254.207', subnet_id='41c073c0-5b6a-4639-9023-115ce3d0e378' |
| id                    | c82401f3-fa76-449e-ba16-75fe4b0f5583                                          |
| ip_address            | None                                                                          |
| mac_address           | fa:16:3e:66:d2:28                                                             |
| name                  | akaris-osc-6klvk-worker-zhs67-akaris-backend                                  |
| network_id            | aa68c208-b6c9-402e-b508-1b9d82f3baa4                                          |
| option_name           | None                                                                          |
| option_value          | None                                                                          |
| port_security_enabled | True                                                                          |
| project_id            | ea78dddee3e240c7af1779b417c64618                                              |
| qos_policy_id         | None                                                                          |
| revision_number       | 6                                                                             |
| security_group_ids    | ae5a722f-aeea-4acd-8260-a04f5dceb624                                          |
| status                | DOWN                                                                          |
| subnet_id             | None                                                                          |
| tags                  |                                                                               |
| trunk_details         | None                                                                          |
| updated_at            | 2020-08-11T14:47:40Z                                                          |
+-----------------------+-------------------------------------------------------------------------------+
[cloud-user@akaris-jump-server openshift]$ openstack port create --network aa68c208-b6c9-402e-b508-1b9d82f3baa4 vip-akaris-backend
+-----------------------+-------------------------------------------------------------------------------+
| Field                 | Value                                                                         |
+-----------------------+-------------------------------------------------------------------------------+
| admin_state_up        | UP                                                                            |
| allowed_address_pairs |                                                                               |
| binding_host_id       | None                                                                          |
| binding_profile       | None                                                                          |
| binding_vif_details   | None                                                                          |
| binding_vif_type      | None                                                                          |
| binding_vnic_type     | normal                                                                        |
| created_at            | 2020-08-11T18:32:53Z                                                          |
| data_plane_status     | None                                                                          |
| description           |                                                                               |
| device_id             |                                                                               |
| device_owner          |                                                                               |
| dns_assignment        | None                                                                          |
| dns_name              | None                                                                          |
| extra_dhcp_opts       |                                                                               |
| fixed_ips             | ip_address='172.31.254.118', subnet_id='41c073c0-5b6a-4639-9023-115ce3d0e378' |
| id                    | 4d0b32f5-203b-4e2b-b92b-b25f349b1d83                                          |
| ip_address            | None                                                                          |
| mac_address           | fa:16:3e:0d:3e:cd                                                             |
| name                  | vip-akaris-backend                                                            |
| network_id            | aa68c208-b6c9-402e-b508-1b9d82f3baa4                                          |
| option_name           | None                                                                          |
| option_value          | None                                                                          |
| port_security_enabled | True                                                                          |
| project_id            | ea78dddee3e240c7af1779b417c64618                                              |
| qos_policy_id         | None                                                                          |
| revision_number       | 6                                                                             |
| security_group_ids    | ae5a722f-aeea-4acd-8260-a04f5dceb624                                          |
| status                | DOWN                                                                          |
| subnet_id             | None                                                                          |
| tags                  |                                                                               |
| trunk_details         | None                                                                          |
| updated_at            | 2020-08-11T18:32:53Z                                                          |
+-----------------------+-------------------------------------------------------------------------------+
[cloud-user@akaris-jump-server openshift]$ openstack server add port akaris-osc-6klvk-worker-hp2hz akaris-osc-6klvk-worker-hp2hz-akaris-backend
[cloud-user@akaris-jump-server openshift]$ openstack server add port akaris-osc-6klvk-worker-zhs67 akaris-osc-6klvk-worker-zhs67-akaris-backend
[cloud-user@akaris-jump-server openshift]$ openstack floating ip set --port akaris-osc-6klvk-worker-hp2hz-akaris-backend 10.0.92.194
[cloud-user@akaris-jump-server openshift]$ openstack floating ip set --port akaris-osc-6klvk-worker-zhs67-akaris-backend 10.0.92.205
[cloud-user@akaris-jump-server openshift]$ openstack floating ip set --port vip-akaris-backend 10.0.89.89
~~~

After this, the floating IPs look like this in OpenStack:
~~~
[cloud-user@akaris-jump-server openshift]$ openstack floating ip list | grep 172.31.254
| dd055cc4-af00-4dbd-8e4a-94fac680299d | 10.0.89.89          | 172.31.254.118   | 4d0b32f5-203b-4e2b-b92b-b25f349b1d83 | 5cd089f9-8ed2-46bc-8ea7-4e1cdb5262ba | ea78dddee3e240c7af1779b417c64618 |
| e586c5c8-1cd4-4bd5-adb7-790e895b5af8 | 10.0.92.194         | 172.31.254.48    | 8e737c31-0450-40d0-a5e0-54871a735428 | 5cd089f9-8ed2-46bc-8ea7-4e1cdb5262ba | ea78dddee3e240c7af1779b417c64618 |
| f7e082f1-7880-4f07-8a05-3fdb0585eb32 | 10.0.92.205         | 172.31.254.207   | c82401f3-fa76-449e-ba16-75fe4b0f5583 | 5cd089f9-8ed2-46bc-8ea7-4e1cdb5262ba | ea78dddee3e240c7af1779b417c64618 |
~~~

Last but not least, disable port security for these ports so that VRRP can work with these OpenStack ports:
~~~
for port in akaris-osc-6klvk-worker-hp2hz-akaris-backend akaris-osc-6klvk-worker-zhs67-akaris-backend vip-akaris-backend ; do 
    echo === $port ===
    openstack port set --no-security-group $port
    openstack port set --disable-port-security $port
done
~~~

The VRRP floating IP is: 
   IP 172.31.254.118
   MAC address: fa:16:3e:0d:3e:cd
   VIP: 10.0.89.89

Verify that the port was added to the workers:
~~~
[cloud-user@akaris-jump-server openshift]$ oc debug node/akaris-osc-6klvk-worker-hp2hz
Starting pod/akaris-osc-6klvk-worker-hp2hz-debug ...
To use host binaries, run `chroot /host`
Pod IP: 192.168.1.160
If you don't see a command prompt, try pressing enter.
sh-4.2# chroot /host
sh-4.4# toolbox
Trying to pull registry.redhat.io/rhel8/support-tools...
Getting image source signatures
Copying blob 77c58f19bd6e done  
Copying blob 47db82df7f3f done  
Copying blob cdc5441bd52d done  
Copying config 5ef2aab094 done  
Writing manifest to image destination
Storing signatures
5ef2aab094514cc5561fa94b0bb52d75379ecf8a36355e891017f3982bac278c
Spawning a container 'toolbox-' with image 'registry.redhat.io/rhel8/support-tools'
Detected RUN label in the container image. Using that as the default...
command: podman run -it --name toolbox- --privileged --ipc=host --net=host --pid=host -e HOST=/host -e NAME=toolbox- -e IMAGE=registry.redhat.io/rhel8/support-tools:latest -v /run:/run -v /var/log:/var/log -v /etc/machine-id:/etc/machine-id -v /etc/localtime:/etc/localtime -v /:/host registry.redhat.io/rhel8/support-tools:latest
[root@akaris-osc-6klvk-worker-hp2hz /]# yum install iproute -y
(...)
Installed:
  iproute-5.3.0-1.el8.x86_64                                 libmnl-1.0.4-6.el8.x86_64                                

Complete!
[root@akaris-osc-6klvk-worker-hp2hz /]# dmesg | tail
[  745.078322] pci 0000:00:06.0: BAR 0: assigned [io  0x1000-0x103f]
[  745.080319] virtio-pci 0000:00:06.0: enabling device (0000 -> 0003)
[  745.100662] PCI Interrupt Link [LNKB] enabled at IRQ 11
[  745.109702] virtio_net virtio3 ens6: renamed from eth0
[  745.156833] IPv6: ADDRCONF(NETDEV_UP): ens6: link is not ready
[  745.159290] IPv6: ADDRCONF(NETDEV_UP): ens6: link is not ready
[  745.165677] IPv6: ADDRCONF(NETDEV_UP): ens6: link is not ready
[  745.172694] IPv6: ADDRCONF(NETDEV_UP): ens6: link is not ready
[  746.143079] IPv6: ADDRCONF(NETDEV_CHANGE): ens6: link becomes ready
[  861.279527] SELinux: mount invalid.  Same superblock, different security settings for (dev mqueue, type mqueue)
[root@akaris-osc-6klvk-worker-hp2hz /]# ip link ls dev ens6
10: ens6: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1450 qdisc fq_codel state UP mode DEFAULT group default qlen 1000
    link/ether fa:16:3e:07:66:12 brd ff:ff:ff:ff:ff:ff
~~~

Repeat the same on the other node:
~~~
[cloud-user@akaris-jump-server openshift]$ oc debug node/akaris-osc-6klvk-worker-zhs67
(...)
~~~

### Step 3: create new role and assign new nodes to the role ###

Follow https://www.redhat.com/en/blog/openshift-container-platform-4-how-does-machine-config-pool-work with modifications.

Run this via a script that's triggered by a systemd unit file:
~~~
cat <<'EOF' | oc apply -f -
apiVersion: machineconfiguration.openshift.io/v1
kind: MachineConfigPool
metadata:
  name: staging
spec:
  machineConfigSelector:
    matchExpressions:
      - {key: machineconfiguration.openshift.io/role, operator: In, values: [worker,staging]}
  nodeSelector:
    matchLabels:
      node-role.kubernetes.io/staging: ""
  paused: false
EOF

cat <<'FOE' > network-script.sh
#!/bin/bash

cat <<'EOF' > /etc/sysconfig/network-scripts/ifcfg-ens6
NAME="System ens6"
BOOTPROTO=dhcp
DEFROUTE=no
IPV6INIT=no
DEVICE=ens6
ONBOOT=yes
EOF

# none of the following will work given that we're using DHCP
# cat <<'EOF' > /etc/sysconfig/network-scripts/rule-ens6
# from 172.31.254.0/24 lookup 5000
# EOF

# cat <<'EOF' > /etc/sysconfig/network-scripts/route-ens6
# default via 172.31.254.1 table 5000
# EOF

# the following will not work either
# ip rule add from 172.31.254.0/24 lookup 5000
# ip route add default via 172.31.254.1 table 5000

cat <<'EOF' > /etc/NetworkManager/dispatcher.d/ifup-local
#!/bin/sh

interface="$1"
status="$2"

if [ "$interface" == "ens6" ] && [ "$status" == "up" ]; then
  logger "Interface $interface $status - setting ip rule and ip route"
  ip rule add from 172.31.254.0/24 lookup 5000
  ip route add default via 172.31.254.1 table 5000
fi
EOF
chmod +x /etc/NetworkManager/dispatcher.d/ifup-local
FOE

cat <<EOF | oc apply -f -
apiVersion: machineconfiguration.openshift.io/v1
kind: MachineConfig
metadata:
  creationTimestamp: null
  labels:
    machineconfiguration.openshift.io/role: staging
  name: 01-staging-additional-interface
spec:
  config:
    ignition:
      version: 2.2.0
    storage:
      files:
      - contents:
          source: data:;base64,$(cat network-script.sh | base64 -w 0)
        path: /usr/local/bin/network-script.sh
        filesystem: root
        mode: 0755
    systemd:
      units:
      - contents: "[Unit]\\nDescription=Configure network\\nAfter=ignition-firstboot-complete.service\\nBefore=nodeip-configuration.service kubelet.service crio.service\\n\\n[Service]\\nType=oneshot\\nExecStart=/usr/local/bin/network-script.sh\\n\\n[Install]\\nWantedBy=multi-user.target"
        enabled: true
        name: network-script.service
EOF

rm -f network-script.sh
~~~

Label the nodes as staging:
~~~
oc label node akaris-osc-6klvk-worker-hp2hz node-role.kubernetes.io/worker- node-role.kubernetes.io/staging=
oc label node akaris-osc-6klvk-worker-zhs67 node-role.kubernetes.io/worker- node-role.kubernetes.io/staging=
~~~

The nodes will now reboot with the new setting, so wait:
~~~
[cloud-user@akaris-jump-server openshift]$ oc get nodes
NAME                            STATUS                        ROLES     AGE     VERSION
akaris-osc-6klvk-master-0       Ready                         master    5h48m   v1.18.3+012b3ec
akaris-osc-6klvk-master-1       Ready                         master    5h48m   v1.18.3+012b3ec
akaris-osc-6klvk-master-2       Ready                         master    5h49m   v1.18.3+012b3ec
akaris-osc-6klvk-worker-g4q6z   Ready                         worker    5h34m   v1.18.3+012b3ec
akaris-osc-6klvk-worker-hp2hz   Ready                         staging   32m     v1.18.3+012b3ec
akaris-osc-6klvk-worker-l726s   Ready                         worker    5h31m   v1.18.3+012b3ec
akaris-osc-6klvk-worker-lp2df   Ready                         worker    5h36m   v1.18.3+012b3ec
akaris-osc-6klvk-worker-zhs67   NotReady,SchedulingDisabled   staging   32m     v1.18.3+012b3ec
[cloud-user@akaris-jump-server openshift]$ 
~~~

Make sure that the interface was created:
~~~
[cloud-user@akaris-jump-server openshift]$ oc debug node/akaris-osc-6klvk-worker-zhs67
Starting pod/akaris-osc-6klvk-worker-zhs67-debug ...
To use host binaries, run `chroot /host`
Pod IP: 192.168.2.48
If you don't see a command prompt, try pressing enter.
sh-4.4# cat /etc/sysconfig/network-scripts/ifcfg-ens6 
NAME="System ens6"
BOOTPROTO=dhcp
DEFROUTE=no
IPV6INIT=no
DEVICE=ens6
ONBOOT=yes
TYPE=Ethernet
PROXY_METHOD=none
BROWSER_ONLY=no
ETHTOOL_OPTS="-K ens6 tx-checksum-ip-generic off"
IPV4_FAILURE_FATAL=no
UUID=9325ba04-7907-b4a4-1414-177267ba3519
sh-4.2# chroot /host
sh-4.4# ip a ls dev ens6
3: ens6: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1450 qdisc fq_codel state UP group default qlen 1000
    link/ether fa:16:3e:66:d2:28 brd ff:ff:ff:ff:ff:ff
    inet 172.31.254.207/24 brd 172.31.254.255 scope global dynamic noprefixroute ens6
       valid_lft 86276sec preferred_lft 86276sec
    inet6 fe80::f816:3eff:fe66:d228/64 scope link 
       valid_lft forever preferred_lft forever
sh-4.4# ip route ls table 5000
default via 172.31.254.1 dev ens6 
sh-4.4# ip rule ls | grep 5000
32765:	from 172.31.254.0/24 lookup 5000
~~~

Run the same verification on the other worker.

Ping both floating IPs and make sure that policy routing works:
~~~
[cloud-user@akaris-jump-server openshift]$ ping 10.0.92.194
PING 10.0.92.194 (10.0.92.194) 56(84) bytes of data.
64 bytes from 10.0.92.194: icmp_seq=1 ttl=62 time=13.8 ms
^C
--- 10.0.92.194 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 13.897/13.897/13.897/0.000 ms
[cloud-user@akaris-jump-server openshift]$ ping 10.0.92.205
PING 10.0.92.205 (10.0.92.205) 56(84) bytes of data.
64 bytes from 10.0.92.205: icmp_seq=1 ttl=62 time=1.24 ms
^C
--- 10.0.92.205 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 1.242/1.242/1.242/0.000 ms
~~~

### Step 4: Configure keepalived template for the staging node ###
~~~
cat <<'EOF' > keepalived.conf.tmpl
# TODO: Improve this check. The port is assumed to be alive.
# Need to assess what is the ramification if the port is not there.
vrrp_script chk_ingress {
    script "/usr/bin/curl -o /dev/null -kLfs http://0:1936/healthz"
    interval 1
    weight 50
}
vrrp_instance staging_INGRESS {
    state BACKUP
    interface ens6
    virtual_router_id 100
    priority 40
    advert_int 1
    authentication {
        auth_type PASS
        auth_pass staging_vip
    }
    virtual_ipaddress {
        172.31.254.118/24
    }
    track_script {
        chk_ingress
    }
}
EOF

cat <<EOF | oc apply -f -
apiVersion: machineconfiguration.openshift.io/v1
kind: MachineConfig
metadata:
  labels:
    machineconfiguration.openshift.io/role: staging
  name: 99-staging-keepalived-vip
spec:
  config:
    ignition:
      config: {}
      security:
        tls: {}
      timeouts: {}
      version: 2.2.0
    networkd: {}
    passwd: {}
    storage:
      files:
      - contents:
          source: data:text/plain;charset=utf-8;base64,$(cat keepalived.conf.tmpl | base64 -w0)
          verification: {}
        filesystem: root
        mode: 420
        path: /etc/kubernetes/static-pod-resources/keepalived/keepalived.conf.tmpl
    systemd: {}
EOF

rm -f  keepalived.conf.tmpl
~~~

Verify:
~~~
[root@akaris-osc-6klvk-worker-zhs67 ~]# cat /etc/keepalived/keepalived.conf 
# TODO: Improve this check. The port is assumed to be alive.
# Need to assess what is the ramification if the port is not there.
vrrp_script chk_ingress {
    script "/usr/bin/curl -o /dev/null -kLfs http://0:1936/healthz"
    interval 1
    weight 50
}
vrrp_instance staging_INGRESS {
    state BACKUP
    interface ens6
    virtual_router_id 100
    priority 40
    advert_int 1
    authentication {
        auth_type PASS
        auth_pass staging_vip
    }
    virtual_ipaddress {
        172.31.254.118/24
    }
    track_script {
        chk_ingress
    }
}
[root@akaris-osc-6klvk-worker-zhs67 ~]# ip a | gre^C
[root@akaris-osc-6klvk-worker-zhs67 ~]# ip a ls dev ens6
3: ens6: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1450 qdisc fq_codel state UP group default qlen 1000
    link/ether fa:16:3e:66:d2:28 brd ff:ff:ff:ff:ff:ff
    inet 172.31.254.207/24 brd 172.31.254.255 scope global dynamic noprefixroute ens6
       valid_lft 86337sec preferred_lft 86337sec
    inet 172.31.254.118/24 scope global secondary ens6
       valid_lft forever preferred_lft forever
    inet6 fe80::f816:3eff:fe66:d228/64 scope link 
       valid_lft forever preferred_lft forever
~~~

And ping the VIP:
~~~
[cloud-user@akaris-jump-server ~]$ ping 10.0.89.89
PING 10.0.89.89 (10.0.89.89) 56(84) bytes of data.
64 bytes from 10.0.89.89: icmp_seq=1 ttl=62 time=15.7 ms
^C
--- 10.0.89.89 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 15.791/15.791/15.791/0.000 ms
[cloud-user@akaris-jump-server ~]$ 
~~~

And SSH into the VIP:
~~~
[cloud-user@akaris-jump-server ~]$ ssh core@10.0.89.89
The authenticity of host '10.0.89.89 (10.0.89.89)' can't be established.
ECDSA key fingerprint is SHA256:XB+FYE3vAxFZNI4HakM5WJkHmf74iMdFgI8x+dLx03A.
ECDSA key fingerprint is MD5:a2:54:c1:52:ee:05:67:52:7c:2a:a9:b8:35:a9:a5:ee.
Are you sure you want to continue connecting (yes/no)? yes
Warning: Permanently added '10.0.89.89' (ECDSA) to the list of known hosts.
Red Hat Enterprise Linux CoreOS 45.82.202007240629-0
  Part of OpenShift 4.5, RHCOS is a Kubernetes native operating system
  managed by the Machine Config Operator (`clusteroperator/machine-config`).

WARNING: Direct SSH access to machines is not recommended; instead,
make configuration changes via `machineconfig` objects:
  https://docs.openshift.com/container-platform/4.5/architecture/architecture-rhcos.html

---
Last login: Tue Aug 11 19:14:19 2020 from 10.0.88.55
[core@akaris-osc-6klvk-worker-zhs67 ~]$ 
~~~

### Step 5: Configure an Ingress, use Ingress controller sharding ###

Use:
https://docs.openshift.com/container-platform/4.5/networking/ingress-operator.html#nw-ingress-sharding_configuring-ingress

For example, the following Ingress route should work and be served by the 2 `staging` workers:
~~~
10.0.89.99 test.staging.akaris-osc....
~~~

Configure the default IngressController and have it run only on `worker` nodes - you can also exclude namespaces which shall not be served by the default IngressController:
~~~
cat <<'EOF' > patch.yaml
spec:
  replicas: 2
#  namespaceSelector:
#    matchExpressions:
#    - key: type
#      operator: NotIn
#      values:
#      - test1
#      - test2
  nodePlacement:
    nodeSelector:
      matchLabels:
        node-role.kubernetes.io/worker: ""
EOF
oc patch -n openshift-ingress-operator ingresscontroller default --type="merge" -p "$(cat patch.yaml)"
rm -f patch.yaml
~~~

Configure the staging IngressController and have it run only on `sharded` nodes:
~~~
cat <<'EOF' | oc apply -f -
apiVersion: v1
items:
- apiVersion: operator.openshift.io/v1
  kind: IngressController
  metadata:
    name: staging
    namespace: openshift-ingress-operator
  spec:
    replicas: 2
    domain: staging.akaris-osc....
    nodePlacement:
      nodeSelector:
        matchLabels:
          node-role.kubernetes.io/staging: ""
# --- either ---
#    routeSelector:
#      matchLabels:
#        type: staging
# --- or ---
#    namespaceSelector:
#      matchLabels:
#        type: test1
    endpointPublishingStrategy:
      type: HostNetwork
  status: {}
kind: List
metadata:
  resourceVersion: ""
  selfLink: ""
EOF
~~~

Verify generation and modification of the IngressController and OpenShift router pods:
~~~
[cloud-user@akaris-jump-server openshift]$ oc get nodes
NAME                            STATUS   ROLES     AGE   VERSION
akaris-osc-6klvk-master-0       Ready    master    24h   v1.18.3+012b3ec
akaris-osc-6klvk-master-1       Ready    master    24h   v1.18.3+012b3ec
akaris-osc-6klvk-master-2       Ready    master    24h   v1.18.3+012b3ec
akaris-osc-6klvk-worker-g4q6z   Ready    worker    24h   v1.18.3+012b3ec
akaris-osc-6klvk-worker-hp2hz   Ready    staging   19h   v1.18.3+012b3ec
akaris-osc-6klvk-worker-l726s   Ready    worker    24h   v1.18.3+012b3ec
akaris-osc-6klvk-worker-lp2df   Ready    worker    24h   v1.18.3+012b3ec
akaris-osc-6klvk-worker-zhs67   Ready    staging   19h   v1.18.3+012b3ec
[cloud-user@akaris-jump-server openshift]$ oc get ingresscontroller -n openshift-ingress-operator
NAME      AGE
default   24h
staging   13m
[cloud-user@akaris-jump-server openshift]$ oc get pods -n openshift-ingress -o wide
NAME                              READY   STATUS    RESTARTS   AGE    IP              NODE                            NOMINATED NODE   READINESS GATES
router-default-7fd7bdd4fc-jf55h   1/1     Running   0          2m5s   192.168.0.114   akaris-osc-6klvk-worker-lp2df   <none>           <none>
router-default-7fd7bdd4fc-mc572   1/1     Running   0          106s   192.168.0.251   akaris-osc-6klvk-worker-l726s   <none>           <none>
router-staging-86c864549c-7stx8   1/1     Running   0          14m    192.168.1.160   akaris-osc-6klvk-worker-hp2hz   <none>           <none>
router-staging-86c864549c-nkz28   1/1     Running   0          14m    192.168.2.48    akaris-osc-6klvk-worker-zhs67   <none>           <none>
~~~

### Step 6: Expose route ###

~~~
oc new-project test
oc adm policy add-scc-to-user anyuid -z default
~~~

~~~
cat <<'EOF' > httpbin-ingress.yaml
apiVersion: extensions/v1beta1
kind: Ingress
metadata:
  name: httpbin-ingress
  annotations:
    kubernetes.io/ingress.allow-http: "true"
spec:
  rules:
  - host: httpbin-ingress.staging.akaris-osc....
    http:
      paths:
      - path: /
        backend:
          serviceName: httpbin-service
          servicePort: 80
---
apiVersion: v1
kind: Service
metadata:
  name: httpbin-service
  labels:
    app: httpbin-deployment
spec:
  selector:
    app: httpbin-pod
  ports:
    - protocol: TCP
      port: 80
      targetPort: 80
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: httpbin-deployment
  labels:
    app: httpbin-deployment
spec:
  replicas: 1
  selector:
    matchLabels:
      app: httpbin-pod
  template:
    metadata:
      labels:
        app: httpbin-pod
    spec:
      containers:
      - name: tshark
        image: danielguerra/alpine-tshark 
        command:
          - "tshark" 
          - "-i" 
          - "eth0" 
          - "-Y" 
          - "http" 
          - "-V"  
          - "dst" 
          - "port"
          - "80"
      - name: httpbin
        image: kennethreitz/httpbin
        imagePullPolicy: Always
        command:
        - "gunicorn"
        - "-b"
        - "0.0.0.0:80"
        - "httpbin:app"
        - "-k"
        - "gevent"
        - "--capture-output"
        - "--error-logfile"
        - "-"
        - "--access-logfile"
        - "-"
        - "--access-logformat"
        - "'%(h)s %(t)s %(r)s %(s)s Host: %({Host}i)s} Header-i: %({Header}i)s Header-o: %({Header}o)s'"
EOF
oc apply -f httpbin-ingress.yaml
~~~

~~~
[cloud-user@akaris-jump-server ~]$ oc get routes 
NAME                    HOST/PORT                                                                                 PATH   SERVICES          PORT    TERMINATION   WILDCARD
httpbin-ingress-z9tgf   httpbin-ingress.staging.akaris-osc.... ... 1 more   /      httpbin-service   <all>                 None
[cloud-user@akaris-jump-server ~]$ curl httpbin-ingress.staging.akaris-osc....
{
  "args": {}, 
  "headers": {
    "Accept": "*/*", 
    "Forwarded": "for=10.0.88.55;host=httpbin-ingress.staging.akaris-osc....;proto=http", 
    "Host": "httpbin-ingress.staging.akaris-osc....", 
    "User-Agent": "curl/7.29.0", 
    "X-Forwarded-Host": "httpbin-ingress.staging.akaris-osc...."
  }, 
  "origin": "10.0.88.55", 
  "url": "http://httpbin-ingress.staging.akaris-osc..../get"
}
~~~

### Step 7: Security hardening ###

This is out of scope of this PoC, but note that at the moment, the default Ingress can still handle all domains and thus if the client configures its resolver appropriately, can reach the application via the default VIP.

For example, take this mapping:
~~~
10.0.88.28 *.apps.akaris-osc....
10.0.89.89 *.staging.akaris-osc....
~~~

Then one can still use the other ingress router by modifying the client resolver:
~~~
[cloud-user@akaris-jump-server ~]$ curl --resolve httpbin-ingress.staging.akaris-osc..../get:80:10.0.88.28 httpbin-ingress.staging.akaris-osc..../get
{
  "args": {}, 
  "headers": {
    "Accept": "*/*", 
    "Forwarded": "for=10.0.88.55;host=httpbin-ingress.staging.akaris-osc....;proto=http", 
    "Host": "httpbin-ingress.staging.akaris-osc...., 
    "User-Agent": "curl/7.29.0", 
    "X-Forwarded-Host": "httpbin-ingress.staging.akaris-osc..."
  }, 
  "origin": "10.0.88.55", 
  "url": "http://httpbin-ingress.staging.akaris-osc..../get"
}
[cloud-user@akaris-jump-server ~]$ curl --resolve httpbin-ingress.staging.akaris-osc..../get:80:10.0.89.89 httpbin-ingress.staging.akaris-osc.../get
{
  "args": {}, 
  "headers": {
    "Accept": "*/*", 
    "Forwarded": "for=10.0.88.55;host=httpbin-ingress.staging.akaris-osc....;proto=http", 
    "Host": "httpbin-ingress.staging.akaris-osc...", 
    "User-Agent": "curl/7.29.0", 
    "X-Forwarded-Host": "httpbin-ingress.staging.akaris-osc..."
  }, 
  "origin": "10.0.88.55", 
  "url": "http://httpbin-ingress.staging.akaris-osc.../get"
}
~~~

And the same goes for the other way around:
~~~
[cloud-user@akaris-jump-server ~]$ curl --resolve nginx-deployment-application.apps.akaris-osc....:80:10.0.88.28 nginx-deployment-application.apps.akaris-osc....
Nginx A
[cloud-user@akaris-jump-server ~]$ curl --resolve nginx-deployment-application.apps.akaris-osc....:80:10.0.89.89 nginx-deployment-application.apps.akaris-osc....
Nginx A
~~~

Read the documentation about IngressController sharding for further details.

### Step 8: Troubleshooting ###

Again, a bit outside of the scope of this PoC - the debug pods for my `staging` nodes stopped working overnight. This would need some further troubleshooting:
~~~
[cloud-user@akaris-jump-server ~]$ oc get nodes
NAME                            STATUS   ROLES     AGE   VERSION
akaris-osc-6klvk-master-0       Ready    master    25h   v1.18.3+012b3ec
akaris-osc-6klvk-master-1       Ready    master    25h   v1.18.3+012b3ec
akaris-osc-6klvk-master-2       Ready    master    25h   v1.18.3+012b3ec
akaris-osc-6klvk-worker-g4q6z   Ready    worker    24h   v1.18.3+012b3ec
akaris-osc-6klvk-worker-hp2hz   Ready    staging   19h   v1.18.3+012b3ec
akaris-osc-6klvk-worker-l726s   Ready    worker    24h   v1.18.3+012b3ec
akaris-osc-6klvk-worker-lp2df   Ready    worker    24h   v1.18.3+012b3ec
akaris-osc-6klvk-worker-zhs67   Ready    staging   19h   v1.18.3+012b3ec
~~~

~~~
[cloud-user@akaris-jump-server ~]$ oc debug node/akaris-osc-6klvk-worker-zhs67
Starting pod/akaris-osc-6klvk-worker-zhs67-debug ...
To use host binaries, run `chroot /host`
Pod IP: 192.168.2.48
If you don't see a command prompt, try pressing enter.

Removing debug pod ...
Error from server: error dialing backend: remote error: tls: internal error
[cloud-user@akaris-jump-server ~]$ 
~~~
