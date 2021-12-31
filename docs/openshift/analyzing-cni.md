# Analyzing CNI calls

Let's analyze how CNI actually works. First, you should read through the actual specification: [https://github.com/containernetworking/cni/blob/master/SPEC.md](https://github.com/containernetworking/cni/blob/master/SPEC.md)

Once you have done that, the next best thing to writing your own plugin is surely to write a wrapper script around existing plugins and to observe what is being passed to/from the plugins.

### Setup

All examples are run on a signle-node Kubernetes cluster with CRI-O. I did not install a CNI plugin, instead I am relying on CRI-O's `bridge` plugin. The `bridge` plugin uses the `host-local` plugin for IPAM. The configuration file can be found under `/etc/cni/net.d/100-crio-bridge.conf` and its contents look as follows:
~~~
[root@node1 ~]# cat /etc/cni/net.d/100-crio-bridge.conf 
{
    "cniVersion": "0.3.1",
    "name": "crio",
    "type": "bridge",
    "bridge": "cni0",
    "isGateway": true,
    "ipMasq": true,
    "hairpinMode": true,
    "ipam": {
        "type": "host-local",
        "routes": [
            { "dst": "0.0.0.0/0" },
            { "dst": "1100:200::1/24" }
        ],
        "ranges": [
            [{ "subnet": "10.85.0.0/16" }],
            [{ "subnet": "1100:200::/24" }]
        ]
    }
}
~~~

The actual plugin binaries are `/opt/bin/cni/bridge` and `/opt/bin/cni/host-local`.

### Creating a wrapper script

Create the wrapper script in a temporary location:
~~~
f=$(mktemp)
cat <<'EOF' > $f
#!/bin/bash

SCRIPT=$(readlink -f $0)
LOG_DIR=/tmp/log
if ! [ -d $LOG_DIR ];then
  mkdir -p $LOG_DIR
fi

LOG=${LOG_DIR}/$(basename $0)

date >> $LOG
echo "=================================================================" >> $LOG
echo "CNI params:" >> $LOG
env | grep CNI >> $LOG
echo "env:" >> $LOG
env >> $LOG
if [ ! -t 0 ]; then
  {
  echo "INPUT:" >> $LOG
  output=$(tee -a $LOG | ${SCRIPT}-original "$@" | tee /dev/fd/3)
  echo "" >> $LOG
  echo "OUTPUT:" >> $LOG
  echo "$output" >> $LOG
  } 3>&1
else
  echo "OUTPUT:" >> $LOG
  ${SCRIPT}-original "$@" | tee -a $LOG
fi

echo "Returned at: $(date)" >> $LOG
echo "=================================================================" >> $LOG
echo "" >> $LOG
echo "" >> $LOG
EOF
chmod +x $f
~~~

Then, rename the original plugins to `<name>-original` and copy the wrapper script into the former location of the binary files:
~~~
for plugin in bridge host-local; do
  \cp /opt/cni/bin/${plugin} /opt/cni/bin/${plugin}-original
  \cp $f /opt/cni/bin/${plugin}
done
~~~

### Rollback

To revert this back to defaults, after testing, run:
~~~
for plugin in bridge host-local; do
  \mv /opt/cni/bin/${plugin}-original /opt/cni/bin/${plugin}
done
~~~

### Triggering CNI calls

