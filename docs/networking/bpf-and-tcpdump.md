## tcpdump expressions and BPF

### Introduction

For a good introduction into tcpdump's BPF compiler, look at the following blog post, then come back here:

* [https://blog.cloudflare.com/bpf-the-forgotten-bytecode/](https://blog.cloudflare.com/bpf-the-forgotten-bytecode/)

### tcpdump's inconsistent BPF expression compiler

Now that you have a good idea about this, let's look at how tcpdump compiles the following filter expression:
~~~
icmp
~~~

This will be straight forward:
~~~
[root@host ~]# tcpdump -i lo icmp -d
(000) ldh      [12]
(001) jeq      #0x800           jt 2	jf 5
(002) ldb      [23]
(003) jeq      #0x1             jt 4	jf 5
(004) ret      #262144
(005) ret      #0
~~~

We'd also expect this to be the same if we look at a file capture:
~~~
[root@host ~]# timeout 1 tcpdump -i lo -w lo.pcap
dropped privs to tcpdump
tcpdump: listening on lo, link-type EN10MB (Ethernet), capture size 262144 bytes
0 packets captured
6 packets received by filter
0 packets dropped by kernel
[root@host ~]# tcpdump -r lo.pcap icmp -d
reading from file lo.pcap, link-type EN10MB (Ethernet)
(000) ldh      [12]
(001) jeq      #0x800           jt 2	jf 5             # <---- ethertype for IP
(002) ldb      [23]
(003) jeq      #0x1             jt 4	jf 5             # <---- IP protocol number for ICMP
(004) ret      #262144
(005) ret      #0
~~~

So far, so good and straight forward. We look at byte 23 and check if the IP protocol number is 1. Before that, we check the ethernet header in byte 12 and make sure that we are using IP.

But what happens if we use this expression?
~~~
vlan
~~~

We'd expect to look into byte 12 of the ethernet header and make sure that we have the ethertype for 802.1q there. So we'd want to see 0x8100 and if we look for Q-in-Q we'd look for 0x88A8 and for double tagging we'd also look for 0x9100:

* [https://en.wikipedia.org/wiki/EtherType](https://en.wikipedia.org/wiki/EtherType)

And if we apply the filter to our previously captured file, this is precisely what happens:
~~~
[root@host ~]# tcpdump -r lo.pcap vlan -d
reading from file lo.pcap, link-type EN10MB (Ethernet)
(000) ldh      [12]
(001) jeq      #0x8100          jt 4	jf 2
(002) jeq      #0x88a8          jt 4	jf 3
(003) jeq      #0x9100          jt 4	jf 5
(004) ret      #262144
(005) ret      #0
~~~

Now, let's look at a live capture from the loopback interface. We should expect the same expression:
~~~
[root@host ~]# tcpdump -i lo vlan -d
(000) ldb      [-4048]
(001) jeq      #0x1             jt 6	jf 2
(002) ldh      [12]
(003) jeq      #0x8100          jt 6	jf 4
(004) jeq      #0x88a8          jt 6	jf 5
(005) jeq      #0x9100          jt 6	jf 7
(006) ret      #262144
(007) ret      #0
~~~

The expression is different now! Why is that? Well, the kernel has an internal optimization and will actually store VLAN information in a location with negative offset.

The reason for this is that the kernel no longer passes vlan tag information as-is to libpcap. See [https://bugs.launchpad.net/ubuntu/+source/tcpdump/+bug/1641429](https://bugs.launchpad.net/ubuntu/+source/tcpdump/+bug/1641429) for further details:
~~~
The kernel now longer passes vlan tag information as-is to libpcap, instead BPF needs to access ancillary data.

That is the reason "vlan 114" works (because it does the right thing), and a manual filter doesn't, because libpcap never actually sees this.

Offsets are negative because this is the way to access this ancillary data (like vlan tags) in the Linux kernel:
https://github.com/torvalds/linux/blob/6f0d349d922ba44e4348a17a78ea51b7135965b1/include/uapi/linux/filter.h#L60
~~~

And see the kernel documentation as well: [https://github.com/torvalds/linux/blob/7acac4b3196caee5e21fb5ea53f8bc124e6a16fc/include/uapi/linux/filter.h#L60](https://github.com/torvalds/linux/blob/7acac4b3196caee5e21fb5ea53f8bc124e6a16fc/include/uapi/linux/filter.h#L60):

~~~
/* RATIONALE. Negative offsets are invalid in BPF.
   We use them to reference ancillary data.
   Unlike introduction new instructions, it does not break
   existing compilers/optimizers.
 */
~~~

For example, let's look for VLAN 32 (0x20):
~~~
[root@host ~]# tcpdump -r lo.pcap vlan 32 -d
reading from file lo.pcap, link-type EN10MB (Ethernet)
(000) ldh      [12]                                  # <---- look for dot1q, etc. at byte 12
(001) jeq      #0x8100          jt 4	jf 2
(002) jeq      #0x88a8          jt 4	jf 3
(003) jeq      #0x9100          jt 4	jf 8
(004) ldh      [14]
(005) and      #0xfff                                # <---- extract the last 12 bits from the field (VID)
(006) jeq      #0x20            jt 7	jf 8         # <---- check for VLAN 32
(007) ret      #262144
(008) ret      #0
~~~

That, too, makes sense. But if we do a live capture, the logic becomes more complex:
~~~
[root@host ~]# tcpdump -i lo vlan 32 -d
(000) ldb      [-4048]
(001) jeq      #0x1             jt 6	jf 2       # <---- does the kernel special field report this as a VLAN?
(002) ldh      [12]                                # <---- otherwise fallback and check the on-wire format as above
(003) jeq      #0x8100          jt 6	jf 4
(004) jeq      #0x88a8          jt 6	jf 5
(005) jeq      #0x9100          jt 6	jf 14
(006) ldb      [-4048]                             # <---- does the kernel special field report this as a VLAN?
(007) jeq      #0x1             jt 8	jf 10
(008) ldb      [-4052]
(009) ja       11
(010) ldh      [14]
(011) and      #0xfff
(012) jeq      #0x20            jt 13	jf 14      # <---- look for VLAN 32 as earlier
(013) ret      #262144
(014) ret      #0
~~~

As a conclusion:

* Data from a `.pcap` file is treated as if everything came directly from the wire, the offsets are the same as we'd see them on the physical layer
* When we capture data from an interface, libpcap will use kernel ancillary data but it will also add a fallback expression in newer versions

### How does tcpdump compile user provided expressions - Source code analysis

Let's look at how tcpdump (by means of libpcap) compiles its expressions into the appropriate BPF bytecode.

tcpdump will compile different types of BPF depending on if the optimization flag is set but also particularly depending on if we open a file for reading or if we run a live capture.

The `-r` indicates to read from a file and the parameter is read here:

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L1759](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L1759)

~~~
		case 'r':
			RFileName = optarg;
			break;
~~~

Opening a file for reading:

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2053](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2053)

This is done here:
~~~
#ifdef HAVE_PCAP_SET_TSTAMP_PRECISION
		pd = pcap_open_offline_with_tstamp_precision(RFileName,
		    ndo->ndo_tstamp_precision, ebuf);
#else
		pd = pcap_open_offline(RFileName, ebuf);
#endif
~~~

If `-r` is not specified, then libpcap will listen on the interface instead:

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2135](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2135)
~~~
		/*
		 * Try to open the interface with the specified name.
		 */
		pd = open_interface(device, ndo, ebuf);
~~~

If a file was used as an input or an socket on a network interface determines how the BPF will be compiled. The the section above.

The actual compilation of the user provided filter expression to BPF happens here:

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2238](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2238)
~~~
	if (pcap_compile(pd, &fcode, cmdbuf, Oflag, netmask) < 0)
		error("%s", pcap_geterr(pd));
