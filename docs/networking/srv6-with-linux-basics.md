## SRV6 IP Tunnels with Linux: End.DT46 Basics
 
This article demonstrates how to configure SRV6 tunnels using Linux. While SRV6 tunnels can be implemented using FRR static routes or FRR's L3VPN, this guide focuses specifically on dataplane configuration using `iproute2`.

### Lab Setup

#### Lab layout

We configure our lab using Linux namespaces. Conceptually, our lab simulates the following 5 hosts:

```
   ┌─────────────────────┐    ┌───────────────────────────────────┐    ┌─────────────────────┐
   │edgeleft             │    │transit                            │    │edgeright            │
   │                     │    │                                   │    │                     │ 
   ┌─────────┐           │    │                                   │    │            ┌────────┐
   │     vrfa│  totransit│────│toedgeleft               toedgeleft│────│totransit   │    vrfa│
   │         │           │    │                                   │    │            │        │
   │tohost   │           │    │                                   │    │            │tohost  │
   └─────────┘───────────┘    └───────────────────────────────────┘    └────────────└────────┘
      │                                                                                │
   ┌─────────────────────┐                                             ┌─────────────────────┐
   │toedge       hostleft│                                             │hostright    toedge  │
   │                     │                                             │                     │
   │                     │                                             │                     │
   │                     │                                             │                     │
   │                     │                                             │                     │
   │                     │                                             │                     │
   └─────────────────────┘                                             └─────────────────────┘
```

The links will have the following IP addresses:

- hostleft/toedge: 192.168.160.1/24
- edgeleft/tohost: 192.168.160.2/24
- edgeleft/totransit: 2001:db8:168:160::2/64
- transit/toedgeleft: 2001:db8:168:160::1/64
- transit/toedgeright: 2001:db8:168:161::1/64
- edgeright/totransit: 2001:db8:168:161::2/64
- edgeright/tohost: 192.168.161.2/24
- hostright/toedge: 192.168.161.1/24

And the following default routes are set to drive traffic from the hosts and edges to the transit host:

- hostleft: default via 192.168.160.2
- edgeleft: default via 2001:db8:168:160::1
- edgeright: default via 2001:db8:168:161::1
- hostright: default via 192.168.161.2

Later in this blog post, we add routes to the tunnel endpoints on the edge nodes. Therefore, the transit network must know how to reach these routes:

- transit: fd00:168:161::/48 via 2001:db8:168:161::2
- transit: fd00:168:160::/48 via 2001:db8:168:160::2

Don't worry, the purpose behind these routes will become clear soon.

In order to make SRV6 work, the following sysctl settings are configured on the edge nodes:

- net.vrf.strict_mode=1
- net.ipv4.conf.all.forwarding=1
- net.ipv6.conf.all.forwarding=1
- net.ipv6.seg6_flowlabel=1
- net.ipv6.conf.all.seg6_enabled=1

And on the transit node we enable IPv6 forwarding:

- net.ipv6.conf.all.forwarding=1
                                                                                             
#### Setup script

We set up our lab environment using the following script:

