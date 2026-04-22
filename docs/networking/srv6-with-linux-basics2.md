## SRv6 Source Routing with Linux

### A quick overview of SRv6 forwarding

For a great explanation of how SRv6 forwarding works, have a look at the [Segment Routing v6](https://www.segment-routing.net/images/201901-SRv6.pdf) presentation.
I cross-referenced it regularly while working on this blog post. Another great reference is [SRv6 in the Linux kernel](https://www.segment-routing.net/images/20250311-srv6-usid-linux-netdev-0x19.pdf).

In short, the sender of a packet that wishes to use SRv6 Source Routing will have a special route requesting SRv6 encapsulation via `encap seg6`.

In our lab, we configure the following routes on `host1` and `host4`, respectively.

```
ip route add fd02:0:4::1 encap seg6 mode inline segs fd02:0:2::10,fd02:0:6::10,fd02:0:3::10,fd02:0:5::10 dev tohost2
```

```
ip route add fd02:0:1::1 encap seg6 mode inline segs fd02:0:3::10,fd02:0:6::10 dev tohost3
```

These routes set the `mode` to `inline`, meaning the payload directly follows the Segment Routing Header (SRH). In other scenarios, such as tunneling, the `mode` can be set to `encap` or `encap.red` and the SRH will be followed by an inner IP or IPv6 header.

The list of segments, e.g., `segs fd02:0:2::10,fd02:0:6::10,fd02:0:3::10,fd02:0:5::10`, will appear in the Segment Routing Header (ordered from last to first).

The SRH holds a pointer into the list of segments (called the `Segments Left` field) marking the current destination. At each hop, the `Segments Left` field is decreased by one, and the address pointed to
by `Segments Left` is copied into the IPv6 Destination address.  The entire SRH with the list of all segments will be maintained until the SRH is popped off, in our case - due to `flavors psp` - by the penultimate hop (the node just before the destination). A node identifies itself as the penultimate hop when its `Segments Left` is `1`.

Not all nodes in a network have to be SRv6 enabled. These transit nodes forward the IPv6 packet according to the IPv6 destination address and ignore the SRH.

### Lab setup

We use [containerlab](https://containerlab.dev/) to set up the following 6 node topology.

![Lab setup](/blog/src/srv6-source-routing.png)

Create a new directory, and add file `source-routing.clab.yml` with the following content.

```
name: source-routing

topology:
  groups:
    fedora:
      kind: linux
      image: nicolaka/netshoot
      exec:
      - ip route delete default
      - ip -6 route delete default
      - sysctl -w net.ipv4.ip_forward=1
      - sysctl -w net.ipv6.conf.all.forwarding=1
      - sysctl -w net.ipv6.seg6_flowlabel=1
      - sysctl -w net.ipv6.conf.all.seg6_enabled=1

  nodes:
    host1:
      group: fedora
      exec:
      - ip addr add fd02:0:1::1/128 dev lo
      - ip addr add 2001:db8:1:2::1/64 dev tohost2
      - ip addr add 2001:db8:1:6::1/64 dev tohost6
      - ip route add fd02:0:2::/48 via 2001:db8:1:2::2
      - ip route add fd02:0:3::/48 via 2001:db8:1:2::2
      - ip route add fd02:0:4::/48 via 2001:db8:1:2::2
      - ip route add fd02:0:5::/48 via 2001:db8:1:6::6
      - ip route add fd02:0:6::/48 via 2001:db8:1:6::6
      - ip link add dummy0 type dummy
      - ip link set dev dummy0 up
      - ip route add fd02:0:1::10/128 encap seg6local action End flavors psp count dev dummy0
      - ip route add fd02:0:4::1 encap seg6 mode inline segs fd02:0:2::10,fd02:0:6::10,fd02:0:3::10,fd02:0:5::10 dev tohost2
      # ping -c1 -W1 -I fd02:0:1::1 fd02:0:4::1
    host2:
      group: fedora
      exec:
      - ip addr add fd02:0:2::1/128 dev lo
      - ip addr add 2001:db8:1:2::2/64 dev tohost1
      - ip addr add 2001:db8:2:3::2/64 dev tohost3
      - ip addr add 2001:db8:2:5::2/64 dev tohost5
      - ip addr add 2001:db8:2:6::2/64 dev tohost6
      - ip route add fd02:0:1::/48 via 2001:db8:1:2::1
      - ip route add fd02:0:3::/48 via 2001:db8:2:3::3
      - ip route add fd02:0:4::/48 via 2001:db8:2:3::3
      - ip route add fd02:0:5::/48 via 2001:db8:2:5::5
      - ip route add fd02:0:6::/48 via 2001:db8:2:6::6
      - ip link add dummy0 type dummy
      - ip link set dev dummy0 up
      - ip route add fd02:0:2::10/128 encap seg6local action End flavors psp count dev dummy0
    host3:
      group: fedora
      exec:
      - ip addr add fd02:0:3::1/128 dev lo
      - ip addr add 2001:db8:2:3::3/64 dev tohost2
      - ip addr add 2001:db8:3:4::3/64 dev tohost4
      - ip addr add 2001:db8:3:5::3/64 dev tohost5
      - ip addr add 2001:db8:3:6::3/64 dev tohost6
      - ip route add fd02:0:1::/48 via 2001:db8:2:3::2
      - ip route add fd02:0:2::/48 via 2001:db8:2:3::2
      - ip route add fd02:0:4::/48 via 2001:db8:3:4::4
      - ip route add fd02:0:5::/48 via 2001:db8:3:5::5
      - ip route add fd02:0:6::/48 via 2001:db8:3:6::6
      - ip link add dummy0 type dummy
      - ip link set dev dummy0 up
      - ip route add fd02:0:3::10/128 encap seg6local action End flavors psp count dev dummy0
    host4:
      group: fedora
      exec:
      - ip addr add fd02:0:4::1/128 dev lo
      - ip addr add 2001:db8:3:4::4/64 dev tohost3
      - ip addr add 2001:db8:4:5::4/64 dev tohost5
      - ip route add fd02:0:1::/48 via 2001:db8:3:4::3
      - ip route add fd02:0:2::/48 via 2001:db8:3:4::3
      - ip route add fd02:0:3::/48 via 2001:db8:3:4::3
      - ip route add fd02:0:5::/48 via 2001:db8:4:5::5
      - ip route add fd02:0:6::/48 via 2001:db8:4:5::5
      - ip link add dummy0 type dummy
      - ip link set dev dummy0 up
      - ip route add fd02:0:4::10/128 encap seg6local action End flavors psp count dev dummy0
      - ip route add fd02:0:1::1 encap seg6 mode inline segs fd02:0:3::10,fd02:0:6::10 dev tohost3
    host5:
      group: fedora
      exec:
      - ip addr add fd02:0:5::1/128 dev lo
      - ip addr add 2001:db8:2:5::5/64 dev tohost2
      - ip addr add 2001:db8:3:5::5/64 dev tohost3
      - ip addr add 2001:db8:4:5::5/64 dev tohost4
      - ip addr add 2001:db8:5:6::5/64 dev tohost6
      - ip route add fd02:0:1::/48 via 2001:db8:5:6::6
      - ip route add fd02:0:2::/48 via 2001:db8:2:5::2
      - ip route add fd02:0:3::/48 via 2001:db8:3:5::3
      - ip route add fd02:0:4::/48 via 2001:db8:4:5::4
      - ip route add fd02:0:6::/48 via 2001:db8:5:6::6
      - ip link add dummy0 type dummy
      - ip link set dev dummy0 up
      - ip route add fd02:0:5::10/128 encap seg6local action End flavors psp count dev dummy0
    host6:
      group: fedora
      exec:
      - ip addr add fd02:0:6::1/128 dev lo
      - ip addr add 2001:db8:1:6::6/64 dev tohost1
      - ip addr add 2001:db8:2:6::6/64 dev tohost2
      - ip addr add 2001:db8:3:6::6/64 dev tohost3
      - ip addr add 2001:db8:5:6::6/64 dev tohost5
      - ip route add fd02:0:1::/48 via 2001:db8:1:6::1
      - ip route add fd02:0:2::/48 via 2001:db8:2:6::2
      - ip route add fd02:0:3::/48 via 2001:db8:3:6::3
      - ip route add fd02:0:4::/48 via 2001:db8:5:6::5
      - ip route add fd02:0:5::/48 via 2001:db8:5:6::5
      - ip link add dummy0 type dummy
      - ip link set dev dummy0 up
      - ip route add fd02:0:6::10/128 encap seg6local action End flavors psp count dev dummy0

  links:
    - endpoints: ["host1:tohost2", "host2:tohost1"] # 2001:db8:1:2::/64
      mtu: 1500
    - endpoints: ["host1:tohost6", "host6:tohost1"] # 2001:db8:1:6::/64
      mtu: 1500
    - endpoints: ["host2:tohost3", "host3:tohost2"] # 2001:db8:2:3::/64
      mtu: 1500
    - endpoints: ["host2:tohost5", "host5:tohost2"] # 2001:db8:2:5::/64
      mtu: 1500
    - endpoints: ["host2:tohost6", "host6:tohost2"] # 2001:db8:2:6::/64
      mtu: 1500
    - endpoints: ["host3:tohost4", "host4:tohost3"] # 2001:db8:3:4::/64
      mtu: 1500
    - endpoints: ["host3:tohost5", "host5:tohost3"] # 2001:db8:3:5::/64
      mtu: 1500
    - endpoints: ["host3:tohost6", "host6:tohost3"] # 2001:db8:3:6::/64
      mtu: 1500
    - endpoints: ["host4:tohost5", "host5:tohost4"] # 2001:db8:4:5::/64
      mtu: 1500
    - endpoints: ["host5:tohost6", "host6:tohost5"] # 2001:db8:5:6::/64
      mtu: 1500
```

The routes of type `fd02:0:${hostID}::/48` route packets within this CIDR to each host with the corresponding `hostID`.
The lab does not use any IGP on purpose. I wanted to abstract away as much as possible and focus on the configuration of the datapath, only.
Each node is configured with a loopback address of `fd02:0:${hostID}::1/128`. Routing only works between loopbacks, we must therefore specify the source address, e.g. `ping -c1 -W1 -I fd02:0:1::1 fd02:0:2::1`.

Now, let's deploy the lab:

```
# containerlab deploy
```

Once we are done, we can tear down the lab with:

```
# containerlab destroy
```

### SRv6 in our lab

Have another look at file `source-routing.clab.yml`.
We configure each host as a potential SRv6 Endpoint `seg6local action End`. Therefore, each host must be configured for IPv6 forwarding and `seg6` processing:

```
sysctl -w net.ipv6.conf.all.forwarding=1
sysctl -w net.ipv6.seg6_flowlabel=1
sysctl -w net.ipv6.conf.all.seg6_enabled=1
```

And each host will have the following route (where `${hostID}` is the ID of the host, 1 through 6) that triggers SRv6 Endoint forwarding  via `action End` with the `psp` flavor.

```
ip link add dummy0 type dummy
ip link set dev dummy0 up
ip route add fd02:0:${hostID}::10/128 encap seg6local action End flavors psp count dev dummy0
```
> **Note:** `encap seg6local` requires that the route point to some device, or `iproute2` will refuse the route. In our specific case, it does not matter which device, as long as it is up. Therefore, we add a `dummy0` interface to each host and use that one as the destination device. Packet forwarding will still happen according to the actual IPv6 destination of the packet.

The [iproute2](https://man7.org/linux/man-pages/man8/ip-route.8.html) man page explains how `encap action End` works.

```
                             seg6local
                               SEG6_ACTION [ SEG6_ACTION_PARAM ] [ count
                               ] - Operation to perform on matching
                               packets. The optional count attribute is
                               used to collect statistics on the
                               processing of actions.  Three counters are
                               implemented: 1) packets correctly
                               processed; 2) bytes correctly processed;
                               3) packets that cause a processing error
                               (i.e., missing SID List, wrong SID List,
                               etc). To retrieve the counters related to
                               an action use the -s flag in the show
                               command.  The following actions are
                               currently supported (Linux 4.14+ only).

                                 End [ flavors FLAVORS ] - Regular SRv6
                                 processing as intermediate segment
                                 endpoint.  This action only accepts
                                 packets with a non-zero Segments Left
                                 value. Other matching packets are
                                 dropped. The presence of flavors can
                                 change the regular processing of an End
                                 behavior according to the user-provided
                                 Flavor operations and information
                                 carried in the packet.  See Flavors
                                 parameters section.
```

It also provides a good explanation of  `flavors psp` which is responsible for penultimate Segment Pop operations.

```
psp - The Penultimate Segment Pop
  (PSP) copies the last SID from the SID
  List (carried by the outermost SRH)
  into the IPv6 Destination Address (DA)
  and removes (i.e. pops) the SRH from
  the IPv6 header.  The PSP operation
  takes place only at a penultimate SR
  Segment Endpoint node (e.g., the
  Segment Left must be one) and does not
  happen at non-penultimate endpoint
  nodes. This flavor is currently only
  supported by End behavior.
```

Don't worry - once we have a look at the actual datapath and packet captures, you can come back to these sections in the man pages and they will make
a lot more sense.

I already mentioned it in the earlier section, but for completeness:
in our lab, we configure the following routes on `host1` and `host4`, respectively, to describe the Source Routing path for the packets between these 2 hosts.

```
ip route add fd02:0:4::1 encap seg6 mode inline segs fd02:0:2::10,fd02:0:6::10,fd02:0:3::10,fd02:0:5::10 dev tohost2
```

```
ip route add fd02:0:1::1 encap seg6 mode inline segs fd02:0:3::10,fd02:0:6::10 dev tohost3
```

### Analysis of SRv6 traffic

Now, start packet captures on each host:
```
i=1; docker exec -it clab-source-routing-host${i} tcpdump -i any -w host${i}.pcap
i=2; docker exec -it clab-source-routing-host${i} tcpdump -i any -w host${i}.pcap
...
i=6; docker exec -it clab-source-routing-host${i} tcpdump -i any -w host${i}.pcap
```

Connect to `host1` and ping `host4` from there:

```
# docker exec -it clab-source-routing-host1 /bin/bash
host1:~# ping -c1 -W1 -I fd02:0:1::1 fd02:0:4::1
PING fd02:0:4::1 (fd02:0:4::1) from fd02:0:1::1 : 56 data bytes
64 bytes from fd02:0:4::1: icmp_seq=1 ttl=62 time=0.309 ms

--- fd02:0:4::1 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.309/0.309/0.309/0.000 ms
```

Kill the packet captures and copy the `.pcap` files to your local system:

```
for i in {1..6}; do docker cp clab-source-routing-host${i}:/root/host$i.pcap host$i.pcap; done
```

Thanks to the SRv6 routes, the ICMP echo request should follow the path from `host1` to `host4` via `host2`, `host6`, `host3` and `host5`.
Whereas the return ICMP echo reply should leave `host4` with `host1` as its final destination, travelling via `host3` and `host6`.

The ICMPv6 echo request packet leaves `host1`. Because we set `encap seg6 mode` to `inline`, the ICMPv6 payload directly follows the Source Routing Header (SRH), without additional encapsulation.
The SRH shows the list of addresses to visit, in reverse order. `Segments Left: 4`, and `Last Entry: 4` can be interpreted as pointers into the list of addresses.
`Address[Last Entry]` is the first hop to visit. `Address[Segments Left]` is the current hop and should be the same as the IPv6 Destination. `Address[0]` is the final destination of our packet. 

```
$ tshark -r host1.pcap -O ipv6 frame.number==3
Frame 3: Packet, 212 bytes on wire (1696 bits), 212 bytes captured (1696 bits)
Linux cooked capture v2
Internet Protocol Version 6, Src: fd02:0:1::1, Dst: fd02:0:2::10
    0110 .... = Version: 6
    .... 0000 0000 .... .... .... .... .... = Traffic Class: 0x00 (DSCP: CS0, ECN: Not-ECT)
        .... 0000 00.. .... .... .... .... .... = Differentiated Services Codepoint: Default (0)
        .... .... ..00 .... .... .... .... .... = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    .... 0000 1100 0010 1010 1110 = Flow Label: 0x0c2ae
    Payload Length: 152
    Next Header: Routing Header for IPv6 (43)
    Hop Limit: 64
    Source Address: fd02:0:1::1
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    Destination Address: fd02:0:2::10
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    [Stream index: 2]
    Routing Header for IPv6 (Segment Routing)
        Next Header: ICMPv6 (58)
        Length: 10
        [Length: 88 bytes]
        Type: Segment Routing (4)
        Segments Left: 4
        Last Entry: 4
        Flags: 0x00
        Tag: 0000
        Address[0]: fd02:0:4::1
        Address[1]: fd02:0:5::10
        Address[2]: fd02:0:3::10
        Address[3]: fd02:0:6::10
        Address[4]: fd02:0:2::10
Internet Control Message Protocol v6
HiPerConTracer Trace Service
```

The packet is processed on `host2`, where the `Hop Limit` and `Segments Left` are decreased by 1. The IPv6 Destination is changed to `Address[Segments Left]`, and the packet is forwarded to the next hop.

```
$ tshark -r host2.pcap -O ipv6 frame.number==6
Frame 6: Packet, 212 bytes on wire (1696 bits), 212 bytes captured (1696 bits)
Linux cooked capture v2
Internet Protocol Version 6, Src: fd02:0:1::1, Dst: fd02:0:6::10
    0110 .... = Version: 6
    .... 0000 0000 .... .... .... .... .... = Traffic Class: 0x00 (DSCP: CS0, ECN: Not-ECT)
        .... 0000 00.. .... .... .... .... .... = Differentiated Services Codepoint: Default (0)
        .... .... ..00 .... .... .... .... .... = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    .... 0000 1100 0010 1010 1110 = Flow Label: 0x0c2ae
    Payload Length: 152
    Next Header: Routing Header for IPv6 (43)
    Hop Limit: 63
    Source Address: fd02:0:1::1
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    Destination Address: fd02:0:6::10
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    [Stream index: 5]
    Routing Header for IPv6 (Segment Routing)
        Next Header: ICMPv6 (58)
        Length: 10
        [Length: 88 bytes]
        Type: Segment Routing (4)
        Segments Left: 3
        Last Entry: 4
        Flags: 0x00
        Tag: 0000
        Address[0]: fd02:0:4::1
        Address[1]: fd02:0:5::10
        Address[2]: fd02:0:3::10
        Address[3]: fd02:0:6::10
        Address[4]: fd02:0:2::10
Internet Control Message Protocol v6
HiPerConTracer Trace Service
```

The packet next arrives on `host6` where the same forwarding process continues.

```
$ tshark -r host6.pcap -O ipv6 frame.number==6
Frame 6: Packet, 212 bytes on wire (1696 bits), 212 bytes captured (1696 bits)
Linux cooked capture v2
Internet Protocol Version 6, Src: fd02:0:1::1, Dst: fd02:0:3::10
    0110 .... = Version: 6
    .... 0000 0000 .... .... .... .... .... = Traffic Class: 0x00 (DSCP: CS0, ECN: Not-ECT)
        .... 0000 00.. .... .... .... .... .... = Differentiated Services Codepoint: Default (0)
        .... .... ..00 .... .... .... .... .... = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    .... 0000 1100 0010 1010 1110 = Flow Label: 0x0c2ae
    Payload Length: 152
    Next Header: Routing Header for IPv6 (43)
    Hop Limit: 62
    Source Address: fd02:0:1::1
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    Destination Address: fd02:0:3::10
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    [Stream index: 5]
    Routing Header for IPv6 (Segment Routing)
        Next Header: ICMPv6 (58)
        Length: 10
        [Length: 88 bytes]
        Type: Segment Routing (4)
        Segments Left: 2
        Last Entry: 4
        Flags: 0x00
        Tag: 0000
        Address[0]: fd02:0:4::1
        Address[1]: fd02:0:5::10
        Address[2]: fd02:0:3::10
        Address[3]: fd02:0:6::10
        Address[4]: fd02:0:2::10
Internet Control Message Protocol v6
HiPerConTracer Trace Service
```

The packet next hits `host3` where the packet is processed just as before and is forwarded to `host5`.

```
$ tshark -r host3.pcap -O ipv6 frame.number==6
Frame 6: Packet, 212 bytes on wire (1696 bits), 212 bytes captured (1696 bits)
Linux cooked capture v2
Internet Protocol Version 6, Src: fd02:0:1::1, Dst: fd02:0:5::10
    0110 .... = Version: 6
    .... 0000 0000 .... .... .... .... .... = Traffic Class: 0x00 (DSCP: CS0, ECN: Not-ECT)
        .... 0000 00.. .... .... .... .... .... = Differentiated Services Codepoint: Default (0)
        .... .... ..00 .... .... .... .... .... = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    .... 0000 1100 0010 1010 1110 = Flow Label: 0x0c2ae
    Payload Length: 152
    Next Header: Routing Header for IPv6 (43)
    Hop Limit: 61
    Source Address: fd02:0:1::1
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    Destination Address: fd02:0:5::10
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    [Stream index: 5]
    Routing Header for IPv6 (Segment Routing)
        Next Header: ICMPv6 (58)
        Length: 10
        [Length: 88 bytes]
        Type: Segment Routing (4)
        Segments Left: 1
        Last Entry: 4
        Flags: 0x00
        Tag: 0000
        Address[0]: fd02:0:4::1
        Address[1]: fd02:0:5::10
        Address[2]: fd02:0:3::10
        Address[3]: fd02:0:6::10
        Address[4]: fd02:0:2::10
Internet Control Message Protocol v6
HiPerConTracer Trace Service
```

`host5` checks the `Segments Left` of the incoming packet and determines that it is the penultimate hop (`Segments Left: 1`). Therefore, the logic now changes and `flavors psp` causes `host5` to behave differently.

The host sets the IPv6 Destination to the value of `Address[0]` and pops the SRH.

```
$ tshark -r host5.pcap -O ipv6 frame.number==6
Frame 6: Packet, 124 bytes on wire (992 bits), 124 bytes captured (992 bits)
Linux cooked capture v2
Internet Protocol Version 6, Src: fd02:0:1::1, Dst: fd02:0:4::1
    0110 .... = Version: 6
    .... 0000 0000 .... .... .... .... .... = Traffic Class: 0x00 (DSCP: CS0, ECN: Not-ECT)
        .... 0000 00.. .... .... .... .... .... = Differentiated Services Codepoint: Default (0)
        .... .... ..00 .... .... .... .... .... = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    .... 0000 1100 0010 1010 1110 = Flow Label: 0x0c2ae
    Payload Length: 64
    Next Header: ICMPv6 (58)
    Hop Limit: 60
    Source Address: fd02:0:1::1
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    Destination Address: fd02:0:4::1
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    [Stream index: 5]
Internet Control Message Protocol v6
HiPerConTracer Trace Service
```

The packet is forwarded to `host4`'s loopback and `host4` generates an ICMPv6 echo reply together with a Source Routing Header that instructs the packet to travel via `host3` and `host6`.

```
$ tshark -r host4.pcap -O ipv6 frame.number==6
Frame 6: Packet, 180 bytes on wire (1440 bits), 180 bytes captured (1440 bits)
Linux cooked capture v2
Internet Protocol Version 6, Src: fd02:0:4::1, Dst: fd02:0:3::10
    0110 .... = Version: 6
    .... 0000 0000 .... .... .... .... .... = Traffic Class: 0x00 (DSCP: CS0, ECN: Not-ECT)
        .... 0000 00.. .... .... .... .... .... = Differentiated Services Codepoint: Default (0)
        .... .... ..00 .... .... .... .... .... = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    .... 1000 0101 0110 1100 1011 = Flow Label: 0x856cb
    Payload Length: 120
    Next Header: Routing Header for IPv6 (43)
    Hop Limit: 64
    Source Address: fd02:0:4::1
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    Destination Address: fd02:0:3::10
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    [Stream index: 5]
    Routing Header for IPv6 (Segment Routing)
        Next Header: ICMPv6 (58)
        Length: 6
        [Length: 56 bytes]
        Type: Segment Routing (4)
        Segments Left: 2
        Last Entry: 2
        Flags: 0x00
        Tag: 0000
        Address[0]: fd02:0:1::1
        Address[1]: fd02:0:6::10
        Address[2]: fd02:0:3::10
Internet Control Message Protocol v6
HiPerConTracer Trace Service
```

`host6` pops the SRH, and the packet is delivered to `host1`.

## Conclusion

In this blog post, we explored the basics of SRv6 Source Routing with Linux. We set up a lab using containerlab and configured SRv6 routes to demonstrate how packets are forwarded along a specified path.
