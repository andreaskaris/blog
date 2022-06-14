# OVN standalone on Fedora  #

For instruction to build OVN and OVS from source, see: https://docs.ovn.org/en/latest/intro/install/fedora.html#fedora-rhel-7-x-packaging-for-ovn
Otherwise, use the RPMs that ship with the Fedora repos.

## Base setup ##

### Single node DB on ovn1 ###

#### Configuration and installation ####

The following starts an OVN cluster on Fedora 31 with 3 nodes. Non-clustered DB with ovn1 as the DB node, and ovn2 and ovn3 join ovn1's DB.

All nodes have 2 interfaces. One management interface on eth0. DNS names ovn1, ovn2, ovn3 map to the IP addresses on eth0. Interface eth1 has no IP addresses and will be used for the dataplane (attached to br-provider).

IP address configuration:
~~~
[root@ovn1 ~]# cat /etc/sysconfig/network-scripts/ifcfg-eth0 
DEVICE="eth0"
BOOTPROTO="static"
ONBOOT="yes"
TYPE="Ethernet"
PREFIX=24
IPADDR=192.168.122.201
GATEWAY=192.168.122.1
DNS1=192.168.122.1
~~~

~~~
[root@ovn2 ~]# cat /etc/sysconfig/network-scripts/ifcfg-eth0 
DEVICE="eth0"
BOOTPROTO="static"
ONBOOT="yes"
TYPE="Ethernet"
PREFIX=24
IPADDR=192.168.122.202
GATEWAY=192.168.122.1
DNS1=192.168.122.1
~~~

~~~
[root@ovn3 ~]# cat /etc/sysconfig/network-scripts/ifcfg-eth0 
DEVICE="eth0"
BOOTPROTO="static"
ONBOOT="yes"
TYPE="Ethernet"
PREFIX=24
IPADDR=192.168.122.203
GATEWAY=192.168.122.1
DNS1=192.168.122.1
~~~

Hostname to IP address mapping:
~~~
# cat /etc/hosts
127.0.0.1   localhost localhost.localdomain localhost4 localhost4.localdomain4
::1         localhost localhost.localdomain localhost6 localhost6.localdomain6

192.168.122.201  ovn1  # eth0
192.168.122.202  ovn2  # eth0
192.168.122.203  ovn3  # eth0
~~~

Disable NM control over eth1 on all hosts:
~~~
echo "NM_CONTROLLED=no" > /etc/sysconfig/network-scripts/ifcfg-eth1
~~~

Install Open vSwitch and OVN on all nodes:
~~~
yum install ovn -y
yum install ovn-central -y
yum install ovn-host -y
~~~

Enable Open vSwitch and ovn-controller on all hosts and start right away:
~~~
systemctl enable --now openvswitch
systemctl enable --now ovn-controller
~~~

On the master nodes that are to hold the ovn-northd "control plane", execute:
~~~
echo 'OVN_NORTHD_OPTS="--db-nb-addr=ovn1 --db-nb-create-insecure-remote=yes --db-sb-addr=ovn1  --db-sb-create-insecure-remote=yes  --db-nb-cluster-local-addr=ovn1 --db-sb-cluster-local-addr=ovn1 --ovn-northd-nb-db=tcp:ovn1:6641 --ovn-northd-sb-db=tcp:ovn1:6642"' >> /etc/sysconfig/ovn
systemctl enable --now ovn-northd
~~~

Note that the above will start a single node cluster. The OVN man page contains an example for a full 3 node clustered DB: [http://www.openvswitch.org/support/dist-docs/ovn-ctl.8.txt](http://www.openvswitch.org/support/dist-docs/ovn-ctl.8.txt)

Now, configure OVS to connect to the OVN DBs. Note that geneve / vxlan tunnel needs IP addresses, DNS entries do not work:

On master node ovn1:
~~~
# [root@ovn1 ~]# 
ovs-vsctl set open . external-ids:ovn-remote=tcp:ovn1:6642
ovs-vsctl set open . external-ids:ovn-encap-type=geneve
# could also be: ovs-vsctl set open . external-ids:ovn-encap-type=geneve,vxlan
ovs-vsctl set open . external-ids:ovn-encap-ip=192.168.122.201
ovs-vsctl set open . external-ids:ovn-bridge=br-int
~~~

On slave node ovn2:
~~~
# [root@ovn2 ~]# 
ovs-vsctl set open . external-ids:ovn-remote=tcp:ovn1:6642
ovs-vsctl set open . external-ids:ovn-encap-ip=192.168.122.202
ovs-vsctl set open . external-ids:ovn-encap-type=geneve
# could also be: ovs-vsctl set open . external-ids:ovn-encap-type=geneve,vxlan
ovs-vsctl set open . external-ids:ovn-bridge=br-int
~~~

On slave node ovn3:
~~~
# [root@ovn3 ~]# 
ovs-vsctl set open . external-ids:ovn-remote=tcp:ovn1:6642
ovs-vsctl set open . external-ids:ovn-encap-ip=192.168.122.203
ovs-vsctl set open . external-ids:ovn-encap-type=geneve
# could also be: ovs-vsctl set open . external-ids:ovn-encap-type=geneve,vxlan
ovs-vsctl set open . external-ids:ovn-bridge=br-int
~~~

#### Verification ####

Verify on ovn1:
~~~
ovs-vsctl show
ovn-sbctl show
~~~

ovn2:
~~~
ovs-vsctl show
timeout 2 ovn-sbctl show
timeout 2 ovn-nbctl show
~~~

ovn3:
~~~
ovs-vsctl show
timeout 2 ovn-sbctl show
timeout 2 ovn-nbctl show
~~~

~~~
[root@ovn1 ~]# ovs-vsctl show
64003a9a-1f2a-403c-8d21-cd187d2f717c
    Bridge br-int
        fail_mode: secure
        Port ovn-b5337e-0
            Interface ovn-b5337e-0
                type: geneve
                options: {csum="true", key=flow, remote_ip="192.168.122.203"}
        Port br-int
            Interface br-int
                type: internal
        Port ovn-8b9ad2-0
            Interface ovn-8b9ad2-0
                type: geneve
                options: {csum="true", key=flow, remote_ip="192.168.122.202"}
    ovs_version: "2.13.0"
[root@ovn1 ~]# ovn-sbctl show
Chassis "b5337e05-2495-4bf0-8728-25840472baa4"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.203"
        options: {csum="true"}
Chassis "c3f90802-4fd5-44ad-a338-154a9150e46f"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.201"
        options: {csum="true"}
Chassis "8b9ad2e3-c1bc-4ea2-973e-a8bd1d38e502"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.202"
        options: {csum="true"}
~~~

~~~
[root@ovn2 ~]# ovs-vsctl show
33e3d97e-ee6a-4eeb-ac2a-188744895be7
    Bridge br-int
        fail_mode: secure
        Port ovn-c3f908-0
            Interface ovn-c3f908-0
                type: geneve
                options: {csum="true", key=flow, remote_ip="192.168.122.201"}
        Port br-int
            Interface br-int
                type: internal
        Port ovn-b5337e-0
            Interface ovn-b5337e-0
                type: geneve
                options: {csum="true", key=flow, remote_ip="192.168.122.203"}
    ovs_version: "2.13.0"
[root@ovn2 ~]# timeout 2 ovn-sbctl show

2020-07-21T11:13:15Z|00001|fatal_signal|WARN|terminating with signal 15 (Terminated)
[root@ovn2 ~]# timeout 2 ovn-nbctl show
ovn-nbctl: unix:/var/run/ovn/ovnnb_db.sock: database connection failed (No such file or directory)
[root@ovn2 ~]# 
~~~

~~~
[root@ovn3 ~]# ovs-vsctl show
a03dc1d7-a6f7-4400-b696-59ccda197a19
    Bridge br-int
        fail_mode: secure
        Port ovn-c3f908-0
            Interface ovn-c3f908-0
                type: geneve
                options: {csum="true", key=flow, remote_ip="192.168.122.201"}
        Port br-int
            Interface br-int
                type: internal
        Port ovn-8b9ad2-0
            Interface ovn-8b9ad2-0
                type: geneve
                options: {csum="true", key=flow, remote_ip="192.168.122.202"}
    ovs_version: "2.13.0"
[root@ovn3 ~]# timeout 2 ovn-sbctl show

2020-07-21T11:13:21Z|00001|fatal_signal|WARN|terminating with signal 15 (Terminated)
[root@ovn3 ~]# timeout 2 ovn-nbctl show
ovn-nbctl: unix:/var/run/ovn/ovnnb_db.sock: database connection failed (No such file or directory)
[root@ovn3 ~]# 
~~~

## Adding virtual network with one virtual switch s0 ##

On ovn1, configure a logical switch and port:
~~~
ovn-nbctl ls-add s0
ovn-nbctl lsp-add s0 port01
ovn-nbctl lsp-set-addresses port01 00:00:00:00:00:01
ovn-nbctl lsp-add s0 port02
ovn-nbctl lsp-set-addresses port02 00:00:00:00:00:02
~~~

Now, "real" ports need to be wired to the above ports. Note that the logical port name has to match the `external_ids:iface-id` identifier. If we added  `ovn-nbctl lsp-add s0 foo` instead of `port0`, then we would have to set `ovs-vsctl set Interface port0 external_ids:iface-id=foo` on ovn2. Note that we need to know the MAC address of that port. Therefore, when creating the veth, we are making sure to create it with the correct MAC address.

On ovn2, execute:
~~~
ip link add name veth01 type veth peer name port01
ip netns add ns0
ip link set dev veth01 netns ns0
ip netns exec ns0 ip link set dev lo up
ip netns exec ns0 ip link set dev veth01 up
ip netns exec ns0 ip link set veth01 address 00:00:00:00:00:01
ip netns exec ns0 ip address add 10.0.0.1/24 dev veth01
ip link set dev port01 up
ovs-vsctl add-port br-int port01
ovs-vsctl set Interface port01 external_ids:iface-id=port01
~~~

On ovn3, execute:
~~~
ip link add name veth02 type veth peer name port02
ip netns add ns0
ip link set dev veth02 netns ns0
ip netns exec ns0 ip link set dev lo up
ip netns exec ns0 ip link set dev veth02 up
ip netns exec ns0 ip link set veth02 address 00:00:00:00:00:02
ip netns exec ns0 ip address add 10.0.0.2/24 dev veth02
ip link set dev port02 up
ovs-vsctl add-port br-int port02 external_ids:iface-id=port02
ovs-vsctl set Interface port02 external_ids:iface-id=port02
~~~

Verify the new configuration on ovn1:
~~~
ovn-nbctl show
ovn-sbctl show
~~~

~~~
[root@ovn1 ~]# ovn-nbctl show
switch 40f56f4d-db18-4d77-bb8b-c93e5179d346 (s0)
    port port02
        addresses: ["00:00:00:00:00:02"]
    port port01
        addresses: ["00:00:00:00:00:01"]
[root@ovn1 ~]# ovn-sbctl show
Chassis "b5337e05-2495-4bf0-8728-25840472baa4"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.203"
        options: {csum="true"}
    Port_Binding port02
Chassis "c3f90802-4fd5-44ad-a338-154a9150e46f"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.201"
        options: {csum="true"}
Chassis "8b9ad2e3-c1bc-4ea2-973e-a8bd1d38e502"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.202"
        options: {csum="true"}
    Port_Binding port01
~~~
> **Note:** The southbound database now shows port bindings.

Verify logical flows:
~~~
ovn-sbctl lflow-list]# 
~~~

