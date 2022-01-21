# Accept source routing

**Note:** I could only test this for IPv4. For IPv6, the kernel seems to be hardened by default.

## IPv4

On RHEL, the default settings for IPv4 will look something like this:
~~~
# sysctl -a | grep source_route | grep ipv4
net.ipv4.conf.all.accept_source_route = 0
net.ipv4.conf.br-ext.accept_source_route = 1
net.ipv4.conf.br-vxlan.accept_source_route = 1
net.ipv4.conf.br0.accept_source_route = 1
net.ipv4.conf.default.accept_source_route = 1
net.ipv4.conf.eth0.accept_source_route = 1
net.ipv4.conf.eth1.accept_source_route = 1
net.ipv4.conf.lo.accept_source_route = 1
net.ipv4.conf.ovs-system.accept_source_route = 1
net.ipv4.conf.vxlan_sys_4789.accept_source_route = 1
~~~

`net.ipv4.conf.all.accept_source_route = 0` will override anything else.

The documentation clearly states that this is a logical AND relationship:
[https://www.kernel.org/doc/Documentation/networking/ip-sysctl.txt](https://www.kernel.org/doc/Documentation/networking/ip-sysctl.txt)
~~~
accept_source_route - BOOLEAN
	Accept packets with SRR option.
	conf/all/accept_source_route must also be set to TRUE to accept packets
	with SRR option on the interface
	default TRUE (router)
		FALSE (host)
~~~

Having a look at the kernel code:
* [https://github.com/torvalds/linux/blob/8efd0d9c316af470377894a6a0f9ff63ce18c177/include/linux/inetdevice.h#L103](https://github.com/torvalds/linux/blob/8efd0d9c316af470377894a6a0f9ff63ce18c177/include/linux/inetdevice.h#L103)
* [https://github.com/torvalds/linux/blob/8efd0d9c316af470377894a6a0f9ff63ce18c177/include/linux/inetdevice.h#L83](https://github.com/torvalds/linux/blob/8efd0d9c316af470377894a6a0f9ff63ce18c177/include/linux/inetdevice.h#L83)
* [https://github.com/torvalds/linux/blob/8efd0d9c316af470377894a6a0f9ff63ce18c177/include/linux/inetdevice.h#L56](https://github.com/torvalds/linux/blob/8efd0d9c316af470377894a6a0f9ff63ce18c177/include/linux/inetdevice.h#L56)

The actual drop for IPv4 is happening here:
* [https://github.com/torvalds/linux/blob/5bfc75d92efd494db37f5c4c173d3639d4772966/net/ipv4/ip_input.c#L293](https://github.com/torvalds/linux/blob/5bfc75d92efd494db37f5c4c173d3639d4772966/net/ipv4/ip_input.c#L293)

We should be able to see what's happening if we enable martian logging:
* [https://github.com/torvalds/linux/blob/5bfc75d92efd494db37f5c4c173d3639d4772966/net/ipv4/ip_input.c#L290](https://github.com/torvalds/linux/blob/5bfc75d92efd494db37f5c4c173d3639d4772966/net/ipv4/ip_input.c#L290)

### Test setup

I tested with forwarding on:
~~~
[root@centos84 ~]# sysctl -a | grep ip_forward
net.ipv4.ip_forward = 1
net.ipv4.ip_forward_update_priority = 1
net.ipv4.ip_forward_use_pmtu = 0
[root@centos84 ~]# sysctl -a | grep forward | grep eth0
net.ipv4.conf.eth0.bc_forwarding = 0
net.ipv4.conf.eth0.forwarding = 1
net.ipv4.conf.eth0.mc_forwarding = 0
net.ipv6.conf.eth0.forwarding = 0
net.ipv6.conf.eth0.mc_forwarding = 0
~~~

The traceroute command `traceroute -I 192.168.122.6 -g 192.168.122.6` that I'm using below yields the following IPv4 header + ICMP message:
~~~
(...)
Internet Protocol Version 4, Src: 192.168.122.1, Dst: 192.168.122.6, Via: 192.168.122.6
    0100 .... = Version: 4
    .... 1000 = Header Length: 32 bytes (8)
    Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..00 = Explicit Congestion Notification: Not ECN-Capable Transport (0)
    Total Length: 72
    Identification: 0x8307 (33543)
    Flags: 0x0000
        0... .... .... .... = Reserved bit: Not set
        .0.. .... .... .... = Don't fragment: Not set
        ..0. .... .... .... = More fragments: Not set
        ...0 0000 0000 0000 = Fragment offset: 0
    Time to live: 6
    Protocol: ICMP (1)
    Header checksum: 0x3770 [validation disabled]
    [Header checksum status: Unverified]
    Source: 192.168.122.1
    Current Route: 192.168.122.6
    Options: (12 bytes), Loose Source Route
        IP Option - No-Operation (NOP)
            Type: 1
                0... .... = Copy on fragmentation: No
                .00. .... = Class: Control (0)
                ...0 0001 = Number: No-Operation (NOP) (1)
        IP Option - Loose Source Route (11 bytes)
            Type: 131
                1... .... = Copy on fragmentation: Yes
                .00. .... = Class: Control (0)
                ...0 0011 = Number: Loose source route (3)
            Length: 11
            Pointer: 4
            Source Route: 192.168.122.6 <- (next)
            Destination: 192.168.122.6
Internet Control Message Protocol
    Type: 8 (Echo (ping) request)
    Code: 0
    Checksum: 0x822c [correct]
    [Checksum Status: Good]
    Identifier (BE): 62 (0x003e)
    Identifier (LE): 15872 (0x3e00)
    Sequence number (BE): 16 (0x0010)
    Sequence number (LE): 4096 (0x1000)
    Data (32 bytes)

0000  48 49 4a 4b 4c 4d 4e 4f 50 51 52 53 54 55 56 57   HIJKLMNOPQRSTUVW
0010  58 59 5a 5b 5c 5d 5e 5f 60 61 62 63 64 65 66 67   XYZ[\]^_`abcdefg
        Data: 48494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f...
        [Length: 32]
~~~

### Testing in a live systems - source routing disabled

Here's how this can be tested, on my CentOS 8.4 system:
~~~
[root@centos84 ~]# ip a | grep 192.168.122.6
    inet 192.168.122.6/24 brd 192.168.122.255 scope global dynamic noprefixroute eth0
[root@centos84 ~]# sysctl -a | grep source_route | grep ipv4
net.ipv4.conf.all.accept_source_route = 0
net.ipv4.conf.br-ext.accept_source_route = 1
net.ipv4.conf.br-vxlan.accept_source_route = 1
net.ipv4.conf.br0.accept_source_route = 1
net.ipv4.conf.default.accept_source_route = 1
net.ipv4.conf.eth0.accept_source_route = 1
net.ipv4.conf.eth1.accept_source_route = 1
net.ipv4.conf.lo.accept_source_route = 1
net.ipv4.conf.ovs-system.accept_source_route = 1
net.ipv4.conf.vxlan_sys_4789.accept_source_route = 1
[root@centos84 ~]# dropwatch -lkas
Initalizing kallsyms db
dropwatch> start
Enabling monitoring...
Kernel monitoring activated.
Issue Ctrl-C to stop monitoring
(...)
~~~

Now, go to a second CLI, and run:
~~~
[akaris@linux ~]$ traceroute -I 192.168.122.6 -g 192.168.122.6
traceroute to 192.168.122.6 (192.168.122.6), 30 hops max, 72 byte packets^C
[akaris@linux ~]$ timeout 5 traceroute -I 192.168.122.6 -g 192.168.122.6
traceroute to 192.168.122.6 (192.168.122.6), 30 hops max, 72 byte packets
[akaris@linux ~]$ 
[akaris@linux ~]$ timeout 5 traceroute -I 192.168.122.6 -g 192.168.122.6
traceroute to 192.168.122.6 (192.168.122.6), 30 hops max, 72 byte packets
[akaris@linux ~]$ 
~~~

As you can see, the traceroute will fail, and dropwatch on the first CLI will report 16 drops
from the traceroute in ip_rcv_finish:
~~~
(...)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
16 drops at ip_rcv_finish+20d (0xffffffff8625274d)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
16 drops at ip_rcv_finish+20d (0xffffffff8625274d)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
16 drops at ip_rcv_finish+20d (0xffffffff8625274d)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
^CGot a stop message
dropwatch> stop
~~~

You can also enable log martians (yes, *this* one is OR'ed, ironically) ...
[https://www.kernel.org/doc/Documentation/networking/ip-sysctl.txt](https://www.kernel.org/doc/Documentation/networking/ip-sysctl.txt)
~~~
log_martians - BOOLEAN
	Log packets with impossible addresses to kernel log.
	log_martians for the interface will be enabled if at least one of
	conf/{all,interface}/log_martians is set to TRUE,
	it will be disabled otherwise
~~~

And upon source route drop, one will be able to see messages containing string `IPv4: source route option` in the logs:
~~~
[root@centos84 ~]# sysctl -w net.ipv4.conf.all.log_martians=1
net.ipv4.conf.all.log_martians = 1
[root@centos84 ~]# sysctl -a | grep martians
net.ipv4.conf.all.log_martians = 1
net.ipv4.conf.br-ext.log_martians = 0
net.ipv4.conf.br-vxlan.log_martians = 0
net.ipv4.conf.br0.log_martians = 0
net.ipv4.conf.default.log_martians = 0
net.ipv4.conf.eth0.log_martians = 0
net.ipv4.conf.eth1.log_martians = 0
net.ipv4.conf.lo.log_martians = 0
net.ipv4.conf.ovs-system.log_martians = 0
net.ipv4.conf.vxlan_sys_4789.log_martians = 0
[root@centos84 ~]# sysctl -a | grep source_route | grep ipv4
net.ipv4.conf.all.accept_source_route = 0
net.ipv4.conf.br-ext.accept_source_route = 1
net.ipv4.conf.br-vxlan.accept_source_route = 1
net.ipv4.conf.br0.accept_source_route = 1
net.ipv4.conf.default.accept_source_route = 1
net.ipv4.conf.eth0.accept_source_route = 1
net.ipv4.conf.eth1.accept_source_route = 1
net.ipv4.conf.lo.accept_source_route = 1
net.ipv4.conf.ovs-system.accept_source_route = 1
net.ipv4.conf.vxlan_sys_4789.accept_source_route = 1
[root@centos84 ~]# journalctl -f
(... sending source route packets here ...)

Jan 21 10:01:01 centos84 CROND[17134]: (root) CMD (run-parts /etc/cron.hourly)
Jan 21 10:01:01 centos84 run-parts[17137]: (/etc/cron.hourly) starting 0anacron
Jan 21 10:01:01 centos84 anacron[17143]: Anacron started on 2022-01-21
Jan 21 10:01:01 centos84 run-parts[17145]: (/etc/cron.hourly) finished 0anacron
Jan 21 10:01:01 centos84 anacron[17143]: Will run job `cron.daily' in 21 min.
Jan 21 10:01:01 centos84 anacron[17143]: Jobs will be executed sequentially
Jan 21 10:01:01 centos84 kernel: net_ratelimit: 6 callbacks suppressed
Jan 21 10:01:01 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:01 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:01 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:01 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:01 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:01 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:01 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:01 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:01 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:01 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6

(... sending source route packets here ...)

Jan 21 10:01:08 centos84 kernel: net_ratelimit: 6 callbacks suppressed
Jan 21 10:01:08 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:08 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:08 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:08 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:08 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:08 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:08 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:08 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:08 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
Jan 21 10:01:08 centos84 kernel: IPv4: source route option 192.168.122.1 -> 192.168.122.6
^C
~~~

### Testing in a live systems - source routing enabled

Now, I tested enabling source routing with `net.ipv4.conf.all.accept_source_route = 1`:
~~~
[root@centos84 ~]# sysctl -w net.ipv4.conf.all.accept_source_route=1
net.ipv4.conf.all.accept_source_route = 1
[root@centos84 ~]# sysctl -a | grep source_route | grep ipv4
net.ipv4.conf.all.accept_source_route = 1
net.ipv4.conf.br-ext.accept_source_route = 1
net.ipv4.conf.br-vxlan.accept_source_route = 1
net.ipv4.conf.br0.accept_source_route = 1
net.ipv4.conf.default.accept_source_route = 1
net.ipv4.conf.eth0.accept_source_route = 1
net.ipv4.conf.eth1.accept_source_route = 1
net.ipv4.conf.lo.accept_source_route = 1
net.ipv4.conf.ovs-system.accept_source_route = 1
net.ipv4.conf.vxlan_sys_4789.accept_source_route = 1
[root@centos84 ~]# dropwatch -lkas
Initalizing kallsyms db
dropwatch> start
Enabling monitoring...
Kernel monitoring activated.
Issue Ctrl-C to stop monitoring
(...)
~~~

Now, my traceroute with the source route option set works:
~~~
[akaris@linux ~]$ timeout 5 traceroute -I 192.168.122.6 -g 192.168.122.6
traceroute to 192.168.122.6 (192.168.122.6), 30 hops max, 72 byte packets
 1  centos84 (192.168.122.6)  0.526 ms  0.509 ms  0.494 ms
[akaris@linux ~]$ timeout 5 traceroute -I 192.168.122.6 -g 192.168.122.6
traceroute to 192.168.122.6 (192.168.122.6), 30 hops max, 72 byte packets
 1  centos84 (192.168.122.6)  0.459 ms  0.433 ms  0.425 ms
[akaris@linux ~]$ timeout 5 traceroute -I 192.168.122.6 -g 192.168.122.6
traceroute to 192.168.122.6 (192.168.122.6), 30 hops max, 72 byte packets
 1  centos84 (192.168.122.6)  0.424 ms  0.388 ms  0.387 ms
[akaris@linux ~]$ timeout 5 traceroute -I 192.168.122.6 -g 192.168.122.6
traceroute to 192.168.122.6 (192.168.122.6), 30 hops max, 72 byte packets
 1  centos84 (192.168.122.6)  0.475 ms  0.460 ms  0.455 ms
[akaris@linux ~]$ timeout 5 traceroute -I 192.168.122.6 -g 192.168.122.6
traceroute to 192.168.122.6 (192.168.122.6), 30 hops max, 72 byte packets
 1  centos84 (192.168.122.6)  0.571 ms  0.546 ms  0.538 ms
[akaris@linux ~]$ 
~~~

And dropwatch only shows unrelated drops elsewhere:
~~~
(...)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
2 drops at __init_scratch_end+37b6e440 (0xffffffffc076e440)
^CGot a stop message
dropwatch> 
~~~

And the kernel with martian logging on does not report anything.

## IPv6

For IPv6, the default settings on RHEL for source routing will yield something like this:
~~~
# sysctl -a | grep source_route | grep ipv6
net.ipv6.conf.all.accept_source_route = 0
net.ipv6.conf.br-ext.accept_source_route = 0
net.ipv6.conf.br-vxlan.accept_source_route = 0
net.ipv6.conf.br0.accept_source_route = 0
net.ipv6.conf.default.accept_source_route = 0
net.ipv6.conf.eth0.accept_source_route = 0
net.ipv6.conf.eth1.accept_source_route = 0
net.ipv6.conf.lo.accept_source_route = 0
net.ipv6.conf.ovs-system.accept_source_route = 0
net.ipv6.conf.vxlan_sys_4789.accept_source_route = 0
~~~

For IPv6, I tried crafting RH0 type packets, but failed miserably at the task. With that said, the kernel only ever accepts RH2 and the default is off, anyway: 
[https://www.kernel.org/doc/Documentation/networking/ip-sysctl.txt(https://www.kernel.org/doc/Documentation/networking/ip-sysctl.txt:)
~~~
accept_source_route - INTEGER
	Accept source routing (routing extension header).

	>= 0: Accept only routing header type 2.
	< 0: Do not accept routing header.

	Default: 0
~~~
It looks like this was merged 14 years ago, so we should be good ;-) [https://github.com/torvalds/linux/commit/bb4dbf9e61d0801927e7df2569bb3dd8287ea301](https://github.com/torvalds/linux/commit/bb4dbf9e61d0801927e7df2569bb3dd8287ea301)
The following article on LWN might be of interest, too: [https://lwn.net/Articles/232781/](https://lwn.net/Articles/232781/)

But then, a more thorough test could use scapy or some other tool to actually craft a corret packet.

For IPv6 code, the relevant part might be here, but I have a very limited understanding of the kernel code:
[https://github.com/torvalds/linux/blob/8efd0d9c316af470377894a6a0f9ff63ce18c177/net/ipv6/exthdrs.c#L687-690](https://github.com/torvalds/linux/blob/8efd0d9c316af470377894a6a0f9ff63ce18c177/net/ipv6/exthdrs.c#L687-690)

