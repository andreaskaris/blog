# Useful commands for OpenShift

## Querying the OCP 4.x upgrades info API #

### Function definitions

The following bash functions are useful when needing information about possible upgrade paths and OCP versions:

~~~
function ocp-upgrade-paths() {
  version=$1
  for channel in stable fast candidate ; do
      echo "=== $channel-$version ==="
      curl -sH 'Accept: application/json' "https://api.openshift.com/api/upgrades_info/v1/graph?channel=$channel-$version" | jq -r '[.nodes[].version] | sort | unique[]'
  done
}

function ocp-version-info() {
  channel=$1
  version=$2
  major=$(echo $version | awk -F '.' '{print $1 "." $2}')
  minor=$(echo $version | awk -F '.' '{print $NF}')
  echo "Checking for $channel and major $major and minor $minor"
  url="https://api.openshift.com/api/upgrades_info/v1/graph?channel=$channel-$major&x86_64'"
  echo $url
  curl -sH "Accept:application/json" $url | jq ".nodes[] | select(.version == \"$version\")"
}
~~~

### Usage

Upgrade paths:
~~~
$ ocp-upgrade-paths 4.4
=== stable-4.4 ===
4.3.12
4.3.13
4.3.18
4.3.19
4.3.21
4.3.22
4.3.23
4.3.25
4.3.26
4.3.27
4.3.28
4.3.29
4.3.31
4.3.33
4.3.35
4.4.10
4.4.11
4.4.12
4.4.13
4.4.14
4.4.15
4.4.16
4.4.17
4.4.18
4.4.19
4.4.20
4.4.3
4.4.4
4.4.5
4.4.6
4.4.8
4.4.9
=== fast-4.4 ===
4.3.12
4.3.13
4.3.18
4.3.19
4.3.21
4.3.22
4.3.23
4.3.25
4.3.26
4.3.27
4.3.28
4.3.29
4.3.31
4.3.33
4.3.35
4.4.10
4.4.11
4.4.12
4.4.13
4.4.14
4.4.15
4.4.16
4.4.17
4.4.18
4.4.19
4.4.20
4.4.3
4.4.4
4.4.5
4.4.6
4.4.8
4.4.9
=== candidate-4.4 ===
4.3.10
4.3.11
4.3.12
4.3.13
4.3.14
4.3.15
4.3.17
4.3.18
4.3.19
4.3.21
4.3.22
4.3.23
4.3.24
4.3.25
4.3.26
4.3.27
4.3.28
4.3.29
4.3.31
4.3.33
4.3.35
4.3.5
4.3.8
4.3.9
4.4.0
4.4.0-rc.0
4.4.0-rc.1
4.4.0-rc.10
4.4.0-rc.11
4.4.0-rc.12
4.4.0-rc.13
4.4.0-rc.2
4.4.0-rc.4
4.4.0-rc.6
4.4.0-rc.7
4.4.0-rc.8
4.4.0-rc.9
4.4.10
4.4.11
4.4.12
4.4.13
4.4.14
4.4.15
4.4.16
4.4.17
4.4.18
4.4.19
4.4.2
4.4.20
4.4.21
4.4.3
4.4.4
4.4.5
4.4.6
4.4.7
4.4.8
4.4.9
~~~

Version info:
~~~
$ ocp-version-info stable 4.5.7
Checking for stable and major 4.5 and minor 7
https://api.openshift.com/api/upgrades_info/v1/graph?channel=stable-4.5&x86_64'
{
  "version": "4.5.7",
  "payload": "quay.io/openshift-release-dev/ocp-release@sha256:776b7e8158edf64c82f18f5ec4d6ef378ac3de81ba0dc2700b885ceb62e71279",
  "metadata": {
    "description": "",
    "io.openshift.upgrades.graph.previous.remove_regex": "4.4.12",
    "io.openshift.upgrades.graph.release.channels": "candidate-4.5,fast-4.5,stable-4.5,candidate-4.6",
    "io.openshift.upgrades.graph.release.manifestref": "sha256:776b7e8158edf64c82f18f5ec4d6ef378ac3de81ba0dc2700b885ceb62e71279",
    "url": "https://access.redhat.com/errata/RHBA-2020:3436"
  }
}
~~~

