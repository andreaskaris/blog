## Kubernets iSCSI volume driver

The iSCSI volume driver can work as both a single path iSCSI initiator or with multipath.
For some information about the driver, see [https://github.com/kubernetes/examples/tree/master/volumes/iscsi](https://github.com/kubernetes/examples/tree/master/volumes/iscsi).
The driver detects if the system is configured for multipath and will use it when available.
See [https://github.com/kubernetes/examples/blob/master/volumes/iscsi/iscsi.yaml](https://github.com/kubernetes/examples/blob/master/volumes/iscsi/iscsi.yaml)
for an example pod resource.

## Configuring an iSCSI target for testing

If you do not have an iSCSI target, you can create one for testing with RHEL. See
[the documentation for creating an iSCSI target](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/managing_storage_devices/configuring-an-iscsi-target_managing-storage-devices).

If you want to test that iSCSI is working without OpenShift, you can initiate an iSCSI connection from a test RHEL node.
Have a look at
[the documentation for creating an iSCSI initiator](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/managing_storage_devices/configuring-an-iscsi-initiator_managing-storage-devices).

For multipath tests, simply add more secondary IP addresses (x.x.x.x/32) to your host. If you want to simulate a path
failure, you can simply create an iptables rule that drops traffic to the VIPs:
```
iptables -I INPUT --dst <x.x.x.x> --j REJECT
```

## How to use iSCSI with OpenShift

In Red Hat OpenShift Container Platform 4.14, the `iscsid` service is already enabled by default, and the iSCSI
initiator name is already configured for each node in `/etc/iscsi/initiatorname.iscsi`. The initiator name can be
changed with a MachineConfig if desired.

The OpenShift documentation about
[Persistent Storage using iSCSI](https://docs.openshift.com/container-platform/4.14/storage/persistent_storage/persistent-storage-iscsi.html#persistent-storage-iscsi-provisioning_persistent-storage-iscsi)
holds an example for how to provision a PV with a single iSCSI path.

A full example to create a PV, PVC and a pod using them would be:

```
apiVersion: v1
kind: PersistentVolume
metadata:
  name: iscsi-pv
spec:
  storageClassName: manual
  capacity:
    storage: 20Gi
  accessModes:
    - ReadWriteOnce
  iscsi:
     targetPortal: 192.168.100.10:3260
     iqn: iqn.2003-01.org.linux-iscsi.iscsi-target-host.x8664:sn.aabbccddeeff
     lun: 0
     fsType: 'xfs'
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: iscsi-pv-claim
spec:
  storageClassName: manual
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 20Gi
---
apiVersion: v1
kind: Pod
metadata:
  name: iscsi-pv-pod
spec:
  containers:
    - name: iscsi-pv-container
      image: fedora
      command:
        - sleep
        - infinity
      volumeMounts:
        - mountPath: "/iscsi"
          name: iscsi-pv-storage
  volumes:
    - name: iscsi-pv-storage
      persistentVolumeClaim:
        claimName: iscsi-pv-claim
```

Apply the above, and verify that everything is up and running as expected:

```
[user@host ~]$ oc get pv
oNAME       CAPACITY   ACCESS MODES   RECLAIM POLICY   STATUS   CLAIM                 STORAGECLASS   REASON   AGE
iscsi-pv   20Gi       RWO            Retain           Bound    test/iscsi-pv-claim   manual                  23s
c[user@host ~]$ oc get pvc
oNAME             STATUS   VOLUME     CAPACITY   ACCESS MODES   STORAGECLASS   AGE
iscsi-pv-claim   Bound    iscsi-pv   20Gi       RWO            manual         24s
[user@host ~]$ oc get pods
NAME           READY   STATUS    RESTARTS   AGE
iscsi-pv-pod   1/1     Running   0          25s
```

You can also see that the host mounted the new iSCSI target to the pod:

```
[root@openshift-sno ~]# iscsiadm -m session -P3 | grep Lun -A1
		scsi11 Channel 00 Id 0 Lun: 0
			Attached scsi disk sdb		State: running
[root@openshift-sno ~]# mount | grep sdb
/dev/sdb on /var/lib/kubelet/plugins/kubernetes.io/iscsi/iface-default/192.168.100.10:3260-iqn.2003-01.org.linux-iscsi.iscsi-target-host.x8664:sn.aabbccddeeff-lun-0 type xfs (rw,relatime,seclabel,attr2,inode64,logbufs=8,logbsize=32k,noquota)
/dev/sdb on /var/lib/kubelet/pods/25a1c200-c53b-4774-8d42-2c9442d941be/volumes/kubernetes.io~iscsi/iscsi-pv type xfs (rw,relatime,seclabel,attr2,inode64,logbufs=8,logbsize=32k,noquota)
```

## How to configure multipath with OpenShift

In Red Hat OpenShift Container Platform 4.14, it is also possible to configure multipathing. The OpenShift documentation
explains how to configure
[multipathing](https://docs.openshift.com/container-platform/4.14/storage/persistent_storage/persistent-storage-iscsi.html#iscsi-multipath_persistent-storage-iscsi)
with multiple portals.
Note that in order to configure true multipathing, you must make sure to configure the `multipathd` daemon first, see
[https://access.redhat.com/solutions/5607891](https://access.redhat.com/solutions/5607891).
Otherwise, both iSCSI targets will be added by the kernel, but you aren't going to see true multipathing via multipathd.

Create the multipath.conf file:

```
cat << 'EOF' > /tmp/multipath.conf
defaults {
        user_friendly_names yes
}
EOF
```

And then deploy it to your nodes with a MachineConfig. The following example is for a Single Node OpenShift deployment and therefore targets the `master` role:

```
cat << EOF | oc apply -f -
apiVersion: machineconfiguration.openshift.io/v1
kind: MachineConfig
metadata:
  labels:
    machineconfiguration.openshift.io/role: master
  name: 99-master-multipath-conf
spec:
  config:
    ignition:
      version: 2.2.0
    storage:
      files:
      - contents:
          source: data:text/plain;charset=utf-8;base64,$(cat /tmp/multipath.conf | base64 -w0)
        filesystem: root
        mode: 420
        path: /etc/multipath.conf
EOF
```

After the node rebooted, you should be able to connect to it and list all multipaths on the node:

```
multipath -l
```

You can now create a multipath PersistentVolume, for example with:

```
apiVersion: v1
kind: PersistentVolume
metadata:
  name: iscsi-pv
spec:
  storageClassName: manual
  capacity:
    storage: 20Gi
  accessModes:
    - ReadWriteOnce
  iscsi:
     targetPortal: 192.168.100.123:3260
     portals: ['192.168.100.124:3260']
     iqn: iqn.2003-01.org.linux-iscsi.iscsi-target-host.x8664:sn.aabbccddeeff
     lun: 0
     fsType: 'xfs'
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: iscsi-pv-claim
spec:
  storageClassName: manual
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 20Gi
---
apiVersion: v1
kind: Pod
metadata:
  name: iscsi-pv-pod
spec:
  containers:
    - name: iscsi-pv-container
      image: fedora
      command:
        - sleep
        - infinity
      volumeMounts:
        - mountPath: "/iscsi"
          name: iscsi-pv-storage
  volumes:
    - name: iscsi-pv-storage
      persistentVolumeClaim:
        claimName: iscsi-pv-claim
```
 
Apply the above, and verify that everything is up and running as expected:

```
[user@host ~]$ oc get pv
oNAME       CAPACITY   ACCESS MODES   RECLAIM POLICY   STATUS   CLAIM                 STORAGECLASS   REASON   AGE
iscsi-pv   20Gi       RWO            Retain           Bound    test/iscsi-pv-claim   manual                  22s
[user@host ~]$ oc get pvc
ocNAME             STATUS   VOLUME     CAPACITY   ACCESS MODES   STORAGECLASS   AGE
iscsi-pv-claim   Bound    iscsi-pv   20Gi       RWO            manual         22s
[user@host ~]$ oc get pods
NAME           READY   STATUS    RESTARTS   AGE
iscsi-pv-pod   1/1     Running   0          24s
```
 
On the host, you can now query the multipath state:

```
[root@openshift-sno ~]# multipath -l | tail -n 6
mpathf (360014051ce1f6918cfe457e972b2f2ae) dm-4 LIO-ORG,vdb
size=20G features='0' hwhandler='1 alua' wp=rw
|-+- policy='service-time 0' prio=0 status=active
| `- 11:0:0:0  sdb     8:16  active undef running
`-+- policy='service-time 0' prio=0 status=enabled
  `- 12:0:0:0  sdc     8:32  active undef running
```

In addition, you can verify that the host mounted the multipath into the container:
```
[root@openshift-sno ~]# mount | grep mpathf
/dev/mapper/mpathf on /var/lib/kubelet/plugins/kubernetes.io/iscsi/iface-default/192.168.100.123:3260-iqn.2003-01.org.linux-iscsi.iscsi-target-host.x8664:sn.aabbccddeeff-lun-0 type xfs (rw,relatime,seclabel,attr2,inode64,logbufs=8,logbsize=32k,noquota)
/dev/mapper/mpathf on /var/lib/kubelet/pods/474111cd-0a86-4dd7-9b13-7a9c0d3217dc/volumes/kubernetes.io~iscsi/iscsi-pv type xfs (rw,relatime,seclabel,attr2,inode64,logbufs=8,logbsize=32k,noquota)
```
 
On the target, simulate a path failure:

```
[root@iscsi-target-host ~]# iptables -I INPUT --dst 192.168.100.124 --j REJECT
```

The multipath daemon will detect the failure:

```
[root@openshift-sno ~]# multipath -l | tail -n 6
mpathf (360014051ce1f6918cfe457e972b2f2ae) dm-4 LIO-ORG,vdb
size=20G features='0' hwhandler='1 alua' wp=rw
|-+- policy='service-time 0' prio=0 status=active
| `- 11:0:0:0  sdb     8:16  active undef running
`-+- policy='service-time 0' prio=0 status=enabled
  `- 12:0:0:0  sdc     8:32  failed faulty running
```

But your pod can still list the contents of the iSCSI disk:

```
[user@host ~]$ oc exec -it iscsi-pv-pod -- ls /iscsi
a  b  c  d
```

You can repeat the test for the other path by removing the current iptables rule, waiting for both paths to become
active again, and creating an iptables rule that drops traffic to the other path.
