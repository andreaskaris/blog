# OVN standalone on Fedora  #

## Base setup ##

### Single node DB on ovn1 ###

#### Configuration and installation ####

For this tutorial, I am using Fedora 42.

First, we will set up 3 virtual machines. The OVN database will be non-clustered with ovn1 as the database node, and ovn2 and ovn3 will join ovn1's DB.

All nodes have 2 interfaces. One management interface on enp1s0. DNS names ovn1, ovn2, ovn3 map to the IP addresses on enp1s0. Interface enp2s0 has no IP addresses and will be used for the dataplane (attached to br-provider).

IP address configuration:
```
declare -A hostnameIPs
hostnameIPs=( ["ovn1"]="192.168.122.182/24" ["ovn2"]="192.168.122.159/24" ["ovn3"]="192.168.122.107/24" )
echo "Setting IP address to ${hostnameIPs[$(hostname)]}"
nmcli conn mod "Wired connection 1" connection.id enp1s0
nmcli conn mod enp1s0 ipv4.method manual ipv4.dns 192.168.122.1 ipv4.gateway 192.168.122.1 ipv4.addresses ${hostnameIPs[$(hostname)]} ipv6.method disabled
nmcli conn mod "Wired connection 2" connection.id enp2s0
nmcli conn mod enp2s0 ipv4.method disabled ipv6.method disabled
systemctl restart NetworkManager
```

Hostname to IP address mapping:
```
cat <<EOF >> /etc/hosts

192.168.122.182 ovn1
192.168.122.159 ovn2
192.168.122.107 ovn3
EOF
```

Install Open vSwitch on all nodes. In this case, I built newest upstream from source. But it is also possible (and
easier) to use the package manager of the given distribution.
See https://docs.openvswitch.org/en/latest/intro/install/

Install OVN on all nodes. I built the newest upstream from source. But it is also possible (and easier) to use
the package manager of the given distribution.
See https://docs.ovn.org/en/latest/intro/install/fedora.html

 and OVN on all nodes:
```
yum install ovn -y
yum install ovn-central -y
yum install ovn-host -y
```

Enable Open vSwitch and ovn-controller on all hosts and start right away:
```
systemctl enable --now openvswitch
systemctl enable --now ovn-controller
```

ovn1 is going to be our control plane node for the cluster. Execute on ovn1:
```
echo 'OVN_NORTHD_OPTS="--db-nb-addr=ovn1 --db-nb-create-insecure-remote=yes --db-sb-addr=ovn1  --db-sb-create-insecure-remote=yes  --db-nb-cluster-local-addr=ovn1 --db-sb-cluster-local-addr=ovn1 --ovn-northd-nb-db=tcp:ovn1:6641 --ovn-northd-sb-db=tcp:ovn1:6642"' >> /etc/sysconfig/ovn
systemctl enable --now ovn-northd
```

Note that the above will start a single node cluster. The OVN man page contains an example for a full 3 node clustered DB: https://www.ovn.org/support/dist-docs/ovn-ctl.8.txt

Now, configure OVS to connect to the OVN DBs. Note that geneve / vxlan tunnel must be set to the IP address of the respective
node itself; DNS entries do not work:

On control-plane node ovn1:

```
# [root@ovn1 ~]# 
ovs-vsctl set open . external-ids:ovn-remote=tcp:ovn1:6642
ovs-vsctl set open . external-ids:ovn-encap-type=geneve
# could also be: ovs-vsctl set open . external-ids:ovn-encap-type=geneve,vxlan
ovs-vsctl set open . external-ids:ovn-encap-ip=192.168.122.182
ovs-vsctl set open . external-ids:ovn-bridge=br-int
```

On node ovn2:

```
# [root@ovn2 ~]# 
ovs-vsctl set open . external-ids:ovn-remote=tcp:ovn1:6642
ovs-vsctl set open . external-ids:ovn-encap-type=geneve
# could also be: ovs-vsctl set open . external-ids:ovn-encap-type=geneve,vxlan
ovs-vsctl set open . external-ids:ovn-encap-ip=192.168.122.159
ovs-vsctl set open . external-ids:ovn-bridge=br-int
```

On node ovn3:

```
# [root@ovn3 ~]# 
ovs-vsctl set open . external-ids:ovn-remote=tcp:ovn1:6642
ovs-vsctl set open . external-ids:ovn-encap-type=geneve
# could also be: ovs-vsctl set open . external-ids:ovn-encap-type=geneve,vxlan
ovs-vsctl set open . external-ids:ovn-encap-ip=192.168.122.107
ovs-vsctl set open . external-ids:ovn-bridge=br-int
```

#### Verification ####

Verify on ovn1 with:

```
ovs-vsctl show
ovn-sbctl show
```

```
[root@ovn1 ~]# ovs-vsctl show
1717f06c-b2be-42f0-99c2-5081333e6ebb
    Bridge br-int
        fail_mode: secure
        datapath_type: system
        Port ovn-8de4d5-0
            Interface ovn-8de4d5-0
                type: geneve
                options: {csum="true", key=flow, local_ip="192.168.122.182", remote_ip="192.168.122.107"}
        Port br-int
            Interface br-int
                type: internal
        Port ovn-108c2b-0
            Interface ovn-108c2b-0
                type: geneve
                options: {csum="true", key=flow, local_ip="192.168.122.182", remote_ip="192.168.122.159"}
    ovs_version: "3.5.90-1.fc42"
[root@ovn1 ~]# ovn-sbctl show
Chassis "108c2b72-5ab9-43a0-9617-55f23f16482a"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.159"
        options: {csum="true"}
Chassis "8de4d500-ab50-4ecc-a6df-3b5387efdf81"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.107"
        options: {csum="true"}
Chassis "dacba18d-52f9-4adb-8b3d-8ca27b700a73"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.182"
        options: {csum="true"}
```

On ovn2 and ovn3, only chick with `ovs-vsctl show`.

```
[root@ovn2 ~]# ovs-vsctl show
5692cb1b-bc2c-4c9d-a9c7-9001e293d013
    Bridge br-int
        fail_mode: secure
        datapath_type: system
        Port br-int
            Interface br-int
                type: internal
        Port ovn-8de4d5-0
            Interface ovn-8de4d5-0
                type: geneve
                options: {csum="true", key=flow, local_ip="192.168.122.159", remote_ip="192.168.122.107"}
        Port ovn-dacba1-0
            Interface ovn-dacba1-0
                type: geneve
                options: {csum="true", key=flow, local_ip="192.168.122.159", remote_ip="192.168.122.182"}
    ovs_version: "3.5.90-1.fc42"
```

```
[root@ovn3 ovn]# ovs-vsctl show
53739cb0-98fa-4173-a81f-547f39528225
    Bridge br-int
        fail_mode: secure
        datapath_type: system
        Port ovn-108c2b-0
            Interface ovn-108c2b-0
                type: geneve
                options: {csum="true", key=flow, local_ip="192.168.122.107", remote_ip="192.168.122.159"}
        Port br-int
            Interface br-int
                type: internal
        Port ovn-dacba1-0
            Interface ovn-dacba1-0
                type: geneve
                options: {csum="true", key=flow, local_ip="192.168.122.107", remote_ip="192.168.122.182"}
    ovs_version: "3.5.90-1.fc42"
```

You can also run `ovn-appctl connection-status` which should yield `connected` on each host.

## Adding persistent namespace setup

If we want our test setup to survive reboots, we must make our namespaces persistent. The following will run namespace
setup once on a reboot.

Run this on all nodes:

```
mkdir /opt/ns-setup
cat <<'EOF' > /etc/systemd/system/ns-setup.service
[Unit]
Description=Run all NS setup scripts inside /opt/ns-setup
After=network-online.target
Wants=network-online.target

[Service]
Type=oneshot
ExecStart=/bin/bash -c 'for f in /opt/ns-setup/*; do $f; done'
RemainAfterExit=no

[Install]
WantedBy=multi-user.target
EOF
systemctl daemon-reload
systemctl enable ns-setup
```

Note, if SELinux is enabled, this will fail because my scripts below create the namespace directly inside bash with
`ip netns add`. You'd see the following:

```
# journalctl -u ns-setup | grep mount
Jun 26 15:33:56 ovn3 bash[118808]: mount of /sys failed: Permission denied
```

If this is the case, have it fail once (to log an error in /var/log/audit/audit.log) and then run:

```
grep "avc.*denied.*mounton.*ifconfig.*mock_var_lib_t" /var/log/audit/audit.log | audit2allow -M netns_fix
semodule -i netns_fix.pp
```

## Adding virtual network with one virtual switch s0 ##

On ovn1, configure a logical switch and port:
```
ovn-nbctl ls-add s0
ovn-nbctl lsp-add s0 port01
ovn-nbctl lsp-set-addresses port01 00:00:00:00:00:01
ovn-nbctl lsp-add s0 port02
ovn-nbctl lsp-set-addresses port02 00:00:00:00:00:02
```

Now, "real" ports need to be wired to the above ports. Note that the logical port name has to match the `external_ids:iface-id` identifier. If we added  `ovn-nbctl lsp-add s0 foo` instead of `port01` then we would have to set `ovs-vsctl set Interface port01 external_ids:iface-id=foo` on ovn2. Note that we need to know the MAC address of that port. Therefore, when creating the veth, we are making sure to create it with the correct MAC address.

On ovn2, execute:
```
cat <<EOF > /opt/ns-setup/ns0.sh
#!/bin/bash -eux
ip link add name veth1 type veth peer name port01
ip netns add ns0
ip link set dev veth1 netns ns0
ip netns exec ns0 ip link set dev lo up
ip netns exec ns0 ip link set dev veth1 up
ip netns exec ns0 ip link set veth1 address 00:00:00:00:00:01
ip netns exec ns0 ip address add 10.0.0.1/24 dev veth1
ip link set dev port01 up
ovs-vsctl add-port br-int port01 || true
ovs-vsctl set Interface port01 external_ids:iface-id=port01 || true
EOF
chmod +x /opt/ns-setup/ns0.sh
```

