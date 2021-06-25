# RHEL: Booting a virtual machine with UEFI but without secure boot

## Sources

* [https://github.com/tianocore/tianocore.github.io/wiki/OVMF](https://github.com/tianocore/tianocore.github.io/wiki/OVMF)
* [https://www.kraxel.org/repos/](https://www.kraxel.org/repos/)
* [https://fedoraproject.org/wiki/Using_UEFI_with_QEMU#Install_a_Fedora_VM_with_UEFI](https://fedoraproject.org/wiki/Using_UEFI_with_QEMU#Install_a_Fedora_VM_with_UEFI)
* [https://that.guru/blog/uefi-secure-boot-in-libvirt/](https://that.guru/blog/uefi-secure-boot-in-libvirt/)
* [https://bugzilla.redhat.com/show_bug.cgi?id=1906500#c23](https://bugzilla.redhat.com/show_bug.cgi?id=1906500#c23)
* [https://bugzilla.redhat.com/show_bug.cgi?id=1929357](https://bugzilla.redhat.com/show_bug.cgi?id=1929357)

## About Secure Boot with libvirt on RHEL type distributions

The default RHEL/CentOS/Fedora RPMs provide a UEFI firmware file named `/usr/share/edk2/ovmf/OVMF_CODE.secboot.fd`. The actual firmware can be configured to enforce Secure Boot or to ignore it. More on this later.

The firmware is bundled in RPM `edk2-ovmf-...`:
~~~
# rpm -qa | grep ovmf
edk2-ovmf-20200602gitca407c7246bf-4.el8.noarch
# rpm -ql edk2-ovmf-20200602gitca407c7246bf-4.el8.noarch
/usr/share/OVMF
/usr/share/OVMF/OVMF_CODE.secboot.fd
/usr/share/OVMF/OVMF_VARS.fd
/usr/share/OVMF/OVMF_VARS.secboot.fd
/usr/share/OVMF/UefiShell.iso
/usr/share/doc/edk2-ovmf
/usr/share/doc/edk2-ovmf/README
/usr/share/doc/edk2-ovmf/ovmf-whitepaper-c770f8c.txt
/usr/share/edk2
/usr/share/edk2/ovmf
/usr/share/edk2/ovmf/EnrollDefaultKeys.efi
/usr/share/edk2/ovmf/OVMF_CODE.secboot.fd
/usr/share/edk2/ovmf/OVMF_VARS.fd
/usr/share/edk2/ovmf/OVMF_VARS.secboot.fd
/usr/share/edk2/ovmf/Shell.efi
/usr/share/edk2/ovmf/UefiShell.iso
/usr/share/licenses/edk2-ovmf
/usr/share/licenses/edk2-ovmf/LICENSE.openssl
/usr/share/licenses/edk2-ovmf/License-History.txt
/usr/share/licenses/edk2-ovmf/License.OvmfPkg.txt
/usr/share/licenses/edk2-ovmf/License.txt
/usr/share/qemu
/usr/share/qemu/firmware
/usr/share/qemu/firmware/40-edk2-ovmf-sb.json
/usr/share/qemu/firmware/50-edk2-ovmf.json
~~~

When creating a domain with `virt-install`, the configuration will default do secure boot enabled:
~~~
virt-install --name f34-uefi --network network=management    --ram 2048 --disk size=20    --boot uefi    --location https://dl.fedoraproject.org/pub/fedora/linux/releases/34/Server/x86_64/os/ --dry-run --print-xml
~~~

The XML will contain the following:
~~~
  <os>
    <type arch='x86_64' machine='pc-q35-rhel8.2.0'>hvm</type>
    <loader readonly='yes' secure='yes' type='pflash'>/usr/share/edk2/ovmf/OVMF_CODE.secboot.fd</loader>
    <nvram>/var/lib/libvirt/qemu/nvram/f34-uefi_VARS.fd</nvram>
    <bootmenu enable='yes'/>
  </os>
~~~

According to [https://libvirt.org/formatdomain.html#elementsOSBIOS](https://libvirt.org/formatdomain.html#elementsOSBIOS), this cannot be disabled by setting `secure='no'`.

## Creating a virtual machine with UEFI

For all of the following tests, we will need a virtual machine with UEFI enabled.

Use virt-install to generate the XML for your virtual machine, e.g.:
~~~
virt-install --name f34-uefi --network network=management    --ram 2048 --disk size=20    --boot uefi    --location https://dl.fedoraproject.org/pub/fedora/linux/releases/34/Server/x86_64/os/ --dry-run --print-xml > vm.xml
~~~

You will have to add disks and NICs to that VM and then you can create the actual domain. Or, you define it first and then use virt-manager to conveniently add the relevant components. The above is just a rudimentary example.

The XML will contain the following configuration which will enable UEFI with secboot by default:
~~~
  <os>
    <type arch='x86_64' machine='pc-q35-rhel8.2.0'>hvm</type>
    <loader readonly='yes' secure='yes' type='pflash'>/usr/share/edk2/ovmf/OVMF_CODE.secboot.fd</loader>
    <nvram>/var/lib/libvirt/qemu/nvram/f34-uefi_VARS.fd</nvram>
    <bootmenu enable='yes'/>
  </os>
~~~

Define the VM with `virsh define ...` once the XML looks good to you.

## Testing UEFI and Secure Boot in action

Download Alpine Linux from [https://alpinelinux.org/downloads/](https://alpinelinux.org/downloads/)
~~~
curl -O https://dl-cdn.alpinelinux.org/alpine/v3.14/releases/x86_64/alpine-extended-3.14.0-x86_64.iso
~~~

Now, start the VM with the Alpine ISO as the first entry in the boot order:
~~~
  <devices>
(...)
    <disk type='file' device='cdrom'>
      <driver name='qemu' type='raw'/>
      <source file='/var/lib/libvirt/images/alpine-extended-3.14.0-x86_64.iso'/>
      <target dev='sda' bus='sata'/>
      <readonly/>
      <boot order='1'/>
      <address type='drive' controller='0' bus='0' target='0' unit='0'/>
    </disk>
(...)
  </devices>
~~~

When the VM starts and tries to boot from the ISO, you will see:
~~~
# virsh start vm1 ; virsh console vm1
Domain vm1 started

Connected to domain openshift-master-1-uefi
Escape character is ^]

BdsDxe: loading Boot0001 "UEFI QEMU DVD-ROM QM00001 " from PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0xFFFF,0x0)
BdsDxe: failed to load Boot0001 "UEFI QEMU DVD-ROM QM00001 " from PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0xFFFF,0x0): Access Denied
~~~

## Disabling Secure Boot

Keep everything as is, but make sure to overwrite the VM's nvram which is in `/var/lib/libvirt/qemu/nvram/f34-uefi_VARS.fd` with the non Secure Boot variables to disable the feature. As a reminder, from the VM's XML:
~~~
  <os>
    <type arch='x86_64' machine='pc-q35-rhel8.2.0'>hvm</type>
    <loader readonly='yes' secure='no' type='pflash'>/usr/share/edk2.git/ovmf-x64/OVMF_CODE-pure-efi.fd</loader>
    <nvram>/var/lib/libvirt/qemu/nvram/f34-uefi_VARS.fd</nvram>
    <bootmenu enable='yes'/>
  </os>
~~~

Use the non Secure Boot nvram:
~~~
\cp /usr/share/edk2/ovmf/OVMF_VARS.fd /var/lib/libvirt/qemu/nvram/f34-uefi_VARS.fd
~~~

The VM will now boot without issues.

### Using custom UEFI firmware from upstream

If you are feeling adventurous, you can download upstream UEFI firmware from the OVMF project without secure boot:
~~~
cd /etc/yum.repos.d/
curl -O https://www.kraxel.org/repos/firmware.repo
yum install edk2.git-ovmf-x64 -y
~~~

The default XML will contain the following:
~~~
  <os>
    <type arch='x86_64' machine='pc-q35-rhel8.2.0'>hvm</type>
    <loader readonly='yes' secure='yes' type='pflash'>/usr/share/edk2/ovmf/OVMF_CODE.secboot.fd</loader>
    <nvram>/var/lib/libvirt/qemu/nvram/f34-uefi_VARS.fd</nvram>
    <bootmenu enable='yes'/>
  </os>
~~~

Change that to refer to the upstream firmware:
~~~
  <os>
    <type arch='x86_64' machine='pc-q35-rhel8.2.0'>hvm</type>
    <loader readonly='yes' secure='no' type='pflash'>/usr/share/edk2.git/ovmf-x64/OVMF_CODE-pure-efi.fd</loader>
    <nvram>/var/lib/libvirt/qemu/nvram/f34-uefi_VARS.fd</nvram>
    <bootmenu enable='yes'/>
  </os>
~~~

And make sure to copy the matching `*_VARS.fd` file:
~~~
cp /usr/share/edk2.git/ovmf-x64/OVMF_VARS-pure-efi.fd /var/lib/libvirt/qemu/nvram/f34-uefi_VARS.fd
~~~
