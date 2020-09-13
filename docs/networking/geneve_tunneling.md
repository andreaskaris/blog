# Geneve tunneling vs VXLAN #

## Packet captures ##

### VXLAN ###
~~~
[root@rhel-test1 ~]# ip link add dev vxlan0 type vxlan remote 192.168.1.12 vni 1234
vxlan: destination port not specified
Will use Linux kernel default (non-standard value)
Use 'dstport 4789' to get the IANA assigned value
Use 'dstport 0' to get default and quiet this message
[root@rhel-test1 ~]# ip link del dev vxlan0
[root@rhel-test1 ~]# ip link add dev vxlan0 type vxlan remote 192.168.1.12 vni 1234 dstport 4789
[root@rhel-test1 ~]# ip a a 192.168.124.1/30 dev vxlan0
[root@rhel-test1 ~]# ip link set dev vxlan0 up
[root@rhel-test1 ~]# ping 192.168.124.2
PING 192.168.124.2 (192.168.124.2) 56(84) bytes of data.
64 bytes from 192.168.124.2: icmp_seq=1 ttl=64 time=3.02 ms
64 bytes from 192.168.124.2: icmp_seq=2 ttl=64 time=1.87 ms
^C
--- 192.168.124.2 ping statistics ---
2 packets transmitted, 2 received, 0% packet loss, time 1001ms
rtt min/avg/max/mdev = 1.879/2.453/3.027/0.574 ms
[root@rhel-test1 ~]# ping 192.168.124.2
PING 192.168.124.2 (192.168.124.2) 56(84) bytes of data.
64 bytes from 192.168.124.2: icmp_seq=1 ttl=64 time=1.15 ms
^C
--- 192.168.124.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 1.158/1.158/1.158/0.000 ms
[root@rhel-test1 ~]# 
~~~

~~~
[root@rhel-test2 ~]# ip link add dev vxlan0 type vxlan remote 192.168.0.12 vni 1234 dstport 4789
[root@rhel-test2 ~]# ip a a 192.168.124.2/30 vxlan0
Error: either "local" is duplicate, or "vxlan0" is a garbage.
[root@rhel-test2 ~]# ip a a 192.168.124.2/30 dev vxlan0
[root@rhel-test2 ~]# ip link set dev vlan0  up
Cannot find device "vlan0"
[root@rhel-test2 ~]# ip link set dev vxlan0  up
[root@rhel-test2 ~]# tcpdump -nne -i vxlan0
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on vxlan0, link-type EN10MB (Ethernet), capture size 65535 bytes
13:25:54.405073 b6:37:34:97:b8:e3 > 56:cf:14:90:3b:e0, ethertype IPv4 (0x0800), length 98: 192.168.124.1 > 192.168.124.2: ICMP echo request, id 2192, seq 1, length 64
13:25:54.405131 56:cf:14:90:3b:e0 > b6:37:34:97:b8:e3, ethertype IPv4 (0x0800), length 98: 192.168.124.2 > 192.168.124.1: ICMP echo reply, id 2192, seq 1, length 64
^C
2 packets captured
2 packets received by filter
0 packets dropped by kernel
[root@rhel-test2 ~]# 
~~~

~~~
[akaris@wks-akaris geneve]$ tshark -r vxlan.pcap -V frame.number==43
Frame 43: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits)
    Encapsulation type: Ethernet (1)
    Arrival Time: Mar  6, 2019 13:25:54.545467000 EST
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1551896754.545467000 seconds
    [Time delta from previous captured frame: 5.489084000 seconds]
    [Time delta from previous displayed frame: 0.000000000 seconds]
    [Time since reference or first frame: 61.528243000 seconds]
    Frame Number: 43
    Frame Length: 148 bytes (1184 bits)
    Capture Length: 148 bytes (1184 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:udp:vxlan:eth:ethertype:ip:icmp:data]
