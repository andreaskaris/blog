~~~
[root@openshift-master-0 ~]# cat /etc/systemd/system/kubelet.service
[Unit]
Description=Kubernetes Kubelet
Wants=rpc-statd.service network-online.target crio.service
After=network-online.target crio.service

[Service]
Type=notify
ExecStartPre=/bin/mkdir --parents /etc/kubernetes/manifests
ExecStartPre=/bin/rm -f /var/lib/kubelet/cpu_manager_state
EnvironmentFile=/etc/os-release
EnvironmentFile=-/etc/kubernetes/kubelet-workaround
EnvironmentFile=-/etc/kubernetes/kubelet-env

ExecStart=/usr/bin/hyperkube \
    kubelet \
      --config=/etc/kubernetes/kubelet.conf \
      --bootstrap-kubeconfig=/etc/kubernetes/kubeconfig \
      --kubeconfig=/var/lib/kubelet/kubeconfig \
      --container-runtime=remote \
      --container-runtime-endpoint=/var/run/crio/crio.sock \
      --runtime-cgroups=/system.slice/crio.service \
      --node-labels=node-role.kubernetes.io/master,node.openshift.io/os_id=${ID} \
      --node-ip=${KUBELET_NODE_IPS} \
      --minimum-container-ttl-duration=6m0s \
      --cloud-provider= \
      --volume-plugin-dir=/etc/kubernetes/kubelet-plugins/volume/exec \
       \
      --register-with-taints=node-role.kubernetes.io/master=:NoSchedule \
      --pod-infra-container-image=quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:9519ae9a0a3e262e311c7f12a08adb2568e29e1576d2c6c229fd5d355c551d4b \
      --v=${KUBELET_LOG_LEVEL}

Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
~~~

~~~
[root@openshift-master-0 ~]# cat /etc/systemd/system/kubelet.service.d/20-nodenet.conf 
[Service]
Environment="KUBELET_NODE_IP=192.168.123.200" "KUBELET_NODE_IPS=192.168.123.200,fc00::fc1c:1e22:b052:ef48"
[root@openshift-master-0 ~]# cat /etc/systemd/system/nodeip-configuration.service
[Unit]
Description=Writes IP address configuration so that kubelet and crio services select a valid node IP
Wants=network-online.target
After=network-online.target ignition-firstboot-complete.service
Before=kubelet.service crio.service

[Service]
# Need oneshot to delay kubelet
Type=oneshot
# Would prefer to do Restart=on-failure instead of this bash retry loop, but
# the version of systemd we have right now doesn't support it. It should be
# available in systemd v244 and higher.
ExecStart=/bin/bash -c " \
  until \
  /usr/bin/podman run --rm \
  --authfile /var/lib/kubelet/config.json \
  --net=host \
  --volume /etc/systemd/system:/etc/systemd/system:z \
  quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:b1e1542aa0934233fd1515872d2e1be4f1f1e5ce0c8d35860eb2847badd3c609 \
  node-ip \
  set --retry-on-failure; \
  do \
  sleep 5; \
  done"

[Install]
RequiredBy=kubelet.service
~~~

