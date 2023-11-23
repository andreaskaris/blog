# kernel-ml on OpenShift

In order to find out if a kernel bug was already fixed upstream, it may sometimes be necessary to test the upstream
kernel on top of OpenShift. RHEL users can easily use the kernel-ml package (http://elrepo.org/tiki/HomePage) for
such testing. Unfortunately, it is a bit more complex to achieve this on Red Hat CoreOS with rpm-ostree, but it is
definitely possible. Keep in mind that this is for testing and tinkering only, and should not be done on production
systems.

## Preparation steps

You can download the kernel-ml packages (for example with yumdownloader). For OpenShift 4.12 with the RHEL 8.6 kernel,
you will have to download `kernel-ml`, `kernel-ml-core`, `kernel-ml-modules` and `kernel-ml-modules-extra`. I didn't
test this with OCP 4.13 and above (which are based on RHEL 9), but I expect the procedure to be similar.

Let's tar the packages into a single tar archive:
```
$ tar -tf kernel-ml-6.6.2-1.el8.tar.gz
kernel-ml-6.6.2-1.el8/
kernel-ml-6.6.2-1.el8/kernel-ml-6.6.2-1.el8.elrepo.x86_64.rpm
kernel-ml-6.6.2-1.el8/kernel-ml-core-6.6.2-1.el8.elrepo.x86_64.rpm
kernel-ml-6.6.2-1.el8/kernel-ml-modules-6.6.2-1.el8.elrepo.x86_64.rpm
kernel-ml-6.6.2-1.el8/kernel-ml-modules-extra-6.6.2-1.el8.elrepo.x86_64.rpm
```

## Installation

Now, transfer the tar archive to the node:
```
KERNEL=kernel-ml-6.6.2-1.el8
HOST=<host IP>
scp ${KERNEL}.tar.gz core@${HOST}:
```

Next, SSH or login to the node and switch to root user
```
ssh core@${HOST}
sudo -i
```

Then, reset the kernel (needed only if there's already an override / a custom kernel in place):
```
rpm-ostree reset
```
> Note: This should not be necessary if this is the first time you're running this procedure. However, if you see a
message indicating that the kernel was downgraded, reboot the node first.

Finally, install the kernel-ml packages. This command will take a few minutes while rpm-ostree replaces packages and
recreates the initramfs:
```
KERNEL=kernel-ml-6.6.2-1.el8
tar -xf /home/core/${KERNEL}.tar.gz
cd ${KERNEL}/
rpm-ostree override remove kernel kernel-core kernel-modules kernel-modules-extra --install kernel-ml-6.6.2-1.el8.elrepo.x86_64.rpm --install kernel-ml-core-6.6.2-1.el8.elrepo.x86_64.rpm --install kernel-ml-modules-6.6.2-1.el8.elrepo.x86_64.rpm --install kernel-ml-modules-extra-6.6.2-1.el8.elrepo.x86_64.rpm
```

Before rebooting the node, check the status of rpm-ostree:
```
rpm-ostree status
```

You should see something along the following lines:
```
# rpm-ostree status
State: idle
Deployments:
  ostree-unverified-registry:quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:ac009e6f53dcff62b363e925a6ab30c6da0e31e3587316c6410f687b8e63305b
                   Digest: sha256:ac009e6f53dcff62b363e925a6ab30c6da0e31e3587316c6410f687b8e63305b
                  Version: 412.86.202308081039-0 (2023-09-18T17:50:17Z)
                     Diff: 4 removed, 4 added
      RemovedBasePackages: kernel-core kernel-modules kernel kernel-modules-extra 4.18.0-372.69.1.el8_6
            LocalPackages: kernel-ml-6.6.2-1.el8.elrepo.x86_64 kernel-ml-core-6.6.2-1.el8.elrepo.x86_64
                           kernel-ml-modules-6.6.2-1.el8.elrepo.x86_64
                           kernel-ml-modules-extra-6.6.2-1.el8.elrepo.x86_64

* ostree-unverified-registry:quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:ac009e6f53dcff62b363e925a6ab30c6da0e31e3587316c6410f687b8e63305b
                   Digest: sha256:ac009e6f53dcff62b363e925a6ab30c6da0e31e3587316c6410f687b8e63305b
                  Version: 412.86.202308081039-0 (2023-09-18T17:50:17Z)
```

Reboot the node:
```
systemctl reboot
```

After node reboot, make sure that the kernel is upgraded and also verify that the kubelet is running correctly and
that containers spawn on the node:
```
uname -r
sudo rpm-ostree status
```


## Rollback

It's significantly easier to roll back. You will only need the following commands:
```
sudo -i
rpm-ostree reset
systemctl reboot
```
