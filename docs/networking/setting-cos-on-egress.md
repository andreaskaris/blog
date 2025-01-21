# Setting COS on egress in Linux

In order to set COS ([802.1Q](https://en.wikipedia.org/wiki/IEEE_802.1Q) Priority Code Point) on outgoing frames, you
first need to set the VLAN interface's [egress-qos-map](https://man7.org/linux/man-pages/man8/ip-link.8.html) to match
the kernel's internal [SKB priorities](https://github.com/torvalds/linux/blob/95ec54a420b8f445e04a7ca0ea8deb72c51fe1d3/include/linux/skbuff.h#L779).
According to the man page:

> defines a mapping of
> Linux internal packet priority to VLAN header prio
> field but for outgoing frames

Becaue the COS has 8 distinct values, let's map internal priorities one to one up to 7:

```
ip link set enp1s0.123 type vlan egress 0:0 1:1 2:2 3:3 4:4 5:5 6:6 7:7
```

Next, if the packet originates from the local node, instruct it to set the internal priority class in the OUTPUT chain
of the mangle table. For example, to set all ICMP packets that leave this node to SKB priority 6 and subsequently COS 6,
you can use the following command:

```
iptables -t mangle -I OUTPUT -p icmp -j CLASSIFY --set-class 0:6
```