```
#!/bin/bash

noop=false

run_cmd() {
  cmd="$1"
  echo "${cmd}"
  if ! $noop; then
    eval "${cmd}"
  fi
}

namespaces=("hostleft" "edgeleft" "transit" "edgeright" "hostright")
edge_namespaces=("edgeleft" "edgeright")
transit_netns="transit"
transit_netns_routes=("fd00:168:161::/48 via 2001:db8:168:161::2" "fd00:168:160::/48 via 2001:db8:168:160::2")

declare -A if_hostleft
if_hostleft[name]="toedge"
if_hostleft[ns]="hostleft"
if_hostleft[peer]="tohost"
if_hostleft[peer_ns]="edgeleft"
if_hostleft[ip]="192.168.160.1/24"
if_hostleft[peer_ip]="192.168.160.2/24"
if_hostleft[route]="default via 192.168.160.2"

declare -A if_edgeleft
if_edgeleft[name]="totransit"
if_edgeleft[ns]="edgeleft"
if_edgeleft[peer]="toedgeleft"
if_edgeleft[peer_ns]="transit"
if_edgeleft[ip]="2001:db8:168:160::2/64"
if_edgeleft[peer_ip]="2001:db8:168:160::1/64"
if_edgeleft[route]="default via 2001:db8:168:160::1"

declare -A if_edgeright
if_edgeright[name]="totransit"
if_edgeright[ns]="edgeright"
if_edgeright[peer]="toedgeright"
if_edgeright[peer_ns]="transit"
if_edgeright[ip]="2001:db8:168:161::2/64"
if_edgeright[peer_ip]="2001:db8:168:161::1/64"
if_edgeright[route]="default via 2001:db8:168:161::1"

declare -A if_hostright
if_hostright[name]="toedge"
if_hostright[ns]="hostright"
if_hostright[peer]="tohost"
if_hostright[peer_ns]="edgeright"
if_hostright[ip]="192.168.161.1/24"
if_hostright[peer_ip]="192.168.161.2/24"
if_hostright[route]="default via 192.168.161.2"

interfaces=(if_hostleft if_edgeleft if_edgeright if_hostright)

setup() {
  for intf_name in "${interfaces[@]}"; do
    declare -n intf="${intf_name}"
    if ip netns exec "${intf[ns]}" ip link ls dev "${intf[name]}" >/dev/null 2>&1; then
      continue
    fi
    echo "=== Adding link ${intf[name]} to ${intf[peer]} ==="
    run_cmd "ip netns add ${intf[ns]}" 2>/dev/null || true
    run_cmd "ip netns add ${intf[peer_ns]}" 2>/dev/null || true
    run_cmd "ip netns exec ${intf[ns]} ip link set lo up"
    run_cmd "ip netns exec ${intf[peer_ns]} ip link set lo up"
    run_cmd "ip link add dev ${intf[name]} type veth peer name ${intf[peer]}" 
    run_cmd "ip link set ${intf[name]} netns ${intf[ns]}"
    run_cmd "ip link set ${intf[peer]} netns ${intf[peer_ns]}"
    run_cmd "ip netns exec ${intf[ns]} ip link set dev ${intf[name]} up"
    run_cmd "ip netns exec ${intf[peer_ns]} ip link set dev ${intf[peer]} up"
    run_cmd "ip netns exec ${intf[ns]} ip a a dev ${intf[name]} ${intf[ip]}"
    run_cmd "ip netns exec ${intf[peer_ns]} ip a a dev ${intf[peer]} ${intf[peer_ip]}"
    run_cmd "ip netns exec ${intf[ns]} ip route a ${intf[route]}"
  done

  for route in "${transit_netns_routes[@]}"; do
    if ! ip netns exec "${transit_netns}" ip -6 route ls | grep -q "${route}"; then
      run_cmd "ip netns exec ${transit_netns} ip route add ${route}"
    fi
  done

  for edge_netns in "${edge_namespaces[@]}"; do
    run_cmd "ip netns exec ${edge_netns} sysctl -w net.vrf.strict_mode=1"
    run_cmd "ip netns exec ${edge_netns} sysctl -w net.ipv4.conf.all.forwarding=1"
    run_cmd "ip netns exec ${edge_netns} sysctl -w net.ipv6.conf.all.forwarding=1"
    run_cmd "ip netns exec ${edge_netns} sysctl -w net.ipv6.seg6_flowlabel=1"
    run_cmd "ip netns exec ${edge_netns} sysctl -w net.ipv6.conf.all.seg6_enabled=1"
  done

  run_cmd "ip netns exec ${transit_netns} sysctl -w net.ipv6.conf.all.forwarding=1"
}

cleanup() {
  for ns in "${namespaces[@]}"; do
    run_cmd "ip netns del ${ns}"
  done 
}

op="${1:-setup}"
if [ "$op" == "setup" ]; then
  setup
fi
if [ "$op" == "cleanup" ]; then
  cleanup
fi
```

The script will print and execute the commands. For your reference, here is the full output:

