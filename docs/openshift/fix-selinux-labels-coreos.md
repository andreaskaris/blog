# Fix SElinux labels on CoreOS 

If your CoreOS fails to boot due to SELinux issue.

## Option 1

Reboot the system

Inside the grub menu:

* hit `e` to modify the entry, then remove the `console=<...>` parameters (there are 2 of them) from the kernel cmdline

* append `rd.break`

Hit CTRL-x to boot, once dracut prompts for it hit enter to get into the emergency console.

Then run:
~~~
mount -o remount,rw /sysroot
mount -o remount,rw /sysroot/usr
chroot /sysroot
setfiles /etc/selinux/targeted/contexts/files/file_contexts /
setfiles /etc/selinux/targeted/contexts/files/file_contexts /*
~~~

Mileage may vary with this command.

## Option 2

Did not work for me: Restorecon from selinux permissive mode

Edit the grub kernel cmdline. Change the kernel cmdline and append:
~~~
selinux=0
~~~

Then, edit /etc/selinux/config and set the system to "permissive"
~~~
vi /etc/selinux/config
~~~

Then, reboot the system.
~~~
reboot
~~~

Then, run the following to fix the labels again:
~~~
restorecon -RF / 2>/dev/null
~~~

Then, change /etc/selinux/config again to `enforcing`:
~~~
vi /etc/selinux/config
~~~

## Option 3

coreos-relabel from rd.break 

Remove the console=<...> parameters (there are 2 of them) from the CLI and append rd.break

Then, run:
~~~
mount -o remount,rw /sysroot
mount -o remount,rw /sysroot/usr
coreos-relabel /sysroot/*
~~~

## Resources

[https://github.com/coreos/ignition-dracut/pull/138](https://github.com/coreos/ignition-dracut/pull/138)
[https://github.com/coreos/fedora-coreos-config/search?q=coreos-relabel](https://github.com/coreos/fedora-coreos-config/search?q)
[https://github.com/coreos/ignition/pull/996](https://github.com/coreos/ignition/pull/996)
[https://lore.kernel.org/selinux/20190819193032.848-1-jlebon@redhat.com/](https://lore.kernel.org/selinux/20190819193032.848-1-jlebon)
[https://github.com/coreos/ignition/pull/846/commits/80ca4b2f834007ea13762cec5f9df43fd7061d00](https://github.com/coreos/ignition/pull/846/commits/80ca4b2f834007ea13762cec5f9df43fd7061d00)
[https://github.com/coreos/fedora-coreos-tracker/issues/94](https://github.com/coreos/fedora-coreos-tracker/issues/94https://github.com/coreos/fedora-coreos-tracker/issues/94)
[https://github.com/coreos/ignition/issues/635](https://github.com/coreos/ignition/issues/635)
