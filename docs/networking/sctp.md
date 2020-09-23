## Analyzing SCTP in OpenShift ##

This short blod post shows how to deploy SCTP capability and pods in OpenShift and has a brief look at an SCTP packet capture. For further info and more details, see the `Resources` section.

## Deploying in Openshift ##

`load-sctp-module.yaml`:
~~~
apiVersion: machineconfiguration.openshift.io/v1
kind: MachineConfig
metadata:
  labels:
    machineconfiguration.openshift.io/role: worker
  name: load-sctp-module
spec:
  config:
    ignition:
      version: 2.2.0
    storage:
      files:
        - contents:
            source: data:,
            verification: {}
          filesystem: root
          mode: 420
          path: /etc/modprobe.d/sctp-blacklist.conf
        - contents:
            source: data:text/plain;charset=utf-8,sctp
          filesystem: root
          mode: 420
          path: /etc/modules-load.d/sctp-load.conf
~~~

~~~
oc apply -f load-sctp-module.yaml
~~~

`sctp-pods.yaml`:
~~~
#!/bin/bash

echo "Installing SCTP on nodes"
echo "https://docs.openshift.com/container-platform/4.4/networking/using-sctp.html"

oc new-project sctp
oc project sctp

cat <<'EOF' | oc apply -f -
apiVersion: v1
kind: Service
metadata:
  name: sctpservice
  labels:
    app: sctpserver
spec:
  type: NodePort
  selector:
    app: sctpserver
  ports:
    - name: sctpserver
      protocol: SCTP
      port: 30102
      targetPort: 30102
EOF

cat <<'EOF' | oc apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: sctpserver
  labels:
    app: sctpserver
spec:
  containers:
    - name: sctpserver
      image: fedora
      command: ["/bin/sh", "-c"]
      args:
        ["dnf install -y nc iperf3 procps-ng iproute && iperf3 -s"]
      ports:
        - containerPort: 30102
          name: sctpserver
          protocol: SCTP
    - name: tshark
      image: danielguerra/alpine-tshark
      command:
        - "tshark"
        - "-i"
        - "eth0"
        - "-V"
        - "sctp"

EOF

cat <<'EOF' | oc apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: sctpclient
  labels:
    app: sctpclient
spec:
  containers:
    - name: sctpclient
      image: fedora
      command: ["/bin/sh", "-c"]
      args:
        ["dnf install -y nc tcpdump iperf3 procps-ng iproute && echo 'iperf3 -c <IP> -t 3600 --sctp' > /etc/motd && sleep inf"]
    - name: tshark
      image: danielguerra/alpine-tshark
      command:
        - "tshark"
        - "-i"
        - "eth0"
        - "-V"
        - "sctp"
EOF

oc get pods -o wide
sctpserverip=$(oc get pods -o wide | grep sctpserver | awk '{print $6}')
echo "Run:"
echo "oc rsh sctpclient"
echo "iperf3 -c $sctpserverip -t 60 --sctp"
echo "You can access the packet capture with:"
echo "oc logs -f -c tcpdump sctpserver"
~~~

~~~
oc apply -f sctp-pods.yaml
~~~

## Analysis ##

Server:
~~~
sh-5.0# ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
3: eth0@if17: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1400 qdisc noqueue state UP group default 
    link/ether d6:96:23:1b:02:05 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 172.27.2.4/23 brd 172.27.3.255 scope global eth0
       valid_lft forever preferred_lft forever
    inet6 fe80::7066:5ff:fe64:1990/64 scope link 
       valid_lft forever preferred_lft forever
       
sh-5.0#  nc -l -p 30102 --sctp -k
hello world!
~~~

Client:
~~~
sh-5.0# ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
3: eth0@if18: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1400 qdisc noqueue state UP group default 
    link/ether d6:96:23:18:02:06 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 172.24.2.5/23 brd 172.24.3.255 scope global eth0
       valid_lft forever preferred_lft forever
    inet6 fe80::a856:85ff:feb0:f262/64 scope link 
       valid_lft forever preferred_lft forever

sh-5.0# nc --sctp 172.27.2.4 30102
hello world!
~~~

--------------

