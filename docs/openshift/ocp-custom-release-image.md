## Building custom release images for OpenShift

### Using custom container image for a specific component / operator

The following example builds a custom `baremetal-runtimecfg` image and deploys it with `openshift-install` via a custom release image.

### Prerequisite: quay.io repositories

I created 2 repositories for this example:

* Custom component image: quay.io/akaris/baremetal-runtimecfg
* Custom release image locatio: quay.io/akaris/ocp-release

### Prerequsiste: log into all registries

For pulling the release image and also for pushing to your own repositories, make sure to log into all repositories as needed. The oc adm release commands let you specify a custom auth file with the `-a` parameter if needed.

#### Prerequisite: creating a custom component image

For the following example, I will rebuild the `baremetal-runtimecfg` container with some custom modifications. We will then build an `ocp-release` image to deploy the payload at installation time.

Clone the repository:
~~~
git clone https://github.com/openshift/baremetal-runtimecfg.git
cd baremetal-runtimecfg
~~~

Customize the code. Then, build the image:
~~~
make image IMAGE_REPO=akaris IMAGE_NAME=baremetal-runtimecfg
make push IMAGE_REPO=akaris IMAGE_NAME=baremetal-runtimecfg
~~~

We will also need the SHA of that image:
~~~
$ podman inspect quay.io/akaris/baremetal-runtimecfg:latest | grep '"Digest"'
          "Digest": "sha256:9a83210b0536661a826fbddb6be9dcf5cebf16d91e5fe7687b741166ab69c3c2"
~~~

#### Using oc adm release

If you want to use a custom container image for a specific component or operator then this can easily be achieved with the `oc adm release` command. Unfortunately, this feature is not extremely well documented.

Extract the OCP release - in this case, I am going to use the latest 4.12 release condidate that was available at time of this writing:
~~~
mkdir payload
cd payload
oc adm release new --from-release quay.io/openshift-release-dev/ocp-release:4.12.0-ec.4-x86_64 --dir .
~~~

Now, modify the `image-references` and point to the new `baremetal-runtimecfg` image - change the `name` to match the new container location and SHA:
~~~
   458       {
   459         "name": "baremetal-runtimecfg",
   460         "annotations": {
   461           "io.openshift.build.commit.id": "4f1174e010f295ae64e1f6eba2e786542d0863b3",
   462           "io.openshift.build.commit.ref": "",
   463           "io.openshift.build.source-location": "https://github.com/openshift/baremetal-runtimecfg"
   464         },
   465         "from": {
   466           "kind": "DockerImage",
   467           "name": "quay.io/akaris/baremetal-runtimecfg@sha256:9a83210b0536661a826fbddb6be9dcf5cebf16d91e5fe768       7b741166ab69c3c2"
   468         },
   469         "generation": null,
   470         "importPolicy": {},
   471         "referencePolicy": {
   472           "type": ""
   473         }
   474       },
~~~

Now, build a new release from the directory contents - this will take a little while but the image will eventually be pushed to your registr:
~~~
oc adm release new  --from-dir . --to-image quay.io/akaris/ocp-release:test
~~~

Inspect the newly created ocp-release image:
~~~
 podman run --rm -it --entrypoint=/bin/grep quay.io/akaris/ocp-release:test runtimecfg ./release-manifests/image-references
        "name": "baremetal-runtimecfg",
          "io.openshift.build.source-location": "https://github.com/openshift/baremetal-runtimecfg"
          "name": "quay.io/akaris/baremetal-runtimecfg@sha256:9a83210b0536661a826fbddb6be9dcf5cebf16d91e5fe7687b741166ab69c3c2"
~~~

Create a new `openshift-install` command:
~~~
oc adm release extract --tools quay.io/akaris/ocp-release:test
tar -xf openshift-install-linux-0.0.1-2022-10-19-104245.tar.gz
~~~

And install the cluster with the custom release payload:
~~~
./openshift-install create cluster --dir=install-config --log-level=debug
~~~

#### Using buildah

It is also possible to use buildah to achieve the exact same outcome. TBD.

#### Verification

After installation, you can check the `nodeip-configuration service` on one of your nodes. It will contain a reference to the new image:
~~~
$ cat /etc/systemd/system/nodeip-configuration.service
[Unit]
Description=Writes IP address configuration so that kubelet and crio services select a valid node IP
Wants=network-online.target crio-wipe.service
After=network-online.target ignition-firstboot-complete.service crio-wipe.service
Before=kubelet.service crio.service

[Service]
# Need oneshot to delay kubelet
Type=oneshot
# Would prefer to do Restart=on-failure instead of this bash retry loop, but
# the version of systemd we have right now doesn't support it. It should be
# available in systemd v244 and higher.
ExecStart=/bin/bash -c " \
  until \
  /usr/bin/podman run --rm \
  --authfile /var/lib/kubelet/config.json \
  --net=host \
  --security-opt label=disable \
  --volume /etc/systemd/system:/etc/systemd/system \
  quay.io/akaris/baremetal-runtimecfg@sha256:9a83210b0536661a826fbddb6be9dcf5cebf16d91e5fe7687b741166ab69c3c2 \
  node-ip \
  set \
  --retry-on-failure \
  ${NODEIP_HINT:-${KUBELET_NODEIP_HINT:-}}; \
  do \
  sleep 5; \
  done"
ExecStart=/bin/systemctl daemon-reload

EnvironmentFile=-/etc/default/nodeip-configuration

[Install]
RequiredBy=kubelet.service
~~~
