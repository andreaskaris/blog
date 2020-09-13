### Using OpenShift cluster loader on OCP 3.11 ###

* https://docs.openshift.com/container-platform/3.11/scaling_performance/using_cluster_loader.html
* https://blog.openshift.com/managing-the-lifecycle-of-openshift-clusters-vetting-openshift-installations/

~~~
yum install python2-boto3 -y
yum install python-flask -y
git clone https://github.com/openshift/svt.git
~~~

Export kubeconfig or put it into `~/.kube/config`

~~~
cd svt/openshift_scalability/
~~~

Generate test file:
~~~
cat<<'EOF'>test.yaml
projects:
  - num: 1
    basename: testproject
    ifexists: delete
    tuning: default
    quota: default
    users:
      - num: 2
        role: admin
        basename: demo
        password: demo
        userpassfile: /etc/origin/openshift-passwd
    pods:
      - total: 10
      - num: 40
        image: openshift/hello-openshift:v1.0.6
        basename: hellopods
        file: default
        storage:
          - type: none
      - num: 60
        image: rhscl/python-34-rhel7:latest
        basename: pyrhelpods
        file: default

quotas:
  - name: default
    file: default

tuningsets:
  - name: default
    pods:
      stepping:
        stepsize: 5
        pause: 10 s
      rate_limit:
        delay: 250 ms
EOF
~~~
> **Note:** In this and in the following example, I'm using `ifexists: delete`
> This means that if a project with the same name exists, it will be deleted.
> One can also use `ifexists: reuse` to reuse the project.
> Alternatively, if the project already exists, the tool will return a failure.

Run test. Note the test **must** be run from `svt/opnstack_scalability`:
~~~
./cluster-loader.py -f test.yaml 
~~~

Verify:
~~~
$ oc get pods -n testproject0
NAME          READY     STATUS    RESTARTS   AGE
hellopods0    1/1       Running   0          2m
hellopods1    1/1       Running   0          2m
hellopods2    1/1       Running   0          2m
hellopods3    1/1       Running   0          2m
pyrhelpods0   1/1       Running   0          2m
pyrhelpods1   1/1       Running   0          1m
pyrhelpods2   1/1       Running   0          1m
pyrhelpods3   1/1       Running   0          1m
pyrhelpods4   1/1       Running   0          1m
pyrhelpods5   1/1       Running   0          1m
~~~

Now, create a more demanding loadtest:
~~~
cat<<'EOF'>test2.yaml
projects:
- num: 1000
  basename: testproject
  ifexists: delete
  tuning: default
  rcs:
    - num: 1
      replicas: 2
      file: default
      basename: testrc
      image: openshift/hello-openshift:v1.0.6
  templates:
    - num: 5
      file: ./content/build-config-template.json
    - num: 20
      file: ./content/ssh-secret-template.json
    - num: 2
      file: ./content/route-template.json
tuningsets:
- name: default
  pods:
    stepping:
      stepsize: 5
      pause: 10 s
    rate_limit:
      delay: 250 ms
quotas:
- name: default
EOF
~~~

Run test. Note the test **must** be run from `svt/opnstack_scalability`:
~~~
./cluster-loader.py -f test.yaml 
~~~