On ovn3, execute:
```
cat <<EOF > /opt/ns-setup/ns0.sh
#!/bin/bash -eux
ip link add name veth2 type veth peer name port02
ip netns add ns0
ip link set dev veth2 netns ns0
ip netns exec ns0 ip link set dev lo up
ip netns exec ns0 ip link set dev veth2 up
ip netns exec ns0 ip link set veth2 address 00:00:00:00:00:02
ip netns exec ns0 ip address add 10.0.0.2/24 dev veth2
ip link set dev port02 up
ovs-vsctl add-port br-int port02 external_ids:iface-id=port02 || true
ovs-vsctl set Interface port02 external_ids:iface-id=port02 || true
EOF
chmod +x /opt/ns-setup/ns0.sh
```

Verify the new configuration on ovn1:
```
ovn-nbctl show
ovn-sbctl show
```

```
[root@ovn1 ~]# ovn-sbctl show
Chassis "108c2b72-5ab9-43a0-9617-55f23f16482a"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.159"
        options: {csum="true"}
    Port_Binding port01
Chassis "8de4d500-ab50-4ecc-a6df-3b5387efdf81"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.107"
        options: {csum="true"}
    Port_Binding port02
Chassis "dacba18d-52f9-4adb-8b3d-8ca27b700a73"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.182"
        options: {csum="true"}
[root@ovn1 ~]# ovn-nbctl show
switch 70a89317-1bc0-4fcb-825d-c93754a5a6ca (s0)
    port port01
        addresses: ["00:00:00:00:00:01"]
    port port02
        addresses: ["00:00:00:00:00:02"]
```
> **Note:** The southbound database now shows port bindings.

You can also check the OVN Southbound logical flows:

```
[root@ovn1 ~]# ovn-sbctl lflow-list
Datapath: "s0" (49345921-e4da-4816-bf85-b092efe8eb29)  Pipeline: ingress
  table=0 (ls_in_check_port_sec), priority=110  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && eth.src == 00:00:00:00:00:01 && outport == "port01" && !is_chassis_resident("port01") && flags.tunnel_rx == 1), action=(outport <-> inport; next;)
  table=0 (ls_in_check_port_sec), priority=110  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && eth.src == 00:00:00:00:00:02 && outport == "port02" && !is_chassis_resident("port02") && flags.tunnel_rx == 1), action=(outport <-> inport; next;)
  table=0 (ls_in_check_port_sec), priority=105  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && flags.tunnel_rx == 1), action=(drop;)
  table=0 (ls_in_check_port_sec), priority=100  , match=(eth.src[40]), action=(drop;)
  table=0 (ls_in_check_port_sec), priority=100  , match=(vlan.present), action=(drop;)
  table=0 (ls_in_check_port_sec), priority=50   , match=(1), action=(reg0[15] = check_in_port_sec(); next;)
  table=1 (ls_in_apply_port_sec), priority=50   , match=(reg0[15] == 1), action=(drop;)
  table=1 (ls_in_apply_port_sec), priority=0    , match=(1), action=(next;)
  table=2 (ls_in_mirror       ), priority=0    , match=(1), action=(next;)
  table=3 (ls_in_lookup_fdb   ), priority=0    , match=(1), action=(next;)
  table=4 (ls_in_put_fdb      ), priority=0    , match=(1), action=(next;)
  table=5 (ls_in_pre_acl      ), priority=110  , match=(eth.dst == $svc_monitor_mac), action=(next;)
  table=5 (ls_in_pre_acl      ), priority=0    , match=(1), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) ||(ip6 && icmp6.type == 2 && icmp6.code == 0)) && flags.tunnel_rx == 1), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(eth.dst == $svc_monitor_mac), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(eth.mcast), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(nd || nd_rs || nd_ra || mldv1 || mldv2), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(reg0[16] == 1), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=0    , match=(1), action=(next;)
  table=7 (ls_in_pre_stateful ), priority=115  , match=(reg0[2] == 1 && ip.is_frag), action=(reg0[19] = 1; ct_lb_mark;)
  table=7 (ls_in_pre_stateful ), priority=110  , match=(reg0[2] == 1), action=(ct_lb_mark;)
  table=7 (ls_in_pre_stateful ), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=7 (ls_in_pre_stateful ), priority=0    , match=(1), action=(next;)
  table=8 (ls_in_acl_hint     ), priority=65535, match=(1), action=(next;)
  table=9 (ls_in_acl_eval     ), priority=65535, match=(1), action=(next;)
  table=9 (ls_in_acl_eval     ), priority=65532, match=(nd || nd_ra || nd_rs || mldv1 || mldv2), action=(reg8[16] = 1; next;)
  table=10(ls_in_acl_sample   ), priority=0    , match=(1), action=(next;)
  table=11(ls_in_acl_action   ), priority=0    , match=(1), action=(next;)
  table=12(ls_in_qos          ), priority=0    , match=(1), action=(next;)
  table=13(ls_in_lb_aff_check ), priority=0    , match=(1), action=(next;)
  table=14(ls_in_lb           ), priority=0    , match=(1), action=(next;)
  table=15(ls_in_lb_aff_learn ), priority=0    , match=(1), action=(next;)
  table=16(ls_in_pre_hairpin  ), priority=0    , match=(1), action=(next;)
  table=17(ls_in_nat_hairpin  ), priority=0    , match=(1), action=(next;)
  table=18(ls_in_hairpin      ), priority=0    , match=(1), action=(next;)
  table=19(ls_in_acl_after_lb_eval), priority=65532, match=(nd || nd_ra || nd_rs || mldv1 || mldv2), action=(reg8[16] = 1; next;)
  table=19(ls_in_acl_after_lb_eval), priority=0    , match=(1), action=(next;)
  table=20(ls_in_acl_after_lb_sample), priority=0    , match=(1), action=(next;)
  table=21(ls_in_acl_after_lb_action), priority=0    , match=(1), action=(next;)
  table=22(ls_in_stateful     ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 0), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_label.acl_id = reg2[16..31]; }; next;)
  table=22(ls_in_stateful     ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 1), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_mark.obs_stage = reg8[19..20]; ct_mark.obs_collector_id = reg8[8..15]; ct_label.obs_point_id = reg9; ct_label.acl_id = reg2[16..31]; }; next;)
  table=22(ls_in_stateful     ), priority=0    , match=(1), action=(next;)
  table=23(ls_in_arp_rsp      ), priority=0    , match=(1), action=(next;)
  table=24(ls_in_dhcp_options ), priority=0    , match=(1), action=(next;)
  table=25(ls_in_dhcp_response), priority=0    , match=(1), action=(next;)
  table=26(ls_in_dns_lookup   ), priority=0    , match=(1), action=(next;)
  table=27(ls_in_dns_response ), priority=0    , match=(1), action=(next;)
  table=28(ls_in_external_port), priority=0    , match=(1), action=(next;)
  table=29(ls_in_l2_lkup      ), priority=110  , match=(eth.dst == $svc_monitor_mac && (tcp || icmp || icmp6)), action=(handle_svc_check(inport);)
  table=29(ls_in_l2_lkup      ), priority=70   , match=(eth.mcast), action=(outport = "_MC_flood"; output;)
  table=29(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:01), action=(outport = "port01"; output;)
  table=29(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:02), action=(outport = "port02"; output;)
  table=29(ls_in_l2_lkup      ), priority=0    , match=(1), action=(outport = get_fdb(eth.dst); next;)
  table=30(ls_in_l2_unknown   ), priority=50   , match=(outport == "none"), action=(drop;)
  table=30(ls_in_l2_unknown   ), priority=0    , match=(1), action=(output;)
Datapath: "s0" (49345921-e4da-4816-bf85-b092efe8eb29)  Pipeline: egress
  table=0 (ls_out_lookup_fdb  ), priority=0    , match=(1), action=(next;)
  table=1 (ls_out_put_fdb     ), priority=0    , match=(1), action=(next;)
  table=2 (ls_out_pre_acl     ), priority=110  , match=(eth.src == $svc_monitor_mac), action=(next;)
  table=2 (ls_out_pre_acl     ), priority=0    , match=(1), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(eth.mcast), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(eth.src == $svc_monitor_mac), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(nd || nd_rs || nd_ra || mldv1 || mldv2), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(reg0[16] == 1), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=0    , match=(1), action=(next;)
  table=4 (ls_out_pre_stateful), priority=110  , match=(reg0[2] == 1), action=(ct_lb_mark;)
  table=4 (ls_out_pre_stateful), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=4 (ls_out_pre_stateful), priority=0    , match=(1), action=(next;)
  table=5 (ls_out_acl_hint    ), priority=65535, match=(1), action=(next;)
  table=6 (ls_out_acl_eval    ), priority=65535, match=(1), action=(next;)
  table=6 (ls_out_acl_eval    ), priority=65532, match=(nd || nd_ra || nd_rs || mldv1 || mldv2), action=(reg8[16] = 1; next;)
  table=7 (ls_out_acl_sample  ), priority=0    , match=(1), action=(next;)
  table=8 (ls_out_acl_action  ), priority=0    , match=(1), action=(next;)
  table=9 (ls_out_mirror      ), priority=0    , match=(1), action=(next;)
  table=10(ls_out_qos         ), priority=0    , match=(1), action=(next;)
  table=11(ls_out_stateful    ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 0), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_label.acl_id = reg2[16..31]; }; next;)
  table=11(ls_out_stateful    ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 1), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_mark.obs_stage = reg8[19..20]; ct_mark.obs_collector_id = reg8[8..15]; ct_label.obs_point_id = reg9; ct_label.acl_id = reg2[16..31]; }; next;)
  table=11(ls_out_stateful    ), priority=0    , match=(1), action=(next;)
  table=12(ls_out_check_port_sec), priority=100  , match=(eth.mcast), action=(reg0[15] = 0; next;)
  table=12(ls_out_check_port_sec), priority=0    , match=(1), action=(reg0[15] = check_out_port_sec(); next;)
  table=13(ls_out_apply_port_sec), priority=50   , match=(reg0[15] == 1), action=(drop;)
  table=13(ls_out_apply_port_sec), priority=0    , match=(1), action=(output;)
```

