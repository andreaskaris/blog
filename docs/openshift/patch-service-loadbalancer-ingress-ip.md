## How to patch status.loadBalancer.ingress IPs manually to a service in a baremetal deployment?

For testing purposes, you might want to modify the status.loadBalancer.ingress of a service directly.

Here's how you can do this. First, create a service:
``` bash linenums="1"
cat <<'EOF' > myservice.yaml
apiVersion: v1
kind: Service
metadata:
  name: myservice
spec:
  ports:
  - name: http
    port: 8888
    protocol: TCP
    targetPort: 80
  type: LoadBalancer
  selector:
    app: nginx-app
EOF
oc apply -f myservice.yaml
```

Then, open a proxy session:
``` bash linenums="1"
oc proxy
```

Then, open another CLI session and patch the service's status field:
``` bash linenums="1"
curl -XPATCH  -H "Accept: application/json" -H "Content-Type: application/json-patch+json"  --data '[{"op": "add", "path": "/status/loadBalancer/ingress", "value":[{"ip": "172.18.0.13"}]}]' http://localhost:8001/api/v1/namespaces/default/services/myservice/status
```

Now, verify:
``` bash linenums="1"
[root@ovnkubernetes ~]# oc get svc  myservice -o yaml | tail -n 10
    protocol: TCP
    targetPort: 80
  selector:
    app: nginx-app
  sessionAffinity: None
  type: LoadBalancer
status:
  loadBalancer:
    ingress:
    - ip: 172.18.0.13
[root@ovnkubernetes ~]# oc get svc
NAME             TYPE           CLUSTER-IP      EXTERNAL-IP   PORT(S)          AGE
myservice   LoadBalancer   10.96.105.206   172.18.0.13   8888:32165/TCP   12s
```
