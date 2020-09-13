## Failover of OVS active/standby bonds ##
In a cloud environment, what happens if an active-backup bond's active interface goes down? 

If the VMs are constantly chatting, this is not too much of an issue, as outgoing VM traffic will make it out the formerly passive interface, into the network's physical switches which will learn the new source of the MAC address (at least for all switches that receive the frames, hence which are in the path between source and destination or in the case of broadcast frames, all switches that share the same L2 domain). 

Additionally to the above, and particularly for traffic towards VMs that are not chatty: If the active interface becomes unavailable, then the switch connected to that interface will detect that link as down and will flush its MAC entries for that port and switch to a flooding behavior. It will also send a TCN to the root bridge of the network which in turn will flood that TCN to all bridges in the network, shortening the MAC address timeout in the network to a low level. This in turn will tell the network to quickly time out MAC addresses and will lead to unknown unicast flooding for those MAC addresses. I think in Cisco world, this is 15 seconds, but it depends on the vendor. Active/passive bonds don't need to do anything special, the logic is built into the whole spanning-tree operation, although I only know the Cisco stuff, I don't know how other vendors deal with it. If you want failovers without loss of communication, use LACP instead. However, OVS is trying to be a good neighbor and has a neat feature ...
RARP to announce contents of OVS FDB