Last but not least, run a test ping between the namespaces on both hosts:

```
[root@ovn2 ~]# ip netns exec ns0 ping -c1 -W1 10.0.0.2
PING 10.0.0.2 (10.0.0.2) 56(84) bytes of data.
64 bytes from 10.0.0.2: icmp_seq=1 ttl=64 time=1.37 ms

--- 10.0.0.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 1.370/1.370/1.370/0.000 ms
```

## Adding virtual network with one virtual switch s1 ##

On ovn1, configure a logical switch and port:

```
ovn-nbctl ls-add s1
ovn-nbctl lsp-add s1 port11
ovn-nbctl lsp-set-addresses port11 00:00:00:00:01:01
ovn-nbctl lsp-add s1 port12
ovn-nbctl lsp-set-addresses port12 00:00:00:00:01:02
```

Now, "real" ports need to be wired to the above ports. Note that the logical port name has to match the `external_ids:iface-id` identifier. If we added  `ovn-nbctl lsp-add s1 foo` instead of `port11`, then we would have to set `ovs-vsctl set Interface port11 external_ids:iface-id=foo` on ovn2. Note that we need to know the MAC address of that port. Therefore, when creating the veth, we are making sure to create it with the correct MAC address.

On ovn2, execute:

```
cat <<EOF > /opt/ns-setup/ns1.sh
#!/bin/bash -eux
ip link add name veth11 type veth peer name port11
ip netns add ns1
ip link set dev veth11 netns ns1
ip netns exec ns1 ip link set dev lo up
ip netns exec ns1 ip link set dev veth11 up
ip netns exec ns1 ip link set veth11 address 00:00:00:00:01:01
ip netns exec ns1 ip address add 10.0.1.1/24 dev veth11
ip link set dev port11 up
ovs-vsctl add-port br-int port11 || true
ovs-vsctl set Interface port11 external_ids:iface-id=port11 || true
EOF
chmod +x /opt/ns-setup/ns1.sh
```

On ovn3, execute:

```
cat <<EOF > /opt/ns-setup/ns1.sh
#!/bin/bash -eux
ip link add name veth12 type veth peer name port12
ip netns add ns1
ip link set dev veth12 netns ns1
ip netns exec ns1 ip link set dev lo up
ip netns exec ns1 ip link set dev veth12 up
ip netns exec ns1 ip link set veth12 address 00:00:00:00:01:02
ip netns exec ns1 ip address add 10.0.1.2/24 dev veth12
ip link set dev port12 up
ovs-vsctl add-port br-int port12 external_ids:iface-id=port12 || true
ovs-vsctl set Interface port12 external_ids:iface-id=port12 || true
EOF
chmod +x /opt/ns-setup/ns1.sh
```

Verify the new configuration on ovn1:

```
ovn-nbctl show
ovn-sbctl show
```

```
[root@ovn1 ~]# ovn-nbctl show
switch 313fb333-5c33-4bf2-8936-72514bc20de3 (s1)
    port port11
        addresses: ["00:00:00:00:01:01"]
    port port12
        addresses: ["00:00:00:00:01:02"]
switch 70a89317-1bc0-4fcb-825d-c93754a5a6ca (s0)
    port port01
        addresses: ["00:00:00:00:00:01"]
    port port02
        addresses: ["00:00:00:00:00:02"]
[root@ovn1 ~]# ovn-sbctl show
Chassis "8de4d500-ab50-4ecc-a6df-3b5387efdf81"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.107"
        options: {csum="true"}
    Port_Binding port12
    Port_Binding port02
Chassis "108c2b72-5ab9-43a0-9617-55f23f16482a"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.159"
        options: {csum="true"}
    Port_Binding port11
    Port_Binding port01
Chassis "dacba18d-52f9-4adb-8b3d-8ca27b700a73"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.182"
        options: {csum="true"}
```
> **Note:** The southbound database now shows port bindings.

Verify logical flows, the same as earlier.

```
ovn-sbctl lflow-list
```

Last but not least, run a test ping between the namespaces on both hosts:

```
[root@ovn2 ~]# ip netns exec ns1 ping 10.0.1.2 -c1 -W1
PING 10.0.1.2 (10.0.1.2) 56(84) bytes of data.
64 bytes from 10.0.1.2: icmp_seq=1 ttl=64 time=2.50 ms

--- 10.0.1.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 2.496/2.496/2.496/0.000 ms
```

## Adding virtual router r1 ## 

### Introduction ###

Next, we want to be able to route between the two namespaces. In order to be able to do so, we must add a virtual router
in between the two switches.

https://www.ovn.org/support/dist-docs/ovn-architecture.7.html
```
   Logical Routers and Logical Patch Ports
       Typically logical routers and logical patch ports do not have a  physi‐
       cal  location  and  effectively reside on every hypervisor. This is the
       case for logical  patch  ports  between  logical  routers  and  logical
       switches behind those logical routers, to which VMs (and VIFs) attach.

       Consider a packet sent from one virtual machine or container to another
       VM  or  container  that  resides on a different subnet. The packet will
       traverse tables 0 to 65 as described in the previous section  Architec‐␈‐
       tural  Physical Life Cycle of a Packet, using the logical datapath rep‐
       resenting the logical switch that the sender is attached to.  At  table
       42, the packet will use the fallback flow that resubmits locally to ta‐
       ble 43 on the same hypervisor. In this case, all of the processing from
       table 0 to table 65 occurs on the hypervisor where the sender resides.

       When  the packet reaches table 65, the logical egress port is a logical
       patch port. ovn-controller implements output to the  logical  patch  is
       packet  by cloning and resubmitting directly to the first OpenFlow flow
       table in the ingress pipeline, setting the logical ingress port to  the
       peer  logical patch port, and using the peer logical patch port’s logi‐
       cal datapath (that represents the logical router).

       The packet re-enters the ingress pipeline in order to traverse tables 8
       to 65 again, this time using the logical datapath representing the log‐
       ical router. The processing continues as described in the previous sec‐
       tion Architectural Physical Life Cycle of a  Packet.  When  the  packet
       reaches  table 65, the logical egress port will once again be a logical
       patch port. In the same manner as described above, this  logical  patch
       port  will  cause  the packet to be resubmitted to OpenFlow tables 8 to
       65, this time using  the  logical  datapath  representing  the  logical
       switch that the destination VM or container is attached to.

       The packet traverses tables 8 to 65 a third and final time. If the des‐
       tination  VM or container resides on a remote hypervisor, then table 39
       will send the packet on a tunnel port from the sender’s  hypervisor  to
       the remote hypervisor. Finally table 65 will output the packet directly
       to the destination VM or container.
```

### Setup ###

Configure namespace routes for virtual routing - on both hosts, run:
```
ip netns exec ns0 ip r add default via 10.0.0.254
ip netns exec ns1 ip r add default via 10.0.1.254
echo "ip netns exec ns0 ip r add default via 10.0.0.254" >> /opt/ns-setup/ns0.sh 
echo "ip netns exec ns1 ip r add default via 10.0.1.254" >> /opt/ns-setup/ns1.sh
```

Now, set up the OVN northbound database router and router ports - run this on ovn1:
```
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
```

Verify the new configuration:
```
ovn-nbctl show
ovn-sbctl show
```

```
[root@ovn1 ~]# ovn-nbctl show
switch 313fb333-5c33-4bf2-8936-72514bc20de3 (s1)
    port s1r1
        type: router
        addresses: ["00:00:00:00:01:ff"]
        router-port: r1s1
    port port11
        addresses: ["00:00:00:00:01:01"]
    port port12
        addresses: ["00:00:00:00:01:02"]
switch 70a89317-1bc0-4fcb-825d-c93754a5a6ca (s0)
    port port01
        addresses: ["00:00:00:00:00:01"]
    port s0r1
        type: router
        addresses: ["00:00:00:00:00:ff"]
        router-port: r1s0
    port port02
        addresses: ["00:00:00:00:00:02"]
router fb0101fd-881e-4aa2-ae05-16a93428dc53 (r1)
    port r1s1
        mac: "00:00:00:00:01:ff"
        ipv6-lla: "fe80::200:ff:fe00:1ff"
        networks: ["10.0.1.254/24"]
    port r1s0
        mac: "00:00:00:00:00:ff"
        ipv6-lla: "fe80::200:ff:fe00:ff"
        networks: ["10.0.0.254/24"]
[root@ovn1 ~]# ovn-sbctl show
Chassis "8de4d500-ab50-4ecc-a6df-3b5387efdf81"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.107"
        options: {csum="true"}
    Port_Binding port12
    Port_Binding port02
Chassis "108c2b72-5ab9-43a0-9617-55f23f16482a"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.159"
        options: {csum="true"}
    Port_Binding port01
    Port_Binding port11
Chassis "dacba18d-52f9-4adb-8b3d-8ca27b700a73"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.182"
        options: {csum="true"}
```

Verify logical flows:
```
ovn-sbctl lflow-list
```

