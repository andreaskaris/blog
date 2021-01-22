## How RHCOS updates work

* https://github.com/openshift/machine-config-operator/blob/1d4e8d0a4658fe9a9683bdf41c6444db10ee876a/docs/MachineConfigDaemon.md#os-updates

~~~
OS updates

In addition to handling Ignition configs, the MachineConfigDaemon also takes care of updating the base operating system.

Updates are provided via the OSImageURL component of a MachineConfig object. This should generally be controlled by the cluster-version-operator, and its current existence in MachineConfig objects should be thought of as an implementation detail.

MachineConfigDaemon only supports updating Red Hat CoreOS, which uses rpm-ostree. The OSImageURL refers to a container image that carries inside it an OSTree payload. When the OSImageURL changes, it will be passed to the pivot command which is included in Red Hat CoreOS, and in turn takes care of passing it to rpm-ostree.

Once an update is prepared (in terms of a new bootloader entry which points to a new OSTree "deployment" or filesystem tree), then the MachineConfigDaemon will reboot.
~~~

## Force pivoting a node's OS via the MachineConfigOperator or pivot

### Obtaining the image URL

Select a release that you would like to pivot to from:
* https://quay.io/repository/openshift-release-dev/ocp-release?tag=latest&tab=tags

Select a release image and get the docker/podman pull for that release image:
~~~
docker pull quay.io/openshift-release-dev/ocp-release@sha256:8a9e40df2a19db4cc51dc8624d54163bef6e88b7d88cc0f577652ba25466e338
~~~

Follow https://andreaskaris.github.io/blog/openshift/mounting-container-image/ to inspect the image on an external host:
~~~
# yum install buildah -y
# buildah from quay.io/openshift-release-dev/ocp-release@sha256:8a9e40df2a19db4cc51dc8624d54163bef6e88b7d88cc0f577652ba25466e338
# buildah ps
CONTAINER ID  BUILDER  IMAGE ID     IMAGE NAME                       CONTAINER NAME
970e896c4c23     *     9fd52d5d304d quay.io/openshift-release-dev... ocp-release-working-container
# buildah mount ocp-release-working-container
/var/lib/containers/storage/overlay/d9682dba5a0b1291805c6b430124f6d7a2bab23d22c28c3f023932d17a29cc5b/merged
~~~

Now, inspect the image catalog:
~~~
# DIR=/var/lib/containers/storage/overlay/d9682dba5a0b1291805c6b430124f6d7a2bab23d22c28c3f023932d17a29cc5b/merged
# less $DIR/release-manifests/image-references
~~~

And get the machine-os image:
~~~
# cat $DIR/release-manifests/image-references | jq '.spec.tags[] | select(.name == "machine-os-content") .from.name'
{
  "name": "machine-os-content",
  "annotations": {
    "io.openshift.build.commit.id": "",
    "io.openshift.build.commit.ref": "",
    "io.openshift.build.source-location": "",
    "io.openshift.build.version-display-names": "machine-os=Red Hat Enterprise Linux CoreOS",
    "io.openshift.build.versions": "machine-os=46.82.202101191342-0",
    "release.openshift.io/inconsistency": "[\"Multiple versions of RPM containers-common used\", \"Multiple versions of RPM cri-o used\", \"Multiple versions of RPM dnsmasq used\", \"Multiple versions of RPM skopeo used\"]"
  },
  "from": {
    "kind": "DockerImage",
    "name": "quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515"
  },
  "generation": 2,
  "importPolicy": {},
  "referencePolicy": {
    "type": "Source"
  }
}
~~~

~~~
# cat $DIR/release-manifests/image-references | jq '.spec.tags[] | select(.name == "machine-os-content") .from.name'
"quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515"
~~~

### Using pivot command to force pivot on a node

Using `/run/bin/machine-config-daemon pivot`, it is possible to pivot to another os image. As this can easily run into issues, one should have IPMI access to the node:
~~~
# ssh core@openshift-worker-0.example.com
(...)
[root@openshift-worker-0 ~]# rpm-ostree status
State: idle
Deployments:
● pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202012051820-0 (2020-12-05T18:24:10Z)

  pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202012051820-0 (2020-12-05T18:24:10Z)