Ethernet II, Src: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7), Dst: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4)
    Destination: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4)
        Address: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7)
        Address: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 192.168.0.12, Dst: 192.168.1.12
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Total Length: 134
    Identification: 0x45eb (17899)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
        ...0 0000 0000 0000 = Fragment offset: 0
    Time to live: 63
    Protocol: UDP (17)
    Header checksum: 0xb313 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.0.12
    Destination: 192.168.1.12
User Datagram Protocol, Src Port: 34990, Dst Port: 4789
    Source Port: 34990
    Destination Port: 4789
    Length: 114
    [Checksum: [missing]]
    [Checksum Status: Not present]
    [Stream index: 11]
Virtual eXtensible Local Area Network
    Flags: 0x0800, VXLAN Network ID (VNI)
        0... .... .... .... = GBP Extension: Not defined
        .... .... .0.. .... = Don't Learn: False
        .... 1... .... .... = VXLAN Network ID (VNI): True
        .... .... .... 0... = Policy Applied: False
        .000 .000 0.00 .000 = Reserved(R): 0x0000
    Group Policy ID: 0
    VXLAN Network Identifier (VNI): 1234
    Reserved: 0
Ethernet II, Src: b6:37:34:97:b8:e3 (b6:37:34:97:b8:e3), Dst: 56:cf:14:90:3b:e0 (56:cf:14:90:3b:e0)
    Destination: 56:cf:14:90:3b:e0 (56:cf:14:90:3b:e0)
        Address: 56:cf:14:90:3b:e0 (56:cf:14:90:3b:e0)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: b6:37:34:97:b8:e3 (b6:37:34:97:b8:e3)
        Address: b6:37:34:97:b8:e3 (b6:37:34:97:b8:e3)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 192.168.124.1, Dst: 192.168.124.2
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Total Length: 84
    Identification: 0x3470 (13424)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
        ...0 0000 0000 0000 = Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0x8ce4 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.124.1
    Destination: 192.168.124.2
Internet Control Message Protocol
    Type: 8 (Echo (ping) request)
    Code: 0
    Checksum: 0x7c23 [correct]
    [Checksum Status: Good]
    Identifier (BE): 2192 (0x0890)
    Identifier (LE): 36872 (0x9008)
    Sequence number (BE): 1 (0x0001)
    Sequence number (LE): 256 (0x0100)
    Timestamp from icmp data: Mar  6, 2019 13:25:53.000000000 EST
    [Timestamp from icmp data (relative): 1.545467000 seconds]
    Data (48 bytes)

0000  7d 0b 06 00 00 00 00 00 10 11 12 13 14 15 16 17   }...............
0010  18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27   ........ !"#$%&'
0020  28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35 36 37   ()*+,-./01234567
        Data: 7d0b060000000000101112131415161718191a1b1c1d1e1f...
        [Length: 48]

[akaris@wks-akaris geneve]$ tshark -r vxlan.pcap -V frame.number==44
Frame 44: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits)
    Encapsulation type: Ethernet (1)
    Arrival Time: Mar  6, 2019 13:25:54.545788000 EST
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1551896754.545788000 seconds
    [Time delta from previous captured frame: 0.000321000 seconds]
    [Time delta from previous displayed frame: 0.000000000 seconds]
    [Time since reference or first frame: 61.528564000 seconds]
    Frame Number: 44
    Frame Length: 148 bytes (1184 bits)
    Capture Length: 148 bytes (1184 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:udp:vxlan:eth:ethertype:ip:icmp:data]
Ethernet II, Src: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4), Dst: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7)
    Destination: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7)
        Address: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4)
        Address: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 192.168.1.12, Dst: 192.168.0.12
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Total Length: 134
    Identification: 0xe768 (59240)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
        ...0 0000 0000 0000 = Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0x1096 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.1.12
    Destination: 192.168.0.12
User Datagram Protocol, Src Port: 38748, Dst Port: 4789
    Source Port: 38748
    Destination Port: 4789
    Length: 114
    [Checksum: [missing]]
    [Checksum Status: Not present]
    [Stream index: 12]