```
[root@ovn1 ~]#  ovn-sbctl lflow-list
Datapath: "r1" (1287fc2b-1e3b-4bf9-8cf8-db890e663dc3)  Pipeline: ingress
  table=0 (lr_in_admission    ), priority=110  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && flags.tunnel_rx == 1), action=(drop;)
  table=0 (lr_in_admission    ), priority=100  , match=(vlan.present || eth.src[40]), action=(drop;)
  table=0 (lr_in_admission    ), priority=50   , match=(eth.dst == 00:00:00:00:00:ff && inport == "r1s0"), action=(xreg0[0..47] = 00:00:00:00:00:ff; next;)
  table=0 (lr_in_admission    ), priority=50   , match=(eth.dst == 00:00:00:00:01:ff && inport == "r1s1"), action=(xreg0[0..47] = 00:00:00:00:01:ff; next;)
  table=0 (lr_in_admission    ), priority=50   , match=(eth.mcast && inport == "r1s0"), action=(xreg0[0..47] = 00:00:00:00:00:ff; next;)
  table=0 (lr_in_admission    ), priority=50   , match=(eth.mcast && inport == "r1s1"), action=(xreg0[0..47] = 00:00:00:00:01:ff; next;)
  table=0 (lr_in_admission    ), priority=0    , match=(1), action=(drop;)
  table=1 (lr_in_lookup_neighbor), priority=100  , match=(arp.op == 2), action=(reg9[2] = lookup_arp(inport, arp.spa, arp.sha); next;)
  table=1 (lr_in_lookup_neighbor), priority=100  , match=(inport == "r1s0" && arp.spa == 10.0.0.0/24 && arp.op == 1), action=(reg9[2] = lookup_arp(inport, arp.spa, arp.sha); next;)
  table=1 (lr_in_lookup_neighbor), priority=100  , match=(inport == "r1s1" && arp.spa == 10.0.1.0/24 && arp.op == 1), action=(reg9[2] = lookup_arp(inport, arp.spa, arp.sha); next;)
  table=1 (lr_in_lookup_neighbor), priority=100  , match=(nd_na), action=(reg9[2] = lookup_nd(inport, nd.target, nd.tll); next;)
  table=1 (lr_in_lookup_neighbor), priority=100  , match=(nd_ns), action=(reg9[2] = lookup_nd(inport, ip6.src, nd.sll); next;)
  table=1 (lr_in_lookup_neighbor), priority=0    , match=(1), action=(reg9[2] = 1; next;)
  table=2 (lr_in_learn_neighbor), priority=100  , match=(reg9[2] == 1), action=(mac_cache_use; next;)
  table=2 (lr_in_learn_neighbor), priority=95   , match=(nd_na && nd.tll == 0), action=(put_nd(inport, nd.target, eth.src); next;)
  table=2 (lr_in_learn_neighbor), priority=95   , match=(nd_ns && (ip6.src == 0 || nd.sll == 0)), action=(next;)
  table=2 (lr_in_learn_neighbor), priority=90   , match=(arp), action=(put_arp(inport, arp.spa, arp.sha); next;)
  table=2 (lr_in_learn_neighbor), priority=90   , match=(nd_na), action=(put_nd(inport, nd.target, nd.tll); next;)
  table=2 (lr_in_learn_neighbor), priority=90   , match=(nd_ns), action=(put_nd(inport, ip6.src, nd.sll); next;)
  table=2 (lr_in_learn_neighbor), priority=0    , match=(1), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=100  , match=(ip4.src == {10.0.0.254, 10.0.0.255} && reg9[0] == 0), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=100  , match=(ip4.src == {10.0.1.254, 10.0.1.255} && reg9[0] == 0), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=100  , match=(ip4.src_mcast ||ip4.src == 255.255.255.255 || ip4.src == 127.0.0.0/8 || ip4.dst == 127.0.0.0/8 || ip4.src == 0.0.0.0/8 || ip4.dst == 0.0.0.0/8), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=90   , match=(inport == "r1s0" && arp.op == 1 && arp.tpa == 10.0.0.254 && arp.spa == 10.0.0.0/24), action=(eth.dst = eth.src; eth.src = xreg0[0..47]; arp.op = 2; /* ARP reply */ arp.tha = arp.sha; arp.sha = xreg0[0..47]; arp.tpa <-> arp.spa; outport = inport; flags.loopback = 1; output;)
  table=3 (lr_in_ip_input     ), priority=90   , match=(inport == "r1s0" && ip6.dst == {fe80::200:ff:fe00:ff, ff02::1:ff00:ff} && nd_ns && nd.target == fe80::200:ff:fe00:ff), action=(nd_na_router { eth.src = xreg0[0..47]; ip6.src = nd.target; nd.tll = xreg0[0..47]; outport = inport; flags.loopback = 1; output; };)
  table=3 (lr_in_ip_input     ), priority=90   , match=(inport == "r1s1" && arp.op == 1 && arp.tpa == 10.0.1.254 && arp.spa == 10.0.1.0/24), action=(eth.dst = eth.src; eth.src = xreg0[0..47]; arp.op = 2; /* ARP reply */ arp.tha = arp.sha; arp.sha = xreg0[0..47]; arp.tpa <-> arp.spa; outport = inport; flags.loopback = 1; output;)
  table=3 (lr_in_ip_input     ), priority=90   , match=(inport == "r1s1" && ip6.dst == {fe80::200:ff:fe00:1ff, ff02::1:ff00:1ff} && nd_ns && nd.target == fe80::200:ff:fe00:1ff), action=(nd_na_router { eth.src = xreg0[0..47]; ip6.src = nd.target; nd.tll = xreg0[0..47]; outport = inport; flags.loopback = 1; output; };)
  table=3 (lr_in_ip_input     ), priority=90   , match=(ip4.dst == 10.0.0.254 && icmp4.type == 8 && icmp4.code == 0), action=(ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 0; flags.loopback = 1; next; )
  table=3 (lr_in_ip_input     ), priority=90   , match=(ip4.dst == 10.0.1.254 && icmp4.type == 8 && icmp4.code == 0), action=(ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 0; flags.loopback = 1; next; )
  table=3 (lr_in_ip_input     ), priority=90   , match=(ip6.dst == fe80::200:ff:fe00:1ff && icmp6.type == 128 && icmp6.code == 0), action=(ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 129; flags.loopback = 1; next; )
  table=3 (lr_in_ip_input     ), priority=90   , match=(ip6.dst == fe80::200:ff:fe00:ff && icmp6.type == 128 && icmp6.code == 0), action=(ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 129; flags.loopback = 1; next; )
  table=3 (lr_in_ip_input     ), priority=85   , match=(arp || nd), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=84   , match=(nd_rs || nd_ra), action=(next;)
  table=3 (lr_in_ip_input     ), priority=83   , match=(ip6.mcast_rsvd), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=82   , match=(ip4.mcast || ip6.mcast), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip4 && ip4.dst == 10.0.0.254 && !ip.later_frag && sctp), action=(sctp_abort {eth.dst <-> eth.src; ip4.dst <-> ip4.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip4 && ip4.dst == 10.0.0.254 && !ip.later_frag && tcp), action=(tcp_reset {eth.dst <-> eth.src; ip4.dst <-> ip4.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip4 && ip4.dst == 10.0.0.254 && !ip.later_frag && udp), action=(icmp4 {eth.dst <-> eth.src; ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 3; icmp4.code = 3; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip4 && ip4.dst == 10.0.1.254 && !ip.later_frag && sctp), action=(sctp_abort {eth.dst <-> eth.src; ip4.dst <-> ip4.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip4 && ip4.dst == 10.0.1.254 && !ip.later_frag && tcp), action=(tcp_reset {eth.dst <-> eth.src; ip4.dst <-> ip4.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip4 && ip4.dst == 10.0.1.254 && !ip.later_frag && udp), action=(icmp4 {eth.dst <-> eth.src; ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 3; icmp4.code = 3; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:1ff && !ip.later_frag && sctp), action=(sctp_abort {eth.dst <-> eth.src; ip6.dst <-> ip6.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:1ff && !ip.later_frag && tcp), action=(tcp_reset {eth.dst <-> eth.src; ip6.dst <-> ip6.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:1ff && !ip.later_frag && udp), action=(icmp6 {eth.dst <-> eth.src; ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 1; icmp6.code = 4; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:ff && !ip.later_frag && sctp), action=(sctp_abort {eth.dst <-> eth.src; ip6.dst <-> ip6.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:ff && !ip.later_frag && tcp), action=(tcp_reset {eth.dst <-> eth.src; ip6.dst <-> ip6.src; next; };)
  table=3 (lr_in_ip_input     ), priority=80   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:ff && !ip.later_frag && udp), action=(icmp6 {eth.dst <-> eth.src; ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 1; icmp6.code = 4; next; };)
  table=3 (lr_in_ip_input     ), priority=70   , match=(ip4 && ip4.dst == 10.0.0.254 && !ip.later_frag), action=(icmp4 {eth.dst <-> eth.src; ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 3; icmp4.code = 2; next; };)
  table=3 (lr_in_ip_input     ), priority=70   , match=(ip4 && ip4.dst == 10.0.1.254 && !ip.later_frag), action=(icmp4 {eth.dst <-> eth.src; ip4.dst <-> ip4.src; ip.ttl = 255; icmp4.type = 3; icmp4.code = 2; next; };)
  table=3 (lr_in_ip_input     ), priority=70   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:1ff && !ip.later_frag), action=(icmp6 {eth.dst <-> eth.src; ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 1; icmp6.code = 3; next; };)
  table=3 (lr_in_ip_input     ), priority=70   , match=(ip6 && ip6.dst == fe80::200:ff:fe00:ff && !ip.later_frag), action=(icmp6 {eth.dst <-> eth.src; ip6.dst <-> ip6.src; ip.ttl = 255; icmp6.type = 1; icmp6.code = 3; next; };)
  table=3 (lr_in_ip_input     ), priority=60   , match=(ip4.dst == {10.0.0.254}), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=60   , match=(ip4.dst == {10.0.1.254}), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=60   , match=(ip6.dst == {fe80::200:ff:fe00:1ff}), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=60   , match=(ip6.dst == {fe80::200:ff:fe00:ff}), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=50   , match=(eth.bcast), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=32   , match=(ip.ttl == {0, 1} && !ip.later_frag && (ip4.mcast || ip6.mcast)), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=31   , match=(inport == "r1s0" && ip4 && ip.ttl == {0, 1} && !ip.later_frag), action=(icmp4 {eth.dst <-> eth.src; icmp4.type = 11; /* Time exceeded */ icmp4.code = 0; /* TTL exceeded in transit */ ip4.dst = ip4.src; ip4.src = 10.0.0.254 ; ip.ttl = 254; outport = "r1s0"; flags.loopback = 1; output; };)
  table=3 (lr_in_ip_input     ), priority=31   , match=(inport == "r1s1" && ip4 && ip.ttl == {0, 1} && !ip.later_frag), action=(icmp4 {eth.dst <-> eth.src; icmp4.type = 11; /* Time exceeded */ icmp4.code = 0; /* TTL exceeded in transit */ ip4.dst = ip4.src; ip4.src = 10.0.1.254 ; ip.ttl = 254; outport = "r1s1"; flags.loopback = 1; output; };)
  table=3 (lr_in_ip_input     ), priority=30   , match=(ip.ttl == {0, 1}), action=(drop;)
  table=3 (lr_in_ip_input     ), priority=0    , match=(1), action=(next;)
  table=4 (lr_in_dhcp_relay_req), priority=0    , match=(1), action=(next;)
  table=5 (lr_in_unsnat       ), priority=0    , match=(1), action=(next;)
  table=6 (lr_in_post_unsnat  ), priority=0    , match=(1), action=(next;)
  table=7 (lr_in_defrag       ), priority=0    , match=(1), action=(next;)
  table=8 (lr_in_lb_aff_check ), priority=0    , match=(1), action=(next;)
  table=9 (lr_in_dnat         ), priority=0    , match=(1), action=(next;)
  table=10(lr_in_lb_aff_learn ), priority=0    , match=(1), action=(next;)
  table=11(lr_in_ecmp_stateful), priority=0    , match=(1), action=(next;)
  table=12(lr_in_nd_ra_options), priority=0    , match=(1), action=(next;)
  table=13(lr_in_nd_ra_response), priority=0    , match=(1), action=(next;)
  table=14(lr_in_ip_routing_pre), priority=0    , match=(1), action=(reg7 = 0; next;)
  table=15(lr_in_ip_routing   ), priority=10550, match=(nd_rs || nd_ra), action=(drop;)
  table=15(lr_in_ip_routing   ), priority=518  , match=(inport == "r1s0" && ip6.dst == fe80::/64), action=(ip.ttl--; reg8[0..15] = 0; xxreg0 = ip6.dst; xxreg1 = fe80::200:ff:fe00:ff; eth.src = 00:00:00:00:00:ff; outport = "r1s0"; flags.loopback = 1; reg9[9] = 0; next;)
  table=15(lr_in_ip_routing   ), priority=518  , match=(inport == "r1s1" && ip6.dst == fe80::/64), action=(ip.ttl--; reg8[0..15] = 0; xxreg0 = ip6.dst; xxreg1 = fe80::200:ff:fe00:1ff; eth.src = 00:00:00:00:01:ff; outport = "r1s1"; flags.loopback = 1; reg9[9] = 0; next;)
  table=15(lr_in_ip_routing   ), priority=198  , match=(ip4.dst == 10.0.0.0/24), action=(ip.ttl--; reg8[0..15] = 0; reg0 = ip4.dst; reg5 = 10.0.0.254; eth.src = 00:00:00:00:00:ff; outport = "r1s0"; flags.loopback = 1; reg9[9] = 1; next;)
  table=15(lr_in_ip_routing   ), priority=198  , match=(ip4.dst == 10.0.1.0/24), action=(ip.ttl--; reg8[0..15] = 0; reg0 = ip4.dst; reg5 = 10.0.1.254; eth.src = 00:00:00:00:01:ff; outport = "r1s1"; flags.loopback = 1; reg9[9] = 1; next;)
  table=15(lr_in_ip_routing   ), priority=0    , match=(1), action=(drop;)
  table=16(lr_in_ip_routing_ecmp), priority=150  , match=(reg8[0..15] == 0), action=(next;)
  table=16(lr_in_ip_routing_ecmp), priority=0    , match=(1), action=(drop;)
  table=17(lr_in_policy       ), priority=0    , match=(1), action=(reg8[0..15] = 0; next;)
  table=18(lr_in_policy_ecmp  ), priority=150  , match=(reg8[0..15] == 0), action=(next;)
  table=18(lr_in_policy_ecmp  ), priority=0    , match=(1), action=(drop;)
  table=19(lr_in_dhcp_relay_resp_chk), priority=0    , match=(1), action=(next;)
  table=20(lr_in_dhcp_relay_resp), priority=0    , match=(1), action=(next;)
  table=21(lr_in_arp_resolve  ), priority=500  , match=(ip4.mcast || ip6.mcast), action=(next;)
  table=21(lr_in_arp_resolve  ), priority=1    , match=(reg9[9] == 0), action=(get_nd(outport, xxreg0); next;)
  table=21(lr_in_arp_resolve  ), priority=1    , match=(reg9[9] == 1), action=(get_arp(outport, reg0); next;)
  table=21(lr_in_arp_resolve  ), priority=0    , match=(1), action=(drop;)
  table=22(lr_in_chk_pkt_len  ), priority=0    , match=(1), action=(next;)
  table=23(lr_in_larger_pkts  ), priority=0    , match=(1), action=(next;)
  table=24(lr_in_gw_redirect  ), priority=0    , match=(1), action=(next;)
  table=25(lr_in_network_id   ), priority=110  , match=(outport == "r1s0" && ip4 && reg0 == 10.0.0.254/24), action=(flags.network_id = 0; next;)
  table=25(lr_in_network_id   ), priority=110  , match=(outport == "r1s1" && ip4 && reg0 == 10.0.1.254/24), action=(flags.network_id = 0; next;)
  table=25(lr_in_network_id   ), priority=105  , match=(1), action=(flags.network_id = 0; next;)
  table=25(lr_in_network_id   ), priority=0    , match=(1), action=(next;)
  table=26(lr_in_arp_request  ), priority=100  , match=(eth.dst == 00:00:00:00:00:00 && reg9[9] == 0), action=(nd_ns { nd.target = xxreg0; output; }; output;)
  table=26(lr_in_arp_request  ), priority=100  , match=(eth.dst == 00:00:00:00:00:00 && reg9[9] == 1), action=(arp { eth.dst = ff:ff:ff:ff:ff:ff; arp.spa = reg5; arp.tpa = reg0; arp.op = 1; output; }; output;)
  table=26(lr_in_arp_request  ), priority=0    , match=(1), action=(output;)
Datapath: "r1" (1287fc2b-1e3b-4bf9-8cf8-db890e663dc3)  Pipeline: egress
  table=0 (lr_out_chk_dnat_local), priority=0    , match=(1), action=(reg9[4] = 0; next;)
  table=1 (lr_out_undnat      ), priority=0    , match=(1), action=(next;)
  table=2 (lr_out_post_undnat ), priority=0    , match=(1), action=(next;)
  table=3 (lr_out_snat        ), priority=120  , match=(nd_ns), action=(next;)
  table=3 (lr_out_snat        ), priority=0    , match=(1), action=(next;)
  table=4 (lr_out_post_snat   ), priority=0    , match=(1), action=(next;)
  table=5 (lr_out_egr_loop    ), priority=0    , match=(1), action=(next;)
  table=6 (lr_out_delivery    ), priority=100  , match=(outport == "r1s0"), action=(output;)
  table=6 (lr_out_delivery    ), priority=100  , match=(outport == "r1s1"), action=(output;)
  table=6 (lr_out_delivery    ), priority=0    , match=(1), action=(drop;)
Datapath: "s0" (49345921-e4da-4816-bf85-b092efe8eb29)  Pipeline: ingress
  table=0 (ls_in_check_port_sec), priority=120  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && eth.dst == 00:00:00:00:00:ff && flags.tunnel_rx == 1), action=(outport <-> inport; next(pipeline=ingress,table=29);)
  table=0 (ls_in_check_port_sec), priority=110  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && eth.src == 00:00:00:00:00:01 && outport == "port01" && !is_chassis_resident("port01") && flags.tunnel_rx == 1), action=(outport <-> inport; next;)
  table=0 (ls_in_check_port_sec), priority=110  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && eth.src == 00:00:00:00:00:02 && outport == "port02" && !is_chassis_resident("port02") && flags.tunnel_rx == 1), action=(outport <-> inport; next;)
  table=0 (ls_in_check_port_sec), priority=105  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && flags.tunnel_rx == 1), action=(drop;)
  table=0 (ls_in_check_port_sec), priority=100  , match=(eth.src[40]), action=(drop;)
  table=0 (ls_in_check_port_sec), priority=100  , match=(vlan.present), action=(drop;)
  table=0 (ls_in_check_port_sec), priority=70   , match=(inport == "s0r1"), action=(reg0[18] = 1; next;)
  table=0 (ls_in_check_port_sec), priority=50   , match=(1), action=(reg0[15] = check_in_port_sec(); next;)
  table=1 (ls_in_apply_port_sec), priority=50   , match=(reg0[15] == 1), action=(drop;)
  table=1 (ls_in_apply_port_sec), priority=0    , match=(1), action=(next;)
  table=2 (ls_in_mirror       ), priority=0    , match=(1), action=(next;)
  table=3 (ls_in_lookup_fdb   ), priority=0    , match=(1), action=(next;)
  table=4 (ls_in_put_fdb      ), priority=0    , match=(1), action=(next;)
  table=5 (ls_in_pre_acl      ), priority=110  , match=(eth.dst == $svc_monitor_mac), action=(next;)
  table=5 (ls_in_pre_acl      ), priority=0    , match=(1), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) ||(ip6 && icmp6.type == 2 && icmp6.code == 0)) && flags.tunnel_rx == 1), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(eth.dst == $svc_monitor_mac), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(eth.mcast), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(ip && inport == "s0r1"), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(nd || nd_rs || nd_ra || mldv1 || mldv2), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(reg0[16] == 1), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=0    , match=(1), action=(next;)
  table=7 (ls_in_pre_stateful ), priority=115  , match=(reg0[2] == 1 && ip.is_frag), action=(reg0[19] = 1; ct_lb_mark;)
  table=7 (ls_in_pre_stateful ), priority=110  , match=(reg0[2] == 1), action=(ct_lb_mark;)
  table=7 (ls_in_pre_stateful ), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=7 (ls_in_pre_stateful ), priority=0    , match=(1), action=(next;)
  table=8 (ls_in_acl_hint     ), priority=65535, match=(1), action=(next;)
  table=9 (ls_in_acl_eval     ), priority=65535, match=(1), action=(next;)
  table=9 (ls_in_acl_eval     ), priority=65532, match=(nd || nd_ra || nd_rs || mldv1 || mldv2), action=(reg8[16] = 1; next;)
  table=10(ls_in_acl_sample   ), priority=0    , match=(1), action=(next;)
  table=11(ls_in_acl_action   ), priority=0    , match=(1), action=(next;)
  table=12(ls_in_qos          ), priority=0    , match=(1), action=(next;)
  table=13(ls_in_lb_aff_check ), priority=0    , match=(1), action=(next;)
  table=14(ls_in_lb           ), priority=0    , match=(1), action=(next;)
  table=15(ls_in_lb_aff_learn ), priority=0    , match=(1), action=(next;)
  table=16(ls_in_pre_hairpin  ), priority=0    , match=(1), action=(next;)
  table=17(ls_in_nat_hairpin  ), priority=0    , match=(1), action=(next;)
  table=18(ls_in_hairpin      ), priority=0    , match=(1), action=(next;)
  table=19(ls_in_acl_after_lb_eval), priority=65532, match=(nd || nd_ra || nd_rs || mldv1 || mldv2), action=(reg8[16] = 1; next;)
  table=19(ls_in_acl_after_lb_eval), priority=0    , match=(1), action=(next;)
  table=20(ls_in_acl_after_lb_sample), priority=0    , match=(1), action=(next;)
  table=21(ls_in_acl_after_lb_action), priority=0    , match=(1), action=(next;)
  table=22(ls_in_stateful     ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 0), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_label.acl_id = reg2[16..31]; }; next;)
  table=22(ls_in_stateful     ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 1), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_mark.obs_stage = reg8[19..20]; ct_mark.obs_collector_id = reg8[8..15]; ct_label.obs_point_id = reg9; ct_label.acl_id = reg2[16..31]; }; next;)
  table=22(ls_in_stateful     ), priority=0    , match=(1), action=(next;)
  table=23(ls_in_arp_rsp      ), priority=0    , match=(1), action=(next;)
  table=24(ls_in_dhcp_options ), priority=0    , match=(1), action=(next;)
  table=25(ls_in_dhcp_response), priority=0    , match=(1), action=(next;)
  table=26(ls_in_dns_lookup   ), priority=0    , match=(1), action=(next;)
  table=27(ls_in_dns_response ), priority=0    , match=(1), action=(next;)
  table=28(ls_in_external_port), priority=0    , match=(1), action=(next;)
  table=29(ls_in_l2_lkup      ), priority=110  , match=(eth.dst == $svc_monitor_mac && (tcp || icmp || icmp6)), action=(handle_svc_check(inport);)
  table=29(ls_in_l2_lkup      ), priority=80   , match=(flags[1] == 0 && arp.op == 1 && arp.tpa == 10.0.0.254), action=(clone {outport = "s0r1"; output; }; outport = "_MC_flood_l2"; output;)
  table=29(ls_in_l2_lkup      ), priority=80   , match=(flags[1] == 0 && nd_ns && nd.target == fe80::200:ff:fe00:ff), action=(clone {outport = "s0r1"; output; }; outport = "_MC_flood_l2"; output;)
  table=29(ls_in_l2_lkup      ), priority=75   , match=(eth.src == {00:00:00:00:00:ff} && eth.dst == ff:ff:ff:ff:ff:ff && (arp.op == 1 || rarp.op == 3 || nd_ns)), action=(outport = "_MC_flood_l2"; output;)
  table=29(ls_in_l2_lkup      ), priority=70   , match=(eth.mcast), action=(outport = "_MC_flood"; output;)
  table=29(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:01), action=(outport = "port01"; output;)
  table=29(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:02), action=(outport = "port02"; output;)
  table=29(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:00:ff), action=(outport = "s0r1"; output;)
  table=29(ls_in_l2_lkup      ), priority=0    , match=(1), action=(outport = get_fdb(eth.dst); next;)
  table=30(ls_in_l2_unknown   ), priority=50   , match=(outport == "none"), action=(drop;)
  table=30(ls_in_l2_unknown   ), priority=0    , match=(1), action=(output;)
Datapath: "s0" (49345921-e4da-4816-bf85-b092efe8eb29)  Pipeline: egress
  table=0 (ls_out_lookup_fdb  ), priority=0    , match=(1), action=(next;)
  table=1 (ls_out_put_fdb     ), priority=0    , match=(1), action=(next;)
  table=2 (ls_out_pre_acl     ), priority=110  , match=(eth.src == $svc_monitor_mac), action=(next;)
  table=2 (ls_out_pre_acl     ), priority=0    , match=(1), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(eth.mcast), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(eth.src == $svc_monitor_mac), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(ip && outport == "s0r1"), action=(ct_clear; next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(nd || nd_rs || nd_ra || mldv1 || mldv2), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(reg0[16] == 1), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=0    , match=(1), action=(next;)
  table=4 (ls_out_pre_stateful), priority=110  , match=(reg0[2] == 1), action=(ct_lb_mark;)
  table=4 (ls_out_pre_stateful), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=4 (ls_out_pre_stateful), priority=0    , match=(1), action=(next;)
  table=5 (ls_out_acl_hint    ), priority=65535, match=(1), action=(next;)
  table=6 (ls_out_acl_eval    ), priority=65535, match=(1), action=(next;)
  table=6 (ls_out_acl_eval    ), priority=65532, match=(nd || nd_ra || nd_rs || mldv1 || mldv2), action=(reg8[16] = 1; next;)
  table=7 (ls_out_acl_sample  ), priority=0    , match=(1), action=(next;)
  table=8 (ls_out_acl_action  ), priority=0    , match=(1), action=(next;)
  table=9 (ls_out_mirror      ), priority=0    , match=(1), action=(next;)
  table=10(ls_out_qos         ), priority=0    , match=(1), action=(next;)
  table=11(ls_out_stateful    ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 0), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_label.acl_id = reg2[16..31]; }; next;)
  table=11(ls_out_stateful    ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 1), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_mark.obs_stage = reg8[19..20]; ct_mark.obs_collector_id = reg8[8..15]; ct_label.obs_point_id = reg9; ct_label.acl_id = reg2[16..31]; }; next;)
  table=11(ls_out_stateful    ), priority=0    , match=(1), action=(next;)
  table=12(ls_out_check_port_sec), priority=100  , match=(eth.mcast), action=(reg0[15] = 0; next;)
  table=12(ls_out_check_port_sec), priority=0    , match=(1), action=(reg0[15] = check_out_port_sec(); next;)
  table=13(ls_out_apply_port_sec), priority=50   , match=(reg0[15] == 1), action=(drop;)
  table=13(ls_out_apply_port_sec), priority=0    , match=(1), action=(output;)
Datapath: "s1" (d0f86629-a2df-4188-a210-98fbfd4124b6)  Pipeline: ingress
  table=0 (ls_in_check_port_sec), priority=120  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && eth.dst == 00:00:00:00:01:ff && flags.tunnel_rx == 1), action=(outport <-> inport; next(pipeline=ingress,table=29);)
  table=0 (ls_in_check_port_sec), priority=110  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && eth.src == 00:00:00:00:01:01 && outport == "port11" && !is_chassis_resident("port11") && flags.tunnel_rx == 1), action=(outport <-> inport; next;)
  table=0 (ls_in_check_port_sec), priority=110  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && eth.src == 00:00:00:00:01:02 && outport == "port12" && !is_chassis_resident("port12") && flags.tunnel_rx == 1), action=(outport <-> inport; next;)
  table=0 (ls_in_check_port_sec), priority=105  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) || (ip6 && icmp6.type == 2 && icmp6.code == 0)) && flags.tunnel_rx == 1), action=(drop;)
  table=0 (ls_in_check_port_sec), priority=100  , match=(eth.src[40]), action=(drop;)
  table=0 (ls_in_check_port_sec), priority=100  , match=(vlan.present), action=(drop;)
  table=0 (ls_in_check_port_sec), priority=70   , match=(inport == "s1r1"), action=(reg0[18] = 1; next;)
  table=0 (ls_in_check_port_sec), priority=50   , match=(1), action=(reg0[15] = check_in_port_sec(); next;)
  table=1 (ls_in_apply_port_sec), priority=50   , match=(reg0[15] == 1), action=(drop;)
  table=1 (ls_in_apply_port_sec), priority=0    , match=(1), action=(next;)
  table=2 (ls_in_mirror       ), priority=0    , match=(1), action=(next;)
  table=3 (ls_in_lookup_fdb   ), priority=0    , match=(1), action=(next;)
  table=4 (ls_in_put_fdb      ), priority=0    , match=(1), action=(next;)
  table=5 (ls_in_pre_acl      ), priority=110  , match=(eth.dst == $svc_monitor_mac), action=(next;)
  table=5 (ls_in_pre_acl      ), priority=0    , match=(1), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(((ip4 && icmp4.type == 3 && icmp4.code == 4) ||(ip6 && icmp6.type == 2 && icmp6.code == 0)) && flags.tunnel_rx == 1), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(eth.dst == $svc_monitor_mac), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(eth.mcast), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(ip && inport == "s1r1"), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(nd || nd_rs || nd_ra || mldv1 || mldv2), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=110  , match=(reg0[16] == 1), action=(next;)
  table=6 (ls_in_pre_lb       ), priority=0    , match=(1), action=(next;)
  table=7 (ls_in_pre_stateful ), priority=115  , match=(reg0[2] == 1 && ip.is_frag), action=(reg0[19] = 1; ct_lb_mark;)
  table=7 (ls_in_pre_stateful ), priority=110  , match=(reg0[2] == 1), action=(ct_lb_mark;)
  table=7 (ls_in_pre_stateful ), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=7 (ls_in_pre_stateful ), priority=0    , match=(1), action=(next;)
  table=8 (ls_in_acl_hint     ), priority=65535, match=(1), action=(next;)
  table=9 (ls_in_acl_eval     ), priority=65535, match=(1), action=(next;)
  table=9 (ls_in_acl_eval     ), priority=65532, match=(nd || nd_ra || nd_rs || mldv1 || mldv2), action=(reg8[16] = 1; next;)
  table=10(ls_in_acl_sample   ), priority=0    , match=(1), action=(next;)
  table=11(ls_in_acl_action   ), priority=0    , match=(1), action=(next;)
  table=12(ls_in_qos          ), priority=0    , match=(1), action=(next;)
  table=13(ls_in_lb_aff_check ), priority=0    , match=(1), action=(next;)
  table=14(ls_in_lb           ), priority=0    , match=(1), action=(next;)
  table=15(ls_in_lb_aff_learn ), priority=0    , match=(1), action=(next;)
  table=16(ls_in_pre_hairpin  ), priority=0    , match=(1), action=(next;)
  table=17(ls_in_nat_hairpin  ), priority=0    , match=(1), action=(next;)
  table=18(ls_in_hairpin      ), priority=0    , match=(1), action=(next;)
  table=19(ls_in_acl_after_lb_eval), priority=65532, match=(nd || nd_ra || nd_rs || mldv1 || mldv2), action=(reg8[16] = 1; next;)
  table=19(ls_in_acl_after_lb_eval), priority=0    , match=(1), action=(next;)
  table=20(ls_in_acl_after_lb_sample), priority=0    , match=(1), action=(next;)
  table=21(ls_in_acl_after_lb_action), priority=0    , match=(1), action=(next;)
  table=22(ls_in_stateful     ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 0), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_label.acl_id = reg2[16..31]; }; next;)
  table=22(ls_in_stateful     ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 1), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_mark.obs_stage = reg8[19..20]; ct_mark.obs_collector_id = reg8[8..15]; ct_label.obs_point_id = reg9; ct_label.acl_id = reg2[16..31]; }; next;)
  table=22(ls_in_stateful     ), priority=0    , match=(1), action=(next;)
  table=23(ls_in_arp_rsp      ), priority=0    , match=(1), action=(next;)
  table=24(ls_in_dhcp_options ), priority=0    , match=(1), action=(next;)
  table=25(ls_in_dhcp_response), priority=0    , match=(1), action=(next;)
  table=26(ls_in_dns_lookup   ), priority=0    , match=(1), action=(next;)
  table=27(ls_in_dns_response ), priority=0    , match=(1), action=(next;)
  table=28(ls_in_external_port), priority=0    , match=(1), action=(next;)
  table=29(ls_in_l2_lkup      ), priority=110  , match=(eth.dst == $svc_monitor_mac && (tcp || icmp || icmp6)), action=(handle_svc_check(inport);)
  table=29(ls_in_l2_lkup      ), priority=80   , match=(flags[1] == 0 && arp.op == 1 && arp.tpa == 10.0.1.254), action=(clone {outport = "s1r1"; output; }; outport = "_MC_flood_l2"; output;)
  table=29(ls_in_l2_lkup      ), priority=80   , match=(flags[1] == 0 && nd_ns && nd.target == fe80::200:ff:fe00:1ff), action=(clone {outport = "s1r1"; output; }; outport = "_MC_flood_l2"; output;)
  table=29(ls_in_l2_lkup      ), priority=75   , match=(eth.src == {00:00:00:00:01:ff} && eth.dst == ff:ff:ff:ff:ff:ff && (arp.op == 1 || rarp.op == 3 || nd_ns)), action=(outport = "_MC_flood_l2"; output;)
  table=29(ls_in_l2_lkup      ), priority=70   , match=(eth.mcast), action=(outport = "_MC_flood"; output;)
  table=29(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:01:01), action=(outport = "port11"; output;)
  table=29(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:01:02), action=(outport = "port12"; output;)
  table=29(ls_in_l2_lkup      ), priority=50   , match=(eth.dst == 00:00:00:00:01:ff), action=(outport = "s1r1"; output;)
  table=29(ls_in_l2_lkup      ), priority=0    , match=(1), action=(outport = get_fdb(eth.dst); next;)
  table=30(ls_in_l2_unknown   ), priority=50   , match=(outport == "none"), action=(drop;)
  table=30(ls_in_l2_unknown   ), priority=0    , match=(1), action=(output;)
Datapath: "s1" (d0f86629-a2df-4188-a210-98fbfd4124b6)  Pipeline: egress
  table=0 (ls_out_lookup_fdb  ), priority=0    , match=(1), action=(next;)
  table=1 (ls_out_put_fdb     ), priority=0    , match=(1), action=(next;)
  table=2 (ls_out_pre_acl     ), priority=110  , match=(eth.src == $svc_monitor_mac), action=(next;)
  table=2 (ls_out_pre_acl     ), priority=0    , match=(1), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(eth.mcast), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(eth.src == $svc_monitor_mac), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(ip && outport == "s1r1"), action=(ct_clear; next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(nd || nd_rs || nd_ra || mldv1 || mldv2), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=110  , match=(reg0[16] == 1), action=(next;)
  table=3 (ls_out_pre_lb      ), priority=0    , match=(1), action=(next;)
  table=4 (ls_out_pre_stateful), priority=110  , match=(reg0[2] == 1), action=(ct_lb_mark;)
  table=4 (ls_out_pre_stateful), priority=100  , match=(reg0[0] == 1), action=(ct_next;)
  table=4 (ls_out_pre_stateful), priority=0    , match=(1), action=(next;)
  table=5 (ls_out_acl_hint    ), priority=65535, match=(1), action=(next;)
  table=6 (ls_out_acl_eval    ), priority=65535, match=(1), action=(next;)
  table=6 (ls_out_acl_eval    ), priority=65532, match=(nd || nd_ra || nd_rs || mldv1 || mldv2), action=(reg8[16] = 1; next;)
  table=7 (ls_out_acl_sample  ), priority=0    , match=(1), action=(next;)
  table=8 (ls_out_acl_action  ), priority=0    , match=(1), action=(next;)
  table=9 (ls_out_mirror      ), priority=0    , match=(1), action=(next;)
  table=10(ls_out_qos         ), priority=0    , match=(1), action=(next;)
  table=11(ls_out_stateful    ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 0), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_label.acl_id = reg2[16..31]; }; next;)
  table=11(ls_out_stateful    ), priority=100  , match=(reg0[1] == 1 && reg0[13] == 1), action=(ct_commit { ct_mark.blocked = 0; ct_mark.allow_established = reg0[20]; ct_mark.obs_stage = reg8[19..20]; ct_mark.obs_collector_id = reg8[8..15]; ct_label.obs_point_id = reg9; ct_label.acl_id = reg2[16..31]; }; next;)
  table=11(ls_out_stateful    ), priority=0    , match=(1), action=(next;)
  table=12(ls_out_check_port_sec), priority=100  , match=(eth.mcast), action=(reg0[15] = 0; next;)
  table=12(ls_out_check_port_sec), priority=0    , match=(1), action=(reg0[15] = check_out_port_sec(); next;)
  table=13(ls_out_apply_port_sec), priority=50   , match=(reg0[15] == 1), action=(drop;)
  table=13(ls_out_apply_port_sec), priority=0    , match=(1), action=(output;)
```