[root@openshift-worker-0 ~]# /run/bin/machine-config-daemon pivot quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515
I0121 22:27:41.460321   13015 run.go:18] Running: nice -- ionice -c 3 oc image extract --path /:/run/mco-machine-os-content/os-content-638982111 --registry-config /var/lib/kubelet/config.json quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515
I0121 22:28:00.620039   13015 rpm-ostree.go:261] Running captured: rpm-ostree status --json
I0121 22:28:00.655338   13015 rpm-ostree.go:179] Previous pivot: quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
I0121 22:28:01.701252   13015 rpm-ostree.go:211] Pivoting to: 46.82.202101191342-0 (478636b3f9d960112359f202481507e4c5467dcccdc4e8faba291de4abb3b8bc)
I0121 22:28:01.701306   13015 rpm-ostree.go:243] Executing rebase from repo path /run/mco-machine-os-content/os-content-638982111/srv/repo with customImageURL pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515 and checksum 478636b3f9d960112359f202481507e4c5467dcccdc4e8faba291de4abb3b8bc
I0121 22:28:01.701314   13015 rpm-ostree.go:261] Running captured: rpm-ostree rebase --experimental /run/mco-machine-os-content/os-content-638982111/srv/repo:478636b3f9d960112359f202481507e4c5467dcccdc4e8faba291de4abb3b8bc --custom-origin-url pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515 --custom-origin-description Managed by machine-config-operator
[root@openshift-worker-0 ~]# rpm-ostree status
State: idle
Deployments:
  pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202101191342-0 (2021-01-19T13:45:40Z)
                      Diff: 19 upgraded

● pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202012051820-0 (2020-12-05T18:24:10Z)

  pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202012051820-0 (2020-12-05T18:24:10Z)
[root@openshift-worker-0 ~]# reboot
Connection to openshift-worker-0.example.com closed by remote host.
Connection to openshift-worker-0.example.com closed.
~~~

After the reboot, verify:
~~~
[root@openshift-worker-0 ~]# rpm-ostree status
State: idle
Deployments:
● pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202101191342-0 (2021-01-19T13:45:40Z)

  pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202012051820-0 (2020-12-05T18:24:10Z)
~~~

Note that the MachineConfigPool for the role will now report a mismatch:
~~~
# oc describe mcp worker
(...)
    Message:               Node openshift-worker-0 is reporting: "unexpected on-disk state validating against rendered-worker-0820700c2d496a3cacc73c82b7cd5c5f: expected target osImageURL \"quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024\", have \"quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515\""
(...)
~~~

To roll back, simply pivot back to the original image on the same worker node:
~~~
# /run/bin/machine-config-daemon pivot quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
~~~

~~~
[root@openshift-worker-0 ~]# /run/bin/machine-config-daemon pivot quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
I0122 10:28:54.824754  701236 run.go:18] Running: nice -- ionice -c 3 oc image extract --path /:/run/mco-machine-os-content/os-content-026372431 --registry-config /var/lib/kubelet/config.json quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
I0122 10:29:25.677174  701236 rpm-ostree.go:261] Running captured: rpm-ostree status --json
I0122 10:29:25.762600  701236 rpm-ostree.go:179] Previous pivot: quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515
I0122 10:29:26.382134  701236 rpm-ostree.go:211] Pivoting to: 46.82.202012051820-0 (7e014b64b96536487eb3701fa4f0e604697730bde2f2f9034c4f56c024c67119)
I0122 10:29:26.382152  701236 rpm-ostree.go:243] Executing rebase from repo path /run/mco-machine-os-content/os-content-026372431/srv/repo with customImageURL pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024 and checksum 7e014b64b96536487eb3701fa4f0e604697730bde2f2f9034c4f56c024c67119
I0122 10:29:26.382177  701236 rpm-ostree.go:261] Running captured: rpm-ostree rebase --experimental /run/mco-machine-os-content/os-content-026372431/srv/repo:7e014b64b96536487eb3701fa4f0e604697730bde2f2f9034c4f56c024c67119 --custom-origin-url pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024 --custom-origin-description Managed by machine-config-operator
[root@openshift-worker-0 ~]# rpm-ostree status
State: idle
Deployments:
  pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202012051820-0 (2020-12-05T18:24:10Z)
                      Diff: 19 downgraded

● pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202101191342-0 (2021-01-19T13:45:40Z)

  pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202012051820-0 (2020-12-05T18:24:10Z)
[root@openshift-worker-0 ~]# reboot
~~~

### Using the  machine-config-osimageurl ConfigMap to pivot all nodes

> For more details, see: https://github.com/openshift/machine-config-operator/blob/master/docs/HACKING.md#applying-a-custom-oscontainer

First, disable the cluster-version-operator:
~~~
$ oc -n openshift-cluster-version scale --replicas=0 deploy/cluster-version-operator
deployment.apps/cluster-version-operator scaled
~~~

