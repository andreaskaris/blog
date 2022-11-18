## tcpdump expressions and BPF

### Introduction

I sometimes used to find myself in situations where tcpdump's filters seemingly did not work the way that I expected them to. In those situations I often simply ran tcpdump in line buffered mode (`-l`) and piped the output into `grep` to find what I was looking for. Particularly the VLAN filter used to give me some headaches with older versions of tcpdump and libpcap. In this article, I will try to shed some light at how tcpdump (via libpcap) generates bytecode for filtering packets. Understanding how the bytecode is generated and how it works should help you understand when things go wrong and when a filter expression might not match a specific packet that you are looking for.

This article extends upon the introduction into tcpdump's BPF compiler provided in the blog post [BPF - the forgotten bytecode](https://blog.cloudflare.com/bpf-the-forgotten-bytecode/){target=_blank}. Before reading on, read through this resource, then come back here. The blog post also links to the paper [The BSD Packet Filter: A New Architecture for User-level Packet Capture](https://www.tcpdump.org/papers/bpf-usenix93.pdf){target=_blank} if you want to further deepen your understanding. The [Linux Socket Filtering aka Berkeley Packet Filter (BPF)](https://www.kernel.org/doc/Documentation/networking/filter.txt){target=_blank} kernel documentation might come in handy when following the examples below. Please also note that when I refer to BPF in this article, I refer to classic BPF and not to eBPF.

### Compiling a basic BPF expression

After reading through the aforementioned material, you should already have an understanding of tcpdump's BPF compiler. Any expression that you give to tcpdump will be compiled into bytecode which in turn will be given to the kernel's JIT compiler. Let's use an easy example and look at the `icmp` filter.

First, we will capture on the loopback interface and ping `127.0.0.1` at the same time:
~~~
[root@host ~]# timeout 1 tcpdump -i lo -w lo.pcap
dropped privs to tcpdump
tcpdump: listening on lo, link-type EN10MB (Ethernet), capture size 262144 bytes
0 packets captured
6 packets received by filter
0 packets dropped by kernel
~~~

Let's look at one of the captured packets. Each visual block in the hexdump is a halfword, or 16 bits. Offsets in tcpdump are specified in Bytes.
The IP ethertype is 0x800 and can be found in Bytes 12 and 13, the ICMP protocol number for ICMP is 0x01 and can be found in Byte 23:
~~~
[root@host ~]# tcpdump -r lo.pcap -xx
reading from file lo.pcap, link-type EN10MB (Ethernet)
dropped privs to tcpdump
11:50:13.451990 IP localhost > localhost: ICMP echo request, id 1, seq 1, length 64

                                            | -- ethertype for IP (0800)
                                            |
                                            v
	0x0000:  0000 0000 0000 0000 0000 0000 0800 4500
	0x0010:  0054 17ee 4000 4001 24b9 7f00 0001 7f00
                               ^
                               | --- IP protocol number for ICMP (01)

	0x0020:  0001 0800 947c 0001 0001 4566 7663 0000
	0x0030:  0000 e2e4 0600 0000 0000 1011 1213 1415
	0x0040:  1617 1819 1a1b 1c1d 1e1f 2021 2223 2425
	0x0050:  2627 2829 2a2b 2c2d 2e2f 3031 3233 3435
	0x0060:  3637
~~~

And here is how tcpdump compiles the `icmp` filter expression for an interface for a live capture:
~~~
[root@host ~]# tcpdump -i lo -d icmp
(000) ldh      [12]
(001) jeq      #0x800           jt 2	jf 5             # <---- ethertype for IP
(002) ldb      [23]
(003) jeq      #0x1             jt 4	jf 5             # <---- IP protocol number for ICMP
(004) ret      #262144
(005) ret      #0
~~~

We'd also expect this to be the same when we look at our capture file:
~~~
[root@host ~]# tcpdump -r lo.pcap -d icmp
reading from file lo.pcap, link-type EN10MB (Ethernet)
(000) ldh      [12]
(001) jeq      #0x800           jt 2	jf 5             # <---- ethertype for IP
(002) ldb      [23]
(003) jeq      #0x1             jt 4	jf 5             # <---- IP protocol number for ICMP
(004) ret      #262144
(005) ret      #0
~~~

So far, so good and straight forward. We look at Byte 23 and check if the IP protocol number is 1. Before that, we check the ethernet header in Byte 12 and make sure that we are using IP.

### Differences when filtering on the wire versus reading from a packet capture file

Depending on the tcpdump version in use, your provided expression might be compiled slightly differently. What's more, there is a difference between the bytecode that is compiled for live packet captures on interfaces and for the bytecode that is compiled for filtering packet capture files. Earlier, we looked at the `icmp` filter. We will now see what happens when we filter for a VLAN number with the `vlan` filter.

