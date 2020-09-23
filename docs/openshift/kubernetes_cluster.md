# Hints for installing a kubernetes cluster on Fedora #

Hints for installing a kubernetes cluster on Fedora in a virtual environment for lab purposes

## DNS | /etc/hosts for all nodes ##

Make sure to set up `/etc/hosts` on all nodes:
~~~
cat<<EOF>/etc/hosts
127.0.0.1   localhost localhost.localdomain localhost4 localhost4.localdomain4
::1         localhost localhost.localdomain localhost6 localhost6.localdomain6
192.168.122.250 api.example.com
192.168.122.140 vm2
192.168.122.77 vm4
192.168.122.248 vm1
192.168.122.70 vm3
192.168.122.117 vm5
EOF
~~~

## Keepalived configuration for master nodes ##

Master nodes can be set up with the following keepalived configuration for HA:
~~~
cat<<'EOF'>/etc/keepalived/keepalived.conf 
vrrp_instance vip {
 state MASTER
 notify /usr/local/sbin/notify-keepalived.sh
 interface eth0
 virtual_router_id 12
 priority 150
 advert_int 1
 virtual_ipaddress {
   192.168.122.250/32
 }
}
EOF
~~~

~~~
cat<<'EOF'>/usr/local/sbin/notify-keepalived.sh
#!/bin/bash
TYPE=$1
NAME=$2
STATE=$3

sleep 2

case $STATE in
        "MASTER") /usr/bin/systemctl start haproxy
                  ;;
        "BACKUP") /usr/bin/systemctl stop haproxy
                  ;;
        "FAULT")  /usr/bin/systemctl stop haproxy
                  exit 0
                  ;;
        *)        /sbin/logger "haproxy unknown state"
                  exit 1
                  ;;
esac
EOF
~~~

~~~
chmod +x /usr/local/sbin/notify-keepalived.sh
~~~

## haproxy setup on master nodes ##

This goes hand in hand with the keepalived coniguration mentioned earlier - run this on all of the masters:
~~~
cat<<EOF>/etc/haproxy/haproxy.cfg
global
    log         127.0.0.1 local2
    chroot      /var/lib/haproxy
    pidfile     /var/run/haproxy.pid
    maxconn     4000
    user        haproxy
    group       haproxy
    daemon
    stats socket /var/lib/haproxy/stats
    ssl-default-bind-ciphers PROFILE=SYSTEM
    ssl-default-server-ciphers PROFILE=SYSTEM

defaults
    mode                    tcp
    log                     global
    option                  httplog
    option                  dontlognull
    option http-server-close
    option forwardfor       except 127.0.0.0/8
    option                  redispatch
    retries                 3
    timeout http-request    10s
    timeout queue           1m
    timeout connect         10s
    timeout client          1m
    timeout server          1m
    timeout http-keep-alive 10s
    timeout check           10s
    maxconn                 3000

frontend main
    bind api.example.com:6443
    mode tcp
    default_backend             app

backend app
    balance     roundrobin
    server  app1 vm1:6443 check
    server  app2 vm2:6443 check
    server  app3 vm3:6443 check
EOF
~~~

### Install more recent version of containerd ###