Virtual eXtensible Local Area Network
    Flags: 0x0800, VXLAN Network ID (VNI)
        0... .... .... .... = GBP Extension: Not defined
        .... .... .0.. .... = Don't Learn: False
        .... 1... .... .... = VXLAN Network ID (VNI): True
        .... .... .... 0... = Policy Applied: False
        .000 .000 0.00 .000 = Reserved(R): 0x0000
    Group Policy ID: 0
    VXLAN Network Identifier (VNI): 1234
    Reserved: 0
Ethernet II, Src: 56:cf:14:90:3b:e0 (56:cf:14:90:3b:e0), Dst: b6:37:34:97:b8:e3 (b6:37:34:97:b8:e3)
    Destination: b6:37:34:97:b8:e3 (b6:37:34:97:b8:e3)
        Address: b6:37:34:97:b8:e3 (b6:37:34:97:b8:e3)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: 56:cf:14:90:3b:e0 (56:cf:14:90:3b:e0)
        Address: 56:cf:14:90:3b:e0 (56:cf:14:90:3b:e0)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 192.168.124.2, Dst: 192.168.124.1
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Total Length: 84
    Identification: 0x7bc6 (31686)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
        ...0 0000 0000 0000 = Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0x858e [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.124.2
    Destination: 192.168.124.1
Internet Control Message Protocol
    Type: 0 (Echo (ping) reply)
    Code: 0
    Checksum: 0x8423 [correct]
    [Checksum Status: Good]
    Identifier (BE): 2192 (0x0890)
    Identifier (LE): 36872 (0x9008)
    Sequence number (BE): 1 (0x0001)
    Sequence number (LE): 256 (0x0100)
    [Request frame: 43]
    [Response time: 0.321 ms]
    Timestamp from icmp data: Mar  6, 2019 13:25:53.000000000 EST
    [Timestamp from icmp data (relative): 1.545788000 seconds]
    Data (48 bytes)

0000  7d 0b 06 00 00 00 00 00 10 11 12 13 14 15 16 17   }...............
0010  18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27   ........ !"#$%&'
0020  28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35 36 37   ()*+,-./01234567
        Data: 7d0b060000000000101112131415161718191a1b1c1d1e1f...
        [Length: 48]
~~~

### Geneve ###