### Upgrading to a specific out of graph image

~~~
[akaris@linux ~]$ ocp-upgrade-paths 4.5
=== stable-4.5 ===
4.5.1
4.5.2
4.5.3
4.5.4
=== fast-4.5 ===
4.4.10
4.4.11
4.4.12
4.4.13
4.4.14
4.4.15
4.4.16
4.5.1
4.5.2
4.5.3
4.5.4
=== candidate-4.5 ===
4.4.10
4.4.11
4.4.12
4.4.13
4.4.14
4.4.15
4.4.16
4.4.6
4.4.8
4.4.9
4.5.0
4.5.0-rc.1
4.5.0-rc.2
4.5.0-rc.4
4.5.0-rc.5
4.5.0-rc.6
4.5.0-rc.7
4.5.1
4.5.1-rc.0
4.5.2
4.5.3
4.5.4
4.5.5
~~~

~~~
[akaris@linux ~]$ ocp-version-info stable 4.5.7
Checking for stable and major 4.5 and minor 7
https://api.openshift.com/api/upgrades_info/v1/graph?channel=stable-4.5&x86_64'
{
  "version": "4.5.7",
  "payload": "quay.io/openshift-release-dev/ocp-release@sha256:776b7e8158edf64c82f18f5ec4d6ef378ac3de81ba0dc2700b885ceb62e71279",
  "metadata": {
    "description": "",
    "io.openshift.upgrades.graph.previous.remove_regex": "4.4.12",
    "io.openshift.upgrades.graph.release.channels": "candidate-4.5,fast-4.5,stable-4.5,candidate-4.6",
    "io.openshift.upgrades.graph.release.manifestref": "sha256:776b7e8158edf64c82f18f5ec4d6ef378ac3de81ba0dc2700b885ceb62e71279",
    "url": "https://access.redhat.com/errata/RHBA-2020:3436"
  }
}
[akaris@linux ~]$ ocp-version-info stable 4.5.8
Checking for stable and major 4.5 and minor 8
https://api.openshift.com/api/upgrades_info/v1/graph?channel=stable-4.5&x86_64'
[akaris@linux ~]$ ocp-version-info candidate 4.5.8
Checking for candidate and major 4.5 and minor 8
https://api.openshift.com/api/upgrades_info/v1/graph?channel=candidate-4.5&x86_64'
{
  "version": "4.5.8",
  "payload": "quay.io/openshift-release-dev/ocp-release@sha256:ae61753ad8c8a26ed67fa233eea578194600d6c72622edab2516879cfbf019fd",
  "metadata": {
    "description": "",
    "io.openshift.upgrades.graph.release.channels": "candidate-4.5,candidate-4.6",
    "io.openshift.upgrades.graph.release.manifestref": "sha256:ae61753ad8c8a26ed67fa233eea578194600d6c72622edab2516879cfbf019fd",
    "url": "https://access.redhat.com/errata/RHBA-2020:3510"
  }
}
~~~

How to uprgade to an image that's not on the graph (not supported). Look at `payload` from ocp-version-info and use that image:
~~~
oc adm upgrade --allow-explicit-upgrade --to-image quay.io/openshift-release-dev/ocp-release@sha256:776b7e8158edf64c82f18f5ec4d6ef378ac3de81ba0dc2700b885ceb62e71279
~~~

## Gathering all resources from a namespace with oc adm inspect

Use the following command to gather all resources from a namespace. 

> **Warning:** This will include secrets!!
~~~
namespace=pipelines-tutorial
oc adm inspect -n $namespace $(oc api-resources --verbs=get,list --namespaced=true | tail -n+2 | awk '{print $1}' | tr '\n' ',' | sed 's/,$//')
~~~