```
# bash setup.sh
=== Adding link toedge to tohost ===
ip netns add hostleft
ip netns add edgeleft
ip netns exec hostleft ip link set lo up
ip netns exec edgeleft ip link set lo up
ip link add dev toedge type veth peer name tohost
ip link set toedge netns hostleft
ip link set tohost netns edgeleft
ip netns exec hostleft ip link set dev toedge up
ip netns exec edgeleft ip link set dev tohost up
ip netns exec hostleft ip a a dev toedge 192.168.160.1/24
ip netns exec edgeleft ip a a dev tohost 192.168.160.2/24
ip netns exec hostleft ip route a default via 192.168.160.2
=== Adding link totransit to toedgeleft ===
ip netns add edgeleft
ip netns add transit
ip netns exec edgeleft ip link set lo up
ip netns exec transit ip link set lo up
ip link add dev totransit type veth peer name toedgeleft
ip link set totransit netns edgeleft
ip link set toedgeleft netns transit
ip netns exec edgeleft ip link set dev totransit up
ip netns exec transit ip link set dev toedgeleft up
ip netns exec edgeleft ip a a dev totransit 2001:db8:168:160::2/64
ip netns exec transit ip a a dev toedgeleft 2001:db8:168:160::1/64
ip netns exec edgeleft ip route a default via 2001:db8:168:160::1
=== Adding link totransit to toedgeright ===
ip netns add edgeright
ip netns add transit
ip netns exec edgeright ip link set lo up
ip netns exec transit ip link set lo up
ip link add dev totransit type veth peer name toedgeright
ip link set totransit netns edgeright
ip link set toedgeright netns transit
ip netns exec edgeright ip link set dev totransit up
ip netns exec transit ip link set dev toedgeright up
ip netns exec edgeright ip a a dev totransit 2001:db8:168:161::2/64
ip netns exec transit ip a a dev toedgeright 2001:db8:168:161::1/64
ip netns exec edgeright ip route a default via 2001:db8:168:161::1
=== Adding link toedge to tohost ===
ip netns add hostright
ip netns add edgeright
ip netns exec hostright ip link set lo up
ip netns exec edgeright ip link set lo up
ip link add dev toedge type veth peer name tohost
ip link set toedge netns hostright
ip link set tohost netns edgeright
ip netns exec hostright ip link set dev toedge up
ip netns exec edgeright ip link set dev tohost up
ip netns exec hostright ip a a dev toedge 192.168.161.1/24
ip netns exec edgeright ip a a dev tohost 192.168.161.2/24
ip netns exec hostright ip route a default via 192.168.161.2
ip netns exec transit ip route add fd00:168:161::/48 via 2001:db8:168:161::2
ip netns exec transit ip route add fd00:168:160::/48 via 2001:db8:168:160::2
ip netns exec edgeleft sysctl -w net.vrf.strict_mode=1
net.vrf.strict_mode = 1
ip netns exec edgeleft sysctl -w net.ipv4.conf.all.forwarding=1
net.ipv4.conf.all.forwarding = 1
ip netns exec edgeleft sysctl -w net.ipv6.conf.all.forwarding=1
net.ipv6.conf.all.forwarding = 1
ip netns exec edgeleft sysctl -w net.ipv6.seg6_flowlabel=1
net.ipv6.seg6_flowlabel = 1
ip netns exec edgeleft sysctl -w net.ipv6.conf.all.seg6_enabled=1
net.ipv6.conf.all.seg6_enabled = 1
ip netns exec edgeright sysctl -w net.vrf.strict_mode=1
net.vrf.strict_mode = 1
ip netns exec edgeright sysctl -w net.ipv4.conf.all.forwarding=1
net.ipv4.conf.all.forwarding = 1
ip netns exec edgeright sysctl -w net.ipv6.conf.all.forwarding=1
net.ipv6.conf.all.forwarding = 1
ip netns exec edgeright sysctl -w net.ipv6.seg6_flowlabel=1
net.ipv6.seg6_flowlabel = 1
ip netns exec edgeright sysctl -w net.ipv6.conf.all.seg6_enabled=1
net.ipv6.conf.all.seg6_enabled = 1
ip netns exec transit sysctl -w net.ipv6.conf.all.forwarding=1
net.ipv6.conf.all.forwarding = 1
```

#### Lab Verification

With the lab environment now configured, we can connect to our simulated hosts:

```
# ip netns exec hostleft /bin/bash
# ip a && ip r
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host proto kernel_lo 
       valid_lft forever preferred_lft forever
863: toedge@if862: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 4a:f5:2f:ff:f7:82 brd ff:ff:ff:ff:ff:ff link-netns edgeleft
    inet 192.168.160.1/24 scope global toedge
       valid_lft forever preferred_lft forever
    inet6 fe80::48f5:2fff:feff:f782/64 scope link proto kernel_ll 
       valid_lft forever preferred_lft forever
default via 192.168.160.2 dev toedge 
192.168.160.0/24 dev toedge proto kernel scope link src 192.168.160.1
```

```
# ip netns exec edgeleft /bin/bash
# ip a && ip r && ip -6 r
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host proto kernel_lo 
       valid_lft forever preferred_lft forever
862: tohost@if863: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 7e:54:bc:8b:57:5e brd ff:ff:ff:ff:ff:ff link-netns hostleft
    inet 192.168.160.2/24 scope global tohost
       valid_lft forever preferred_lft forever
    inet6 fe80::7c54:bcff:fe8b:575e/64 scope link proto kernel_ll 
       valid_lft forever preferred_lft forever
865: totransit@if864: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether c2:62:80:bc:5e:e8 brd ff:ff:ff:ff:ff:ff link-netns transit
    inet6 2001:db8:168:160::2/64 scope global 
       valid_lft forever preferred_lft forever
    inet6 fe80::c062:80ff:febc:5ee8/64 scope link proto kernel_ll 
       valid_lft forever preferred_lft forever
192.168.160.0/24 dev tohost proto kernel scope link src 192.168.160.2 
2001:db8:168:160::/64 dev totransit proto kernel metric 256 pref medium
fe80::/64 dev tohost proto kernel metric 256 pref medium
fe80::/64 dev totransit proto kernel metric 256 pref medium
default via 2001:db8:168:160::1 dev totransit metric 1024 pref medium
```