[https://www.ietf.org/id/draft-ietf-nvo3-geneve-10.txt](https://www.ietf.org/id/draft-ietf-nvo3-geneve-10.txt)
~~~
3.3.  UDP Header

   The use of an encapsulating UDP [RFC0768] header follows the
   connectionless semantics of Ethernet and IP in addition to providing
   entropy to routers performing ECMP.  The header fields are therefore
   interpreted as follows:

   Source port:  A source port selected by the originating tunnel
      endpoint.  This source port SHOULD be the same for all packets
      belonging to a single encapsulated flow to prevent reordering due
      to the use of different paths.  To encourage an even distribution
      of flows across multiple links, the source port SHOULD be
      calculated using a hash of the encapsulated packet headers using,
      for example, a traditional 5-tuple.  Since the port represents a
      flow identifier rather than a true UDP connection, the entire
      16-bit range MAY be used to maximize entropy.

   Dest port:  IANA has assigned port 6081 as the fixed well-known
      destination port for Geneve.  Although the well-known value should
      be used by default, it is RECOMMENDED that implementations make
      this configurable.  The chosen port is used for identification of
      Geneve packets and MUST NOT be reversed for different ends of a
      connection as is done with TCP.
~~~

~~~
[cloud-user@rhel-test1 ~]$ sudo -i
[root@rhel-test1 ~]# ip link add dev gnv0 type geneve remote 192.168.22.1 vni 1234
[root@rhel-test1 ~]# ip link ls dev gnv0
3: gnv0: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT qlen 1000
    link/ether ca:02:1d:c7:57:52 brd ff:ff:ff:ff:ff:ff
[root@rhel-test1 ~]# ip link del dev gnv0
[root@rhel-test1 ~]# ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN qlen 1
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1446 qdisc pfifo_fast state UP qlen 1000
    link/ether fa:16:3e:4f:be:dc brd ff:ff:ff:ff:ff:ff
    inet 192.168.0.12/24 brd 192.168.0.255 scope global dynamic eth0
       valid_lft 85923sec preferred_lft 85923sec
    inet6 2000:192:168:1:f816:3eff:fe4f:bedc/64 scope global noprefixroute dynamic 
       valid_lft 86369sec preferred_lft 14369sec
    inet6 fe80::f816:3eff:fe4f:bedc/64 scope link 
       valid_lft forever preferred_lft forever
[root@rhel-test1 ~]# ping 192.168.1.12
PING 192.168.1.12 (192.168.1.12) 56(84) bytes of data.
64 bytes from 192.168.1.12: icmp_seq=1 ttl=63 time=1.64 ms
^C
--- 192.168.1.12 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 1.648/1.648/1.648/0.000 ms
[root@rhel-test1 ~]# ip link add dev gnv0 type geneve remote 192.168.1.12 vni 1234
[root@rhel-test1 ~]# ip link set dev gnv0 up
[root@rhel-test1 ~]# ip a a dev gnv0 192.168.123.1/30 
[root@rhel-test1 ~]# ping 192.168.123.2
PING 192.168.123.2 (192.168.123.2) 56(84) bytes of data.
64 bytes from 192.168.123.2: icmp_seq=1 ttl=64 time=3.30 ms
64 bytes from 192.168.123.2: icmp_seq=2 ttl=64 time=1.62 ms
64 bytes from 192.168.123.2: icmp_seq=3 ttl=64 time=1.61 ms
^C
--- 192.168.123.2 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2004ms
rtt min/avg/max/mdev = 1.614/2.180/3.308/0.798 ms
[root@rhel-test1 ~]# 
~~~

~~~
sudo -i[cloud-user@rhel-test2 ~]$ sudo -i
[root@rhel-test2 ~]# ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN qlen 1
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1446 qdisc pfifo_fast state UP qlen 1000
    link/ether fa:16:3e:bf:f2:d4 brd ff:ff:ff:ff:ff:ff
    inet 192.168.1.12/24 brd 192.168.1.255 scope global dynamic eth0
       valid_lft 86302sec preferred_lft 86302sec
    inet6 fe80::f816:3eff:febf:f2d4/64 scope link 
       valid_lft forever preferred_lft forever
[root@rhel-test2 ~]# ip link add dev gnv0 type geneve remote 192.168.0.12 vni 1234
[root@rhel-test2 ~]# ip link set dev gnv0 up
[root@rhel-test2 ~]# ip a a dev gnv0 192.168.123.2/30
[root@rhel-test2 ~]# 
~~~

~~~
[root@overcloud-compute-0 ~]# ip link ls | grep tap
22: tapf1d88f44-52: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1446 qdisc pfifo_fast master qbrf1d88f44-52 state UNKNOWN mode DEFAULT group default qlen 1000
[root@overcloud-compute-0 ~]# tcpdump -nne -itapf1d88f44-52 not port 22
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on tapf1d88f44-52, link-type EN10MB (Ethernet), capture size 262144 bytes
18:14:08.648156 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype ARP (0x0806), length 42: Request who-has 192.168.0.12 tell 192.168.0.1, length 28
18:14:08.648706 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype ARP (0x0806), length 42: Reply 192.168.0.12 is-at fa:16:3e:4f:be:dc, length 28
18:14:24.204432 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 140: 192.168.0.12.36852 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > 33:33:00:00:00:16, ethertype IPv6 (0x86dd), length 90: :: > ff02::16: HBH ICMP6, multicast listener report v2, 1 group record(s), length 28
18:14:24.205955 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 168: 192.168.1.12 > 192.168.0.12: ICMP 192.168.1.12 udp port 6081 unreachable, length 134
18:14:24.288637 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 140: 192.168.0.12.36852 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > 33:33:00:00:00:16, ethertype IPv6 (0x86dd), length 90: :: > ff02::16: HBH ICMP6, multicast listener report v2, 1 group record(s), length 28
18:14:24.289879 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 168: 192.168.1.12 > 192.168.0.12: ICMP 192.168.1.12 udp port 6081 unreachable, length 134
18:14:24.481721 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 128: 192.168.0.12.18960 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > 33:33:ff:75:3b:53, ethertype IPv6 (0x86dd), length 78: :: > ff02::1:ff75:3b53: ICMP6, neighbor solicitation, who has fe80::f85f:19ff:fe75:3b53, length 24
18:14:24.483052 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 156: 192.168.1.12 > 192.168.0.12: ICMP 192.168.1.12 udp port 6081 unreachable, length 122
18:14:25.485623 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 140: 192.168.0.12.15830 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > 33:33:00:00:00:16, ethertype IPv6 (0x86dd), length 90: fe80::f85f:19ff:fe75:3b53 > ff02::16: HBH ICMP6, multicast listener report v2, 1 group record(s), length 28
18:14:25.485733 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 120: 192.168.0.12.13198 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > 33:33:00:00:00:02, ethertype IPv6 (0x86dd), length 70: fe80::f85f:19ff:fe75:3b53 > ff02::2: ICMP6, router solicitation, length 16
18:14:25.487070 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 168: 192.168.1.12 > 192.168.0.12: ICMP 192.168.1.12 udp port 6081 unreachable, length 134
18:14:25.487083 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 148: 192.168.1.12 > 192.168.0.12: ICMP 192.168.1.12 udp port 6081 unreachable, length 114
18:14:26.400718 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 140: 192.168.0.12.15830 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > 33:33:00:00:00:16, ethertype IPv6 (0x86dd), length 90: fe80::f85f:19ff:fe75:3b53 > ff02::16: HBH ICMP6, multicast listener report v2, 1 group record(s), length 28
18:14:26.402262 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 168: 192.168.1.12 > 192.168.0.12: ICMP 192.168.1.12 udp port 6081 unreachable, length 134
18:14:29.488599 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 120: 192.168.0.12.13198 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > 33:33:00:00:00:02, ethertype IPv6 (0x86dd), length 70: fe80::f85f:19ff:fe75:3b53 > ff02::2: ICMP6, router solicitation, length 16
18:14:29.489939 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 148: 192.168.1.12 > 192.168.0.12: ICMP 192.168.1.12 udp port 6081 unreachable, length 114
18:14:33.496721 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 120: 192.168.0.12.13198 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > 33:33:00:00:00:02, ethertype IPv6 (0x86dd), length 70: fe80::f85f:19ff:fe75:3b53 > ff02::2: ICMP6, router solicitation, length 16
18:14:33.498447 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 148: 192.168.1.12 > 192.168.0.12: ICMP 192.168.1.12 udp port 6081 unreachable, length 114
18:14:40.076232 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 140: 192.168.1.12.278 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > 33:33:00:00:00:16, ethertype IPv6 (0x86dd), length 90: :: > ff02::16: HBH ICMP6, multicast listener report v2, 1 group record(s), length 28
18:14:40.217265 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 140: 192.168.1.12.278 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > 33:33:00:00:00:16, ethertype IPv6 (0x86dd), length 90: :: > ff02::16: HBH ICMP6, multicast listener report v2, 1 group record(s), length 28
18:14:40.625369 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 128: 192.168.1.12.47096 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > 33:33:ff:23:5a:f1, ethertype IPv6 (0x86dd), length 78: :: > ff02::1:ff23:5af1: ICMP6, neighbor solicitation, who has fe80::6c51:d3ff:fe23:5af1, length 24
18:14:41.627556 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 140: 192.168.1.12.55971 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > 33:33:00:00:00:16, ethertype IPv6 (0x86dd), length 90: fe80::6c51:d3ff:fe23:5af1 > ff02::16: HBH ICMP6, multicast listener report v2, 1 group record(s), length 28
18:14:41.627589 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 120: 192.168.1.12.16772 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > 33:33:00:00:00:02, ethertype IPv6 (0x86dd), length 70: fe80::6c51:d3ff:fe23:5af1 > ff02::2: ICMP6, router solicitation, length 16
18:14:42.169268 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 140: 192.168.1.12.55971 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > 33:33:00:00:00:16, ethertype IPv6 (0x86dd), length 90: fe80::6c51:d3ff:fe23:5af1 > ff02::16: HBH ICMP6, multicast listener report v2, 1 group record(s), length 28
18:14:45.081086 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype ARP (0x0806), length 42: Request who-has 192.168.0.12 tell 192.168.0.1, length 28
18:14:45.081657 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype ARP (0x0806), length 42: Reply 192.168.0.12 is-at fa:16:3e:4f:be:dc, length 28
18:14:45.631348 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 120: 192.168.1.12.16772 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > 33:33:00:00:00:02, ethertype IPv6 (0x86dd), length 70: fe80::6c51:d3ff:fe23:5af1 > ff02::2: ICMP6, router solicitation, length 16
18:14:49.639089 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 120: 192.168.1.12.16772 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > 33:33:00:00:00:02, ethertype IPv6 (0x86dd), length 70: fe80::6c51:d3ff:fe23:5af1 > ff02::2: ICMP6, router solicitation, length 16
18:14:52.450778 fa:16:3e:37:aa:90 > 33:33:00:00:00:01, ethertype IPv6 (0x86dd), length 118: fe80::f816:3eff:fe37:aa90 > ff02::1: ICMP6, router advertisement, length 64
18:15:20.233535 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype ARP (0x0806), length 42: Request who-has 192.168.0.12 tell 192.168.0.1, length 28
18:15:20.234077 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype ARP (0x0806), length 42: Reply 192.168.0.12 is-at fa:16:3e:4f:be:dc, length 28
18:15:28.043937 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 92: 192.168.0.12.29278 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > ff:ff:ff:ff:ff:ff, ethertype ARP (0x0806), length 42: Request who-has 192.168.123.2 tell 192.168.123.1, length 28
18:15:28.045199 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 92: 192.168.1.12.28059 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > fa:5f:19:75:3b:53, ethertype ARP (0x0806), length 42: Reply 192.168.123.2 is-at 6e:51:d3:23:5a:f1, length 28
18:15:28.045900 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 148: 192.168.0.12.62602 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > 6e:51:d3:23:5a:f1, ethertype IPv4 (0x0800), length 98: 192.168.123.1 > 192.168.123.2: ICMP echo request, id 2135, seq 1, length 64
18:15:28.046928 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 148: 192.168.1.12.59192 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > fa:5f:19:75:3b:53, ethertype IPv4 (0x0800), length 98: 192.168.123.2 > 192.168.123.1: ICMP echo reply, id 2135, seq 1, length 64
18:15:29.045996 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 148: 192.168.0.12.62602 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > 6e:51:d3:23:5a:f1, ethertype IPv4 (0x0800), length 98: 192.168.123.1 > 192.168.123.2: ICMP echo request, id 2135, seq 2, length 64
18:15:29.047241 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 148: 192.168.1.12.59192 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > fa:5f:19:75:3b:53, ethertype IPv4 (0x0800), length 98: 192.168.123.2 > 192.168.123.1: ICMP echo reply, id 2135, seq 2, length 64
18:15:30.048222 fa:16:3e:4f:be:dc > fa:16:3e:cd:6b:54, ethertype IPv4 (0x0800), length 148: 192.168.0.12.62602 > 192.168.1.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): fa:5f:19:75:3b:53 > 6e:51:d3:23:5a:f1, ethertype IPv4 (0x0800), length 98: 192.168.123.1 > 192.168.123.2: ICMP echo request, id 2135, seq 3, length 64
18:15:30.049395 fa:16:3e:cd:6b:54 > fa:16:3e:4f:be:dc, ethertype IPv4 (0x0800), length 148: 192.168.1.12.59192 > 192.168.0.12.6081: Geneve, Flags [none], vni 0x4d2, proto TEB (0x6558): 6e:51:d3:23:5a:f1 > fa:5f:19:75:3b:53, ethertype IPv4 (0x0800), length 98: 192.168.123.2 > 192.168.123.1: ICMP echo reply, id 2135, seq 3, length 64
^C
39 packets captured
39 packets received by filter
0 packets dropped by kernel
[root@overcloud-compute-0 ~]# 
~~~