[http://docs.openvswitch.org/en/latest/topics/bonding/](http://docs.openvswitch.org/en/latest/topics/bonding/)
~~~
    When a slave becomes disabled, the vswitch immediately chooses a new output port for traffic that was destined for that slave (see bond_enable_slave()). It also sends a “gratuitous learning packet”, specifically a RARP, on the bond port (on the newly chosen slave) for each MAC address that the vswitch has learned on a port other than the bond (see bundle_send_learning_packets()

    ), to teach the physical switch that the new slave should be used in place of the one that is now disabled. (This behavior probably makes sense only for a vswitch that has only one port (the bond) connected to a physical switch; vswitchd should probably provide a way to disable or configure it in other scenarios.)
~~~

Now, that code actually is here and not where the doc says it should be, but it doesn't really matter: [https://github.com/openvswitch/ovs/blame/b47e7e2bac7f21f8c81aaeb1068598a6064c348b/ofproto/ofproto-dpif.c#L3273](https://github.com/openvswitch/ovs/blame/b47e7e2bac7f21f8c81aaeb1068598a6064c348b/ofproto/ofproto-dpif.c#L3273)


Configure this on a system with OVS. I tried this with OVS 2.9:
~~~
ovs-vsctl add-br br1 -- set bridge br1 datapath_type=netdev

ip link add ovs-bond-if0 type veth peer name lx-bond-if0

ip link add ovs-bond-if1 type veth peer name lx-bond-if1

ip link add lx-bond0 type bond miimon 100 mode 1

ip link set dev lx-bond-if0 master lx-bond0

ip link set dev lx-bond-if1 master lx-bond0

ip link set dev lx-bond-if0 up

ip link set dev lx-bond-if1 up

ip link set dev ovs-bond-if0 up

ip link set dev ovs-bond-if1 up

ip link set dev lx-bond0 up

ovs-vsctl add-bond br1 testbond1 ovs-bond-if0 ovs-bond-if1

ip link add name lx-bond0.905 link lx-bond0 type vlan id 905

ip link set dev lx-bond0.905 up

ip a a dev lx-bond0.905 192.168.123.10/24

ip link add veth2 type veth peer name veth3

ip netns add testns

ip link set dev veth2 netns testns

ip link set dev veth3 up

ip netns exec testns ip link set dev lo up

ip netns exec testns ip link set dev veth2 up

ip netns exec testns ip a a dev veth2 192.168.123.11/24

ovs-vsctl add-port br1 veth3 tag=905
~~~

That creates a linux bond and an OVS bond, active-standby, and adds a port with VLAN tag 905 in namestapce testns and another port in the global namespace on lx-bond0.905 192.168.123.10/24 (aa:a3:6c:56:f3:58) is behind the Linux bond, 192.168.123.11 (82:43:37:7a:0f:20) is behind the OVS bond. According to the upstream guide, I'd expect to see at least RARP for 192.168.123.11.

~~~
[root@overcloud-novacomputeiha-0 ~]# cat /proc/net/bonding/lx-bond0

Ethernet Channel Bonding Driver: v3.7.1 (April 27, 2011)


Bonding Mode: fault-tolerance (active-backup)

Primary Slave: None

Currently Active Slave: lx-bond-if1

MII Status: up

MII Polling Interval (ms): 100

Up Delay (ms): 0

Down Delay (ms): 0


Slave Interface: lx-bond-if0

MII Status: up

Speed: 10000 Mbps

Duplex: full

Link Failure Count: 1

Permanent HW addr: fe:b8:03:54:bc:0d

Slave queue ID: 0


Slave Interface: lx-bond-if1

MII Status: up

Speed: 10000 Mbps

Duplex: full

Link Failure Count: 0

Permanent HW addr: 0e:f1:64:48:25:84

Slave queue ID: 0

[root@overcloud-novacomputeiha-0 ~]# ovs-appctl bond/show

---- testbond1 ----

bond_mode: active-backup

bond may use recirculation: no, Recirc-ID : -1

bond-hash-basis: 0

updelay: 0 ms

downdelay: 0 ms

lacp_status: off

lacp_fallback_ab: false

active slave mac: c6:ff:04:24:49:74(ovs-bond-if1)


slave ovs-bond-if0: enabled

    may_enable: true


slave ovs-bond-if1: enabled

    active slave

    may_enable: true
~~~

Ping the destination to update the ARP cache:

~~~
[root@overcloud-novacomputeiha-0 ~]# ping 192.168.123.11

PING 192.168.123.11 (192.168.123.11) 56(84) bytes of data.

64 bytes from 192.168.123.11: icmp_seq=1 ttl=64 time=0.292 ms

64 bytes from 192.168.123.11: icmp_seq=2 ttl=64 time=0.227 ms

^C

--- 192.168.123.11 ping statistics ---

2 packets transmitted, 2 received, 0% packet loss, time 1001ms

rtt min/avg/max/mdev = 0.227/0.259/0.292/0.036 ms

[root@overcloud-novacomputeiha-0 ~]# arp -n | grep 192.168.123

192.168.123.11           ether   22:a6:81:76:8a:78   C

    lx-bond0.905
~~~

Now start a tcpdump and shut down the active port:

~~~
[root@overcloud-novacomputeiha-0 ~]# tcpdump -nne -i ovs-bond-if0 & ip

link set dev ovs-bond-if1 down ; sleep 3 ; killall tcpdump

[1] 28019

tcpdump: verbose output suppressed, use -v or -vv for full protocol decode

listening on ovs-bond-if0, link-type EN10MB (Ethernet), capture size 262144 bytes

19:02:50.565443 22:a6:81:76:8a:78 > ff:ff:ff:ff:ff:ff, ethertype 802.1Q (0x8100), length 46: vlan 905, p 0, ethertype Reverse ARP, Reverse Request who-is 22:a6:81:76:8a:78 tell 22:a6:81:76:8a:78, length 28

19:02:50.590969 fe:b8:03:54:bc:0d > 33:33:00:00:00:01, ethertype IPv6 (0x86dd), length 86: fe80::fcb8:3ff:fe54:bc0d > ff02::1: ICMP6, neighbor advertisement, tgt is fe80::fcb8:3ff:fe54:bc0d, length 32

19:02:50.591127 fe:b8:03:54:bc:0d > ff:ff:ff:ff:ff:ff, ethertype 802.1Q (0x8100), length 46: vlan 905, p 0, ethertype ARP, Request who-has 192.168.123.10 tell 192.168.123.10, length 28

19:02:50.591172 fe:b8:03:54:bc:0d > 33:33:00:00:00:01, ethertype 802.1Q (0x8100), length 90: vlan 905, p 0, ethertype IPv6, fe80::fcb8:3ff:fe54:bc0d > ff02::1: ICMP6, neighbor advertisement, tgt

is fe80::fcb8:3ff:fe54:bc0d, length 32 19:02:50.747058 fe:b8:03:54:bc:0d > 22:a6:81:76:8a:78, ethertype 802.1Q (0x8100), length 46: vlan 905, p 0, ethertype ARP, Request who-has 192.168.123.11 tell 192.168.123.10, length 28

19:02:50.747309 22:a6:81:76:8a:78 > fe:b8:03:54:bc:0d, ethertype 802.1Q (0x8100), length 46: vlan 905, p 0, ethertype ARP, Reply 192.168.123.11 is-at 22:a6:81:76:8a:78, length 28


[root@overcloud-novacomputeiha-0 ~]# 6 packets captured

6 packets received by filter

0 packets dropped by kernel

^C

[1]+  Done                    tcpdump -nne -i ovs-bond-if0

[root@overcloud-novacomputeiha-0 ~]# fg

-bash: fg: current: no such job

[root@overcloud-novacomputeiha-0 ~]# ^C

[root@overcloud-novacomputeiha-0 ~]#
~~~

Note this line:
~~~
19:02:50.565443 22:a6:81:76:8a:78 > ff:ff:ff:ff:ff:ff, ethertype 802.1Q (0x8100), length 46: vlan 905, p 0, ethertype Reverse ARP, Reverse Request who-is 22:a6:81:76:8a:78 tell 22:a6:81:76:8a:78, length 28
~~~

OVS is indeed announcing the content of its FDB to its neighbor, as soon as the active port goes down.