Exclude critical resources with:
~~~
namespace=pipelines-tutorial
exclude_list="secrets"
oc adm inspect -n $namespace $(oc api-resources --verbs=get,list --namespaced=true | tail -n+2 | egrep -v "$exclude_list" | awk '{print $1}' | tr '\n' ',' | sed 's/,$//')
~~~

## Listing specific pod columns

### QOS class

~~~
oc get pods --output=custom-columns="NAME:.metadata.name,STATUS:.status.qosClass"
~~~

Example:
~~~
# oc get pods --output=custom-columns="NAME:.metadata.name,STATUS:.status.qosClass"
NAME                                    STATUS
poda                                    Burstable
podb                                    Guaranteed
~~~

### SCC

~~~
oc get pods --output=custom-columns='NAME:.metadata.name,SCC:.metadata.annotations.openshift\.io/scc'
~~~

Example:
~~~
# oc get pods --output=custom-columns='NAME:.metadata.name,SCC:.metadata.annotations.openshift\.io/scc'
NAME                                    SCC
poda                                    privileged
podb                                    privileged
~~~

### OVNKubernetes

Find the active OVN northbound database node:
~~~
for pod in $(oc -n openshift-ovn-kubernetes get pod -l app=ovnkube-master -o name | awk -F '/' '{print $NF}'); do
  status=$(oc -n openshift-ovn-kubernetes logs $pod -c northd | egrep -o 'active|standby' | tail -1)
  if [ "$status" == "active" ]; then
        export POD="$pod"
  fi
done
~~~

Then, you can use this to query the database, e.g.:
~~~
oc -n openshift-ovn-kubernetes exec -it $POD -- ovn-nbctl show
~~~

### Pod Security Admissin

In order to make a namespace priviledge from a pod security admission, use the following function:
~~~
privileged(){
    oc label ns $1 security.openshift.io/scc.podSecurityLabelSync="false" --overwrite=true
    oc label ns $1 pod-security.kubernetes.io/enforce=privileged --overwrite=true
    oc label ns $1 pod-security.kubernetes.io/warn=privileged --overwrite=true
    oc label ns $1 pod-security.kubernetes.io/audit=privileged --overwrite=true
}
privileged <namespace name>
~~~

### OCP release images

Show info about release images:
~~~
oc adm release info --commits quay.io/openshift-release-dev/ocp-release:4.12.0-ec.4-x86_64
~~~

Extract manifests:
~~~
oc adm release extract --to manifests quay.io/openshift-release-dev/ocp-release:4.12.0-ec.4-x86_64
~~~

### OVN Kubernetes - gathering TS data when mg fails

Collect OVN databases:
~~~
d=$(mktemp -d);
pushd $d;
for pp in $(oc get pods -n openshift-ovn-kubernetes -l app=ovnkube-master -o name); do
  p=${pp/pod\//};
  for db in nb sb; do
    oc exec -it -n openshift-ovn-kubernetes -c ${db}db $p -- cat /etc/ovn/ovn${db}_db.db > ovn${db}_db.db.${p} ;
  done;
done;
popd;
tar -czf /tmp/ovndbs.tar.gz $d
echo "Collected:"
tar -tf   /tmp/ovndbs.tar.gz
~~~

Collect pod logs:
~~~
d=$(mktemp -d)
pushd $d
oc get pods -n openshift-ovn-kubernetes | tee oc_get_pods.txt
for pp in $(oc get pods -n openshift-ovn-kubernetes -o name); do
  p=${pp/pod\//};
  oc logs -n openshift-ovn-kubernetes $p --all-containers > $p.txt; done
done
popd
tar -czf /tmp/ovnlogs.tar.gz $d
echo "Collected:"
tar -tf /tmp/ovnlogs.tar.gz
~~~