Last but not least, run a test ping between the namespaces on both hosts:

```
[root@ovn2 ~]# ip netns exec ns0 ping 10.0.1.1 -c1 -W1 # ping ns1 on ovn2
[root@ovn2 ~]# ip netns exec ns0 ping 10.0.1.2 -c1 -W1 # ping ns1 on ovn3
```

```
[root@ovn2 ~]# ip netns exec ns0 ping 10.0.1.1 -c1 -W1
PING 10.0.1.1 (10.0.1.1) 56(84) bytes of data.
64 bytes from 10.0.1.1: icmp_seq=1 ttl=63 time=18.3 ms

--- 10.0.1.1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 18.334/18.334/18.334/0.000 ms
[root@ovn2 ~]# ip netns exec ns0 ping 10.0.1.2 -c1 -W1
PING 10.0.1.2 (10.0.1.2) 56(84) bytes of data.
64 bytes from 10.0.1.2: icmp_seq=1 ttl=63 time=14.3 ms

--- 10.0.1.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 14.306/14.306/14.306/0.000 ms
[root@ovn2 ~]# 
```

## Adding external gateway  router, non-distributed ##

### Introduction ###

Next, let's add a gateway router on ovn1 with interface enp2s0. This means that all traffic from the namespaces to the
outside world will egress from / ingress via ovn1.

