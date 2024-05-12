## Netlink address fields IFLA_ADDRESS, IFLA_BROADCAST and IFLA_PERM_ADDRESS

A few days ago, I had to figure out how applications such as iproute2 read the MAC address from kernel interfaces. More
specifically, we observed that the MAC addresses of tunnel interfaces seemed to be shorter or longer than 6 Bytes,
ranging from 4 to 16 Bytes depending on the tunnel type.
It turned out that the kernel actually communicates no such field as the MAC address. Instead, a netlink field called
`IFLA_ADDRESS` transmits the address of the underlying medium which can be an Ethernet MAC (6 Bytes) in case of Ethernet,
or the 4 Byte IPv4 tunnel source address, the 16 Byte IPv6 tunnel source address, etc.

### The kernel

The [kernel sets](https://github.com/torvalds/linux/blob/448b3fe5a0eab5b625a7e15c67c7972169e47ff8/net/core/rtnetlink.c#L1881)
netlink field `IFLA_ADDRESS` to the value of `dev->addr` with length `dev->addr_len` and netlink field
`IFLA_BROADCAST` to the value of `dev->broadcast` with length `dev->addr_len`. It
[sets](https://github.com/torvalds/linux/blob/448b3fe5a0eab5b625a7e15c67c7972169e47ff8/net/core/rtnetlink.c#L1923)
netlink field `IFLA_PERM_ADDESS` to the value of `dev->perm_addr` with length `dev->addr_len`.

For Ethernet devices, the `addr` field is
[populated with the hardware address of the device](https://github.com/torvalds/linux/blob/ba16c1cf11c9f264b5455cb7d57267b39925409a/include/linux/etherdevice.h#L319).
The `broadcast` field is set to [ff:ff:ff:ff:ff:ff](https://github.com/torvalds/linux/blob/ba16c1cf11c9f264b5455cb7d57267b39925409a/net/ethernet/eth.c#L361).
The `addr_len` is [set to 6 Bytes](https://github.com/torvalds/linux/blob/ba16c1cf11c9f264b5455cb7d57267b39925409a/net/ethernet/eth.c#L356).
I couldn't find the exact location where the perm_addr is set (maybe [here](https://github.com/torvalds/linux/blob/ba16c1cf11c9f264b5455cb7d57267b39925409a/net/core/dev.c#L10313)?), however, from strace output, it is clear that for Ethernet devices, the `perm_addr` equals the `addr`.

For IPv6 GRE, the `addr` and `broadcast` fields are populated with `laddr` and `raddr` of the tunnel, at locations:

* [https://github.com/torvalds/linux/blob/448b3fe5a0eab5b625a7e15c67c7972169e47ff8/net/ipv6/ip6_tunnel.c#L1468](https://github.com/torvalds/linux/blob/448b3fe5a0eab5b625a7e15c67c7972169e47ff8/net/ipv6/ip6_tunnel.c#L1468)
* [https://github.com/torvalds/linux/blob/448b3fe5a0eab5b625a7e15c67c7972169e47ff8/net/ipv6/ip6_gre.c#L1542C2-L1543C72](https://github.com/torvalds/linux/blob/448b3fe5a0eab5b625a7e15c67c7972169e47ff8/net/ipv6/ip6_gre.c#L1542C2-L1543C72)
* [https://github.com/torvalds/linux/blob/448b3fe5a0eab5b625a7e15c67c7972169e47ff8/net/ipv6/ip6_gre.c#L1108](https://github.com/torvalds/linux/blob/448b3fe5a0eab5b625a7e15c67c7972169e47ff8/net/ipv6/ip6_gre.c#L1108)

The length of the 2 fields is set to the size of `in6_addr`, meaning to the size of an IPv6 address (16 Bytes if we
exclude overhead).

Similar code is used for IPv6 IP in IP tunnels. Even though I have not verified it, I assume that IPv4 tunnels are
handled the same way.

#### The strange handling of tunnel IFLA_PERM_ADDRESS length

For tunnel interfaces,  the `perm_addr` field actually
[contains a randomly generated MAC address](https://github.com/torvalds/linux/blob/448b3fe5a0eab5b625a7e15c67c7972169e47ff8/net/ipv6/ip6_gre.c#L1450),
and thus the
[advertised length](https://github.com/torvalds/linux/blob/448b3fe5a0eab5b625a7e15c67c7972169e47ff8/net/core/rtnetlink.c#L1923)
of `IFLA_PERM_ADDRESS` does not match the length of the field's contents.
A permanent address for an IPv6 tunnel holding a random 6 Byte MAC address will be interpreted as a 16 Byte permanent
address on the client side. And for IPv4 tunnels, the address will actually be interpreted as having the length of an
IPv4 address, and the last 2 Bytes are cut off. We can clearly see this from the various outputs of strace.

* strace output of field `IFLA_PERM_ADDRESS` for an Ethernet interface:
```
[{nla_len=10, nla_type=IFLA_PERM_ADDRESS}, 52:54:00:37:3b:25]
```

* strace output of field `IFLA_PERM_ADDRESS` for an IPv4 GRE tunnel:
```
[{nla_len=8, nla_type=IFLA_PERM_ADDRESS}, c0:a8:7b:0a]
```

* strace output of field `IFLA_PERM_ADDRESS` for an IPv6 GRE tunnel:
```
[{nla_len=20, nla_type=IFLA_PERM_ADDRESS}, 8a:c3:53:32:b0:22:00]
```

This leads to a display bug in the output of `ip link` for IPv6 tunnels, whereas the `permaddr` field is not displayed
for IPv4 tunnels.

### iproute2

Let's use iproute2 to list an IPv6 GRE link:
```
# strace -s4096 -f -o /tmp/strace2.out ip link ls test0
14: test0@NONE: <POINTOPOINT,NOARP> mtu 1448 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/gre6 2345::25 peer 2345::26 permaddr 8ac3:5332:b022::
```

In iproute2, interface information is retrieved in function
[iplink_get](https://github.com/iproute2/iproute2/blob/853f34bf09c38542c6cf2cacf0893fd778284c26/ip/iplink.c#L1100).

In the strace output, we can see the netlink request to the kernel:
```
2726  sendmsg(4, {msg_name={sa_family=AF_NETLINK, nl_pid=0, nl_groups=00000000}, msg_namelen=12, msg_iov=[{iov_base=[{nlmsg_len=52, nlmsg_type=RTM_GETLINK, nl
msg_flags=NLM_F_REQUEST, nlmsg_seq=1715534685, nlmsg_pid=0}, {ifi_family=AF_UNSPEC, ifi_type=ARPHRD_NETROM, ifi_index=0, ifi_flags=0, ifi_change=0}, [[{nla_le
n=8, nla_type=IFLA_EXT_MASK}, RTEXT_FILTER_VF|RTEXT_FILTER_SKIP_STATS], [{nla_len=10, nla_type=IFLA_IFNAME}, "test0"]]], iov_len=52}], msg_iovlen=1, msg_contr
ollen=0, msg_flags=0}, 0) = 52
```

Note how we are requesting information for interface `[{nla_len=10, nla_type=IFLA_IFNAME}, "test0"]]]` because an interface
name was specified:
```
	if (name) {
		addattr_l(&req.n, sizeof(req),
			  !check_ifname(name) ? IFLA_IFNAME : IFLA_ALT_IFNAME,
			  name, strlen(name) + 1);
	}
```

The netlink request is sent and received in `rtnl_talk`:
```
	if (rtnl_talk(&rth, &req.n, &answer) < 0)
		return -2;
```

The strace shows a fairly long answer:
```
# strace response with manual formatting
2726  recvmsg(4, {msg_name={sa_family=AF_NETLINK, nl_pid=0, nl_groups=00000000}, msg_namelen=12, msg_iov=[
{iov_base=[{nlmsg_len=1192, nlmsg_type=RTM_NEWLINK, nlmsg_flags=0, nlmsg_seq=1715534685, nlmsg_pid=-1224665210},
           {ifi_family=AF_UNSPEC, ifi_type=ARPHRD_IP6GRE, ifi_index=if_nametoindex("test0"), ifi_flags=IFF_POINTOPOINT|IFF_NOARP, ifi_change=0},
           [[{nla_len=10, nla_type=IFLA_IFNAME}, "test0"], 
            [{nla_len=8, nla_type=IFLA_TXQLEN}, 1000],
            [{nla_len=5, nla_type=IFLA_OPERSTATE}, 2],
            [{nla_len=5, nla_type=IFLA_LINKMODE}, 0], 
            [{nla_len=8, nla_type=IFLA_MTU}, 1448], 
            [{nla_len=8, nla_type=IFLA_MIN_MTU}, 0], 
            [{nla_len=8, nla_type=IFLA_MAX_MTU}, 0], 
            [{nla_len=8, nla_type=IFLA_GROUP}, 0], 
            [{nla_len=8, nla_type=IFLA_PROMISCUITY}, 0], 
            [{nla_len=8, nla_type=IFLA_NUM_TX_QUEUES}, 1], 
            [{nla_len=8, nla_type=IFLA_GSO_MAX_SEGS}, 65535], 
            [{nla_len=8, nla_type=IFLA_GSO_MAX_SIZE}, 65536], 
            [{nla_len=8, nla_type=IFLA_GRO_MAX_SIZE}, 65536], 
            [{nla_len=8, nla_type=0x3b /* IFLA_??? */}, "\x00\x00\x01\x00"], 
            [{nla_len=8, nla_type=0x3c /* IFLA_??? */}, "\xff\xff\x00\x00"], 
            [{nla_len=8, nla_type=IFLA_NUM_RX_QUEUES}, 1], 
            [{nla_len=5, nla_type=IFLA_CARRIER}, 1], 
            [{nla_len=9, nla_type=IFLA_QDISC}, "noop"], 
            [{nla_len=8, nla_type=IFLA_CARRIER_CHANGES}, 0], 
            [{nla_len=8, nla_type=IFLA_CARRIER_UP_COUNT}, 0], 
            [{nla_len=8, nla_type=IFLA_CARRIER_DOWN_COUNT}, 0], 
            [{nla_len=5, nla_type=IFLA_PROTO_DOWN}, 0], 
            [{nla_len=36, nla_type=IFLA_MAP}, {mem_start=0, mem_end=0, base_addr=0, irq=0, dma=0, port=0}], 
            [{nla_len=20, nla_type=IFLA_ADDRESS}, 23:45:00:00:00:00:00], 
            [{nla_len=20, nla_type=IFLA_BROADCAST}, 23:45:00:00:00:00:00], (... output omitted ...), 
            [{nla_len=20, nla_type=IFLA_PERM_ADDRESS}, 8a:c3:53:32:b0:22:00], (... output omitted ...)
```
> **Note:** You can see fields here such as `IFLA_PERM_ADDRESS`, `IFLA_ADDRESS`, `IFLA_BROADCAST`.

> **Note:** Both `IFLA_ADDRESS` and `IFLA_BROADCAST` are not displayed correctly by strace and in the case of tunnels
contain the actual left and right addresses.

The retrieved information is then printed with function
[print_linkinfo](https://github.com/iproute2/iproute2/blob/853f34bf09c38542c6cf2cacf0893fd778284c26/ip/ipaddress.c#L1002).
The first line of the `ip link` output is printed with
[print_nl](https://github.com/iproute2/iproute2/blob/853f34bf09c38542c6cf2cacf0893fd778284c26/ip/ipaddress.c#L1107).
The link type (`link/%s`) is read from the `ifi_type` field. The source address is read from `IFLA_ADDRESS`, and if
inside `ifi_flags` flag `IFF_POINTOPOINT` is set to true, the `peer` keyword will be printed, followed by the
`IFLA_BROADCAST`. From this side of the code, it is clear that the left tunnel address is stored inside `IFLA_ADDRESS`
and the right tunnel address is stored inside `IFLA_BROADCAST`.

#### The strange handling of tunnel IFLA_PERM_ADDRESS length

The permanent address
([permaddr](https://github.com/iproute2/iproute2/blob/853f34bf09c38542c6cf2cacf0893fd778284c26/ip/ipaddress.c#L1149))
is extracted from `IFLA_PERM_ADDRESS`.

We can observe an interesting detail from the strace, and also from the output of `ip link`: even though the `permaddr`
holds a randomly generated, 6 Byte MAC address, the field length in total is 20 - 4 = 16 Bytes. That's the length of
an IPv6 address. As a consequence, `ip link` also prints the address in IPv6 format, even though it should actually
print until the first 6 Bytes in MAC address notation.
We already explained in the kernel section why we see this. However, for IPv4 tunnels, iproute2 does not print
the `permaddr`, probably because the field actually holds 6 Bytes of content, but the length field only advertises
4 Bytes + overhead:
```
# ip link ls dev test2
19: test2@NONE: <POINTOPOINT,NOARP> mtu 1476 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/gre 192.168.123.10 peer 192.168.123.20
```

To me, these findings indicate a minor bug in iproute2 as it should probably not print the permaddr for IPv6 tunnels.