Then, update the `machine-config-osimageurl` configmap:
~~~
[root@openshift-jumpserver-0 ~]# oc patch cm -n openshift-machine-config-operator machine-config-osimageurl --type=merge -p '{"data": {"osImageURL":"quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515"}'
configmap/machine-config-osimageurl patched
[root@openshift-jumpserver-0 ~]# oc get -o yaml cm -n openshift-machine-config-operator machine-config-osimageurl
apiVersion: v1
data:
  osImageURL: quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515
  releaseVersion: 4.6.8
kind: ConfigMap
metadata:
  creationTimestamp: "2020-12-22T16:48:36Z"
  managedFields:
  - apiVersion: v1
    fieldsType: FieldsV1
    fieldsV1:
      f:data: {}
    manager: cluster-version-operator
    operation: Update
    time: "2020-12-23T10:47:03Z"
  - apiVersion: v1
    fieldsType: FieldsV1
    fieldsV1:
      f:data:
        f:osImageURL: {}
        f:releaseVersion: {}
    manager: kubectl-patch
    operation: Update
    time: "2021-01-22T10:25:15Z"
  name: machine-config-osimageurl
  namespace: openshift-machine-config-operator
  resourceVersion: "17699272"
  selfLink: /api/v1/namespaces/openshift-machine-config-operator/configmaps/machine-config-osimageurl
  uid: b26e3885-15e8-4ccc-8e84-d03bed956ea9
~~~

> Note: Do not patch `releaseVersion`, or the MachineConfigOperator will complain if it detects a version mismatch:
~~~
# oc logs machine-config-operator-b956c8ccc-mktl5
(...)
E0122 10:30:34.398824       1 operator.go:314] refusing to read osImageURL version "4.6.13", operator version "4.6.8"
(...)
~~~

Now, the MachineConfigOperator will create new `rendered-<role>` MachineConfigurations:
~~~
# oc get mc
NAME                                               GENERATEDBYCONTROLLER                      IGNITIONVERSION   AGE
00-master                                          c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             30d
00-worker                                          c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             30d
01-master-container-runtime                        c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             30d
01-master-kubelet                                  c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             30d
01-worker-container-runtime                        c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             30d
01-worker-kubelet                                  c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             30d
99-master-generated-registries                     c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             30d
99-master-mtu                                                                                 2.2.0             30d
99-master-ssh                                                                                 3.1.0             30d
99-worker-generated-registries                     c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             30d
99-worker-mtu                                                                                 2.2.0             30d
99-worker-ssh                                                                                 3.1.0             30d
rendered-master-219315c6f26485a186b6bab35c0c32ba   c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             3m44s
rendered-master-bc9b6c496348bd8e46b0cf7f5a4d852f   6896f6bc491fde7e8314631652f70841c7f9f31d   3.1.0             30d
rendered-master-ddd54a7e3be109e549ff25b0250472cd   c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             29d
rendered-worker-0820700c2d496a3cacc73c82b7cd5c5f   c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             29d
rendered-worker-643d787eaa872b91fd73db39aba74619   6896f6bc491fde7e8314631652f70841c7f9f31d   3.1.0             30d
rendered-worker-98a743e40098703869e9a8e0083f4f4a   c470febe19e3b004fb99baa6679e7597f50554c5   3.1.0             3m44s
# oc get mc -o yaml rendered-worker-98a743e40098703869e9a8e0083f4f4a | grep -i 'osImageURL'
        f:osImageURL: {}
  osImageURL: quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515
~~~

And the MachineConfigPools will be updated, node by node:
~~~
# oc get mcp
NAME     CONFIG                                             UPDATED   UPDATING   DEGRADED   MACHINECOUNT   READYMACHINECOUNT   UPDATEDMACHINECOUNT   DEGRADEDMACHINECOUNT   AGE
master   rendered-master-ddd54a7e3be109e549ff25b0250472cd   False     True       False      3              1                   1                     0                      30d
worker   rendered-worker-0820700c2d496a3cacc73c82b7cd5c5f   False     True       False      2              0                   0                     0                      30d
~~~

Wait until this completes and then verify on the nodes:
~~~
[root@openshift-master-0 ~]# rpm-ostree status
State: idle
Deployments:
● pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:8869972a7f3cf7be7c936ef5b3277f20da7f5b577d66e37a5b9db25889c2a515
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202101191342-0 (2021-01-19T13:45:40Z)

  pivot://quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:4c5b83a192734ad6aa33da798f51b4b7ebe0f633ed63d53867a0c3fb73993024
              CustomOrigin: Managed by machine-config-operator
                   Version: 46.82.202012051820-0 (2020-12-05T18:24:10Z)
~~~