~~~
[root@openshift-master-0 ~]# /bin/bash -c " \
>   until \
>   /usr/bin/podman run --rm \
>   --authfile /var/lib/kubelet/config.json \
>   --net=host \
>   --volume /etc/systemd/system:/etc/systemd/system:z \
>   quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:b1e1542aa0934233fd1515872d2e1be4f1f1e5ce0c8d35860eb2847badd3c609 \
>   node-ip \
>   set --retry-on-failure; \
>   do \
>   sleep 5; \
>   done"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::c67d:23db:c725:39ac/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::b88b:5cff:feba:1a5f/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::8439:b2ff:fe8c:447/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address 169.254.0.1/20 ovn-k8s-gw0"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::858:a9ff:fefe:1/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::d0ce:52ff:fe6b:3fb3/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::5003:cdff:fee0:bb89/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::7ca8:80ff:fefb:c27c/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::5018:baff:fefc:7af7/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::d4:3dff:fecf:b3d9/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::4015:fcff:fe4d:4478/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::9c5d:12ff:fea7:a3f1/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::8002:8cff:fefe:62d9/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::a82e:27ff:fea7:32bf/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::ac39:45ff:fe8e:9b34/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::3048:40ff:fe62:c808/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::b4d8:1fff:fe2b:38f/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::c0df:53ff:febd:561a/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::783d:fcff:feb3:4faa/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::80cc:9bff:feb7:155f/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::f8e5:3cff:fe26:84bc/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::1873:4fff:fed1:8873/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::fcfc:cff:fe45:aa0d/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::8c1f:3dff:fe3b:d9a2/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::541b:34ff:feed:2260/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::28d4:ceff:fe39:c312/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::a82b:88ff:fe7d:287b/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::98d1:d3ff:fe26:9d4b/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::acb4:a3ff:fe7d:5bf2/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::2c9e:5dff:fe90:83e4/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::1cde:13ff:fe6d:be66/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::88d6:41ff:fe5b:31bb/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::5cd2:f5ff:fef6:1674/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::c8b:61ff:fe4a:1c77/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::6c57:cbff:feed:fb0c/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::9c1e:c8ff:fe07:952c/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::5c15:46ff:feaf:c7c9/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::68bd:69ff:fef0:2ed4/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::38cf:70ff:febd:6d2a/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::fc6f:14ff:fe3a:9eff/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::dc4d:1bff:fe47:6f1a/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::8c6b:94ff:fe85:5608/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::b469:8eff:fee6:e641/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::a4b0:3aff:fe7b:3101/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::947f:36ff:fece:dee5/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::8cd1:4bff:fef9:e396/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::7089:27ff:fe86:9a6f/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::fc01:10ff:fe46:5bf0/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::203c:6bff:fe7b:e7d5/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::3840:b4ff:fed7:873c/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::3474:feff:fefd:837e/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::9ca5:f7ff:fe2b:9d1a/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::507f:58ff:fe45:d20f/64"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered address fe80::18b8:32ff:fef6:849a/64"
time="2021-01-31T16:40:16Z" level=debug msg="retrieved Address map map[0xc00021ab40:[127.0.0.1/8 lo ::1/128] 0xc00021afc0:[192.168.123.200/24 br-ex fc00::fc1c:1e22:b052:ef48/64] 0xc00021b200:[172.24.0.2/23 ovn-k8s-mp0 fd01:0:0:1::2/64] 0xc00021b440:[fd99::1/64]]"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 9 Dst: 169.254.0.0/20 Src: 169.254.0.1 Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 7 Dst: 172.24.0.0/23 Src: 172.24.0.2 Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 7 Dst: 172.24.0.0/14 Src: <nil> Gw: 172.24.0.1 Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 7 Dst: 172.30.0.0/16 Src: <nil> Gw: 172.24.0.1 Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 5 Dst: 192.168.123.0/24 Src: 192.168.123.200 Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 1 Dst: ::1/128 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 5 Dst: fc00::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 7 Dst: fd01:0:0:1::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 7 Dst: fd01::/48 Src: <nil> Gw: fd01:0:0:1::1 Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 7 Dst: fd02::/112 Src: <nil> Gw: fd01:0:0:1::1 Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 9 Dst: fd99::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 5 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 7 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 8 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 9 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 11 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 12 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 13 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 14 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 15 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 16 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 17 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 18 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 19 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 20 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 23 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 21 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 22 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 24 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 25 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 26 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 27 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 28 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 29 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 30 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 31 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 32 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 33 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 34 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 35 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 36 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 37 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 38 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 39 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 40 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 41 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 42 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 43 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 44 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 45 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 46 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 47 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 48 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 49 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 51 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 52 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 53 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 54 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 55 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 56 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 57 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 58 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 59 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Ignoring filtered route {Ifindex: 60 Dst: fe80::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
time="2021-01-31T16:40:16Z" level=debug msg="Retrieved route map map[5:[{Ifindex: 5 Dst: <nil> Src: <nil> Gw: 192.168.123.1 Flags: [] Table: 254} {Ifindex: 5 Dst: <nil> Src: <nil> Gw: fe80::5054:ff:fede:44c Flags: [] Table: 254}]]"
time="2021-01-31T16:40:16Z" level=debug msg="Address 192.168.123.200/24 br-ex is on interface br-ex with default route"
time="2021-01-31T16:40:16Z" level=debug msg="Address fc00::fc1c:1e22:b052:ef48/64 is on interface br-ex with default route"
time="2021-01-31T16:40:16Z" level=info msg="Chosen Node IPs: [192.168.123.200 fc00::fc1c:1e22:b052:ef48]"
time="2021-01-31T16:40:16Z" level=info msg="Opening Kubelet service override path /etc/systemd/system/kubelet.service.d/20-nodenet.conf"
time="2021-01-31T16:40:16Z" level=info msg="Writing Kubelet service override with content [Service]\nEnvironment=\"KUBELET_NODE_IP=192.168.123.200\" \"KUBELET_NODE_IPS=192.168.123.200,fc00::fc1c:1e22:b052:ef48\"\n"
time="2021-01-31T16:40:16Z" level=info msg="Opening CRI-O service override path /etc/systemd/system/crio.service.d/20-nodenet.conf"
time="2021-01-31T16:40:16Z" level=info msg="Writing CRI-O service override with content [Service]\nEnvironment=\"CONTAINER_STREAM_ADDRESS=192.168.123.200\"\n"
^C[root@openshift-master-0 ~]# 
[root@openshift-master-0 ~]# 
[root@openshift-master-0 ~]# 
[root@openshift-master-0 ~]# 
~~~


https://github.com/openshift/cluster-network-operator/blob/c33d0b72a98f1fc022e34e93bc1a37b1d66757c3/pkg/network/ovn_kubernetes.go#L336



https://developer.gnome.org/NetworkManager/stable/settings-ipv6.html



ot@openshift-master-0 ~]# journalctl | grep fc00
Jan 31 15:54:50 localhost kernel: BIOS-e820: [mem 0x000000000009fc00-0x000000000009ffff] reserved
Jan 31 15:54:50 localhost kernel: BIOS-e820: [mem 0x00000000feffc000-0x00000000feffffff] reserved
Jan 31 15:54:50 localhost kernel: BIOS-e820: [mem 0x00000000fffc0000-0x00000000ffffffff] reserved
Jan 31 15:54:50 localhost kernel: PM: Registered nosave memory: [mem 0xfeffc000-0xfeffffff]
Jan 31 15:54:50 localhost kernel: PM: Registered nosave memory: [mem 0xfffc0000-0xffffffff]
Jan 31 15:54:50 localhost kernel: pci 0000:00:02.0: reg 0x10: [mem 0xfc000000-0xfdffffff pref]
Jan 31 15:54:50 localhost kernel: e820: reserve RAM buffer [mem 0x0009fc00-0x0009ffff]
Jan 31 15:55:24 openshift-master-0 bash[1737]: time="2021-01-31T15:55:24Z" level=debug msg="retrieved Address map map[0xc0002827e0:[127.0.0.1/8 lo ::1/128] 0xc000282900:[192.168.123.200/24 ens3 fc00::5c77:e4fe:8950:c79c/64]]"
Jan 31 15:55:24 openshift-master-0 bash[1737]: time="2021-01-31T15:55:24Z" level=debug msg="Ignoring filtered route {Ifindex: 2 Dst: fc00::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
Jan 31 15:55:24 openshift-master-0 bash[1737]: time="2021-01-31T15:55:24Z" level=debug msg="Address fc00::5c77:e4fe:8950:c79c/64 is on interface ens3 with default route"
Jan 31 15:55:24 openshift-master-0 bash[1737]: time="2021-01-31T15:55:24Z" level=info msg="Chosen Node IPs: [192.168.123.200 fc00::5c77:e4fe:8950:c79c]"
Jan 31 15:55:24 openshift-master-0 bash[1737]: time="2021-01-31T15:55:24Z" level=info msg="Writing Kubelet service override with content [Service]\nEnvironment=\"KUBELET_NODE_IP=192.168.123.200\" \"KUBELET_NODE_IPS=192.168.123.200,fc00::5c77:e4fe:8950:c79c\"\n"
Jan 31 15:56:27 localhost kernel: BIOS-e820: [mem 0x000000000009fc00-0x000000000009ffff] reserved
Jan 31 15:56:27 localhost kernel: BIOS-e820: [mem 0x00000000feffc000-0x00000000feffffff] reserved
Jan 31 15:56:27 localhost kernel: BIOS-e820: [mem 0x00000000fffc0000-0x00000000ffffffff] reserved
Jan 31 15:56:27 localhost kernel: PM: Registered nosave memory: [mem 0xfeffc000-0xfeffffff]
Jan 31 15:56:27 localhost kernel: PM: Registered nosave memory: [mem 0xfffc0000-0xffffffff]
Jan 31 15:56:27 localhost kernel: pci 0000:00:02.0: reg 0x10: [mem 0xfc000000-0xfdffffff pref]
Jan 31 15:56:27 localhost kernel: e820: reserve RAM buffer [mem 0x0009fc00-0x0009ffff]
Jan 31 15:56:46 openshift-master-0 bash[1652]: time="2021-01-31T15:56:46Z" level=debug msg="retrieved Address map map[0xc0002906c0:[127.0.0.1/8 lo ::1/128] 0xc000290b40:[192.168.123.200/24 br-ex fc00::fc1c:1e22:b052:ef48/64]]"
Jan 31 15:56:46 openshift-master-0 bash[1652]: time="2021-01-31T15:56:46Z" level=debug msg="Ignoring filtered route {Ifindex: 5 Dst: fc00::/64 Src: <nil> Gw: <nil> Flags: [] Table: 254}"
Jan 31 15:56:46 openshift-master-0 bash[1652]: time="2021-01-31T15:56:46Z" level=debug msg="Address fc00::fc1c:1e22:b052:ef48/64 is on interface br-ex with default route"
Jan 31 15:56:46 openshift-master-0 bash[1652]: time="2021-01-31T15:56:46Z" level=info msg="Chosen Node IPs: [192.168.123.200 fc00::fc1c:1e22:b052:ef48]"
Jan 31 15:56:46 openshift-master-0 bash[1652]: time="2021-01-31T15:56:46Z" level=info msg="Writing Kubelet service override with content [Service]\nEnvironment=\"KUBELET_NODE_IP=192.168.123.200\" \"KUBELET_NODE_IPS=192.168.123.200,fc00::fc1c:1e22:b052:ef48\"\n"
Jan 31 15:56:46 openshift-master-0 hyperkube[1838]: I0131 15:56:46.668044    1838 flags.go:59] FLAG: --node-ip="192.168.123.200,fc00::5c77:e4fe:8950:c79c"
Jan 31 15:56:57 openshift-master-0 hyperkube[1838]: E0131 15:56:57.845567    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:56:58 openshift-master-0 hyperkube[1838]: E0131 15:56:58.054892    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:56:58 openshift-master-0 hyperkube[1838]: E0131 15:56:58.463134    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:56:59 openshift-master-0 hyperkube[1838]: E0131 15:56:59.272138    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:57:00 openshift-master-0 hyperkube[1838]: E0131 15:57:00.880505    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:57:04 openshift-master-0 hyperkube[1838]: E0131 15:57:04.091759    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:57:07 openshift-master-0 hyperkube[1838]: E0131 15:57:07.849532    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:57:09 openshift-master-0 hyperkube[1838]: E0131 15:57:09.174765    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:57:09 openshift-master-0 hyperkube[1838]: E0131 15:57:09.182435    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:57:10 openshift-master-0 hyperkube[1838]: E0131 15:57:10.502112    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:57:10 openshift-master-0 hyperkube[1838]: E0131 15:57:10.516377    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:57:20 openshift-master-0 hyperkube[1838]: E0131 15:57:20.538667    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:57:30 openshift-master-0 hyperkube[1838]: E0131 15:57:30.554856    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:57:40 openshift-master-0 hyperkube[1838]: E0131 15:57:40.572778    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:57:50 openshift-master-0 hyperkube[1838]: E0131 15:57:50.583840    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:58:00 openshift-master-0 hyperkube[1838]: E0131 15:58:00.594109    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:58:10 openshift-master-0 hyperkube[1838]: E0131 15:58:10.604623    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:58:20 openshift-master-0 hyperkube[1838]: E0131 15:58:20.614737    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:58:30 openshift-master-0 hyperkube[1838]: E0131 15:58:30.625496    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:58:40 openshift-master-0 hyperkube[1838]: E0131 15:58:40.637582    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:58:50 openshift-master-0 hyperkube[1838]: E0131 15:58:50.646834    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:59:00 openshift-master-0 hyperkube[1838]: E0131 15:59:00.658235    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:59:10 openshift-master-0 hyperkube[1838]: E0131 15:59:10.669242    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:59:20 openshift-master-0 hyperkube[1838]: E0131 15:59:20.679367    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:59:30 openshift-master-0 hyperkube[1838]: E0131 15:59:30.689784    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:59:40 openshift-master-0 hyperkube[1838]: E0131 15:59:40.700473    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 15:59:50 openshift-master-0 hyperkube[1838]: E0131 15:59:50.710888    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 16:00:00 openshift-master-0 hyperkube[1838]: E0131 16:00:00.722242    1838 kubelet_node_status.go:586] Failed to set some node status fields: failed to validate secondaryNodeIP: node IP: "fc00::5c77:e4fe:8950:c79c" not found in the host's network interfaces
Jan 31 16:00:10 openshift-master-0 hyperku










gg
