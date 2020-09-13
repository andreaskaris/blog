# Querying the OCP 4.x upgrades info API #

## Function definitions ##

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

## Usage ##

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

## Upgrading to a specific out of graph image ##

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

