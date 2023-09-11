# Hands-on with OVN Interconnection (OVN IC)

## Introduction to the test environment

We will use a setup consisting of 3 machines, named ovn1 (192.168.122.26), ovn2 (192.168.122.177) and ovn3 (192.168.122.54). Each machine’s primary interface is called `if0`, and OVN geneve tunnels are established via these interfaces. The machines run Red Hat Enterprise Linux 9.2.

![ovn-interconnection01](https://github.com/andreaskaris/blog/assets/3291433/29a9df6e-3edc-45cd-86fd-c5725ba52865)

All machines can reach each other via their hostname, by a mapping in /etc/hosts:

```
# cat /etc/hosts
127.0.0.1   localhost localhost.localdomain localhost4 localhost4.localdomain4
::1         localhost localhost.localdomain localhost6 localhost6.localdomain6
192.168.122.26 ovn1
192.168.122.177 ovn2
192.168.122.54 ovn3
```

## Setting up OVN control plane components

We are first going to configure the control plane components. In order to do so, we will configure 2 OVN interconnection availability zones. The first zone consists of ovn1 and ovn2, where ovn1 hosts the OVN control plane and both ovn1 and ovn2 run ovn-controller. The second zone consists of ovn3 which runs a second OVN control plane and ovn-controller.

![ovn-interconnection02](https://github.com/andreaskaris/blog/assets/3291433/1d732562-4845-4b7c-bb17-ea6beda92116)

### Setting up OVN common setup

On all machines, we enable the OpenShift 4.13 and Fast Data Path repositories which allow us to pull recent versions of Open vSwitch and OVN:

```
subscription-manager repos –enable=rhocp-4.13-for-rhel-9-x86_64-rpms
subscription-manager repos --enable=fast-datapath-for-rhel-9-x86_64-rpms
```

And we install Open vSwitch and OVN:

```
yum install openvswitch3.1 -y
yum install ovn23.09 ovn23.09-central ovn23.09-host -y
```

We enable Open vSwitch and ovn-controller on all hosts and start them right away:

```
systemctl enable --now openvswitch
systemctl enable --now ovn-controller
```

### Setting up OVN hosts for standalone availability zone 0

On ovn1, we enable the OVN control plane with:

```
cat <<EOF > /etc/sysconfig/ovn
OVN_NORTHD_OPTS="--db-nb-addr=$(hostname) --db-nb-create-insecure-remote=yes --db-sb-addr=$(hostname)  --db-sb-create-insecure-remote=yes  --db-nb-cluster-local-addr=$(hostname) --db-sb-cluster-local-addr=$(hostname) --ovn-northd-nb-db=tcp:$(hostname):6641 --ovn-northd-sb-db=tcp:$(hostname):6642"
EOF
systemctl enable --now ovn-northd
```

On both ovn1 and ovn2, we instruct OVS and ovn-controller to connect to the OVN control plane with:

```
OVN_REMOTE=ovn1
ENCAP_IP=$(ip --json a ls dev if0 | jq -r '.[0].addr_info[] | select(.family=="inet") | .local')
ovs-vsctl set open . external-ids:ovn-remote=tcp:${OVN_REMOTE}:6642
ovs-vsctl set open . external-ids:ovn-encap-ip=${ENCAP_IP}
ovs-vsctl set open . external-ids:ovn-encap-type=geneve
ovs-vsctl set open . external-ids:ovn-bridge=br-int
```

At this point in time, we should see the 2 chassis in OVN’s SBDB on ovn1:

```
[root@ovn1 ~]# ovn-nbctl show
[root@ovn1 ~]# ovn-sbctl show
Chassis "ed34fbb9-e3e6-4ee7-92a2-3bdebb50cc79"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.177"
        options: {csum="true"}
Chassis "0672211f-aaec-42dc-bfa0-06ab3df285de"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.26"
        options: {csum="true"}
```

### Setting up OVN hosts for standalone availability zone 1

On ovn3, we enable the OVN control plane with:

```
cat <<EOF > /etc/sysconfig/ovn
OVN_NORTHD_OPTS="--db-nb-addr=$(hostname) --db-nb-create-insecure-remote=yes --db-sb-addr=$(hostname)  --db-sb-create-insecure-remote=yes  --db-nb-cluster-local-addr=$(hostname) --db-sb-cluster-local-addr=$(hostname) --ovn-northd-nb-db=tcp:$(hostname):6641 --ovn-northd-sb-db=tcp:$(hostname):6642"
EOF
systemctl enable --now ovn-northd
```

On ovn3, we instruct OVS and ovn-controller to connect to the OVN control plane with:

```
OVN_REMOTE=ovn3
ENCAP_IP=$(ip --json a ls dev if0 | jq -r '.[0].addr_info[] | select(.family=="inet") | .local')
ovs-vsctl set open . external-ids:ovn-remote=tcp:${OVN_REMOTE}:6642
ovs-vsctl set open . external-ids:ovn-encap-ip=${ENCAP_IP}
ovs-vsctl set open . external-ids:ovn-encap-type=geneve
ovs-vsctl set open . external-ids:ovn-bridge=br-int
```

At this point in time, we should see the ovn3 chassis in OVN’s SBDB on ovn3:

```
[root@ovn3 ~]# ovn-nbctl show
[root@ovn3 ~]# ovn-sbctl show
Chassis "5f72f95b-80a3-4a77-8aab-58875dfd2a67"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.54"
        options: {csum="true"}
```

### Setting up OVN hosts for OVN IC

> **Note:** We could also set up OVN IC after fully configuring all OVN elements such as hosts, switches and routers in both az0 and az1. However, to make things clearer, we already set up all required OVN IC control plane elements during the setup phase.
> 
In order to connect the 2 independent availability zones 0 and 1, we must now add OVN Interconnection to both OVN control plane nodes, thus ovn1 and ovn3. Ovn1 will host the OVN IC database and both ovn1’s and ovn3’s OVN IC daemon will connect to this database.

![ovn-interconnection03](https://github.com/andreaskaris/blog/assets/3291433/28f5a64c-b40d-4ab2-823a-82579bccc465)

Only on ovn1, enable and run the standalone OVN IC database daemon which will start the IC NBDB and IC SBDB:

```
cat <<'EOF' > /etc/systemd/system/ovn-ic-ovsdb-standalone.service
[Unit]
Description=OVN IC database daemon
After=syslog.target

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/share/ovn/scripts/ovn-ctl --db-ic-nb-create-insecure-remote=yes \
          --db-ic-sb-create-insecure-remote=yes start_ic_ovsdb
ExecStop=/usr/share/ovn/scripts/ovn-ctl stop_ic_ovsdb

[Install]
WantedBy=multi-user.target
EOF
systemctl daemon-reload
systemctl enable --now ovn-ic-ovsdb-standalone
```

On each independent OVN instance, enable the OVN IC Daemon, that is on ovn1 and ovn3:

```
GLOBAL_DB=ovn1
cat <<EOF > /etc/systemd/system/ovn-ic-daemon.service
[Unit]
Description=OVN IC management daemon
After=syslog.target

[Service]
Type=oneshot
RemainAfterExit=yes
EnvironmentFile=-/etc/sysconfig/ovn-ic
ExecStart=/usr/share/ovn/scripts/ovn-ctl start_ic --ovn-ic-nb-db=tcp:${GLOBAL_DB}:6645 --ovn-ic-sb-db=tcp:${GLOBAL_DB}:6646 --ovn-northd-nb-db=tcp:$(hostname):6641 --ovn-northd-sb-db=tcp:$(hostname):6642
ExecStop=/usr/share/ovn/scripts/ovn-ctl stop_ic

[Install]
WantedBy=multi-user.target
EOF
systemctl daemon-reload
systemctl enable --now ovn-ic-daemon
```

Set the availability zone name for both zones and enable interconnection. On ovn1, run:

```
ovn-nbctl set NB_Global . name=az0
ovs-vsctl set open_vswitch . external_ids:ovn-is-interconn=true
```

On ovn3, run:

```
ovn-nbctl set NB_Global . name=az1
ovs-vsctl set open_vswitch . external_ids:ovn-is-interconn=true
```

Now, make sure that the OVN IC SBDB can find both availability-zones:

```
[root@ovn1 ~]#  ovn-ic-sbctl show
availability-zone az0
    gateway 0672211f-aaec-42dc-bfa0-06ab3df285de
        hostname: ovn1
        type: geneve
            ip: 192.168.122.26
availability-zone az1
    gateway 5f72f95b-80a3-4a77-8aab-58875dfd2a67
        hostname: ovn3
        type: geneve
            ip: 192.168.122.54
```

The IC daemon on ovn1 should also have populated the OVN SB chassis table with ovn3’s chassis:

```
[root@ovn1 ~]# ovn-sbctl show
Chassis "ed34fbb9-e3e6-4ee7-92a2-3bdebb50cc79"
    hostname: ovn2
    Encap geneve
        ip: "192.168.122.177"
        options: {csum="true"}
Chassis "0672211f-aaec-42dc-bfa0-06ab3df285de"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.26"
        options: {csum="true"}
Chassis "5f72f95b-80a3-4a77-8aab-58875dfd2a67"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.54"
        options: {csum="true"}
```

And the same should have happened on ovn3 with ovn1’s chassis:
```
[root@ovn3 ~]# ovn-sbctl show
Chassis "5f72f95b-80a3-4a77-8aab-58875dfd2a67"
    hostname: ovn3
    Encap geneve
        ip: "192.168.122.54"
        options: {csum="true"}
Chassis "0672211f-aaec-42dc-bfa0-06ab3df285de"
    hostname: ovn1
    Encap geneve
        ip: "192.168.122.26"
        options: {csum="true"}
```

## Configuring the dataplane - hosts and virtual networks

In both availability zones, we will simulate virtual hosts and connect them to each other via OVN virtual networks.

### Configuring the dataplane inside availability zone 0

We will now simulate 2 hosts, each of which resides inside its own subnet on its own host. The hosts will be able to communicate via OVN router `r1`:

![ovn-interconnection04](https://github.com/andreaskaris/blog/assets/3291433/7f700f88-d60b-4d40-a81e-3a694a5325bd)

On ovn1, configure veth01:

```
ip netns add ns0
ip link add name veth01 type veth peer name port01
ip link set dev port01 up
ip link set dev veth01 netns ns0
ip netns exec ns0 ip link set dev lo up
ip netns exec ns0 ip link set dev veth01 up
ip netns exec ns0 ip link set veth01 address 00:00:00:00:00:01
ip netns exec ns0 ip address add 10.0.0.1/24 dev veth01
ip netns exec ns0 ip r a default via 10.0.0.254
ovs-vsctl add-port br-int port01
ovs-vsctl set Interface port01 external_ids:iface-id=port01
```

On ovn2, configure veth02:

```
ip netns add ns0
ip link add name veth02 type veth peer name port02
ip link set dev port02 up
ip link set dev veth02 netns ns0
ip netns exec ns0 ip link set dev lo up
ip netns exec ns0 ip link set dev veth02 up
ip netns exec ns0 ip link set veth02 address 00:00:00:00:01:02
ip netns exec ns0 ip address add 10.0.1.1/24 dev veth02
ip netns exec ns0 ip r a default via 10.0.1.254
ovs-vsctl add-port br-int port02
ovs-vsctl set Interface port02 external_ids:iface-id=port02
```

On ovn1, create the virtual switch and router required to establish connectivity between both interfaces:

```
ovn-nbctl ls-add s0
ovn-nbctl lsp-add s0 port01
ovn-nbctl lsp-set-addresses port01 00:00:00:00:00:01

ovn-nbctl ls-add s1
ovn-nbctl lsp-add s1 port02
ovn-nbctl lsp-set-addresses port02 00:00:00:00:01:02

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

Verify that 10.0.0.1 inside ns0 on ovn1 can reach 10.0.1.1 inside ns0 on ovn2, via router r1 (10.0.0.254):

```
[root@ovn1 ~]# ip netns exec ns0 ping -c1 -W1 10.0.0.254
PING 10.0.0.254 (10.0.0.254) 56(84) bytes of data.
64 bytes from 10.0.0.254: icmp_seq=1 ttl=254 time=0.370 ms

--- 10.0.0.254 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.370/0.370/0.370/0.000 ms
[root@ovn1 ~]# ip netns exec ns0 ping -c1 -W1 10.0.1.1
PING 10.0.1.1 (10.0.1.1) 56(84) bytes of data.
64 bytes from 10.0.1.1: icmp_seq=1 ttl=63 time=2.30 ms

--- 10.0.1.1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 2.299/2.299/2.299/0.000 ms
[root@ovn1 ~]# ip netns exec ns0 traceroute -I -n 10.0.1.1
traceroute to 10.0.1.1 (10.0.1.1), 30 hops max, 60 byte packets
 1  10.0.0.254  1.743 ms  1.784 ms  1.827 ms
 2  10.0.1.1  4.080 ms  4.091 ms  4.091 ms
```

### Configuring the dataplane inside availability zone 1

We will now simulate 2 hosts, each of which resides inside its own subnet and namespace on ovn3. The hosts will be able to communicate via OVN router `r2`:

![ovn-interconnection05](https://github.com/andreaskaris/blog/assets/3291433/65c91806-c881-4588-af0b-0eb32d5921af)

On ovn3, configure veth03 inside ns0:

```
ip link add name veth03 type veth peer name port03
ip netns add ns0
ip link set dev veth03 netns ns0
ip netns exec ns0 ip link set dev lo up
ip netns exec ns0 ip link set dev veth03 up
ip netns exec ns0 ip link set veth03 address 00:00:00:00:02:03
ip netns exec ns0 ip address add 10.0.2.1/24 dev veth03
ip netns exec ns0 ip r add default via 10.0.2.254
ip link set dev port03 up
ovs-vsctl add-port br-int port03
ovs-vsctl set Interface port03 external_ids:iface-id=port03
```

On ovn3, configure veth04 inside ns1:

```
ip link add name veth04 type veth peer name port04
ip netns add ns1
ip link set dev veth04 netns ns1
ip netns exec ns1 ip link set dev lo up
ip netns exec ns1 ip link set dev veth04 up
ip netns exec ns1 ip link set veth04 address 00:00:00:00:03:04
ip netns exec ns1 ip address add 10.0.3.1/24 dev veth04
ip netns exec ns1 ip r add default via 10.0.3.254
ip link set dev port04 up
ovs-vsctl add-port br-int port04
ovs-vsctl set Interface port04 external_ids:iface-id=port04
```

On ovn3, create the virtual switches and router required to establish connectivity between both interfaces:

```
ovn-nbctl ls-add s3
ovn-nbctl lsp-add s3 port03
ovn-nbctl lsp-set-addresses port03 00:00:00:00:02:03

ovn-nbctl ls-add s4
ovn-nbctl lsp-add s4 port04
ovn-nbctl lsp-set-addresses port04 00:00:00:00:03:04

ovn-nbctl lr-add r2
ovn-nbctl lrp-add r2 r2s3 00:00:00:00:02:ff 10.0.2.254/24
ovn-nbctl lsp-add s3 s3r2 
ovn-nbctl lsp-set-addresses s3r2 00:00:00:00:02:ff
ovn-nbctl lsp-set-options s3r2 router-port=r2s3
ovn-nbctl lsp-set-type s3r2 router
ovn-nbctl lrp-add r2 r2s4 00:00:00:00:03:ff 10.0.3.254/24
ovn-nbctl lsp-add s4 s4r2 
ovn-nbctl lsp-set-addresses s4r2 00:00:00:00:03:ff
ovn-nbctl lsp-set-options s4r2 router-port=r2s4
ovn-nbctl lsp-set-type s4r2 router
```

Verify that 10.0.2.1 inside ns0 on ovn3 can reach 10.0.3.1 inside ns1 on ovn3, via router r2 (10.0.2.254):

```
[root@ovn3 ~]# ip netns exec ns0 ping -c1 -W1 10.0.2.254
PING 10.0.2.254 (10.0.2.254) 56(84) bytes of data.
64 bytes from 10.0.2.254: icmp_seq=1 ttl=254 time=36.4 ms

--- 10.0.2.254 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 36.396/36.396/36.396/0.000 ms
[root@ovn3 ~]# ip netns exec ns0 ping -c1 -W1 10.0.3.1
PING 10.0.3.1 (10.0.3.1) 56(84) bytes of data.
64 bytes from 10.0.3.1: icmp_seq=1 ttl=63 time=10.7 ms

--- 10.0.3.1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 10.715/10.715/10.715/0.000 ms
[root@ovn3 ~]# ip netns exec ns0 traceroute -I -n 10.0.3.1
traceroute to 10.0.3.1 (10.0.3.1), 30 hops max, 60 byte packets
 1  10.0.2.254  2.416 ms  2.687 ms  2.924 ms
 2  10.0.3.1  2.132 ms  2.156 ms  2.164 ms
```

### Configuring the dataplane with OVN Interconnection

We will now interconnect the 2 availability zones’ dataplanes with OVN Interconnection.

![ovn-interconnection06](https://github.com/andreaskaris/blog/assets/3291433/dbbd32ce-9f65-4dc4-b343-fe7f647e1d1d)

On ovn1, create a transit switch inside the Interconnection database:

```
ovn-ic-nbctl ts-add ts1
```
 
OVN IC will automatically create the transit switch inside all OVN NBDBs, e.g. inside the NBDB of ovn3:

```
[root@ovn3 ~]# ovn-nbctl find logical_switch other_config:interconn-ts=ts1
_uuid               : e6c545b4-28f3-4f10-b1ac-cb4cab39db04
acls                : []
copp                : []
dns_records         : []
external_ids        : {}
forwarding_groups   : []
load_balancer       : []
load_balancer_group : []
name                : ts1
other_config        : {interconn-ts=ts1, requested-tnl-key="16711682"}
ports               : []
qos_rules           : []
```

Connect `r1` inside availability zone 0 to transit switch `ts1`. Run the following commands on ovn1:

```
ovn-nbctl lrp-add r1 lrp-r1-ts1 aa:aa:aa:aa:aa:01 169.254.100.1/24
ovn-nbctl lsp-add ts1 lsp-ts1-r1 -- \
        lsp-set-addresses lsp-ts1-r1 router -- \
        lsp-set-type lsp-ts1-r1 router -- \
        lsp-set-options lsp-ts1-r1 router-port=lrp-r1-ts1
chassis=$(ovn-sbctl find chassis hostname=ovn1 | awk '/^name/ {print $NF}' | sed 's/"//g')
ovn-nbctl lrp-set-gateway-chassis lrp-r1-ts1 ${chassis}
ovn-nbctl lr-route-add r1 10.0.2.0/23 169.254.100.2
```

Connect `r2` inside availability zone 1 to transit switch `ts1`. Run the following commands on ovn3:

```
ovn-nbctl lrp-add r2 lrp-r2-ts1 aa:aa:aa:aa:aa:02 169.254.100.2/24
ovn-nbctl lsp-add ts1 lsp-ts1-r2 -- \
        lsp-set-addresses lsp-ts1-r2 router -- \
        lsp-set-type lsp-ts1-r2 router -- \
        lsp-set-options lsp-ts1-r2 router-port=lrp-r2-ts1
chassis=$(ovn-sbctl find chassis hostname=ovn3 | awk '/^name/ {print $NF}' | sed 's/"//g')
ovn-nbctl lrp-set-gateway-chassis lrp-r2-ts1 ${chassis}
ovn-nbctl lr-route-add r2 10.0.0.0/23 169.254.100.1
```

Check that host 10.0.0.1 in ns0 on ovn1 can ping host 10.0.3.1 in ns1 on ovn3, via the interconnection dataplane:

```
[root@ovn1 ~]# ip netns exec ns0 ping -c1 -W1 10.0.3.1
PING 10.0.3.1 (10.0.3.1) 56(84) bytes of data.
64 bytes from 10.0.3.1: icmp_seq=1 ttl=62 time=0.608 ms

--- 10.0.3.1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.608/0.608/0.608/0.000 ms
[root@ovn1 ~]# ip netns exec ns0 traceroute -I -n 10.0.3.1
traceroute to 10.0.3.1 (10.0.3.1), 30 hops max, 60 byte packets
 1  10.0.0.254  1.991 ms  2.036 ms  2.084 ms
 2  10.0.3.1  4.390 ms  4.425 ms  4.429 ms
 3  10.0.3.1  3.227 ms  3.222 ms  3.219 ms
```