~~~
[root@ovn1 ~]# ovn-sbctl lflow-list
Datapath: "s0" (63c97e9f-194e-49b1-b075-661cd2132895)  Pipeline: ingress
  table=0 (ls_in_port_sec_l2  ), priority=100  , match=(eth.src[40]), action=(drop;)
  table=0 (ls_in_port_sec_l2  ), priority=100  , match=(vlan.present), action=(drop;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "port01"), action=(next;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "port02"), action=(next;)
  table=1 (ls_in_port_sec_ip  ), priority=0    , match=(1), action=(next;)
  table=2 (ls_in_port_sec_nd  ), priority=0    , match=(1), action=(next;)
  table=3 (ls_in_pre_acl      ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=3 (ls_in_pre_acl      ), priority=0    , match=(1), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=110  , match=(nd || nd_rs || nd_ra), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=0    , match=(1), action=(next;)
  table=5 (ls_in_pre_stateful ), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=5 (ls_in_pre_stateful ), priority=0    , match=(1), action=(next;)
  table=6 (ls_in_acl          ), priority=34000, match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=6 (ls_in_acl          ), priority=0    , match=(1), action=(next;)
  table=7 (ls_in_qos_mark     ), priority=0    , match=(1), action=(next;)
  table=8 (ls_in_qos_meter    ), priority=0    , match=(1), action=(next;)
  table=9 (ls_in_lb           ), priority=0    , match=(1), action=(next;)
  table=10(ls_in_stateful     ), priority=100  , match=(reg0[1] == 1), action=(ct_commit(ct_label=0/1); next;)
  table=10(ls_in_stateful     ), priority=100  , match=(reg0[2] == 1), action=(ct_lb;)
  table=10(ls_in_stateful     ), priority=0    , match=(1), action=(next;)
  table=11(ls_in_pre_hairpin  ), priority=0    , match=(1), action=(next;)
  table=12(ls_in_hairpin      ), priority=1    , match=(reg0[6] == 1), action=(eth.dst <-> eth.src;outport = inport;flags.loopback = 1;output;)
  table=12(ls_in_hairpin      ), priority=0    , match=(1), action=(next;)
  table=13(ls_in_arp_rsp      ), priority=0    , match=(1), action=(next;)
  table=14(ls_in_dhcp_options ), priority=0    , match=(1), action=(next;)
  table=15(ls_in_dhcp_response), priority=0    , match=(1), action=(next;)
  table=16(ls_in_dns_lookup   ), priority=0    , match=(1), action=(next;)
  table=17(ls_in_dns_response ), priority=0    , match=(1), action=(next;)
  table=18(ls_in_external_port), priority=0    , match=(1), action=(next;)
  table=19(ls_in_l2_lkup      ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(handle_svc_check(inport);)
  table=19(ls_in_l2_lkup      ), priority=70   , match=(eth.mcast), action=(outport = "_MC_flood"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:01), action=(outport = "port01"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:02), action=(outport = "port02"; output;)
Datapath: "s0" (63c97e9f-194e-49b1-b075-661cd2132895)  Pipeline: egress
  table=0 (ls_out_pre_lb      ), priority=110  , match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=0 (ls_out_pre_lb      ), priority=110  , match=(nd || nd_rs || nd_ra), action=(next;)
  table=0 (ls_out_pre_lb      ), priority=0    , match=(1), action=(next;)
  table=1 (ls_out_pre_acl     ), priority=110  , match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=1 (ls_out_pre_acl     ), priority=0    , match=(1), action=(next;)
  table=2 (ls_out_pre_stateful), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=2 (ls_out_pre_stateful), priority=0    , match=(1), action=(next;)
  table=3 (ls_out_lb          ), priority=0    , match=(1), action=(next;)
  table=4 (ls_out_acl         ), priority=34000, match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=4 (ls_out_acl         ), priority=0    , match=(1), action=(next;)
  table=5 (ls_out_qos_mark    ), priority=0    , match=(1), action=(next;)
  table=6 (ls_out_qos_meter   ), priority=0    , match=(1), action=(next;)
  table=7 (ls_out_stateful    ), priority=100  , match=(reg0[1] == 1), action=(ct_commit(ct_label=0/1); next;)
  table=7 (ls_out_stateful    ), priority=100  , match=(reg0[2] == 1), action=(ct_lb;)
  table=7 (ls_out_stateful    ), priority=0    , match=(1), action=(next;)
  table=8 (ls_out_port_sec_ip ), priority=0    , match=(1), action=(next;)
  table=9 (ls_out_port_sec_l2 ), priority=100  , match=(eth.mcast), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "port01"), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "port02"), action=(output;)
~~~

Last but not least, run a test ping between the namespaces on both hosts:
~~~
[root@ovn2 ~]# ip netns exec ns0 ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
8: veth01@if7: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 00:00:00:00:00:01 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 10.0.0.1/24 scope global veth01
       valid_lft forever preferred_lft forever
    inet6 fe80::200:ff:fe00:1/64 scope link 
       valid_lft forever preferred_lft forever
[root@ovn2 ~]# ip netns exec ns0 ping 10.0.0.2 -c1 -W1
PING 10.0.0.2 (10.0.0.2) 56(84) bytes of data.
64 bytes from 10.0.0.2: icmp_seq=1 ttl=64 time=1.41 ms

--- 10.0.0.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 1.413/1.413/1.413/0.000 ms
[root@ovn2 ~]# 
~~~

~~~
[root@ovn3 ~]# ip netns exec ns0 ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
8: veth02@if7: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 00:00:00:00:00:02 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 10.0.0.2/24 scope global veth02
       valid_lft forever preferred_lft forever
    inet6 fe80::200:ff:fe00:2/64 scope link 
       valid_lft forever preferred_lft forever
[root@ovn3 ~]# ip netns exec ns0 ping 10.0.0.1 -c1 -W1
PING 10.0.0.1 (10.0.0.1) 56(84) bytes of data.
64 bytes from 10.0.0.1: icmp_seq=1 ttl=64 time=0.886 ms

--- 10.0.0.1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.886/0.886/0.886/0.000 ms
[root@ovn3 ~]# 
~~~

## Adding virtual network with one virtual switch s1 ##

On ovn1, configure a logical switch and port:
~~~
ovn-nbctl ls-add s1
ovn-nbctl lsp-add s1 port11
ovn-nbctl lsp-set-addresses port11 00:00:00:00:01:01
ovn-nbctl lsp-add s1 port12
ovn-nbctl lsp-set-addresses port12 00:00:00:00:01:02
~~~

Now, "real" ports need to be wired to the above ports. Note that the logical port name has to match the `external_ids:iface-id` identifier. If we added  `ovn-nbctl lsp-add s1 foo` instead of `port0`, then we would have to set `ovs-vsctl set Interface port0 external_ids:iface-id=foo` on ovn2. Note that we need to know the MAC address of that port. Therefore, when creating the veth, we are making sure to create it with the correct MAC address.

On ovn2, execute:
~~~
ip link add name veth11 type veth peer name port11
ip netns add ns1
ip link set dev veth11 netns ns1
ip netns exec ns1 ip link set dev lo up
ip netns exec ns1 ip link set dev veth11 up
ip netns exec ns1 ip link set veth11 address 00:00:00:00:01:01
ip netns exec ns1 ip address add 10.0.1.1/24 dev veth11
ip link set dev port11 up
ovs-vsctl add-port br-int port11
ovs-vsctl set Interface port11 external_ids:iface-id=port11
~~~

On ovn3, execute:
~~~
ip link add name veth12 type veth peer name port12
ip netns add ns1
ip link set dev veth12 netns ns1
ip netns exec ns1 ip link set dev lo up
ip netns exec ns1 ip link set dev veth12 up
ip netns exec ns1 ip link set veth12 address 00:00:00:00:01:02
ip netns exec ns1 ip address add 10.0.1.2/24 dev veth12
ip link set dev port12 up
ovs-vsctl add-port br-int port12 external_ids:iface-id=port12
ovs-vsctl set Interface port12 external_ids:iface-id=port12
~~~

Verify the new configuration on ovn1:
~~~
ovn-nbctl show
ovn-sbctl show
~~~

~~~
[root@ovn1 ~]# ovn-nbctl show
switch 722300a5-73e1-42de-ae19-b7997f1f9a86 (s1)
    port port12
        addresses: ["00:00:00:00:01:02"]
    port port11
        addresses: ["00:00:00:00:01:01"]
switch 40f56f4d-db18-4d77-bb8b-c93e5179d346 (s0)
    port port02
        addresses: ["00:00:00:00:00:02"]
    port port01
        addresses: ["00:00:00:00:00:01"]