~~~
[root@openshift-jumpserver-0 ~]# oc logs -f sctpserver tshark --tail=0
### connection setup ###

    Verification tag: 0x2222afc5
    [Association index: 0]
    Checksum: 0x00000000 [unverified]
    [Checksum Status: Unverified]
    SHUTDOWN_COMPLETE chunk
        Chunk type: SHUTDOWN_COMPLETE (14)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x00
            .... ...0 = T-Bit: Tag not reflected
        Chunk length: 4

Frame 30: 82 bytes on wire (656 bits), 82 bytes captured (656 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Jul 23, 2020 15:41:58.370315008 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1595518918.370315008 seconds
    [Time delta from previous captured frame: 36.580134390 seconds]
    [Time delta from previous displayed frame: 36.580134390 seconds]
    [Time since reference or first frame: 206.820352621 seconds]
    Frame Number: 30
    Frame Length: 82 bytes (656 bits)
    Capture Length: 82 bytes (656 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:sctp]
Ethernet II, Src: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01), Dst: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
    Destination: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        Address: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        Address: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 172.24.2.5, Dst: 172.27.2.4
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 68
    Identification: 0x0000 (0)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 63
    Protocol: SCTP (132)
    Header checksum: 0xdef7 [validation disabled]
    [Header checksum status: Unverified]
    Source: 172.24.2.5
    Destination: 172.27.2.4
Stream Control Transmission Protocol, Src Port: 57630 (57630), Dst Port: 30102 (30102)
    Source port: 57630
    Destination port: 30102
    Verification tag: 0x00000000
    [Association index: 1]
    Checksum: 0x87034a86 [unverified]
    [Checksum Status: Unverified]
    INIT chunk (Outbound streams: 10, inbound streams: 65535)
        Chunk type: INIT (1)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x00
        Chunk length: 36
        Initiate tag: 0xf3dc3087
        Advertised receiver window credit (a_rwnd): 106496
        Number of outbound streams: 10
        Number of inbound streams: 65535
        Initial TSN: 3207583439
        Supported address types parameter (Supported types: IPv4)
            Parameter type: Supported address types (0x000c)
                0... .... .... .... = Bit: Stop processing of chunk
                .0.. .... .... .... = Bit: Do not report
            Parameter length: 6
            Supported address type: IPv4 address (5)
            Parameter padding: 0000
        ECN parameter
            Parameter type: ECN (0x8000)
                1... .... .... .... = Bit: Skip parameter and continue processing of the chunk
                .0.. .... .... .... = Bit: Do not report
            Parameter length: 4
        Forward TSN supported parameter
            Parameter type: Forward TSN supported (0xc000)
                1... .... .... .... = Bit: Skip parameter and continue processing of the chunk
                .1.. .... .... .... = Bit: Do report
            Parameter length: 4

