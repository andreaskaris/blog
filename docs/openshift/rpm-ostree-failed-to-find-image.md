## rpm-ostreed failed to find image

Today, I ran into a strange issue after messing around a bit too much with OpenShift's Machine Config Operator.
After deploying a MachineConfig with a custom `osImageURL:` and after undeploying the MachineConfig, the node's
rpm-ostreed service got stuck with the following error message:
```
[root@bos2kyoung04 ~]# systemctl status rpm-ostreed
● rpm-ostreed.service - rpm-ostree System Management Daemon
(...)
   Status: "error: Couldn't start daemon: Error setting up sysroot: Reading deployment 1: Failed to find image ostree-unverified-image:docker://registry.example.com/dir/image:tag"
      CPU: 33ms

Aug 15 17:54:38 host.example.com systemd[1]: Starting rpm-ostree System Management Daemon...
Aug 15 17:54:38 host.example.com rpm-ostree[301754]: Reading config file '/etc/rpm-ostreed.conf'
Aug 15 17:54:38 host.example.com rpm-ostree[301754]: Failed to find image ostree-unverified-image:docker://registry.example.com/dir/image:tag
Aug 15 17:54:38 host.example.com rpm-ostree[301754]: error: Couldn't start daemon: Error setting up sysroot: Reading deployment 1: Failed to find image ostree-unverified-image:docker://registry.example.com/dir/image:tag
Aug 15 17:54:38 host.example.com systemd[1]: rpm-ostreed.service: Main process exited, code=exited, status=1/FAILURE
Aug 15 17:54:38 host.example.com systemd[1]: rpm-ostreed.service: Failed with result 'exit-code'.
Aug 15 17:54:38 host.example.com systemd[1]: Failed to start rpm-ostree System Management Daemon.
Aug 15 17:54:38 host.example.com systemd[1]: rpm-ostreed.service: Consumed 33ms CPU time
```

I found the solution to my issue in
[https://github.com/coreos/rpm-ostree/issues/4185](https://github.com/coreos/rpm-ostree/issues/4185):
```
unshare -m /bin/sh -c \
  'mount -o remount,rw /sysroot && ostree container image pull /ostree/repo ostree-unverified-image:docker://registry.example.com/dir/image:tag'
Wrote: ostree-unverified-image:docker://registry.example.com/dir/image:tag => e8d83702a7c9a86b67d67ea6991a7bcfea717f4bf756fe031619a9d560283d81
```

Followed by a restart of rpm-ostreed:
```
systemctl restart rpm-ostreed
```