[root@ovn1 ~]# ovn-sbctl show
Chassis "b5337e05-2495-4bf0-8728-25840472baa4"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.203"
        options: {csum="true"}
    Port_Binding port12
    Port_Binding port02
Chassis "c3f90802-4fd5-44ad-a338-154a9150e46f"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.201"
        options: {csum="true"}
Chassis "8b9ad2e3-c1bc-4ea2-973e-a8bd1d38e502"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.202"
        options: {csum="true"}
    Port_Binding port01
    Port_Binding port11
~~~
> **Note:** The southbound database now shows port bindings.

Verify logical flows:
~~~
ovn-sbctl lflow-list
~~~

~~~
[root@ovn1 ~]# ovn-sbctl lflow-list
Datapath: "s0" (63c97e9f-194e-49b1-b075-661cd2132895)  Pipeline: ingress
  table=0 (ls_in_port_sec_l2  ), priority=100  , match=(eth.src[40]), action=(drop;)
  table=0 (ls_in_port_sec_l2  ), priority=100  , match=(vlan.present), action=(drop;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "port01"), action=(next;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "port02"), action=(next;)
  table=1 (ls_in_port_sec_ip  ), priority=0    , match=(1), action=(next;)
  table=2 (ls_in_port_sec_nd  ), priority=0    , match=(1), action=(next;)
  table=3 (ls_in_pre_acl      ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=3 (ls_in_pre_acl      ), priority=0    , match=(1), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=110  , match=(nd || nd_rs || nd_ra), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=0    , match=(1), action=(next;)
  table=5 (ls_in_pre_stateful ), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=5 (ls_in_pre_stateful ), priority=0    , match=(1), action=(next;)
  table=6 (ls_in_acl          ), priority=34000, match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=6 (ls_in_acl          ), priority=0    , match=(1), action=(next;)
  table=7 (ls_in_qos_mark     ), priority=0    , match=(1), action=(next;)
  table=8 (ls_in_qos_meter    ), priority=0    , match=(1), action=(next;)
  table=9 (ls_in_lb           ), priority=0    , match=(1), action=(next;)
  table=10(ls_in_stateful     ), priority=100  , match=(reg0[1] == 1), action=(ct_commit(ct_label=0/1); next;)
  table=10(ls_in_stateful     ), priority=100  , match=(reg0[2] == 1), action=(ct_lb;)
  table=10(ls_in_stateful     ), priority=0    , match=(1), action=(next;)
  table=11(ls_in_pre_hairpin  ), priority=0    , match=(1), action=(next;)
  table=12(ls_in_hairpin      ), priority=1    , match=(reg0[6] == 1), action=(eth.dst <-> eth.src;outport = inport;flags.loopback = 1;output;)
  table=12(ls_in_hairpin      ), priority=0    , match=(1), action=(next;)
  table=13(ls_in_arp_rsp      ), priority=0    , match=(1), action=(next;)
  table=14(ls_in_dhcp_options ), priority=0    , match=(1), action=(next;)
  table=15(ls_in_dhcp_response), priority=0    , match=(1), action=(next;)
  table=16(ls_in_dns_lookup   ), priority=0    , match=(1), action=(next;)
  table=17(ls_in_dns_response ), priority=0    , match=(1), action=(next;)
  table=18(ls_in_external_port), priority=0    , match=(1), action=(next;)
  table=19(ls_in_l2_lkup      ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(handle_svc_check(inport);)
  table=19(ls_in_l2_lkup      ), priority=70   , match=(eth.mcast), action=(outport = "_MC_flood"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:01), action=(outport = "port01"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:02), action=(outport = "port02"; output;)
Datapath: "s0" (63c97e9f-194e-49b1-b075-661cd2132895)  Pipeline: egress
  table=0 (ls_out_pre_lb      ), priority=110  , match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=0 (ls_out_pre_lb      ), priority=110  , match=(nd || nd_rs || nd_ra), action=(next;)
  table=0 (ls_out_pre_lb      ), priority=0    , match=(1), action=(next;)
  table=1 (ls_out_pre_acl     ), priority=110  , match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=1 (ls_out_pre_acl     ), priority=0    , match=(1), action=(next;)
  table=2 (ls_out_pre_stateful), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=2 (ls_out_pre_stateful), priority=0    , match=(1), action=(next;)
  table=3 (ls_out_lb          ), priority=0    , match=(1), action=(next;)
  table=4 (ls_out_acl         ), priority=34000, match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=4 (ls_out_acl         ), priority=0    , match=(1), action=(next;)
  table=5 (ls_out_qos_mark    ), priority=0    , match=(1), action=(next;)
  table=6 (ls_out_qos_meter   ), priority=0    , match=(1), action=(next;)
  table=7 (ls_out_stateful    ), priority=100  , match=(reg0[1] == 1), action=(ct_commit(ct_label=0/1); next;)
  table=7 (ls_out_stateful    ), priority=100  , match=(reg0[2] == 1), action=(ct_lb;)
  table=7 (ls_out_stateful    ), priority=0    , match=(1), action=(next;)
  table=8 (ls_out_port_sec_ip ), priority=0    , match=(1), action=(next;)
  table=9 (ls_out_port_sec_l2 ), priority=100  , match=(eth.mcast), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "port01"), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "port02"), action=(output;)
Datapath: "s1" (7ba84139-1ab7-47fe-874c-6956be31ab0a)  Pipeline: ingress
  table=0 (ls_in_port_sec_l2  ), priority=100  , match=(eth.src[40]), action=(drop;)
  table=0 (ls_in_port_sec_l2  ), priority=100  , match=(vlan.present), action=(drop;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "port11"), action=(next;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "port12"), action=(next;)
  table=1 (ls_in_port_sec_ip  ), priority=0    , match=(1), action=(next;)
  table=2 (ls_in_port_sec_nd  ), priority=0    , match=(1), action=(next;)
  table=3 (ls_in_pre_acl      ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=3 (ls_in_pre_acl      ), priority=0    , match=(1), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=110  , match=(nd || nd_rs || nd_ra), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=0    , match=(1), action=(next;)
  table=5 (ls_in_pre_stateful ), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=5 (ls_in_pre_stateful ), priority=0    , match=(1), action=(next;)
  table=6 (ls_in_acl          ), priority=34000, match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=6 (ls_in_acl          ), priority=0    , match=(1), action=(next;)
  table=7 (ls_in_qos_mark     ), priority=0    , match=(1), action=(next;)
  table=8 (ls_in_qos_meter    ), priority=0    , match=(1), action=(next;)
  table=9 (ls_in_lb           ), priority=0    , match=(1), action=(next;)
  table=10(ls_in_stateful     ), priority=100  , match=(reg0[1] == 1), action=(ct_commit(ct_label=0/1); next;)
  table=10(ls_in_stateful     ), priority=100  , match=(reg0[2] == 1), action=(ct_lb;)
  table=10(ls_in_stateful     ), priority=0    , match=(1), action=(next;)
  table=11(ls_in_pre_hairpin  ), priority=0    , match=(1), action=(next;)
  table=12(ls_in_hairpin      ), priority=1    , match=(reg0[6] == 1), action=(eth.dst <-> eth.src;outport = inport;flags.loopback = 1;output;)
  table=12(ls_in_hairpin      ), priority=0    , match=(1), action=(next;)
  table=13(ls_in_arp_rsp      ), priority=0    , match=(1), action=(next;)
  table=14(ls_in_dhcp_options ), priority=0    , match=(1), action=(next;)
  table=15(ls_in_dhcp_response), priority=0    , match=(1), action=(next;)
  table=16(ls_in_dns_lookup   ), priority=0    , match=(1), action=(next;)
  table=17(ls_in_dns_response ), priority=0    , match=(1), action=(next;)
  table=18(ls_in_external_port), priority=0    , match=(1), action=(next;)
  table=19(ls_in_l2_lkup      ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(handle_svc_check(inport);)
  table=19(ls_in_l2_lkup      ), priority=70   , match=(eth.mcast), action=(outport = "_MC_flood"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:01:01), action=(outport = "port11"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:01:02), action=(outport = "port12"; output;)
Datapath: "s1" (7ba84139-1ab7-47fe-874c-6956be31ab0a)  Pipeline: egress
  table=0 (ls_out_pre_lb      ), priority=110  , match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=0 (ls_out_pre_lb      ), priority=110  , match=(nd || nd_rs || nd_ra), action=(next;)
  table=0 (ls_out_pre_lb      ), priority=0    , match=(1), action=(next;)
  table=1 (ls_out_pre_acl     ), priority=110  , match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=1 (ls_out_pre_acl     ), priority=0    , match=(1), action=(next;)
  table=2 (ls_out_pre_stateful), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=2 (ls_out_pre_stateful), priority=0    , match=(1), action=(next;)
  table=3 (ls_out_lb          ), priority=0    , match=(1), action=(next;)
  table=4 (ls_out_acl         ), priority=34000, match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=4 (ls_out_acl         ), priority=0    , match=(1), action=(next;)
  table=5 (ls_out_qos_mark    ), priority=0    , match=(1), action=(next;)
  table=6 (ls_out_qos_meter   ), priority=0    , match=(1), action=(next;)
  table=7 (ls_out_stateful    ), priority=100  , match=(reg0[1] == 1), action=(ct_commit(ct_label=0/1); next;)
  table=7 (ls_out_stateful    ), priority=100  , match=(reg0[2] == 1), action=(ct_lb;)
  table=7 (ls_out_stateful    ), priority=0    , match=(1), action=(next;)
  table=8 (ls_out_port_sec_ip ), priority=0    , match=(1), action=(next;)
  table=9 (ls_out_port_sec_l2 ), priority=100  , match=(eth.mcast), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "port11"), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "port12"), action=(output;)
~~~

Last but not least, run a test ping between the namespaces on both hosts:
~~~
[root@ovn2 ~]# ip netns exec ns1 ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
10: veth11@if9: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 00:00:00:00:01:01 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 10.0.1.1/24 scope global veth11
       valid_lft forever preferred_lft forever
    inet6 fe80::200:ff:fe00:101/64 scope link 
       valid_lft forever preferred_lft forever