```
# ip netns exec transit /bin/bash
# ip a && ip r && ip -6 r
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host proto kernel_lo 
       valid_lft forever preferred_lft forever
864: toedgeleft@if865: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 6e:1e:04:f9:11:54 brd ff:ff:ff:ff:ff:ff link-netns edgeleft
    inet6 2001:db8:168:160::1/64 scope global 
       valid_lft forever preferred_lft forever
    inet6 fe80::6c1e:4ff:fef9:1154/64 scope link proto kernel_ll 
       valid_lft forever preferred_lft forever
866: toedgeright@if867: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 46:da:f4:4d:ad:f3 brd ff:ff:ff:ff:ff:ff link-netns edgeright
    inet6 2001:db8:168:161::1/64 scope global 
       valid_lft forever preferred_lft forever
    inet6 fe80::44da:f4ff:fe4d:adf3/64 scope link proto kernel_ll 
       valid_lft forever preferred_lft forever
2001:db8:168:160::/64 dev toedgeleft proto kernel metric 256 pref medium
2001:db8:168:161::/64 dev toedgeright proto kernel metric 256 pref medium
fd00:168:160::/48 via 2001:db8:168:160::2 dev toedgeleft metric 1024 pref medium
fd00:168:161::/48 via 2001:db8:168:161::2 dev toedgeright metric 1024 pref medium
fe80::/64 dev toedgeleft proto kernel metric 256 pref medium
fe80::/64 dev toedgeright proto kernel metric 256 pref medium
```

```
# ip netns exec edgeright /bin/bash
# ip a && ip r && ip -6 r
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host proto kernel_lo 
       valid_lft forever preferred_lft forever
867: totransit@if866: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether be:28:4a:5f:fa:ff brd ff:ff:ff:ff:ff:ff link-netns transit
    inet6 2001:db8:168:161::2/64 scope global 
       valid_lft forever preferred_lft forever
    inet6 fe80::bc28:4aff:fe5f:faff/64 scope link proto kernel_ll 
       valid_lft forever preferred_lft forever
868: tohost@if869: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 7e:ee:e7:71:de:63 brd ff:ff:ff:ff:ff:ff link-netns hostright
    inet 192.168.161.2/24 scope global tohost
       valid_lft forever preferred_lft forever
    inet6 fe80::7cee:e7ff:fe71:de63/64 scope link proto kernel_ll 
       valid_lft forever preferred_lft forever
192.168.161.0/24 dev tohost proto kernel scope link src 192.168.161.2 
2001:db8:168:161::/64 dev totransit proto kernel metric 256 pref medium
fe80::/64 dev totransit proto kernel metric 256 pref medium
fe80::/64 dev tohost proto kernel metric 256 pref medium
default via 2001:db8:168:161::1 dev totransit metric 1024 pref medium
```

```
# ip netns exec hostright /bin/bash
# ip a && ip r
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host proto kernel_lo 
       valid_lft forever preferred_lft forever
869: toedge@if868: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 1a:4b:fd:20:b5:c7 brd ff:ff:ff:ff:ff:ff link-netns edgeright
    inet 192.168.161.1/24 scope global toedge
       valid_lft forever preferred_lft forever
    inet6 fe80::184b:fdff:fe20:b5c7/64 scope link proto kernel_ll 
       valid_lft forever preferred_lft forever
default via 192.168.161.2 dev toedge 
192.168.161.0/24 dev toedge proto kernel scope link src 192.168.161.1
```

### Configuring SRV6 End.DT46 Tunnels

Next, we set up packet captures on the transit interfaces. In the first terminal:

```
# intf=toedgeleft; tcpdump -i $intf -w ${intf}.pcap & tcpdump -nne -i ${intf}
```

In the second terminal:

```
# intf=toedgeright; tcpdump -i $intf -w ${intf}.pcap & tcpdump -nne -i ${intf}
```

Let's move the `tohost` interface on both edge nodes into a dedicated VRF. Execute this within the namespace of each edge:

```
ip link add vrfa type vrf table 100
ip link set vrfa up
ip link set tohost master vrfa
```

Let's now attempt to ping from `hostleft` to `hostright`. As expected, this should fail:

```
# ping -c1 -W1 192.168.161.2
PING 192.168.161.2 (192.168.161.2) 56(84) bytes of data.
From 192.168.160.2 icmp_seq=1 Destination Net Unreachable

--- 192.168.161.2 ping statistics ---
1 packets transmitted, 0 received, +1 errors, 100% packet loss, time 0ms
```

And the packet captures show no traffic on the transit.

Now, we add the following 2 routes to `edgeleft`:

```
ip r a fd00:168:161::/48 via 2001:db8:168:160::1
ip r a 192.168.161.0/24 encap seg6 mode encap segs fd00:168:161:abcd:: table 100 via inet6 2001:db8:168:160::1
```

The first route isn't strictly necessary since we already have a default route via `2001:db8:168:160::1`. However, we add this route for demonstration purposes.
In SRV6 setups with ISIS and BGP, such a `/48` to the locator prefix would be added by the control plane for each host participating in the overlay tunnels.

```
# ip -6 r
2001:db8:168:160::/64 dev totransit proto kernel metric 256 pref medium
fd00:168:161::/48 via 2001:db8:168:160::1 dev totransit metric 1024 pref medium
fe80::/64 dev totransit proto kernel metric 256 pref medium
default via 2001:db8:168:160::1 dev totransit metric 1024 pref medium
# ip r show table 100
192.168.160.0/24 dev tohost proto kernel scope link src 192.168.160.2 
local 192.168.160.2 dev tohost proto kernel scope host src 192.168.160.2 
broadcast 192.168.160.255 dev tohost proto kernel scope link src 192.168.160.2 
192.168.161.0/24  encap seg6 mode encap segs 1 [ fd00:168:161:abcd:: ] via inet6 2001:db8:168:160::1 dev totransit
```

And we do the same on `edgeright`:

```
ip r a fd00:168:160::/48 via 2001:db8:168:161::1
ip r a 192.168.160.0/24 encap seg6 mode encap segs fd00:168:160:abcd:: table 100 via inet6 2001:db8:168:161::1
```

```
# ip -6 r
2001:db8:168:161::/64 dev totransit proto kernel metric 256 pref medium
fd00:168:160::/48 via 2001:db8:168:161::1 dev totransit metric 1024 pref medium
fe80::/64 dev totransit proto kernel metric 256 pref medium
default via 2001:db8:168:161::1 dev totransit metric 1024 pref medium
# ip r show table 100
192.168.160.0/24  encap seg6 mode encap segs 1 [ fd00:168:160:abcd:: ] via inet6 2001:db8:168:161::1 dev totransit 
192.168.161.0/24 dev tohost proto kernel scope link src 192.168.161.2 
local 192.168.161.2 dev tohost proto kernel scope host src 192.168.161.2 
broadcast 192.168.161.255 dev tohost proto kernel scope link src 192.168.161.2
```

Let's now run our ping again, from `hostleft` to `hostright`:

```
# ping -c1 -W1 192.168.161.2
```

On the left side, we see that the ICMP echo request is encapsulated and sent via the SRV6 tunnel, followed by a `time exceeded in transit` from the router to `edgeright`: 

```
06:31:41.118050 c2:62:80:bc:5e:e8 > 33:33:ff:00:00:01, ethertype IPv6 (0x86dd), length 86: 2001:db8:168:160::2 > ff02::1:ff00:1: ICMP6, neighbor solicitation, who has 2001:db8:168:160::1, length 32
06:31:41.118083 6e:1e:04:f9:11:54 > c2:62:80:bc:5e:e8, ethertype IPv6 (0x86dd), length 86: 2001:db8:168:160::1 > 2001:db8:168:160::2: ICMP6, neighbor advertisement, tgt is 2001:db8:168:160::1, length 32
06:31:41.118094 c2:62:80:bc:5e:e8 > 6e:1e:04:f9:11:54, ethertype IPv6 (0x86dd), length 162: 2001:db8:168:160::2 > fd00:168:161:abcd::: RT6 (len=2, type=4, segleft=0, last-entry=0, tag=0, [0]fd00:168:161:abcd::) 192.168.160.1 > 192.168.161.2: ICMP echo request, id 34206, seq 1, length 64
06:31:41.118252 6e:1e:04:f9:11:54 > c2:62:80:bc:5e:e8, ethertype IPv6 (0x86dd), length 210: 2001:db8:168:161::1 > 2001:db8:168:160::2: ICMP6, time exceeded in-transit for fd00:168:161:abcd::, length 156
```

Looking at the other packet capture, we can understand why we see a `time exceeded in transit`. Since `edgeright` lacks a route for the SRV6 prefix, it sends the packet back to the `transit` node, which then sends it back to `edgeright`, creating a loop until the hop limit is exceeded:

```
06:31:41.118104 46:da:f4:4d:ad:f3 > 33:33:ff:00:00:02, ethertype IPv6 (0x86dd), length 86: fe80::44da:f4ff:fe4d:adf3 > ff02::1:ff00:2: ICMP6, neighbor solicitation, who has 2001:db8:168:161::2, length 32
06:31:41.118117 be:28:4a:5f:fa:ff > 46:da:f4:4d:ad:f3, ethertype IPv6 (0x86dd), length 86: 2001:db8:168:161::2 > fe80::44da:f4ff:fe4d:adf3: ICMP6, neighbor advertisement, tgt is 2001:db8:168:161::2, length 32
06:31:41.118121 46:da:f4:4d:ad:f3 > be:28:4a:5f:fa:ff, ethertype IPv6 (0x86dd), length 162: 2001:db8:168:160::2 > fd00:168:161:abcd::: RT6 (len=2, type=4, segleft=0, last-entry=0, tag=0, [0]fd00:168:161:abcd::) 192.168.160.1 > 192.168.161.2: ICMP echo request, id 34206, seq 1, length 64
06:31:41.118131 be:28:4a:5f:fa:ff > 33:33:ff:00:00:01, ethertype IPv6 (0x86dd), length 86: fe80::bc28:4aff:fe5f:faff > ff02::1:ff00:1: ICMP6, neighbor solicitation, who has 2001:db8:168:161::1, length 32
06:31:41.118137 46:da:f4:4d:ad:f3 > be:28:4a:5f:fa:ff, ethertype IPv6 (0x86dd), length 86: 2001:db8:168:161::1 > fe80::bc28:4aff:fe5f:faff: ICMP6, neighbor advertisement, tgt is 2001:db8:168:161::1, length 32
06:31:41.118143 be:28:4a:5f:fa:ff > 46:da:f4:4d:ad:f3, ethertype IPv6 (0x86dd), length 162: 2001:db8:168:160::2 > fd00:168:161:abcd::: RT6 (len=2, type=4, segleft=0, last-entry=0, tag=0, [0]fd00:168:161:abcd::) 192.168.160.1 > 192.168.161.2: ICMP echo request, id 34206, seq 1, length 64
06:31:41.118145 46:da:f4:4d:ad:f3 > be:28:4a:5f:fa:ff, ethertype IPv6 (0x86dd), length 162: 2001:db8:168:160::2 > fd00:168:161:abcd::: RT6 (len=2, type=4, segleft=0, last-entry=0, tag=0, [0]fd00:168:161:abcd::) 192.168.160.1 > 192.168.161.2: ICMP echo request, id 34206, seq 1, length 64
(... the loop continues ...)
```

The missing piece is configuring routes for the `End.DT46` tunnels on each edge system - on `edgeleft`:

```
# ip route add fd00:168:160:abcd:: encap seg6local action End.DT46 vrftable 100 dev vrfa
```

And on `edgeright`:

```
# ip route add fd00:168:161:abcd:: encap seg6local action End.DT46 vrftable 100 dev vrfa
```

Now, the ping works:

```
[root@metallb ~]#  ping -c1 -W1 192.168.161.2
PING 192.168.161.2 (192.168.161.2) 56(84) bytes of data.
64 bytes from 192.168.161.2: icmp_seq=1 ttl=63 time=0.141 ms

--- 192.168.161.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.141/0.141/0.141/0.000 ms
```

And this can also be seen in the packet captures:

```
06:39:08.204775 c2:62:80:bc:5e:e8 > 6e:1e:04:f9:11:54, ethertype IPv6 (0x86dd), length 162: 2001:db8:168:160::2 > fd00:168:161:abcd::: RT6 (len=2, type=4, segleft=0, last-entry=0, tag=0, [0]fd00:168:161:abcd::) 192.168.160.1 > 192.168.161.2: ICMP echo request, id 34218, seq 1, length 64
06:39:08.204822 6e:1e:04:f9:11:54 > c2:62:80:bc:5e:e8, ethertype IPv6 (0x86dd), length 162: 2001:db8:168:161::2 > fd00:168:160:abcd::: RT6 (len=2, type=4, segleft=0, last-entry=0, tag=0, [0]fd00:168:160:abcd::) 192.168.161.2 > 192.168.160.1: ICMP echo reply, id 34218, seq 1, length 64
```

```
06:39:08.204784 46:da:f4:4d:ad:f3 > be:28:4a:5f:fa:ff, ethertype IPv6 (0x86dd), length 162: 2001:db8:168:160::2 > fd00:168:161:abcd::: RT6 (len=2, type=4, segleft=0, last-entry=0, tag=0, [0]fd00:168:161:abcd::) 192.168.160.1 > 192.168.161.2: ICMP echo request, id 34218, seq 1, length 64
06:39:08.204818 be:28:4a:5f:fa:ff > 46:da:f4:4d:ad:f3, ethertype IPv6 (0x86dd), length 162: 2001:db8:168:161::2 > fd00:168:160:abcd::: RT6 (len=2, type=4, segleft=0, last-entry=0, tag=0, [0]fd00:168:160:abcd::) 192.168.161.2 > 192.168.160.1: ICMP echo reply, id 34218, seq 1, length 64
```

### Understanding SRV6 End.DT46 Tunnels

As demonstrated earlier, in addition to the VRF setup and required sysctl settings, only two routes are needed on each edge node.

