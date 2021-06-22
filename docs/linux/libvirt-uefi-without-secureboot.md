# RHEL: Booting a virtual machine with UEFI but without secure boot

## Sources

* [https://github.com/tianocore/tianocore.github.io/wiki/OVMF](https://github.com/tianocore/tianocore.github.io/wiki/OVMF)
* [https://www.kraxel.org/repos/](https://www.kraxel.org/repos/)
* [https://fedoraproject.org/wiki/Using_UEFI_with_QEMU#Install_a_Fedora_VM_with_UEFI](https://fedoraproject.org/wiki/Using_UEFI_with_QEMU#Install_a_Fedora_VM_with_UEFI)

## How to

The default RHEL/CentOS/Fedora RPMs only provide UEFI firmware with Secure Boot enforced.
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

According to [https://libvirt.org/formatdomain.html#elementsOSBIOS](https://libvirt.org/formatdomain.html#elementsOSBIOS), this cannot be disabled. 

Instead, download upstream UEFI firmware from the OVMF project without secure boot:
~~~
cd /etc/yum.repos.d/
curl -O https://www.kraxel.org/repos/firmware.repo
yum install edk2.git-ovmf-x64 -y
~~~

Use virt-install to generate the XML for your virtual machine, e.g.:
~~~
virt-install --name f34-uefi --network network=management    --ram 2048 --disk size=20    --boot uefi    --location https://dl.fedoraproject.org/pub/fedora/linux/releases/34/Server/x86_64/os/ --dry-run --print-xml
~~~

You will have to add disks and NICs to that VM and then you can define it. Or, you define it first and then use virt-manager to conveniently add the relevant components. The above is just a rudimentary example.

The XML will contain the following:
~~~
  <os>
    <type arch='x86_64' machine='pc-q35-rhel8.2.0'>hvm</type>
    <loader readonly='yes' secure='yes' type='pflash'>/usr/share/edk2/ovmf/OVMF_CODE.secboot.fd</loader>
    <nvram>/var/lib/libvirt/qemu/nvram/f34-uefi_VARS.fd</nvram>
    <bootmenu enable='yes'/>
  </os>
~~~

Change that to:
~~~
  <os>
    <type arch='x86_64' machine='pc-q35-rhel8.2.0'>hvm</type>
    <loader readonly='yes' secure='no' type='pflash'>/usr/share/edk2.git/ovmf-x64/OVMF_CODE-pure-efi.fd</loader>
    <nvram>/var/lib/libvirt/qemu/nvram/f34-uefi_VARS.fd</nvram>
    <bootmenu enable='yes'/>
  </os>
~~~

And make sure to copy the `*_VARS.fd` file:
~~~
cp /usr/share/edk2.git/ovmf-x64/OVMF_VARS-pure-efi.fd /var/lib/libvirt/qemu/nvram/f34-uefi_VARS.fd
~~~
