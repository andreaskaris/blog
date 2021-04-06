##############################################
## Creating a valid udev rule manually
##############################################

Let's assume I want to change all X710 cards and name them x710-<UNIQUE IDENTIFIER> where the <UNIQUE IDENTIFIER> is the MAC address in HEX only, without the separating colons. That will guarantee that each interface is unique and that it's name persists across reboots and that it does not change (at least for as long as the NIC has the same MAC).

We'd create the rules manually as follows:
~~~
[root@openshift-worker-0 ~]# udevadm info /sys/devices/pci0000:3a/0000:3a:00.0/0000:3b:00.0/net/ens1f0
P: /devices/pci0000:3a/0000:3a:00.0/0000:3b:00.0/net/ens1f0
E: DEVPATH=/devices/pci0000:3a/0000:3a:00.0/0000:3b:00.0/net/ens1f0
E: ID_BUS=pci
E: ID_MODEL_FROM_DATABASE=Ethernet Controller X710 for 10GbE SFP+ (Ethernet 10G 2P X710 Adapter)
E: ID_MODEL_ID=0x1572
E: ID_NET_DRIVER=i40e
E: ID_NET_LINK_FILE=/usr/lib/systemd/network/99-default.link
E: ID_NET_NAME=ens1f0
E: ID_NET_NAME_MAC=enxf8f21e84c2f0
E: ID_NET_NAME_PATH=enp59s0f0
E: ID_NET_NAME_SLOT=ens1f0
E: ID_OUI_FROM_DATABASE=Intel Corporate
E: ID_PATH=pci-0000:3b:00.0
E: ID_PATH_TAG=pci-0000_3b_00_0
E: ID_PCI_CLASS_FROM_DATABASE=Network controller
E: ID_PCI_SUBCLASS_FROM_DATABASE=Ethernet controller
E: ID_VENDOR_FROM_DATABASE=Intel Corporation
E: ID_VENDOR_ID=0x8086
E: IFINDEX=10
E: INTERFACE=ens1f0
E: SUBSYSTEM=net
E: SYSTEMD_ALIAS=/sys/subsystem/net/devices/ens1f0
E: TAGS=:systemd:
E: USEC_INITIALIZED=18659852
~~~

We'd walk the attributes to get the correct device and vendor values:
~~~
[root@openshift-worker-0 ~]# udevadm info /sys/class/net/ens6f0 --attribute-walk | egrep 'device|vendor'
Udevadm info starts with the device specified by the devpath and then
walks up the chain of parent devices. It prints for every device
A rule to match, can be composed by the attributes of the device
and the attributes from one single parent device.
  looking at device '/devices/pci0000:17/0000:17:00.0/0000:18:00.0/net/ens6f0':
  looking at parent device '/devices/pci0000:17/0000:17:00.0/0000:18:00.0':
    ATTRS{device}=="0x1572"
    ATTRS{sriov_vf_device}=="154c"
    ATTRS{subsystem_device}=="0x0006"
    ATTRS{subsystem_vendor}=="0x8086"
    ATTRS{vendor}=="0x8086"
  looking at parent device '/devices/pci0000:17/0000:17:00.0':
    ATTRS{device}=="0x2030"
    ATTRS{subsystem_device}=="0x0000"
    ATTRS{subsystem_vendor}=="0x8086"
    ATTRS{vendor}=="0x8086"
  looking at parent device '/devices/pci0000:17':
~~~

> Note that I'm not sure how "safe" the following, in my lab this works (after a lot of trying due to the script part). But the assumption is that you have your own custom udev configuration of which you know that it already works and this udev rule would correctly set unique interface names. 

So our udev rule could just call a script for every interface and the script would set a custom name if certain criteria was matched. 
~~~
cat <<'EOF' > /etc/custom-nic-names.sh
#!/bin/bash
IDENTIFIER=$(cat /sys/${DEVPATH}/address | sed 's/://g')
VENDOR=$(cat /sys/${DEVPATH}/device/vendor)
DEVICE=$(cat /sys/${DEVPATH}/device/device)