https://www.ovn.org/support/dist-docs/ovn-architecture.7.html

```
     Gateway Routers

       A gateway router is a logical router that is bound to a physical  loca‐
       tion.  This  includes  all  of  the  logical patch ports of the logical
       router, as well as all of the  peer  logical  patch  ports  on  logical
       switches.  In the OVN Southbound database, the Port_Binding entries for
       these logical patch ports use the type l3gateway rather than patch,  in
       order  to  distinguish  that  these  logical patch ports are bound to a
       chassis.
```

### Setup ###

Add provider bridge mapping on ovn1:

```
# ovn1
ovs-vsctl set open . external-ids:ovn-bridge-mappings=provider:br-provider
ovs-vsctl --may-exist add-br br-provider
ovs-vsctl --may-exist add-port br-provider enp2s0
```

Bind logical router rg (router gateway) to chassis ovn1 and assign an internal IP pointing towards the VMs: 

```
chassis=$(ovn-sbctl --columns=name --bar find chassis hostname=ovn1)
ovn-nbctl create Logical_Router name=rg options:chassis=$chassis
ovn-nbctl lrp-add rg rgsj 00:00:00:01:00:01 10.1.0.1/30
```

Create switch sj (switch join) and add the router port of rg to it:

```
ovn-nbctl ls-add sj
ovn-nbctl lsp-add sj sjrg
ovn-nbctl lsp-set-type sjrg router
ovn-nbctl lsp-set-addresses sjrg 00:00:00:01:00:01
ovn-nbctl lsp-set-options sjrg router-port=rgsj
```