~~~
[akaris@wks-akaris geneve]$ tshark -r geneve.pcap -V frame.number==102
Frame 102: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits)
    Encapsulation type: Ethernet (1)
    Arrival Time: Mar  6, 2019 13:15:28.046414000 EST
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1551896128.046414000 seconds
    [Time delta from previous captured frame: 0.001384000 seconds]
    [Time delta from previous displayed frame: 0.000000000 seconds]
    [Time since reference or first frame: 67.862074000 seconds]
    Frame Number: 102
    Frame Length: 148 bytes (1184 bits)
    Capture Length: 148 bytes (1184 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:udp:geneve:eth:ethertype:ip:icmp:data]
Ethernet II, Src: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7), Dst: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4)
    Destination: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4)
        Address: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7)
        Address: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 192.168.0.12, Dst: 192.168.1.12
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Total Length: 134
    Identification: 0xa840 (43072)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
        ...0 0000 0000 0000 = Fragment offset: 0
    Time to live: 63
    Protocol: UDP (17)
    Header checksum: 0x50be [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.0.12
    Destination: 192.168.1.12
User Datagram Protocol, Src Port: 62602, Dst Port: 6081
    Source Port: 62602
    Destination Port: 6081
    Length: 114
    Checksum: 0x4569 [unverified]
    [Checksum Status: Unverified]
    [Stream index: 18]
Generic Network Virtualization Encapsulation, VNI: 0x0004d2
    Version: 0
    Length: 0 bytes
    Flags: 0x00
        0... .... = Operations, Administration and Management Frame: False
        .0.. .... = Critical Options Present: False
        ..00 0000 = Reserved: False
    Protocol Type: Transparent Ethernet bridging (0x6558)
    Virtual Network Identifier (VNI): 0x0004d2
Ethernet II, Src: fa:5f:19:75:3b:53 (fa:5f:19:75:3b:53), Dst: 6e:51:d3:23:5a:f1 (6e:51:d3:23:5a:f1)
    Destination: 6e:51:d3:23:5a:f1 (6e:51:d3:23:5a:f1)
        Address: 6e:51:d3:23:5a:f1 (6e:51:d3:23:5a:f1)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: fa:5f:19:75:3b:53 (fa:5f:19:75:3b:53)
        Address: fa:5f:19:75:3b:53 (fa:5f:19:75:3b:53)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 192.168.123.1, Dst: 192.168.123.2
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Total Length: 84
    Identification: 0x0cba (3258)
    Flags: 0x4000, Don't fragment
        0... .... .... .... = Reserved bit: Not set
        .1.. .... .... .... = Don't fragment: Set
        ..0. .... .... .... = More fragments: Not set
        ...0 0000 0000 0000 = Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0xb69a [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.1
    Destination: 192.168.123.2
Internet Control Message Protocol
    Type: 8 (Echo (ping) request)
    Code: 0
    Checksum: 0xa3c1 [correct]
    [Checksum Status: Good]
    Identifier (BE): 2135 (0x0857)
    Identifier (LE): 22280 (0x5708)
    Sequence number (BE): 1 (0x0001)
    Sequence number (LE): 256 (0x0100)
    Timestamp from icmp data: Mar  6, 2019 13:15:26.000000000 EST
    [Timestamp from icmp data (relative): 2.046414000 seconds]
    Data (48 bytes)

0000  c1 a8 0d 00 00 00 00 00 10 11 12 13 14 15 16 17   ................
0010  18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27   ........ !"#$%&'
0020  28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35 36 37   ()*+,-./01234567
        Data: c1a80d0000000000101112131415161718191a1b1c1d1e1f...
        [Length: 48]

[akaris@wks-akaris geneve]$ 
[akaris@wks-akaris geneve]$ tshark -r geneve.pcap -V frame.number==103
Frame 103: 148 bytes on wire (1184 bits), 148 bytes captured (1184 bits)
    Encapsulation type: Ethernet (1)
    Arrival Time: Mar  6, 2019 13:15:28.046802000 EST
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1551896128.046802000 seconds
    [Time delta from previous captured frame: 0.000388000 seconds]
    [Time delta from previous displayed frame: 0.000000000 seconds]
    [Time since reference or first frame: 67.862462000 seconds]
    Frame Number: 103
    Frame Length: 148 bytes (1184 bits)
    Capture Length: 148 bytes (1184 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:udp:geneve:eth:ethertype:ip:icmp:data]
Ethernet II, Src: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4), Dst: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7)
    Destination: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7)
        Address: fa:16:3e:40:02:d7 (fa:16:3e:40:02:d7)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4)
        Address: fa:16:3e:bf:f2:d4 (fa:16:3e:bf:f2:d4)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 192.168.1.12, Dst: 192.168.0.12
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Total Length: 134
    Identification: 0x5bdb (23515)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
        ...0 0000 0000 0000 = Fragment offset: 0
    Time to live: 64
    Protocol: UDP (17)
    Header checksum: 0x9c23 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.1.12
    Destination: 192.168.0.12