The [Wikipedia article about EtherTypes](https://en.wikipedia.org/wiki/EtherType){target=_blank} lists the EtherTypes for VLANS: 
for 802.1q the value is `0x8100`, for Q-in-Q we would expect to see `0x88A8` and for double tagging `0x9100`. We expect
to find one of these values in Bytes 12 and 13 of the Ethernet header.
And when we apply the filter to our previously captured file, this is precisely what the expression is compiled to. Tcpdump first loads the halfword at Bytes 12 and 13. Then it sequentially checks if the value matches either of the known EtherTypes for VLAN:
~~~
[root@host ~]# tcpdump -r lo.pcap -d vlan
reading from file lo.pcap, link-type EN10MB (Ethernet)
(000) ldh      [12]
(001) jeq      #0x8100          jt 4	jf 2    # <---- ethertype for 802.1q
(002) jeq      #0x88a8          jt 4	jf 3    # <---- ethertype for Q-in-Q
(003) jeq      #0x9100          jt 4	jf 5    # <---- ethertype for double tagging
(004) ret      #262144
(005) ret      #0
~~~

Now, let's look at a live capture from the loopback interface. We should expect the same bytecode, but instead we see the following:
~~~
[root@host ~]# tcpdump -i lo -d vlan
(000) ldb      [-4048]
(001) jeq      #0x1             jt 6	jf 2
(002) ldh      [12]
(003) jeq      #0x8100          jt 6	jf 4
(004) jeq      #0x88a8          jt 6	jf 5
(005) jeq      #0x9100          jt 6	jf 7
(006) ret      #262144
(007) ret      #0
~~~

The expression is different now! Why is that? Well, the kernel has an internal optimization and will actually store VLAN and other ancillary information in a location with negative a offset.

The reason for this is that the kernel no longer passes specific information as-is to libpcap. See [https://bugs.launchpad.net/ubuntu/+source/tcpdump/+bug/1641429](https://bugs.launchpad.net/ubuntu/+source/tcpdump/+bug/1641429){target=_blank} for further details:
~~~
The kernel now longer passes vlan tag information as-is to libpcap, instead BPF needs to access ancillary data.

That is the reason "vlan 114" works (because it does the right thing), and a manual filter doesn't, because libpcap never actually sees this.

Offsets are negative because this is the way to access this ancillary data (like vlan tags) in the Linux kernel:
https://github.com/torvalds/linux/blob/6f0d349d922ba44e4348a17a78ea51b7135965b1/include/uapi/linux/filter.h#L60
~~~

And see the kernel documentation as well: [https://github.com/torvalds/linux/blob/7acac4b3196caee5e21fb5ea53f8bc124e6a16fc/include/uapi/linux/filter.h#L60](https://github.com/torvalds/linux/blob/7acac4b3196caee5e21fb5ea53f8bc124e6a16fc/include/uapi/linux/filter.h#L60){target=_blank}:

~~~
/* RATIONALE. Negative offsets are invalid in BPF.
   We use them to reference ancillary data.
   Unlike introduction new instructions, it does not break
   existing compilers/optimizers.
 */
#define SKF_AD_OFF    (-0x1000)
 (...)
#define SKF_AD_VLAN_TAG	44
#define SKF_AD_VLAN_TAG_PRESENT 48
 (...)
~~~

According to the kernel code, we expect to find an indication that a VLAN tag is present at the following location: 

* `-0x1000 + 48 = - 4096 + 48 = −4048`

The VLAN number is stored at location:

* `-0x1000 + 44 = - 4096 + 44 = −4052`

For example, let's look for VLAN 32 (0x20) in our packet capture file. We first verify if we find a VLAN ethertype in
Bytes 12 and 13. If so, we load Bytes 14 and 15 and extract the last 12 bits (the VID). We then check if those match 0x20:
~~~
[root@host ~]# tcpdump -r lo.pcap -d vlan 32
reading from file lo.pcap, link-type EN10MB (Ethernet)
(000) ldh      [12]                                  # <---- look for dot1q, etc. at Byte 12
(001) jeq      #0x8100          jt 4	jf 2         # <---- ethertype for 802.1q 
(002) jeq      #0x88a8          jt 4	jf 3         # <---- ethertype for Q-in-Q
(003) jeq      #0x9100          jt 4	jf 8         # <---- ethertype for double tagging
(004) ldh      [14]                                  # <---- load Bytes 14 and 15
(005) and      #0xfff                                # <---- extract the last 12 bits from the field (VID)
(006) jeq      #0x20            jt 7	jf 8         # <---- check for VLAN 32
(007) ret      #262144
(008) ret      #0
~~~

That, too, makes sense. But if we do a live capture, the logic becomes more complex. We first check at location
`SKF_AD_VLAN_TAG_PRESENT` (-4048) to find out if a VLAN is present. If true, we load location `SKF_AD_VLAN_TAG` (-4052) and
extract the last 12 bits from there and then check if the value equals 0x20. Otherwise, we check Bytes 12 and 14 like above:
~~~
[root@host ~]# tcpdump -i lo vlan 32 -d
(000) ldb      [-4048]                             # <---- load value at location SKF_AD_VLAN_TAG_PRESENT
(001) jeq      #0x1             jt 6	jf 2       # <---- if the value is true, check SKF_AD_VLAN_TAG, otherwise read in Byte 12
(002) ldh      [12]                                # <---- load the content at Byte 12
(003) jeq      #0x8100          jt 6	jf 4
(004) jeq      #0x88a8          jt 6	jf 5
(005) jeq      #0x9100          jt 6	jf 14
(006) ldb      [-4048]                             # <---- does the kernel special field report this as a VLAN?
(007) jeq      #0x1             jt 8	jf 10
(008) ldb      [-4052]                             # <---- load value at location SKF_AD_VLAN_TAG
(009) ja       11                                  # <---- jump to instruction 11
(010) ldh      [14]
(011) and      #0xfff                              # <---- extract the last 12 bits from the field (VID)
(012) jeq      #0x20            jt 13	jf 14      # <---- look for VLAN 32 as earlier
(013) ret      #262144
(014) ret      #0
~~~

As a conclusion:

* Data from a `.pcap` file is treated as if everything came directly from the wire, the offsets are the same as we'd see them on the physical layer.
* When we capture data from an interface, libpcap will use kernel ancillary data but it will also add a fallback expression in newer versions.

### How does tcpdump compile user provided expressions - Source code analysis

Let's look at how tcpdump (by means of libpcap) compiles its expressions into the appropriate BPF bytecode.

tcpdump will compile different types of BPF depending on if the optimization flag is set but also particularly depending on if we open a file for reading or if we run a live capture.

The `-r` indicates to read from a file and the parameter is read here:

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L1759](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L1759){target=_blank}

~~~
		case 'r':
			RFileName = optarg;
			break;
~~~

Opening a file for reading happens here:

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2053](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2053){target=_blank}
~~~
#ifdef HAVE_PCAP_SET_TSTAMP_PRECISION
		pd = pcap_open_offline_with_tstamp_precision(RFileName,
		    ndo->ndo_tstamp_precision, ebuf);
#else
		pd = pcap_open_offline(RFileName, ebuf);
#endif
~~~

If `-r` is not specified, then libpcap will listen on the interface instead:

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2135](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2135){target=_blank}
~~~
		/*
		 * Try to open the interface with the specified name.
		 */
		pd = open_interface(device, ndo, ebuf);