~~~

If the `-d` parameter (and thus the `dflag`) is set, tcpdump will call libpcap's `bpf_dump` function to print the expression in one of three formats:

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L1600](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L1600)
~~~
		case 'D':
			Dflag++;
			break;
~~~

* [https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2240](https://github.com/the-tcpdump-group/tcpdump/blob/8281c4ae6e7e01524d20dd69b2275d0ed7949216/tcpdump.c#L2240)
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

* [https://github.com/the-tcpdump-group/tcpdump/blob/master/tcpdump.c#L2238](https://github.com/the-tcpdump-group/tcpdump/blob/master/tcpdump.c#L2238)

And at this documentation here:

* [https://www.tcpdump.org/pcap.html](https://www.tcpdump.org/pcap.html)

#### Installing dependencies

Install build dependencies:
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
            arg_string = strcat(arg_string, " ");
        }
        arg_string = strcat(arg_string, argv[i]);
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

The tool will now produce the same output as tcpdump with the `-d`, `-dd` or `-ddd` flag set:

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

## BPF Bytecode

* [https://www.kernel.org/doc/Documentation/networking/filter.txt](https://www.kernel.org/doc/Documentation/networking/filter.txt)
* [https://github.com/torvalds/linux/tree/master/tools/bpf](https://github.com/torvalds/linux/tree/master/tools/bpf)

### Compiling BPF Bytecode

In order to get to BPF Bytecode, there is an easy, an intermediate and a more complex way.

#### tcpdump with filter expression

This is the easiest way:
~~~
# tcpdump -i lo -e -nn  "tcp dst port 8080" -d 
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
~~~

You get the Bytecode with:
~~~
# tcpdump -i lo -e -nn  "tcp dst port 8080" -ddd | tr '\n' ','
16,40 0 0 12,21 0 4 34525,48 0 0 20,21 0 11 6,40 0 0 56,21 8 9 8080,21 0 8 2048,48 0 0 23,21 0 6 6,40 0 0 20,69 4 0 8191,177 0 0 14,72 0 0 16,21 0 1 8080,6 0 0 262144,6 0 0 0,
~~~

#### tcpdump with explicit offset expression

Let's get a bit more fancy. In `test.pcap`, I captured a TCP request to port 8080. The hexdump looks as follows:
~~~
# tcpdump -e -nn -r test.pcap -XX | tail -n 12
reading from file test.pcap, link-type EN10MB (Ethernet)
dropped privs to tcpdump
07:03:36.901256 00:00:00:00:00:00 > 00:00:00:00:00:00, ethertype IPv4 (0x0800), length 66: 192.168.122.92.36166 > 192.168.122.92.8080: Flags [F.], seq 84, ack 1993, win 1373, options [nop,nop,TS val 3322938017 ecr 3322938017], length 0
	0x0000:  0000 0000 0000 0000 0000 0000 0800 4500  ..............E.
	0x0010:  0034 7023 4000 4006 5497 c0a8 7a5c c0a8  .4p#@.@.T...z\..
	0x0020:  7a5c 8d46 1f90 86e0 b75e 7a9a 5a92 8011  z\.F.....^z.Z...
	0x0030:  055d 7630 0000 0101 080a c610 02a1 c610  .]v0............
	0x0040:  02a1                                     ..
07:03:36.901275 00:00:00:00:00:00 > 00:00:00:00:00:00, ethertype IPv4 (0x0800), length 66: 192.168.122.92.8080 > 192.168.122.92.36166: Flags [.], ack 85, win 342, options [nop,nop,TS val 3322938017 ecr 3322938017], length 0
	0x0000:  0000 0000 0000 0000 0000 0000 0800 4500  ..............E.
	0x0010:  0034 f1fa 4000 4006 d2bf c0a8 7a5c c0a8  .4..@.@.....z\..
	0x0020:  7a5c 1f90 8d46 7a9a 5a92 86e0 b75f 8010  z\...Fz.Z...._..
	0x0030:  0156 7630 0000 0101 080a c610 02a1 c610  .Vv0............
	0x0040:  02a1    
~~~

We want to identify all packets with a dst port of `8080` (or `0x1f90` in hex notation). Each visual block in the hexdump is a half word, or 16 bits. Offsets in tcpdump are specified in Bytes, and the dst port field is 2 Bytes in lenght. We are looking for the 18th half word, or Bytes 36 and 37. The tcpdump expression would hence be:
~~~
ether[36:2] == 0x1f90
~~~

Here's the result of the filter:
~~~
# tcpdump -e -nn -r test.pcap ether[36:2] == 0x1f90
reading from file test.pcap, link-type EN10MB (Ethernet)
dropped privs to tcpdump
07:03:36.900276 00:00:00:00:00:00 > 00:00:00:00:00:00, ethertype IPv4 (0x0800), length 74: 192.168.122.92.36166 > 192.168.122.92.8080: Flags [S], seq 2262873866, win 43690, options [mss 65495,sackOK,TS val 3322938016 ecr 0,nop,wscale 7], length 0
07:03:36.900297 00:00:00:00:00:00 > 00:00:00:00:00:00, ethertype IPv4 (0x0800), length 66: 192.168.122.92.36166 > 192.168.122.92.8080: Flags [.], ack 2056934090, win 342, options [nop,nop,TS val 3322938016 ecr 3322938016], length 0
07:03:36.900328 00:00:00:00:00:00 > 00:00:00:00:00:00, ethertype IPv4 (0x0800), length 149: 192.168.122.92.36166 > 192.168.122.92.8080: Flags [P.], seq 0:83, ack 1, win 342, options [nop,nop,TS val 3322938016 ecr 3322938016], length 83: HTTP: GET / HTTP/1.1
07:03:36.901097 00:00:00:00:00:00 > 00:00:00:00:00:00, ethertype IPv4 (0x0800), length 66: 192.168.122.92.36166 > 192.168.122.92.8080: Flags [.], ack 156, win 350, options [nop,nop,TS val 3322938016 ecr 3322938016], length 0
07:03:36.901122 00:00:00:00:00:00 > 00:00:00:00:00:00, ethertype IPv4 (0x0800), length 66: 192.168.122.92.36166 > 192.168.122.92.8080: Flags [.], ack 1992, win 1373, options [nop,nop,TS val 3322938017 ecr 3322938016], length 0
07:03:36.901256 00:00:00:00:00:00 > 00:00:00:00:00:00, ethertype IPv4 (0x0800), length 66: 192.168.122.92.36166 > 192.168.122.92.8080: Flags [F.], seq 83, ack 1993, win 1373, options [nop,nop,TS val 3322938017 ecr 3322938017], length 0
~~~

And the compiled expression:
~~~
# tcpdump -e -nn -i lo ether[36:2] == 0x1f90 -d
(000) ldh      [36]
(001) jeq      #0x1f90          jt 2	jf 3
(002) ret      #262144
(003) ret      #0
# tcpdump -e -nn -i lo ether[36:2] == 0x1f90 -ddd | tr '\n' ','
4,40 0 0 36,21 0 1 8080,6 0 0 262144,6 0 0 0,
~~~

#### Using BPF asm-like code

##### Compiling helper binaries 

First, you will need the bpf_asm binary from the kernel source code. 

For RHEL/CentOS 8, install build dependencies:
~~~
yum install '@Development Tools' -y
yum install gcc-toolset -y
yum install binutils-devel -y
yum install readline-devel -y
yum install elfutils-libelf-devel -y
yum install clang -y
yum install llvm -y
~~~

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

##### Building Byte code

Now, create the following ASM like code:
~~~
# cat test.bpf 
     ldh [36]
     jneq #0x1f90, lb_1
     ret #1

lb_1:
     ret #0
~~~

And compile it with:
~~~
# bpf_asm test.bpf 
4,40 0 0 36,21 0 1 8080,6 0 0 1,6 0 0 0,
~~~

Compare this to the earlier output of `tcpdump -ddd`:
~~~
# tcpdump -e -nn -i lo ether[36:2] == 0x1f90 -ddd | tr '\n' ','
4,40 0 0 36,21 0 1 8080,6 0 0 262144,6 0 0 0,
~~~

The only thing that's different here is the return code.

For further details, see:

* [https://www.kernel.org/doc/Documentation/networking/filter.txt](https://www.kernel.org/doc/Documentation/networking/filter.txt)

## Using BPF in iptables expressions

BPF allows us to write complex iptables rules. However, if you simply try using the rules that  we had compiled above, you will soon find out that this does not work. 

Instead, look at the following resources:

* [https://www.lowendtalk.com/discussion/47469/bpf-a-bytecode-for-filtering](https://www.lowendtalk.com/discussion/47469/bpf-a-bytecode-for-filtering)
* [https://git.netfilter.org/iptables/tree/utils/nfbpf_compile.c](https://git.netfilter.org/iptables/tree/utils/nfbpf_compile.c)

`nfbpf_compile` will create slightly different Bytecode, suited for iptables. Here's the full script from the above link (just in case the original resource changes):
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

Now, run this:
~~~
# ./nbpf_compile RAW 'tcp port 8080'
23,48 0 0 0,84 0 0 240,21 0 6 96,48 0 0 6,21 0 17 6,40 0 0 40,21 14 0 8080,40 0 0 42,21 12 13 8080,48 0 0 0,84 0 0 240,21 0 10 64,48 0 0 9,21 0 8 6,40 0 0 6,69 6 0 8191,177 0 0 0,72 0 0 0,21 2 0 8080,72 0 0 2,21 0 1 8080,6 0 0 65535,6 0 0 0
~~~

Compare this to the tcpdump output which will not work:
~~~
# tcpdump -r  test.pcap -d tcp port 8080 -ddd | tr '\n' ' '
reading from file test.pcap, link-type EN10MB (Ethernet)
20 40 0 0 12 21 0 6 34525 48 0 0 20 21 0 15 6 40 0 0 54 21 12 0 8080 40 0 0 56 21 10 11 8080 21 0 10 2048 48 0 0 23 21 0 8 6 40 0 0 20 69 6 0 8191 177 0 0 14 72 0 0 14 21 2 0 8080 72 0 0 16 21 0 1 8080 6 0 0 262144 6 0 0 0 
# tcpdump -i lo  -d tcp port 8080 -ddd | tr '\n' ' '
20 40 0 0 12 21 0 6 34525 48 0 0 20 21 0 15 6 40 0 0 54 21 12 0 8080 40 0 0 56 21 10 11 8080 21 0 10 2048 48 0 0 23 21 0 8 6 40 0 0 20 69 6 0 8191 177 0 0 14 72 0 0 14 21 2 0 8080 72 0 0 16 21 0 1 8080 6 0 0 262144 6 0 0 0
~~~

And also run a little web server for the sake of this test:
~~~
#  python3 -m http.server 8080 &
~~~

Run curl against the web server just to check that it's serving documents and can be reached:
~~~
# curl 192.168.122.92:8080 -I
HTTP/1.0 200 OK
Server: SimpleHTTP/0.6 Python/3.6.8
Date: Fri, 26 Mar 2021 15:46:30 GMT
Content-type: text/html; charset=utf-8
Content-Length: 2272
~~~

Now, let's create an iptables rule that will match on the Byte code and apply it:
~~~
# iptables -I INPUT -m bpf --bytecode "23,48 0 0 0,84 0 0 240,21 0 6 96,48 0 0 6,21 0 17 6,40 0 0 40,21 14 0 8080,40 0 0 42,21 12 13 8080,48 0 0 0,84 0 0 240,21 0 10 64,48 0 0 9,21 0 8 6,40 0 0 6,69 6 0 8191,177 0 0 0,72 0 0 0,21 2 0 8080,72 0 0 2,21 0 1 8080,6 0 0 65535,6 0 0 0" --j REJECT
~~~
> This *must* be from `nbpf_compile`!

After applying the rule, we can no longer contact the web server:
~~~
# curl 192.168.122.92:8080 -I
curl: (7) Failed connect to 192.168.122.92:8080; Connection refused
~~~

And we can see that this rule here was matched:
~~~
# iptables -L INPUT -nv
Chain INPUT (policy ACCEPT 0 packets, 0 bytes)
 pkts bytes target     prot opt in     out     source               destination         
    5   300 REJECT     all  --  *      *       0.0.0.0/0            0.0.0.0/0           match bpf 48 0 0 0,84 0 0 240,21 0 6 96,48 0 0 6,21 0 17 6,40 0 0 40,21 14 0 8080,40 0 0 42,21 12 13 8080,48 0 0 0,84 0 0 240,21 0 10 64,48 0 0 9,21 0 8 6,40 0 0 6,69 6 0 8191,177 0 0 0,72 0 0 0,21 2 0 8080,72 0 0 2,21 0 1 8080,6 0 0 65535,6 0 0 0 reject-with icmp-port-unreachable
~~~