[root@ovn2 ~]# ip netns exec ns1 ping 10.0.1.2 -c1 -W1
PING 10.0.1.2 (10.0.1.2) 56(84) bytes of data.
64 bytes from 10.0.1.2: icmp_seq=1 ttl=64 time=1.83 ms

--- 10.0.1.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 1.833/1.833/1.833/0.000 ms
[root@ovn2 ~]# 
~~~

~~~
[root@ovn3 ~]# ip netns exec ns1 ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
10: veth12@if9: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 00:00:00:00:01:02 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 10.0.1.2/24 scope global veth12
       valid_lft forever preferred_lft forever
    inet6 fe80::200:ff:fe00:102/64 scope link 
       valid_lft forever preferred_lft forever
[root@ovn3 ~]# ip netns exec ns1 ping 10.0.1.1 -c1 -W1
PING 10.0.1.1 (10.0.1.1) 56(84) bytes of data.
64 bytes from 10.0.1.1: icmp_seq=1 ttl=64 time=0.916 ms

--- 10.0.1.1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.916/0.916/0.916/0.000 ms
[root@ovn3 ~]# 
~~~


## Adding virtual router r1 ## 

### Introduction ###

[http://www.openvswitch.org/support/dist-docs/ovn-architecture.7.html](http://www.openvswitch.org/support/dist-docs/ovn-architecture.7.html)
~~~
Logical Routers and Logical Patch Ports
       Typically  logical routers and logical patch ports do not have a physi‐
       cal location and effectively reside on every hypervisor.  This  is  the
       case  for  logical  patch  ports  between  logical  routers and logical
       switches behind those logical routers, to which VMs (and VIFs) attach.

       Consider a packet sent from one virtual machine or container to another
       VM  or  container  that  resides on a different subnet. The packet will
       traverse tables 0 to 65 as described in the previous section  Architec‐
       tural  Physical Life Cycle of a Packet, using the logical datapath rep‐
       resenting the logical switch that the sender is attached to.  At  table
       32, the packet will use the fallback flow that resubmits locally to ta‐
       ble 33 on the same hypervisor. In this case, all of the processing from
       table 0 to table 65 occurs on the hypervisor where the sender resides.

       When  the packet reaches table 65, the logical egress port is a logical
       patch port. The implementation in table 65 differs depending on the OVS
       version, although the observed behavior is meant to be the same:

              •      In  OVS  versions 2.6 and earlier, table 65 outputs to an
                     OVS patch port that represents the  logical  patch  port.
                     The packet re-enters the OpenFlow flow table from the OVS
                     patch port’s peer in table 0, which identifies the  logi‐
                     cal  datapath  and  logical  input  port based on the OVS
                     patch port’s OpenFlow port number.

              •      In OVS versions 2.7 and later, the packet is  cloned  and
                     resubmitted  directly to the first OpenFlow flow table in
                     the ingress pipeline, setting the logical ingress port to
                     the  peer  logical patch port, and using the peer logical
                     patch port’s logical datapath (that represents the  logi‐
                     cal router).

       The packet re-enters the ingress pipeline in order to traverse tables 8
       to 65 again, this time using the logical datapath representing the log‐
       ical router. The processing continues as described in the previous sec‐
       tion Architectural Physical Life Cycle of a  Packet.  When  the  packet
       reachs  table  65, the logical egress port will once again be a logical
       patch port. In the same manner as described above, this  logical  patch
       port  will  cause  the packet to be resubmitted to OpenFlow tables 8 to
       65, this time using  the  logical  datapath  representing  the  logical
       switch that the destination VM or container is attached to.

       The packet traverses tables 8 to 65 a third and final time. If the des‐
       tination VM or container resides on a remote hypervisor, then table  32
       will  send  the packet on a tunnel port from the sender’s hypervisor to
       the remote hypervisor. Finally table 65 will output the packet directly
       to the destination VM or container.

       The  following  sections describe two exceptions, where logical routers
       and/or logical patch ports are associated with a physical location.
~~~

### Setup ###

Configure namespace routes for virtual routing - on both hosts, run:
~~~
ip netns exec ns0 ip r add default via 10.0.0.254
ip netns exec ns1 ip r add default via 10.0.1.254
~~~

Now, set up the OVN northbound db - router and router ports:
~~~
ovn-nbctl lr-add r1

ovn-nbctl lrp-add r1 r1s0 00:00:00:00:00:ff 10.0.0.254/24
ovn-nbctl lsp-add s0 s0r1 
ovn-nbctl lsp-set-addresses s0r1 00:00:00:00:00:ff
ovn-nbctl lsp-set-options s0r1 router-port=r1s0
ovn-nbctl lsp-set-type s0r1 router

ovn-nbctl lrp-add r1 r1s1 00:00:00:00:01:ff 10.0.1.254/24
ovn-nbctl lsp-add s1 s1r1 
ovn-nbctl lsp-set-addresses s1r1 00:00:00:00:01:ff
ovn-nbctl lsp-set-options s1r1 router-port=r1s1
ovn-nbctl lsp-set-type s1r1 router
~~~

Verify the new configuration:
~~~
ovn-nbctl show
ovn-sbctl show
~~~

~~~
[root@ovn1 ~]#  ovn-nbctl show
switch 722300a5-73e1-42de-ae19-b7997f1f9a86 (s1)
    port port12
        addresses: ["00:00:00:00:01:02"]
    port s1r1
        type: router
        addresses: ["00:00:00:00:01:ff"]
        router-port: r1s1
    port port11
        addresses: ["00:00:00:00:01:01"]
switch 40f56f4d-db18-4d77-bb8b-c93e5179d346 (s0)
    port s0r1
        type: router
        addresses: ["00:00:00:00:00:ff"]
        router-port: r1s0
    port port02
        addresses: ["00:00:00:00:00:02"]
    port port01
        addresses: ["00:00:00:00:00:01"]
router 3affb86c-5f1a-461d-bd07-b4a9580ed9bf (r1)
    port r1s0
        mac: "00:00:00:00:00:ff"
        networks: ["10.0.0.254/24"]
    port r1s1
        mac: "00:00:00:00:01:ff"
        networks: ["10.0.1.254/24"]
[root@ovn1 ~]# ovn-sbctl show
Chassis "b5337e05-2495-4bf0-8728-25840472baa4"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.203"
        options: {csum="true"}
    Port_Binding port12
    Port_Binding port02
Chassis "c3f90802-4fd5-44ad-a338-154a9150e46f"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.201"
        options: {csum="true"}
Chassis "8b9ad2e3-c1bc-4ea2-973e-a8bd1d38e502"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.202"
        options: {csum="true"}
    Port_Binding port01
    Port_Binding port11
~~~

Verify logical flows:
~~~
ovn-sbctl lflow-list
~~~

~~~
[root@ovn1 ~]# ovn-sbctl lflow-list
Datapath: "s0" (63c97e9f-194e-49b1-b075-661cd2132895)  Pipeline: ingress
  table=0 (ls_in_port_sec_l2  ), priority=100  , match=(eth.src[40]), action=(drop;)
  table=0 (ls_in_port_sec_l2  ), priority=100  , match=(vlan.present), action=(drop;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "port01"), action=(next;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "port02"), action=(next;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "s0r1"), action=(next;)
  table=1 (ls_in_port_sec_ip  ), priority=0    , match=(1), action=(next;)
  table=2 (ls_in_port_sec_nd  ), priority=0    , match=(1), action=(next;)
  table=3 (ls_in_pre_acl      ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=3 (ls_in_pre_acl      ), priority=0    , match=(1), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=110  , match=(nd || nd_rs || nd_ra), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=0    , match=(1), action=(next;)
  table=5 (ls_in_pre_stateful ), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=5 (ls_in_pre_stateful ), priority=0    , match=(1), action=(next;)
  table=6 (ls_in_acl          ), priority=34000, match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=6 (ls_in_acl          ), priority=0    , match=(1), action=(next;)
  table=7 (ls_in_qos_mark     ), priority=0    , match=(1), action=(next;)
  table=8 (ls_in_qos_meter    ), priority=0    , match=(1), action=(next;)
  table=9 (ls_in_lb           ), priority=0    , match=(1), action=(next;)
  table=10(ls_in_stateful     ), priority=100  , match=(reg0[1] == 1), action=(ct_commit(ct_label=0/1); next;)
  table=10(ls_in_stateful     ), priority=100  , match=(reg0[2] == 1), action=(ct_lb;)
  table=10(ls_in_stateful     ), priority=0    , match=(1), action=(next;)
  table=11(ls_in_pre_hairpin  ), priority=0    , match=(1), action=(next;)
  table=12(ls_in_hairpin      ), priority=1    , match=(reg0[6] == 1), action=(eth.dst <-> eth.src;outport = inport;flags.loopback = 1;output;)
  table=12(ls_in_hairpin      ), priority=0    , match=(1), action=(next;)
  table=13(ls_in_arp_rsp      ), priority=0    , match=(1), action=(next;)
  table=14(ls_in_dhcp_options ), priority=0    , match=(1), action=(next;)
  table=15(ls_in_dhcp_response), priority=0    , match=(1), action=(next;)
  table=16(ls_in_dns_lookup   ), priority=0    , match=(1), action=(next;)
  table=17(ls_in_dns_response ), priority=0    , match=(1), action=(next;)
  table=18(ls_in_external_port), priority=0    , match=(1), action=(next;)
  table=19(ls_in_l2_lkup      ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(handle_svc_check(inport);)
  table=19(ls_in_l2_lkup      ), priority=80   , match=(eth.src == { 00:00:00:00:00:ff} && (arp.op == 1 || nd_ns)), action=(outport = "_MC_flood"; output;)
  table=19(ls_in_l2_lkup      ), priority=75   , match=(flags[1] == 0 && arp.op == 1 && arp.tpa == { 10.0.0.254}), action=(outport = "s0r1"; output;)
  table=19(ls_in_l2_lkup      ), priority=75   , match=(flags[1] == 0 && nd_ns && nd.target == { fe80::200:ff:fe00:ff}), action=(outport = "s0r1"; output;)
  table=19(ls_in_l2_lkup      ), priority=70   , match=(eth.mcast), action=(outport = "_MC_flood"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:01), action=(outport = "port01"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:02), action=(outport = "port02"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:ff), action=(outport = "s0r1"; output;)
Datapath: "s0" (63c97e9f-194e-49b1-b075-661cd2132895)  Pipeline: egress
  table=0 (ls_out_pre_lb      ), priority=110  , match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=0 (ls_out_pre_lb      ), priority=110  , match=(nd || nd_rs || nd_ra), action=(next;)
  table=0 (ls_out_pre_lb      ), priority=0    , match=(1), action=(next;)
  table=1 (ls_out_pre_acl     ), priority=110  , match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=1 (ls_out_pre_acl     ), priority=0    , match=(1), action=(next;)
  table=2 (ls_out_pre_stateful), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=2 (ls_out_pre_stateful), priority=0    , match=(1), action=(next;)
  table=3 (ls_out_lb          ), priority=0    , match=(1), action=(next;)
  table=4 (ls_out_acl         ), priority=34000, match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=4 (ls_out_acl         ), priority=0    , match=(1), action=(next;)
  table=5 (ls_out_qos_mark    ), priority=0    , match=(1), action=(next;)
  table=6 (ls_out_qos_meter   ), priority=0    , match=(1), action=(next;)
  table=7 (ls_out_stateful    ), priority=100  , match=(reg0[1] == 1), action=(ct_commit(ct_label=0/1); next;)
  table=7 (ls_out_stateful    ), priority=100  , match=(reg0[2] == 1), action=(ct_lb;)
  table=7 (ls_out_stateful    ), priority=0    , match=(1), action=(next;)
  table=8 (ls_out_port_sec_ip ), priority=0    , match=(1), action=(next;)
  table=9 (ls_out_port_sec_l2 ), priority=100  , match=(eth.mcast), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "port01"), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "port02"), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "s0r1"), action=(output;)
