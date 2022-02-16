# Packet tracing with OVN

## Running various OVN traces in an ovn-kubernetes environment

### Environment

I use an [ovn-kubernetes](https://github.com/ovn-org/ovn-kubernetes) kind deployment as a base environment for the following traces.

I spawn an environment with:
~~~
contrib/kind.sh
~~~

Then, I deploy the following deployment and service:
~~~
# cat nginx.yaml 
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-deployment
  labels:
    app: nginx
spec:
  replicas: 1
  selector:
    matchLabels:
      app: nginx
  template:
    metadata:
      labels:
        app: nginx
    spec:
      containers:
      - name: nginx
        image: nginx:1.14.2
        ports:
        - containerPort: 80
# cat nginx-svc.yaml 
apiVersion: v1
kind: Service
metadata:
  name: nginx-np
spec:
  selector:
    app: nginx
  ports:
    - protocol: TCP
      port: 80
      targetPort: 80
      nodePort: 30007
  type:  NodePort
~~~

My pod is on ovn-worker, and the node's IP address is 172.18.0.3. The service can be reached on port 30007 and will then be NATed and forwarded to pod nginx-deployment-9456bbbf9-x6pc6 with IP 10.244.1.3:
~~~
[root@ovnkubernetes egressfw]# oc get svc -o wide
NAME         TYPE        CLUSTER-IP     EXTERNAL-IP   PORT(S)        AGE   SELECTOR
kubernetes   ClusterIP   10.96.0.1      <none>        443/TCP        84m   <none>
nginx-np     NodePort    10.96.91.159   <none>        80:30007/TCP   63m   app=nginx
[root@ovnkubernetes egressfw]# oc get pods -o wide
NAME                               READY   STATUS    RESTARTS   AGE   IP           NODE         NOMINATED NODE   READINESS GATES
nginx-deployment-9456bbbf9-x6pc6   1/1     Running   0          63m   10.244.1.3   ovn-worker   <none>           <none>
[root@ovnkubernetes egressfw]# oc get nodes -o wide
NAME                STATUS   ROLES                  AGE   VERSION   INTERNAL-IP   EXTERNAL-IP   OS-IMAGE       KERNEL-VERSION          CONTAINER-RUNTIME
ovn-control-plane   Ready    control-plane,master   84m   v1.23.3   172.18.0.3    <none>        Ubuntu 21.10   4.18.0-358.el8.x86_64   containerd://1.5.9
ovn-worker          Ready    <none>                 84m   v1.23.3   172.18.0.4    <none>        Ubuntu 21.10   4.18.0-358.el8.x86_64   containerd://1.5.9
ovn-worker2         Ready    <none>                 84m   v1.23.3   172.18.0.2    <none>        Ubuntu 21.10   4.18.0-358.el8.x86_64   containerd://1.5.9
~~~

We can run curl against this service from the next-hop gateway with IP address 172.18.0.1 and MAC address 02:42:7a:79:82:ca:
~~~
# ip a ls dev br-519277dba8fe
4: br-519277dba8fe: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default 
    link/ether 02:42:7a:79:82:ca brd ff:ff:ff:ff:ff:ff
    inet 172.18.0.1/16 brd 172.18.255.255 scope global br-519277dba8fe
       valid_lft forever preferred_lft forever
    inet6 fc00:f853:ccd:e793::1/64 scope global 
       valid_lft forever preferred_lft forever
    inet6 fe80::42:7aff:fe79:82ca/64 scope link 
       valid_lft forever preferred_lft forever
    inet6 fe80::1/64 scope link 
       valid_lft forever preferred_lft forever
# curl -I 172.18.0.4:30007 
HTTP/1.1 200 OK
Server: nginx/1.14.2
Date: Wed, 16 Feb 2022 11:32:44 GMT
Content-Type: text/html
Content-Length: 612
Last-Modified: Tue, 04 Dec 2018 14:44:49 GMT
Connection: keep-alive
ETag: "5c0692e1-264"
Accept-Ranges: bytes
~~~

The pod has the following IP address and gateway:
~~~
# ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
3: eth0@if7: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1400 qdisc noqueue state UP group default 
    link/ether 0a:58:0a:f4:01:03 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 10.244.1.3/24 brd 10.244.1.255 scope global eth0
       valid_lft forever preferred_lft forever
    inet6 fe80::858:aff:fef4:103/64 scope link 
       valid_lft forever preferred_lft forever
# ip r show default
default via 10.244.1.1 dev eth0
# ip neigh
10.244.1.1 dev eth0 lladdr 0a:58:0a:f4:01:01 STALE
~~~

And on the pod, we see the following flow which has gone through DNAT and SNAT:
~~~
# tcpdump -nne -i eth0
dropped privs to tcpdump
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on eth0, link-type EN10MB (Ethernet), capture size 262144 bytes
11:32:44.407949 0a:58:0a:f4:01:01 > 0a:58:0a:f4:01:03, ethertype IPv4 (0x0800), length 74: 100.64.0.3.53642 > 10.244.1.3.80: Flags [S], seq 2722623038, win 29200, options [mss 1460,sackOK,TS val 757469725 ecr 0,nop,wscale 7], length 0
11:32:44.408002 0a:58:0a:f4:01:03 > 0a:58:0a:f4:01:01, ethertype IPv4 (0x0800), length 74: 10.244.1.3.80 > 100.64.0.3.53642: Flags [S.], seq 519304349, ack 2722623039, win 26960, options [mss 1360,sackOK,TS val 1904257279 ecr 757469725,nop,wscale 7], length 0
11:32:44.409948 0a:58:0a:f4:01:01 > 0a:58:0a:f4:01:03, ethertype IPv4 (0x0800), length 147: 100.64.0.3.53642 > 10.244.1.3.80: Flags [P.], seq 1:82, ack 1, win 229, options [nop,nop,TS val 757469728 ecr 1904257279], length 81: HTTP: HEAD / HTTP/1.1
11:32:44.409953 0a:58:0a:f4:01:01 > 0a:58:0a:f4:01:03, ethertype IPv4 (0x0800), length 66: 100.64.0.3.53642 > 10.244.1.3.80: Flags [.], ack 1, win 229, options [nop,nop,TS val 757469728 ecr 1904257279], length 0
11:32:44.410001 0a:58:0a:f4:01:03 > 0a:58:0a:f4:01:01, ethertype IPv4 (0x0800), length 66: 10.244.1.3.80 > 100.64.0.3.53642: Flags [.], ack 82, win 211, options [nop,nop,TS val 1904257281 ecr 757469728], length 0
11:32:44.410031 0a:58:0a:f4:01:03 > 0a:58:0a:f4:01:01, ethertype IPv4 (0x0800), length 66: 10.244.1.3.80 > 100.64.0.3.53642: Flags [.], ack 82, win 211, options [nop,nop,TS val 1904257281 ecr 757469728], length 0
11:32:44.410399 0a:58:0a:f4:01:03 > 0a:58:0a:f4:01:01, ethertype IPv4 (0x0800), length 304: 10.244.1.3.80 > 100.64.0.3.53642: Flags [P.], seq 1:239, ack 82, win 211, options [nop,nop,TS val 1904257281 ecr 757469728], length 238: HTTP: HTTP/1.1 200 OK
11:32:44.410491 0a:58:0a:f4:01:01 > 0a:58:0a:f4:01:03, ethertype IPv4 (0x0800), length 66: 100.64.0.3.53642 > 10.244.1.3.80: Flags [.], ack 239, win 237, options [nop,nop,TS val 757469729 ecr 1904257281], length 0
11:32:44.410772 0a:58:0a:f4:01:01 > 0a:58:0a:f4:01:03, ethertype IPv4 (0x0800), length 66: 100.64.0.3.53642 > 10.244.1.3.80: Flags [F.], seq 82, ack 239, win 237, options [nop,nop,TS val 757469730 ecr 1904257281], length 0
11:32:44.411039 0a:58:0a:f4:01:03 > 0a:58:0a:f4:01:01, ethertype IPv4 (0x0800), length 66: 10.244.1.3.80 > 100.64.0.3.53642: Flags [F.], seq 239, ack 83, win 211, options [nop,nop,TS val 1904257282 ecr 757469730], length 0
11:32:44.411154 0a:58:0a:f4:01:01 > 0a:58:0a:f4:01:03, ethertype IPv4 (0x0800), length 66: 100.64.0.3.53642 > 10.244.1.3.80: Flags [.], ack 240, win 237, options [nop,nop,TS val 757469730 ecr 1904257282], length 0
^C
11 packets captured
11 packets received by filter
0 packets dropped by kernel
~~~

Here is an overview of the NBDB state:
~~~
sh-5.1# ovn-nbctl show
switch cd928d81-4f0f-4c04-860a-f75022f7e0b9 (ext_ovn-control-plane)
    port breth0_ovn-control-plane
        type: localnet
        addresses: ["unknown"]
    port etor-GR_ovn-control-plane
        type: router
        addresses: ["02:42:ac:12:00:03"]
        router-port: rtoe-GR_ovn-control-plane
switch 25b2a2f5-437f-4513-90cd-e3e1aaf7f14f (ovn-control-plane)
    port local-path-storage_local-path-provisioner-5ddd94ff66-krkk7
        addresses: ["0a:58:0a:f4:02:04 10.244.2.4"]
    port stor-ovn-control-plane
        type: router
        router-port: rtos-ovn-control-plane
    port k8s-ovn-control-plane
        addresses: ["82:24:48:8a:77:cc 10.244.2.2"]
    port kube-system_coredns-64897985d-wdvs9
        addresses: ["0a:58:0a:f4:02:05 10.244.2.5"]
    port kube-system_coredns-64897985d-rzjfr
        addresses: ["0a:58:0a:f4:02:03 10.244.2.3"]
switch b4dede7d-7660-42bf-a954-c5b3f0f28a7c (ovn-worker)
    port stor-ovn-worker
        type: router
        router-port: rtos-ovn-worker
    port default_nginx-deployment-9456bbbf9-x6pc6
        addresses: ["0a:58:0a:f4:01:03 10.244.1.3"]
    port k8s-ovn-worker
        addresses: ["4a:d4:28:63:24:1c 10.244.1.2"]
switch 214f4d51-750d-414a-aad1-94e84e623f9b (ext_ovn-worker)
    port breth0_ovn-worker
        type: localnet
        addresses: ["unknown"]
    port etor-GR_ovn-worker
        type: router
        addresses: ["02:42:ac:12:00:04"]
        router-port: rtoe-GR_ovn-worker
switch bcec069e-f578-4927-93d2-6e129595a716 (join)
    port jtor-GR_ovn-worker
        type: router
        router-port: rtoj-GR_ovn-worker
    port jtor-GR_ovn-control-plane
        type: router
        router-port: rtoj-GR_ovn-control-plane
    port jtor-GR_ovn-worker2
        type: router
        router-port: rtoj-GR_ovn-worker2
    port jtor-ovn_cluster_router
        type: router
        router-port: rtoj-ovn_cluster_router
switch ff675836-2b46-4d83-b53f-41cba7297d8b (ext_ovn-worker2)
    port breth0_ovn-worker2
        type: localnet
        addresses: ["unknown"]
    port etor-GR_ovn-worker2
        type: router
        addresses: ["02:42:ac:12:00:02"]
        router-port: rtoe-GR_ovn-worker2
switch e1f67383-f731-4bf7-a018-6361ec60f477 (ovn-worker2)
    port k8s-ovn-worker2
        addresses: ["4e:19:7f:89:93:bc 10.244.0.2"]
    port stor-ovn-worker2
        type: router
        router-port: rtos-ovn-worker2
router 7072022c-7226-4928-a397-fc770b75f98f (GR_ovn-worker2)
    port rtoj-GR_ovn-worker2
        mac: "0a:58:64:40:00:02"
        networks: ["100.64.0.2/16"]
    port rtoe-GR_ovn-worker2
        mac: "02:42:ac:12:00:02"
        networks: ["172.18.0.2/16"]
    nat d87cc0f7-5053-4739-9617-a952cb37d74c
        external ip: "172.18.0.2"
        logical ip: "10.244.0.0/16"
        type: "snat"
router 5ea87f54-ceeb-442a-92c8-4b522278b5dd (GR_ovn-worker)
    port rtoj-GR_ovn-worker
        mac: "0a:58:64:40:00:03"
        networks: ["100.64.0.3/16"]
    port rtoe-GR_ovn-worker
        mac: "02:42:ac:12:00:04"
        networks: ["172.18.0.4/16"]
    nat ca3ef3e0-22c1-427d-a977-b10256a96177
        external ip: "172.18.0.4"
        logical ip: "10.244.0.0/16"
        type: "snat"
router 1dead945-8773-4d33-b56b-67e5ab8fcca3 (GR_ovn-control-plane)
    port rtoe-GR_ovn-control-plane
        mac: "02:42:ac:12:00:03"
        networks: ["172.18.0.3/16"]
    port rtoj-GR_ovn-control-plane
        mac: "0a:58:64:40:00:04"
        networks: ["100.64.0.4/16"]
    nat 70a6067c-ffa4-407a-9d0f-cc581b8b1c3a
        external ip: "172.18.0.3"
        logical ip: "10.244.0.0/16"
        type: "snat"
router 9ac72c76-9f8f-4f3a-9a82-0e18846c4cd9 (ovn_cluster_router)
    port rtos-ovn-control-plane
        mac: "0a:58:0a:f4:02:01"
        networks: ["10.244.2.1/24"]
        gateway chassis: [9268048a-65bf-467e-b166-d0faffafa686]
    port rtos-ovn-worker2
        mac: "0a:58:0a:f4:00:01"
        networks: ["10.244.0.1/24"]
        gateway chassis: [fbb949c1-6d62-44cc-b67e-9f93b40d1691]
    port rtos-ovn-worker
        mac: "0a:58:0a:f4:01:01"
        networks: ["10.244.1.1/24"]
        gateway chassis: [4da007a6-ed1d-45ff-bcf4-62ae8914bbbc]
    port rtoj-ovn_cluster_router
        mac: "0a:58:64:40:00:01"
        networks: ["100.64.0.1/16"]
~~~

The service is implemented as an OVN loadbalancer - we are going to look into this load-balancer further in the examples section below:
~~~
# ovn-nbctl lr-lb-list GR_ovn-worker
UUID                                    LB                  PROTO      VIP                 IPs
bc20db47-e79c-4906-b0d8-dd2dcfaf54b4    Service_default/    tcp        172.18.0.4:30007    10.244.1.3:80
e7486241-66e1-453a-b44d-3b3e7b2ebc4f    Service_default/    tcp        10.96.0.1:443       172.18.0.3:6443
~~~

### Collection of examples

#### Simulating new flows through conntrack

Tracing a new TCP flow from the outside of the cluster to the service IP and port, knowing that NAT and conntrack are involved. A `--minimal` trace is always a good starting point. The most important bit here is the `--ct new` command line parameter to simulate a new conntrack flow:
~~~
# ovn-trace --minimal  --ovs ext_ovn-worker 'inport == "breth0_ovn-worker" && eth.src == 02:42:7a:79:82:ca &&  eth.dst == 02:42:ac:12:00:04 && ip4.src == 172.18.0.1 && ip4.dst == 172.18.0.4 && ip.ttl == 64 && tcp.src==12345 && tcp.dst == 30007' --ct new
~~~

Result of the `--minimal` trace:
~~~
sh-5.1# ovn-trace --minimal  --ovs ext_ovn-worker 'inport == "breth0_ovn-worker" && eth.src == 02:42:7a:79:82:ca &&  eth.dst == 02:42:ac:12:00:04 && ip4.src == 172.18.0.1 && ip4.dst == 172.18.0.4 && ip.ttl == 64 && tcp.src==12345 && tcp.dst == 30007' --ct new
# tcp,reg14=0x1,vlan_tci=0x0000,dl_src=02:42:7a:79:82:ca,dl_dst=02:42:ac:12:00:04,nw_src=172.18.0.1,nw_dst=172.18.0.4,nw_tos=0,nw_ecn=0,nw_ttl=64,tp_src=12345,tp_dst=30007,tcp_flags=0
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
ct_dnat /* assuming no un-dnat entry, so no change */ {
    *** no OpenFlow flows;
    ct_lb /* default (use --ct to customize) */ {
        *** no OpenFlow flows;
        ip.ttl--;
        eth.src = 0a:58:64:40:00:03;
        eth.dst = 0a:58:64:40:00:01;
        ct_dnat /* assuming no un-dnat entry, so no change */ {
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            ip.ttl--;
            eth.src = 0a:58:0a:f4:01:01;
            eth.dst = 0a:58:0a:f4:01:03;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            reg0[6] = 0;
            *** chk_lb_hairpin_reply action not implemented;
            reg0[12] = 0;
            *** no OpenFlow flows;
            ct_lb /* default (use --ct to customize) */ {
                *** no OpenFlow flows;
                *** no OpenFlow flows;
                output("default_nginx-deployment-9456bbbf9-x6pc6");
            };
        };
    };
};
~~~

In the detailed trace, we can also see the destination IP after the DNAT:
~~~
sh-5.1# ovn-trace --friendly-names  --ovs ext_ovn-worker 'inport == "breth0_ovn-worker" && eth.src == 02:42:7a:79:82:ca &&  eth.dst == 02:42:ac:12:00:04 && ip4.src == 172.18.0.1 && ip4.dst == 172.18.0.4 && ip.ttl == 64 && tcp.src==12345 && tcp.dst == 30007' --ct new | tail -n 15

ct_lb /* default (use --ct to customize) */
-------------------------------------------
 3. ls_out_acl_hint (northd.c:6041): ct.est && ct_label.blocked == 0, priority 1, uuid 5f2e1810
    cookie=0x5f2e1810, duration=5184.167s, table=43, n_packets=0, n_bytes=0, idle_age=5184, priority=1,ct_state=+est+trk,ct_label=0/0x1,metadata=0x5 actions=set_field:0x400000000000000000000000000/0x400000000000000000000000000->xxreg0,resubmit(,44)
    cookie=0x5f2e1810, duration=5184.149s, table=43, n_packets=28886, n_bytes=7169503, idle_age=1, priority=1,ct_state=+est+trk,ct_label=0/0x1,metadata=0x3 actions=set_field:0x400000000000000000000000000/0x400000000000000000000000000->xxreg0,resubmit(,44)
    reg0[10] = 1;
    next;
 8. ls_out_port_sec_ip (northd.c:5130): outport == "default_nginx-deployment-9456bbbf9-x6pc6" && eth.dst == 0a:58:0a:f4:01:03 && ip4.dst == {255.255.255.255, 224.0.0.0/4, 10.244.1.3}, priority 90, uuid 701b3cf6
    *** no OpenFlow flows
    next;
 9. ls_out_port_sec_l2 (northd.c:5594): outport == "default_nginx-deployment-9456bbbf9-x6pc6" && eth.dst == {0a:58:0a:f4:01:03}, priority 50, uuid b6daec2d
    *** no OpenFlow flows
    output;
    /* output to "default_nginx-deployment-9456bbbf9-x6pc6", type "" */
~~~

Unfortunately, it is very difficult to see what the final packet would look like. We know that an SNAT is involved, too, but that is "hidden" somewhere in the output:
~~~
sh-5.1# ovn-trace --friendly-names  --ovs ext_ovn-worker 'inport == "breth0_ovn-worker" && eth.src == 02:42:7a:79:82:ca &&  eth.dst == 02:42:ac:12:00:04 && ip4.src == 172.18.0.1 && ip4.dst == 172.18.0.4 && ip.ttl == 64 && tcp.src==12345 && tcp.dst == 30007' --ct new | grep 100.64     
    reg0 = 100.64.0.1;
    reg1 = 100.64.0.3;
~~~

It would indeed be desirable for ovn-trace to show the full packet state of the processed and ready-to-send packet.

If we look at the flow in a bit more detail, we can see that in the `ingress` pipeline of `GR_ovn-worker`, the packet hits the load-balancer flow. SNAT is forced for this load-balancer, and the packet shall be sent to backend `10.244.1.3:80`:
~~~
# ovn-trace --ct new --ct new --ct new  --ovs ext_ovn-worker 'inport == "breth0_ovn-worker" && eth.src == 02:42:7a:79:82:ca &&  eth.dst == 02:42:ac:12:00:04 && ip4.src == 172.18.0.1 && ip4.dst == 172.18.0.4 && ip.ttl == 64 && tcp.src==12345 && tcp.dst == 30007' 
(...)
ingress(dp="GR_ovn-worker", inport="rtoe-GR_ovn-worker")
--------------------------------------------------------
 0. lr_in_admission (northd.c:10523): eth.dst == 02:42:ac:12:00:04 && inport == "rtoe-GR_ovn-worker", priority 50, uuid 71cc3085
    *** no OpenFlow flows
    xreg0[0..47] = 02:42:ac:12:00:04;
    next;
 1. lr_in_lookup_neighbor (northd.c:10667): 1, priority 0, uuid 39881020
    cookie=0x39881020, duration=19439.530s, table=9, n_packets=71802, n_bytes=13904424, idle_age=0, priority=0,metadata=0x1 actions=set_field:0x4/0x4->xreg4,resubmit(,10)
    cookie=0x39881020, duration=19439.497s, table=9, n_packets=29937, n_bytes=2833154, idle_age=0, priority=0,metadata=0x9 actions=set_field:0x4/0x4->xreg4,resubmit(,10)
    reg9[2] = 1;
    next;
 2. lr_in_learn_neighbor (northd.c:10676): reg9[2] == 1 || reg9[3] == 0, priority 100, uuid 6be9d860
    cookie=0x6be9d860, duration=19439.530s, table=10, n_packets=71802, n_bytes=13904424, idle_age=0, priority=100,reg9=0/0x8,metadata=0x1 actions=resubmit(,11)
    cookie=0x6be9d860, duration=19439.496s, table=10, n_packets=29949, n_bytes=2833922, idle_age=0, priority=100,reg9=0/0x8,metadata=0x9 actions=resubmit(,11)
    cookie=0x6be9d860, duration=19439.497s, table=10, n_packets=689, n_bytes=30478, idle_age=150, priority=100,reg9=0x4/0x4,metadata=0x9 actions=resubmit(,11)
    cookie=0x6be9d860, duration=19439.530s, table=10, n_packets=0, n_bytes=0, idle_age=19439, priority=100,reg9=0x4/0x4,metadata=0x1 actions=resubmit(,11)
    next;
 4. lr_in_unsnat (northd.c:9670): ip4 && ip4.dst == 172.18.0.4 && tcp && tcp.dst == 30007, priority 120, uuid 60d3c124
    *** no OpenFlow flows
    next;
 5. lr_in_defrag (northd.c:9865): ip && ip4.dst == 172.18.0.4 && tcp, priority 110, uuid 4bb8c7f6
    *** no OpenFlow flows
    reg0 = 172.18.0.4;
    reg9[16..31] = tcp.dst;
    ct_dnat;

ct_dnat /* assuming no un-dnat entry, so no change */
-----------------------------------------------------
 6. lr_in_dnat (northd.c:9701): ct.new && ip4 && reg0 == 172.18.0.4 && tcp && reg9[16..31] == 30007, priority 120, uuid 25f1ad55
    *** no OpenFlow flows
    flags.force_snat_for_lb = 1;
    ct_lb(backends=10.244.1.3:80);

ct_lb
-----
10. lr_in_ip_routing_pre (northd.c:10910): 1, priority 0, uuid 219bc578
    cookie=0x219bc578, duration=19439.530s, table=18, n_packets=71797, n_bytes=13903874, idle_age=0, priority=0,metadata=0x1 actions=set_field:0/0xffffffff->xxreg1,resubmit(,19)
    cookie=0x219bc578, duration=19439.496s, table=18, n_packets=29925, n_bytes=2831554, idle_age=0, priority=0,metadata=0x9 actions=set_field:0/0xffffffff->xxreg1,resubmit(,19)
    reg7 = 0;
    next;
11. lr_in_ip_routing (northd.c:9439): reg7 == 0 && ip4.dst == 10.244.0.0/16, priority 49, uuid fbba9ce1
    *** no OpenFlow flows
    ip.ttl--;
    reg8[0..15] = 0;
    reg0 = 100.64.0.1;
    reg1 = 100.64.0.3;
    eth.src = 0a:58:64:40:00:03;
    outport = "rtoj-GR_ovn-worker";
    flags.loopback = 1;
    next;
~~~

We can find the same flow here inside the OVN Southbound database flows:
~~~
# ovn-sbctl dump-flows 
(...)
Datapath: "GR_ovn-worker" (15e72dff-677d-4794-8f70-a5fea49ad788)  Pipeline: ingress
(...)
  table=6 (lr_in_dnat         ), priority=120  , match=(ct.new && ip4 && reg0 == 172.18.0.4 && tcp && reg9[16..31] == 30007), action=(flags.force_snat_for_lb = 1; ct_lb(backends=10.244.1.3:80);)
(...)
  table=11(lr_in_ip_routing   ), priority=49   , match=(reg7 == 0 && ip4.dst == 10.244.0.0/16), action=(ip.ttl--; reg8[0..15] = 0; reg0 = 100.64.0.1; reg1 = 100.64.0.3; eth.src = 0a:58:64:40:00:03; outport = "rtoj-GR_ovn-worker"; flags.loopback = 1; next;)
(...)
~~~

#### Specifying multiple `--ct` actions

From the ovn-trace man page:
~~~
            A packet might pass through the connection tracker twice in
            one trip through OVN: once following egress from a VM as it
            passes outward through a firewall, and once preceding
            ingress to a second VM as it passes inward through a
            firewall. Use multiple --ct options to specify the flags for
            multiple ct_next actions.
~~~

Compare the following outputs - note that there is a difference with every `--ct new` set:
~~~
sh-5.1# ovn-trace --minimal  --ovs ext_ovn-worker 'inport == "breth0_ovn-worker" && eth.src == 02:42:7a:79:82:ca &&  eth.dst == 02:42:ac:12:00:04 && ip4.src == 172.18.0.1 && ip4.dst == 172.18.0.4 && ip.ttl == 64 && tcp.src==12345 && tcp.dst == 30007'              
# tcp,reg14=0x1,vlan_tci=0x0000,dl_src=02:42:7a:79:82:ca,dl_dst=02:42:ac:12:00:04,nw_src=172.18.0.1,nw_dst=172.18.0.4,nw_tos=0,nw_ecn=0,nw_ttl=64,tp_src=12345,tp_dst=30007,tcp_flags=0
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
ct_dnat /* assuming no un-dnat entry, so no change */ /* default (use --ct to customize) */ {
    *** no OpenFlow flows;
    ip.ttl--;
    eth.src = 02:42:ac:12:00:04;
    *** no OpenFlow flows;
};
sh-5.1# ovn-trace --ct new --minimal  --ovs ext_ovn-worker 'inport == "breth0_ovn-worker" && eth.src == 02:42:7a:79:82:ca &&  eth.dst == 02:42:ac:12:00:04 && ip4.src == 172.18.0.1 && ip4.dst == 172.18.0.4 && ip.ttl == 64 && tcp.src==12345 && tcp.dst == 30007' 
# tcp,reg14=0x1,vlan_tci=0x0000,dl_src=02:42:7a:79:82:ca,dl_dst=02:42:ac:12:00:04,nw_src=172.18.0.1,nw_dst=172.18.0.4,nw_tos=0,nw_ecn=0,nw_ttl=64,tp_src=12345,tp_dst=30007,tcp_flags=0
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
ct_dnat /* assuming no un-dnat entry, so no change */ {
    *** no OpenFlow flows;
    ct_lb /* default (use --ct to customize) */ {
        *** no OpenFlow flows;
        ip.ttl--;
        eth.src = 0a:58:64:40:00:03;
        eth.dst = 0a:58:64:40:00:01;
        ct_dnat /* assuming no un-dnat entry, so no change */ {
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            ip.ttl--;
            eth.src = 0a:58:0a:f4:01:01;
            eth.dst = 0a:58:0a:f4:01:03;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            reg0[6] = 0;
            *** chk_lb_hairpin_reply action not implemented;
            reg0[12] = 0;
            *** no OpenFlow flows;
            ct_lb /* default (use --ct to customize) */ {
                *** no OpenFlow flows;
                *** no OpenFlow flows;
                output("default_nginx-deployment-9456bbbf9-x6pc6");
            };
        };
    };
};
sh-5.1# ovn-trace --ct new --ct new --minimal  --ovs ext_ovn-worker 'inport == "breth0_ovn-worker" && eth.src == 02:42:7a:79:82:ca &&  eth.dst == 02:42:ac:12:00:04 && ip4.src == 172.18.0.1 && ip4.dst == 172.18.0.4 && ip.ttl == 64 && tcp.src==12345 && tcp.dst == 30007' 
# tcp,reg14=0x1,vlan_tci=0x0000,dl_src=02:42:7a:79:82:ca,dl_dst=02:42:ac:12:00:04,nw_src=172.18.0.1,nw_dst=172.18.0.4,nw_tos=0,nw_ecn=0,nw_ttl=64,tp_src=12345,tp_dst=30007,tcp_flags=0
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
ct_dnat /* assuming no un-dnat entry, so no change */ {
    *** no OpenFlow flows;
    ct_lb {
        *** no OpenFlow flows;
        ip.ttl--;
        eth.src = 0a:58:64:40:00:03;
        eth.dst = 0a:58:64:40:00:01;
        ct_dnat /* assuming no un-dnat entry, so no change */ {
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            ip.ttl--;
            eth.src = 0a:58:0a:f4:01:01;
            eth.dst = 0a:58:0a:f4:01:03;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            reg0[6] = 0;
            *** chk_lb_hairpin_reply action not implemented;
            reg0[12] = 0;
            *** no OpenFlow flows;
            ct_lb /* default (use --ct to customize) */ {
                *** no OpenFlow flows;
                *** no OpenFlow flows;
                output("default_nginx-deployment-9456bbbf9-x6pc6");
            };
        };
    };
};
sh-5.1# ovn-trace --ct new --ct new --ct new --minimal  --ovs ext_ovn-worker 'inport == "breth0_ovn-worker" && eth.src == 02:42:7a:79:82:ca &&  eth.dst == 02:42:ac:12:00:04 && ip4.src == 172.18.0.1 && ip4.dst == 172.18.0.4 && ip.ttl == 64 && tcp.src==12345 && tcp.dst == 30007' 
# tcp,reg14=0x1,vlan_tci=0x0000,dl_src=02:42:7a:79:82:ca,dl_dst=02:42:ac:12:00:04,nw_src=172.18.0.1,nw_dst=172.18.0.4,nw_tos=0,nw_ecn=0,nw_ttl=64,tp_src=12345,tp_dst=30007,tcp_flags=0
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
ct_dnat /* assuming no un-dnat entry, so no change */ {
    *** no OpenFlow flows;
    ct_lb {
        *** no OpenFlow flows;
        ip.ttl--;
        eth.src = 0a:58:64:40:00:03;
        eth.dst = 0a:58:64:40:00:01;
        ct_dnat /* assuming no un-dnat entry, so no change */ {
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            ip.ttl--;
            eth.src = 0a:58:0a:f4:01:01;
            eth.dst = 0a:58:0a:f4:01:03;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            reg0[6] = 0;
            *** chk_lb_hairpin_reply action not implemented;
            reg0[12] = 0;
            *** no OpenFlow flows;
            ct_lb {
                *** no OpenFlow flows;
                *** no OpenFlow flows;
                output("default_nginx-deployment-9456bbbf9-x6pc6");
            };
        };
    };
};
sh-5.1# 
~~~

#### Specifying TCP flags in ovn-trace

It is also possible to provide the exact TCP flags. See RFC 793 for further details:
~~~
  Control Bits:  6 bits (from left to right):

    URG:  Urgent Pointer field significant
    ACK:  Acknowledgment field significant
    PSH:  Push Function
    RST:  Reset the connection
    SYN:  Synchronize sequence numbers
    FIN:  No more data from sender
~~~

A FIN is hence `0x1`, a SYN 0x2, a RST is 0x4, a PSH is 0x8, an ACK is 0x10, a SYN/ACK is 0x12 and so on.

The resulting ovn-trace command:
~~~
# ovn-trace --minimal  --ovs ext_ovn-worker 'inport == "breth0_ovn-worker" && eth.src == 02:42:7a:79:82:ca &&  eth.dst == 02:42:ac:12:00:04 && ip4.src == 172.18.0.1 && ip4.dst == 172.18.0.4 && ip.ttl == 64 && tcp.src==12345 && tcp.dst == 30007 && tcp.flags == 0x2' --ct new 
# tcp,reg14=0x1,vlan_tci=0x0000,dl_src=02:42:7a:79:82:ca,dl_dst=02:42:ac:12:00:04,nw_src=172.18.0.1,nw_dst=172.18.0.4,nw_tos=0,nw_ecn=0,nw_ttl=64,tp_src=12345,tp_dst=30007,tcp_flags=syn
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
ct_dnat /* assuming no un-dnat entry, so no change */ {
    *** no OpenFlow flows;
    ct_lb /* default (use --ct to customize) */ {
        *** no OpenFlow flows;
        ip.ttl--;
        eth.src = 0a:58:64:40:00:03;
        eth.dst = 0a:58:64:40:00:01;
        ct_dnat /* assuming no un-dnat entry, so no change */ {
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            ip.ttl--;
            eth.src = 0a:58:0a:f4:01:01;
            eth.dst = 0a:58:0a:f4:01:03;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            *** no OpenFlow flows;
            reg0[6] = 0;
            *** chk_lb_hairpin_reply action not implemented;
            reg0[12] = 0;
            *** no OpenFlow flows;
            ct_lb /* default (use --ct to customize) */ {
                *** no OpenFlow flows;
                *** no OpenFlow flows;
                output("default_nginx-deployment-9456bbbf9-x6pc6");
            };
        };
    };
};
~~~

#### Simulating reverse flows through conntrack

Let's continue with the earlier example. This time, let's simulate the the answer to the earlier flow. Unfortunately, something seems to go wrong:
~~~
# ovn-trace --ct rpl,est --minimal  --ovs ovn-worker 'inport == "default_nginx-deployment-9456bbbf9-x6pc6" && eth.src == 0a:58:0a:f4:01:03 &&  eth.dst == 0a:58:0a:f4:01:01 && ip4.src == 10.244.1.3 && ip4.dst == 100.64.0.3 && ip.ttl == 64 && tcp.src==80 && tcp.dst == 47306' 
# tcp,reg14=0x3,vlan_tci=0x0000,dl_src=0a:58:0a:f4:01:03,dl_dst=0a:58:0a:f4:01:01,nw_src=10.244.1.3,nw_dst=100.64.0.3,nw_tos=0,nw_ecn=0,nw_ttl=64,tp_src=80,tp_dst=47306,tcp_flags=0
*** no OpenFlow flows;
*** no OpenFlow flows;
ct_lb {
    reg0[6] = 0;
    *** chk_lb_hairpin_reply action not implemented;
    reg0[12] = 0;
    *** no OpenFlow flows;
    *** no OpenFlow flows;
    *** no OpenFlow flows;
    *** no OpenFlow flows;
    *** no OpenFlow flows;
    ip.ttl--;
    eth.src = 0a:58:64:40:00:01;
    eth.dst = 0a:58:64:40:00:03;
    *** no OpenFlow flows;
    *** no OpenFlow flows;
    *** no OpenFlow flows;
    *** no OpenFlow flows;
};
~~~

What happens if we run a detailed trace:
~~~
# ovn-trace --ct rpl,est --ovs ovn-worker 'inport == "default_nginx-deployment-9456bbbf9-x6pc6" && eth.src == 0a:58:0a:f4:01:03 &&  eth.dst == 0a:58:0a:f4:01:01 && ip4.src == 10.244.1.3 && ip4.dst == 100.64.0.3 && ip.ttl == 64 && tcp.src==80 && tcp.dst == 47306' 
# tcp,reg14=0x3,vlan_tci=0x0000,dl_src=0a:58:0a:f4:01:03,dl_dst=0a:58:0a:f4:01:01,nw_src=10.244.1.3,nw_dst=100.64.0.3,nw_tos=0,nw_ecn=0,nw_ttl=64,tp_src=80,tp_dst=47306,tcp_flags=0

ingress(dp="ovn-worker", inport="default_nginx-deployment-9456bbbf9-x6pc6")
---------------------------------------------------------------------------
 0. ls_in_port_sec_l2 (northd.c:5497): inport == "default_nginx-deployment-9456bbbf9-x6pc6" && eth.src == {0a:58:0a:f4:01:03}, priority 50, uuid 7fade81c
    *** no OpenFlow flows
    next;
 1. ls_in_port_sec_ip (northd.c:5130): inport == "default_nginx-deployment-9456bbbf9-x6pc6" && eth.src == 0a:58:0a:f4:01:03 && ip4.src == {10.244.1.3}, priority 90, uuid 4d81359d
    *** no OpenFlow flows
    next;
 5. ls_in_pre_acl (northd.c:5758): ip, priority 100, uuid 7d029aba
    cookie=0x7d029aba, duration=22959.183s, table=13, n_packets=0, n_bytes=0, idle_age=22959, priority=100,ipv6,metadata=0x3 actions=set_field:0x1000000000000000000000000/0x1000000000000000000000000->xxreg0,resubmit(,14)
    cookie=0x7d029aba, duration=22958.578s, table=13, n_packets=0, n_bytes=0, idle_age=22958, priority=100,ip,metadata=0x5 actions=set_field:0x1000000000000000000000000/0x1000000000000000000000000->xxreg0,resubmit(,14)
    cookie=0x7d029aba, duration=22958.578s, table=13, n_packets=0, n_bytes=0, idle_age=22958, priority=100,ipv6,metadata=0x5 actions=set_field:0x1000000000000000000000000/0x1000000000000000000000000->xxreg0,resubmit(,14)
    cookie=0x7d029aba, duration=22959.182s, table=13, n_packets=260424, n_bytes=52306797, idle_age=0, priority=100,ip,metadata=0x3 actions=set_field:0x1000000000000000000000000/0x1000000000000000000000000->xxreg0,resubmit(,14)
    reg0[0] = 1;
    next;
 6. ls_in_pre_lb (northd.c:5890): ip, priority 100, uuid 8a8d61ca
    cookie=0x8a8d61ca, duration=22959.233s, table=14, n_packets=0, n_bytes=0, idle_age=22959, priority=100,ipv6,metadata=0x5 actions=set_field:0x4000000000000000000000000/0x4000000000000000000000000->xxreg0,resubmit(,15)
    cookie=0x8a8d61ca, duration=22959.204s, table=14, n_packets=260424, n_bytes=52306797, idle_age=0, priority=100,ip,metadata=0x3 actions=set_field:0x4000000000000000000000000/0x4000000000000000000000000->xxreg0,resubmit(,15)
    cookie=0x8a8d61ca, duration=22959.204s, table=14, n_packets=0, n_bytes=0, idle_age=22959, priority=100,ipv6,metadata=0x3 actions=set_field:0x4000000000000000000000000/0x4000000000000000000000000->xxreg0,resubmit(,15)
    cookie=0x8a8d61ca, duration=22959.233s, table=14, n_packets=0, n_bytes=0, idle_age=22959, priority=100,ip,metadata=0x5 actions=set_field:0x4000000000000000000000000/0x4000000000000000000000000->xxreg0,resubmit(,15)
    reg0[2] = 1;
    next;
 7. ls_in_pre_stateful (northd.c:5917): reg0[2] == 1 && ip4 && tcp, priority 120, uuid 01fdf58b
    cookie=0x1fdf58b, duration=22959.233s, table=15, n_packets=0, n_bytes=0, idle_age=22959, priority=120,tcp,reg0=0x4/0x4,metadata=0x5 actions=move:NXM_OF_IP_DST[]->NXM_NX_XXREG0[64..95],move:NXM_OF_TCP_DST[]->NXM_NX_XXREG0[32..47],ct(table=16,zone=NXM_NX_REG13[0..15],nat)
    cookie=0x1fdf58b, duration=22959.184s, table=15, n_packets=0, n_bytes=0, idle_age=22959, priority=120,tcp,reg0=0x4/0x4,metadata=0x8 actions=move:NXM_OF_IP_DST[]->NXM_NX_XXREG0[64..95],move:NXM_OF_TCP_DST[]->NXM_NX_XXREG0[32..47],ct(table=16,zone=NXM_NX_REG13[0..15],nat)
    cookie=0x1fdf58b, duration=22959.204s, table=15, n_packets=260424, n_bytes=52306797, idle_age=0, priority=120,tcp,reg0=0x4/0x4,metadata=0x3 actions=move:NXM_OF_IP_DST[]->NXM_NX_XXREG0[64..95],move:NXM_OF_TCP_DST[]->NXM_NX_XXREG0[32..47],ct(table=16,zone=NXM_NX_REG13[0..15],nat)
    cookie=0x1fdf58b, duration=22959.232s, table=15, n_packets=0, n_bytes=0, idle_age=22959, priority=120,tcp,reg0=0x4/0x4,metadata=0x2 actions=move:NXM_OF_IP_DST[]->NXM_NX_XXREG0[64..95],move:NXM_OF_TCP_DST[]->NXM_NX_XXREG0[32..47],ct(table=16,zone=NXM_NX_REG13[0..15],nat)
    reg1 = ip4.dst;
    reg2[0..15] = tcp.dst;
    ct_lb;

ct_lb
-----
 8. ls_in_acl_hint (northd.c:6041): ct.est && ct_label.blocked == 0, priority 1, uuid a17a896f
    cookie=0xa17a896f, duration=22959.233s, table=16, n_packets=0, n_bytes=0, idle_age=22959, priority=1,ct_state=+est+trk,ct_label=0/0x1,metadata=0x5 actions=set_field:0x400000000000000000000000000/0x400000000000000000000000000->xxreg0,resubmit(,17)
    cookie=0xa17a896f, duration=22959.204s, table=16, n_packets=128010, n_bytes=31684495, idle_age=0, priority=1,ct_state=+est+trk,ct_label=0/0x1,metadata=0x3 actions=set_field:0x400000000000000000000000000/0x400000000000000000000000000->xxreg0,resubmit(,17)
    reg0[10] = 1;
    next;
 9. ls_in_acl (northd.c:6471): ct.est && !ct.rel && !ct.new && !ct.inv && ct.rpl && ct_label.blocked == 0, priority 65532, uuid 5ae98b31
    cookie=0x5ae98b31, duration=22959.233s, table=17, n_packets=0, n_bytes=0, idle_age=22959, priority=65532,ct_state=-new+est-rel+rpl-inv+trk,ct_label=0/0x1,metadata=0x5 actions=resubmit(,18)
    cookie=0x5ae98b31, duration=22959.215s, table=17, n_packets=128010, n_bytes=31684495, idle_age=0, priority=65532,ct_state=-new+est-rel+rpl-inv+trk,ct_label=0/0x1,metadata=0x3 actions=resubmit(,18)
    next;
13. ls_in_pre_hairpin (northd.c:6801): ip && ct.trk, priority 100, uuid 2520b1a4
    cookie=0x2520b1a4, duration=22959.233s, table=21, n_packets=0, n_bytes=0, idle_age=22959, priority=100,ct_state=+trk,ip,metadata=0x5 actions=set_field:0/0x80->reg10,resubmit(,68),move:NXM_NX_REG10[7]->NXM_NX_XXREG0[102],set_field:0/0x80->reg10,resubmit(,69),move:NXM_NX_REG10[7]->NXM_NX_XXREG0[108],resubmit(,22)
    cookie=0x2520b1a4, duration=22959.215s, table=21, n_packets=0, n_bytes=0, idle_age=22959, priority=100,ct_state=+trk,ipv6,metadata=0x3 actions=set_field:0/0x80->reg10,resubmit(,68),move:NXM_NX_REG10[7]->NXM_NX_XXREG0[102],set_field:0/0x80->reg10,resubmit(,69),move:NXM_NX_REG10[7]->NXM_NX_XXREG0[108],resubmit(,22)
    cookie=0x2520b1a4, duration=22959.215s, table=21, n_packets=251254, n_bytes=51628217, idle_age=0, priority=100,ct_state=+trk,ip,metadata=0x3 actions=set_field:0/0x80->reg10,resubmit(,68),move:NXM_NX_REG10[7]->NXM_NX_XXREG0[102],set_field:0/0x80->reg10,resubmit(,69),move:NXM_NX_REG10[7]->NXM_NX_XXREG0[108],resubmit(,22)
    cookie=0x2520b1a4, duration=22959.233s, table=21, n_packets=0, n_bytes=0, idle_age=22959, priority=100,ct_state=+trk,ipv6,metadata=0x5 actions=set_field:0/0x80->reg10,resubmit(,68),move:NXM_NX_REG10[7]->NXM_NX_XXREG0[102],set_field:0/0x80->reg10,resubmit(,69),move:NXM_NX_REG10[7]->NXM_NX_XXREG0[108],resubmit(,22)
    reg0[6] = chk_lb_hairpin();
    reg0[12] = chk_lb_hairpin_reply();
    *** chk_lb_hairpin_reply action not implemented
    next;
22. ls_in_l2_lkup (northd.c:8292): eth.dst == 0a:58:0a:f4:01:01, priority 50, uuid e7b71eee
    *** no OpenFlow flows
    outport = "stor-ovn-worker";
    output;

egress(dp="ovn-worker", inport="default_nginx-deployment-9456bbbf9-x6pc6", outport="stor-ovn-worker")
-----------------------------------------------------------------------------------------------------
 0. ls_out_pre_lb (northd.c:5647): ip && outport == "stor-ovn-worker", priority 110, uuid a85134e9
    *** no OpenFlow flows
    next;
 1. ls_out_pre_acl (northd.c:5647): ip && outport == "stor-ovn-worker", priority 110, uuid 473dc0a5
    *** no OpenFlow flows
    next;
 3. ls_out_acl_hint (northd.c:6041): ct.est && ct_label.blocked == 0, priority 1, uuid 5f2e1810
    cookie=0x5f2e1810, duration=22959.234s, table=43, n_packets=0, n_bytes=0, idle_age=22959, priority=1,ct_state=+est+trk,ct_label=0/0x1,metadata=0x5 actions=set_field:0x400000000000000000000000000/0x400000000000000000000000000->xxreg0,resubmit(,44)
    cookie=0x5f2e1810, duration=22959.216s, table=43, n_packets=128010, n_bytes=31684495, idle_age=0, priority=1,ct_state=+est+trk,ct_label=0/0x1,metadata=0x3 actions=set_field:0x400000000000000000000000000/0x400000000000000000000000000->xxreg0,resubmit(,44)
    reg0[10] = 1;
    next;
 4. ls_out_acl (northd.c:6473): ct.est && !ct.rel && !ct.new && !ct.inv && ct.rpl && ct_label.blocked == 0, priority 65532, uuid 72e85294
    cookie=0x72e85294, duration=22959.234s, table=44, n_packets=0, n_bytes=0, idle_age=22959, priority=65532,ct_state=-new+est-rel+rpl-inv+trk,ct_label=0/0x1,metadata=0x5 actions=resubmit(,45)
    cookie=0x72e85294, duration=22959.216s, table=44, n_packets=128010, n_bytes=31684495, idle_age=0, priority=65532,ct_state=-new+est-rel+rpl-inv+trk,ct_label=0/0x1,metadata=0x3 actions=resubmit(,45)
    next;
 9. ls_out_port_sec_l2 (northd.c:5594): outport == "stor-ovn-worker", priority 50, uuid ae50e49e
    *** no OpenFlow flows
    output;
    /* output to "stor-ovn-worker", type "patch" */

ingress(dp="ovn_cluster_router", inport="rtos-ovn-worker")
----------------------------------------------------------
 0. lr_in_admission (northd.c:10523): eth.dst == 0a:58:0a:f4:01:01 && inport == "rtos-ovn-worker" && is_chassis_resident("cr-rtos-ovn-worker"), priority 50, uuid 16833f18
    *** no OpenFlow flows
    xreg0[0..47] = 0a:58:0a:f4:01:01;
    next;
 1. lr_in_lookup_neighbor (northd.c:10667): 1, priority 0, uuid 39881020
    cookie=0x39881020, duration=22959.217s, table=9, n_packets=84769, n_bytes=16419002, idle_age=0, priority=0,metadata=0x1 actions=set_field:0x4/0x4->xreg4,resubmit(,10)
    cookie=0x39881020, duration=22959.184s, table=9, n_packets=34573, n_bytes=3262795, idle_age=0, priority=0,metadata=0x9 actions=set_field:0x4/0x4->xreg4,resubmit(,10)
    reg9[2] = 1;
    next;
 2. lr_in_learn_neighbor (northd.c:10676): reg9[2] == 1 || reg9[3] == 0, priority 100, uuid 6be9d860
    cookie=0x6be9d860, duration=22959.217s, table=10, n_packets=84769, n_bytes=16419002, idle_age=0, priority=100,reg9=0/0x8,metadata=0x1 actions=resubmit(,11)
    cookie=0x6be9d860, duration=22959.183s, table=10, n_packets=34585, n_bytes=3263563, idle_age=0, priority=100,reg9=0/0x8,metadata=0x9 actions=resubmit(,11)
    cookie=0x6be9d860, duration=22959.184s, table=10, n_packets=869, n_bytes=39094, idle_age=29, priority=100,reg9=0x4/0x4,metadata=0x9 actions=resubmit(,11)
    cookie=0x6be9d860, duration=22959.217s, table=10, n_packets=0, n_bytes=0, idle_age=22959, priority=100,reg9=0x4/0x4,metadata=0x1 actions=resubmit(,11)
    next;
10. lr_in_ip_routing_pre (northd.c:10910): 1, priority 0, uuid 219bc578
    cookie=0x219bc578, duration=22959.217s, table=18, n_packets=84764, n_bytes=16418452, idle_age=0, priority=0,metadata=0x1 actions=set_field:0/0xffffffff->xxreg1,resubmit(,19)
    cookie=0x219bc578, duration=22959.183s, table=18, n_packets=34561, n_bytes=3261195, idle_age=0, priority=0,metadata=0x9 actions=set_field:0/0xffffffff->xxreg1,resubmit(,19)
    reg7 = 0;
    next;
11. lr_in_ip_routing (northd.c:9439): reg7 == 0 && ip4.dst == 100.64.0.3/32, priority 97, uuid 4b1e23e5
    cookie=0x4b1e23e5, duration=22959.217s, table=19, n_packets=0, n_bytes=0, idle_age=22959, priority=97,ip,reg7=0,metadata=0x1,nw_dst=100.64.0.3 actions=dec_ttl(),set_field:0/0xffff00000000->xreg4,set_field:0x64400003000000000000000000000000/0xffffffff000000000000000000000000->xxreg0,set_field:0x644000010000000000000000/0xffffffff0000000000000000->xxreg0,set_field:0a:58:64:40:00:01->eth_src,set_field:0x1->reg15,set_field:0x1/0x1->reg10,resubmit(,20)
    ip.ttl--;
    reg8[0..15] = 0;
    reg0 = 100.64.0.3;
    reg1 = 100.64.0.1;
    eth.src = 0a:58:64:40:00:01;
    outport = "rtoj-ovn_cluster_router";
    flags.loopback = 1;
    next;
12. lr_in_ip_routing_ecmp (northd.c:10985): reg8[0..15] == 0, priority 150, uuid f65895a6
    cookie=0xf65895a6, duration=22959.217s, table=20, n_packets=84764, n_bytes=16418452, idle_age=0, priority=150,reg8=0/0xffff,metadata=0x1 actions=resubmit(,21)
    cookie=0xf65895a6, duration=22959.186s, table=20, n_packets=21, n_bytes=2064, idle_age=14155, priority=150,reg8=0/0xffff,metadata=0x9 actions=resubmit(,21)
    next;
13. lr_in_policy (northd.c:8672): ip4.src == 10.244.0.0/16 && ip4.dst == 100.64.0.0/16, priority 101, uuid f385bfe8
    cookie=0xf385bfe8, duration=22959.218s, table=21, n_packets=0, n_bytes=0, idle_age=22959, priority=101,ip,metadata=0x1,nw_src=10.244.0.0/16,nw_dst=100.64.0.0/16 actions=set_field:0/0xffff00000000->xreg4,resubmit(,22)
    reg8[0..15] = 0;
    next;
14. lr_in_policy_ecmp (northd.c:11120): reg8[0..15] == 0, priority 150, uuid e3a1301b
    cookie=0xe3a1301b, duration=22959.217s, table=22, n_packets=84764, n_bytes=16418452, idle_age=0, priority=150,reg8=0/0xffff,metadata=0x1 actions=resubmit(,23)
    cookie=0xe3a1301b, duration=22959.186s, table=22, n_packets=21, n_bytes=2064, idle_age=14155, priority=150,reg8=0/0xffff,metadata=0x9 actions=resubmit(,23)
    next;
15. lr_in_arp_resolve (northd.c:11506): outport == "rtoj-ovn_cluster_router" && reg0 == 100.64.0.3, priority 100, uuid 33f04862
    cookie=0x33f04862, duration=22959.218s, table=23, n_packets=0, n_bytes=0, idle_age=22959, priority=100,reg0=0x64400003,reg15=0x1,metadata=0x1 actions=set_field:0a:58:64:40:00:03->eth_dst,resubmit(,24)
    eth.dst = 0a:58:64:40:00:03;
    next;
19. lr_in_arp_request (northd.c:11801): 1, priority 0, uuid 0abbeff8
    cookie=0xabbeff8, duration=22959.218s, table=27, n_packets=84764, n_bytes=16418452, idle_age=0, priority=0,metadata=0x1 actions=resubmit(,37)
    cookie=0xabbeff8, duration=22959.184s, table=27, n_packets=20, n_bytes=1990, idle_age=14155, priority=0,metadata=0x9 actions=resubmit(,37)
    output;

egress(dp="ovn_cluster_router", inport="rtos-ovn-worker", outport="rtoj-ovn_cluster_router")
--------------------------------------------------------------------------------------------
 0. lr_out_chk_dnat_local (northd.c:13026): 1, priority 0, uuid a02f42a1
    cookie=0xa02f42a1, duration=22959.218s, table=40, n_packets=84752, n_bytes=16417482, idle_age=0, priority=0,metadata=0x1 actions=set_field:0/0x10->xreg4,resubmit(,41)
    cookie=0xa02f42a1, duration=22959.184s, table=40, n_packets=465, n_bytes=21988, idle_age=29, priority=0,metadata=0x9 actions=set_field:0/0x10->xreg4,resubmit(,41)
    reg9[4] = 0;
    next;
 6. lr_out_delivery (northd.c:11848): outport == "rtoj-ovn_cluster_router", priority 100, uuid 5428c7e9
    cookie=0x5428c7e9, duration=22959.218s, table=46, n_packets=0, n_bytes=0, idle_age=22959, priority=100,reg15=0x1,metadata=0x1 actions=resubmit(,64)
    output;
    /* output to "rtoj-ovn_cluster_router", type "patch" */

ingress(dp="join", inport="jtor-ovn_cluster_router")
----------------------------------------------------
 0. ls_in_port_sec_l2 (northd.c:5497): inport == "jtor-ovn_cluster_router", priority 50, uuid 485af0cc
    cookie=0x485af0cc, duration=22959.234s, table=8, n_packets=0, n_bytes=0, idle_age=22959, priority=50,reg14=0x1,metadata=0x2 actions=resubmit(,9)
    next;
 6. ls_in_pre_lb (northd.c:5644): ip && inport == "jtor-ovn_cluster_router", priority 110, uuid 61c2f8b4
    cookie=0x61c2f8b4, duration=22959.234s, table=14, n_packets=0, n_bytes=0, idle_age=22959, priority=110,ipv6,reg14=0x1,metadata=0x2 actions=resubmit(,15)
    cookie=0x61c2f8b4, duration=22959.234s, table=14, n_packets=0, n_bytes=0, idle_age=22959, priority=110,ip,reg14=0x1,metadata=0x2 actions=resubmit(,15)
    next;
22. ls_in_l2_lkup (northd.c:8292): eth.dst == 0a:58:64:40:00:03, priority 50, uuid 4ec697e3
    cookie=0x4ec697e3, duration=22959.234s, table=30, n_packets=0, n_bytes=0, idle_age=22959, priority=50,metadata=0x2,dl_dst=0a:58:64:40:00:03 actions=set_field:0x2->reg15,resubmit(,37)
    outport = "jtor-GR_ovn-worker";
    output;

egress(dp="join", inport="jtor-ovn_cluster_router", outport="jtor-GR_ovn-worker")
---------------------------------------------------------------------------------
 0. ls_out_pre_lb (northd.c:5647): ip && outport == "jtor-GR_ovn-worker", priority 110, uuid ff3a234e
    *** no OpenFlow flows
    next;
 9. ls_out_port_sec_l2 (northd.c:5594): outport == "jtor-GR_ovn-worker", priority 50, uuid 65252832
    *** no OpenFlow flows
    output;
    /* output to "jtor-GR_ovn-worker", type "l3gateway" */

ingress(dp="GR_ovn-worker", inport="rtoj-GR_ovn-worker")
--------------------------------------------------------
 0. lr_in_admission (northd.c:10523): eth.dst == 0a:58:64:40:00:03 && inport == "rtoj-GR_ovn-worker", priority 50, uuid edfaf0af
    *** no OpenFlow flows
    xreg0[0..47] = 0a:58:64:40:00:03;
    next;
 1. lr_in_lookup_neighbor (northd.c:10667): 1, priority 0, uuid 39881020
    cookie=0x39881020, duration=22959.218s, table=9, n_packets=84769, n_bytes=16419002, idle_age=0, priority=0,metadata=0x1 actions=set_field:0x4/0x4->xreg4,resubmit(,10)
    cookie=0x39881020, duration=22959.185s, table=9, n_packets=34573, n_bytes=3262795, idle_age=0, priority=0,metadata=0x9 actions=set_field:0x4/0x4->xreg4,resubmit(,10)
    reg9[2] = 1;
    next;
 2. lr_in_learn_neighbor (northd.c:10676): reg9[2] == 1 || reg9[3] == 0, priority 100, uuid 6be9d860
    cookie=0x6be9d860, duration=22959.219s, table=10, n_packets=84769, n_bytes=16419002, idle_age=0, priority=100,reg9=0/0x8,metadata=0x1 actions=resubmit(,11)
    cookie=0x6be9d860, duration=22959.185s, table=10, n_packets=34585, n_bytes=3263563, idle_age=0, priority=100,reg9=0/0x8,metadata=0x9 actions=resubmit(,11)
    cookie=0x6be9d860, duration=22959.186s, table=10, n_packets=869, n_bytes=39094, idle_age=29, priority=100,reg9=0x4/0x4,metadata=0x9 actions=resubmit(,11)
    cookie=0x6be9d860, duration=22959.219s, table=10, n_packets=0, n_bytes=0, idle_age=22959, priority=100,reg9=0x4/0x4,metadata=0x1 actions=resubmit(,11)
    next;
 4. lr_in_unsnat (northd.c:10378): inport == "rtoj-GR_ovn-worker" && ip4.dst == 100.64.0.3, priority 110, uuid e548aaeb
    *** no OpenFlow flows
    ct_snat;
~~~

We can see that we hit `ct_snat` in the `ingress` pipeline of `GR_ovn-worker`:
~~~
# ovn-sbctl dump-flows
(...)
Datapath: "GR_ovn-worker" (15e72dff-677d-4794-8f70-a5fea49ad788)  Pipeline: ingress
(...)
  table=4 (lr_in_unsnat       ), priority=110  , match=(inport == "rtoj-GR_ovn-worker" && ip4.dst == 100.64.0.3), action=(ct_snat;)
(...)
~~~

Now, here, we hit an unfortunate shortcoming of ovn-trace - it does not what to do due to missing conntrack entries, and thus on a gatway router it will treat `ct_snat` as if this was a noop:
~~~
      This action distinguishes between gateway routers and distributed
      routers.  A gateway router is defined as a logical datapath that contains
      an <code>l3gateway</code> port; any other logical datapath is a
      distributed router.  On a gateway router, <code>ct_snat;</code> is
      treated as a no-op.  On a distributed router, it is treated the same way
      as <code>ct_dnat;</code>.
~~~
> [https://github.com/ovn-org/ovn/blob/ed81be75e8b3b33745eeb9b6ce2686b87ef72cd0/utilities/ovn-trace.8.xml#L242](https://github.com/ovn-org/ovn/blob/ed81be75e8b3b33745eeb9b6ce2686b87ef72cd0/utilities/ovn-trace.8.xml#L242)

For our example, this means that the processing stops here, and that we have to figure out ourselves what would happen here. We know that the loadbalancer should rewrite the DNAT and SNAT that was done initially, and we can simply run a new trace. First, though, we have to find out from where.

We know that the packet will go out of `GR_ovn-worker` and from port `rtoe-GR_ovn-worker`:
~~~
# ovn-nbctl show
(...)
switch 214f4d51-750d-414a-aad1-94e84e623f9b (ext_ovn-worker)
    port breth0_ovn-worker
        type: localnet
        addresses: ["unknown"]
    port etor-GR_ovn-worker
        type: router
        addresses: ["02:42:ac:12:00:04"]
        router-port: rtoe-GR_ovn-worker
(...)
router 5ea87f54-ceeb-442a-92c8-4b522278b5dd (GR_ovn-worker)
    port rtoj-GR_ovn-worker
        mac: "0a:58:64:40:00:03"
        networks: ["100.64.0.3/16"]
    port rtoe-GR_ovn-worker
        mac: "02:42:ac:12:00:04"
        networks: ["172.18.0.4/16"]
    nat ca3ef3e0-22c1-427d-a977-b10256a96177
        external ip: "172.18.0.4"
        logical ip: "10.244.0.0/16"
        type: "snat"
(...)
~~~

Let's find the switch port that matches this router port:
~~~
sh-5.1# ovn-nbctl find logical_switch_port "options={router-port=rtoe-GR_ovn-worker}"
_uuid               : 2b6e5ae2-a67f-4526-8b53-316a265a1acc
addresses           : ["02:42:ac:12:00:04"]
dhcpv4_options      : []
dhcpv6_options      : []
dynamic_addresses   : []
enabled             : []
external_ids        : {}
ha_chassis_group    : []
name                : etor-GR_ovn-worker
options             : {router-port=rtoe-GR_ovn-worker}
parent_name         : []
port_security       : []
tag                 : []
tag_request         : []
type                : router
up                  : true
~~~

And now, we can craft the ovn-trace command that shows our packet is indeed leaving on `breth0_ovn-worker`:
~~~
sh-5.1# ovn-trace --ct est,rpl --ovs ext_ovn-worker 'inport == "etor-GR_ovn-worker" && eth.src == 02:42:ac:12:00:04 &&  eth.dst == 02:42:7a:79:82:ca && ip4.src == 171.18.0.4 && ip4.dst == 171.18.0.1 && ip.ttl == 64 && tcp.src==30007 && tcp.dst == 47306'  --minimal
# tcp,reg14=0x2,vlan_tci=0x0000,dl_src=02:42:ac:12:00:04,dl_dst=02:42:7a:79:82:ca,nw_src=171.18.0.4,nw_dst=171.18.0.1,nw_tos=0,nw_ecn=0,nw_ttl=64,tp_src=30007,tp_dst=47306,tcp_flags=0
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
*** no OpenFlow flows;
output("breth0_ovn-worker");
sh-5.1# 
~~~

## Sources and references

Man pages:

* [https://man7.org/linux/man-pages/man8/ovn-trace.8.html](https://man7.org/linux/man-pages/man8/ovn-trace.8.html)
* [https://man7.org/linux/man-pages/man5/ovn-sb.5.html](https://man7.org/linux/man-pages/man5/ovn-sb.5.html)

Some discussion around ovn-trace:

* [https://www.mail-archive.com/ovs-discuss@openvswitch.org/msg07214.html](https://www.mail-archive.com/ovs-discuss@openvswitch.org/msg07214.html)

Examples for ovn-trace commands can be found inside the OVN test cases:

* [https://www.mail-archive.com/ovs-dev@openvswitch.org/msg55608.html](https://www.mail-archive.com/ovs-dev@openvswitch.org/msg55608.html)
* Simple test cases with `--ct new`: [https://github.com/ovn-org/ovn/blob/main/tests/ovn-northd.at](https://github.com/ovn-org/ovn/blob/main/tests/ovn-northd.at)
* More complex test cases: [https://github.com/ovn-org/ovn/blob/main/tests/ovn.at](https://github.com/ovn-org/ovn/blob/main/tests/ovn.at)

RFCs:

* [https://datatracker.ietf.org/doc/html/rfc793#section-3.1](https://datatracker.ietf.org/doc/html/rfc793#section-3.1)