if [ "$VENDOR" != "0x8086" ] || [ "$DEVICE" != "0x1572" ]; then
  echo "CUSTOM_NET_NAME=\"\"" 
  exit
fi

echo "CUSTOM_NET_NAME=\"x710-$IDENTIFIER\""
EOF
chmod +x /etc/custom-nic-names.sh

cat <<'EOF' > /etc/udev/rules.d/79-custom-net.rules
# ignore this rule if this is not net, is not action add
SUBSYSTEM!="net", GOTO="net_setup_link_end"
ACTION!="add", GOTO="net_setup_link_end"

# generate a CUSTOM_NET_NAME via script and set the name
IMPORT{program}="/etc/custom-nic-names.sh"
ENV{CUSTOM_NET_NAME}!="", NAME="$env{CUSTOM_NET_NAME}"

LABEL="net_setup_link_end"
EOF
~~~

And we can then test this with:
~~~
[root@openshift-worker-1 ~]#  NIC=/sys/class/net/ens6f0 ; udevadm --debug test --action=add $NIC 2>&1 | grep NAME | grep NAME
'/usr/local/bin/custom-nic-names.sh'(out) 'CUSTOM_NET_NAME="x710-f8f21e8316c0"'
NAME 'x710-f8f21e8316c0' /etc/udev/rules.d/70-custom-net.rules:8
CUSTOM_NET_NAME=x710-f8f21e8316c0
ID_NET_NAME=ens6f0
ID_NET_NAME_MAC=enxf8f21e8316c0
ID_NET_NAME_PATH=enp24s0f0
ID_NET_NAME_SLOT=ens6f0
~~~

##############################################
## Applying the above with NetworkManager
##############################################

We will want to apply this rule via the MachineConfigOperator - for example, in order to push this to all workers:
~~~
cat <<'EOF' > custom-nic-names.sh
#!/bin/bash
IDENTIFIER=$(cat /sys/${DEVPATH}/address | sed 's/://g')
echo "CUSTOM_NET_NAME=\"x710-$IDENTIFIER\""
EOF

cat <<'EOF' > 70-custom-net.rules

# ignore this rule if this is not net, is not action add
SUBSYSTEM!="net", GOTO="net_setup_link_end"
ACTION!="add", GOTO="net_setup_link_end"

# generate a CUSTOM_NET_NAME via script and set the name
ATTRS{vendor}=="0x8086", ATTRS{device}=="0x1572" IMPORT{program}="/usr/local/bin/custom-nic-names.sh"
ATTRS{vendor}=="0x8086", ATTRS{device}=="0x1572" NAME=="", ENV{CUSTOM_NET_NAME}!="", NAME="$env{CUSTOM_NET_NAME}"

LABEL="net_setup_link_end"
EOF

cat <<EOF | oc apply -f -
apiVersion: machineconfiguration.openshift.io/v1
kind: MachineConfig
metadata:
  labels:
    machineconfiguration.openshift.io/role: worker
  name: 99-worker-custom-nic-names
spec:
  config:
    ignition:
      version: 3.2.0
    storage:
      files:
      - contents:
          source: data:text/plain;charset=utf-8;base64,$(cat custom-nic-names.sh | base64 -w0)
        filesystem: root
        mode: 0755
        path: /usr/local/bin/custom-nic-names.sh
      - contents:
          source: data:text/plain;charset=utf-8;base64,$(cat 70-custom-net.rules | base64 -w0)
        filesystem: root
        mode: 0644
        path: /etc/udev/rules.d/70-custom-net.rules
EOF
~~~
> Note:  `version: 3.2.0`  changes depending on the OpenShift version. Run `oc get machineconfig` and pick the higher valid IGNITIONVERSION (IIRC, in OCP 4.5 it's 2.2.0 and in OCP 4.6 it's  3.1.0)