Required sysctl settings:

```
sysctl -w net.vrf.strict_mode=1
sysctl -w net.ipv4.conf.all.forwarding=1
sysctl -w net.ipv6.conf.all.forwarding=1
sysctl -w net.ipv6.seg6_flowlabel=1
sysctl -w net.ipv6.conf.all.seg6_enabled=1
```

The route for encapsulation is added to the VRF table (`100` in this case):

```
ip route add 192.168.161.0/24 encap seg6 mode encap segs fd00:168:161:abcd:: table 100 via inet6 2001:db8:168:160::1
```

The route for decapsulation instructs the kernel to forward packets encapsulated in IPv6 Segment Routing headers with a destination of `fd00:168:160:abcd::` to the local table `100` and device `vrfa`:

```
ip route add fd00:168:160:abcd:: encap seg6local action End.DT46 vrftable 100 dev vrfa
```

A detailed look at one of the packets shows that the encapsulation is pretty simple. The packet is sent to the destination segment `fd00:168:161:abcd::` and the IPv6
Next Header is set to `Routing Header for IPv6 (43)`. The IPv6 routing header is of type Segment Routing and the first and only segment is again the segment address `fd00:168:161:abcd::`.
The next header field is set to IPv4 and the next header is already our IPv4 + ICMP payload. The receiving tunnel end point can then decapsulate the payload and forward the inner packet to the destination
inside the VRF:

```
$ tshark -r toedgeleft.pcap -V frame.number==11
Frame 11: Packet, 162 bytes on wire (1296 bits), 162 bytes captured (1296 bits)
(...)
Ethernet II, Src: c2:62:80:bc:5e:e8 (c2:62:80:bc:5e:e8), Dst: 6e:1e:04:f9:11:54 (6e:1e:04:f9:11:54)
(...)
Internet Protocol Version 6, Src: 2001:db8:168:160::2, Dst: fd00:168:161:abcd::
    0110 .... = Version: 6
    .... 0000 0000 .... .... .... .... .... = Traffic Class: 0x00 (DSCP: CS0, ECN: Not-ECT)
        .... 0000 00.. .... .... .... .... .... = Differentiated Services Codepoint: Default (0)
        .... .... ..00 .... .... .... .... .... = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    .... 1000 0010 1111 1100 1000 = Flow Label: 0x82fc8
    Payload Length: 108
    Next Header: Routing Header for IPv6 (43)
    Hop Limit: 63
    Source Address: 2001:db8:168:160::2
        [Address Space: Global Unicast]
        [Special-Purpose Allocation: Documentation]
            [Source: False]
            [Destination: False]
            [Forwardable: False]
            [Globally Reachable: False]
            [Reserved-by-Protocol: False]
    Destination Address: fd00:168:161:abcd::
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    [Stream index: 2]
    Routing Header for IPv6 (Segment Routing)
        Next Header: IPIP (4)
        Length: 2
        [Length: 24 bytes]
        Type: Segment Routing (4)
        Segments Left: 0
        Last Entry: 0
        Flags: 0x00
        Tag: 0000
        Address[0]: fd00:168:161:abcd::
Internet Protocol Version 4, Src: 192.168.160.1, Dst: 192.168.161.2
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Total Length: 84
    Identification: 0x67b6 (26550)
    010. .... = Flags: 0x2, Don't fragment
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    ...0 0000 0000 0000 = Fragment Offset: 0
    Time to Live: 64
    Protocol: ICMP (1)
    Header Checksum: 0x109e [validation disabled]
    [Header checksum status: Unverified]
    Source Address: 192.168.160.1
    Destination Address: 192.168.161.2
    [Stream index: 0]
Internet Control Message Protocol
(...)
```

### IPv6 Payloads

Naturally, we can also encapsulate IPv6 traffic.

To do this on each edge host, we configure new destination routes. For example, on `edgeleft`:

```
ip a a dev tohost fd02::2/64
ip r a fd03::/64  encap seg6 mode encap segs fd00:168:161:abcd:: via inet6 2001:db8:168:160::1 dev totransit
```

And on `edgeright`:

```
ip a a dev tohost fd03::2/64
ip r a fd02::/64  encap seg6 mode encap segs fd00:168:160:abcd:: via inet6 2001:db8:168:161::1 dev totransit
```

On `hostleft`:

```
ip a a dev toedge fd02::1/64
ip -6 r a default via fd02::2
```

And on `hostright`:

```
ip a a dev toedge fd03::1/64
ip -6 r a default via fd03::2
```

We can then ping from `hostright` to `hostleft`:

```
# ping fd02::2
PING fd02::2 (fd02::2) 56 data bytes
64 bytes from fd02::2: icmp_seq=1 ttl=63 time=0.189 ms
```