User Datagram Protocol, Src Port: 59192, Dst Port: 6081
    Source Port: 59192
    Destination Port: 6081
    Length: 114
    Checksum: 0x52bb [unverified]
    [Checksum Status: Unverified]
    [Stream index: 19]
Generic Network Virtualization Encapsulation, VNI: 0x0004d2
    Version: 0
    Length: 0 bytes
    Flags: 0x00
        0... .... = Operations, Administration and Management Frame: False
        .0.. .... = Critical Options Present: False
        ..00 0000 = Reserved: False
    Protocol Type: Transparent Ethernet bridging (0x6558)
    Virtual Network Identifier (VNI): 0x0004d2
Ethernet II, Src: 6e:51:d3:23:5a:f1 (6e:51:d3:23:5a:f1), Dst: fa:5f:19:75:3b:53 (fa:5f:19:75:3b:53)
    Destination: fa:5f:19:75:3b:53 (fa:5f:19:75:3b:53)
        Address: fa:5f:19:75:3b:53 (fa:5f:19:75:3b:53)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: 6e:51:d3:23:5a:f1 (6e:51:d3:23:5a:f1)
        Address: 6e:51:d3:23:5a:f1 (6e:51:d3:23:5a:f1)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 192.168.123.2, Dst: 192.168.123.1
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Total Length: 84
    Identification: 0x7754 (30548)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
        ...0 0000 0000 0000 = Fragment offset: 0
    Time to live: 64
    Protocol: ICMP (1)
    Header checksum: 0x8c00 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.123.2
    Destination: 192.168.123.1
Internet Control Message Protocol
    Type: 0 (Echo (ping) reply)
    Code: 0
    Checksum: 0xabc1 [correct]
    [Checksum Status: Good]
    Identifier (BE): 2135 (0x0857)
    Identifier (LE): 22280 (0x5708)
    Sequence number (BE): 1 (0x0001)
    Sequence number (LE): 256 (0x0100)
    [Request frame: 102]
    [Response time: 0.388 ms]
    Timestamp from icmp data: Mar  6, 2019 13:15:26.000000000 EST
    [Timestamp from icmp data (relative): 2.046802000 seconds]
    Data (48 bytes)

0000  c1 a8 0d 00 00 00 00 00 10 11 12 13 14 15 16 17   ................
0010  18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27   ........ !"#$%&'
0020  28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35 36 37   ()*+,-./01234567
        Data: c1a80d0000000000101112131415161718191a1b1c1d1e1f...
        [Length: 48]

~~~