~~~

How the BPF will be compiled depends on if a file was used as an input or if tcpdump reads from a socket on a network interface. The actual compilation of the user provided filter expression to BPF happens here:

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2238](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2238){target=_blank}
~~~
	if (pcap_compile(pd, &fcode, cmdbuf, Oflag, netmask) < 0)
		error("%s", pcap_geterr(pd));
~~~

If the `-d` parameter (and thus the `dflag`) is set, tcpdump will call libpcap's `bpf_dump` function to print the expression in one of three formats:

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L1595](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L1595){target=_blank}
~~~
		case 'd':
			++dflag;
			break;
~~~

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2240](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2240){target=_blank}
~~~
	if (dflag) {
		bpf_dump(&fcode, dflag);
		pcap_close(pd);
		free(cmdbuf);
		pcap_freecode(&fcode);
		exit_tcpdump(S_SUCCESS);
	}
~~~

The different dump formats are explained in the man page:
~~~
man tcpdump
(...)
       -d     Dump the compiled packet-matching code in a human readable form to standard output and stop.

              Please  mind  that  although code compilation is always DLT-specific, typically it is impossible (and
              unnecessary) to specify which DLT to use for the dump because tcpdump uses either the DLT of the  in‐
              put  pcap  file  specified with -r, or the default DLT of the network interface specified with -i, or
              the particular DLT of the network interface specified with -y and -i respectively. In these cases the
              dump shows the same exact code that would filter the input file or the network interface without -d.

              However, when neither -r nor -i is specified, specifying -d prevents tcpdump from guessing a suitable
              network interface (see -i).  In this case the DLT defaults to EN10MB and can be set to another  valid
              value manually with -y.

       -dd    Dump packet-matching code as a C program fragment.

       -ddd   Dump packet-matching code as decimal numbers (preceded with a count).
(...)
~~~

### Building a filter expression compiler

#### Documentation

In order to build a filter expression compiler, we look at the source code:

