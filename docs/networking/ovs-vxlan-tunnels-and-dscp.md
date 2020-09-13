# OVS VXLAN tunnels and DSCP #

## Lab ##

Fedora 32
~~~
yum install openvswitch -y
systemctl enable --now openvswitch
~~~

~~~
[root@ovs-tunnel-test-1 ~]# ovs-vsctl show | grep version
    ovs_version: "2.13.0"
~~~

## Setup ##

* node A
~~~
ovs-vsctl --may-exist add-br br-int -- set Bridge br-int datapath_type=system -- br-set-external-id br-int bridge-id br-int
ovs-vsctl add-port br-int vxlan0 -- set interface vxlan0 type=vxlan options:remote_ip=192.168.122.174
ip netns add private
ip link add name veth-host type veth peer name veth-guest
ovs-vsctl add-port br-int veth-host
ip link set dev veth-guest netns private
ip link set dev veth-host up
ip -n private set dev veth-guest up
ip -n private link set dev veth-guest up
ip -n private link set dev lo up
ip -n private a a dev veth-guest 192.168.123.1/24
~~~

* node B
~~~
ovs-vsctl --may-exist add-br br-int -- set Bridge br-int datapath_type=system -- br-set-external-id br-int bridge-id br-int
ovs-vsctl add-port br-int vxlan0 -- set interface vxlan0 type=vxlan options:remote_ip=192.168.122.41
ip netns add private
ip link add name veth-host type veth peer name veth-guest
ovs-vsctl add-port br-int veth-host
ip link set dev veth-guest netns private
ip link set dev veth-host up
ip -n private set dev veth-guest up
ip -n private link set dev veth-guest up
ip -n private link set dev lo up
ip -n private a a dev veth-guest 192.168.123.2/24
~~~

## Only setting the ECN bits to ECT(0) - ToS 0x2 ##

~~~
[root@ovs-tunnel-test-1 ~]# ip netns exec private ping 192.168.123.2 -c1 -W1 -Q 0x2
PING 192.168.123.2 (192.168.123.2) 56(84) bytes of data.
64 bytes from 192.168.123.2: icmp_seq=1 ttl=64 time=2.51 ms

--- 192.168.123.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 2.513/2.513/2.513/0.000 ms
[root@ovs-tunnel-test-1 ~]# 
~~~

~~~
[root@ovs-tunnel-test-2 ~]# tshark -nn -i eth0 -O ip port vxlan
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
Frame 1: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
Ethernet II, Src: 52:54:00:e3:af:ab, Dst: 52:54:00:9e:bc:3a
Internet Protocol Version 4, Src: 192.168.122.41, Dst: 192.168.122.174
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 134
    Identification: 0x8486 (33926)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0x3fb6 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.122.41
    Destination: 192.168.122.174
User Datagram Protocol, Src Port: 34788, Dst Port: 4789
Virtual eXtensible Local Area Network
Ethernet II, Src: 16:28:3c:ba:5b:13, Dst: 56:13:a0:be:1f:f4
Internet Protocol Version 4, Src: 192.168.123.1, Dst: 192.168.123.2
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0xc12e (49454)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0x0224 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.1
    Destination: 192.168.123.2
Internet Control Message Protocol

Frame 2: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
Ethernet II, Src: 52:54:00:9e:bc:3a, Dst: 52:54:00:e3:af:ab
Internet Protocol Version 4, Src: 192.168.122.174, Dst: 192.168.122.41
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 134
    Identification: 0x717a (29050)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0x52c2 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.122.174
    Destination: 192.168.122.41