Everything else remains pretty much the same. Looking at a packet capture, we can see that the SRV6 header now has a `Next Header` of `IPv6` to match the new payload:

```
    Routing Header for IPv6 (Segment Routing)
        Next Header: IPv6 (41)
        Length: 2
        [Length: 24 bytes]
        Type: Segment Routing (4)
        Segments Left: 0
        Last Entry: 0
        Flags: 0x00
        Tag: 0000
        Address[0]: fd00:168:160:abcd::
```

### SRv6 Headend Reduced

As demonstrated earlier, the SRV6 header contains redundant information. Headend Reduced encapsulation addresses this by reducing the Segment Routing Header (SRH) length. This is achieved by excluding the first segment from the Segment ID (SID) list and placing the excluded SID directly in the IPv6 destination address field. With only one segment, reduced encapsulation can eliminate the Segment Routing Header entirely.

Let's try this on the edge nodes for IPv4 payloads. On `edgeleft`, run:

```
ip r d 192.168.161.0/24 encap seg6 mode encap segs fd00:168:161:abcd:: table 100 via inet6 2001:db8:168:160::1
ip r a 192.168.161.0/24 encap seg6 mode encap.red segs fd00:168:161:abcd:: table 100 via inet6 2001:db8:168:160::1
```

On `edgeright`, run:

```
ip r d 192.168.160.0/24 encap seg6 mode encap segs fd00:168:160:abcd:: table 100 via inet6 2001:db8:168:161::1
ip r a 192.168.160.0/24 encap seg6 mode encap.red segs fd00:168:160:abcd:: table 100 via inet6 2001:db8:168:161::1
```

Let's ping from `hostleft` to `hostright`:

```
#  ping -c1 -W1 192.168.161.2
PING 192.168.161.2 (192.168.161.2) 56(84) bytes of data.
64 bytes from 192.168.161.2: icmp_seq=1 ttl=63 time=0.177 ms

--- 192.168.161.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.177/0.177/0.177/0.000 ms
```

A packet capture on interface `toedgeleft` inside the `transit` namespace shows that the Segment Routing Header is now gone:

```
# intf=toedgeleft; tshark -V -i ${intf}
Frame 3: 138 bytes on wire (1104 bits), 138 bytes captured (1104 bits) on interface toedgeleft, id 0
(...)
Ethernet II, Src: c2:62:80:bc:5e:e8 (c2:62:80:bc:5e:e8), Dst: 6e:1e:04:f9:11:54 (6e:1e:04:f9:11:54)
(...)
Internet Protocol Version 6, Src: 2001:db8:168:160::2, Dst: fd00:168:161:abcd::
    0110 .... = Version: 6
    .... 0000 0000 .... .... .... .... .... = Traffic Class: 0x00 (DSCP: CS0, ECN: Not-ECT)
        .... 0000 00.. .... .... .... .... .... = Differentiated Services Codepoint: Default (0)
        .... .... ..00 .... .... .... .... .... = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    .... 1000 0010 1111 1100 1000 = Flow Label: 0x82fc8
    Payload Length: 84
    Next Header: IPIP (4)
    Hop Limit: 63
    Source Address: 2001:db8:168:160::2
        [Address Space: Global Unicast]
        [Special-Purpose Allocation: Documentation]
            [Source: False]
            [Destination: False]
            [Forwardable: False]
            [Globally Reachable: False]
            [Reserved-by-Protocol: False]
    Destination Address: fd00:168:161:abcd::
        [Address Space: Unique Local Unicast]
        [Special-Purpose Allocation: Unique-Local]
            [Source: True]
            [Destination: True]
            [Forwardable: True]
            [Reserved-by-Protocol: False]
    [Stream index: 2]
Internet Protocol Version 4, Src: 192.168.160.1, Dst: 192.168.161.2
(...)
Internet Control Message Protocol
    Type: 8 (Echo (ping) request)
    Code: 0
(...)
```

It's important to note that, as of this writing, FRR's Zebra only programs `encap.red` routes for static routes. However, the BGPD daemon only supports normal `encap`.
See the [FRR source code](https://github.com/FRRouting/frr/blob/afe1ddf1e9f659ddc2aa6bd33402a73cc73fab47/zebra/rt_netlink.c#L1865) and the [FRR documentation](https://docs.frrouting.org/en/latest/static.html#srv6-route-commands).

### Sources

- https://www.segment-routing.net/images/20250311-srv6-usid-linux-netdev-0x19.pdf
- https://onvox.net/2024/12/16/srv6-frr/
- https://datatracker.ietf.org/doc/rfc8986/
- https://github.com/iproute2/iproute2/commit/f1d037ab4ad3d4af7b77ae4f8969d658e04b5d3f
- https://man7.org/linux/man-pages/man8/ip-route.8.html