In Fedora 31, it's required to install a more recent version of containerd than what is provided by the package manager:
~~~
yum install containerd -y
tar -xf containerd-1.3.0.linux-amd64.tar.gz
mv bin/* /usr/local/sbin/.
cat<<'EOF'>/etc/systemd/system/containerd.service 
[Unit]
Description=containerd container runtime
Documentation=https://containerd.io
After=network.target

[Service]
ExecStartPre=/sbin/modprobe overlay
ExecStart=/usr/local/sbin/containerd
Delegate=yes
KillMode=process

[Install]
WantedBy=multi-user.target
EOF
systemctl daemon-reload
systemctl restart containerd
systemctl enable containerd
crictl --runtime-endpoint=/var/run/containerd/containerd.sock info
~~~

## Revert Fedora to use cgroupv1 ##

Revert Fedora 31 to cgroupv1 as this will not work with cgroupv2
[https://medium.com/nttlabs/cgroup-v2-596d035be4d7](https://medium.com/nttlabs/cgroup-v2-596d035be4d7)

~~~
sudo dnf install -y grubby
sudo grubby \
  --update-kernel=ALL \
  --args="systemd.unified_cgroup_hierarchy=0"
~~~

## Work around issues with Flannel in k8s 1.16 ##

Problems with flannel in k8s 1.16:
[https://stackoverflow.com/questions/58024643/kubernetes-master-node-not-ready-state](https://stackoverflow.com/questions/58024643/kubernetes-master-node-not-ready-state)
[https://github.com/coreos/flannel/issues/1185](https://github.com/coreos/flannel/issues/1185)

~~~
[root@vm1 ~]# ADVERTISE_URL="https://127.0.0.1:2379"
[root@vm1 ~]# ETCDCTL_API=3 etcdctl --endpoints $ADVERTISE_URL --cacert /etc/kubernetes/pki/etcd/ca.crt --key /etc/kubernetes/pki/etcd/server.key --cert /etc/kubernetes/pki/etcd/server.crt --insecure-skip-tls-verify member list
7f6515a7c372e765, started, vm3, https://192.168.122.70:2380, https://192.168.122.70:2379
cf8371b51a17571b, started, vm2, https://192.168.122.140:2380, https://192.168.122.140:2379
ec403cb4ca539d7b, started, vm1, https://192.168.122.248:2380, https://192.168.122.248:2379
~~~

~~~
ETCDCTL_API=3 etcdctl --endpoints $ADVERTISE_URL --cacert /etc/kubernetes/pki/etcd/ca.crt --key /etc/kubernetes/pki/etcd/server.key --cert /etc/kubernetes/pki/etcd/server.crt get --prefix /registry -w json | python -m json.tool
~~~

~~~
[root@vm1 ~]# ETCDCTL_API=3 etcdctl --endpoints $ADVERTISE_URL --cacert /etc/kubernetes/pki/etcd/ca.crt --key /etc/kubernetes/pki/etcd/server.key --cert /etc/kubernetes/pki/etcd/server.crt get --prefix /registry --keys-only | grep flann
/registry/clusterrolebindings/flannel
/registry/clusterroles/flannel
/registry/configmaps/kube-system/kube-flannel-cfg
/registry/controllerrevisions/kube-system/kube-flannel-ds-amd64-67f65bfbc7
/registry/controllerrevisions/kube-system/kube-flannel-ds-arm-74f7486b59
/registry/controllerrevisions/kube-system/kube-flannel-ds-arm64-575fdc5885
/registry/controllerrevisions/kube-system/kube-flannel-ds-ppc64le-84596b9cb9
/registry/controllerrevisions/kube-system/kube-flannel-ds-s390x-7f96755bd4
/registry/daemonsets/kube-system/kube-flannel-ds-amd64
/registry/daemonsets/kube-system/kube-flannel-ds-arm
/registry/daemonsets/kube-system/kube-flannel-ds-arm64
/registry/daemonsets/kube-system/kube-flannel-ds-ppc64le
/registry/daemonsets/kube-system/kube-flannel-ds-s390x
/registry/pods/kube-system/kube-flannel-ds-amd64-7gd69
/registry/pods/kube-system/kube-flannel-ds-amd64-9jkzr
/registry/pods/kube-system/kube-flannel-ds-amd64-krgj4
/registry/pods/kube-system/kube-flannel-ds-amd64-v6wqc
/registry/pods/kube-system/kube-flannel-ds-amd64-vfnhx
/registry/podsecuritypolicy/psp.flannel.unprivileged
/registry/secrets/kube-system/flannel-token-ff78q
/registry/serviceaccounts/kube-system/flannel
~~~

~~~
[root@vm1 ~]# kubectl exec -it kube-flannel-ds-amd64-7gd69 -n kube-system /bin/bash
bash-4.4# ps
PID   USER     TIME  COMMAND
    1 root      0:08 /opt/bin/flanneld --ip-masq --kube-subnet-mgr
 8183 root      0:00 /bin/bash
 8190 root      0:00 ps
bash-4.4# cat /etc/kube-flannel/net-conf.json
{
  "Network": "10.244.0.0/16",
  "Backend": {
    "Type": "vxlan"
  }
}
bash-4.4# 
~~~

[https://github.com/coreos/flannel/blob/master/Documentation/configuration.md](https://github.com/coreos/flannel/blob/master/Documentation/configuration.md)

Issue with systemd and flannel:
[https://github.com/coreos/flannel/issues/1155](https://github.com/coreos/flannel/issues/1155)
~~~
cat<<'EOF'>/etc/systemd/network/10-flannel.link
[Match]
OriginalName=flannel*

[Link]
MACAddressPolicy=none
EOF
~~~

~~~
[root@vm4 ~]# cat /etc/default/grub  | grep CMDLINE
GRUB_CMDLINE_LINUX="no_timer_check net.ifnames=0 console=tty1 console=ttyS0,115200n8 systemd.unified_cgroup_hierarchy=0"
~~~

~~~
[root@vm4 ~]# cat /etc/yum.repos.d/kubernetes.repo 
[kubernetes]
name=Kubernetes
baseurl=https://packages.cloud.google.com/yum/repos/kubernetes-el7-x86_64
enabled=1
gpgcheck=1
repo_gpgcheck=1
gpgkey=https://packages.cloud.google.com/yum/doc/yum-key.gpg https://packages.cloud.google.com/yum/doc/rpm-package-key.gpg
~~~

~~~
[root@vm4 ~]# rpm -qa | egrep 'kube|docker'
docker-ce-19.03.4-3.fc31.x86_64
kubectl-1.16.2-0.x86_64
kubelet-1.16.2-0.x86_64
kubeadm-1.16.2-0.x86_64
kubernetes-cni-0.7.5-0.x86_64
docker-ce-cli-19.03.4-3.fc31.x86_64
~~~

~~~
root@vm4 ~]# cat /etc/yum.repos.d/docker-ce.repo
[docker-ce-stable]
name=Docker CE Stable - $basearch
baseurl=https://download.docker.com/linux/fedora/$releasever/$basearch/stable
enabled=1
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg

[docker-ce-stable-debuginfo]
name=Docker CE Stable - Debuginfo $basearch
baseurl=https://download.docker.com/linux/fedora/$releasever/debug-$basearch/stable
enabled=0
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg

[docker-ce-stable-source]
name=Docker CE Stable - Sources
baseurl=https://download.docker.com/linux/fedora/$releasever/source/stable
enabled=0
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg

[docker-ce-edge]
name=Docker CE Edge - $basearch
baseurl=https://download.docker.com/linux/fedora/$releasever/$basearch/edge
enabled=0
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg

[docker-ce-edge-debuginfo]
name=Docker CE Edge - Debuginfo $basearch
baseurl=https://download.docker.com/linux/fedora/$releasever/debug-$basearch/edge
enabled=0
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg

[docker-ce-edge-source]
name=Docker CE Edge - Sources
baseurl=https://download.docker.com/linux/fedora/$releasever/source/edge
enabled=0
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg

[docker-ce-test]
name=Docker CE Test - $basearch
baseurl=https://download.docker.com/linux/fedora/$releasever/$basearch/test
enabled=0
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg

[docker-ce-test-debuginfo]
name=Docker CE Test - Debuginfo $basearch
baseurl=https://download.docker.com/linux/fedora/$releasever/debug-$basearch/test
enabled=0
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg

[docker-ce-test-source]
name=Docker CE Test - Sources
baseurl=https://download.docker.com/linux/fedora/$releasever/source/test
enabled=0
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg

[docker-ce-nightly]
name=Docker CE Nightly - $basearch
baseurl=https://download.docker.com/linux/fedora/$releasever/$basearch/nightly
enabled=0
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg

[docker-ce-nightly-debuginfo]
name=Docker CE Nightly - Debuginfo $basearch
baseurl=https://download.docker.com/linux/fedora/$releasever/debug-$basearch/nightly
enabled=0
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg

[docker-ce-nightly-source]
name=Docker CE Nightly - Sources
baseurl=https://download.docker.com/linux/fedora/$releasever/source/nightly
enabled=0
gpgcheck=1
gpgkey=https://download.docker.com/linux/fedora/gpg
~~~

~~~
yum install kubeadm kubectl kubelet kubernetes-cni docker-ce -y
~~~
