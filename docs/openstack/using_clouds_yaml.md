# clouds.yaml - how to generate and use it #

## How to generate and use clouds.yaml ##

### Downloading from horizon ###

The easiest way to get the file is opening horizon and going to Project -> Project -> API Access. 
Then, click on "Download OpenStack RC File" and select to download the "clouds.yaml" file. 

![Download clouds.yaml file](/src/clouds_yaml.png)

Then, copy this file into the current Director as `clouds.yaml`. 

This file will need one minor modification, as the password needs to be added to:
~~~
clouds:
  <cloud identifier>:
    auth:
(...)
      password: VrcxVu7RmZAzpzKUaHmcMv22q
~~~

Test it with:
~~~
openstack --os-cloud <cloud identifier> token issue
~~~

### Manually creating the file ###

Use the following script:
~~~
#!/bin/bash

source /home/stack/overcloudrc

PROJECT_ID=$(openstack project list | grep $OS_PROJECT_NAME | awk '{print $2}')

cat << EOF > clouds.yaml
clouds:
  openstack:
    auth:
      auth_url: $OS_AUTH_URL
      username: "$OS_USERNAME"
      password: "$OS_PASSWORD"
      project_name: "$OS_PROJECT_NAME"
      project_id: "$PROJECT_ID"
      user_domain_name: "$OS_USER_DOMAIN_NAME"
    region_name: "$OS_REGION_NAME"
    interface: "public"
    identity_api_version: $OS_IDENTITY_API_VERSION
EOF
~~~

#### Further details ####

Look at `overcloudrc`:
~~~
[stack@undercloud-0 ~]$ cat overcloudrc 
# Clear any old environment that may conflict.
for key in $( set | awk '{FS="="}  /^OS_/ {print $1}' ); do unset $key ; done
export OS_NO_CACHE=True
export COMPUTE_API_VERSION=1.1
export OS_USERNAME=admin
export no_proxy=,172.16.0.199,192.168.24.12
export OS_USER_DOMAIN_NAME=Default
export OS_VOLUME_API_VERSION=3
export OS_CLOUDNAME=overcloud
export OS_AUTH_URL=http://172.16.0.199:5000//v3
export NOVA_VERSION=1.1
export OS_IMAGE_API_VERSION=2
export OS_PASSWORD=VrcxVu7RmZAzpzKUaHmcMv22q
export OS_PROJECT_DOMAIN_NAME=Default
export OS_IDENTITY_API_VERSION=3
export OS_PROJECT_NAME=admin
export OS_AUTH_TYPE=password
export PYTHONWARNINGS="ignore:Certificate has no, ignore:A true SSLContext object is not available"

# Add OS_CLOUDNAME to PS1
if [ -z "${CLOUDPROMPT_ENABLED:-}" ]; then
    export PS1=${PS1:-""}
    export PS1=\${OS_CLOUDNAME:+"(\$OS_CLOUDNAME)"}\ $PS1
    export CLOUDPROMPT_ENABLED=1
fi
~~~

Get the project id with:
~~~
openstack project list | grep $OS_PROJECT_NAME | awk '{print $2}'
~~~

Generate clouds.yaml with the above credentials. Adjust this to use the actual configuration and credentials as obtained from overcloudrc:
~~~
cat <<'EOF' > clouds.yaml
clouds:
  openstack:
    auth:
      auth_url: http://172.16.0.199:5000//v3
      username: "admin"
      password: VrcxVu7RmZAzpzKUaHmcMv22q
      project_name: "admin"
      project_id: "a416f556938f454f849da42faa317cd3"
      user_domain_name: "Default"
    region_name: "regionOne"
    interface: "public"
    identity_api_version: 3
EOF
~~~

## Using clouds.yaml ##

With the `openstack` CLI, use clouds.yaml by providing `--os-cloud <cloud identifier from YAML file>`:
~~~
[stack@undercloud-0 ~]$ openstack --os-cloud openstack token issue
+------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Field      | Value                                                                                                                                                                                   |
+------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| expires    | 2019-10-26T11:18:10+0000                                                                                                                                                                |
| id         | gAAAAABdtB1i8wnsh-pkN6fGMFD5vX7bcvxWms__01c9FFeDp3U-iG6NB31NJc9QhVxVB7WB9_D5J9gwGX91TIdMEiqmhTuI66Wz8eGkw-jxoiAR81y1UYskPrORlAj4Vl0u2L7bifalN7VnPoWu6ISgDCIhd1vF6BZwtU1NrLpfT0KBqMX83_Q |
| project_id | a40944973ca8466cb30faeb669646359                                                                                                                                                        |
| user_id    | f41f1e1433744957985eef31d5d64309                                                                                                                                                        |
+------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
[stack@undercloud-0 ~]$ openstack --os-cloud openstack network list
+--------------------------------------+----------+---------+
| ID                                   | Name     | Subnets |
+--------------------------------------+----------+---------+
| a01c429e-9095-4838-8ff9-c14ed0683025 | private1 |         |
+--------------------------------------+----------+---------+
~~~
