### Summary ###
The summit was really interesting, but for somebody like me working more with the top-level OpenStack components many times lacked some high-level overview of what a specific talk was actually about. Understandably, the further up a presentation was on the stack, the easier it was for me to follow along with it. The OVS presentations, particularly of the first day, hence were far easier to follow than the DPDK presentations.

In this document, I annotate some of the presentations that seemed interesting **to me** with clarifying documents and quotes.

### DPDK summit ###

Agenda: [https://events.linuxfoundation.org/events/dpdknorthamerica2018/dpdk-na-program/agenda/](https://events.linuxfoundation.org/events/dpdknorthamerica2018/dpdk-na-program/agenda/)

#### SW Assisted vDPA for Live Migration - Xiao Wang, Intel ####

Presentation:  [https://schd.ws/hosted_files/dpdksummitnorthamerica2018/8f/XiaoWang-DPDK-US-Summit-SW-assisted-VDPA-for-LM-v2.pdf](https://schd.ws/hosted_files/dpdksummitnorthamerica2018/8f/XiaoWang-DPDK-US-Summit-SW-assisted-VDPA-for-LM-v2.pdf)
~~~
Key Takeaways
• VDPA combines SW Flex & HW Perf
• SW assisted VDPA could further simplify HW design
• A generic zero copy framework for all NICs with VDPA
~~~

##### What is vDPA? #####

vDPA: vhost Data Path Acceleration

Brand new patch to the kernel: [https://lwn.net/Articles/750770/](https://lwn.net/Articles/750770/)
~~~
This patch introduces a mdev (mediated device) based hardware
vhost backend. This backend is an abstraction of the various
hardware vhost accelerators (potentially any device that uses
virtio ring can be used as a vhost accelerator). Some generic
mdev parent ops are provided for accelerator drivers to support
generating mdev instances.
(...)
Difference between vDPA and PCI passthru
========================================

The key difference between vDPA and PCI passthru is that, in
vDPA only the data path of the device (e.g. DMA ring, notify
region and queue interrupt) is pass-throughed to the VM, the
device control path (e.g. PCI configuration space and MMIO
regions) is still defined and emulated by QEMU.

The benefits of keeping virtio device emulation in QEMU compared
with virtio device PCI passthru include (but not limit to):

- consistent device interface for guest OS in the VM;
- max flexibility on the hardware design, especially the
  accelerator for each vhost backend doesn't have to be a
  full PCI device;
- leveraging the existing virtio live-migration framework;
~~~

The important take-away is that we'll hopefully soon have a means to get the performance of SR-IOV with the flexibility of virtio, especially with regards to live-migration.

##### What is mdev? #####

mdev is "a common interface for mediated device management that can be used by drivers of different devices." and allows to "query and configure mediated devices in a hardware-agnostic fashion". The key point is that it's the hardware abstraction technology which enables vDPA.

[https://www.kernel.org/doc/Documentation/vfio-mediated-device.txt](https://www.kernel.org/doc/Documentation/vfio-mediated-device.txt)
~~~
Virtual Function I/O (VFIO) Mediated devices[1]
===============================================

The number of use cases for virtualizing DMA devices that do not have built-in
SR_IOV capability is increasing. Previously, to virtualize such devices,
developers had to create their own management interfaces and APIs, and then
integrate them with user space software. To simplify integration with user space
software, we have identified common requirements and a unified management
interface for such devices.

The VFIO driver framework provides unified APIs for direct device access. It is
an IOMMU/device-agnostic framework for exposing direct device access to user
space in a secure, IOMMU-protected environment. This framework is used for
multiple devices, such as GPUs, network adapters, and compute accelerators. With
direct device access, virtual machines or user space applications have direct
access to the physical device. This framework is reused for mediated devices.

The mediated core driver provides a common interface for mediated device
management that can be used by drivers of different devices. This module
provides a generic interface to perform these operations:

* Create and destroy a mediated device
* Add a mediated device to and remove it from a mediated bus driver
* Add a mediated device to and remove it from an IOMMU group
(...)
Mediated Device Management Interface Through sysfs
==================================================

The management interface through sysfs enables user space software, such as
libvirt, to query and configure mediated devices in a hardware-agnostic fashion.
This management interface provides flexibility to the underlying physical
device's driver to support features such as:

* Mediated device hot plug
* Multiple mediated devices in a single virtual machine
* Multiple mediated devices from different physical devices
(...)
~~~

#### Using nDPI over DPDK to Classify and Block Unwanted Network Traffic ####

Presentation: [https://schd.ws/hosted_files/dpdksummitnorthamerica2018/a5/LUCADERI_NDPI.pdf](https://schd.ws/hosted_files/dpdksummitnorthamerica2018/a5/LUCADERI_NDPI.pdf)

Summary:
* Presented nDPI an open source 
DPI toolkit able to detect many popular 
Internet protocols and scale at 10 Gbit on 
commodity hardware platforms. 

* Its open design make it suitable for using it 
both in open-source and security applications 
where code inspection is compulsory. 

* Code Availability (GNU LGPLv3)
https://github.com/ntop/nDPI

#### Thread Quiescent State (TQS) Library - Honnappa Nagarahalli, Arm ####

Presentation: [https://schd.ws/hosted_files/dpdksummitnorthamerica2018/e3/lock_free_rte_tqs_Honnappa.pptx](https://schd.ws/hosted_files/dpdksummitnorthamerica2018/e3/lock_free_rte_tqs_Honnappa.pptx)

#### A Hierarchical SW Load Balancing Solution for Cloud Deployment - Hongjun Ni, Intel #### 

#### DPDK Based L4 Load Balancer - M Jayakumar, Intel ####

Presentation: [https://schd.ws/hosted_files/dpdksummitnorthamerica2018/02/MJay_DPDKBasedLoadBalancers.pdf](https://schd.ws/hosted_files/dpdksummitnorthamerica2018/02/MJay_DPDKBasedLoadBalancers.pdf)

Presents the DPDK Virtual Server (DPVS): [https://github.com/iqiyi/dpvs](https://github.com/iqiyi/dpvs)

#### Accelerating Telco NFV Deployments with DPDK and Smart NIC - Kalimani Venkatesan Govindarajan, Aricent & Barak Perlman , Ethernity Network ####

Presentation: [https://schd.ws/hosted_files/dpdksummitnorthamerica2018/c6/Kalimani%20and%20Barak%20Accelerating%20NFV%20with%20DPDK%20and%20SmartNICs.pdf](https://schd.ws/hosted_files/dpdksummitnorthamerica2018/c6/Kalimani%20and%20Barak%20Accelerating%20NFV%20with%20DPDK%20and%20SmartNICs.pdf)

##### What is a vBNG ####
virtual Broadband Network Gateway
~~~
A broadband remote access server (BRAS, B-RAS or BBRAS) routes traffic to and from broadband remote access devices such as digital subscriber line access multiplexers (DSLAM) on an Internet service provider's (ISP) network.[1][2] BRAS can also be referred to as a Broadband Network Gateway (BNG).[3]

The BRAS sits at the edge of an ISP's core network, and aggregates user sessions from the access network. It is at the BRAS that an ISP can inject policy management and IP Quality of Service (QoS). 
(...)
By acting as the network termination point, the BRAS is responsible for assigning network parameters such as IP addresses to the clients. The BRAS is also the first IP hop from the client to the Internet.
~~~
[https://en.wikipedia.org/wiki/Broadband_remote_access_server](https://en.wikipedia.org/wiki/Broadband_remote_access_server)

#### NFF-Go: Bringing DPDK to the Cloud - Areg Melik-Adamyan, Intel #####

#### Enabing P4 in DPDK - Cristian Dumitrescu, Intel & Antonin Bas, Barefoot Networks | Accelerating DPDK via P4-programmable FPGA-based Smart NICs - Petr Kastovsky, Netcope Technologies ####

##### P4 #####

Website: [http://www.p4.org](http://www.p4.org)

P4 is a high-level, imperative, domain specific language. As an Open Source project, it intends to be protocol and device independent. The default file extension is `.p4`.
Source: `p4-tutorial.pdf`

Backend targets of P4 are software switches, NICs, packet processors, FPGAS, GPUS, ASICs, etc. Because it is a high-level language, it prevents vendor lockin. 

P4 allows to easily program the dataplane of Smartnics. P4 is not intended to implement the control plane.
Source: `p4-tutorial.pdf`
Source: `file:///home/akaris/blog/dpdksummit/P4%20(programming%20language)%20-%20Wikipedia.html`

P4 syntax is similar to python, however on purpose the language is not Turing complete. P4 can easily be translated into a JSON representation.

Runtime changes to the device's configuration are possible.

P4 is protocol independent, meaning that the programmer needs to define header fields for any protocol, such as VLAN or TCP.
Source: `file:///home/akaris/blog/dpdksummit/P4%20(programming%20language)%20-%20Wikipedia.html`

#### RTE_FLOW ####
[https://doc.dpdk.org/guides/prog_guide/rte_flow.html](https://doc.dpdk.org/guides/prog_guide/rte_flow.html)


#### DPDK Tunnel Offloading  - Yongseok Koh & Rony Efraim, Mellanox ####

Presentation: [https://schd.ws/hosted_files/dpdksummitnorthamerica2018/81/Rony_Yongseok_DPDK_Tunnel_Offloading.pdf](https://schd.ws/hosted_files/dpdksummitnorthamerica2018/81/Rony_Yongseok_DPDK_Tunnel_Offloading.pdf)

#### DPDK on F5 BIG-IP Virtual ADCs - Brent Blood, F5 Networks ####

Presentation: [https://schd.ws/hosted_files/dpdksummitnorthamerica2018/46/BrentBlood_DPDKonF5BIG-IPVirtualADCs.pdf](https://schd.ws/hosted_files/dpdksummitnorthamerica2018/46/BrentBlood_DPDKonF5BIG-IPVirtualADCs.pdf)

Description of how F5 makes use of DPDK in their BIG-IP reverse proxy

#### Arm’s Efforts for DPDK and Optimization Plan - Gavin Hu & Honnappa Nagarahalli, Arm ####

Presentation: [https://schd.ws/hosted_files/dpdksummitnorthamerica2018/b5/Gavin_Hu_Arm%27s%20efforts%20to%20DPDK%20and%20future%20plan.pdf](https://schd.ws/hosted_files/dpdksummitnorthamerica2018/b5/Gavin_Hu_Arm%27s%20efforts%20to%20DPDK%20and%20future%20plan.pdf)

DPDK on ARM

#### DPDK Flow Classification and Traffic Profiling & Measurement  - Ren Wang & Yipeng Wang, Intel Labs ####

#### Projects using DPDK - Stephen Hemminger, Microsoft ####

#### DPDK Open Lab Performance Continious Integration - Jeremy Plsek, University of New Hampshire InterOperability Laboratory ####

Presentation: [https://schd.ws/hosted_files/dpdksummitnorthamerica2018/03/JEREMYPLSEK_COMMUNITYLAB3.pdf](https://schd.ws/hosted_files/dpdksummitnorthamerica2018/03/JEREMYPLSEK_COMMUNITYLAB3.pdf)

~~~
The DPDK Open Lab is a performance based continuous integration system, supported by the DPDK project. When a patch is submitted, it is automatically sent to our CI to be applied and built. Once the patch is compiled and installed, it is ran against each of the bare metal environments hosted in the lab. This is to check for performance degradations or speed ups within DPDK on various hardware platforms. This talk will explore how the this system supports the development community, such as accepting patches based on performance and tracking how performance has changed in DPDK over time. We will go over how to navigate and use the Dashboard. We will show how the performance has changed in DPDK over the past six months, looking at relative numbers and graphs of various platforms. Finally, we will also talk about the future of the Open Lab, such as running more test cases, running unit tests for DPDK, additional capabilities for the dashboard, and making the systems more accessible to the development community. 
~~~

#### Fast Prototyping DPDK Apps in Containernet - Andrew Wang, Comcast ####

#### Implementing DPDK Based Application Container Framework with SPP - Yasufumi Ogawa, NTT ####

#### Shaping the Future of IP Broadcasting with Cisco's vMI and DPDK on Windows - Harini Ramakrishnan, Microsoft & Michael O'Gorman, Cisco ####

#### Improving Security and Flexibility within Windows DPDK Networking Stacks - Ranjit Menon, Intel Corporation & Omar Cardona, Microsoft ####

#### Use DPDK to Accelerate Data Compression for Storage Applications - Fiona Trahe & Paul Luse, Intel ####

#### Fine-grained Device Infrastructure for Network I/O Slicing in DPDK - Cunming Liang & John Mangan, Intel ####

#### Embracing Externally Allocated Memory - Yongseok Koh, Mellanox ####

#### Accelerating DPDK Para-Virtual I/O with DMA Copy Offload Engine - Jiayu Hu, Intel ####

#### Revise 4K Pages Performance Impact for DPDK Applications - Lei Yao & Jiayu Hu, Intel ####

#### DPDK IPsec Library - Declan Doherty, Intel ####

#### Tungsten Fabric Performance Optimization by DPDK - Lei Yao, Intel ####

#### DPDK Based Vswitch Upgrade - Yuanhan Liu, Tencent ####

#### Using New DPDK Port Representor by Switch Application like OVS - Rony Efraim, Mellanox ####

### OVS summit ###

[http://www.openvswitch.org/support/ovscon2018/](http://www.openvswitch.org/support/ovscon2018/)

#### Running OVS-DPDK Without Hugepages, Busy Loop, and Exclusive Cores (Yi Yang, Inspur) 	####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/0910-yang.pdf](http://www.openvswitch.org/support/ovscon2018/5/0910-yang.pdf)


#### Enabling TSO in OvS-DPDK (Tiago Lam, Intel) 	####

Presentation: [www.openvswitch.org/support/ovscon2018/5/0935-lam.pptx](www.openvswitch.org/support/ovscon2018/5/0935-lam.pptx)

This is related to: [https://access.redhat.com/solutions/2971871](https://access.redhat.com/solutions/2971871)
Currently, TCP with large segment sizes achieves less performance on OVS-DPDK than with normal OVS. The problem is that TSO / LSO ([https://en.wikipedia.org/wiki/Large_send_offload](https://en.wikipedia.org/wiki/Large_send_offload) does not work with OVS DPDK. Intel has been working on this feature for a while. Implementing this requires them to change the implementation of DPDK mbufs. 

Intel submitted patches upstream: 
* [https://mail.openvswitch.org/pipermail/ovs-dev/2018-October/352889.html](https://mail.openvswitch.org/pipermail/ovs-dev/2018-October/352889.html)
* [https://mail.openvswitch.org/pipermail/ovs-dev/2018-August/350832.html](https://mail.openvswitch.org/pipermail/ovs-dev/2018-August/350832.html)

#### OVS-DPDK Memory Management and Debugging (Kevin Traynor, Red Hat, and Ian Stokes, Intel) 	####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1000-traynor.pdf](http://www.openvswitch.org/support/ovscon2018/5/1000-traynor.pdf)

Explains current and future memory management models with mbufs. Currently, shared mbuf model.As of OVS 2.10, a per port memory model where mempools are allocated per interface can be enabled by request with:
~~~
ovs-vsctl set Open_vSwitch . other_config:per-port-memory=true
~~~

The presentors continue with a small excursion to memory debugging: error messages that administrators may come across and how to react to them.

#### Empowering OVS with eBPF (Yi-Hung Wei, William Tu, and Yifeng Sun, VMware) 	####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1045-wei.pptx](http://www.openvswitch.org/support/ovscon2018/5/1045-wei.pptx)

##### What is eBPF? #####
extended Berkeley Packet Filter

From [http://www.openvswitch.org/support/ovscon2018/5/1045-wei.pptx](http://www.openvswitch.org/support/ovscon2018/5/1045-wei.pptx):

    A way to write a restricted C program and runs in Linux kernel
         A virtual machine that runs eBPF bytecode in Linux kernel
         Safety guaranteed by BPF verifier
    Maps
        Efficient key/value store resides in kernel space
        Can be shared between eBPF program and user space applications 
        Ex: Implement flow table 
    Helper Functions
        A core kernel defined set of functions for eBPF program to retrieve/push data from/to the kernel
        Ex: BPF_FUNC_map_lookup_elem(), BPF_FUNC_skb_get_tunnel_key()


From [https://lwn.net/Articles/740157/](https://lwn.net/Articles/740157/)
~~~
was designed for capturing and filtering network packets that matched specific rules. Filters are implemented as programs to be run on a register-based virtual machine.

The ability to run user-supplied programs inside of the kernel proved to be a useful design decision but other aspects of the original BPF design didn't hold up so well. 
(...)
Alexei Starovoitov introduced the extended BPF (eBPF) design to take advantage of advances in modern hardware. The eBPF virtual machine more closely resembles contemporary processors, allowing eBPF instructions to be mapped more closely to the hardware ISA for improved performance.
(...)
An eBPF program is "attached" to a designated code path in the kernel. When the code path is traversed, any attached eBPF programs are executed. Given its origin, eBPF is especially suited to writing network programs and it's possible to write programs that attach to a network socket to filter traffic, to classify traffic, and to run network classifier actions. It's even possible to modify the settings of an established network socket with an eBPF program. The XDP project, in particular, uses eBPF to do high-performance packet processing by running eBPF programs at the lowest level of the network stack, immediately after a packet is received. 
~~~

eBPF allows to run user-supplied programs either at the `XDP` entry point, directly after receiving the packet. This will benefit from the kernel's driver, contrary to DPDK, no reimplementation of the driver is needed. On the other hand, this will allow to bypass the kernel's networking stack, or for DDOS mitigation to drop unwanted packets early and hand off the remaining packets to the kernel for processing. Another entry point in addition to `XDP` is `tc`. When this entry point is used, further processing of the packet has already taken place and the eBPF program can take advantage of data structures that were initialized and filled by the kernel, e.g. the skb data structure. 

A really detailed read can be found here: [https://cilium.readthedocs.io/en/v1.3/bpf/](https://cilium.readthedocs.io/en/v1.3/bpf/)
And a really good presentation (definitely worth a watch!) is here: [https://www.youtube.com/watch?v=JRFNIKUROPE](https://www.youtube.com/watch?v=JRFNIKUROPE)

The above presentation focuses on the new debugging possibilities that are brought to us by eBPF. Read this blog article by mcroce about eBPF network debugging in RHEL 8: [https://developers.redhat.com/blog/2018/12/03/network-debugging-with-ebpf/](https://developers.redhat.com/blog/2018/12/03/network-debugging-with-ebpf/)

#### Open vSwitch Extensions with BPF (Paul Chaignon, Orange Labs) 	####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1110-chaignon.pdf](http://www.openvswitch.org/support/ovscon2018/5/1110-chaignon.pdf)

#### Fast Userspace OVS with AF_XDP (William Tu, VMware) ####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1135-tu.pptx](http://www.openvswitch.org/support/ovscon2018/5/1135-tu.pptx)

~~~
AF_XDP is a high-speed Linux socket type
We add a new netdev type based on AF_XDP
Re-use the userspace datapath used by OVS-DPDK
~~~

~~~
Need high packet rate but can’t deploy DPDK? Use AF_XDP!
Still slower than OVS-DPDK [1], more optimizations are coming [2]

Comparison with OVS-DPDK
Better integration with Linux kernel and management tool
Selectively use kernel’s feature, no re-injection needed
Do not require dedicated device or CPU
~~~

Using AF_XDP to get into the netdev datapath, instead of using vfio-pci and the DPDK userspace drivers:
~~~
# ovs-vsctl add-br br0 -- \
		    set Bridge br0 datapath_type=netdev 
# ovs-vsctl add-port br0 eth0 -- \
            set int enp2s0 type="afxdp”
~~~

#### OVS with DPDK Community Update (Ian Stokes, Intel) 	####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1305-stokes.pptx](http://www.openvswitch.org/support/ovscon2018/5/1305-stokes.pptx)

#### Why use DPDK LTS? (Kevin Traynor, Red Hat) ####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1315-traynor.pdf](http://www.openvswitch.org/support/ovscon2018/5/1315-traynor.pdf)

Explaining why DPDK LTS > DPDK latest for OVS

#### Flow Monitoring in OVS (Ashish Varma, VMware) ####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1320-varma.pptx](http://www.openvswitch.org/support/ovscon2018/5/1320-varma.pptx)

~~~
What is flow monitor?

Allows a controller to keep track of changes to the flow table.
Controller can ask the switch to send events for all changes OR filtered based on:
Flow Command Action (INITIAL/ADD/DELETE/MODIFY)
Certain Match Fields (e.g. eth_type=0x0800, ip_proto=132 [SCTP])
			         e.g. eth_type=0x8847 [MPLS])
out_port / out_group
table_id

Multiple Flow Monitors can be installed by a single controller.
Events would be generated by the OpenFlow Switch based on Flow Add/Delete/Modify matching a Flow Monitor.
~~~

#### OVS and PVP testing (Eelco Chaudron, Red Hat) ####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1325-chaudron.pdf](http://www.openvswitch.org/support/ovscon2018/5/1325-chaudron.pdf)

PVP test framework: [https://github.com/chaudron/ovs_perf](https://github.com/chaudron/ovs_perf)

#### Testing the Performance Impact of the Exact Match Cache (Andrew Theurer, Red Hat) ####

First level of DPDK caching infrastructure in OVS:
    * EMC - Exact Match Cache, by default 8192 entries
    * SMC - Signature Match Cache, 1 M flows 
    
[http://docs.openvswitch.org/en/latest/topics/dpdk/bridge/](http://docs.openvswitch.org/en/latest/topics/dpdk/bridge/)
~~~
SMC cache (experimental)

SMC cache or signature match cache is a new cache level after EMC cache. The difference between SMC and EMC is SMC only stores a signature of a flow thus it is much more memory efficient. With same memory space, EMC can store 8k flows while SMC can store 1M flows. When traffic flow count is much larger than EMC size, it is generally beneficial to turn off EMC and turn on SMC. It is currently turned off by default and an experimental feature.

To turn on SMC:

$ ovs-vsctl --no-wait set Open_vSwitch . other_config:smc-enable=tru
~~~

#### Applying SIMD Optimizations to the OVS Datapath Classifier (Harry van Haaren, Intel) 	####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1355-van-haaren.pdf](http://www.openvswitch.org/support/ovscon2018/5/1355-van-haaren.pdf)

#### PMD Auto Load Balancing (Nitin Katiyar, Jan Scheurich, and Venkatesan Pradeep, Ericsson) 	####

Presentation: [http://www.openvswitch.org/support/ovscon2018/](http://www.openvswitch.org/support/ovscon2018/)

RXQs are currently either assigned round-robin to the available PMDs or they are sorted by processing cycles spent for each Rx queue and then assigned to PMDs ordered by that value. Cycles are calculated within 6 snapshots each spanning 10 seconds (1 minute average). 
Dynamic port assignment is currently triggered whenever a configuration change is made to OVS (port addition, etc.)  or when the pmd-rxq-rebalance command is executed.
The presentation proposes automatic load balancing of RXQs across PMDs. Reassignment of RXQs to PMDs will be triggered if it will result in better load distribution of RXQs across PMDs.

#### All or Nothing: The Challenge of Hardware Offload (Dan Daly, Intel) 	####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1510-daly.pptx](http://www.openvswitch.org/support/ovscon2018/5/1510-daly.pptx)

Hardware offloading with Intel NICs

#### Reprogrammable Packet Processing Pipeline in FPGA for OVS Offloading (Debashis Chatterjee, Intel) ####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1535-chatterjee.pptx](http://www.openvswitch.org/support/ovscon2018/5/1535-chatterjee.pptx)

Hardware offloading with Intel NICs

#### Offloading Linux LAG Devices Via Open vSwitch and TC (John Hurley, Netronome) ####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1600-hurley.pptx](http://www.openvswitch.org/support/ovscon2018/5/1600-hurley.pptx)

Netronome LAG hardware offloading

#### Connection Tracing Hardware Offload via TC (Rony Efraim, Yossi Kuperman, and Guy Shattah, Mellanox) ####

Presentation: [http://www.openvswitch.org/support/ovscon2018/5/1625-efraim.pptx](http://www.openvswitch.org/support/ovscon2018/5/1625-efraim.pptx)

Hardware offloading with Melanox

#### Bleep bloop! A robot workshop. (Aaron Conole, Red Hat) 	####

Robot for OVS repository

#### Comparison Between OVS and Tungsten Fabric vRouter (Yi Yang, Inspur) 	####
#### The Discrepancy of the Megaflow Cache in OVS (Levente Csikor and Gabor Retvari, Budapest University of Technology and Economics) 	####
#### Elmo: Source-Routed Multicast for Public Clouds (Muhammad Shahbaz, Stanford) 	####
#### Sangfor Cloud Security Pool, The First-Ever NSH Use Case in Service Function Chaining Product (XiaoFan Chen, Sangfor and Yi Yang, Inspur) 	####
#### Answering the Open Questions About an OVN/OVS Split (Mark Michelson, Red Hat) 	####

Discussion about how to best split OVN and OVS

#### OVN performance: past, present, and future (Mark Michelson, Red Hat) ####

#### Unencapsulated OVN: What we have and what we want (Mark Michelson, Red Hat) 	####

#### Connectivity for External Networks on the Overlay (Gregory A Smith, Nutanix) 	####

#### Active-Active load balancing with liveness detection through OVN forwarding group (Manoj Sharma, Nutanix) 	PPTX 	Video

#### Debugging OVS with GDB (macros) (Eelco Chaudron, Red Hat) ####

Presentation: [http://www.openvswitch.org/support/ovscon2018/6/1345-chaudron.pdf](http://www.openvswitch.org/support/ovscon2018/6/1345-chaudron.pdf)

GDB macros for OVS: [https://github.com/openvswitch/ovs/blob/master/utilities/gdb/ovs_gdb.py](https://github.com/openvswitch/ovs/blob/master/utilities/gdb/ovs_gdb.py)

#### OVN Controller Incremental Processing (Han Zhou, eBay) 	####
#### OVN DBs HA with Scale Test (Aliasgar Ginwala, eBay) 	####
#### Distributed Virtual Routing for VLAN Backed Networks Through OVN (Ankur Sharma, Nutanix) ####
#### Policy-Based Routing in OVS (Mary Manohar, Nutanix) ####
#### Encrypting OVN Tunnels with IPSEC (Qiuyu Xiao, University of North Carolina at Chapel Hill) 	####
#### Windows Community Updates on OVS and OVN (Ionut-Madalin Balutoiu, Cloudbase, and Anand Kumar and Sairam Venugopal, VMware)  ####
