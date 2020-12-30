### How to mount a container image

This is an easy task with `buildah`.
~~~
buildah from <image name>:<tag>
~~~

Identify the container name of the container that was generated from the image:
~~~
buildah ps
~~~

Mount the container locally:
~~~
buildah mount <container name>
~~~ 

Full example:
~~~
# buildah from quay.io/akaris/sosreport-operator-bundle
sosreport-operator-bundle-working-container
# buildah ps
CONTAINER ID  BUILDER  IMAGE ID     IMAGE NAME                       CONTAINER NAME
29ec805903bf     *     062a9b2fe5ee quay.io/akaris/sosreport-oper... sosreport-operator-bundle-working-container
# buildah mount sosreport-operator-bundle-working-container
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged
# find /var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged/manifests
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged/manifests/sosreport-operator-controller-manager-metrics-service_v1_service.yaml
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged/manifests/sosreport-operator-metrics-reader_rbac.authorization.k8s.io_v1_clusterrole.yaml
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged/manifests/sosreport-operator.clusterserviceversion.yaml
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged/manifests/support.openshift.io_sosreports.yaml
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged/tests
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged/tests/scorecard
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged/tests/scorecard/config.yaml
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged/metadata
/var/lib/containers/storage/overlay/a2c8b0bb76a14d28e6030ed85dc7b749d4657df6c3752b2029178475cdcc9c1e/merged/metadata/annotations.yaml
~~~
