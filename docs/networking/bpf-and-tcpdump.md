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

### Source code analysis

Let's look at how tcpdump (by means of libpcap) compiles its expressions into the appropriate BPF bytecode.

tcpdump will compile different types of BPF depending on if the optimization flag is set but also particularly depending on if we open a file for reading or if we run a live capture.


The `-r` parameter is read here:

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
		pd = pcap_open_offline_with_tstamp_precision(RFileName,
		    ndo->ndo_tstamp_precision, ebuf);
~~~

(... TBD ...)

### Building a filter expression compiler

#### Documentation

In order to build a filter expressioin compiler, we look at the source code:

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

// #include <pcap.h>
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
	printf("\n*** Dump packet-matching code as decimal numbers (-ddd) ***\n\n");
	bpf_dump(&fcode, 3);

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

*** Dump packet-matching code as decimal numbers (-ddd) ***

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
~~~

## Using BPF 

* [https://www.kernel.org/doc/Documentation/networking/filter.txt](https://www.kernel.org/doc/Documentation/networking/filter.txt)
* [https://github.com/torvalds/linux/tree/master/tools/bpf](https://github.com/torvalds/linux/tree/master/tools/bpf)

### Compile BPF kernel tools

Install build dependencies:
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
cd kernel-*/tools/bpf
make all
cp bpf_asm /usr/local/bin/.
cp bpf_dbg /usr/local/bin/.
cp bpf_jit_disasm /usr/local/bin/.
cp bpftool/bpftool /usr/local/bin/.
~~~

(... TBD ...)


## Sniffing traffic with golang

* [https://itnext.io/sniffing-creds-with-go-a-journey-with-libpcap-73bc3e74966](https://itnext.io/sniffing-creds-with-go-a-journey-with-libpcap-73bc3e74966)