Datapath: "s1" (7ba84139-1ab7-47fe-874c-6956be31ab0a)  Pipeline: ingress
  table=0 (ls_in_port_sec_l2  ), priority=100  , match=(eth.src[40]), action=(drop;)
  table=0 (ls_in_port_sec_l2  ), priority=100  , match=(vlan.present), action=(drop;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "port11"), action=(next;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "port12"), action=(next;)
  table=0 (ls_in_port_sec_l2  ), priority=50   , match=(inport == "s1r1"), action=(next;)
  table=1 (ls_in_port_sec_ip  ), priority=0    , match=(1), action=(next;)
  table=2 (ls_in_port_sec_nd  ), priority=0    , match=(1), action=(next;)
  table=3 (ls_in_pre_acl      ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=3 (ls_in_pre_acl      ), priority=0    , match=(1), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=110  , match=(nd || nd_rs || nd_ra), action=(next;)
  table=4 (ls_in_pre_lb       ), priority=0    , match=(1), action=(next;)
  table=5 (ls_in_pre_stateful ), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=5 (ls_in_pre_stateful ), priority=0    , match=(1), action=(next;)
  table=6 (ls_in_acl          ), priority=34000, match=(eth.dst == fa:bb:da:7b:a8:58), action=(next;)
  table=6 (ls_in_acl          ), priority=0    , match=(1), action=(next;)
  table=7 (ls_in_qos_mark     ), priority=0    , match=(1), action=(next;)
  table=8 (ls_in_qos_meter    ), priority=0    , match=(1), action=(next;)
  table=9 (ls_in_lb           ), priority=0    , match=(1), action=(next;)
  table=10(ls_in_stateful     ), priority=100  , match=(reg0[1] == 1), action=(ct_commit(ct_label=0/1); next;)
  table=10(ls_in_stateful     ), priority=100  , match=(reg0[2] == 1), action=(ct_lb;)
  table=10(ls_in_stateful     ), priority=0    , match=(1), action=(next;)
  table=11(ls_in_pre_hairpin  ), priority=0    , match=(1), action=(next;)
  table=12(ls_in_hairpin      ), priority=1    , match=(reg0[6] == 1), action=(eth.dst <-> eth.src;outport = inport;flags.loopback = 1;output;)
  table=12(ls_in_hairpin      ), priority=0    , match=(1), action=(next;)
  table=13(ls_in_arp_rsp      ), priority=0    , match=(1), action=(next;)
  table=14(ls_in_dhcp_options ), priority=0    , match=(1), action=(next;)
  table=15(ls_in_dhcp_response), priority=0    , match=(1), action=(next;)
  table=16(ls_in_dns_lookup   ), priority=0    , match=(1), action=(next;)
  table=17(ls_in_dns_response ), priority=0    , match=(1), action=(next;)
  table=18(ls_in_external_port), priority=0    , match=(1), action=(next;)
  table=19(ls_in_l2_lkup      ), priority=110  , match=(eth.dst == fa:bb:da:7b:a8:58), action=(handle_svc_check(inport);)
  table=19(ls_in_l2_lkup      ), priority=80   , match=(eth.src == { 00:00:00:00:01:ff} && (arp.op == 1 || nd_ns)), action=(outport = "_MC_flood"; output;)
  table=19(ls_in_l2_lkup      ), priority=75   , match=(flags[1] == 0 && arp.op == 1 && arp.tpa == { 10.0.1.254}), action=(outport = "s1r1"; output;)
  table=19(ls_in_l2_lkup      ), priority=75   , match=(flags[1] == 0 && nd_ns && nd.target == { fe80::200:ff:fe00:1ff}), action=(outport = "s1r1"; output;)
  table=19(ls_in_l2_lkup      ), priority=70   , match=(eth.mcast), action=(outport = "_MC_flood"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:01:01), action=(outport = "port11"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:01:02), action=(outport = "port12"; output;)
  table=19(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:01:ff), action=(outport = "s1r1"; output;)
Datapath: "s1" (7ba84139-1ab7-47fe-874c-6956be31ab0a)  Pipeline: egress
  table=0 (ls_out_pre_lb      ), priority=110  , match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=0 (ls_out_pre_lb      ), priority=110  , match=(nd || nd_rs || nd_ra), action=(next;)
  table=0 (ls_out_pre_lb      ), priority=0    , match=(1), action=(next;)
  table=1 (ls_out_pre_acl     ), priority=110  , match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=1 (ls_out_pre_acl     ), priority=0    , match=(1), action=(next;)
  table=2 (ls_out_pre_stateful), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=2 (ls_out_pre_stateful), priority=0    , match=(1), action=(next;)
  table=3 (ls_out_lb          ), priority=0    , match=(1), action=(next;)
  table=4 (ls_out_acl         ), priority=34000, match=(eth.src == fa:bb:da:7b:a8:58), action=(next;)
  table=4 (ls_out_acl         ), priority=0    , match=(1), action=(next;)
  table=5 (ls_out_qos_mark    ), priority=0    , match=(1), action=(next;)
  table=6 (ls_out_qos_meter   ), priority=0    , match=(1), action=(next;)
  table=7 (ls_out_stateful    ), priority=100  , match=(reg0[1] == 1), action=(ct_commit(ct_label=0/1); next;)
  table=7 (ls_out_stateful    ), priority=100  , match=(reg0[2] == 1), action=(ct_lb;)
  table=7 (ls_out_stateful    ), priority=0    , match=(1), action=(next;)
  table=8 (ls_out_port_sec_ip ), priority=0    , match=(1), action=(next;)
  table=9 (ls_out_port_sec_l2 ), priority=100  , match=(eth.mcast), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "port11"), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "port12"), action=(output;)
  table=9 (ls_out_port_sec_l2 ), priority=50   , match=(outport == "s1r1"), action=(output;)
