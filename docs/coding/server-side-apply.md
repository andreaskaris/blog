# Server-Side Apply in Kubernetes controllers

This blog post explores Server-Side Apply (SSA) in the context of `controller-runtime` controllers. We'll examine how
kubectl sends server-side apply requests to understand the underlying mechanics, and demonstrate SSA implementation
using a small dummy controller built with the Operator SDK (kubebuilder). Through practical examples and network traces,
we'll see how `controller-runtime` reconciles resources using SSA and how field management works in practice.

## Why Server-Side Apply is of interest for controller developers

An [upstream kubernetes project blog post]( https://kubernetes.io/blog/2022/10/20/advanced-server-side-apply/#reconstructive-controllers)
states that SSA is an ideal tool for constructive controllers:

> This kind of controller wasn't really possible prior to SSA. The idea here is to (whenever something changes etc) reconstruct from scratch the fields of the object as the controller wishes them to be, and then apply the change to the server, letting it figure out the result. I now recommend that new controllers start out this way–it's less fiddly to say what you want an object to look like than it is to say how you want it to change.
>
> The client library supports this method of operation by default.

It also addresses one of the downsides of sending unneeded apply requests, but also states that this is likely not
an issue in real-life scenarios and that it also cannot easily be fixed by sending an extra GET to retrieve the existing
state:

> The only downside is that you may end up sending unneeded apply requests to the API server, even if actually the object already matches your controller’s desires. This doesn't matter if it happens once in a while, but for extremely high-throughput controllers, it might cause a performance problem for the cluster–specifically, the API server. No-op writes are not written to storage (etcd) or broadcast to any watchers, so it’s not really that big of a deal. If you’re worried about this anyway, today you could use the method explained in the previous section, or you could still do it this way for now, and wait for an additional client-side mechanism to suppress zero-change applies.
>
> To get around this downside, why not GET the object and only send your apply if the object needs it? Surprisingly, it doesn't help much – a no-op apply is not very much more work for the API server than an extra GET; and an apply that changes things is cheaper than that same apply with a preceding GET. Worse, since it is a distributed system, something could change between your GET and apply, invalidating your computation

The upstream [kubernetes documentation suggests](https://kubernetes.io/docs/reference/using-api/server-side-apply/#using-server-side-apply-in-a-controller)
using Server-Side Apply in controllers:

> Using Server-Side Apply in a controller
>
> As a developer of a controller, you can use Server-Side Apply as a way to simplify the update logic of your controller. The main differences with a read-modify-write and/or patch are the following:
>
>    the applied object must contain all the fields that the controller cares about.
>    there is no way to remove fields that haven't been applied by the controller before (controller can still send a patch or update for these use-cases).
>    the object doesn't have to be read beforehand; resourceVersion doesn't have to be specified.

In addition, the documentation strongly recommends to force ownership of fields in case of conflicts:

> It is strongly recommended for controllers to always force conflicts on objects that they own and manage, since they might not be able to resolve or act on these conflicts

## What does Server-Side Apply look like?

Similar to a patch, Server-Side Apply allows a user to define only the fields that they want to be changed and to apply
those changes. In this section, we want to change the replica count for existing deployment `reconciler` inside namespace
`reconciler`:

```
$ cat test.yaml 
apiVersion: apps/v1
kind: Deployment
metadata:
  name: reconciler
  namespace: reconciler
spec:
  replicas: 8
```

We can then apply this change with SSA with kubectl by using the `--server-side` flag:

```
kubectl apply -f test.yaml --server-side
```

Here's what server side apply looks like for the API, where `Content-Type: application/apply-patch+yaml` indicates that
[server side apply shall be used](https://kubernetes.io/docs/reference/using-api/server-side-apply/#api-implementation):

```
$ kubectl apply -v10 -f test.yaml --server-side --force-conflicts
(...)
I0902 15:48:02.616662  957821 helper.go:269] "Request Body" body=<
        {"apiVersion":"apps/v1","kind":"Deployment","metadata":{"name":"reconciler","namespace":"reconciler"},"spec":{"replicas":8}}
 >
I0902 15:48:02.616722  957821 round_trippers.go:527] "Request" curlCommand=<
        curl -v -XPATCH  -H "Accept: application/json" -H "Content-Type: application/apply-patch+yaml" -H "User-Agent: kubectl/v1.33.2 (linux/amd64) kubernetes/a57b6f7" 'https://127.0.0.1:38503/apis/apps/v1/namespaces/reconciler/deployments/reconciler?fieldManager=kubectl&fieldValidation=Strict&force=true'
 >
I0902 15:48:02.625206  957821 round_trippers.go:632] "Response" verb="PATCH" url="https://127.0.0.1:38503/apis/apps/v1/namespaces/reconciler/deployments/reconciler?fieldManager=kubectl&fieldValidation=Strict&force=true" status="200 OK" headers=<
        Audit-Id: eb6f9d6e-26bb-4c97-bb00-6f7f5da9db32
        Cache-Control: no-cache, private
        Content-Type: application/json
        Date: Tue, 02 Sep 2025 13:48:02 GMT
        X-Kubernetes-Pf-Flowschema-Uid: 8391073b-d875-43da-9f1a-94ef82459735
        X-Kubernetes-Pf-Prioritylevel-Uid: 3f30e2d2-3e73-402b-98b1-9c0ddce8c521
 > milliseconds=8 getConnectionMilliseconds=0 serverProcessingMilliseconds=8
I0902 15:48:02.625362  957821 helper.go:269] "Response Body" body=<
        {"kind":"Deployment","apiVersion":"apps/v1","metadata":{"name":"reconciler","namespace":"reconciler","uid":"684b66af-77bf-4bc5-96f4-bf2c0018a5a5","resourceVersion":"18761","generation":13,"creationTimestamp":"2025-09-02T10:53:31Z","annotations":{"deployment.kubernetes.io/revision":"1"},"ownerReferences":[{"apiVersion":"test.example.com/v1alpha1","kind":"Reconciler","name":"reconciler-sample","uid":"c8f0aab2-b8bd-438f-ad50-043e65017963","controller":true,"blockOwnerDeletion":true}],"managedFields":[{"manager":"reconciler-controller","operation":"Apply","apiVersion":"apps/v1","time":"2025-09-02T12:04:06Z","fieldsType":"FieldsV1","fieldsV1":{"f:metadata":{"f:ownerReferences":{"k:{\"uid\":\"c8f0aab2-b8bd-438f-ad50-043e65017963\"}":{}}},"f:spec":{"f:selector":{},"f:strategy":{},"f:template":{"f:metadata":{"f:creationTimestamp":{},"f:labels":{"f:app":{}}},"f:spec":{"f:containers":{"k:{\"name\":\"netshoot\"}":{".":{},"f:command":{},"f:image":{},"f:imagePullPolicy":{},"f:name":{},"f:resources":{},"f:securityContext":{"f:allowPrivilegeEscalation":{},"f:capabilities":{"f:drop":{}},"f:runAsNonRoot":{},"f:runAsUser":{}}}},"f:securityContext":{"f:runAsNonRoot":{},"f:seccompProfile":{"f:type":{}}}}}}}},{"manager":"kubectl","operation":"Apply","apiVersion":"apps/v1","time":"2025-09-02T13:48:02Z","fieldsType":"FieldsV1","fieldsV1":{"f:spec":{"f:replicas":{}}}},{"manager":"main","operation":"Update","apiVersion":"apps/v1","time":"2025-09-02T10:53:31Z","fieldsType":"FieldsV1","fieldsV1":{"f:metadata":{"f:ownerReferences":{".":{},"k:{\"uid\":\"c8f0aab2-b8bd-438f-ad50-043e65017963\"}":{}}},"f:spec":{"f:progressDeadlineSeconds":{},"f:revisionHistoryLimit":{},"f:selector":{},"f:strategy":{"f:rollingUpdate":{".":{},"f:maxSurge":{},"f:maxUnavailable":{}},"f:type":{}},"f:template":{"f:metadata":{"f:labels":{".":{},"f:app":{}}},"f:spec":{"f:containers":{"k:{\"name\":\"netshoot\"}":{".":{},"f:command":{},"f:image":{},"f:imagePullPolicy":{},"f:name":{},"f:resources":{},"f:securityContext":{".":{},"f:allowPrivilegeEscalation":{},"f:capabilities":{".":{},"f:drop":{}},"f:runAsNonRoot":{},"f:runAsUser":{}},"f:terminationMessagePath":{},"f:terminationMessagePolicy":{}}},"f:dnsPolicy":{},"f:restartPolicy":{},"f:schedulerName":{},"f:securityContext":{".":{},"f:runAsNonRoot":{},"f:seccompProfile":{".":{},"f:type":{}}},"f:terminationGracePeriodSeconds":{}}}}}},{"manager":"kube-controller-manager","operation":"Update","apiVersion":"apps/v1","time":"2025-09-02T12:07:23Z","fieldsType":"FieldsV1","fieldsV1":{"f:metadata":{"f:annotations":{".":{},"f:deployment.kubernetes.io/revision":{}}},"f:status":{"f:availableReplicas":{},"f:conditions":{".":{},"k:{\"type\":\"Available\"}":{".":{},"f:lastTransitionTime":{},"f:lastUpdateTime":{},"f:message":{},"f:reason":{},"f:status":{},"f:type":{}},"k:{\"type\":\"Progressing\"}":{".":{},"f:lastTransitionTime":{},"f:lastUpdateTime":{},"f:message":{},"f:reason":{},"f:status":{},"f:type":{}}},"f:observedGeneration":{},"f:readyReplicas":{},"f:replicas":{},"f:updatedReplicas":{}}},"subresource":"status"}]},"spec":{"replicas":8,"selector":{"matchLabels":{"app":"reconciler-deployment"}},"template":{"metadata":{"creationTimestamp":null,"labels":{"app":"reconciler-deployment"}},"spec":{"containers":[{"name":"netshoot","image":"nicolaka/netshoot","command":["sleep","infinity"],"resources":{},"terminationMessagePath":"/dev/termination-log","terminationMessagePolicy":"File","imagePullPolicy":"IfNotPresent","securityContext":{"capabilities":{"drop":["ALL"]},"runAsUser":1001,"runAsNonRoot":false,"allowPrivilegeEscalation":false}}],"restartPolicy":"Always","terminationGracePeriodSeconds":30,"dnsPolicy":"ClusterFirst","securityContext":{"runAsNonRoot":true,"seccompProfile":{"type":"RuntimeDefault"}},"schedulerName":"default-scheduler"}},"strategy":{"type":"RollingUpdate","rollingUpdate":{"maxUnavailable":"25%","maxSurge":"25%"}},"revisionHistoryLimit":10,"progressDeadlineSeconds":600},"status":{"observedGeneration":12,"replicas":14,"updatedReplicas":14,"readyReplicas":14,"availableReplicas":14,"conditions":[{"type":"Progressing","status":"True","lastUpdateTime":"2025-09-02T10:53:32Z","lastTransitionTime":"2025-09-02T10:53:31Z","reason":"NewReplicaSetAvailable","message":"ReplicaSet \"reconciler-57ddfb79c9\" has successfully progressed."},{"type":"Available","status":"True","lastUpdateTime":"2025-09-02T12:04:07Z","lastTransitionTime":"2025-09-02T12:04:07Z","reason":"MinimumReplicasAvailable","message":"Deployment has minimum availability."}]}}
 >
deployment.apps/reconciler serverside-applied
I0902 15:48:02.625965  957821 apply.go:548] Running apply post-processor function
```

You can see in the above that the payload sent to the API server is exactly the minimal JSON containing the metadata and
the replica count.

There's an additional caveat - we had to specify the `--force-conflicts` flag because kubectl was
not the field manager. Otherwise, we would get a conflict in cases that this object's `.spec.replicas` field was managed
by a different manager. In the section about `Field management`, we will understand why.

## Current status for Server-Side Apply in controller-runtime

At time of this writing, Server-Side Apply can be used together with `controller-runtime` in the form of a Patch statement:

```
if err := r.Patch(ctx, desiredDeployment, client.Apply, client.ForceOwnership, client.FieldOwner(r.Name)); err != nil {
	return ctrl.Result{}, err
}
```

Where `client.Apply` tells the `Patch` method to use the [ApplyYAMLPatchType](https://github.com/kubernetes/apimachinery/blob/2340d9bf7725073021aecf60ac5a4553ceaf305c/pkg/types/patch.go#L29):

```
ApplyYAMLPatchType      PatchType = "application/apply-patch+yaml"
```

Using a `Patch` with `client.Apply` may work well in most cases for the `controller-runtime`. However, the detailed
documentation for [applyconfigurations](https://pkg.go.dev/k8s.io/client-go/applyconfigurations) states a caveat: the
standard library structs have many non-pointer fields and thus fields are set to their default values. On the other hand:

```
Each "apply configuration" type represents the same Kubernetes object kind as the corresponding go struct, but where all fields are pointers to make them optional, allowing apply requests to be accurately represented.
```

SSA was recently [made a first-class citizen](https://github.com/kubernetes-sigs/controller-runtime/issues/3183) of the
`controller-runtime` project, even though some pieces are still missing, such as [documentation](github.com/kubernetes-sigs/kubebuilder/issues/2514).
Therefore, in the near future, it should be fully documented how to use Server-Side Apply natively with the `controller-runtime`.


## Field management

Whether Server-Side Apply detects a conflict comes down to [Field management](https://kubernetes.io/docs/reference/using-api/server-side-apply/#field-management):

> The Kubernetes API server tracks managed fields for all newly created objects.
>
> When trying to apply an object, fields that have a different value and are owned by another manager will result in a conflict. This is done in order to signal that the operation might undo another collaborator's changes. Writes to objects with managed fields can be forced, in which case the value of any conflicted field will be overridden, and the ownership will be transferred.
>
> Whenever a field's value does change, ownership moves from its current manager to the manager making the change.
```

In order for us to understand how field management works, let's look at [a small test operator](https://github.com/andreaskaris/reconciler-operator/blob/741884f5396b1ca5b0a7793fd4657fa5f2c4048f/internal/controller/reconciler_controller.go#L102).
The operator has the following patch logic:

```
func (r *ReconcilerReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
	logger := logf.FromContext(ctx)

	// TODO(user): your logic here
	logger.Info("reconciler was called")
	reconciler := &v1alpha1.Reconciler{}
	err := r.Get(ctx, req.NamespacedName, reconciler)
	if err != nil {
		if apierrors.IsNotFound(err) {
			logger.Info("reconciler resource not found. Ignoring since object must be deleted")
			return ctrl.Result{}, nil
		}
		// Error reading the object - requeue the request.
		logger.Error(err, "Failed to get reconciler")
		return ctrl.Result{}, err
	}

	desiredDeployment, err := r.getReconcilerDeployment(reconciler)
	if err != nil {
		return ctrl.Result{}, err
	}

	logger.Info("deployment is", "deployment", desiredDeployment)

	currentDeployment := &appsv1.Deployment{}
	namespacedName := types.NamespacedName{Name: desiredDeployment.Name, Namespace: desiredDeployment.Namespace}
	if err = r.Get(ctx, namespacedName, currentDeployment); err != nil {
		if apierrors.IsNotFound(err) {
			logger.Info("creating deployment", "deployment", desiredDeployment)
			if err := r.Create(ctx, desiredDeployment); err != nil {
				return ctrl.Result{}, err
			}
			return ctrl.Result{}, nil
		}
		// Error reading the object - requeue the request.
		logger.Error(err, "Failed to get deployment")
		return ctrl.Result{}, err
	}

	// Patch the deployment here!
	desiredDeployment.APIVersion = "apps/v1"
	desiredDeployment.Kind = "Deployment"
	logger.Info("patching deployment", "deployment", desiredDeployment)
	if err := r.Patch(ctx, desiredDeployment, client.Apply, client.ForceOwnership, client.FieldOwner(r.Name)); err != nil {
		return ctrl.Result{}, err
	}

	return ctrl.Result{}, nil
}
```

In this concrete example, when the deployment is not present and when the operator first starts, the controller creates
the desired deployment because it does not yet exist:

```
2025-09-03T12:19:29+02:00       INFO    creating deployment     {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler", "Reconciler": {"name":"reconciler-sample"}, "namespace": "", "name": "reconciler-sample", "reconcileID": "f136a051-e0c1-45a0-93dc-e7bb48fa552a", "deployment": "&Deployment{ObjectMeta:{reconciler  reconciler    0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[] map[] [{test.example.com/v1alpha1 Reconciler reconciler-sample 6c9e435e-5edf-4a13-868d-d0b85aa09d1a 0xc00015a622 0xc00015a621}] [] []},Spec:DeploymentSpec{Replicas:*1,Selector:&v1.LabelSelector{MatchLabels:map[string]string{app: reconciler-deployment,},MatchExpressions:[]LabelSelectorRequirement{},},Template:{{      0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[app:reconciler-deployment] map[] [] [] []} {[] [] [{netshoot nicolaka/netshoot [sleep infinity] []  [] [] [] {map[] map[] []} [] <nil> [] [] nil nil nil nil   IfNotPresent SecurityContext{Capabilities:&Capabilities{Add:[],Drop:[ALL],},Privileged:nil,SELinuxOptions:nil,RunAsUser:*1001,RunAsNonRoot:*false,ReadOnlyRootFilesystem:nil,AllowPrivilegeEscalation:*false,RunAsGroup:nil,ProcMount:nil,WindowsOptions:nil,SeccompProfile:nil,AppArmorProfile:nil,} false false false}] []  <nil> <nil>  map[]   <nil>  false false false <nil> &PodSecurityContext{SELinuxOptions:nil,RunAsUser:nil,RunAsNonRoot:*true,SupplementalGroups:[],FSGroup:nil,RunAsGroup:nil,Sysctls:[]Sysctl{},WindowsOptions:nil,FSGroupChangePolicy:nil,SeccompProfile:&SeccompProfile{Type:RuntimeDefault,LocalhostProfile:nil,},AppArmorProfile:nil,SupplementalGroupsPolicy:nil,SELinuxChangePolicy:nil,} []   nil  [] []  <nil> nil [] <nil> <nil> <nil> map[] [] <nil> nil <nil> [] [] nil}},Strategy:DeploymentStrategy{Type:,RollingUpdate:nil,},MinReadySeconds:0,RevisionHistoryLimit:nil,Paused:false,ProgressDeadlineSeconds:nil,},Status:DeploymentStatus{ObservedGeneration:0,Replicas:0,UpdatedReplicas:0,AvailableReplicas:0,UnavailableReplicas:0,Conditions:[]DeploymentCondition{},ReadyReplicas:0,CollisionCount:nil,TerminatingReplicas:nil,},}"}
```

We can look at the managed fields, all of which belong to `manager: main` (and `kube-controller-manager` for the `status`)
as we went through the normal `controller-runtime` creation logic:

```
$ kubectl get deployment -n reconciler reconciler --show-managed-fields -o yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  annotations:
    deployment.kubernetes.io/revision: "1"
  creationTimestamp: "2025-09-03T10:19:29Z"
  generation: 1
  managedFields:
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:ownerReferences:
          .: {}
          k:{"uid":"6c9e435e-5edf-4a13-868d-d0b85aa09d1a"}: {}
      f:spec:
        f:progressDeadlineSeconds: {}
        f:replicas: {}
        f:revisionHistoryLimit: {}
        f:selector: {}
        f:strategy:
          f:rollingUpdate:
            .: {}
            f:maxSurge: {}
            f:maxUnavailable: {}
          f:type: {}
        f:template:
          f:metadata:
            f:labels:
              .: {}
              f:app: {}
          f:spec:
            f:containers:
              k:{"name":"netshoot"}:
                .: {}
                f:command: {}
                f:image: {}
                f:imagePullPolicy: {}
                f:name: {}
                f:resources: {}
                f:securityContext:
                  .: {}
                  f:allowPrivilegeEscalation: {}
                  f:capabilities:
                    .: {}
                    f:drop: {}
                  f:runAsNonRoot: {}
                  f:runAsUser: {}
                f:terminationMessagePath: {}
                f:terminationMessagePolicy: {}
            f:dnsPolicy: {}
            f:restartPolicy: {}
            f:schedulerName: {}
            f:securityContext:
              .: {}
              f:runAsNonRoot: {}
              f:seccompProfile:
                .: {}
                f:type: {}
            f:terminationGracePeriodSeconds: {}
    manager: main
    operation: Update
    time: "2025-09-03T10:19:29Z"
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:annotations:
          .: {}
          f:deployment.kubernetes.io/revision: {}
      f:status:
        f:availableReplicas: {}
        f:conditions:
          .: {}
          k:{"type":"Available"}:
            .: {}
            f:lastTransitionTime: {}
            f:lastUpdateTime: {}
            f:message: {}
            f:reason: {}
            f:status: {}
            f:type: {}
          k:{"type":"Progressing"}:
            .: {}
            f:lastTransitionTime: {}
            f:lastUpdateTime: {}
            f:message: {}
            f:reason: {}
            f:status: {}
            f:type: {}
        f:observedGeneration: {}
        f:readyReplicas: {}
        f:replicas: {}
        f:updatedReplicas: {}
    manager: kube-controller-manager
    operation: Update
    subresource: status
    time: "2025-09-03T10:19:30Z"
(...)
```

We now trigger the reconciler loop again by making a change to the `reconciler` object:

```
$ kubectl patch reconciler reconciler-sample --type='merge' -p='{"spec":{"replicas":2}}'
reconciler.test.example.com/reconciler-sample patched
```

Now, the reconciler uses the `Patch` method:

```
2025-09-03T12:24:31+02:00       INFO    patching deployment     {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler", "Reconciler": {"name":"reconciler-sample"}, "namespace": "", "name": "reconciler-sample", "reconcileID": "21ff9b91-f540-455d-9266-ff9b83d3b3b0", "deployment": "&Deployment{ObjectMeta:{reconciler  reconciler    0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[] map[] [{test.example.com/v1alpha1 Reconciler reconciler-sample 6c9e435e-5edf-4a13-868d-d0b85aa09d1a 0xc000680312 0xc000680311}] [] []},Spec:DeploymentSpec{Replicas:*2,Selector:&v1.LabelSelector{MatchLabels:map[string]string{app: reconciler-deployment,},MatchExpressions:[]LabelSelectorRequirement{},},Template:{{      0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[app:reconciler-deployment] map[] [] [] []} {[] [] [{netshoot nicolaka/netshoot [sleep infinity] []  [] [] [] {map[] map[] []} [] <nil> [] [] nil nil nil nil   IfNotPresent SecurityContext{Capabilities:&Capabilities{Add:[],Drop:[ALL],},Privileged:nil,SELinuxOptions:nil,RunAsUser:*1001,RunAsNonRoot:*false,ReadOnlyRootFilesystem:nil,AllowPrivilegeEscalation:*false,RunAsGroup:nil,ProcMount:nil,WindowsOptions:nil,SeccompProfile:nil,AppArmorProfile:nil,} false false false}] []  <nil> <nil>  map[]   <nil>  false false false <nil> &PodSecurityContext{SELinuxOptions:nil,RunAsUser:nil,RunAsNonRoot:*true,SupplementalGroups:[],FSGroup:nil,RunAsGroup:nil,Sysctls:[]Sysctl{},WindowsOptions:nil,FSGroupChangePolicy:nil,SeccompProfile:&SeccompProfile{Type:RuntimeDefault,LocalhostProfile:nil,},AppArmorProfile:nil,SupplementalGroupsPolicy:nil,SELinuxChangePolicy:nil,} []   nil  [] []  <nil> nil [] <nil> <nil> <nil> map[] [] <nil> nil <nil> [] [] nil}},Strategy:DeploymentStrategy{Type:,RollingUpdate:nil,},MinReadySeconds:0,RevisionHistoryLimit:nil,Paused:false,ProgressDeadlineSeconds:nil,},Status:DeploymentStatus{ObservedGeneration:0,Replicas:0,UpdatedReplicas:0,AvailableReplicas:0,UnavailableReplicas:0,Conditions:[]DeploymentCondition{},ReadyReplicas:0,CollisionCount:nil,TerminatingReplicas:nil,},}"}
```

And we can see that the field manager changes. We see an `Apply` operation, and `manager: reconciler-controller` is
now the field manager, among others for the `.spec.replica` field:

```
$ kubectl get deployment -n reconciler reconciler --show-managed-fields -o yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  annotations:
    deployment.kubernetes.io/revision: "1"
  creationTimestamp: "2025-09-03T10:19:29Z"
  generation: 2
  managedFields:
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:ownerReferences:
          k:{"uid":"6c9e435e-5edf-4a13-868d-d0b85aa09d1a"}: {}
      f:spec:
        f:replicas: {}
        f:selector: {}
        f:strategy: {}
        f:template:
          f:metadata:
            f:creationTimestamp: {}
            f:labels:
              f:app: {}
          f:spec:
            f:containers:
              k:{"name":"netshoot"}:
                .: {}
                f:command: {}
                f:image: {}
                f:imagePullPolicy: {}
                f:name: {}
                f:resources: {}
                f:securityContext:
                  f:allowPrivilegeEscalation: {}
                  f:capabilities:
                    f:drop: {}
                  f:runAsNonRoot: {}
                  f:runAsUser: {}
            f:securityContext:
              f:runAsNonRoot: {}
              f:seccompProfile:
                f:type: {}
    manager: reconciler-controller
    operation: Apply
    time: "2025-09-03T10:24:31Z"
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:ownerReferences:
          .: {}
          k:{"uid":"6c9e435e-5edf-4a13-868d-d0b85aa09d1a"}: {}
      f:spec:
        f:progressDeadlineSeconds: {}
        f:revisionHistoryLimit: {}
        f:selector: {}
        f:strategy:
          f:rollingUpdate:
            .: {}
            f:maxSurge: {}
            f:maxUnavailable: {}
          f:type: {}
        f:template:
          f:metadata:
            f:labels:
              .: {}
              f:app: {}
          f:spec:
            f:containers:
              k:{"name":"netshoot"}:
                .: {}
                f:command: {}
                f:image: {}
                f:imagePullPolicy: {}
                f:name: {}
                f:resources: {}
                f:securityContext:
                  .: {}
                  f:allowPrivilegeEscalation: {}
                  f:capabilities:
                    .: {}
                    f:drop: {}
                  f:runAsNonRoot: {}
                  f:runAsUser: {}
                f:terminationMessagePath: {}
                f:terminationMessagePolicy: {}
            f:dnsPolicy: {}
            f:restartPolicy: {}
            f:schedulerName: {}
            f:securityContext:
              .: {}
              f:runAsNonRoot: {}
              f:seccompProfile:
                .: {}
                f:type: {}
            f:terminationGracePeriodSeconds: {}
    manager: main
    operation: Update
    time: "2025-09-03T10:19:29Z"
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:annotations:
          .: {}
          f:deployment.kubernetes.io/revision: {}
      f:status:
        f:availableReplicas: {}
        f:conditions:
          .: {}
          k:{"type":"Available"}:
            .: {}
            f:lastTransitionTime: {}
            f:lastUpdateTime: {}
            f:message: {}
            f:reason: {}
            f:status: {}
            f:type: {}
          k:{"type":"Progressing"}:
            .: {}
            f:lastTransitionTime: {}
            f:lastUpdateTime: {}
            f:message: {}
            f:reason: {}
            f:status: {}
            f:type: {}
        f:observedGeneration: {}
        f:readyReplicas: {}
        f:replicas: {}
        f:updatedReplicas: {}
    manager: kube-controller-manager
    operation: Update
    subresource: status
    time: "2025-09-03T10:24:32Z"
(...)
```

We now apply the following (partial) Deployment inside namespace `reconciler` with name `reconciler`:

```
$ cat test.yaml 
apiVersion: apps/v1
kind: Deployment
metadata:
  name: reconciler
  namespace: reconciler
spec:
  replicas: 8
```

This fails:

```
$ kubectl apply -f test.yaml --server-side
error: Apply failed with 1 conflict: conflict with "reconciler-controller": .spec.replicas
Please review the fields above--they currently have other managers. Here
are the ways you can resolve this warning:
* If you intend to manage all of these fields, please re-run the apply
  command with the `--force-conflicts` flag.
* If you do not intend to manage all of the fields, please edit your
  manifest to remove references to the fields that should keep their
  current managers.
* You may co-own fields by updating your manifest to match the existing
  value; in this case, you'll become the manager if the other manager(s)
  stop managing the field (remove it from their configuration).
See https://kubernetes.io/docs/reference/using-api/server-side-apply/#conflicts
```

The reason is that the field manager for `.spec.replicas` is set to `reconciler-controller`, but the kubectl Server-Side
Apply has manager name `kubectl`.

As already stated above, we can however, force the apply:

```
$ kubectl apply -f test.yaml --server-side --force-conflicts
deployment.apps/reconciler serverside-applied
```

Now, let's fetch the managed fields for the deployment:

```
$ kubectl get deployments  -n reconciler reconciler -o yaml  --show-managed-fields 
apiVersion: apps/v1
kind: Deployment
metadata:
  annotations:
    deployment.kubernetes.io/revision: "1"
  creationTimestamp: "2025-09-03T10:19:29Z"
  generation: 3
  managedFields:
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:ownerReferences:
          k:{"uid":"6c9e435e-5edf-4a13-868d-d0b85aa09d1a"}: {}
      f:spec:
        f:selector: {}
        f:strategy: {}
        f:template:
          f:metadata:
            f:creationTimestamp: {}
            f:labels:
              f:app: {}
          f:spec:
            f:containers:
              k:{"name":"netshoot"}:
                .: {}
                f:command: {}
                f:image: {}
                f:imagePullPolicy: {}
                f:name: {}
                f:resources: {}
                f:securityContext:
                  f:allowPrivilegeEscalation: {}
                  f:capabilities:
                    f:drop: {}
                  f:runAsNonRoot: {}
                  f:runAsUser: {}
            f:securityContext:
              f:runAsNonRoot: {}
              f:seccompProfile:
                f:type: {}
    manager: reconciler-controller
    operation: Apply
    time: "2025-09-03T10:24:31Z"
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:spec:
        f:replicas: {}
    manager: kubectl
    operation: Apply
    time: "2025-09-03T10:33:02Z"
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:ownerReferences:
          .: {}
          k:{"uid":"6c9e435e-5edf-4a13-868d-d0b85aa09d1a"}: {}
      f:spec:
        f:progressDeadlineSeconds: {}
        f:revisionHistoryLimit: {}
        f:selector: {}
        f:strategy:
          f:rollingUpdate:
            .: {}
            f:maxSurge: {}
            f:maxUnavailable: {}
          f:type: {}
        f:template:
          f:metadata:
            f:labels:
              .: {}
              f:app: {}
          f:spec:
            f:containers:
              k:{"name":"netshoot"}:
                .: {}
                f:command: {}
                f:image: {}
                f:imagePullPolicy: {}
                f:name: {}
                f:resources: {}
                f:securityContext:
                  .: {}
                  f:allowPrivilegeEscalation: {}
                  f:capabilities:
                    .: {}
                    f:drop: {}
                  f:runAsNonRoot: {}
                  f:runAsUser: {}
                f:terminationMessagePath: {}
                f:terminationMessagePolicy: {}
            f:dnsPolicy: {}
            f:restartPolicy: {}
            f:schedulerName: {}
            f:securityContext:
              .: {}
              f:runAsNonRoot: {}
              f:seccompProfile:
                .: {}
                f:type: {}
            f:terminationGracePeriodSeconds: {}
(...)
```

In the above output, we can see that `manager: kubectl` last ran an `Apply` operation and that it is the manager of
field `.spec.replicas` now. Whereas `reconciler-controller` is the manager of many of the other fields.

Now, let's trigger the controller's reconciler loop, again:

```
$ kubectl patch reconciler reconciler-sample --type='merge' -p='{"spec":{"replicas":3}}'
reconciler.test.example.com/reconciler-sample patched
```

The reconciler's logic is triggered and the reconciler patches the deployment:

```
2025-09-03T12:35:04+02:00       INFO    patching deployment     {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler", "Reconciler": {"name":"reconciler-sample"}, "namespace": "", "name": "reconciler-sample", "reconcileID": "4493242e-dbd6-4a87-a25c-a00a2d9cd2ae", "deployment": "&Deployment{ObjectMeta:{reconciler  reconciler    0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[] map[] [{test.example.com/v1alpha1 Reconciler reconciler-sample 6c9e435e-5edf-4a13-868d-d0b85aa09d1a 0xc00071c262 0xc00071c261}] [] []},Spec:DeploymentSpec{Replicas:*3,Selector:&v1.LabelSelector{MatchLabels:map[string]string{app: reconciler-deployment,},MatchExpressions:[]LabelSelectorRequirement{},},Template:{{      0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[app:reconciler-deployment] map[] [] [] []} {[] [] [{netshoot nicolaka/netshoot [sleep infinity] []  [] [] [] {map[] map[] []} [] <nil> [] [] nil nil nil nil   IfNotPresent SecurityContext{Capabilities:&Capabilities{Add:[],Drop:[ALL],},Privileged:nil,SELinuxOptions:nil,RunAsUser:*1001,RunAsNonRoot:*false,ReadOnlyRootFilesystem:nil,AllowPrivilegeEscalation:*false,RunAsGroup:nil,ProcMount:nil,WindowsOptions:nil,SeccompProfile:nil,AppArmorProfile:nil,} false false false}] []  <nil> <nil>  map[]   <nil>  false false false <nil> &PodSecurityContext{SELinuxOptions:nil,RunAsUser:nil,RunAsNonRoot:*true,SupplementalGroups:[],FSGroup:nil,RunAsGroup:nil,Sysctls:[]Sysctl{},WindowsOptions:nil,FSGroupChangePolicy:nil,SeccompProfile:&SeccompProfile{Type:RuntimeDefault,LocalhostProfile:nil,},AppArmorProfile:nil,SupplementalGroupsPolicy:nil,SELinuxChangePolicy:nil,} []   nil  [] []  <nil> nil [] <nil> <nil> <nil> map[] [] <nil> nil <nil> [] [] nil}},Strategy:DeploymentStrategy{Type:,RollingUpdate:nil,},MinReadySeconds:0,RevisionHistoryLimit:nil,Paused:false,ProgressDeadlineSeconds:nil,},Status:DeploymentStatus{ObservedGeneration:0,Replicas:0,UpdatedReplicas:0,AvailableReplicas:0,UnavailableReplicas:0,Conditions:[]DeploymentCondition{},ReadyReplicas:0,CollisionCount:nil,TerminatingReplicas:nil,},}"}
```

After the next run of the reconciler, the entry with `manager: kubectl` is completely gone from the `managedFields` and
instead the `manager: reconciler-controller` is again the manager of `.spec.replicas`. This works because the operator
[forces ownership](https://github.com/andreaskaris/reconciler-operator/blob/741884f5396b1ca5b0a7793fd4657fa5f2c4048f/internal/controller/reconciler_controller.go#L102):

```
$ kubectl get deployments  -n reconciler reconciler -o yaml  --show-managed-fields 
apiVersion: apps/v1
kind: Deployment
metadata:
  annotations:
    deployment.kubernetes.io/revision: "1"
  creationTimestamp: "2025-09-03T10:19:29Z"
  generation: 4
  managedFields:
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:ownerReferences:
          k:{"uid":"6c9e435e-5edf-4a13-868d-d0b85aa09d1a"}: {}
      f:spec:
        f:replicas: {}
        f:selector: {}
        f:strategy: {}
        f:template:
          f:metadata:
            f:creationTimestamp: {}
            f:labels:
              f:app: {}
          f:spec:
            f:containers:
              k:{"name":"netshoot"}:
                .: {}
                f:command: {}
                f:image: {}
                f:imagePullPolicy: {}
                f:name: {}
                f:resources: {}
                f:securityContext:
                  f:allowPrivilegeEscalation: {}
                  f:capabilities:
                    f:drop: {}
                  f:runAsNonRoot: {}
                  f:runAsUser: {}
            f:securityContext:
              f:runAsNonRoot: {}
              f:seccompProfile:
                f:type: {}
    manager: reconciler-controller
    operation: Apply
    time: "2025-09-03T10:35:04Z"
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:ownerReferences:
          .: {}
          k:{"uid":"6c9e435e-5edf-4a13-868d-d0b85aa09d1a"}: {}
      f:spec:
        f:progressDeadlineSeconds: {}
        f:revisionHistoryLimit: {}
        f:selector: {}
        f:strategy:
          f:rollingUpdate:
            .: {}
            f:maxSurge: {}
            f:maxUnavailable: {}
          f:type: {}
        f:template:
          f:metadata:
            f:labels:
              .: {}
              f:app: {}
          f:spec:
            f:containers:
              k:{"name":"netshoot"}:
                .: {}
                f:command: {}
                f:image: {}
                f:imagePullPolicy: {}
                f:name: {}
                f:resources: {}
                f:securityContext:
                  .: {}
                  f:allowPrivilegeEscalation: {}
                  f:capabilities:
                    .: {}
                    f:drop: {}
                  f:runAsNonRoot: {}
                  f:runAsUser: {}
                f:terminationMessagePath: {}
                f:terminationMessagePolicy: {}
            f:dnsPolicy: {}
            f:restartPolicy: {}
            f:schedulerName: {}
            f:securityContext:
              .: {}
              f:runAsNonRoot: {}
              f:seccompProfile:
                .: {}
                f:type: {}
            f:terminationGracePeriodSeconds: {}
    manager: main
    operation: Update
    time: "2025-09-03T10:19:29Z"
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:annotations:
          .: {}
          f:deployment.kubernetes.io/revision: {}
      f:status:
        f:availableReplicas: {}
        f:conditions:
          .: {}
          k:{"type":"Available"}:
            .: {}
            f:lastTransitionTime: {}
            f:lastUpdateTime: {}
            f:message: {}
            f:reason: {}
            f:status: {}
            f:type: {}
          k:{"type":"Progressing"}:
            .: {}
            f:lastTransitionTime: {}
            f:lastUpdateTime: {}
            f:message: {}
            f:reason: {}
            f:status: {}
            f:type: {}
        f:observedGeneration: {}
        f:readyReplicas: {}
        f:replicas: {}
        f:updatedReplicas: {}
    manager: kube-controller-manager
    operation: Update
    subresource: status
    time: "2025-09-03T10:35:04Z"
(... the kubectl Apply entry is completely gone, now ...)
```

Next, we make a manual edit to the deployment object:

```
$ kubectl edit deployment -n reconciler reconciler
# --> Change replica count to 4.
```

This does not use Server-Side Apply, and changes the managed fields to:

```
$ kubectl get deployments  -n reconciler reconciler -o yaml  --show-managed-fields 
apiVersion: apps/v1
kind: Deployment
metadata:
  annotations:
    deployment.kubernetes.io/revision: "1"
  creationTimestamp: "2025-09-03T10:19:29Z"
  generation: 5
  managedFields:
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:ownerReferences:
          k:{"uid":"6c9e435e-5edf-4a13-868d-d0b85aa09d1a"}: {}
      f:spec:
        f:selector: {}
        f:strategy: {}
        f:template:
          f:metadata:
            f:creationTimestamp: {}
            f:labels:
              f:app: {}
          f:spec:
            f:containers:
              k:{"name":"netshoot"}:
                .: {}
                f:command: {}
                f:image: {}
                f:imagePullPolicy: {}
                f:name: {}
                f:resources: {}
                f:securityContext:
                  f:allowPrivilegeEscalation: {}
                  f:capabilities:
                    f:drop: {}
                  f:runAsNonRoot: {}
                  f:runAsUser: {}
            f:securityContext:
              f:runAsNonRoot: {}
              f:seccompProfile:
                f:type: {}
    manager: reconciler-controller
    operation: Apply
    time: "2025-09-03T10:35:04Z"
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:ownerReferences:
          .: {}
          k:{"uid":"6c9e435e-5edf-4a13-868d-d0b85aa09d1a"}: {}
      f:spec:
        f:progressDeadlineSeconds: {}
        f:revisionHistoryLimit: {}
        f:selector: {}
        f:strategy:
          f:rollingUpdate:
            .: {}
            f:maxSurge: {}
            f:maxUnavailable: {}
          f:type: {}
        f:template:
          f:metadata:
            f:labels:
              .: {}
              f:app: {}
          f:spec:
            f:containers:
              k:{"name":"netshoot"}:
                .: {}
                f:command: {}
                f:image: {}
                f:imagePullPolicy: {}
                f:name: {}
                f:resources: {}
                f:securityContext:
                  .: {}
                  f:allowPrivilegeEscalation: {}
                  f:capabilities:
                    .: {}
                    f:drop: {}
                  f:runAsNonRoot: {}
                  f:runAsUser: {}
                f:terminationMessagePath: {}
                f:terminationMessagePolicy: {}
            f:dnsPolicy: {}
            f:restartPolicy: {}
            f:schedulerName: {}
            f:securityContext:
              .: {}
              f:runAsNonRoot: {}
              f:seccompProfile:
                .: {}
                f:type: {}
            f:terminationGracePeriodSeconds: {}
    manager: main
    operation: Update
    time: "2025-09-03T10:19:29Z"
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:annotations:
          .: {}
          f:deployment.kubernetes.io/revision: {}
      f:status:
        f:availableReplicas: {}
        f:conditions:
          .: {}
          k:{"type":"Available"}:
            .: {}
            f:lastTransitionTime: {}
            f:lastUpdateTime: {}
            f:message: {}
            f:reason: {}
            f:status: {}
            f:type: {}
          k:{"type":"Progressing"}:
            .: {}
            f:lastTransitionTime: {}
            f:lastUpdateTime: {}
            f:message: {}
            f:reason: {}
            f:status: {}
            f:type: {}
        f:observedGeneration: {}
        f:readyReplicas: {}
        f:replicas: {}
        f:updatedReplicas: {}
    manager: kube-controller-manager
    operation: Update
    subresource: status
    time: "2025-09-03T10:37:13Z"
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:spec:
        f:replicas: {}
    manager: kubectl-edit
    operation: Update
    time: "2025-09-03T10:37:13Z"
(...)
```

From the above, you can see the `manager: kubectl-edit` is now the field manager of `.spec.replicas` via `operation: Update`.

An explanation for why this did not create a conflict can be found in the
[kubernetes upstream documentation](https://kubernetes.io/docs/reference/using-api/server-side-apply/#apply-and-update):

```
Unless you specify a forced override, an apply operation that encounters field-level conflicts always fails; by contrast, if you make a change using update that would affect a managed field, a conflict never provokes failure of the operation.

All Server-Side Apply patch requests are required to identify themselves by providing a fieldManager query parameter, while the query parameter is optional for update operations. Finally, when using the Apply operation you cannot define managedFields in the body of the request that you submit.
```

## Partial reconciliation via Patch statement and Server-Side Apply with the controller-runtime

Here's part of a packet capture which includes a `PATCH` requests from the reconciler operator to the kubernetes API:

```
PATCH /apis/apps/v1/namespaces/reconciler/deployments/reconciler?fieldManager=reconciler-controller&force=true HTTP/1.1
Host: 127.0.0.1:8001
User-Agent: main/v0.0.0 (linux/amd64) kubernetes/$Format
Content-Length: 882
Accept: application/vnd.kubernetes.protobuf, */*
Content-Type: application/apply-patch+yaml
Accept-Encoding: gzip

{"kind":"Deployment","apiVersion":"apps/v1","metadata":{"name":"reconciler","namespace":"reconciler","creationTimestamp":null,"ownerReferences":[{"apiVersion":"test.example.com/v1alpha1","kind":"Reconciler","name":"reconciler-sample","uid":"c8f0aab2-b8bd-438f-ad50-043e65017963","controller":true,"blockOwnerDeletion":true}]},"spec":{"replicas":5,"selector":{"matchLabels":{"app":"reconciler-deployment"}},"template":{"metadata":{"creationTimestamp":null,"labels":{"app":"reconciler-deployment"}},"spec":{"containers":[{"name":"netshoot","image":"nicolaka/netshoot","command":["sleep","infinity"],"resources":{},"imagePullPolicy":"IfNotPresent","securityContext":{"capabilities":{"drop":["ALL"]},"runAsUser":1001,"runAsNonRoot":false,"allowPrivilegeEscalation":false}}],"securityContext":{"runAsNonRoot":true,"seccompProfile":{"type":"RuntimeDefault"}}}},"strategy":{}},"status":{}}
```

The reconciler in the above case sends the [full deployment definition](https://github.com/andreaskaris/reconciler-operator/blob/full-reconcile-ssa/internal/controller/reconciler_controller.go#L102)
to the API server. However, we can modify the code to only send the modified replica count during a reconciliation loop:

```
$ git diff full-reconcile-ssa
diff --git a/internal/controller/reconciler_controller.go b/internal/controller/reconciler_controller.go
index 121c1dc..4a6978e 100644
--- a/internal/controller/reconciler_controller.go
+++ b/internal/controller/reconciler_controller.go
@@ -96,6 +96,10 @@ func (r *ReconcilerReconciler) Reconcile(ctx context.Context, req ctrl.Request)
        }
 
        // Patch the deployment here!
+       desiredDeployment, err = r.getReconcilerDeploymentPatch(reconciler)
+       if err != nil {
+               return ctrl.Result{}, err
+       }
        desiredDeployment.APIVersion = "apps/v1"
        desiredDeployment.Kind = "Deployment"
        logger.Info("patching deployment", "deployment", desiredDeployment)
@@ -114,6 +118,28 @@ func (r *ReconcilerReconciler) SetupWithManager(mgr ctrl.Manager) error {
                Complete(r)
 }
 
+func (r *ReconcilerReconciler) getReconcilerDeploymentPatch(
+       reconciler *v1alpha1.Reconciler) (*appsv1.Deployment, error) {
+       ls := map[string]string{
+               "app": "reconciler-deployment",
+       }
+
+       dep := &appsv1.Deployment{
+               ObjectMeta: metav1.ObjectMeta{
+                       Name:      reconciler.Spec.Name,
+                       Namespace: reconciler.Spec.Namespace,
+               },
+               Spec: appsv1.DeploymentSpec{
+                       Replicas: &reconciler.Spec.Replicas,
+                       Selector: &metav1.LabelSelector{
+                               MatchLabels: ls,
+                       },
+               },
+       }
+       return dep, nil
+}
+
 func (r *ReconcilerReconciler) getReconcilerDeployment(
        reconciler *v1alpha1.Reconciler) (*appsv1.Deployment, error) {
```

With [this change](https://github.com/andreaskaris/reconciler-operator/blob/69b73a2f96a82a82fdbe87cb265e6ac44e63fea5/internal/controller/reconciler_controller.go#L106), 
the PATCH requests then look as follows:

```
PATCH /apis/apps/v1/namespaces/reconciler/deployments/reconciler?fieldManager=reconciler-controller&force=true HTTP/1.1
Host: 127.0.0.1:8001
User-Agent: main/v0.0.0 (linux/amd64) kubernetes/$Format
Content-Length: 312
Accept: application/vnd.kubernetes.protobuf, */*
Content-Type: application/apply-patch+yaml
Accept-Encoding: gzip

{"kind":"Deployment","apiVersion":"apps/v1","metadata":{"name":"reconciler","namespace":"reconciler","creationTimestamp":null},"spec":{"replicas":7,"selector":{"matchLabels":{"app":"reconciler-deployment"}},"template":{"metadata":{"creationTimestamp":null},"spec":{"containers":null}},"strategy":{}},"status":{}}
```

In the above diff, you can see that the DeploymentSpec includes and redefines the `Spec.Selector`. This is required because
`controller-runtime` does not fully implement first-citizen Server-Side Apply via the `Apply` statement, yet.
If we did not redefine `.spec.selector`, and the code looked as follows:

```
$ git diff
diff --git a/internal/controller/reconciler_controller.go b/internal/controller/reconciler_controller.go
index 4a6978e..fc2de54 100644
--- a/internal/controller/reconciler_controller.go
+++ b/internal/controller/reconciler_controller.go
@@ -121,9 +121,9 @@ func (r *ReconcilerReconciler) SetupWithManager(mgr ctrl.Manager) error {
 func (r *ReconcilerReconciler) getReconcilerDeploymentPatch(
diff --git a/internal/controller/reconciler_controller.go b/internal/controller/reconciler_controller.go
index 4a6978e..fc2de54 100644
--- a/internal/controller/reconciler_controller.go
+++ b/internal/controller/reconciler_controller.go
@@ -121,9 +121,9 @@ func (r *ReconcilerReconciler) SetupWithManager(mgr ctrl.Manager) error {
 func (r *ReconcilerReconciler) getReconcilerDeploymentPatch(
        reconciler *v1alpha1.Reconciler) (*appsv1.Deployment, error) {
-       ls := map[string]string{
+       /*ls := map[string]string{
                "app": "reconciler-deployment",
-       }
+       }*/
 
        dep := &appsv1.Deployment{
                ObjectMeta: metav1.ObjectMeta{
@@ -132,9 +132,9 @@ func (r *ReconcilerReconciler) getReconcilerDeploymentPatch(
                },
                Spec: appsv1.DeploymentSpec{
                        Replicas: &reconciler.Spec.Replicas,
-                       Selector: &metav1.LabelSelector{
+                       /*Selector: &metav1.LabelSelector{
                                MatchLabels: ls,
-                       },
+                       },*/
                },
        }
        return dep, nil
```

We would get the following error message in the reconciler:

```
2025-09-03T10:44:31+02:00       INFO    patching deployment     {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler", "Reconciler": {"name":"reconciler-sample"}, "namespace": "", "name": "reconciler-sample", "reconcileID": "52587ce8-308d-4611-aad5-67b4cbe17c2f", "deployment": "&Deployment{ObjectMeta:{reconciler  reconciler    0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[] map[] [] [] []},Spec:DeploymentSpec{Replicas:*9,Selector:nil,Template:{{      0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[] map[] [] [] []} {[] [] [] []  <nil> <nil>  map[]   <nil>  false false false <nil> nil []   nil  [] []  <nil> nil [] <nil> <nil> <nil> map[] [] <nil> nil <nil> [] [] nil}},Strategy:DeploymentStrategy{Type:,RollingUpdate:nil,},MinReadySeconds:0,RevisionHistoryLimit:nil,Paused:false,ProgressDeadlineSeconds:nil,},Status:DeploymentStatus{ObservedGeneration:0,Replicas:0,UpdatedReplicas:0,AvailableReplicas:0,UnavailableReplicas:0,Conditions:[]DeploymentCondition{},ReadyReplicas:0,CollisionCount:nil,TerminatingReplicas:nil,},}"}
2025-09-03T10:44:31+02:00       ERROR   Reconciler error        {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler", "Reconciler": {"name":"reconciler-sample"}, "namespace": "", "name": "reconciler-sample", "reconcileID": "52587ce8-308d-4611-aad5-67b4cbe17c2f", "error": "Deployment.apps \"reconciler\" is invalid: [spec.selector: Required value, spec.template.metadata.labels: Invalid value: map[string]string{\"app\":\"reconciler-deployment\"}: `selector` does not match template `labels`, spec.selector: Invalid value: \"null\": field is immutable]"}
```

And the `PATCH` statement would look like this, including a `null` `.spec.selector` causing the issue. We can also see that
other fields are transmitted with their default values:

```
PATCH /apis/apps/v1/namespaces/reconciler/deployments/reconciler?fieldManager=reconciler-controller&force=true HTTP/1.1
Host: 127.0.0.1:8001
User-Agent: main/v0.0.0 (linux/amd64) kubernetes/$Format
Content-Length: 269
Accept: application/vnd.kubernetes.protobuf, */*
Content-Type: application/apply-patch+yaml
Accept-Encoding: gzip

{"kind":"Deployment","apiVersion":"apps/v1","metadata":{"name":"reconciler","namespace":"reconciler","creationTimestamp":null},"spec":{"replicas":9,"selector":null,"template":{"metadata":{"creationTimestamp":null},"spec":{"containers":null}},"strategy":{}},"status":{}}

HTTP/1.1 422 Unprocessable Entity
(...)
```

For more details, see the documentation for [applyconfigurations](https://pkg.go.dev/k8s.io/client-go/applyconfigurations).

Compare this to the request that `kubectl apply -f debugging/test.yaml --server-side` produces:

```
PATCH /apis/apps/v1/namespaces/reconciler/deployments/reconciler?fieldManager=kubectl&fieldValidation=Strict&force=false HTTP/1.1
Host: 127.0.0.1:8001
User-Agent: kubectl/v1.33.2 (linux/amd64) kubernetes/a57b6f7
Content-Length: 125
Accept: application/json
Content-Type: application/apply-patch+yaml
Kubectl-Command: kubectl apply
Kubectl-Session: 3bdd8423-e68d-4b30-9c1f-258b1e7630bd
Accept-Encoding: gzip

{"apiVersion":"apps/v1","kind":"Deployment","metadata":{"name":"reconciler","namespace":"reconciler"},"spec":{"replicas":6}}
```

It does not include the selector, nor any other default fields, thus this does not lead to a conflict, because
kubectl with Server-Side Apply [encodes the entire JSON scheme](https://github.com/kubernetes/kubernetes/blob/d9df4ecff77012e63164292ab900fd23b2a714e0/staging/src/k8s.io/kubectl/pkg/cmd/apply/apply.go#L579)
as is into a `[]byte`  and sends this as a `PATCH` request.

## Skipping Get/Create Get/Update with Patch and fully specified intent for constructive controllers

It is possible to take Server-Side Apply one step further and to use the `Patch` method
[for both object creation and updates](https://github.com/andreaskaris/reconciler-operator/blob/97ab8c69effacb1c43fd7718a9f47144d5607d33/internal/controller/reconciler_controller.go#L82).
This simplifies the logic of the reconciler of
[constructive controllers]( https://kubernetes.io/blog/2022/10/20/advanced-server-side-apply/#reconstructive-controllers):

```
func (r *ReconcilerReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
	logger := logf.FromContext(ctx)

	// TODO(user): your logic here
	logger.Info("reconciler was called")
	reconciler := &v1alpha1.Reconciler{}
	err := r.Get(ctx, req.NamespacedName, reconciler)
	if err != nil {
		if apierrors.IsNotFound(err) {
			logger.Info("reconciler resource not found. Ignoring since object must be deleted")
			return ctrl.Result{}, nil
		}
		// Error reading the object - requeue the request.
		logger.Error(err, "Failed to get reconciler")
		return ctrl.Result{}, err
	}

	desiredDeployment, err := r.getReconcilerDeployment(reconciler)
	if err != nil {
		return ctrl.Result{}, err
	}
	desiredDeployment.APIVersion = "apps/v1"
	desiredDeployment.Kind = "Deployment"
	logger.Info("patching deployment", "deployment", desiredDeployment)
	if err := r.Patch(ctx, desiredDeployment, client.Apply, client.ForceOwnership, client.FieldOwner(r.Name)); err != nil {
		return ctrl.Result{}, err
	}

	return ctrl.Result{}, nil
}
```

Starting from a clean sheet, with only the kind `reconciler` object with name `reconciler-sample` present, this will only
send a single `PATCH` request to the API server:

```
$ make samples
$ go run ./cmd/main.go
2025-09-03T13:04:21+02:00       INFO    setup   starting manager
2025-09-03T13:04:21+02:00       INFO    starting server {"name": "health probe", "addr": "[::]:8081"}
2025-09-03T13:04:21+02:00       INFO    Starting EventSource    {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler", "source": "kind source: *v1alpha1.Reconciler"}
2025-09-03T13:04:21+02:00       INFO    Starting Controller     {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler"}
2025-09-03T13:04:21+02:00       INFO    Starting workers        {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler", "worker count": 1}
2025-09-03T13:04:21+02:00       INFO    reconciler was called   {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler", "Reconciler": {"name":"reconciler-sample"}, "namespace": "", "name": "reconciler-sample", "reconcileID": "74304b1e-bb08-4e38-83fc-c888a64f8b6e"}
2025-09-03T13:04:21+02:00       INFO    patching deployment     {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler", "Reconciler": {"name":"reconciler-sample"}, "namespace": "", "name": "reconciler-sample", "reconcileID": "74304b1e-bb08-4e38-83fc-c888a64f8b6e", "deployment": "&Deployment{ObjectMeta:{reconciler  reconciler    0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[] map[] [{test.example.com/v1alpha1 Reconciler reconciler-sample 259e2baf-46f9-47b4-b472-d7f17623b278 0xc000881982 0xc000881981}] [] []},Spec:DeploymentSpec{Replicas:*1,Selector:&v1.LabelSelector{MatchLabels:map[string]string{app: reconciler-deployment,},MatchExpressions:[]LabelSelectorRequirement{},},Template:{{      0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[app:reconciler-deployment] map[] [] [] []} {[] [] [{netshoot nicolaka/netshoot [sleep infinity] []  [] [] [] {map[] map[] []} [] <nil> [] [] nil nil nil nil   IfNotPresent SecurityContext{Capabilities:&Capabilities{Add:[],Drop:[ALL],},Privileged:nil,SELinuxOptions:nil,RunAsUser:*1001,RunAsNonRoot:*false,ReadOnlyRootFilesystem:nil,AllowPrivilegeEscalation:*false,RunAsGroup:nil,ProcMount:nil,WindowsOptions:nil,SeccompProfile:nil,AppArmorProfile:nil,} false false false}] []  <nil> <nil>  map[]   <nil>  false false false <nil> &PodSecurityContext{SELinuxOptions:nil,RunAsUser:nil,RunAsNonRoot:*true,SupplementalGroups:[],FSGroup:nil,RunAsGroup:nil,Sysctls:[]Sysctl{},WindowsOptions:nil,FSGroupChangePolicy:nil,SeccompProfile:&SeccompProfile{Type:RuntimeDefault,LocalhostProfile:nil,},AppArmorProfile:nil,SupplementalGroupsPolicy:nil,SELinuxChangePolicy:nil,} []   nil  [] []  <nil> nil [] <nil> <nil> <nil> map[] [] <nil> nil <nil> [] [] nil}},Strategy:DeploymentStrategy{Type:,RollingUpdate:nil,},MinReadySeconds:0,RevisionHistoryLimit:nil,Paused:false,ProgressDeadlineSeconds:nil,},Status:DeploymentStatus{ObservedGeneration:0,Replicas:0,UpdatedReplicas:0,AvailableReplicas:0,UnavailableReplicas:0,Conditions:[]DeploymentCondition{},ReadyReplicas:0,CollisionCount:nil,TerminatingReplicas:nil,},}"}
```

`manager: reconciler-controller` is now the only field manager with the exception of `kube-controller-manager` for the
status field:

```
$ kubectl get deployments  -n reconciler reconciler -o yaml  --show-managed-fields 
apiVersion: apps/v1
kind: Deployment
metadata:
  annotations:
    deployment.kubernetes.io/revision: "1"
  creationTimestamp: "2025-09-03T11:04:21Z"
  generation: 1
  managedFields:
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:ownerReferences:
          k:{"uid":"259e2baf-46f9-47b4-b472-d7f17623b278"}: {}
      f:spec:
        f:replicas: {}
        f:selector: {}
        f:strategy: {}
        f:template:
          f:metadata:
            f:creationTimestamp: {}
            f:labels:
              f:app: {}
          f:spec:
            f:containers:
              k:{"name":"netshoot"}:
                .: {}
                f:command: {}
                f:image: {}
                f:imagePullPolicy: {}
                f:name: {}
                f:resources: {}
                f:securityContext:
                  f:allowPrivilegeEscalation: {}
                  f:capabilities:
                    f:drop: {}
                  f:runAsNonRoot: {}
                  f:runAsUser: {}
            f:securityContext:
              f:runAsNonRoot: {}
              f:seccompProfile:
                f:type: {}
    manager: reconciler-controller
    operation: Apply
    time: "2025-09-03T11:04:21Z"
  - apiVersion: apps/v1
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:annotations:
          .: {}
          f:deployment.kubernetes.io/revision: {}
      f:status:
        f:availableReplicas: {}
        f:conditions:
          .: {}
          k:{"type":"Available"}:
            .: {}
            f:lastTransitionTime: {}
            f:lastUpdateTime: {}
            f:message: {}
            f:reason: {}
            f:status: {}
            f:type: {}
          k:{"type":"Progressing"}:
            .: {}
            f:lastTransitionTime: {}
            f:lastUpdateTime: {}
            f:message: {}
            f:reason: {}
            f:status: {}
            f:type: {}
        f:observedGeneration: {}
        f:readyReplicas: {}
        f:replicas: {}
        f:updatedReplicas: {}
    manager: kube-controller-manager
    operation: Update
    subresource: status
    time: "2025-09-03T11:04:21Z"
(...)
```

And reconciliation works with the exact same logic:
```
$ kubectl patch reconciler reconciler-sample --type='merge' -p='{"spec":{"replicas":3}}'
reconciler.test.example.com/reconciler-sample patched
```

```
2025-09-03T13:07:10+02:00       INFO    reconciler was called   {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler", "Reconciler": {"name":"reconciler-sample"}, "namespace": "", "name": "reconciler-sample", "reconcileID": "b0e3cf40-ad0d-4c71-95a9-a54d3f2db3aa"}
2025-09-03T13:07:10+02:00       INFO    patching deployment     {"controller": "reconciler", "controllerGroup": "test.example.com", "controllerKind": "Reconciler", "Reconciler": {"name":"reconciler-sample"}, "namespace": "", "name": "reconciler-sample", "reconcileID": "b0e3cf40-ad0d-4c71-95a9-a54d3f2db3aa", "deployment": "&Deployment{ObjectMeta:{reconciler  reconciler    0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[] map[] [{test.example.com/v1alpha1 Reconciler reconciler-sample 259e2baf-46f9-47b4-b472-d7f17623b278 0xc000726662 0xc000726661}] [] []},Spec:DeploymentSpec{Replicas:*3,Selector:&v1.LabelSelector{MatchLabels:map[string]string{app: reconciler-deployment,},MatchExpressions:[]LabelSelectorRequirement{},},Template:{{      0 0001-01-01 00:00:00 +0000 UTC <nil> <nil> map[app:reconciler-deployment] map[] [] [] []} {[] [] [{netshoot nicolaka/netshoot [sleep infinity] []  [] [] [] {map[] map[] []} [] <nil> [] [] nil nil nil nil   IfNotPresent SecurityContext{Capabilities:&Capabilities{Add:[],Drop:[ALL],},Privileged:nil,SELinuxOptions:nil,RunAsUser:*1001,RunAsNonRoot:*false,ReadOnlyRootFilesystem:nil,AllowPrivilegeEscalation:*false,RunAsGroup:nil,ProcMount:nil,WindowsOptions:nil,SeccompProfile:nil,AppArmorProfile:nil,} false false false}] []  <nil> <nil>  map[]   <nil>  false false false <nil> &PodSecurityContext{SELinuxOptions:nil,RunAsUser:nil,RunAsNonRoot:*true,SupplementalGroups:[],FSGroup:nil,RunAsGroup:nil,Sysctls:[]Sysctl{},WindowsOptions:nil,FSGroupChangePolicy:nil,SeccompProfile:&SeccompProfile{Type:RuntimeDefault,LocalhostProfile:nil,},AppArmorProfile:nil,SupplementalGroupsPolicy:nil,SELinuxChangePolicy:nil,} []   nil  [] []  <nil> nil [] <nil> <nil> <nil> map[] [] <nil> nil <nil> [] [] nil}},Strategy:DeploymentStrategy{Type:,RollingUpdate:nil,},MinReadySeconds:0,RevisionHistoryLimit:nil,Paused:false,ProgressDeadlineSeconds:nil,},Status:DeploymentStatus{ObservedGeneration:0,Replicas:0,UpdatedReplicas:0,AvailableReplicas:0,UnavailableReplicas:0,Conditions:[]DeploymentCondition{},ReadyReplicas:0,CollisionCount:nil,TerminatingReplicas:nil,},}"}
```

## Conclusion

In this blog post, we explored Server-Side Apply in the context of controller-runtime controllers. We started by examining
how kubectl uses Server-Side Apply and understanding the HTTP requests it generates to the API server. We then looked at
the current state of SSA support in controller-runtime and built a working example using the Operator SDK.

We looked at how field management works in practice by showing conflicts between different field managers and how ownership
transfers when using forced applies. We traced network packets to see the actual API requests generated by both kubectl
and controller-runtime implementations.

Finally, we showed how Server-Side Apply can simplify controller logic by eliminating the need for separate Get/Create
and Get/Update patterns, allowing controllers to use a single Patch operation for both object creation and updates.

Server-Side Apply simplifies controller logic by using a single approach for both creating and updating resources.
Instead of checking if an object exists and then choosing between Create or Update operations, controllers can use the
Patch method with `client.Apply` for all cases.

This approach works particularly well for constructive controllers that want to declare the desired state of resources.
The controller builds the complete object definition and lets the Kubernetes API server handle the differences. Field
management ensures that conflicts are detected and ownership is tracked properly.

For new controllers, using the Patch method with `client.Apply`, `client.ForceOwnership`, and `client.FieldOwner` provides
a clean and simple reconciliation pattern that handles both object creation and updates with the same code path.

In the near future, SSA will be fully implemented, supported and documented for the `controller-runtime` and it will then
be best to use the native features instead of the Patch method.
