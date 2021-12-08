## Useful OCP SDN commands

### Increasing log level for specific nodes

See: [https://github.com/openshift/cluster-network-operator/wiki/How-to-increase-the-log-level-on-a-single-SDN-or-OVN-node](https://github.com/openshift/cluster-network-operator/wiki/How-to-increase-the-log-level-on-a-single-SDN-or-OVN-node)

The following is an example:
~~~
cat << 'EOF' > overrides.yaml 
kind: ConfigMap
apiVersion: v1
metadata:
  name: env-overrides
  namespace: openshift-ovn-kubernetes
data:
  openshift-worker-1: |
    # This sets the log level for the ovn-kubernetes node process:
    OVN_KUBE_LOG_LEVEL=5
EOF
oc apply -f overrides.yaml 
oc delete pod -n openshift-ovn-kubernetes --field-selector spec.nodeName=openshift-worker-1 -l app=ovnkube-node
~~~

That will increase the log level to level 5 which allows to get more verbose output:
~~~
$ oc logs -f -n openshift-ovn-kubernetes ovnkube-node-vg55p --tail=0 -c ovnkube-node
(...)
I1208 15:37:38.996350 3343454 healthcheck.go:253] Gateway OpenFlow sync requested
I1208 15:37:38.996370 3343454 gateway_iptables.go:45] Adding rule in table: nat, chain: OVN-KUBE-NODEPORT with args: "-p TCP -m addrtype --dst-type LOCAL --dport 30000 -j DNAT --to-destination 172.30.239.167:27017" for protocol: 0 
I1208 15:37:38.996476 3343454 ovs.go:209] exec(164): /usr/bin/ovs-ofctl -O OpenFlow13 --bundle replace-flows br-ex -
I1208 15:37:39.001098 3343454 gateway_iptables.go:48] Chain: "nat" in table: "OVN-KUBE-NODEPORT" already exists, skipping creation
I1208 15:37:39.010805 3343454 gateway_iptables.go:45] Adding rule in table: nat, chain: OVN-KUBE-NODEPORT with args: "-p TCP -m addrtype --dst-type LOCAL --dport 30000 -j DNAT --to-destination [fd02::82b5]:27017" for protocol: 1 
I1208 15:37:39.015130 3343454 gateway_iptables.go:48] Chain: "nat" in table: "OVN-KUBE-NODEPORT" already exists, skipping creation
(...)
~~~