Frame 31: 306 bytes on wire (2448 bits), 306 bytes captured (2448 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Jul 23, 2020 15:41:58.370379346 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1595518918.370379346 seconds
    [Time delta from previous captured frame: 0.000064338 seconds]
    [Time delta from previous displayed frame: 0.000064338 seconds]
    [Time since reference or first frame: 206.820416959 seconds]
    Frame Number: 31
    Frame Length: 306 bytes (2448 bits)
    Capture Length: 306 bytes (2448 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:sctp]
Ethernet II, Src: d6:96:23:1b:02:05 (d6:96:23:1b:02:05), Dst: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
    Destination: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        Address: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        Address: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 172.27.2.4, Dst: 172.24.2.5
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 292
    Identification: 0x0000 (0)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: SCTP (132)
    Header checksum: 0xdd17 [validation disabled]
    [Header checksum status: Unverified]
    Source: 172.27.2.4
    Destination: 172.24.2.5
Stream Control Transmission Protocol, Src Port: 30102 (30102), Dst Port: 57630 (57630)
    Source port: 30102
    Destination port: 57630
    Verification tag: 0xf3dc3087
    [Association index: 1]
    Checksum: 0x00000000 [unverified]
    [Checksum Status: Unverified]
    INIT_ACK chunk (Outbound streams: 10, inbound streams: 10)
        Chunk type: INIT_ACK (2)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x00
        Chunk length: 260
        Initiate tag: 0xaeacd509
        Advertised receiver window credit (a_rwnd): 106496
        Number of outbound streams: 10
        Number of inbound streams: 10
        Initial TSN: 2976219343
        State cookie parameter (Cookie length: 228 bytes)
            Parameter type: State cookie (0x0007)
                0... .... .... .... = Bit: Stop processing of chunk
                .0.. .... .... .... = Bit: Do not report
            Parameter length: 232
            State cookie: 7754958a8d7e18094f520b506e872d0c68bfe8f200000000...
        ECN parameter
            Parameter type: ECN (0x8000)
                1... .... .... .... = Bit: Skip parameter and continue processing of the chunk
                .0.. .... .... .... = Bit: Do not report
            Parameter length: 4
        Forward TSN supported parameter
            Parameter type: Forward TSN supported (0xc000)
                1... .... .... .... = Bit: Skip parameter and continue processing of the chunk
                .1.. .... .... .... = Bit: Do report
            Parameter length: 4

Frame 32: 278 bytes on wire (2224 bits), 278 bytes captured (2224 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Jul 23, 2020 15:41:58.372081995 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1595518918.372081995 seconds
    [Time delta from previous captured frame: 0.001702649 seconds]
    [Time delta from previous displayed frame: 0.001702649 seconds]
    [Time since reference or first frame: 206.822119608 seconds]
    Frame Number: 32
    Frame Length: 278 bytes (2224 bits)
    Capture Length: 278 bytes (2224 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:sctp]
Ethernet II, Src: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01), Dst: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
    Destination: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        Address: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        Address: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 172.24.2.5, Dst: 172.27.2.4
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 264
    Identification: 0x0000 (0)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 63
    Protocol: SCTP (132)
    Header checksum: 0xde33 [validation disabled]
    [Header checksum status: Unverified]
    Source: 172.24.2.5
    Destination: 172.27.2.4
Stream Control Transmission Protocol, Src Port: 57630 (57630), Dst Port: 30102 (30102)
    Source port: 57630
    Destination port: 30102
    Verification tag: 0xaeacd509
    [Association index: 1]
    Checksum: 0xfd98e801 [unverified]
    [Checksum Status: Unverified]
    COOKIE_ECHO chunk (Cookie length: 228 bytes)
        Chunk type: COOKIE_ECHO (10)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x00
        Chunk length: 232
        Cookie: 7754958a8d7e18094f520b506e872d0c68bfe8f200000000...

Frame 33: 50 bytes on wire (400 bits), 50 bytes captured (400 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Jul 23, 2020 15:41:58.372120464 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1595518918.372120464 seconds
    [Time delta from previous captured frame: 0.000038469 seconds]
    [Time delta from previous displayed frame: 0.000038469 seconds]
    [Time since reference or first frame: 206.822158077 seconds]
    Frame Number: 33
    Frame Length: 50 bytes (400 bits)
    Capture Length: 50 bytes (400 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:sctp]
Ethernet II, Src: d6:96:23:1b:02:05 (d6:96:23:1b:02:05), Dst: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
    Destination: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        Address: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        Address: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 172.27.2.4, Dst: 172.24.2.5
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 36
    Identification: 0x0000 (0)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: SCTP (132)
    Header checksum: 0xde17 [validation disabled]
    [Header checksum status: Unverified]
    Source: 172.27.2.4
    Destination: 172.24.2.5
Stream Control Transmission Protocol, Src Port: 30102 (30102), Dst Port: 57630 (57630)
    Source port: 30102
    Destination port: 57630
    Verification tag: 0xf3dc3087
    [Association index: 1]
    Checksum: 0x00000000 [unverified]
    [Checksum Status: Unverified]
    COOKIE_ACK chunk
        Chunk type: COOKIE_ACK (11)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x00
        Chunk length: 4
        
        
        
        
### Sending "hello world!" ###

Frame 34: 78 bytes on wire (624 bits), 78 bytes captured (624 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Jul 23, 2020 15:42:23.462046650 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1595518943.462046650 seconds
    [Time delta from previous captured frame: 25.089926186 seconds]
    [Time delta from previous displayed frame: 25.089926186 seconds]
    [Time since reference or first frame: 231.912084263 seconds]
    Frame Number: 34
    Frame Length: 78 bytes (624 bits)
    Capture Length: 78 bytes (624 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:sctp:data]
Ethernet II, Src: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01), Dst: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
    Destination: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        Address: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        Address: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 172.24.2.5, Dst: 172.27.2.4
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 64
    Identification: 0x0001 (1)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 63
    Protocol: SCTP (132)
    Header checksum: 0xdefa [validation disabled]
    [Header checksum status: Unverified]
    Source: 172.24.2.5
    Destination: 172.27.2.4
Stream Control Transmission Protocol, Src Port: 57630 (57630), Dst Port: 30102 (30102)
    Source port: 57630
    Destination port: 30102
    Verification tag: 0xaeacd509
    [Association index: 1]
    Checksum: 0x47de77f1 [unverified]
    [Checksum Status: Unverified]
    DATA chunk(ordered, complete segment, TSN: 3207583439, SID: 0, SSN: 0, PPID: 0, payload length: 13 bytes)
        Chunk type: DATA (0)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x03
            .... ...1 = E-Bit: Last segment
            .... ..1. = B-Bit: First segment
            .... .0.. = U-Bit: Ordered delivery
            .... 0... = I-Bit: Possibly delay SACK
        Chunk length: 29
        Transmission sequence number: 3207583439
        Stream identifier: 0x0000
        Stream sequence number: 0
        Payload protocol identifier: not specified (0)
        Chunk padding: 000000
Data (13 bytes)

0000  68 65 6c 6c 6f 20 77 6f 72 6c 64 21 0a            hello world!.
    Data: 68656c6c6f20776f726c64210a
    [Length: 13]

Frame 35: 62 bytes on wire (496 bits), 62 bytes captured (496 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Jul 23, 2020 15:42:23.462088655 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1595518943.462088655 seconds
    [Time delta from previous captured frame: 0.000042005 seconds]
    [Time delta from previous displayed frame: 0.000042005 seconds]
    [Time since reference or first frame: 231.912126268 seconds]
    Frame Number: 35
    Frame Length: 62 bytes (496 bits)
    Capture Length: 62 bytes (496 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:sctp]
Ethernet II, Src: d6:96:23:1b:02:05 (d6:96:23:1b:02:05), Dst: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
    Destination: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        Address: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        Address: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 172.27.2.4, Dst: 172.24.2.5
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 48
    Identification: 0x6d25 (27941)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: SCTP (132)
    Header checksum: 0x70e6 [validation disabled]
    [Header checksum status: Unverified]
    Source: 172.27.2.4
    Destination: 172.24.2.5
Stream Control Transmission Protocol, Src Port: 30102 (30102), Dst Port: 57630 (57630)
    Source port: 30102
    Destination port: 57630
    Verification tag: 0xf3dc3087
    [Association index: 1]
    Checksum: 0x00000000 [unverified]
    [Checksum Status: Unverified]
    SACK chunk (Cumulative TSN: 3207583439, a_rwnd: 106483, gaps: 0, duplicate TSNs: 0)
        Chunk type: SACK (3)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x00
            .... ...0 = Nounce sum: 0
        Chunk length: 16
        Cumulative TSN ACK: 3207583439
            [Acknowledges TSN: 3207583439]
                [Acknowledges TSN in frame: 34]
                [The RTT since DATA was: 0.000042005 seconds]
        Advertised receiver window credit (a_rwnd): 106483
        Number of gap acknowledgement blocks: 0
        Number of duplicated TSNs: 0








### Connection teardown

Frame 36: 98 bytes on wire (784 bits), 98 bytes captured (784 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Jul 23, 2020 15:42:33.743280804 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1595518953.743280804 seconds
    [Time delta from previous captured frame: 10.281192149 seconds]
    [Time delta from previous displayed frame: 10.281192149 seconds]
    [Time since reference or first frame: 242.193318417 seconds]
    Frame Number: 36
    Frame Length: 98 bytes (784 bits)
    Capture Length: 98 bytes (784 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:sctp]
Ethernet II, Src: d6:96:23:1b:02:05 (d6:96:23:1b:02:05), Dst: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
    Destination: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        Address: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        Address: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 172.27.2.4, Dst: 172.24.2.5
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0x6d26 (27942)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: SCTP (132)
    Header checksum: 0x70c1 [validation disabled]
    [Header checksum status: Unverified]
    Source: 172.27.2.4
    Destination: 172.24.2.5
Stream Control Transmission Protocol, Src Port: 30102 (30102), Dst Port: 57630 (57630)
    Source port: 30102
    Destination port: 57630
    Verification tag: 0xf3dc3087
    [Association index: 1]
    Checksum: 0x00000000 [unverified]
    [Checksum Status: Unverified]
    HEARTBEAT chunk (Information: 48 bytes)
        Chunk type: HEARTBEAT (4)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x00
        Chunk length: 52
        Heartbeat info parameter (Information: 44 bytes)
            Parameter type: Heartbeat info (0x0001)
                0... .... .... .... = Bit: Stop processing of chunk
                .0.. .... .... .... = Bit: Do not report
            Parameter length: 48
            Heartbeat information: 0200e11eac18020500000000000000000000000000000000...

Frame 37: 98 bytes on wire (784 bits), 98 bytes captured (784 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Jul 23, 2020 15:42:33.745135942 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1595518953.745135942 seconds
    [Time delta from previous captured frame: 0.001855138 seconds]
    [Time delta from previous displayed frame: 0.001855138 seconds]
    [Time since reference or first frame: 242.195173555 seconds]
    Frame Number: 37
    Frame Length: 98 bytes (784 bits)
    Capture Length: 98 bytes (784 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:sctp]
Ethernet II, Src: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01), Dst: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
    Destination: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        Address: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        Address: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 172.24.2.5, Dst: 172.27.2.4
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 84
    Identification: 0x0002 (2)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 63
    Protocol: SCTP (132)
    Header checksum: 0xdee5 [validation disabled]
    [Header checksum status: Unverified]
    Source: 172.24.2.5
    Destination: 172.27.2.4
Stream Control Transmission Protocol, Src Port: 57630 (57630), Dst Port: 30102 (30102)
    Source port: 57630
    Destination port: 30102
    Verification tag: 0xaeacd509
    [Association index: 1]
    Checksum: 0x0818f127 [unverified]
    [Checksum Status: Unverified]
    HEARTBEAT_ACK chunk (Information: 48 bytes)
        Chunk type: HEARTBEAT_ACK (5)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x00
        Chunk length: 52
        Heartbeat info parameter (Information: 44 bytes)
            Parameter type: Heartbeat info (0x0001)
                0... .... .... .... = Bit: Stop processing of chunk
                .0.. .... .... .... = Bit: Do not report
            Parameter length: 48
            Heartbeat information: 0200e11eac18020500000000000000000000000000000000...

Frame 38: 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Jul 23, 2020 15:42:34.594482028 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1595518954.594482028 seconds
    [Time delta from previous captured frame: 0.849346086 seconds]
    [Time delta from previous displayed frame: 0.849346086 seconds]
    [Time since reference or first frame: 243.044519641 seconds]
    Frame Number: 38
    Frame Length: 54 bytes (432 bits)
    Capture Length: 54 bytes (432 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:sctp]
Ethernet II, Src: d6:96:23:1b:02:05 (d6:96:23:1b:02:05), Dst: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
    Destination: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        Address: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        Address: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 172.27.2.4, Dst: 172.24.2.5
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 40
    Identification: 0x6d27 (27943)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: SCTP (132)
    Header checksum: 0x70ec [validation disabled]
    [Header checksum status: Unverified]
    Source: 172.27.2.4
    Destination: 172.24.2.5
Stream Control Transmission Protocol, Src Port: 30102 (30102), Dst Port: 57630 (57630)
    Source port: 30102
    Destination port: 57630
    Verification tag: 0xf3dc3087
    [Association index: 1]
    Checksum: 0x00000000 [unverified]
    [Checksum Status: Unverified]
    SHUTDOWN chunk (Cumulative TSN ack: 3207583439)
        Chunk type: SHUTDOWN (7)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x00
        Chunk length: 8
        Cumulative TSN Ack: 3207583439

Frame 39: 50 bytes on wire (400 bits), 50 bytes captured (400 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Jul 23, 2020 15:42:34.594825489 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1595518954.594825489 seconds
    [Time delta from previous captured frame: 0.000343461 seconds]
    [Time delta from previous displayed frame: 0.000343461 seconds]
    [Time since reference or first frame: 243.044863102 seconds]
    Frame Number: 39
    Frame Length: 50 bytes (400 bits)
    Capture Length: 50 bytes (400 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:sctp]
Ethernet II, Src: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01), Dst: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
    Destination: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        Address: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        Address: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 172.24.2.5, Dst: 172.27.2.4
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 36
    Identification: 0x0003 (3)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 63
    Protocol: SCTP (132)
    Header checksum: 0xdf14 [validation disabled]
    [Header checksum status: Unverified]
    Source: 172.24.2.5
    Destination: 172.27.2.4
Stream Control Transmission Protocol, Src Port: 57630 (57630), Dst Port: 30102 (30102)
    Source port: 57630
    Destination port: 30102
    Verification tag: 0xaeacd509
    [Association index: 1]
    Checksum: 0xd7e61e5f [unverified]
    [Checksum Status: Unverified]
    SHUTDOWN_ACK chunk
        Chunk type: SHUTDOWN_ACK (8)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x00
        Chunk length: 4

Frame 40: 50 bytes on wire (400 bits), 50 bytes captured (400 bits) on interface 0
    Interface id: 0 (eth0)
        Interface name: eth0
    Encapsulation type: Ethernet (1)
    Arrival Time: Jul 23, 2020 15:42:34.594846074 UTC
    [Time shift for this packet: 0.000000000 seconds]
    Epoch Time: 1595518954.594846074 seconds
    [Time delta from previous captured frame: 0.000020585 seconds]
    [Time delta from previous displayed frame: 0.000020585 seconds]
    [Time since reference or first frame: 243.044883687 seconds]
    Frame Number: 40
    Frame Length: 50 bytes (400 bits)
    Capture Length: 50 bytes (400 bits)
    [Frame is marked: False]
    [Frame is ignored: False]
    [Protocols in frame: eth:ethertype:ip:sctp]
Ethernet II, Src: d6:96:23:1b:02:05 (d6:96:23:1b:02:05), Dst: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
    Destination: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        Address: 0a:58:ac:1b:02:01 (0a:58:ac:1b:02:01)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Source: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        Address: d6:96:23:1b:02:05 (d6:96:23:1b:02:05)
        .... ..1. .... .... .... .... = LG bit: Locally administered address (this is NOT the factory default)
        .... ...0 .... .... .... .... = IG bit: Individual address (unicast)
    Type: IPv4 (0x0800)
Internet Protocol Version 4, Src: 172.27.2.4, Dst: 172.24.2.5
    0100 .... = Version: 4
    .... 0101 = Header Length: 20 bytes (5)
    Differentiated Services Field: 0x02 (DSCP: CS0, ECN: ECT(0))
        0000 00.. = Differentiated Services Codepoint: Default (0)
        .... ..10 = Explicit Congestion Notification: ECN-Capable Transport codepoint '10' (2)
    Total Length: 36
    Identification: 0x6d28 (27944)
    Flags: 0x02 (Don't Fragment)
        0... .... = Reserved bit: Not set
        .1.. .... = Don't fragment: Set
        ..0. .... = More fragments: Not set
    Fragment offset: 0
    Time to live: 64
    Protocol: SCTP (132)
    Header checksum: 0x70ef [validation disabled]
    [Header checksum status: Unverified]
    Source: 172.27.2.4
    Destination: 172.24.2.5
Stream Control Transmission Protocol, Src Port: 30102 (30102), Dst Port: 57630 (57630)
    Source port: 30102
    Destination port: 57630
    Verification tag: 0xf3dc3087
    [Association index: 1]
    Checksum: 0x00000000 [unverified]
    [Checksum Status: Unverified]
    SHUTDOWN_COMPLETE chunk
        Chunk type: SHUTDOWN_COMPLETE (14)
            0... .... = Bit: Stop processing of the packet
            .0.. .... = Bit: Do not report
        Chunk flags: 0x00


^C
[root@openshift-jumpserver-0 ~]# 
~~~

## Resources ##

* [https://docs.openshift.com/container-platform/4.4/networking/using-sctp.html](https://docs.openshift.com/container-platform/4.4/networking/using-sctp.html)
* [https://en.wikipedia.org/wiki/Stream_Control_Transmission_Protocol](https://en.wikipedia.org/wiki/Stream_Control_Transmission_Protocol)
* [https://tools.ietf.org/html/rfc4960](https://tools.ietf.org/html/rfc4960)