Add r1 port to the join switch:

```
ovn-nbctl lrp-add r1 r1sj 00:00:00:01:00:02 10.1.0.2/30
```

```
ovn-nbctl lsp-add sj sjr1
ovn-nbctl lsp-set-type sjr1 router
ovn-nbctl lsp-set-addresses sjr1 00:00:00:01:00:02
ovn-nbctl lsp-set-options sjr1 router-port=r1sj
```

Add routes - router gateway towards our 2 private subnets via r1 and r1 default route towards rg:

```
ovn-nbctl lr-route-add rg "10.0.0.0/24" 10.1.0.2
ovn-nbctl lr-route-add rg "10.0.1.0/24" 10.1.0.2
ovn-nbctl lr-route-add r1 "0.0.0.0/0" 10.1.0.1
```

Verify routes:
```
[root@ovn1 ~]# ovn-nbctl lr-route-list rg
IPv4 Routes
              10.0.0.0/24                  10.1.0.2 dst-ip
              10.0.1.0/24                  10.1.0.2 dst-ip
[root@ovn1 ~]# ovn-nbctl lr-route-list r1
IPv4 Routes
                0.0.0.0/0                  10.1.0.1 dst-ip
[root@ovn1 ~]# ovn-nbctl list Logical_router_Static_route
_uuid               : b99f6718-0364-4447-8450-0f8c71ac61fe
bfd                 : []
external_ids        : {}
ip_prefix           : "10.0.1.0/24"
nexthop             : "10.1.0.2"
options             : {}
output_port         : []
policy              : []
route_table         : ""
selection_fields    : []

_uuid               : aa67d0ac-5b8c-4e68-bdb1-0a27f7fa48a6
bfd                 : []
external_ids        : {}
ip_prefix           : "0.0.0.0/0"
nexthop             : "10.1.0.1"
options             : {}
output_port         : []
policy              : []
route_table         : ""
selection_fields    : []

_uuid               : b9aad1d3-2bb1-4b4b-8cc6-f0294d668c2a
bfd                 : []
external_ids        : {}
ip_prefix           : "10.0.0.0/24"
nexthop             : "10.1.0.2"
options             : {}
output_port         : []
policy              : []
route_table         : ""
selection_fields    : []
[root@ovn1 ~]# ovn-nbctl list Logical_router_Static_Route
_uuid               : b99f6718-0364-4447-8450-0f8c71ac61fe
bfd                 : []
external_ids        : {}
ip_prefix           : "10.0.1.0/24"
nexthop             : "10.1.0.2"
options             : {}
output_port         : []
policy              : []
route_table         : ""
selection_fields    : []

_uuid               : aa67d0ac-5b8c-4e68-bdb1-0a27f7fa48a6
bfd                 : []
external_ids        : {}
ip_prefix           : "0.0.0.0/0"
nexthop             : "10.1.0.1"
options             : {}
output_port         : []
policy              : []
route_table         : ""
selection_fields    : []

_uuid               : b9aad1d3-2bb1-4b4b-8cc6-f0294d668c2a
bfd                 : []
external_ids        : {}
ip_prefix           : "10.0.0.0/24"
nexthop             : "10.1.0.2"
options             : {}
output_port         : []
policy              : []
route_table         : ""
selection_fields    : []
```