Datapath: "r1" (df1fbb81-4fde-4585-a7bd-5e03b7961947)  Pipeline: ingress
  table=0 (lr_in_admission    ), priority=100  , match=(vlan.present || eth.src[40]), action=(drop;)
  table=0 (lr_in_admission    ), priority=50   , match=(eth.dst == 00:00:00:00:00:ff && inport == "r1s0"), action=(next;)
  table=0 (lr_in_admission    ), priority=50   , match=(eth.dst == 00:00:00:00:01:ff && inport == "r1s1"), action=(next;)
  table=0 (lr_in_admission    ), priority=50   , match=(eth.mcast && inport == "r1s0"), action=(next;)
  table=0 (lr_in_admission    ), priority=50   , match=(eth.mcast && inport == "r1s1"), action=(next;)
  table=1 (lr_in_lookup_neighbor), priority=100  , match=(arp.op == 2), action=(reg9[2] = lookup_arp(inport, arp.spa, arp.sha); next;)
  table=1 (lr_in_lookup_neighbor), priority=100  , match=(inport == "r1s0" && arp.spa == 10.0.0.0/24 && arp.op == 1), action=(reg9[2] = lookup_arp(inport, arp.spa, arp.sha); next;)
  table=1 (lr_in_lookup_neighbor), priority=100  , match=(inport == "r1s1" && arp.spa == 10.0.1.0/24 && arp.op == 1), action=(reg9[2] = lookup_arp(inport, arp.spa, arp.sha); next;)
  table=1 (lr_in_lookup_neighbor), priority=100  , match=(nd_na), action=(reg9[2] = lookup_nd(inport, nd.target, nd.tll); next;)
  table=1 (lr_in_lookup_neighbor), priority=100  , match=(nd_ns), action=(reg9[2] = lookup_nd(inport, ip6.src, nd.sll); next;)
  table=1 (lr_in_lookup_neighbor), priority=0    , match=(1), action=(reg9[3] = 1; next;)
  table=2 (lr_in_learn_neighbor), priority=100  , match=(reg9[3] == 1 || reg9[2] == 1), action=(next;)
  table=2 (lr_in_learn_neighbor), priority=90   , match=(arp), action=(put_arp(inport, arp.spa, arp.sha); next;)
  table=2 (lr_in_learn_neighbor), priority=90   , match=(nd_na), action=(put_nd(inport, nd.target, nd.tll); next;)
  table=2 (lr_in_learn_neighbor), priority=90   , match=(nd_ns), action=(put_nd(inport, ip6.src, nd.sll); next;)
  table=3 (lr_in_ip_input     ), priority=100  , match=(ip4.src == {10.0.0.254, 10.0.0.255} && reg9[0] == 0), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=100  , match=(ip4.src == {10.0.1.254, 10.0.1.255} && reg9[0] == 0), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=100  , match=(ip4.src_mcast ||ip4.src == 255.255.255.255 || ip4.src == 127.0.0.0/8 || ip4.dst == 127.0.0.0/8 || ip4.src == 0.0.0.0/8 || ip4.dst == 0.0.0.0/8), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=100  , match=(ip6.dst == fe80::200:ff:fe00:1ff && udp.src == 547 && udp.dst == 546), action=(reg0 = 0; handle_dhcpv6_reply;)
  table=3 (lr_in_ip_input     ), priority=100  , match=(ip6.dst == fe80::200:ff:fe00:ff && udp.src == 547 && udp.dst == 546), action=(reg0 = 0; handle_dhcpv6_reply;)
  table=3 (lr_in_ip_input     ), priority=90   , match=(inport == "r1s0" && arp.spa == 10.0.0.0/24 && arp.tpa == 10.0.0.254 && arp.op == 1), action=(eth.dst = eth.src; eth.src = 00:00:00:00:00:ff; arp.op = 2; /* ARP reply */ arp.tha = arp.sha; arp.sha = 00:00:00:00:00:ff; arp.tpa = arp.spa; arp.spa = 10.0.0.254; outport = "r1s0"; flags.loopback = 1; output;)
  table=3 (lr_in_ip_input     ), priority=90   , match=(inport == "r1s0" && nd_ns && ip6.dst == {fe80::200:ff:fe00:ff, ff02::1:ff00:ff} && nd.target == fe80::200:ff:fe00:ff), action=(nd_na_router { eth.src = 00:00:00:00:00:ff; ip6.src = fe80::200:ff:fe00:ff; nd.target = fe80::200:ff:fe00:ff; nd.tll = 00:00:00:00:00:ff; outport = inport; flags.loopback = 1; output; };)
  table=3 (lr_in_ip_input     ), priority=90   , match=(inport == "r1s1" && arp.spa == 10.0.1.0/24 && arp.tpa == 10.0.1.254 && arp.op == 1), action=(eth.dst = eth.src; eth.src = 00:00:00:00:01:ff; arp.op = 2; /* ARP reply */ arp.tha = arp.sha; arp.sha = 00:00:00:00:01:ff; arp.tpa = arp.spa; arp.spa = 10.0.1.254; outport = "r1s1"; flags.loopback = 1; output;)
  table=3 (lr_in_ip_input     ), priority=90   , match=(inport == "r1s1" && nd_ns && ip6.dst == {fe80::200:ff:fe00:1ff, ff02::1:ff00:1ff} && nd.target == fe80::200:ff:fe00:1ff), action=(nd_na_router { eth.src = 00:00:00:00:01:ff; ip6.src = fe80::200:ff:fe00:1ff; nd.target = fe80::200:ff:fe00:1ff; nd.tll = 00:00:00:00:01:ff; outport = inport; flags.loopback = 1; output; };)
  table=3 (lr_in_ip_input     ), priority=90   , match=(ip4.dst == 10.0.0.254 && icmp4.type == 8 && icmp4.code == 0), action=(ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 0; flags.loopback = 1; next; )
  table=3 (lr_in_ip_input     ), priority=90   , match=(ip4.dst == 10.0.1.254 && icmp4.type == 8 && icmp4.code == 0), action=(ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 0; flags.loopback = 1; next; )
  table=3 (lr_in_ip_input     ), priority=90   , match=(ip6.dst == fe80::200:ff:fe00:1ff && icmp6.type == 128 && icmp6.code == 0), action=(ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 129; flags.loopback = 1; next; )
  table=3 (lr_in_ip_input     ), priority=90   , match=(ip6.dst == fe80::200:ff:fe00:ff && icmp6.type == 128 && icmp6.code == 0), action=(ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 129; flags.loopback = 1; next; )
  table=3 (lr_in_ip_input     ), priority=85   , match=(arp || nd), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=84   , match=(nd_rs || nd_ra), action=(next;)
  table=3 (lr_in_ip_input     ), priority=83   , match=(ip6.mcast_rsvd), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=82   , match=(ip4.mcast || ip6.mcast), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip4 && ip4.dst == 10.0.0.254 && !ip.later_frag && tcp), action=(tcp_reset {eth.dst <-> eth.src; ip4.dst <-> ip4.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip4 && ip4.dst == 10.0.0.254 && !ip.later_frag && udp), action=(icmp4 {eth.dst <-> eth.src; ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 3; icmp4.code = 3; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip4 && ip4.dst == 10.0.1.254 && !ip.later_frag && tcp), action=(tcp_reset {eth.dst <-> eth.src; ip4.dst <-> ip4.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip4 && ip4.dst == 10.0.1.254 && !ip.later_frag && udp), action=(icmp4 {eth.dst <-> eth.src; ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 3; icmp4.code = 3; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:1ff && !ip.later_frag && tcp), action=(tcp_reset {eth.dst <-> eth.src; ip6.dst <-> ip6.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:1ff && !ip.later_frag && udp), action=(icmp6 {eth.dst <-> eth.src; ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 1; icmp6.code = 4; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:ff && !ip.later_frag && tcp), action=(tcp_reset {eth.dst <-> eth.src; ip6.dst <-> ip6.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:ff && !ip.later_frag && udp), action=(icmp6 {eth.dst <-> eth.src; ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 1; icmp6.code = 4; next; };)
  table=3 (lr_in_ip_input     ), priority=70   , match=(ip4 && ip4.dst == 10.0.0.254 && !ip.later_frag), action=(icmp4 {eth.dst <-> eth.src; ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 3; icmp4.code = 2; next; };)
  table=3 (lr_in_ip_input     ), priority=70   , match=(ip4 && ip4.dst == 10.0.1.254 && !ip.later_frag), action=(icmp4 {eth.dst <-> eth.src; ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 3; icmp4.code = 2; next; };)
  table=3 (lr_in_ip_input     ), priority=70   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:1ff && !ip.later_frag), action=(icmp6 {eth.dst <-> eth.src; ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 1; icmp6.code = 3; next; };)
  table=3 (lr_in_ip_input     ), priority=70   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:ff && !ip.later_frag), action=(icmp6 {eth.dst <-> eth.src; ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 1; icmp6.code = 3; next; };)
  table=3 (lr_in_ip_input     ), priority=60   , match=(ip4.dst == {10.0.0.254} || ip6.dst == {fe80::200:ff:fe00:ff}), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=60   , match=(ip4.dst == {10.0.1.254} || ip6.dst == {fe80::200:ff:fe00:1ff}), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=50   , match=(eth.bcast), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=40   , match=(inport == "r1s0" && ip4 && ip.ttl == {0, 1} && !ip.later_frag), action=(icmp4 {eth.dst <-> eth.src; icmp4.type = 11; /* Time exceeded */ icmp4.code = 0; /* TTL exceeded in transit */ ip4.dst = ip4.src; ip4.src = 10.0.0.254; ip.ttl = 255; next; };)
  table=3 (lr_in_ip_input     ), priority=40   , match=(inport == "r1s1" && ip4 && ip.ttl == {0, 1} && !ip.later_frag), action=(icmp4 {eth.dst <-> eth.src; icmp4.type = 11; /* Time exceeded */ icmp4.code = 0; /* TTL exceeded in transit */ ip4.dst = ip4.src; ip4.src = 10.0.1.254; ip.ttl = 255; next; };)
  table=3 (lr_in_ip_input     ), priority=30   , match=(ip4 && ip.ttl == {0, 1}), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=0    , match=(1), action=(next;)
  table=4 (lr_in_defrag       ), priority=0    , match=(1), action=(next;)
  table=5 (lr_in_unsnat       ), priority=0    , match=(1), action=(next;)
  table=6 (lr_in_dnat         ), priority=0    , match=(1), action=(next;)
  table=7 (lr_in_nd_ra_options), priority=0    , match=(1), action=(next;)
  table=8 (lr_in_nd_ra_response), priority=0    , match=(1), action=(next;)
  table=9 (lr_in_ip_routing   ), priority=550  , match=(nd_rs || nd_ra), action=(drop;)
  table=9 (lr_in_ip_routing   ), priority=129  , match=(inport == "r1s0" && ip6.dst == fe80::/64), action=(ip.ttl--; reg8[0..15] = 0; xxreg0 = ip6.dst; xxreg1 = fe80::200:ff:fe00:ff; eth.src = 00:00:00:00:00:ff; outport = "r1s0"; flags.loopback = 1; next;)
  table=9 (lr_in_ip_routing   ), priority=129  , match=(inport == "r1s1" && ip6.dst == fe80::/64), action=(ip.ttl--; reg8[0..15] = 0; xxreg0 = ip6.dst; xxreg1 = fe80::200:ff:fe00:1ff; eth.src = 00:00:00:00:01:ff; outport = "r1s1"; flags.loopback = 1; next;)
  table=9 (lr_in_ip_routing   ), priority=49   , match=(ip4.dst == 10.0.0.0/24), action=(ip.ttl--; reg8[0..15] = 0; reg0 = ip4.dst; reg1 = 10.0.0.254; eth.src = 00:00:00:00:00:ff; outport = "r1s0"; flags.loopback = 1; next;)
  table=9 (lr_in_ip_routing   ), priority=49   , match=(ip4.dst == 10.0.1.0/24), action=(ip.ttl--; reg8[0..15] = 0; reg0 = ip4.dst; reg1 = 10.0.1.254; eth.src = 00:00:00:00:01:ff; outport = "r1s1"; flags.loopback = 1; next;)
  table=10(lr_in_ip_routing_ecmp), priority=150  , match=(reg8[0..15] == 0), action=(next;)
  table=11(lr_in_policy       ), priority=0    , match=(1), action=(next;)
  table=12(lr_in_arp_resolve  ), priority=500  , match=(ip4.mcast || ip6.mcast), action=(next;)
  table=12(lr_in_arp_resolve  ), priority=0    , match=(ip4), action=(get_arp(outport, reg0); next;)
  table=12(lr_in_arp_resolve  ), priority=0    , match=(ip6), action=(get_nd(outport, xxreg0); next;)
  table=13(lr_in_chk_pkt_len  ), priority=0    , match=(1), action=(next;)
  table=14(lr_in_larger_pkts  ), priority=0    , match=(1), action=(next;)
  table=15(lr_in_gw_redirect  ), priority=0    , match=(1), action=(next;)
  table=16(lr_in_arp_request  ), priority=100  , match=(eth.dst == 00:00:00:00:00:00 && ip4), action=(arp { eth.dst = ff:ff:ff:ff:ff:ff; arp.spa = reg1; arp.tpa = reg0; arp.op = 1; output; };)
  table=16(lr_in_arp_request  ), priority=100  , match=(eth.dst == 00:00:00:00:00:00 && ip6), action=(nd_ns { nd.target = xxreg0; output; };)
  table=16(lr_in_arp_request  ), priority=0    , match=(1), action=(output;)
Datapath: "r1" (df1fbb81-4fde-4585-a7bd-5e03b7961947)  Pipeline: egress
  table=0 (lr_out_undnat      ), priority=0    , match=(1), action=(next;)
  table=1 (lr_out_snat        ), priority=120  , match=(nd_ns), action=(next;)
  table=1 (lr_out_snat        ), priority=0    , match=(1), action=(next;)
  table=2 (lr_out_egr_loop    ), priority=0    , match=(1), action=(next;)
  table=3 (lr_out_delivery    ), priority=100  , match=(outport == "r1s0"), action=(output;)
  table=3 (lr_out_delivery    ), priority=100  , match=(outport == "r1s1"), action=(output;)
~~~

Last but not least, run a test ping between the namespaces on both hosts:
~~~
[root@ovn2 ~]# ip netns exec ns0 ping 10.0.1.1 -c1 -W1
[root@ovn2 ~]# ip netns exec ns0 ping 10.0.1.2 -c1 -W1
~~~

~~~
[root@ovn2 ~]# ip netns exec ns0 ping 10.0.1.1 -c1 -W1
PING 10.0.1.1 (10.0.1.1) 56(84) bytes of data.
64 bytes from 10.0.1.1: icmp_seq=1 ttl=63 time=78.8 ms

--- 10.0.1.1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 78.799/78.799/78.799/0.000 ms
[root@ovn2 ~]# ip netns exec ns0 ping 10.0.1.2 -c1 -W1
PING 10.0.1.2 (10.0.1.2) 56(84) bytes of data.
64 bytes from 10.0.1.2: icmp_seq=1 ttl=63 time=21.4 ms

--- 10.0.1.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 21.413/21.413/21.413/0.000 ms
[root@ovn2 ~]# 
~~~

## Adding external gateway  router, non-distributed ##

### Introduction ###

[http://www.openvswitch.org/support/dist-docs/ovn-architecture.7.html](http://www.openvswitch.org/support/dist-docs/ovn-architecture.7.html)
~~~
     Gateway Routers

       A gateway router is a logical router that is bound to a physical  loca‐
       tion.  This  includes  all  of  the  logical patch ports of the logical
       router, as well as all of the  peer  logical  patch  ports  on  logical
       switches.  In the OVN Southbound database, the Port_Binding entries for
       these logical patch ports use the type l3gateway rather than patch,  in
       order  to  distinguish  that  these  logical patch ports are bound to a
       chassis.

       When a hypervisor processes a packet on a logical datapath representing
       a  logical switch, and the logical egress port is a l3gateway port rep‐
       resenting connectivity to a gateway router, the  packet  will  match  a
       flow  in table 32 that sends the packet on a tunnel port to the chassis
       where the gateway router resides. This processing in table 32  is  done
       in the same manner as for VIFs.

       Gateway  routers  are  typically  used  in  between distributed logical
       routers and physical networks. The distributed logical router  and  the
       logical  switches behind it, to which VMs and containers attach, effec‐
       tively reside on each hypervisor. The distributed router and the  gate‐
       way  router are connected by another logical switch, sometimes referred
       to as a join logical switch. On the other side, the gateway router con‐
       nects  to another logical switch that has a localnet port connecting to
       the physical network.

       When using gateway routers, DNAT and SNAT rules are associated with the
       gateway  router, which provides a central location that can handle one-
       to-many SNAT (aka IP masquerading).
~~~

### Setup ###

Add provider bridge mapping on ovn1:
~~~
# ovn1
ovs-vsctl set open . external-ids:ovn-bridge-mappings=provider:br-provider
ovs-vsctl --may-exist add-br br-provider
ovs-vsctl --may-exist add-port br-provider eth1
~~~

Bind logical router rg (router gateway) to chassis ovn1 and assign an internal IP pointing towards the VMs: 
~~~
chassis=$(ovn-sbctl find chassis hostname=ovn1 | awk '/^name/ {print $NF}' | sed 's/"//g')
ovn-nbctl create Logical_Router name=rg options:chassis=$chassis
ovn-nbctl lrp-add rg rgsj 00:00:00:01:00:01 10.1.0.1/30
~~~

Create switch sj (switch join) and add the router port of rg to it:
~~~
ovn-nbctl ls-add sj
ovn-nbctl lsp-add sj sjrg
ovn-nbctl lsp-set-type sjrg router
ovn-nbctl lsp-set-addresses sjrg 00:00:00:01:00:01
ovn-nbctl lsp-set-options sjrg router-port=rgsj
~~~

Add r1 port to the join switch:
~~~
ovn-nbctl lrp-add r1 r1sj 00:00:00:01:00:02 10.1.0.2/30
~~~

~~~
ovn-nbctl lsp-add sj sjr1
ovn-nbctl lsp-set-type sjr1 router
ovn-nbctl lsp-set-addresses sjr1 00:00:00:01:00:02
ovn-nbctl lsp-set-options sjr1 router-port=r1sj
~~~

Add routes - router gateway towards our 2 private subnets via r1 and r1 default route towards rg:
~~~
ovn-nbctl lr-route-add rg "10.0.0.0/24" 10.1.0.2
ovn-nbctl lr-route-add rg "10.0.1.0/24" 10.1.0.2
ovn-nbctl lr-route-add r1 "0.0.0.0/0" 10.1.0.1
~~~

Verify routes:
~~~
[root@ovn1 ~]# ovn-nbctl lr-route-list rg
IPv4 Routes
              10.0.0.0/24                  10.1.0.2 dst-ip
              10.0.1.0/24                  10.1.0.2 dst-ip
[root@ovn1 ~]# ovn-nbctl lr-route-list r1
IPv4 Routes
                0.0.0.0/0                  10.1.0.1 dst-ip
[root@ovn1 ~]# ovn-nbctl list Logical_Router_Static_Route
_uuid               : 9ddd9b58-12c2-43c8-afe2-c20cb71aa824
external_ids        : {}
ip_prefix           : "0.0.0.0/0"
nexthop             : "10.1.0.1"
output_port         : []
policy              : []

_uuid               : 5e915985-d98c-48bf-ba35-25a10c150bb1
external_ids        : {}
ip_prefix           : "10.0.0.0/24"
nexthop             : "10.1.0.2"
output_port         : []
policy              : []

_uuid               : a73f670f-d435-471f-9273-58a6b8767f5f
external_ids        : {}
ip_prefix           : "10.0.1.0/24"
nexthop             : "10.1.0.2"
output_port         : []
policy              : []
~~~

Add an external port to rg:
~~~
ovn-nbctl lrp-add rg rgsp 00:00:00:02:00:ff 10.2.0.254/24
~~~

Add provider network switch (sp):
~~~
ovn-nbctl ls-add sp
ovn-nbctl lsp-add sp sprg
ovn-nbctl lsp-set-type sprg router
ovn-nbctl lsp-set-addresses sprg 00:00:00:02:00:ff
ovn-nbctl lsp-set-options sprg router-port=rgsp
~~~

Add localnet port (eth1) to provider switch (sp):
~~~
ovn-nbctl lsp-add sp sp-localnet
ovn-nbctl lsp-set-addresses sp-localnet unknown
ovn-nbctl lsp-set-type sp-localnet localnet
ovn-nbctl lsp-set-options sp-localnet network_name=provider
~~~

Verify on ovn1:
~~~
ovs-vsctl show
ovn-nbctl show
ovn-sbctl show
~~~

~~~
[root@ovn1 ~]# ovs-vsctl show
64003a9a-1f2a-403c-8d21-cd187d2f717c
    Bridge br-int
        fail_mode: secure
        Port ovn-8b9ad2-0
            Interface ovn-8b9ad2-0
                type: geneve
                options: {csum="true", key=flow, remote_ip="192.168.122.202"}
        Port ovn-b5337e-0
            Interface ovn-b5337e-0
                type: geneve
                options: {csum="true", key=flow, remote_ip="192.168.122.203"}
        Port br-int
            Interface br-int
                type: internal
        Port patch-br-int-to-sp-localnet
            Interface patch-br-int-to-sp-localnet
                type: patch
                options: {peer=patch-sp-localnet-to-br-int}
    Bridge br-provider
        Port eth1
            Interface eth1
        Port patch-sp-localnet-to-br-int
            Interface patch-sp-localnet-to-br-int
                type: patch
                options: {peer=patch-br-int-to-sp-localnet}
        Port br-provider
            Interface br-provider
                type: internal
    ovs_version: "2.13.0"
[root@ovn1 ~]# ovn-nbctl show
switch c11aeac8-3469-444d-819e-cd0f50437175 (sp)
    port sp-localnet
        type: localnet
        addresses: ["unknown"]
    port sprg
        type: router
        addresses: ["00:00:00:02:00:ff"]
        router-port: rgsp
switch 40f56f4d-db18-4d77-bb8b-c93e5179d346 (s0)
    port s0r1
        type: router
        addresses: ["00:00:00:00:00:ff"]
        router-port: r1s0
    port port02
        addresses: ["00:00:00:00:00:02"]
    port port01
        addresses: ["00:00:00:00:00:01"]
switch fe514df1-5e66-4962-9962-0ce69436eaf7 (sj)
    port sjrg
        type: router
        addresses: ["00:00:00:01:00:01"]
        router-port: rgsj
    port sjr1
        type: router
        addresses: ["00:00:00:01:00:02"]
        router-port: r1sj
switch 722300a5-73e1-42de-ae19-b7997f1f9a86 (s1)
    port port12
        addresses: ["00:00:00:00:01:02"]
    port s1r1
        type: router
        addresses: ["00:00:00:00:01:ff"]
        router-port: r1s1
    port port11
        addresses: ["00:00:00:00:01:01"]
router 3affb86c-5f1a-461d-bd07-b4a9580ed9bf (r1)
    port r1s0
        mac: "00:00:00:00:00:ff"
        networks: ["10.0.0.254/24"]
    port r1sj
        mac: "00:00:00:01:00:02"
        networks: ["10.1.0.2/30"]
    port r1s1
        mac: "00:00:00:00:01:ff"
        networks: ["10.0.1.254/24"]
router d041e224-b40b-46fd-8888-df5d93362579 (rg)
    port rgsj
        mac: "00:00:00:01:00:01"
        networks: ["10.1.0.1/30"]
    port rgsp
        mac: "00:00:00:02:00:ff"
        networks: ["10.2.0.254/24"]
[root@ovn1 ~]# ovn-sbctl show
Chassis "b5337e05-2495-4bf0-8728-25840472baa4"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.203"
        options: {csum="true"}
    Port_Binding port12
    Port_Binding port02
Chassis "c3f90802-4fd5-44ad-a338-154a9150e46f"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.201"
        options: {csum="true"}
    Port_Binding sjrg
    Port_Binding rgsp
    Port_Binding sprg
    Port_Binding rgsj
Chassis "8b9ad2e3-c1bc-4ea2-973e-a8bd1d38e502"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.202"
        options: {csum="true"}
    Port_Binding port11
    Port_Binding port01
~~~

Connect to ovn2 and simulate an external node:
~~~
nmcli connection delete 76505bbf-7aef-32f0-bae6-9497dcf93d2e
ip a a dev eth1 10.2.0.10/24
ip link set dev eth1 up
ip route add 10.0.1.0/24 via 10.2.0.254
ip route add 10.1.0.0/24 via 10.2.0.254
ip route add 10.0.0.0/24 via 10.2.0.254
~~~

Verfify:
~~~
[root@ovn2 ~]# ping 10.0.0.1
PING 10.0.0.1 (10.0.0.1) 56(84) bytes of data.
64 bytes from 10.0.0.1: icmp_seq=1 ttl=62 time=3.64 ms
^C
--- 10.0.0.1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 3.642/3.642/3.642/0.000 ms
[root@ovn2 ~]# ping 10.0.0.1 -c1 -W1
PING 10.0.0.1 (10.0.0.1) 56(84) bytes of data.
64 bytes from 10.0.0.1: icmp_seq=1 ttl=62 time=1.73 ms

--- 10.0.0.1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 1.734/1.734/1.734/0.000 ms
[root@ovn2 ~]# traceroute -n -I 10.0.0.1
traceroute to 10.0.0.1 (10.0.0.1), 30 hops max, 60 byte packets
 1  * * *
 2  * * *
 3  10.0.0.1  10.736 ms  10.726 ms  10.716 ms
[root@ovn2 ~]#  ip netns exec ns0 ping 10.2.0.10 -c1 -W1
PING 10.2.0.10 (10.2.0.10) 56(84) bytes of data.
64 bytes from 10.2.0.10: icmp_seq=1 ttl=62 time=1.62 ms

--- 10.2.0.10 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 1.615/1.615/1.615/0.000 ms
[root@ovn2 ~]#  ip netns exec ns0 traceroute -n -I 10.2.0.10 
traceroute to 10.2.0.10 (10.2.0.10), 30 hops max, 60 byte packets
 1  10.0.0.254  2.164 ms  1.773 ms  2.921 ms
 2  * * *
 3  10.2.0.10  5.078 ms *  4.308 ms
[root@ovn2 ~]# ping 10.0.1.2
PING 10.0.1.2 (10.0.1.2) 56(84) bytes of data.
64 bytes from 10.0.1.2: icmp_seq=1 ttl=62 time=4.91 ms
64 bytes from 10.0.1.2: icmp_seq=2 ttl=62 time=1.82 ms
^C
--- 10.0.1.2 ping statistics ---
2 packets transmitted, 2 received, 0% packet loss, time 1001ms
rtt min/avg/max/mdev = 1.815/3.360/4.906/1.545 ms
[root@ovn2 ~]# 
~~~

## Inspecting tunnel encapsulations ##

### Introduction ###

[http://www.openvswitch.org/support/dist-docs/ovn-architecture.7.html](http://www.openvswitch.org/support/dist-docs/ovn-architecture.7.html)
~~~
DESIGN DECISIONS
   Tunnel Encapsulations
       OVN annotates logical network packets that it sends from one hypervisor
       to  another  with the following three pieces of metadata, which are en‐
       coded in an encapsulation-specific fashion:

              •      24-bit logical datapath identifier, from  the  tunnel_key
                     column in the OVN Southbound Datapath_Binding table.

              •      15-bit  logical ingress port identifier. ID 0 is reserved
                     for internal use within OVN. IDs 1 through 32767,  inclu‐
                     sive,  may  be  assigned  to  logical ports (see the tun‐
                     nel_key column in the OVN Southbound Port_Binding table).

              •      16-bit logical egress  port  identifier.  IDs  0  through
                     32767 have the same meaning as for logical ingress ports.
                     IDs 32768 through 65535, inclusive, may  be  assigned  to
                     logical  multicast  groups  (see the tunnel_key column in
                     the OVN Southbound Multicast_Group table).

       For hypervisor-to-hypervisor traffic, OVN supports only Geneve and  STT
       encapsulations, for the following reasons:

              •      Only STT and Geneve support the large amounts of metadata
                     (over 32 bits per packet) that  OVN  uses  (as  described
                     above).

              •      STT  and  Geneve  use  randomized UDP or TCP source ports
                     that allows efficient distribution among  multiple  paths
                     in environments that use ECMP in their underlay.

              •      NICs  are  available to offload STT and Geneve encapsula‐
                     tion and decapsulation.

       Due to its flexibility, the preferred encapsulation between hypervisors
       is Geneve. For Geneve encapsulation, OVN transmits the logical datapath
       identifier in the Geneve VNI. OVN transmits  the  logical  ingress  and
       logical  egress  ports  in  a  TLV  with class 0x0102, type 0x80, and a
       32-bit value encoded as follows, from MSB to LSB:

         1       15          16
       +---+------------+-----------+
       |rsv|ingress port|egress port|
       +---+------------+-----------+
         0


       Environments whose NICs lack Geneve offload may prefer  STT  encapsula‐
       tion  for  performance  reasons. For STT encapsulation, OVN encodes all
       three pieces of logical metadata in the STT 64-bit tunnel  ID  as  fol‐
       lows, from MSB to LSB:

           9          15          16         24
       +--------+------------+-----------+--------+
       |reserved|ingress port|egress port|datapath|
       +--------+------------+-----------+--------+
           0


       For connecting to gateways, in addition to Geneve and STT, OVN supports
       VXLAN, because only  VXLAN  support  is  common  on  top-of-rack  (ToR)
       switches. Currently, gateways have a feature set that matches the capa‐
       bilities as defined by the VTEP schema, so fewer bits of  metadata  are
       necessary.  In  the future, gateways that do not support encapsulations
       with large amounts of metadata may continue to have a  reduced  feature
       set.
~~~

## Distributed gateway router ##

[https://developers.redhat.com/blog/2018/11/08/how-to-create-an-open-virtual-network-distributed-gateway-router/](https://developers.redhat.com/blog/2018/11/08/how-to-create-an-open-virtual-network-distributed-gateway-router/)

## Resources ##

* [https://blog.scottlowe.org/2016/12/09/using-ovn-with-kvm-libvirt/](https://blog.scottlowe.org/2016/12/09/using-ovn-with-kvm-libvirt/)
* [https://baturin.org/docs/iproute2/](https://baturin.org/docs/iproute2/)
* [http://dani.foroselectronica.es/multinode-ovn-setup-509/](http://dani.foroselectronica.es/multinode-ovn-setup-509/)
* [https://hustcat.github.io/ovn-gateway-practice/](https://hustcat.github.io/ovn-gateway-practice/)
* [http://www.openvswitch.org/support/dist-docs/ovn-architecture.7.html](http://www.openvswitch.org/support/dist-docs/ovn-architecture.7.html)
* [https://www.redhat.com/en/blog/what-geneve](https://www.redhat.com/en/blog/what-geneve)
* [https://tools.ietf.org/html/draft-davie-stt-08](https://tools.ietf.org/html/draft-davie-stt-08)
* [https://tools.ietf.org/html/draft-ietf-nvo3-geneve-16](https://tools.ietf.org/html/draft-ietf-nvo3-geneve-16)
* [https://developers.redhat.com/blog/2018/11/08/how-to-create-an-open-virtual-network-distributed-gateway-router/](https://developers.redhat.com/blog/2018/11/08/how-to-create-an-open-virtual-network-distributed-gateway-router/)
* [http://docs.openvswitch.org/en/latest/intro/install/bash-completion/](http://docs.openvswitch.org/en/latest/intro/install/bash-completion/)
* [https://blog.russellbryant.net/2017/05/30/ovn-geneve-vs-vxlan-does-it-matter/](https://blog.russellbryant.net/2017/05/30/ovn-geneve-vs-vxlan-does-it-matter/)
