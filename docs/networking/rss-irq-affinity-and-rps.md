# RSS, IRQ affinity and RPS on Linux

In this blog post, we are going to have a look at the tuning of Linux receive queues and their interrupt requests. We
are going to learn a bit about RSS (Receive Side Scaling), IRQ SMP affinity, RPS (Receive Packet Steering) and how to
analyze CPUs with flamegraphs. I do not aim at going very low-level. Instead, I'd like you to get a high-level
understanding of this topic.

## Lab setup

We spawn two RHEL 9 virtual machines with two interfaces each. We are going to connect to the instances via eth0 and we
are going to run our tests via eth1.

![dut](https://github.com/user-attachments/assets/2bff8414-67c8-44a7-9ad8-42bad2831f98)
> **DUT:** Device Under Test (server, VM with 4 queues on eth1)
  **LG:** Load Generator (client)

Both VMs have 8 GB of memory and 8 CPUs each:

```
  <memory unit='KiB'>8388608</memory>
  <currentMemory unit='KiB'>8388608</currentMemory>
  <vcpu placement='static'>8</vcpu>
```

In addition, the secondary interface of the Device Under Test (DUT) is configured to have 4 queues:

```
    <interface type='network'>
      <mac address='52:54:00:de:e0:51'/>
      <source network='isol1' portid='cd8d35ef-9ac1-4254-beea-ee8f57b0d176' bridge='virbr5'/>
      <target dev='vnet3'/>
      <model type='virtio'/>
      <driver name='vhost' queues='4'/>
      <alias name='net1'/>
      <address type='pci' domain='0x0000' bus='0x07' slot='0x00' function='0x0'/>
    </interface>
```
> **Note** If you set up your VMs with Virtual Machine Manager, you have to add line `<driver name='vhost' queues='4'/>`
via the XML input field.

## Test application

The test application is a simple client <-> server application written in Golang. The client opens a connection, writes
a message which is received by the server, and closes the connection. It does so asynchronously at a given provided
rate. The application's goal is not to be particularly performant; instead, it shall be easy to understand, have a
configurable rate and support both IPv4 UDP and TCP.

You can find the code at [https://github.com/andreaskaris/golang-loadgen/tree/blog-post(https://github.com/andreaskaris/golang-loadgen/tree/blog-post).

The application can send/receive traffic for both TCP and UDP. In the interest of brevity, I'll focus on the UDP part
only.

### Server code

The entire UDP server code is pretty simple and should be self explanatory:

```
func main() {
(...)
	// Code for the server. See server() for more details.
	if *serverFlag {
		if err := server(protocol, *hostFlag, *portFlag); err != nil {
			log.Fatalf("could not create server, err: %q", err)
		}
		return
	}
(...)
}

// server implements the logic for the server. It uses helper functions tcpServer and udpServer to implement servers
// for the respective protocols.
func server(proto, host string, port int) error {
	hostPort := fmt.Sprintf("%s:%d", host, port)
	if proto == UDP {
		return udpServer(proto, hostPort)
	}
	if proto == TCP {
		return tcpServer(proto, hostPort)
	}
	return fmt.Errorf("unsupported protocol: %q", proto)
}

// udpServer implements the logic for a UDP server. It listens on a given UDP socket. It reads from the socket and
// prints the message of the client if flag -debug was provided.
func udpServer(proto, hostPort string) error {
	addr, err := net.ResolveUDPAddr(proto, hostPort)
	if err != nil {
		return err
	}
	conn, err := net.ListenUDP("udp", addr)
	if err != nil {
		return err
	}
	defer conn.Close()
	buffer := make([]byte, 1024)
	for {
		n, remote, err := conn.ReadFromUDP(buffer)
		if err != nil {
			log.Printf("could not read from UDP buffer, err: %q", err)
			continue
		}
		if *debugFlag {
			log.Printf("read from remote %s: %s", remote, string(buffer[:n]))
		}
	}
}
```

The client code is even simpler:

```
func main() {
(...)
	// Code for the client. The client calculates the sleep time between subsequent attempts based on the rate.
	// For example, if the rate is 1000, sleep for 1,000,000,000 / 1,000 = 1,000,000 nanoseconds between messages
	// -> send 100 messages per second.
	sleepInterval := NANOSECONDS_PER_SECOND / *rateFlag
	sleepTime := time.Nanosecond * time.Duration(sleepInterval)
	for {
		time.Sleep(sleepTime)
		// Run each client in its own go routine. See client() for the rest of the client logic.
		go func() {
			if err := client(protocol, *hostFlag, *portFlag, "msg"); err != nil {
				log.Printf("got error on connection attempt, err: %q", err)
			}
		}()
	}
(...)
}

// client implements the client logic for a single connection. It opens a connection of type proto (TCP or UDP) to
// <host>:<port> and it writes <message> to the connection before closing it.
// Note: The terminology may be a bit confusing as the client pushes data to the server, not the other way around.
func client(proto, host string, port int, message string) error {
	conn, err := net.Dial(proto, fmt.Sprintf("%s:%d", host, port))
	if err != nil {
		return err
	}
	defer conn.Close()
	fmt.Fprint(conn, message)
	return nil
}
```

## Disabling irqbalance on the DUT

irqbalance would actually interfere with our tests. Therefore, let's disable it on the Device Under Test:

```
[root@dut ~]# systemctl disable --now irqbalance
[root@dut ~]# systemctl status irqbalance
○ irqbalance.service - irqbalance daemon
     Loaded: loaded (/usr/lib/systemd/system/irqbalance.service; disabled; preset: enabled)
     Active: inactive (dead)
       Docs: man:irqbalance(1)
             https://github.com/Irqbalance/irqbalance
```

## Isolating CPUs with tuned on the DUT

Let's isolate the last 4 CPUs of the Device Under Test with tuned. First, install tuned and the tuned-profiles-realtime
package:

```
[root@dut ~]# yum install tuned tuned-profiles-realtime
```

Configure the isolated cores in `/etc/tuned/realtime-variables.conf`:

```
[root@dut ~]# grep ^isolated_cores /etc/tuned/realtime-variables.conf 
isolated_cores=4-7
```

Enable the realtime profile and reboot:

```
[root@dut ~]# tuned-adm profile realtime
[root@dut ~]# reboot
```

When the system is back up, verify that isolation is configured:

```
[root@dut ~]# cat /proc/cmdline  | grep -Po "isolcpus=.*? "
isolcpus=managed_irq,domain,4-7
[root@dut ~]# cat /proc/$$/status  | grep Cpus_allowed_list
Cpus_allowed_list:	0-3
```

## Building the application

Build the test application on both nodes:

```
[root@dut ~]# git clone https://github.com/andreaskaris/golang-loadgen.git
Cloning into 'golang-loadgen'...
remote: Enumerating objects: 13, done.
remote: Counting objects: 100% (13/13), done.
remote: Compressing objects: 100% (8/8), done.
remote: Total 13 (delta 4), reused 11 (delta 2), pack-reused 0
Receiving objects: 100% (13/13), done.
Resolving deltas: 100% (4/4), done.
[root@dut ~]# cd golang-loadgen/
[root@dut golang-loadgen]# git checkout blog-post
branch 'blog-post' set up to track 'origin/blog-post'.
Switched to a new branch 'blog-post'
[root@dut golang-loadgen]# make build
go build -o _output/loadgen
```

## Running the test application with debug output

Start the test application on the client:

```
[root@lg golang-loadgen]# _output/loadgen -host 192.168.123.10 -port 8080 -protocol udp -rate-per-second 10
```

Start the test application on the server and force it to run on CPUs 6 and 7:

```
[root@dut golang-loadgen]#  taskset -c 6,7 _output/loadgen -server -host 192.168.123.10 -port 8080 -protocol udp -debug
2024/08/08 15:46:05 read from remote 192.168.123.11:54244: msg
2024/08/08 15:46:05 read from remote 192.168.123.11:59288: msg
2024/08/08 15:46:05 read from remote 192.168.123.11:38316: msg
2024/08/08 15:46:05 read from remote 192.168.123.11:54288: msg
2024/08/08 15:46:05 read from remote 192.168.123.11:58354: msg
2024/08/08 15:46:06 read from remote 192.168.123.11:60341: msg
```

## RSS + Tuning NIC queues and RX interrupt affinity on the Device Under Test

### What's RSS?

RSS, short for Receive Side Scaling, is an in-hardware feature that allows a NIC to "send different packets to different
queues to distribute processing among CPUs. The NIC distributes packets by applying a filter to each packet that assigns
it to one of a small number of logical flows. Packets for each flow are steered to a separate receive queue, which in
turn can be processed by separate CPUs." For more details, have a look at the
[Scaling in the Linux Networking Stack](https://www.kernel.org/doc/html/latest/networking/scaling.html).

RSS happens in hardware for modern NICs and is enabled by default. Virtio supports multiqueue and RSS when the vhost
driver is used (see our Virtual Machine setup instructions earlier).

It's possible to show some information about RSS with `ethtool -x <interface>`. As far as I know, it's not possible to
modify RSS configuration for virtio (when I tried, my VM froze). However, for real NICs such as recent Intel or
Mellanox/Nvidia devices, plenty of configuration options are available to influence how RSS steers packets to queues.

```
[root@dut ~]# ethtool -x eth1

RX flow hash indirection table for eth1 with 2 RX ring(s):
Operation not supported
RSS hash key:
Operation not supported
RSS hash function:
    toeplitz: on
    xor: off
    crc32: off
```

### Reducing queue count to 2

To make things a little bit more manageable, let's reduce our queue count (both for RX and TX) to 2:

```
[root@dut golang-loadgen]# ethtool -l eth1
Channel parameters for eth1:
Pre-set maximums:
RX:		n/a
TX:		n/a
Other:		n/a
Combined:	4
Current hardware settings:
RX:		n/a
TX:		n/a
Other:		n/a
Combined:	4
[root@dut golang-loadgen]# ethtool -L eth1 combined 2
[root@dut golang-loadgen]# ethtool -l eth1
Channel parameters for eth1:
Pre-set maximums:
RX:		n/a
TX:		n/a
Other:		n/a
Combined:	4
Current hardware settings:
RX:		n/a
TX:		n/a
Other:		n/a
Combined:	2
```

### Identifying and reading per queue interrupts

Let's find the interrupt names for eth1. Get the bus-info:

```
[root@dut golang-loadgen]# ethtool -i eth1 | grep bus-info
bus-info: 0000:07:00.0
```

And let's get the device name as it appears in /proc/interrupts:

```
[root@dut golang-loadgen]# find /sys/devices -name 0000:07:00.0
/sys/devices/pci0000:00/0000:00:02.6/0000:07:00.0
[root@dut golang-loadgen]# ls -d /sys/devices/pci0000:00/0000:00:02.6/0000:07:00.0/virtio*
/sys/devices/pci0000:00/0000:00:02.6/0000:07:00.0/virtio6
```

We can now read the interrupts for the NIC:

```
[root@dut golang-loadgen]# cat /proc/interrupts | grep -E 'CPU|virtio6'
           CPU0       CPU1       CPU2       CPU3       CPU4       CPU5       CPU6       CPU7       
 52:          0          0          0          0          0          0          0          0  PCI-MSIX-0000:07:00.0   0-edge      virtio6-config
 53:    7159429          0          1          0          0          0          0          0  PCI-MSIX-0000:07:00.0   1-edge      virtio6-input.0
 54:        417          0          0          1          0          0          0          0  PCI-MSIX-0000:07:00.0   2-edge      virtio6-output.0
 55:          0          0   11189465          0          1       2260          0          0  PCI-MSIX-0000:07:00.0   3-edge      virtio6-input.1
 56:          0          0        165          0        220          1          0          0  PCI-MSIX-0000:07:00.0   4-edge      virtio6-output.1
 57:          0          0          0   11296239          0          0          1          0  PCI-MSIX-0000:07:00.0   5-edge      virtio6-input.2
 58:          0          0          0          0          0          0          0          0  PCI-MSIX-0000:07:00.0   6-edge      virtio6-output.2
 59:   11620198          0          0          0          0          0          2          0  PCI-MSIX-0000:07:00.0   7-edge      virtio6-input.3
 60:          0         13          0          0          0          0          0          0  PCI-MSIX-0000:07:00.0   8-edge      virtio6-output.3
```

Note that virtio will show all 4 queues for each input and output in /proc/interrupts, even though 2 of each are disabled.

We can however easily check that the setting is working. Let's start our application on the server on CPUs 6 and 7:

```
[root@dut golang-loadgen]#  taskset -c 6,7 _output/loadgen -server -host 192.168.123.10 -port 8080 -protocol udp
```

And on the client:

```
[root@lg golang-loadgen]# _output/loadgen -host 192.168.123.10 -port 8080 -protocol udp -rate-per-second 1000000000
```

And you can see that interrupts for queues virtio6-input.0 and virtio6-input.1 increment as RSS balances flows
across the 2 remaining queues. At the same time, queues 2 and 3 are disabled. It might be a bit difficult to read, but
remember that these are absolute counters from since when the machines started. Look at the delta for each of the
queues, so compare the lines starting with 53 to each other, then the lines starting with 55, and so on. You will see
that the 2 lines for IRQs 57 and 59 did not change, whereas the counters for IRQs 53 (CPU 0) and 55 (CPU 5) did increase.

```
[root@dut ~]# for i in {1..2}; do grep virtio6-input /proc/interrupts; sleep 5; done
 53:   65257722          0          1          0          0          0          0          0  PCI-MSIX-0000:07:00.0   1-edge      virtio6-input.0
 55:          0          0   11189465          0          1   34166088          0          0  PCI-MSIX-0000:07:00.0   3-edge      virtio6-input.1
 57:          0          0          0   11296239          0          0          1          0  PCI-MSIX-0000:07:00.0   5-edge      virtio6-input.2
 59:   11620198          0          0          0          0          0          2          0  PCI-MSIX-0000:07:00.0   7-edge      virtio6-input.3
 53:   65419955          0          1          0          0          0          0          0  PCI-MSIX-0000:07:00.0   1-edge      virtio6-input.0
 55:          0          0   11189465          0          1   34272710          0          0  PCI-MSIX-0000:07:00.0   3-edge      virtio6-input.1
 57:          0          0          0   11296239          0          0          1          0  PCI-MSIX-0000:07:00.0   5-edge      virtio6-input.2
 59:   11620198          0          0          0          0          0          2          0  PCI-MSIX-0000:07:00.0   7-edge      virtio6-input.3
```

### Querying SMP affinity for RX queue interrupts

You can get the interrupt numbers for virtio6-input.0 (in this case 53) and virtio6-input.1 (in this case 55) from
/proc/interrupts. Then, query /proc/irq/<interrupt number>/smp_affinity and smp_affinity_list.

```
[root@dut golang-loadgen]# cat /proc/irq/53/smp_affinity
0f
[root@dut golang-loadgen]# cat /proc/irq/53/smp_affinity_list
0-3
[root@dut golang-loadgen]# cat /proc/irq/55/smp_affinity
f0
[root@dut golang-loadgen]# cat /proc/irq/55/smp_affinity_list
4-7
```

That matches what we saw earlier: virtio6-input.0's affinity currently is CPUs 0-3 and in /proc/interrupts we saw that
it generated interrupts on CPU 0. virtio6-input.1's affinity currently is CPUs 4-7 and in /proc/interrupts we saw that
it generated interrupts on CPU 5. But wait, irqbalance is switched off, and we even rebooted the system. Why are our
IRQs distributed between our CPUs and why aren't they allowed on all CPUs? To be confirmed, but the answer may be in
[this commit](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=5e385a6ef31f).

### Configuring SMP affinity for RX queue interrupts

Let's force virtio6-input.0 onto CPU 2 and virtio6-input.1 onto CPU 3. The softirqs will be processed on the same NICs
by default. The affinity can be any CPU, regardless of our tuned configuration, either from the system CPU set or from
the isolated CPU set. But I already moved IRQs to isolated CPUs during an earlier test, so for the sake of it, I want
to move them to system reserved CPUs now :-)

```
[root@dut golang-loadgen]# echo 04 > /proc/irq/53/smp_affinity
[root@dut golang-loadgen]# cat /proc/irq/53/smp_affinity_list
2
[root@dut golang-loadgen]# echo 08 > /proc/irq/55/smp_affinity
[root@dut golang-loadgen]# cat /proc/irq/55/smp_affinity_list
3
```

Start the server and the client again with the same affinity for the server:

```
[root@dut golang-loadgen]#  taskset -c 6,7 _output/loadgen -server -host 192.168.123.10 -port 8080 -protocol udp
```

```
[root@lg golang-loadgen]# _output/loadgen -host 192.168.123.10 -port 8080 -protocol udp -rate-per-second 1000000000
```

And now, interrupts increase for virtio6-input.0 on CPU 2 and for virtio6-input.1 on CPU 3:

```
[root@dut ~]# for i in {1..2}; do grep virtio6-input /proc/interrupts; sleep 5; done
 53:   72469349          0     677467          0          0          0          0          0  PCI-MSIX-0000:07:00.0   1-edge      virtio6-input.0
 55:          0          0   11189465     350044          1   38659605          0          0  PCI-MSIX-0000:07:00.0   3-edge      virtio6-input.1
 57:          0          0          0   11296239          0          0          1          0  PCI-MSIX-0000:07:00.0   5-edge      virtio6-input.2
 59:   11620198          0          0          0          0          0          2          0  PCI-MSIX-0000:07:00.0   7-edge      virtio6-input.3
 53:   72469349          0     995563          0          0          0          0          0  PCI-MSIX-0000:07:00.0   1-edge      virtio6-input.0
 55:          0          0   11189465     535187          1   38659605          0          0  PCI-MSIX-0000:07:00.0   3-edge      virtio6-input.1
 57:          0          0          0   11296239          0          0          1          0  PCI-MSIX-0000:07:00.0   5-edge      virtio6-input.2
 59:   11620198          0          0          0          0          0          2          0  PCI-MSIX-0000:07:00.0   7-edge      virtio6-input.3
```

## Using flamegraphs to analyze CPU activity

Now that we configured out server to run on CPUs 6 and 7, and our receive queue interrupts on CPUs 2 and 3, let's
profile our CPUs. Let's use perf script to create flamegraphs:

```
[root@dut ~]# for c in {0..7}; do $(d=$(pwd)/irq_smp_affinity.$c; mkdir $d; pushd $d; perf script flamegraph -C $c -F 99 sleep 5; popd) & done
(... wait for > 5 seconds ) ...
```
> **Note:** The `-F 99` means that iperf samples at a rate of 99 samples per second. Sampling isn't perfect: if the CPU
does something very shortlived between samples, the profiler will not capture it!

Now, copy the flamegraphs to your local system for analysis. You can access the flamegraphs of my test runs here:

* [IRQ smp affinity - flamegraph 0](../src/rss-irq-affinity-and-rps/irq_smp_affinity.0/flamegraph.html)
* [IRQ smp affinity - flamegraph 1](../src/rss-irq-affinity-and-rps/irq_smp_affinity.1/flamegraph.html)
* [IRQ smp affinity - flamegraph 2](../src/rss-irq-affinity-and-rps/irq_smp_affinity.2/flamegraph.html)
* [IRQ smp affinity - flamegraph 3](../src/rss-irq-affinity-and-rps/irq_smp_affinity.3/flamegraph.html)
* [IRQ smp affinity - flamegraph 4](../src/rss-irq-affinity-and-rps/irq_smp_affinity.4/flamegraph.html)
* [IRQ smp affinity - flamegraph 5](../src/rss-irq-affinity-and-rps/irq_smp_affinity.5/flamegraph.html)
* [IRQ smp affinity - flamegraph 6](../src/rss-irq-affinity-and-rps/irq_smp_affinity.6/flamegraph.html)
* [IRQ smp affinity - flamegraph 7](../src/rss-irq-affinity-and-rps/irq_smp_affinity.7/flamegraph.html)

> **Note:** This version of the flamegraphs color codes userspace code in green and kernel code in blue.

The flamegraphs show us largely idle CPUs 0,1, 4 and 5 which is expected. However, CPU 7 is idle as well, even though
the server should be running on CPUs 6 and 7.
Have another look at the server implementation: you will see that the TCP server is multithreaded (use of go routines)
and the UDP server is single threaded. I had not intended it to be this way - it was an omission on my side because
I initially implemented the TCP code and then quickly added the UDP part. And looking at the flamegraph for CPU 7 makes
painfully clear that the CPU is not executing any of our go code.

The flamegraph for CPU 6 shows us our server is spending most of its time in readFromUDP, as expected. About half of
that time is spent waiting in golang function
[internal/poll.runtime_pollWait](https://github.com/golang/go/blob/8bba868de983dd7bf55fcd121495ba8d6e2734e7/src/runtime/netpoll.go#L334).
In turn, this go function does an epoll_wait syscall which leads to a napi_busy_loop which is responsible for receiving
our packets. Most of the other half is spent in syscall recvfrom.

```
man recvfrom()
(...)
   recvfrom()
       recvfrom() places the received message into the buffer buf.  The caller must specify the size of the buffer in len.

       If src_addr is not NULL, and the underlying protocol provides the source address of the message, that  source  address  is  placed  in  the  buffer
       pointed to by src_addr.  In this case, addrlen is a value-result argument.  Before the call, it should be initialized to the size of the buffer as‐
       sociated  with  src_addr.   Upon return, addrlen is updated to contain the actual size of the source address.  The returned address is truncated if
       the buffer provided is too small; in this case, addrlen will return a value greater than was supplied to the call.

       If the caller is not interested in the source address, src_addr and addrlen should be specified as NULL.
(...)
```

![IRQ smp affinity - flamegraph 6](https://github.com/user-attachments/assets/8d796b41-b056-4048-b5d4-5385538ecdb2)


CPUs 2 and 3 process our softirqs, at roughly 20% and 13% respectively. Actually, running top or mpstat during the
tests also showed that the CPUs were spending that amount of time processing hardware interrupts and softirqs.
This is pure speculation, but I suppose that we do not see any hardware interrupts here for 2 reasons:
* Linux NAPI makes sure to spend most of its time polling, thus most of the time will be spent processing softinterrupts
in the [bottom half](https://developer.ibm.com/tutorials/l-tasklets/).
* We do not see hard interrupts because they are missed by our samples.
[Brendan Gregg's blog](https://www.brendangregg.com/FlameGraphs/cpuflamegraphs.html)
might have some tips to get to the bottom of this.

![IRQ smp affinity - flamegraph 2](https://github.com/user-attachments/assets/b0d22412-8cd3-4aba-91a4-bc7c9233854a)

Even though most of what's happening is still difficult to understand for me, at least it's not a black box any more.
We can see what's causing each CPU to be busy, and with the help of man pages or the actual application and kernel code
we can dive deep into the code and try to understand how things work if we want to.