User Datagram Protocol, Src Port: 45375, Dst Port: 4789
Virtual eXtensible Local Area Network
Ethernet II, Src: 56:13:a0:be:1f:f4, Dst: 16:28:3c:ba:5b:13
Internet Protocol Version 4, Src: 192.168.123.2, Dst: 192.168.123.1
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0x1916 (6422)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0xea3c [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.2
    Destination: 192.168.123.1
Internet Control Message Protocol
~~~

## Expedited forwarding and ECT(0) - ToS 0xba ##

~~~
[root@ovs-tunnel-test-1 ~]# ip netns exec private ping 192.168.123.2 -c1 -W1 -Q 0xba
PING 192.168.123.2 (192.168.123.2) 56(84) bytes of data.
64 bytes from 192.168.123.2: icmp_seq=1 ttl=64 time=1.47 ms

--- 192.168.123.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 1.471/1.471/1.471/0.000 ms
[root@ovs-tunnel-test-1 ~]# ^C
[root@ovs-tunnel-test-1 ~]# 
~~~

~~~
[root@ovs-tunnel-test-2 ~]# tshark -nn -i eth0 -O ip port vxlan
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
Frame 1: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
Ethernet II, Src: 52:54:00:e3:af:ab, Dst: 52:54:00:9e:bc:3a
Internet Protocol Version 4, Src: 192.168.122.41, Dst: 192.168.122.174
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 134
    Identification: 0xf90e (63758)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0xcb2d [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.122.41
    Destination: 192.168.122.174
User Datagram Protocol, Src Port: 34788, Dst Port: 4789
Virtual eXtensible Local Area Network
Ethernet II, Src: 16:28:3c:ba:5b:13, Dst: 56:13:a0:be:1f:f4
Internet Protocol Version 4, Src: 192.168.123.1, Dst: 192.168.123.2
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0xba (DSCP: EF PHB, ECN: ECT(0))
        1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (46)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0xf88b (63627)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0xca0e [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.1
    Destination: 192.168.123.2
Internet Control Message Protocol

Frame 2: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
Ethernet II, Src: 52:54:00:9e:bc:3a, Dst: 52:54:00:e3:af:ab
Internet Protocol Version 4, Src: 192.168.122.174, Dst: 192.168.122.41
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 134
    Identification: 0xf7ee (63470)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0xcc4d [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.122.174
    Destination: 192.168.122.41
User Datagram Protocol, Src Port: 45375, Dst Port: 4789
Virtual eXtensible Local Area Network
Ethernet II, Src: 56:13:a0:be:1f:f4, Dst: 16:28:3c:ba:5b:13
Internet Protocol Version 4, Src: 192.168.123.2, Dst: 192.168.123.1
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0xba (DSCP: EF PHB, ECN: ECT(0))
        1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (46)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0x67e2 (26594)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0x9ab8 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.2
    Destination: 192.168.123.1
Internet Control Message Protocol

^C2 packets captured
~~~

## ToS 0xba with tos = inherit 

* `0xba` = `EF` with ECN bits set to `10`

`tos=inherit` will literally only inherit the ToS bits 4,3,2. 
See: [https://en.wikipedia.org/wiki/Type_of_service](https://en.wikipedia.org/wiki/Type_of_service)
~~~
Precedence and ToS

Prior to its deprecation, the Type of Service field was defined as follows from RFC 791:
7 	6 	5 	4 	3 	2 	1 	0
Precedence 	Type of Service 	Unused (0)
~~~

* [http://www.openvswitch.org/support/dist-docs/ovs-vswitchd.conf.db.5.txt](http://www.openvswitch.org/support/dist-docs/ovs-vswitchd.conf.db.5.txt)
~~~
       options : tos: optional string
              Optional. The value of the ToS bits to be set on the encapsulat‐
              ing packet. ToS is interpreted as DSCP and ECN  bits,  ECN  part
              must be zero. It may also be the word inherit, in which case the
              ToS will be copied from the inner packet if it is IPv4  or  IPv6
              (otherwise  it  will be 0). The ECN fields are always inherited.
              Default is 0.
~~~

Run this on both hosts:
~~~
ovs-vsctl set interface vxlan0 options:tos=inherit
~~~

~~~
[root@ovs-tunnel-test-1 ~]# ip netns exec private ping 192.168.123.2 -c1 -W1 -Q 0xba
PING 192.168.123.2 (192.168.123.2) 56(84) bytes of data.
64 bytes from 192.168.123.2: icmp_seq=1 ttl=64 time=2.49 ms

--- 192.168.123.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 2.486/2.486/2.486/0.000 ms
[root@ovs-tunnel-test-1 ~]# 
~~~

~~~
[root@ovs-tunnel-test-2 ~]# tshark -nn -i eth0 -O ip port vxlan
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
Frame 1: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
Ethernet II, Src: 52:54:00:e3:af:ab, Dst: 52:54:00:9e:bc:3a
Internet Protocol Version 4, Src: 192.168.122.41, Dst: 192.168.122.174
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x1a (DSCP: Unknown, ECN: ECT(0))
        0001 10.. = Differentiated Services Codepoint: Unknown (6)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 134
    Identification: 0xd0c2 (53442)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0xf361 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.122.41
    Destination: 192.168.122.174
User Datagram Protocol, Src Port: 34788, Dst Port: 4789
Virtual eXtensible Local Area Network
Ethernet II, Src: 16:28:3c:ba:5b:13, Dst: 56:13:a0:be:1f:f4
Internet Protocol Version 4, Src: 192.168.123.1, Dst: 192.168.123.2
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0xba (DSCP: EF PHB, ECN: ECT(0))
        1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (46)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0xcf83 (53123)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0xf316 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.1
    Destination: 192.168.123.2
Internet Control Message Protocol

Frame 2: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
Ethernet II, Src: 52:54:00:9e:bc:3a, Dst: 52:54:00:e3:af:ab
Internet Protocol Version 4, Src: 192.168.122.174, Dst: 192.168.122.41
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x1a (DSCP: Unknown, ECN: ECT(0))
        0001 10.. = Differentiated Services Codepoint: Unknown (6)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 134
    Identification: 0x3a20 (14880)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0x8a04 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.122.174
    Destination: 192.168.122.41
User Datagram Protocol, Src Port: 45375, Dst Port: 4789
Virtual eXtensible Local Area Network
Ethernet II, Src: 56:13:a0:be:1f:f4, Dst: 16:28:3c:ba:5b:13
Internet Protocol Version 4, Src: 192.168.123.2, Dst: 192.168.123.1
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0xba (DSCP: EF PHB, ECN: ECT(0))
        1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (46)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0x4704 (18180)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0xbb96 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.2
    Destination: 192.168.123.1
Internet Control Message Protocol
~~~

## Force a specific ToS value to the outer header ##


Valid values for the ToS only manipulate the legacy type of service field and leave the legacy IP precedence and ECN fields at 0:
~~~
000 111 00 - 0x1c
000 110 00 - 0x18
000 101 00 - 0x14
000 100 00 - 0x10
000 011 00 - 0xc
000 010 00 - 0x8
000 001 00 - 0x4
000 000 00 - 0x0
~~~

`AF41` is ToS `0x22` - this is not in the aforementioned list and hence all bits will be set to 0.

Run on both hosts:
~~~
ovs-vsctl set interface vxlan0 options:tos=0x22
~~~

Verify - note that this marks down the class to 000000. 0x22 is an invalid value??? Perhaps???
~~~
[root@ovs-tunnel-test-1 ~]# ip netns exec private ping 192.168.123.2 -c1 -W1 -Q 0xba
PING 192.168.123.2 (192.168.123.2) 56(84) bytes of data.
64 bytes from 192.168.123.2: icmp_seq=1 ttl=64 time=1.10 ms

--- 192.168.123.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 1.103/1.103/1.103/0.000 ms
[root@ovs-tunnel-test-1 ~]# 
~~~

~~~
[root@ovs-tunnel-test-2 ~]# tshark -nn -i eth0 -O ip port vxlan
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
Frame 1: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
Ethernet II, Src: 52:54:00:e3:af:ab, Dst: 52:54:00:9e:bc:3a
Internet Protocol Version 4, Src: 192.168.122.41, Dst: 192.168.122.174
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 134
    Identification: 0xa5b1 (42417)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0x1e8b [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.122.41
    Destination: 192.168.122.174
User Datagram Protocol, Src Port: 34788, Dst Port: 4789
Virtual eXtensible Local Area Network
Ethernet II, Src: 16:28:3c:ba:5b:13, Dst: 56:13:a0:be:1f:f4
Internet Protocol Version 4, Src: 192.168.123.1, Dst: 192.168.123.2
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0xba (DSCP: EF PHB, ECN: ECT(0))
        1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (46)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0xe36a (58218)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0xdf2f [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.1
    Destination: 192.168.123.2
Internet Control Message Protocol

Frame 2: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
Ethernet II, Src: 52:54:00:9e:bc:3a, Dst: 52:54:00:e3:af:ab
Internet Protocol Version 4, Src: 192.168.122.174, Dst: 192.168.122.41
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 134
    Identification: 0xb041 (45121)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0x13fb [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.122.174
    Destination: 192.168.122.41
User Datagram Protocol, Src Port: 45375, Dst Port: 4789
Virtual eXtensible Local Area Network
Ethernet II, Src: 56:13:a0:be:1f:f4, Dst: 16:28:3c:ba:5b:13
Internet Protocol Version 4, Src: 192.168.123.2, Dst: 192.168.123.1
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0xba (DSCP: EF PHB, ECN: ECT(0))
        1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (46)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0x75ca (30154)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0x8cd0 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.2
    Destination: 192.168.123.1
Internet Control Message Protocol
~~~

Here is a valid test - on both nodes, set:
~~~
ovs-vsctl set interface vxlan0 options:tos=0xc
~~~

~~~
[root@ovs-tunnel-test-1 ~]# ip netns exec private ping 192.168.123.2 -c1 -W1 -Q 0xba
PING 192.168.123.2 (192.168.123.2) 56(84) bytes of data.
64 bytes from 192.168.123.2: icmp_seq=1 ttl=64 time=2.77 ms

--- 192.168.123.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 2.774/2.774/2.774/0.000 ms
~~~

~~~
[root@ovs-tunnel-test-2 ~]# tshark -nn -i eth0 -O ip port vxlan
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
Frame 1: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
Ethernet II, Src: 52:54:00:e3:af:ab, Dst: 52:54:00:9e:bc:3a
Internet Protocol Version 4, Src: 192.168.122.41, Dst: 192.168.122.174
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x0e (DSCP: Unknown, ECN: ECT(0))
        0000 11.. = Differentiated Services Codepoint: Unknown (3)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 134
    Identification: 0xd7c2 (55234)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0xec6d [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.122.41
    Destination: 192.168.122.174
User Datagram Protocol, Src Port: 34788, Dst Port: 4789
Virtual eXtensible Local Area Network
Ethernet II, Src: 16:28:3c:ba:5b:13, Dst: 56:13:a0:be:1f:f4
Internet Protocol Version 4, Src: 192.168.123.1, Dst: 192.168.123.2
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0xba (DSCP: EF PHB, ECN: ECT(0))
        1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (46)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0xdce0 (56544)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0xe5b9 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.1
    Destination: 192.168.123.2
Internet Control Message Protocol

Frame 2: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
Ethernet II, Src: 52:54:00:9e:bc:3a, Dst: 52:54:00:e3:af:ab
Internet Protocol Version 4, Src: 192.168.122.174, Dst: 192.168.122.41
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x0e (DSCP: Unknown, ECN: ECT(0))
        0000 11.. = Differentiated Services Codepoint: Unknown (3)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 134
    Identification: 0xdd9c (56732)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0xe693 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.122.174
    Destination: 192.168.122.41
User Datagram Protocol, Src Port: 45375, Dst Port: 4789
Virtual eXtensible Local Area Network
Ethernet II, Src: 56:13:a0:be:1f:f4, Dst: 16:28:3c:ba:5b:13
Internet Protocol Version 4, Src: 192.168.123.2, Dst: 192.168.123.1
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0xba (DSCP: EF PHB, ECN: ECT(0))
        1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (46)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0xfab2 (64178)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0x07e8 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.2
    Destination: 192.168.123.1
Internet Control Message Protocol

2 packets captured
~~~

### Further troubleshooting
~~~
    [root@ovs-tunnel-test-1 ~]# ovs-vsctl set interface vxlan0 options:tos=0xb8
    [root@ovs-tunnel-test-1 ~]# systemctl restart openvswitch
    [root@ovs-tunnel-test-1 ~]# ip netns exec private ping 192.168.123.2 -c1 -W1 -Q 0xba
    PING 192.168.123.2 (192.168.123.2) 56(84) bytes of data.
    64 bytes from 192.168.123.2: icmp_seq=1 ttl=64 time=3.27 ms
     
    --- 192.168.123.2 ping statistics ---
    1 packets transmitted, 1 received, 0% packet loss, time 0ms
    rtt min/avg/max/mdev = 3.272/3.272/3.272/0.000 ms
    (failed reverse-i-search)`dcp': ovs-dpctl ^Cmp-flows
    [root@ovs-tunnel-test-1 ~]# ovs-dpctl dump-flows
    recirc_id(0),tunnel(tun_id=0x0,src=192.168.122.174,dst=192.168.122.41,tos=0x18,flags(-df-csum+key)),in_port(2),eth(src=56:13:a0:be:1f:f4,dst=16:28:3c:ba:5b:13),eth_type(0x0806), packets:1, bytes:42, used:5.009s, actions:3
    recirc_id(0),in_port(3),eth(src=16:28:3c:ba:5b:13,dst=56:13:a0:be:1f:f4),eth_type(0x0806), packets:1, bytes:42, used:5.009s, actions:set(tunnel(dst=192.168.122.174,tos=0xb8,ttl=64,tp_dst=4789,flags(df))),2
    [root@ovs-tunnel-test-1 ~]#
 ~~~    
     
 ~~~
    [root@ovs-tunnel-test-2 ~]# tshark -nn -i eth0 -O ip port vxlan | egrep 'Frame|Differentiated Services|Explicit Congestion Notification'
    Running as user "root" and group "root". This could be dangerous.
    Capturing on 'eth0'
    2 Frame 1: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
    6     Differentiated Services Field: 0x1a (DSCP: Unknown, ECN: ECT(0))
            0001 10.. = Differentiated Services Codepoint: Unknown (6)
            .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
        Differentiated Services Field: 0xba (DSCP: EF PHB, ECN: ECT(0))
            1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (46)
            .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Frame 2: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface eth0, id 0
        Differentiated Services Field: 0x1a (DSCP: Unknown, ECN: ECT(0))
            0001 10.. = Differentiated Services Codepoint: Unknown (6)
            .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
        Differentiated Services Field: 0xba (DSCP: EF PHB, ECN: ECT(0))
            1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (46)
            .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Frame 3: 92 bytes on wire (736 bits), 92 bytes captured (736 bits) on interface eth0, id 0
        Differentiated Services Field: 0x18 (DSCP: Unknown, ECN: Not-ECT)
            0001 10.. = Differentiated Services Codepoint: Unknown (6)
            .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Frame 4: 92 bytes on wire (736 bits), 92 bytes captured (736 bits) on interface eth0, id 0
        Differentiated Services Field: 0x18 (DSCP: Unknown, ECN: Not-ECT)
            0001 10.. = Differentiated Services Codepoint: Unknown (6)
            .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Frame 5: 92 bytes on wire (736 bits), 92 bytes captured (736 bits) on interface eth0, id 0
        Differentiated Services Field: 0x18 (DSCP: Unknown, ECN: Not-ECT)
            0001 10.. = Differentiated Services Codepoint: Unknown (6)
            .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Frame 6: 92 bytes on wire (736 bits), 92 bytes captured (736 bits) on interface eth0, id 0
        Differentiated Services Field: 0x18 (DSCP: Unknown, ECN: Not-ECT)
            0001 10.. = Differentiated Services Codepoint: Unknown (6)
            .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    ^C^R
    ^C^C
     
    [root@ovs-tunnel-test-2 ~]# ovs-dpctl dump-flows
    recirc_id(0),tunnel(tun_id=0x0,src=192.168.122.41,dst=192.168.122.174,tos=0x18,flags(-df-csum+key)),in_port(2),eth(src=16:28:3c:ba:5b:13,dst=56:13:a0:be:1f:f4),eth_type(0x0806), packets:1, bytes:42, used:8.951s, actions:3
    recirc_id(0),in_port(3),eth(src=56:13:a0:be:1f:f4,dst=16:28:3c:ba:5b:13),eth_type(0x0806), packets:1, bytes:42, used:8.951s, actions:set(tunnel(dst=192.168.122.41,tos=0xb8,ttl=64,tp_dst=4789,flags(df))),2
    [root@ovs-tunnel-test-2 ~]#
~~~

### Test with RHEL 7.7 ###

The restart is only to reset the dpctl flows. Not necessary to apply the change. In RHEL 7.7, the first 3 bits are set correctly.
~~~
[root@ovs-tunnel-test-1 ~]# ovs-vsctl set interface vxlan0 options:tos=inherit
[root@ovs-tunnel-test-1 ~]# systemctl restart openvswitch
[root@ovs-tunnel-test-1 ~]# ip netns exec private ping 192.168.123.2 -c1 -W1 -Q 0xba
PING 192.168.123.2 (192.168.123.2) 56(84) bytes of data.
64 bytes from 192.168.123.2: icmp_seq=1 ttl=64 time=0.971 ms

--- 192.168.123.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.971/0.971/0.971/0.000 ms
[root@ovs-tunnel-test-1 ~]# ovs-dpctl dump-flows
2020-07-30T13:49:12Z|00001|dpif_netlink|INFO|The kernel module does not support meters.
recirc_id(0),in_port(3),eth(src=fe:61:62:0e:3b:65,dst=56:73:90:10:20:5f),eth_type(0x0800),ipv4(tos=0xba,frag=no), packets:0, bytes:0, used:never, actions:set(tunnel(dst=192.168.122.175,tos=0xba,ttl=64,tp_dst=4789,flags(df))),2
recirc_id(0),tunnel(tun_id=0x0,src=192.168.122.175,dst=192.168.122.42,flags(-df-csum+key)),in_port(2),eth(src=56:73:90:10:20:5f,dst=fe:61:62:0e:3b:65),eth_type(0x0806), packets:0, bytes:0, used:never, actions:3
recirc_id(0),tunnel(tun_id=0x0,src=192.168.122.175,dst=192.168.122.42,tos=0xba,flags(-df-csum+key)),in_port(2),eth(src=56:73:90:10:20:5f,dst=fe:61:62:0e:3b:65),eth_type(0x0800),ipv4(frag=no), packets:0, bytes:0, used:never, actions:3
recirc_id(0),in_port(3),eth(src=fe:61:62:0e:3b:65,dst=56:73:90:10:20:5f),eth_type(0x0806), packets:0, bytes:0, used:never, actions:set(tunnel(dst=192.168.122.175,ttl=64,tp_dst=4789,flags(df))),2
[root@ovs-tunnel-test-1 ~]# 
~~~

~~~
[root@ovs-tunnel-test-2 ~]# ovs-vsctl set interface vxlan0 options:tos=inherit
[root@ovs-tunnel-test-2 ~]# systemctl restart openvswitch
(reverse-i-search)`t': systemctl restart openvswi^Ch
[root@ovs-tunnel-test-2 ~]# tshark -nn -i eth0 -O ip port 4789 | egrep 'Frame|Differentiated Services|Explicit Congestion Notification'
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
4 Frame 1: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface 0
    Differentiated Services Field: 0xba (DSCP 0x2e: Expedited Forwarding; ECN: 0x02: ECT(0) (ECN-Capable Transport))
        1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (0x2e)
        .... ..10 = Explicit Congestion Notification: ECT(0) (ECN-Capable Transport) (0x02)
Frame 2: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface 0
    Differentiated Services Field: 0xba (DSCP 0x2e: Expedited Forwarding; ECN: 0x02: ECT(0) (ECN-Capable Transport))
        1011 10.. = Differentiated Services Codepoint: Expedited Forwarding (0x2e)
        .... ..10 = Explicit Congestion Notification: ECT(0) (ECN-Capable Transport) (0x02)
Frame 3: 92 bytes on wire (736 bits), 92 bytes captured (736 bits) on interface 0
    Differentiated Services Field: 0x00 (DSCP 0x00: Default; ECN: 0x00: Not-ECT (Not ECN-Capable Transport))
        0000 00.. = Differentiated Services Codepoint: Default (0x00)
        .... ..00 = Explicit Congestion Notification: Not-ECT (Not ECN-Capable Transport) (0x00)
Frame 4: 92 bytes on wire (736 bits), 92 bytes captured (736 bits) on interface 0
    Differentiated Services Field: 0x00 (DSCP 0x00: Default; ECN: 0x00: Not-ECT (Not ECN-Capable Transport))
        0000 00.. = Differentiated Services Codepoint: Default (0x00)
^C
[root@ovs-tunnel-test-2 ~]# ovs-dpctl dump-flows
2020-07-30T13:49:15Z|00001|dpif_netlink|INFO|The kernel module does not support meters.
recirc_id(0),in_port(3),eth(src=56:73:90:10:20:5f,dst=fe:61:62:0e:3b:65),eth_type(0x0806), packets:0, bytes:0, used:never, actions:set(tunnel(dst=192.168.122.42,ttl=64,tp_dst=4789,flags(df))),2
recirc_id(0),tunnel(tun_id=0x0,src=192.168.122.42,dst=192.168.122.175,tos=0xba,flags(-df-csum+key)),in_port(2),eth(src=fe:61:62:0e:3b:65,dst=56:73:90:10:20:5f),eth_type(0x0800),ipv4(frag=no), packets:0, bytes:0, used:never, actions:3
recirc_id(0),tunnel(tun_id=0x0,src=192.168.122.42,dst=192.168.122.175,flags(-df-csum+key)),in_port(2),eth(src=fe:61:62:0e:3b:65,dst=56:73:90:10:20:5f),eth_type(0x0806), packets:0, bytes:0, used:never, actions:3
recirc_id(0),in_port(3),eth(src=56:73:90:10:20:5f,dst=fe:61:62:0e:3b:65),eth_type(0x0800),ipv4(tos=0xba,frag=no), packets:0, bytes:0, used:never, actions:set(tunnel(dst=192.168.122.42,tos=0xba,ttl=64,tp_dst=4789,flags(df))),2
[root@ovs-tunnel-test-2 ~]# 
~~~

And explicitly setting this, e.g. to 1111 1100:
~~~
[root@ovs-tunnel-test-1 ~]# systemctl restart openvswitch
[root@ovs-tunnel-test-1 ~]# ovs-vsctl set interface vxlan0 options:tos=0xfc
[root@ovs-tunnel-test-1 ~]# ip netns exec private ping 192.168.123.2 -c1 -W1 -Q 0xba
PING 192.168.123.2 (192.168.123.2) 56(84) bytes of data.
64 bytes from 192.168.123.2: icmp_seq=1 ttl=64 time=2.33 ms

--- 192.168.123.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 2.339/2.339/2.339/0.000 ms
[root@ovs-tunnel-test-1 ~]# ovs-dpctl dump-flows
2020-07-30T13:55:10Z|00001|dpif_netlink|INFO|The kernel module does not support meters.
recirc_id(0),in_port(3),eth(src=fe:61:62:0e:3b:65,dst=56:73:90:10:20:5f),eth_type(0x0800),ipv4(tos=0x2/0x3,frag=no), packets:0, bytes:0, used:never, actions:set(tunnel(dst=192.168.122.175,tos=0xfe,ttl=64,tp_dst=4789,flags(df))),2
recirc_id(0),tunnel(tun_id=0x0,src=192.168.122.175,dst=192.168.122.42,tos=0xfe,flags(-df-csum+key)),in_port(2),eth(src=56:73:90:10:20:5f,dst=fe:61:62:0e:3b:65),eth_type(0x0800),ipv4(frag=no), packets:0, bytes:0, used:never, actions:3
[root@ovs-tunnel-test-1 ~]# 
~~~

~~~
[root@ovs-tunnel-test-2 ~]# systemctl restart openvswitch
[root@ovs-tunnel-test-2 ~]# ovs-vsctl set interface vxlan0 options:tos=0xfc
[root@ovs-tunnel-test-2 ~]# tshark -nn -i eth0 -O ip port 4789 | egrep 'Frame|Differentiated Services|Explicit Congestion Notification'
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
4 Frame 1: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface 0
    Differentiated Services Field: 0xfe (DSCP 0x3f: Unknown DSCP; ECN: 0x02: ECT(0) (ECN-Capable Transport))
        1111 11.. = Differentiated Services Codepoint: Unknown (0x3f)
        .... ..10 = Explicit Congestion Notification: ECT(0) (ECN-Capable Transport) (0x02)
Frame 2: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits) on interface 0
    Differentiated Services Field: 0xfe (DSCP 0x3f: Unknown DSCP; ECN: 0x02: ECT(0) (ECN-Capable Transport))
        1111 11.. = Differentiated Services Codepoint: Unknown (0x3f)
        .... ..10 = Explicit Congestion Notification: ECT(0) (ECN-Capable Transport) (0x02)
Frame 3: 92 bytes on wire (736 bits), 92 bytes captured (736 bits) on interface 0
    Differentiated Services Field: 0xfc (DSCP 0x3f: Unknown DSCP; ECN: 0x00: Not-ECT (Not ECN-Capable Transport))
        1111 11.. = Differentiated Services Codepoint: Unknown (0x3f)
        .... ..00 = Explicit Congestion Notification: Not-ECT (Not ECN-Capable Transport) (0x00)
Frame 4: 92 bytes on wire (736 bits), 92 bytes captured (736 bits) on interface 0
    Differentiated Services Field: 0xfc (DSCP 0x3f: Unknown DSCP; ECN: 0x00: Not-ECT (Not ECN-Capable Transport))
        1111 11.. = Differentiated Services Codepoint: Unknown (0x3f)
^C
[root@ovs-tunnel-test-2 ~]# ovs-dpctl dump-flows
2020-07-30T13:55:18Z|00001|dpif_netlink|INFO|The kernel module does not support meters.
recirc_id(0),in_port(3),eth(src=56:73:90:10:20:5f,dst=fe:61:62:0e:3b:65),eth_type(0x0806), packets:0, bytes:0, used:never, actions:set(tunnel(dst=192.168.122.42,tos=0xfc,ttl=64,tp_dst=4789,flags(df))),2
recirc_id(0),tunnel(tun_id=0x0,src=192.168.122.42,dst=192.168.122.175,tos=0xfc,flags(-df-csum+key)),in_port(2),eth(src=fe:61:62:0e:3b:65,dst=56:73:90:10:20:5f),eth_type(0x0806), packets:0, bytes:0, used:never, actions:3
[root@ovs-tunnel-test-2 ~]# 
~~~

The outer header is correctly set to 1111 1110.

## BUG ##

My tests were affected by: [https://bugzilla.redhat.com/show_bug.cgi?id=1862166](https://bugzilla.redhat.com/show_bug.cgi?id=1862166)

## ECN field ##

Is always inherited:

* [https://github.com/openvswitch/ovs/blob/252c24a61774cac1f593569dcce31e524725676c/ofproto/tunnel.c#L454](https://github.com/openvswitch/ovs/blob/252c24a61774cac1f593569dcce31e524725676c/ofproto/tunnel.c#L454)
~~~
    /* ECN fields are always inherited. */
    if (is_ip_any(flow)) {
        wc->masks.nw_tos |= IP_ECN_MASK;

        if (IP_ECN_is_ce(flow->nw_tos)) {
            flow->tunnel.ip_tos |= IP_ECN_ECT_0;
        } else {
            flow->tunnel.ip_tos |= flow->nw_tos & IP_ECN_MASK;
        }
    }
~~~

## Resources ##

* [https://www.tucny.com/Home/dscp-tos](https://www.tucny.com/Home/dscp-tos)
* [http://docs.openvswitch.org/en/latest/howto/userspace-tunneling/](http://docs.openvswitch.org/en/latest/howto/userspace-tunneling/)