Add an external port to rg:
```
ovn-nbctl lrp-add rg rgsp 00:00:00:02:00:ff 10.2.0.254/24
```

Add provider network switch (sp):
```
ovn-nbctl ls-add sp
ovn-nbctl lsp-add sp sprg
ovn-nbctl lsp-set-type sprg router
ovn-nbctl lsp-set-addresses sprg 00:00:00:02:00:ff
ovn-nbctl lsp-set-options sprg router-port=rgsp
```

Add localnet port (enp2s0) to provider switch (sp):
```
ovn-nbctl lsp-add sp sp-localnet
ovn-nbctl lsp-set-addresses sp-localnet unknown
ovn-nbctl lsp-set-type sp-localnet localnet
ovn-nbctl lsp-set-options sp-localnet network_name=provider
```

Verify on ovn1:
```
ovs-vsctl show
ovn-nbctl show
ovn-sbctl show
```

```
[root@ovn1 ~]# ovs-vsctl show
1717f06c-b2be-42f0-99c2-5081333e6ebb
    Bridge br-int
        fail_mode: secure
        datapath_type: system
        Port br-int
            Interface br-int
                type: internal
        Port ovn-108c2b-0
            Interface ovn-108c2b-0
                type: geneve
                options: {csum="true", key=flow, local_ip="192.168.122.182", remote_ip="192.168.122.159"}
        Port patch-br-int-to-sp-localnet
            Interface patch-br-int-to-sp-localnet
                type: patch
                options: {peer=patch-sp-localnet-to-br-int}
        Port ovn-8de4d5-0
            Interface ovn-8de4d5-0
                type: geneve
                options: {csum="true", key=flow, local_ip="192.168.122.182", remote_ip="192.168.122.107"}
    Bridge br-provider
        Port patch-sp-localnet-to-br-int
            Interface patch-sp-localnet-to-br-int
                type: patch
                options: {peer=patch-br-int-to-sp-localnet}
        Port enp2s0
            Interface enp2s0
        Port br-provider
            Interface br-provider
                type: internal
    ovs_version: "3.5.90-1.fc42"
[root@ovn1 ~]# ovn-nbctl show
switch ecced6ae-5eb3-4725-825a-da4d7461b230 (sp)
    port sp-localnet
        type: localnet
        addresses: ["unknown"]
    port sprg
        type: router
        addresses: ["00:00:00:02:00:ff"]
        router-port: rgsp
switch 70a89317-1bc0-4fcb-825d-c93754a5a6ca (s0)
    port port01
        addresses: ["00:00:00:00:00:01"]
    port s0r1
        type: router
        addresses: ["00:00:00:00:00:ff"]
        router-port: r1s0
    port port02
        addresses: ["00:00:00:00:00:02"]
switch 313fb333-5c33-4bf2-8936-72514bc20de3 (s1)
    port s1r1
        type: router
        addresses: ["00:00:00:00:01:ff"]
        router-port: r1s1
    port port11
        addresses: ["00:00:00:00:01:01"]
    port port12
        addresses: ["00:00:00:00:01:02"]
switch 36e27b77-135e-4841-93b5-013b69da406e (sj)
    port sjrg
        type: router
        addresses: ["00:00:00:01:00:01"]
        router-port: rgsj
    port sjr1
        type: router
        addresses: ["00:00:00:01:00:02"]
        router-port: r1sj
router 5473b860-6b45-4190-b88d-070156c4b1c3 (rg)
    port rgsj
        mac: "00:00:00:01:00:01"
        ipv6-lla: "fe80::200:ff:fe01:1"
        networks: ["10.1.0.1/30"]
    port rgsp
        mac: "00:00:00:02:00:ff"
        ipv6-lla: "fe80::200:ff:fe02:ff"
        networks: ["10.2.0.254/24"]
router fb0101fd-881e-4aa2-ae05-16a93428dc53 (r1)
    port r1s1
        mac: "00:00:00:00:01:ff"
        ipv6-lla: "fe80::200:ff:fe00:1ff"
        networks: ["10.0.1.254/24"]
    port r1s0
        mac: "00:00:00:00:00:ff"
        ipv6-lla: "fe80::200:ff:fe00:ff"
        networks: ["10.0.0.254/24"]
    port r1sj
        mac: "00:00:00:01:00:02"
        ipv6-lla: "fe80::200:ff:fe01:2"
        networks: ["10.1.0.2/30"]
[root@ovn1 ~]# ovn-sbctl show
Chassis "8de4d500-ab50-4ecc-a6df-3b5387efdf81"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.107"
        options: {csum="true"}
    Port_Binding port02
    Port_Binding port12
Chassis "dacba18d-52f9-4adb-8b3d-8ca27b700a73"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.182"
        options: {csum="true"}
    Port_Binding rgsj
    Port_Binding sprg
    Port_Binding rgsp
    Port_Binding sjrg
Chassis "108c2b72-5ab9-43a0-9617-55f23f16482a"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.159"
        options: {csum="true"}
    Port_Binding port01
    Port_Binding port11
```

Connect to ovn2 and simulate an external node, in the default network namespace.

```
nmcli conn mod enp2s0 ipv4.method manual ipv4.address 10.2.0.10/24 \
  +ipv4.routes "10.0.1.0/24 10.2.0.254" \
  +ipv4.routes "10.1.0.0/24 10.2.0.254" \
  +ipv4.routes "10.0.0.0/24 10.2.0.254"
nmcli conn up enp2s0
```

Verify:

```
[root@ovn2 ~]# ping 10.0.0.1 -c1 -W1 >/dev/null && echo "ok"
ok
```

```
[root@ovn3 ~]# ip netns exec ns0 ping 10.2.0.10 -c1 -W1 >/dev/null && echo "ok"
ok
```

A tracepath should show all hops:
```
[root@ovn2 ~]# ip netns exec ns1 tracepath -n 10.2.0.10
 1?: [LOCALHOST]                      pmtu 1500
 1:  10.0.1.254                                            1.001ms asymm  2 
 1:  10.0.1.254                                            0.696ms asymm  2 
 2:  no reply
 3:  10.2.0.10                                             1.075ms pmtu 1442
 3:  10.2.0.10                                             3.760ms reached
     Resume: pmtu 1442 hops 3 back 3 
```

## Inspecting tunnel encapsulations ##

### Introduction ###

For more information about OVN's VXLAN and Geneve encapsulations, see section "Tunnel Encapsulations" in
https://www.ovn.org/support/dist-docs/ovn-architecture.7.html

Here's an excerpt:

```
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
```

## Distributed gateway router ##

For more information about distributed gateway routes / ports, see the following blog post:

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
