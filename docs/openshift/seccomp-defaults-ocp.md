# Seccomp defaults in Red Hat OpenShift Container Platform

Seccomp can be used to restrict the syscalls that processes running inside a container are allowed to make to the
kernel. A pod can
[explicitly request](https://kubernetes.io/docs/tutorials/security/seccomp/#create-a-pod-that-uses-the-container-runtime-default-seccomp-profile)
to use a seccomp profile via field `.spec.securityContext.seccompProfile`. The seccomp profile can be of types
`Localhost` (point to a local file containing a seccomp profile), `RuntimeDefault` (use the container runtime engine's
default seccomp profile) or `Unconfined` (use no seccomp profile for the container).

```
apiVersion: v1
kind: Pod
metadata:
  name: pod0
  labels:
    app: pod0
spec:
  securityContext:
    seccompProfile:
      type: RuntimeDefault
(...)
```

Historically, Kubernetes had no way to enforce default seccomp profiles and pods would run unconfined unless explicitly
requested for each pod. In recent versions of upstream Kubernetes, administrators
[can specify that all pods' seccomp profile shall default to the runtime default](https://kubernetes.io/docs/tutorials/security/seccomp/#enable-the-use-of-runtimedefault-as-the-default-seccomp-profile-for-all-workloads)
with the `--seccomp-default` command line flag.

## Enforcement of RuntimeDefault seccomp rules in Red Hat OpenShift Container Platform

Contrary to upstream Kubernetes, in Red Hat OpenShift Container Platform (OCP) 4.12 and beyond, the use of the runtime
default seccomp profile is enforced through SCCs. For pods that do not explicitly request a seccomp profile but which
are matched by an SCC which specifies the `.seccompProfiles` field, the SCC controller will automatically set the pods'
`.spec.securityContext.seccompProfile` to the value which is requested by the SCC. One of the reasons behind the
introduction of the new `(...)-v2` SCCs in Red Hat OpenShift Container Platform was the automatic enforcement of the
`RuntimeDefault` seccomp profile for pods.

```
$ oc get scc restricted -o custom-columns=SECCOMP_PROFILE:.seccompProfiles
SECCOMP_PROFILE
<none>
$ oc get scc restricted-v2 -o custom-columns=SECCOMP_PROFILE:.seccompProfiles
SECCOMP_PROFILE
[runtime/default]
```

In turn, for OCP this means that when you inspect a pod and neither `.spec.securityContext.seccompProfile` nor
`.spec.containers.securityContext.seccompProfile` are set, the pod will run unconfined seccomp. For example, this can
happen when a pod is matched by the `privileged` SCC.

## Querying the effective seccomp profile

Beyond `oc get pod` or `oc describe pod`, administrators have several different ways to query a container's effective
seccomp. One way is to inspect the containers with `crictl inspect`. When no seccomp profile is applied, field
`.info.runtimeSpec.linux.seccomp` will be empty.

```
# crictl inspect $(crictl ps | awk '/pod-unconfined$/ {print $1}') | jq '.info.runtimeSpec.linux.seccomp'
null
```

When a seccomp profile is applied, field `.info.runtimeSpec.linux.seccomp` will contain the full seccomp profile
definition:

```
# crictl inspect $(crictl ps | awk '/pod-runtime-default$/ {print $1}') | jq '.info.runtimeSpec.linux.seccomp' | head
{
  "defaultAction": "SCMP_ACT_ERRNO",
  "defaultErrnoRet": 38,
  "architectures": [
    "SCMP_ARCH_X86_64",
    "SCMP_ARCH_X86",
    "SCMP_ARCH_X32"
  ],
  "syscalls": [
    {
# crictl inspect $(crictl ps | awk '/default-pod$/ {print $1}') | jq '.info.runtimeSpec.linux.seccomp' | wc -l
637
```

A low level way of determining if a process inside a container is restricted by seccomp is to look at the
`/proc/${pid}/status` file. It contains a field `Seccomp` which will show the processes' seccomp mode, which is either
`0` (`SECCOMP_MODE_DISABLED`), `1` (`SECCOMP_MODE_STRICT`) or `2` (`SECCOMP_MODE_FILTER`).

Therefore, an unconfined container will yield the following:

```
$ oc exec pod-unconfined -- grep Seccomp /proc/1/status

Seccomp: 0
```

Whereas a confined container will show `2` for process `1`'s seccomp status:

```
$ oc exec pod-runtime-default -- grep Seccomp /proc/1/status

Seccomp: 2
```
