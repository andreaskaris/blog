# Kind with private registry

## Setting up a private registry server 

Follow [https://andreaskaris.github.io/blog/openshift/private-registry/](https://andreaskaris.github.io/blog/openshift/private-registry/)

Create a registry without authentication and make sure that the certificates are signed for DNS `kind` and that the registry can be queried via `curl https://kind:5000/v2/_catalog`.

## Setting up kind

### Skipping certificate validation

Deploy kind with `containerConfigPatches` to skip registry verification:
~~~
cat <<'EOF' > kindconfig.yaml
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
# 3 control plane node and 1 workers
nodes:
- role: control-plane
# - role: worker
containerdConfigPatches:
- |-
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors."kind:5000"]
    endpoint = ["https://kind:5000"]
  [plugins."io.containerd.grpc.v1.cri".registry.configs."kind:5000".tls]
    insecure_skip_verify = true
EOF
~~~

Then, create the cluster with:
~~~
kind create cluster -v1 --config kindconfig.yaml
~~~