Now, create a test invocation. For example, use [https://github.com/containernetworking/cni/tree/master/cnitool](https://github.com/containernetworking/cni/tree/master/cnitool)
~~~
git clone  https://github.com/containernetworking/cni
pushd cni/cnitool
go build
mv cnitool /usr/local/bin
popd
ip netns add testing
CNI_PATH=/opt/cni/bin/ cnitool add crio /var/run/netns/testing
CNI_PATH=/opt/cni/bin/ cnitool del crio /var/run/netns/testing
~~~

Alternatively, simply delete a running pod and have it recreated by kubernetes:
~~~
kubectl delete pod -n <...> <...>
~~~

### Examining CNI calls

In file `/tmp/log/bridge` you can see that the `bridge` plugin uses data from the `host-local` to create the final output:
~~~
Fri Aug 27 07:55:52 EDT 2021
=================================================================
CNI params:
CNI_PATH=/opt/cni/bin:/usr/libexec/cni
CNI_ARGS=IgnoreUnknown=1;K8S_POD_NAMESPACE=kube-system;K8S_POD_NAME=coredns-78fcd69978-n5khq;K8S_POD_INFRA_CONTAINER_ID=9b6e84650c3ed8407953c396efa391d2c2173388d8a8b3615c937a5779d526e8;K8S_POD_UID=94f2364f-4fd6-4b74-9f5a-c68e2c6f97dd
CNI_CONTAINERID=9b6e84650c3ed8407953c396efa391d2c2173388d8a8b3615c937a5779d526e8
CNI_IFNAME=eth0
CNI_COMMAND=ADD
CNI_NETNS=/var/run/netns/3526ea0f-6dd9-4985-8f98-0db6d817abd4
env:
CNI_PATH=/opt/cni/bin:/usr/libexec/cni
LANG=en_US.UTF-8
INVOCATION_ID=77a9194a9cd04940a5fe1bdbca0e218b
CNI_ARGS=IgnoreUnknown=1;K8S_POD_NAMESPACE=kube-system;K8S_POD_NAME=coredns-78fcd69978-n5khq;K8S_POD_INFRA_CONTAINER_ID=9b6e84650c3ed8407953c396efa391d2c2173388d8a8b3615c937a5779d526e8;K8S_POD_UID=94f2364f-4fd6-4b74-9f5a-c68e2c6f97dd
CNI_CONTAINERID=9b6e84650c3ed8407953c396efa391d2c2173388d8a8b3615c937a5779d526e8
GOTRACEBACK=crash
PWD=/
JOURNAL_STREAM=9:163692
CNI_IFNAME=eth0
CNI_COMMAND=ADD
CNI_NETNS=/var/run/netns/3526ea0f-6dd9-4985-8f98-0db6d817abd4
SHLVL=1
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin
_=/usr/bin/env
INPUT:
{"bridge":"cni0","cniVersion":"0.3.1","hairpinMode":true,"ipMasq":true,"ipam":{"ranges":[[{"subnet":"10.85.0.0/16"}],[{"subnet":"1100:200::/24"}]],"routes":[{"dst":"0.0.0.0/0"},{"dst":"1100:200::1/24"}],"type":"host-local"},"isGateway":true,"name":"crio","type":"bridge"}
OUTPUT:
{
    "cniVersion": "0.3.1",
    "interfaces": [
        {
            "name": "cni0",
            "mac": "8e:14:0e:0e:f1:b0"
        },
        {
            "name": "vethc4b25c20",
            "mac": "7e:a5:58:f4:d9:93"
        },
        {
            "name": "eth0",
            "mac": "4a:a6:1c:d8:66:2d",
            "sandbox": "/var/run/netns/3526ea0f-6dd9-4985-8f98-0db6d817abd4"
        }
    ],
    "ips": [
        {
            "version": "4",
            "interface": 2,
            "address": "10.85.0.17/16",
            "gateway": "10.85.0.1"
        },
        {
            "version": "6",
            "interface": 2,
            "address": "1100:200::f/24",
            "gateway": "1100:200::1"
        }
    ],
    "routes": [
        {
            "dst": "0.0.0.0/0"
        },
        {
            "dst": "1100:200::1/24"
        }
    ],
    "dns": {}
}
~~~

When we look at [https://github.com/containernetworking/cni/blob/master/SPEC.md#section-4-plugin-delegation](https://github.com/containernetworking/cni/blob/master/SPEC.md#section-4-plugin-delegation), we can see the following:
~~~
It is however the responsibility of the CNI plugin, rather than the runtime, to invoke the IPAM plugin at the proper moment in its execution. The IPAM plugin must determine the interface IP/subnet, Gateway and Routes and return this information to the "main" plugin to apply. The IPAM plugin may obtain the information via a protocol (e.g. dhcp), data stored on a local filesystem, the "ipam" section of the Network Configuration file, etc.
~~~

The `bridge` plugin delegates IP address allocation to an `ipam` plugin here:
[https://github.com/containernetworking/plugins/blob/8632ace977f4126c8ded601975f564571c66b922/plugins/main/bridge/bridge.go#L465](https://github.com/containernetworking/plugins/blob/8632ace977f4126c8ded601975f564571c66b922/plugins/main/bridge/bridge.go#L465)

In file `/tmp/log/host-local`, we can see the following during an `ADD` command. This is the result of the delegation:
~~~
Fri Aug 27 07:55:52 EDT 2021
=================================================================
CNI params:
CNI_PATH=/opt/cni/bin:/usr/libexec/cni
CNI_ARGS=IgnoreUnknown=1;K8S_POD_NAMESPACE=kube-system;K8S_POD_NAME=coredns-78fcd69978-n5khq;K8S_POD_INFRA_CONTAINER_ID=9b6e84650c3ed8407953c396efa391d2c2173388d8a8b3615c937a5779d526e8;K8S_POD_UID=94f2364f-4fd6-4b74-9f5a-c68e2c6f97dd
CNI_CONTAINERID=9b6e84650c3ed8407953c396efa391d2c2173388d8a8b3615c937a5779d526e8
CNI_IFNAME=eth0
CNI_COMMAND=ADD
CNI_NETNS=/var/run/netns/3526ea0f-6dd9-4985-8f98-0db6d817abd4
env:
CNI_PATH=/opt/cni/bin:/usr/libexec/cni
LANG=en_US.UTF-8
INVOCATION_ID=77a9194a9cd04940a5fe1bdbca0e218b
CNI_ARGS=IgnoreUnknown=1;K8S_POD_NAMESPACE=kube-system;K8S_POD_NAME=coredns-78fcd69978-n5khq;K8S_POD_INFRA_CONTAINER_ID=9b6e84650c3ed8407953c396efa391d2c2173388d8a8b3615c937a5779d526e8;K8S_POD_UID=94f2364f-4fd6-4b74-9f5a-c68e2c6f97dd
CNI_CONTAINERID=9b6e84650c3ed8407953c396efa391d2c2173388d8a8b3615c937a5779d526e8
GOTRACEBACK=crash
PWD=/
JOURNAL_STREAM=9:163692
CNI_IFNAME=eth0
CNI_COMMAND=ADD
CNI_NETNS=/var/run/netns/3526ea0f-6dd9-4985-8f98-0db6d817abd4
SHLVL=2
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin
_=/usr/bin/env
INPUT:
{"bridge":"cni0","cniVersion":"0.3.1","hairpinMode":true,"ipMasq":true,"ipam":{"ranges":[[{"subnet":"10.85.0.0/16"}],[{"subnet":"1100:200::/24"}]],"routes":[{"dst":"0.0.0.0/0"},{"dst":"1100:200::1/24"}],"type":"host-local"},"isGateway":true,"name":"crio","type":"bridge"}
OUTPUT:
{
    "cniVersion": "0.3.1",
    "ips": [
        {
            "version": "4",
            "address": "10.85.0.17/16",
            "gateway": "10.85.0.1"
        },
        {
            "version": "6",
            "address": "1100:200::f/24",
            "gateway": "1100:200::1"
        }
    ],
    "routes": [
        {
            "dst": "0.0.0.0/0"
        },
        {
            "dst": "1100:200::1/24"
        }
    ],
    "dns": {}
}
~~~

