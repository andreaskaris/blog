# ARP and the neighbor table #

Document of reference: [http://haifux.org/lectures/180/netLec2.pdf](http://haifux.org/lectures/180/netLec2.pdf)

# Disabling ARP #

## NUD NOARP ##

From [http://haifux.org/lectures/180/netLec2.pdf](http://haifux.org/lectures/180/netLec2.pdf)
~~~
Neighboring Subsystem – states
● Special states:
● NUD_NOARP
● NUD_PERMANENT
● No state transitions are allowed from these states to another 
state.
~~~

This means that if an ARP entry ends up in `NUD_NOARP`, it will not time out, update or change in any way. 
An administrator needs to delete the ARP entry for ARP resolution to work again properly for the IP.

List NOARP entries:
~~~
[root@overcloud-novacomputeiha-0 ~]# ip neigh ls nud noarp
172.16.2.9 dev lo lladdr 00:00:00:00:00:00 NOARP
127.0.0.1 dev lo lladdr 00:00:00:00:00:00 NOARP
ff02::1:ff14:62d7 dev vlan902 lladdr 33:33:ff:14:62:d7 NOARP
ff02::1:ff8d:846d dev vlan903 lladdr 33:33:ff:8d:84:6d NOARP
ff02::16 dev ovs-bond-if1 lladdr 33:33:00:00:00:16 NOARP
ff02::1:ff9f:c668 dev em1 lladdr 33:33:ff:9f:c6:68 NOARP
ff02::1 dev lx-bond0 lladdr 33:33:00:00:00:01 NOARP
ff02::16 dev vlan901 lladdr 33:33:00:00:00:16 NOARP
ff02::16 dev em1 lladdr 33:33:00:00:00:16 NOARP
ff02::16 dev br-ex lladdr 33:33:00:00:00:16 NOARP
ff02::1:ff03:a0eb dev vlan901 lladdr 33:33:ff:03:a0:eb NOARP
ff02::16 dev vlan902 lladdr 33:33:00:00:00:16 NOARP
ff02::2 dev em4 lladdr 33:33:00:00:00:02 NOARP
ff02::16 dev veth3 lladdr 33:33:00:00:00:16 NOARP
ff02::16 dev vlan903 lladdr 33:33:00:00:00:16 NOARP
ff02::16 dev ovs-bond-if0 lladdr 33:33:00:00:00:16 NOARP
ff02::1:ff24:4974 dev ovs-bond-if1 lladdr 33:33:ff:24:49:74 NOARP
ff02::16 dev vxlan_sys_4789 lladdr 33:33:00:00:00:16 NOARP
ff02::1:ffe5:da4c dev br-ex lladdr 33:33:ff:e5:da:4c NOARP
ff02::16 dev em4 lladdr 33:33:00:00:00:16 NOARP
ff02::2 dev vlan901 lladdr 33:33:00:00:00:02 NOARP
ff02::2 dev em1 lladdr 33:33:00:00:00:02 NOARP
ff02::2 dev br-ex lladdr 33:33:00:00:00:02 NOARP
ff02::1:ff9f:c66b dev em4 lladdr 33:33:ff:9f:c6:6b NOARP
ff02::1:ff91:8481 dev ovs-bond-if0 lladdr 33:33:ff:91:84:81 NOARP
ff02::2 dev vlan902 lladdr 33:33:00:00:00:02 NOARP
ff02::16 dev lx-bond0 lladdr 33:33:00:00:00:16 NOARP
ff02::1:ff5f:4750 dev veth3 lladdr 33:33:ff:5f:47:50 NOARP
::1 dev lo lladdr 00:00:00:00:00:00 NOARP
ff02::1:ffb7:7124 dev vxlan_sys_4789 lladdr 33:33:ff:b7:71:24 NOARP
ff02::2 dev vlan903 lladdr 33:33:00:00:00:02 NOARP
ff02::1:ff54:bc0d dev lx-bond0 lladdr 33:33:ff:54:bc:0d NOARP
~~~

## Per interface ##
ARP can be disabled globally on a per interface basis. 
This may be useful e.g. when using some failover / redundancy solution implemented in user space:
~~~
[root@overcloud-novacomputeiha-0 ~]# ping 192.168.123.11
PING 192.168.123.11 (192.168.123.11) 56(84) bytes of data.
64 bytes from 192.168.123.11: icmp_seq=1 ttl=64 time=0.342 ms
64 bytes from 192.168.123.11: icmp_seq=2 ttl=64 time=0.181 ms
^C
--- 192.168.123.11 ping statistics ---
2 packets transmitted, 2 received, 0% packet loss, time 1000ms
rtt min/avg/max/mdev = 0.181/0.261/0.342/0.082 ms
[root@overcloud-novacomputeiha-0 ~]# ip r g 192.168.123.11
192.168.123.11 dev lx-bond0.905 src 192.168.123.10 
    cache 
[root@overcloud-novacomputeiha-0 ~]# ip link set dev lx-bond0.905 arp off
[root@overcloud-novacomputeiha-0 ~]# ip neigh ls | grep 192.168.123.11
[root@overcloud-novacomputeiha-0 ~]# ip neigh ls nud noarp | grep 192.168.123.11
[root@overcloud-novacomputeiha-0 ~]# ping 192.168.123.11
PING 192.168.123.11 (192.168.123.11) 56(84) bytes of data.
^C
--- 192.168.123.11 ping statistics ---
2 packets transmitted, 0 received, 100% packet loss, time 1001ms
~~~

The interface will show `NOARP`:
~~~
[root@overcloud-novacomputeiha-0 ~]# ip link ls dev lx-bond0.905
31: lx-bond0.905@lx-bond0: <BROADCAST,MULTICAST,NOARP,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP mode DEFAULT group default qlen 1000
    link/ether fe:b8:03:54:bc:0d brd ff:ff:ff:ff:ff:ff
~~~

This also means that the opposite side cannot resolve the IP address:
~~~
[root@overcloud-novacomputeiha-0 ~]# ip netns exec test2 ping 192.168.123.10
PING 192.168.123.10 (192.168.123.10) 56(84) bytes of data.
^C
--- 192.168.123.10 ping statistics ---
2 packets transmitted, 0 received, 100% packet loss, time 999ms

[root@overcloud-novacomputeiha-0 ~]# arp -n | grep 192.168.123
[root@overcloud-novacomputeiha-0 ~]# ip netns exec test2 arp -n | grep 192.168.123
192.168.123.10                   (incomplete)                              veth2
[root@overcloud-novacomputeiha-0 ~]# 
~~~

## Per ARP entry ##

Let's set NOARP for 192.168.123.11:
~~~
[root@overcloud-novacomputeiha-0 ~]# ip neigh ls | grep 192.168.123
192.168.123.11 dev lx-bond0.905 lladdr 22:a6:81:76:8a:78 REACHABLE
[root@overcloud-novacomputeiha-0 ~]# ip neigh change 192.168.123.11 dev lx-bond0.905 lladdr 22:a6:81:76:8a:78 nud noarp
[root@overcloud-novacomputeiha-0 ~]# ip neigh ls | grep 192.168.123
[root@overcloud-novacomputeiha-0 ~]# ip neigh ls nud noarp | grep 192.168.123
192.168.123.11 dev lx-bond0.905 lladdr 22:a6:81:76:8a:78 NOARP
[root@overcloud-novacomputeiha-0 ~]# ping 192.168.123.11
PING 192.168.123.11 (192.168.123.11) 56(84) bytes of data.
64 bytes from 192.168.123.11: icmp_seq=1 ttl=64 time=0.344 ms
^C
--- 192.168.123.11 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.344/0.344/0.344/0.000 ms
~~~

Note that flipping the state for the interface resets the NOARP entry for the neighbor:
~~~
[root@overcloud-novacomputeiha-0 ~]# ip link set dev lx-bond0.905 arp off
[root@overcloud-novacomputeiha-0 ~]# ip neigh ls nud noarp | grep 192.168.123
[root@overcloud-novacomputeiha-0 ~]# ip link set dev lx-bond0.905 arp on
[root@overcloud-novacomputeiha-0 ~]# ip neigh ls nud noarp | grep 192.168.123
[root@overcloud-novacomputeiha-0 ~]# ip neigh change 192.168.123.11 dev lx-bond0.905 lladdr 22:a6:81:76:8a:78 nud noarp
RTNETLINK answers: No such file or directory
[root@overcloud-novacomputeiha-0 ~]# ping 192.168.123.11
PING 192.168.123.11 (192.168.123.11) 56(84) bytes of data.
64 bytes from 192.168.123.11: icmp_seq=1 ttl=64 time=0.480 ms
^C
--- 192.168.123.11 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.480/0.480/0.480/0.000 ms
[root@overcloud-novacomputeiha-0 ~]# ip neigh change 192.168.123.11 dev lx-bond0.905 lladdr 22:a6:81:76:8a:78 nud noarp
[root@overcloud-novacomputeiha-0 ~]# 
~~~

We can see that ARP learning is disabled with the following test. Note that we have to ping from the neighbor.
Otherwise, even with correct ARP resolution, this won't work:
~~~
[root@overcloud-novacomputeiha-0 ~]# ping 192.168.123.11
PING 192.168.123.11 (192.168.123.11) 56(84) bytes of data.
^C
--- 192.168.123.11 ping statistics ---
2 packets transmitted, 0 received, 100% packet loss, time 999ms
[root@overcloud-novacomputeiha-0 ~]# ip netns exec test2 ip link set dev veth2 address 22:a6:81:76:8a:78
[root@overcloud-novacomputeiha-0 ~]# ping 192.168.123.11
PING 192.168.123.11 (192.168.123.11) 56(84) bytes of data.
^C
--- 192.168.123.11 ping statistics ---
5 packets transmitted, 0 received, 100% packet loss, time 4000ms

[root@overcloud-novacomputeiha-0 ~]# ip netns exec test2 ping 192.168.123.10
PING 192.168.123.10 (192.168.123.10) 56(84) bytes of data.
64 bytes from 192.168.123.10: icmp_seq=1 ttl=64 time=0.426 ms
^C
--- 192.168.123.10 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.426/0.426/0.426/0.000 ms
~~~

~~~
[root@overcloud-novacomputeiha-0 ~]# ip netns exec test2 ip a ls dev veth2
33: veth2@if32: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 22:a6:81:76:8a:78 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 192.168.123.11/24 scope global veth2
       valid_lft forever preferred_lft forever
    inet6 fe80::20a6:81ff:fe76:8a78/64 scope link 
       valid_lft forever preferred_lft forever
[root@overcloud-novacomputeiha-0 ~]# ip netns exec test2 ip link set dev veth2 address 22:a6:81:76:8a:79
[root@overcloud-novacomputeiha-0 ~]# ip netns exec test2 ip a ls dev veth2
33: veth2@if32: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 22:a6:81:76:8a:79 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 192.168.123.11/24 scope global veth2
       valid_lft forever preferred_lft forever
    inet6 fe80::20a6:81ff:fe76:8a78/64 scope link 
       valid_lft forever preferred_lft forever
[root@overcloud-novacomputeiha-0 ~]# ping 192.168.123.11
PING 192.168.123.11 (192.168.123.11) 56(84) bytes of data.
^C
--- 192.168.123.11 ping statistics ---
2 packets transmitted, 0 received, 100% packet loss, time 999ms

[root@overcloud-novacomputeiha-0 ~]# ip link set dev lx-bond0.905 arp off
[root@overcloud-novacomputeiha-0 ~]# ip link set dev lx-bond0.905 arp on
[root@overcloud-novacomputeiha-0 ~]# ping 192.168.123.11
PING 192.168.123.11 (192.168.123.11) 56(84) bytes of data.
64 bytes from 192.168.123.11: icmp_seq=1 ttl=64 time=0.566 ms
^C
--- 192.168.123.11 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.566/0.566/0.566/0.000 ms
[root@overcloud-novacomputeiha-0 ~]# 
~~~

Here's how it looks with NUD NOARP:
~~~
[root@overcloud-novacomputeiha-0 ~]# ip neigh change 192.168.123.11 dev lx-bond0.905 lladdr 22:a6:81:76:8a:78 nud noarp
[root@overcloud-novacomputeiha-0 ~]# ip netns exec test2 ping 192.168.123.10
PING 192.168.123.10 (192.168.123.10) 56(84) bytes of data.
64 bytes from 192.168.123.10: icmp_seq=1 ttl=64 time=0.175 ms
^C
--- 192.168.123.10 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.175/0.175/0.175/0.000 ms
[root@overcloud-novacomputeiha-0 ~]# ping 192.168.123.11
PING 192.168.123.11 (192.168.123.11) 56(84) bytes of data.
64 bytes from 192.168.123.11: icmp_seq=1 ttl=64 time=0.229 ms
64 bytes from 192.168.123.11: icmp_seq=2 ttl=64 time=0.190 ms
^C
--- 192.168.123.11 ping statistics ---
2 packets transmitted, 2 received, 0% packet loss, time 999ms
rtt min/avg/max/mdev = 0.190/0.209/0.229/0.024 ms
[root@overcloud-novacomputeiha-0 ~]# ip netns exec test2 ip link set dev veth2 address 22:a6:81:76:8a:79
[root@overcloud-novacomputeiha-0 ~]# ping 192.168.123.11
PING 192.168.123.11 (192.168.123.11) 56(84) bytes of data.
^C
--- 192.168.123.11 ping statistics ---
1 packets transmitted, 0 received, 100% packet loss, time 0ms

[root@overcloud-novacomputeiha-0 ~]# ip netns exec test2 ping 192.168.123.10
PING 192.168.123.10 (192.168.123.10) 56(84) bytes of data.
^C
--- 192.168.123.10 ping statistics ---
2 packets transmitted, 0 received, 100% packet loss, time 999ms

[root@overcloud-novacomputeiha-0 ~]# ip link set dev lx-bond0.905 arp off
[root@overcloud-novacomputeiha-0 ~]# ip link set dev lx-bond0.905 arp on
[root@overcloud-novacomputeiha-0 ~]# ip netns exec test2 ping 192.168.123.10
PING 192.168.123.10 (192.168.123.10) 56(84) bytes of data.
64 bytes from 192.168.123.10: icmp_seq=1 ttl=64 time=0.377 ms
^C
--- 192.168.123.10 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.377/0.377/0.377/0.000 ms
~~~