* [https://github.com/the-tcpdump-group/tcpdump/blob/master/tcpdump.c#L2238](https://github.com/the-tcpdump-group/tcpdump/blob/master/tcpdump.c#L2238){target=_blank}

And at this documentation here:

* [https://www.tcpdump.org/pcap.html](https://www.tcpdump.org/pcap.html){target=_blank}

#### Installing dependencies

Install build dependencies. For example, on RHEL 8:
~~~
subscription-manager repos --enable=codeready-builder-for-rhel-8-x86_64-rpms 
yum install libpcap-devel -y
~~~

#### Creating the code

Now, create the following code:
~~~
cat <<'EOF' > libpcap_expression_compiler.c
// From:
// https://github.com/the-tcpdump-group/tcpdump/blob/master/tcpdump.c#L2238
// https://www.tcpdump.org/pcap.html
// https://git.netfilter.org/iptables/tree/utils/nfbpf_compile.c

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>

/*
 * Concatenate all input arguments into a long string
 */
char * concat_args(int argc, char **argv) {
    char * arg_string;
    int len;

    for(int i=1; i<argc; i++) {
        len += strlen(argv[i]);
    }
    arg_string = (char *)malloc(len+argc-1);

    for (int i=1; i < argc; i++) {
        if(i != 1) {
            strcat(arg_string, " ");
        }
        strcat(arg_string, argv[i]);
    }
    return arg_string;
}

int main(int argc, char **argv) {
    pcap_t *handle;
    char dev[] = "lo";
    char errbuf[PCAP_ERRBUF_SIZE];
    struct bpf_program fcode;
    bpf_u_int32 localnet = 0, netmask = 0;
    int Oflag = 1;

    char * filter_exp = concat_args(argc, argv);
    if(argc <= 1) {
        fprintf(stderr, "Empty filter expression\n");
        exit(1);
    }
    printf("Compiling expression '%s'\n\n", filter_exp);

    if (pcap_lookupnet(dev, &localnet, &netmask, errbuf) == -1) {
        fprintf(stderr, "Can't get netmask for device %s\n", dev);
        localnet = 0;
        netmask = 0;
    }
    handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return(2);
    }
    if (pcap_compile(handle, &fcode, filter_exp, Oflag, netmask) < 0) {
        fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
         return(2);
    }

    printf("*** Dump compiled packet-matching code (-d) ***\n\n");
    bpf_dump(&fcode, 1);
    printf("\n*** Dump packet-matching code as a C program fragment (-dd) ***\n\n");
    bpf_dump(&fcode, 2);
    printf("\n*** Dump packet-matching code as bytecode (-ddd) ***\n\n");
    bpf_dump(&fcode, 3);

    // https://git.netfilter.org/iptables/tree/utils/nfbpf_compile.c
    printf("\n*** Dump packet-matching code as bytecode (on one line) ***\n\n");
    struct bpf_insn *ins;
    ins = fcode.bf_insns;

    printf("%d,", fcode.bf_len);
    for (int i = 0; i < fcode.bf_len-1; ++ins, ++i) {
    printf("%u %u %u %u,", ins->code, ins->jt, ins->jf, ins->k);
    }
    printf("%u %u %u %u\n", ins->code, ins->jt, ins->jf, ins->k);

    pcap_close(handle);
    free(filter_exp);
    pcap_freecode(&fcode);

    exit(0);
}
EOF
~~~

#### Compiling the code

And compile it:
~~~
gcc libpcap_expression_compiler.c -o libpcap_expression_compiler -lpcap
~~~

#### Using the tool

The tool will now produce the same output as tcpdump with the `-d`, `-dd` or `-ddd` flag set. In order to do so, it will attach to the loopback interface, so the binary must be run with sufficient privileges.

~~~
# ./libpcap_expression_compiler vlan 32
Compiling expression 'vlan 32'

*** Dump compiled packet-matching code (-d) ***

(000) ldb      [-4048]
(001) jeq      #0x1             jt 6	jf 2
(002) ldh      [12]
(003) jeq      #0x8100          jt 6	jf 4
(004) jeq      #0x88a8          jt 6	jf 5
(005) jeq      #0x9100          jt 6	jf 14
(006) ldb      [-4048]
(007) jeq      #0x1             jt 8	jf 10
(008) ldb      [-4052]
(009) ja       11
(010) ldh      [14]
(011) and      #0xfff
(012) jeq      #0x20            jt 13	jf 14
(013) ret      #8192
(014) ret      #0

*** Dump packet-matching code as a C program fragment (-dd) ***

{ 0x30, 0, 0, 0xfffff030 },
{ 0x15, 4, 0, 0x00000001 },
{ 0x28, 0, 0, 0x0000000c },
{ 0x15, 2, 0, 0x00008100 },
{ 0x15, 1, 0, 0x000088a8 },
{ 0x15, 0, 8, 0x00009100 },
{ 0x30, 0, 0, 0xfffff030 },
{ 0x15, 0, 2, 0x00000001 },
{ 0x30, 0, 0, 0xfffff02c },
{ 0x5, 0, 0, 0x00000001 },
{ 0x28, 0, 0, 0x0000000e },
{ 0x54, 0, 0, 0x00000fff },
{ 0x15, 0, 1, 0x00000020 },
{ 0x6, 0, 0, 0x00002000 },
{ 0x6, 0, 0, 0x00000000 },

*** Dump packet-matching code as bytecode (-ddd) ***

15
48 0 0 4294963248
21 4 0 1
40 0 0 12
21 2 0 33024
21 1 0 34984
21 0 8 37120
48 0 0 4294963248
21 0 2 1
48 0 0 4294963244
5 0 0 1
40 0 0 14
84 0 0 4095
21 0 1 32
6 0 0 8192
6 0 0 0

*** Dump packet-matching code as bytecode (on one line) ***

15,48 0 0 4294963248,21 4 0 1,40 0 0 12,21 2 0 33024,21 1 0 34984,21 0 8 37120,48 0 0 4294963248,21 0 2 1,48 0 0 4294963244,5 0 0 1,40 0 0 14,84 0 0 4095,21 0 1 32,6 0 0 8192,6 0 0 0
~~~

Or let's create another filter expression, just as another example:
~~~
# ./libpcap_expression_compiler host 127.0.0.254 and tcp and port 5353
Compiling expression 'host 127.0.0.254 and tcp and port 5353'

*** Dump compiled packet-matching code (-d) ***

(000) ldh      [12]
(001) jeq      #0x800           jt 2	jf 16
(002) ld       [26]
(003) jeq      #0x7f0000fe      jt 6	jf 4
(004) ld       [30]
(005) jeq      #0x7f0000fe      jt 6	jf 16
(006) ldb      [23]
(007) jeq      #0x6             jt 8	jf 16
(008) ldh      [20]
(009) jset     #0x1fff          jt 16	jf 10
(010) ldxb     4*([14]&0xf)
(011) ldh      [x + 14]
(012) jeq      #0x14e9          jt 15	jf 13
(013) ldh      [x + 16]
(014) jeq      #0x14e9          jt 15	jf 16
(015) ret      #8192
(016) ret      #0

*** Dump packet-matching code as a C program fragment (-dd) ***

{ 0x28, 0, 0, 0x0000000c },
{ 0x15, 0, 14, 0x00000800 },
{ 0x20, 0, 0, 0x0000001a },
{ 0x15, 2, 0, 0x7f0000fe },
{ 0x20, 0, 0, 0x0000001e },
{ 0x15, 0, 10, 0x7f0000fe },
{ 0x30, 0, 0, 0x00000017 },
{ 0x15, 0, 8, 0x00000006 },
{ 0x28, 0, 0, 0x00000014 },
{ 0x45, 6, 0, 0x00001fff },
{ 0xb1, 0, 0, 0x0000000e },
{ 0x48, 0, 0, 0x0000000e },
{ 0x15, 2, 0, 0x000014e9 },
{ 0x48, 0, 0, 0x00000010 },
{ 0x15, 0, 1, 0x000014e9 },
{ 0x6, 0, 0, 0x00002000 },
{ 0x6, 0, 0, 0x00000000 },

*** Dump packet-matching code as bytecode (-ddd) ***

17
40 0 0 12
21 0 14 2048
32 0 0 26
21 2 0 2130706686
32 0 0 30
21 0 10 2130706686
48 0 0 23
21 0 8 6
40 0 0 20
69 6 0 8191
177 0 0 14
72 0 0 14
21 2 0 5353
72 0 0 16
21 0 1 5353
6 0 0 8192
6 0 0 0

*** Dump packet-matching code as bytecode (on one line) ***

17,40 0 0 12,21 0 14 2048,32 0 0 26,21 2 0 2130706686,32 0 0 30,21 0 10 2130706686,48 0 0 23,21 0 8 6,40 0 0 20,69 6 0 8191,177 0 0 14,72 0 0 14,21 2 0 5353,72 0 0 16,21 0 1 5353,6 0 0 8192,6 0 0 0
~~~

## Generating and using BPF bytecode

The aforementioned article [BPF - the forgotten bytecode](https://blog.cloudflare.com/bpf-the-forgotten-bytecode/){target=_blank} shows that BPF bytecode can also be used outside of tcpdump. Let's see how we can use BPF bytecode together with iptables to filter traffic.

### Dependencies

For RHEL/CentOS 8, during my testing I installed the following build dependencies for the examples below:
~~~
yum install '@Development Tools' -y
yum install gcc-toolset -y
yum install binutils-devel -y
yum install readline-devel -y
yum install elfutils-libelf-devel -y
yum install clang -y
yum install llvm -y
~~~

### Generating BPF bytecode with tcpdump

There are several different options to generate BPF bytecode. The easiest way is to use tcpdump to generate it from a provided filter expression. The iptables bpf module requires input that is generated with data link type `DLT_RAW`. 

For further information about DLTs, see [LINK-LAYER HEADER TYPES](https://www.tcpdump.org/linktypes.html){target=_blank} which states the following for `DLT_RAW`:
~~~
Raw IP; the packet begins with an IPv4 or IPv6 header, with the version field of the header indicating whether it's an IPv4 or IPv6 heade
~~~

#### Prerequisites for tcpdump

In my case, `DLT_RAW` was not supported by my distribution's build (RHEL 8). If you find yourself in the same situation, you can build a recent version of tcpdump directly from source. 

Example build instructions:
~~~
git clone https://github.com/the-tcpdump-group/tcpdump
git clone https://github.com/the-tcpdump-group/libpcap.git
cd libpcap/
git checkout origin/libpcap-1.10
./configure
make install
cd ../tcpdump/ 
git checkout origin/tcpdump-4.99
./configure
make install
~~~

#### Using a simple filter expression with tcpdump

Compare the output of the following commands:
~~~
# # implicit EN10MB DLT
# tcpdump -d tcp dst port 8080
Warning: assuming Ethernet
(000) ldh      [12]
(001) jeq      #0x86dd          jt 2	jf 6
(002) ldb      [20]
(003) jeq      #0x6             jt 4	jf 15
(004) ldh      [56]
(005) jeq      #0x1f90          jt 14	jf 15
(006) jeq      #0x800           jt 7	jf 15
(007) ldb      [23]
(008) jeq      #0x6             jt 9	jf 15
(009) ldh      [20]
(010) jset     #0x1fff          jt 15	jf 11
(011) ldxb     4*([14]&0xf)
(012) ldh      [x + 16]
(013) jeq      #0x1f90          jt 14	jf 15
(014) ret      #262144
(015) ret      #0
# RAW DLT
# tcpdump -y RAW -d tcp dst port 8080
(000) ldb      [0]
(001) and      #0xf0
(002) jeq      #0x60            jt 3	jf 7
(003) ldb      [6]
(004) jeq      #0x6             jt 5	jf 18
(005) ldh      [42]
(006) jeq      #0x1f90          jt 17	jf 18
(007) ldb      [0]
(008) and      #0xf0
(009) jeq      #0x40            jt 10	jf 18
(010) ldb      [9]
(011) jeq      #0x6             jt 12	jf 18
(012) ldh      [6]
(013) jset     #0x1fff          jt 18	jf 14
(014) ldxb     4*([0]&0xf)
(015) ldh      [x + 2]
(016) jeq      #0x1f90          jt 17	jf 18
(017) ret      #262144
(018) ret      #0
~~~

You get the bytecode with:
~~~
# tcpdump -y RAW -ddd tcp dst port 8080 | tr '\n' ',' | sed 's/,$//'
19,48 0 0 0,84 0 0 240,21 0 4 96,48 0 0 6,21 0 13 6,40 0 0 42,21 10 11 8080,48 0 0 0,84 0 0 240,21 0 8 64,48 0 0 9,21 0 6 6,40 0 0 6,69 4 0 8191,177 0 0 0,72 0 0 2,21 0 1 8080,6 0 0 262144,6 0 0 0
~~~

And now, you can use this bytecode with iptables:
~~~
# python3 -m http.server 8080 >/dev/null 2>&1 &
# curl -s -o /dev/null -w "%{http_code}" 127.0.0.1:8080
200
# iptables -I INPUT -m bpf --bytecode "$(tcpdump -y RAW -ddd tcp dst port 8080 | tr '\n' ',' | sed 's/,$//')" -j REJECT
# curl -s -o /dev/null -w "%{http_code}" 127.0.0.1:8080
000
# iptables -L INPUT -nv --line-numbers
Chain INPUT (policy ACCEPT 0 packets, 0 bytes)
num   pkts bytes target     prot opt in     out     source               destination
1        2   120 REJECT     all  --  *      *       0.0.0.0/0            0.0.0.0/0           match bpf 48 0 0 0,84 0 0 240,21 0 4 96,48 0 0 6,21 0 13 6,40 0 0 42,21 10 11 8080,48 0 0 0,84 0 0 240,21 0 8 64,48 0 0 9,21 0 6 6,40 0 0 6,69 4 0 8191,177 0 0 0,72 0 0 2,21 0 1 8080,6 0 0 262144,6 0 0 0 reject-with icmp-port-unreachable
~~~

#### tcpdump with an explicit offset expression

Let's get a bit more fancy and let's filter the same packets with a custom offset expression. 
In `test.pcap`, I captured a TCP request to port 8080. The hexdump looks as follows. I am purposefully ignoring the ethernet header by only providing `-x` as `DLT_RAW` begins with the IP header:
~~~
# tcpdump -nn -r test.pcap -x | tail -n 10
reading from file test.pcap, link-type EN10MB (Ethernet), snapshot length 262144
08:58:05.409490 IP 127.0.0.1.8080 > 127.0.0.1.55346: Flags [F.], seq 1, ack 80, win 342, options [nop,nop,TS val 1370696821 ecr 1370694317], length 0
	0x0000:  4500 0034 896d 4000 4006 b354 7f00 0001
	0x0010:  7f00 0001 1f90 d832 2b16 be50 ec11 89b4
	0x0020:  8011 0156 fe28 0000 0101 080a 51b3 2c75
	0x0030:  51b3 22ad
08:58:05.409525 IP 127.0.0.1.55346 > 127.0.0.1.8080: Flags [.], ack 2, win 342, options [nop,nop,TS val 1370696821 ecr 1370696821], length 0
	0x0000:  4500 0034 0000 4000 4006 3cc2 7f00 0001
	0x0010:  7f00 0001 d832 1f90 ec11 89b4 2b16 be51
                             ^
                             |-------- dst port 0x1f90 (Bytes 22 + 23)
	0x0020:  8010 0156 2423 0000 0101 080a 51b3 2c75
	0x0030:  51b3 2c75
~~~

We want to identify all packets with a dst port of `8080` (or `0x1f90` in hex notation). We are looking for halfword 11, or Bytes 22 and 23. The tcpdump expression would hence be:
~~~
ether[22:2] == 0x1f90
~~~
Don't be mislead by the `ether` keyword. We are filtering with `DLT_RAW` and hence have to ignore the etherheader.

And the compiled expression:
~~~
#  tcpdump -y RAW -d ether[22:2] == 0x1f90
(000) ldh      [22]
(001) jeq      #0x1f90          jt 2	jf 3
(002) ret      #262144
(003) ret      #0
# tcpdump -y RAW -ddd ether[22:2] == 0x1f90 | tr '\n' ',' | sed 's/,$//'
4,40 0 0 22,21 0 1 8080,6 0 0 262144,6 0 0 0
~~~

And we can test this again:
~~~
# python3 -m http.server 8080 >/dev/null 2>&1 &
# curl -s -o /dev/null -w "%{http_code}" 127.0.0.1:8080
200
# iptables -I INPUT -m bpf --bytecode "$(tcpdump -y RAW -ddd ether[22:2] == 0x1f90 | tr '\n' ',' | sed 's/,$//')" -j REJECT
# curl -s -o /dev/null -w "%{http_code}" 127.0.0.1:8080
000
# iptables -L INPUT -nv --line-numbers
Chain INPUT (policy ACCEPT 0 packets, 0 bytes)
num   pkts bytes target     prot opt in     out     source               destination
1        2   120 REJECT     all  --  *      *       0.0.0.0/0            0.0.0.0/0           match bpf 40 0 0 22,21 0 1 8080,6 0 0 262144,6 0 0 0 reject-with icmp-port-unreachable
~~~

### Generating BPF bytecode with nfnpf_compile

While searching for more resources, I found [BPF: A Bytecode for filtering](https://www.lowendtalk.com/discussion/47469/bpf-a-bytecode-for-filtering){target=_blank}.
The article links to [nfbpf_compile.c](https://git.netfilter.org/iptables/tree/utils/nfbpf_compile.c){target=_blank} which cat generate iptables -m bpf compatible byte code
by using `DLT_RAW`.

Here's the full script from the above link (just in case the original resource changes):
~~~
cat <<'EOF' > nbpf_compile.c
/*
 * BPF program compilation tool
 *
 * Generates decimal output, similar to `tcpdump -ddd ...`.
 * Unlike tcpdump, will generate for any given link layer type.
 *
 * Written by Willem de Bruijn (willemb@google.com)
 * Copyright Google, Inc. 2013
 * Licensed under the GNU General Public License version 2 (GPLv2)
*/

#include <pcap.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	struct bpf_program program;
	struct bpf_insn *ins;
	int i, dlt = DLT_RAW;

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage:    %s [link] '<program>'\n\n"
				"          link is a pcap linklayer type:\n"
				"          one of EN10MB, RAW, SLIP, ...\n\n"
				"Examples: %s RAW 'tcp and greater 100'\n"
				"          %s EN10MB 'ip proto 47'\n'",
				argv[0], argv[0], argv[0]);
		return 1;
	}

	if (argc == 3) {
		dlt = pcap_datalink_name_to_val(argv[1]);
		if (dlt == -1) {
			fprintf(stderr, "Unknown datalinktype: %s\n", argv[1]);
			return 1;
		}
	}

	if (pcap_compile_nopcap(65535, dlt, &program, argv[argc - 1], 1,
				PCAP_NETMASK_UNKNOWN)) {
		fprintf(stderr, "Compilation error\n");
		return 1;
	}

	printf("%d,", program.bf_len);
	ins = program.bf_insns;
	for (i = 0; i < program.bf_len-1; ++ins, ++i)
		printf("%u %u %u %u,", ins->code, ins->jt, ins->jf, ins->k);

	printf("%u %u %u %u\n", ins->code, ins->jt, ins->jf, ins->k);

	pcap_freecode(&program);
	return 0;
}
EOF
~~~

Compile this with:
~~~
gcc nbpf_compile.c -o nbpf_compile -lpcap
~~~

And generate the bytecode:
~~~
# ./nbpf_compile RAW 'tcp port 8080'
23,48 0 0 0,84 0 0 240,21 0 6 96,48 0 0 6,21 0 17 6,40 0 0 40,21 14 0 8080,40 0 0 42,21 12 13 8080,48 0 0 0,84 0 0 240,21 0 10 64,48 0 0 9,21 0 8 6,40 0 0 6,69 6 0 8191,177 0 0 0,72 0 0 0,21 2 0 8080,72 0 0 2,21 0 1 8080,6 0 0 65535,6 0 0 0
~~~

As earlier, we can test this with:
~~~
# python3 -m http.server 8080 >/dev/null 2>&1 &
# curl -s -o /dev/null -w "%{http_code}" 127.0.0.1:8080
200
# iptables -I INPUT -m bpf --bytecode "$(./nbpf_compile RAW 'tcp port 8080')" -j REJECT
# curl -s -o /dev/null -w "%{http_code}" 127.0.0.1:8080
000
# iptables -L INPUT -nv --line-numbers
Chain INPUT (policy ACCEPT 0 packets, 0 bytes)
num   pkts bytes target     prot opt in     out     source               destination
1        2   120 REJECT     all  --  *      *       0.0.0.0/0            0.0.0.0/0           match bpf 48 0 0 0,84 0 0 240,21 0 6 96,48 0 0 6,21 0 17 6,40 0 0 40,21 14 0 8080,40 0 0 42,21 12 13 8080,48 0 0 0,84 0 0 240,21 0 10 64,48 0 0 9,21 0 8 6,40 0 0 6,69 6 0 8191,177 0 0 0,72 0 0 0,21 2 0 8080,72 0 0 2,21 0 1 8080,6 0 0 65535,6 0 0 0 reject-with icmp-port-unreachable
~~~

### Using BPF asm instructions

You can also choose the low level route and write your own BPF asm instructions and convert them into bytecode.

#### Compiling helper binaries 

First, you will need the bpf_asm binary from the kernel source code. 

Get the kernel source code on RHEL / CentOS:
~~~
mkdir -p kernel/src
cd kernel/
yumdownloader --source kernel
cp kernel-*.src.rpm src/ ; cd src ; rpm2cpio kernel-*.src.rpm | cpio -idmv
tar -xf linux-*.tar.xz
~~~

Compile the BPF toolset:
~~~
cd $(find . -path '*tools/bpf')
make all
cp bpf_asm /usr/local/bin/.
cp bpf_dbg /usr/local/bin/.
cp bpf_jit_disasm /usr/local/bin/.
cp bpftool/bpftool /usr/local/bin/.
~~~

#### Building bytecode

Now, create the following bpf asm instructions:
~~~
# cat <<'EOF' > test.bpf 
     ldh [22]
     jneq #0x1f90, lb_1
     ret #1

lb_1:
     ret #0
EOF
~~~

And compile it to bytecode with:
~~~
# bpf_asm test.bpf 
4,40 0 0 36,21 0 1 8080,6 0 0 1,6 0 0 0,
~~~

As earlier, we can test this with:
~~~
# python3 -m http.server 8080 >/dev/null 2>&1 &
# curl -s -o /dev/null -w "%{http_code}" 127.0.0.1:8080
200
# iptables -I INPUT -m bpf --bytecode "$(bpf_asm test.bpf)" -j REJECT
# curl -s -o /dev/null -w "%{http_code}" 127.0.0.1:8080
000
# iptables -L INPUT -nv --line-numbers
Chain INPUT (policy ACCEPT 0 packets, 0 bytes)
num   pkts bytes target     prot opt in     out     source               destination
1        2   120 REJECT     all  --  *      *       0.0.0.0/0            0.0.0.0/0           match bpf 40 0 0 22,21 0 1 8080,6 0 0 1,6 0 0 0 reject-with icmp-port-unreachable
~~~

For further details about the assembly language, see the [kernel documentation](https://www.kernel.org/doc/Documentation/networking/filter.txt){target=_blank}.
