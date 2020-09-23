# List all images in a registry #

How to list all container images in a registry:
~~~
]# curl -X GET https://local-registry:4443/v2/_catalog | jq ."repositories" | jq .[] | sed 's/"//g' | while read c ; do echo "curl -X GET https://local-registry:4443/v2/$c/tags/list" ;  curl -X GET https://local-registry:4443/v2/$c/tags/list; done
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100    54  100    54    0     0    580      0 --:--:-- --:--:-- --:--:--   580
curl -X GET https://local-registry:4443/v2/fedora-infrared/tags/list
{"name":"fedora1","tags":["latest"]}
curl -X GET https://local-registry:4443/v2/fedora-rundeck/tags/list
{"name":"fedora2","tags":["latest"]}
~~~
