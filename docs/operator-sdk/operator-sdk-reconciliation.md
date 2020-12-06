## Reconciliation with the Operator SDK

In the Operator SDK, controllers implement the [Reconciler](https://github.com/kubernetes-sigs/controller-runtime/blob/43331a6c8f823b497660b368deb4311ae2030206/pkg/reconcile/reconcile.go#L89) interface. 

Meaning that every controller will have a `Reconcile` method.
~~~
Typically, reconcile is triggered by a Controller in response to cluster Events (e.g. Creating, Updating,
Deleting Kubernetes objects) or external Events (GitHub Webhooks, polling external sources, etc).`
~~~
> [https://github.com/kubernetes-sigs/controller-runtime/blob/43331a6c8f823b497660b368deb4311ae2030206/pkg/reconcile/reconcile.go#L59](https://github.com/kubernetes-sigs/controller-runtime/blob/43331a6c8f823b497660b368deb4311ae2030206/pkg/reconcile/reconcile.go#L59)

Simple example:
~~~
[root@kind ~]# cd $GOPATH/src 
[root@kind src]# mkdir example-operator
[root@kind src]# cd example-operator/
[root@kind example-operator]# operator-sdk init --domain=example.com --repo=github.com/andreaskaris/example-operator
Writing scaffold for you to edit...
Get controller runtime:
$ go get sigs.k8s.io/controller-runtime@v0.6.3
Update go.mod:
$ go mod tidy
Running make:
$ make
/root/go/bin/controller-gen object:headerFile="hack/boilerplate.go.txt" paths="./..."
go fmt ./...
go vet ./...
go build -o bin/manager main.go
Next: define a resource with:
$ operator-sdk create api
[root@kind example-operator]# operator-sdk create api --group example --version v1alpha1 --kind Example --resource=true --controller=true
Writing scaffold for you to edit...
api/v1alpha1/example_types.go
controllers/example_controller.go
Running make:
$ make
/root/go/bin/controller-gen object:headerFile="hack/boilerplate.go.txt" paths="./..."
go fmt ./...
go vet ./...
go build -o bin/manager main.go
~~~

The Example controller will be rendered with:
~~~
[root@kind example-operator]# cat controllers/example_controller.go 
/*
Copyright 2020.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package controllers

import (
	"context"

	"github.com/go-logr/logr"
	"k8s.io/apimachinery/pkg/runtime"
	ctrl "sigs.k8s.io/controller-runtime"
	"sigs.k8s.io/controller-runtime/pkg/client"

	examplev1alpha1 "github.com/andreaskaris/example-operator/api/v1alpha1"
)

// ExampleReconciler reconciles a Example object
type ExampleReconciler struct {
	client.Client
	Log    logr.Logger
	Scheme *runtime.Scheme
}

// +kubebuilder:rbac:groups=example.example.com,resources=examples,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=example.example.com,resources=examples/status,verbs=get;update;patch

func (r *ExampleReconciler) Reconcile(req ctrl.Request) (ctrl.Result, error) {
	_ = context.Background()
	_ = r.Log.WithValues("example", req.NamespacedName)

	// your logic here

	return ctrl.Result{}, nil
}

func (r *ExampleReconciler) SetupWithManager(mgr ctrl.Manager) error {
	return ctrl.NewControllerManagedBy(mgr).
		For(&examplev1alpha1.Example{}).
		Complete(r)
}
~~~

Reconciliation logic goes into `Reconcile`. The `Reconcile` method is handed both a `context.Context` and a `reconcile.Request`:
~~~
// Request contains the information necessary to reconcile a Kubernetes object.  This includes the
// information to uniquely identify the object - its Name and Namespace.  It does NOT contain information about
// any specific Event or the object contents itself.
type Request struct {
	// NamespacedName is the name and namespace of the object to reconcile.
	types.NamespacedName
}
~~~
> [https://github.com/kubernetes-sigs/controller-runtime/blob/43331a6c8f823b497660b368deb4311ae2030206/pkg/reconcile/reconcile.go#L44](https://github.com/kubernetes-sigs/controller-runtime/blob/43331a6c8f823b497660b368deb4311ae2030206/pkg/reconcile/reconcile.go#L44)

### Excursion into some background

You can safely skip this section and continue with the next one ;-)

#### About the client interface

When retrieving objects from the API, we refer to the `context`:
~~~
        // retrieve sosreport CRD
        example := &examplev1alpha1.Example{}
        if err := r.Get(ctx, req.NamespacedName, example); err != nil {
                log.Error(err, "Failed to get Example custom resource")
                return ctrl.Result{}, err
        }
~~~

~~~
Package context defines the Context type, which carries deadlines, cancellation signals, and other request-scoped values across API boundaries and between processes. 
~~~
> [https://golang.org/pkg/context](https://golang.org/pkg/context)

The controller's reconciler struct inherits its methods from the Client interface:
~~~
// ExampleReconciler reconciles a Example object
type ExampleReconciler struct {
        client.Client
        Log    logr.Logger
        Scheme *runtime.Scheme
}
~~~

~~~
type Client interface {
    Reader
    Writer
    StatusClient

    // Scheme returns the scheme this client is using.
    Scheme() *runtime.Scheme
    // RESTMapper returns the rest this client is using.
    RESTMapper() meta.RESTMapper
}
~~~
> [https://godoc.org/sigs.k8s.io/controller-runtime/pkg/client#Client](https://godoc.org/sigs.k8s.io/controller-runtime/pkg/client#Client)

For example, the `Get` and `List` methods are inherited from `Reader`:
~~~
type Reader interface {
    // Get retrieves an obj for the given object key from the Kubernetes Cluster.
    // obj must be a struct pointer so that obj can be updated with the response
    // returned by the Server.
    Get(ctx context.Context, key ObjectKey, obj Object) error

    // List retrieves list of objects for a given namespace and list options. On a
    // successful call, Items field in the list will be populated with the
    // result returned from the server.
    List(ctx context.Context, list ObjectList, opts ...ListOption) error
}
~~~
> [https://godoc.org/sigs.k8s.io/controller-runtime/pkg/client#Reader](https://godoc.org/sigs.k8s.io/controller-runtime/pkg/client#Reader)

The `Get` method for the client is implemented here:
~~~
func (c *client) Get(ctx context.Context, key ObjectKey, obj Object) error {
	switch obj.(type) {
	case *unstructured.Unstructured:
		return c.unstructuredClient.Get(ctx, key, obj)
	case *metav1.PartialObjectMetadata:
		return c.metadataClient.Get(ctx, key, obj)
	default:
		return c.typedClient.Get(ctx, key, obj)
	}
}
~~~
> [https://github.com/kubernetes-sigs/controller-runtime/blob/43331a6c8f823b497660b368deb4311ae2030206/pkg/client/client.go#L204](https://github.com/kubernetes-sigs/controller-runtime/blob/43331a6c8f823b497660b368deb4311ae2030206/pkg/client/client.go#L204)

`typeClient` then reads as follows:
~~~
// Get implements client.Client
func (c *typedClient) Get(ctx context.Context, key ObjectKey, obj Object) error {
	r, err := c.cache.getResource(obj)
	if err != nil {
		return err
	}
	return r.Get().
		NamespaceIfScoped(key.Namespace, r.isNamespaced()).
		Resource(r.resource()).
		Name(key.Name).Do(ctx).Into(obj)
}
~~~
> [https://github.com/kubernetes-sigs/controller-runtime/blob/4462fd4ebb88171af826622c1d6b73383aaf2cdf/pkg/client/typed_client.go#L135](https://github.com/kubernetes-sigs/controller-runtime/blob/4462fd4ebb88171af826622c1d6b73383aaf2cdf/pkg/client/typed_client.go#L135)

We could go further down the rabbit hole, but:
~~~
// getResource returns the resource meta information for the given type of object.
// If the object is a list, the resource represents the item's type instead.
func (c *clientCache) getResource(obj runtime.Object) (*resourceMeta, error) {
~~~
> [https://github.com/kubernetes-sigs/controller-runtime/blob/e2261d96d733bbd8b26e1c61b138c12a2408026c/pkg/client/client_cache.go#L73](https://github.com/kubernetes-sigs/controller-runtime/blob/e2261d96d733bbd8b26e1c61b138c12a2408026c/pkg/client/client_cache.go#L73)

`resourceMeta` implements rest.Interface from `	"k8s.io/client-go/rest"`:
~~~
// resourceMeta caches state for a Kubernetes type.
type resourceMeta struct {
	// client is the rest client used to talk to the apiserver
	rest.Interface
	// gvk is the GroupVersionKind of the resourceMeta
	gvk schema.GroupVersionKind
	// mapping is the rest mapping
	mapping *meta.RESTMapping
}
~~~
> [https://github.com/kubernetes-sigs/controller-runtime/blob/e2261d96d733bbd8b26e1c61b138c12a2408026c/pkg/client/client_cache.go#L123](https://github.com/kubernetes-sigs/controller-runtime/blob/e2261d96d733bbd8b26e1c61b138c12a2408026c/pkg/client/client_cache.go#L123)

So, `r.Get()` will return a `Request` of type `HTTP` `GET`: [https://godoc.org/k8s.io/client-go/rest#Interface](https://godoc.org/k8s.io/client-go/rest#Interface)

`r.Get().NamespaceIfScoped(key.Namespace, r.isNamespaced())`  will set the namespace if the object's `kind` is namespaced (GET <resource>/[ns/<namespace>/]): [https://godoc.org/k8s.io/client-go/rest#Request.NamespaceIfScoped](https://godoc.org/k8s.io/client-go/rest#Request.NamespaceIfScoped)

`r.Get().NamespaceIfScoped(key.Namespace, r.isNamespaced()).Resource(r.resource())` sets the resource to access (GET <resource>/[ns/<namespace>/]<name>): [https://godoc.org/k8s.io/client-go/rest#Request.Resource](https://godoc.org/k8s.io/client-go/rest#Request.Resource)

`r.Get().NamespaceIfScoped(key.Namespace, r.isNamespaced()).Resource(r.resource()).Name(key.Name)` sets the name of a resource to access (GET <resource>/[ns/<namespace>/]<name>): [https://godoc.org/k8s.io/client-go/rest#Request.Name](https://godoc.org/k8s.io/client-go/rest#Request.Name)

`r.Get().NamespaceIfScoped(key.Namespace, r.isNamespaced()).Resource(r.resource()).Name(key.Name).Do(ctx)` formats and executes the request. Returns a Result object for easy response processing: [https://godoc.org/k8s.io/client-go/rest#Request.Do](https://godoc.org/k8s.io/client-go/rest#Request.Do)

`r.Get().NamespaceIfScoped(key.Namespace, r.isNamespaced()).Resource(r.resource()).Name(key.Name).Do(ctx).Into(obj)` converts the `Result` object by writing the result into the object. Once this is successfully done, `obj` will now contain what was retrieved by running the query.

#### context, context.TODO(), context.Background()

I have to do some further research, but:

* [https://essential-go.programming-books.io/context-todo-vs-context-background-d5224e27ff724a33a79cb4e03a5eb333](https://essential-go.programming-books.io/context-todo-vs-context-background-d5224e27ff724a33a79cb4e03a5eb333)
* [https://developer.mongodb.com/community/forums/t/go-why-context-todo-and-not-context-background/5009/2](https://developer.mongodb.com/community/forums/t/go-why-context-todo-and-not-context-background/5009/2)
* [https://ddcode.net/2019/05/11/is-golangs-context-background-and-context-todo-the-same-behavior/](https://ddcode.net/2019/05/11/is-golangs-context-background-and-context-todo-the-same-behavior/)

### Working with `Reconcile`

#### What triggers `Reconcile` and what information is passed to the method?

Let's modify the `Reconcile` method to log whenever it is triggered and to capture the contents of the req object.
~~~
// +kubebuilder:rbac:groups=example.example.com,resources=examples,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=example.example.com,resources=examples/status,verbs=get;update;patch

func (r *ExampleReconciler) Reconcile(req ctrl.Request) (ctrl.Result, error) {
        _ = context.Background()
        log := r.Log.WithValues("example", req) // these variables will be shown whenever log prints
        log.Info("Reconciler triggered with req:")

        // your logic here

        return ctrl.Result{}, nil
}
~~~

Compile, install:
~~~
[root@kind example-operator]# make generate
/root/go/bin/controller-gen object:headerFile="hack/boilerplate.go.txt" paths="./..."
[root@kind example-operator]# make manifests
/root/go/bin/controller-gen "crd:trivialVersions=true" rbac:roleName=manager-role webhook paths="./..." output:crd:artifacts:config=config/crd/bases
[root@kind example-operator]# make manifests
/root/go/bin/controller-gen "crd:trivialVersions=true" rbac:roleName=manager-role webhook paths="./..." output:crd:artifacts:config=config/crd/bases
[root@kind example-operator]# make install
/root/go/bin/controller-gen "crd:trivialVersions=true" rbac:roleName=manager-role webhook paths="./..." output:crd:artifacts:config=config/crd/bases
/root/go/bin/kustomize build config/crd | kubectl apply -f -
Warning: apiextensions.k8s.io/v1beta1 CustomResourceDefinition is deprecated in v1.16+, unavailable in v1.22+; use apiextensions.k8s.io/v1 CustomResourceDefinition
customresourcedefinition.apiextensions.k8s.io/examples.example.example.com created
~~~

And run locally:
~~~
[root@kind example-operator]# make run ENABLE_WEBHOOKS=false
/root/go/bin/controller-gen object:headerFile="hack/boilerplate.go.txt" paths="./..."
go fmt ./...
go vet ./...
/root/go/bin/controller-gen "crd:trivialVersions=true" rbac:roleName=manager-role webhook paths="./..." output:crd:artifacts:config=config/crd/bases
go run ./main.go
2020-11-30T14:52:01.520-0500	INFO	controller-runtime.metrics	metrics server is starting to listen	{"addr": ":8080"}
2020-11-30T14:52:01.522-0500	INFO	setup	starting manager
2020-11-30T14:52:01.524-0500	INFO	controller-runtime.manager	starting metrics server	{"path": "/metrics"}
2020-11-30T14:52:01.524-0500	INFO	controller	Starting EventSource	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "source": "kind source: /, Kind="}
2020-11-30T14:52:01.625-0500	INFO	controller	Starting Controller	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example"}
2020-11-30T14:52:01.625-0500	INFO	controller	Starting workers	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "worker count": 1}
~~~

So far, the reconciler has not been triggered. So how can we trigger it? In comes the `SetupWithManager` method:
~~~
func (r *ExampleReconciler) SetupWithManager(mgr ctrl.Manager) error {
        return ctrl.NewControllerManagedBy(mgr).
                For(&examplev1alpha1.Example{}).
                Complete(r)
}
~~~

We'll go into more details a bit later, but this triggers execution of `Reconcile` of this controller whenever a Create, Update, Delete happens to an `Example` Custom Resource:
~~~
[root@kind example-operator]# cat config/samples/example_v1alpha1_example_namespace_test1.yaml
apiVersion: example.example.com/v1alpha1
kind: Example
metadata:
  name: example-sample
  namespace: test1
spec:
  # Add fields here
  foo: bar
[root@kind example-operator]# cat config/samples/example_v1alpha1_example_namespace_test2.yaml
apiVersion: example.example.com/v1alpha1
kind: Example
metadata:
  name: example-sample2
  namespace: test2
spec:
  # Add fields here
  foo: bar
~~~

~~~
[root@kind example-operator]# oc apply -f config/samples/example_v1alpha1_example_namespace_test1.yaml
example.example.example.com/example-sample created
~~~

~~~
2020-11-30T15:07:29.201-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-11-30T15:07:29.201-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
~~~

~~~
[root@kind example-operator]# oc apply -f config/samples/example_v1alpha1_example_namespace_test2.yaml
example.example.example.com/example-sample created
~~~

~~~
2020-11-30T15:08:36.354-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test2/example-sample2"}
2020-11-30T15:08:36.354-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample2", "namespace": "test2"}
~~~

Whenever something happens to an object of type `&examplev1alpha1.Example{}`, this controller's `Reconcile` will be triggered. The only information that it will be passed is a `controllerruntime.Request` which is an alias to `reconcile.Request` and inherits from `types.NamespacedName`: [https://godoc.org/sigs.k8s.io/controller-runtime/pkg/reconcile#Request](https://godoc.org/sigs.k8s.io/controller-runtime/pkg/reconcile#Request) and [https://github.com/kubernetes-sigs/controller-runtime/blob/43331a6c8f823b497660b368deb4311ae2030206/alias.go#L39](https://github.com/kubernetes-sigs/controller-runtime/blob/43331a6c8f823b497660b368deb4311ae2030206/alias.go#L39) and [https://github.com/kubernetes/apimachinery/blob/master/pkg/types/namespacedname.go#L37](https://github.com/kubernetes/apimachinery/blob/master/pkg/types/namespacedname.go#L37)

#### Triggering the reconciler

When the reconciler is triggered is decided by the configuration isinside the `SetupWithManager` method:
~~~
import (
(...)
        (...)ctrl "sigs.k8s.io/controller-runtime"
(...)
)
(...)
func (r *ExampleReconciler) SetupWithManager(mgr ctrl.Manager) error {
        return ctrl.NewControllerManagedBy(mgr).
                For(&examplev1alpha1.Example{}).
                Complete(r)
}
(...)
~~~

`NewControllerManagedBy` is an alias to `builder.ControllerManagedBy`: [https://godoc.org/sigs.k8s.io/controller-runtime](https://godoc.org/sigs.k8s.io/controller-runtime) and [https://godoc.org/sigs.k8s.io/controller-runtime/pkg/builder#ControllerManagedBy](https://godoc.org/sigs.k8s.io/controller-runtime/pkg/builder#ControllerManagedBy)

##### For 

The `Builder`'s `For` method tells the controller to watch all objects of this specifi custom resourse. It will then pass a `NamespacedName` via a `Request` to the reconciler. The `For` method is documented in: [https://godoc.org/sigs.k8s.io/controller-runtime/pkg/builder#Builder.For](https://godoc.org/sigs.k8s.io/controller-runtime/pkg/builder#Builder.For)

~~~
func (*Builder) For

func (blder *Builder) For(object client.Object, opts ...ForOption) *Builder

For defines the type of Object being *reconciled*, and configures the ControllerManagedBy to respond to create / delete / update events by *reconciling the object*. This is the equivalent of calling Watches(&source.Kind{Type: apiType}, &handler.EnqueueRequestForObject{}) 
~~~

This means that one could rewrite the `SetupWithManager` method as:
~~~
func (r *ExampleReconciler) SetupWithManager(mgr ctrl.Manager) error {
        return ctrl.NewControllerManagedBy(mgr).
                Watches(&source.Kind{Type: &examplev1alpha1.Example{}}, &handler.EnqueueRequestForObject{}).
                Complete(r)
}
~~~

This doesn't quite work. something is missing here [TBD]

##### Owns 

If we want to watch objects of other types that were created by an instance of our CRD, then we use `Owns()`.

Add the following to the imports list:
~~~
        appsv1 "k8s.io/api/apps/v1"
        batchv1 "k8s.io/api/batch/v1"
~~~

Then, change the `SetupWithManager` method to:
~~~
func (r *ExampleReconciler) SetupWithManager(mgr ctrl.Manager) error {
        return ctrl.NewControllerManagedBy(mgr).
                For(&examplev1alpha1.Example{}).
                Owns(&appsv1.Deployment{}).
                Owns(&batchv1.Job{}).
                Complete(r)
}
~~~

Now, spawn a new `Example` object:
~~~
[root@kind example-operator]# cat config/samples/example_v1alpha1_example_namespace_test1.yaml
apiVersion: example.example.com/v1alpha1
kind: Example
metadata:
  name: example-sample
  namespace: test1
spec:
  # Add fields here
  foo: bar
oc apply -f config/samples/example_v1alpha1_example_namespace_test1.yaml
~~~

As expected, the reconcile loop get triggered for this object.

~~~
uid=$(oc get example -n test1 -o json example-sample | jq -r '.metadata.uid')
apiversion=$(oc get example -n test1 -o json example-sample | jq -r '.apiVersion')
kind=$(oc get example -n test1 -o json example-sample | jq -r '.kind')
namespace=$(oc get example -n test1 -o json example-sample | jq -r '.metadata.namespace')
name=$(oc get example -n test1 -o json example-sample | jq -r '.metadata.name')
~~~

Then, spawn a deployment and set the test1/example-sammple CR as the owner:
~~~
cat << EOF | oc apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: fedora-deployment
  namespace: $namespace
  labels:
    app: fedora-deployment
  ownerReferences:
  - kind: $kind
    apiVersion: $apiversion
    name: $name
    uid: $uid
    controller: true
spec:
  replicas: 1
  selector:
    matchLabels:
      app: fedora-pod
  template:
    metadata:
      labels:
        app: fedora-pod
    spec:
      containers:
      - name: fedora
        image: fedora
        command:
          - sleep
          - infinity
        imagePullPolicy: IfNotPresent
EOF
~~~

This will trigger a run of the Reconciler:
~~~
2020-12-03T03:23:14.431-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T03:23:14.431-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
2020-12-03T03:23:14.440-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T03:23:14.440-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
2020-12-03T03:23:14.457-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T03:23:14.457-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
2020-12-03T03:23:14.479-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T03:23:14.479-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
2020-12-03T03:23:15.621-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T03:23:15.621-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
~~~

And if we scale the deployment, for example:
~~~
[root@kind example-operator]# oc scale --replicas=2 deployment fedora-deployment -n test1
deployment.apps/fedora-deployment scaled
~~~

~~~
2020-12-03T03:26:47.860-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T03:26:47.860-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
2020-12-03T03:26:47.876-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T03:26:47.877-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
2020-12-03T03:26:47.888-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T03:26:47.888-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
2020-12-03T03:26:47.909-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T03:26:47.909-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
2020-12-03T03:26:49.645-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T03:26:49.646-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
~~~

The `Owns` method can be rewritten with:
~~~
import (
        (...)
        appsv1 "k8s.io/api/apps/v1"
        batchv1 "k8s.io/api/batch/v1"
	(...)
)
(...)
func (r *ExampleReconciler) SetupWithManager(mgr ctrl.Manager) error {
        return ctrl.NewControllerManagedBy(mgr).
                For(&examplev1alpha1.Example{}).
                Watches(&source.Kind{
                        Type: &appsv1.Deployment{}},
                        &handler.EnqueueRequestForOwner{
                                OwnerType: &examplev1alpha1.Example{},
                                IsController: true,
                        }).
                Watches(&source.Kind{
                        Type: &batchv1.Job{}},
                        &handler.EnqueueRequestForOwner{
                                OwnerType: &examplev1alpha1.Example{},
                                IsController: true,
                        }).
                Complete(r)
}
~~~

##### Watches for independent resources

If you want to monitor other custom resources which are not owned by this Controller, that's also possible.

###### Watching for changes to Nodes

For example, let's monitor updates to `Node`s. If any node in the cluster is updated, then we are going to inform all of the Example Custom Resources.

~~~
(...)
import (
(...)
        corev1 "k8s.io/api/core/v1"
        "k8s.io/apimachinery/pkg/types"
        "sigs.k8s.io/controller-runtime/pkg/reconcile" 
(...)
)
(...)
func (r *ExampleReconciler) SetupWithManager(mgr ctrl.Manager) error {
        mapFn := handler.ToRequestsFunc(
                func(a handler.MapObject) []reconcile.Request {
                        client := mgr.GetClient()
                        log := mgr.GetLogger()
                        log.Info("Detected update to resource",
                                "Kind", a.Object.GetObjectKind(),
                                "NameSpace", a.Meta.GetNamespace(),
                                "Name", a.Meta.GetName())

                        log.Info("Retrieving all Example CRs")
                        exampleList := &examplev1alpha1.ExampleList{}
                        err := client.List(context.Background(), exampleList)
                        if err != nil {
                                log.Info("Err, returning empty []reconcile.Request{}")
                                return []reconcile.Request{}
                        }
                        var reconcileRequests []reconcile.Request
                        for _, example := range exampleList.Items {
                                log.Info("Sending reconcile request to example custom resource",
                                        "Name",
                                        example.Name)
                                reconcileRequests = append(reconcileRequests, reconcile.Request{
                                        NamespacedName: types.NamespacedName{
                                                Name:      example.Name,
                                                Namespace: example.Namespace,
                                        },
                                })
                        }
                        return reconcileRequests
                })


        return ctrl.NewControllerManagedBy(mgr).
                For(&examplev1alpha1.Example{}).
                Watches(&source.Kind{Type: &corev1.Node{}},
                        &handler.EnqueueRequestsFromMapFunc{ToRequests: mapFn}).
                Complete(r)
}
~~~

~~~
[root@kind example-operator]# oc annotate  node kind-control-plane foo=bar
node/kind-control-plane annotated
[root@kind example-operator]# 
~~~

~~~
2020-12-03T04:11:37.062-0500	INFO	Detected update to resource	{"Kind": "&TypeMeta{Kind:,APIVersion:,}", "NameSpace": "", "Name": "kind-control-plane"}
2020-12-03T04:11:37.062-0500	INFO	Retrieving all Example CRs
2020-12-03T04:11:37.062-0500	INFO	Sending reconcile request to example custom resource	{"Name": "example-sample"}
2020-12-03T04:11:37.062-0500	INFO	Detected update to resource	{"Kind": "&TypeMeta{Kind:,APIVersion:,}", "NameSpace": "", "Name": "kind-control-plane"}
2020-12-03T04:11:37.062-0500	INFO	Retrieving all Example CRs
2020-12-03T04:11:37.062-0500	INFO	Sending reconcile request to example custom resource	{"Name": "example-sample"}
2020-12-03T04:11:37.062-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T04:11:37.062-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
~~~

###### Watching for changes to other Custom Resources

Let's create a new custom resource, `ExampleInstance`:
~~~
operator-sdk create api --group example --version v1alpha1 --kind ExampleInstance --resource=true --controller=false
make generate
make manifests
make install
~~~

We need to make an important change to `mapFn`. Given that we are working with a namespaced Custom Resource, we need to make sure that we only trigger the reconciler when the namespace of the `Example` and `ExampleInstance` resources are the same:
~~~
(...)
                        for _, example := range exampleList.Items {
                                if a.Meta.GetNamespace() != example.Namespace {
                                        continue
                                }
(...)
~~~

Our method would look like this:
~~~
func (r *ExampleReconciler) SetupWithManager(mgr ctrl.Manager) error {
        mapFn := handler.ToRequestsFunc(
                func(a handler.MapObject) []reconcile.Request {
                        client := mgr.GetClient()
                        log := mgr.GetLogger()
                        log.Info("Detected update to resource",
                                "Kind", a.Object.GetObjectKind(),
                                "NameSpace", a.Meta.GetNamespace(),
                                "Name", a.Meta.GetName())

                        log.Info("Retrieving all Example CRs")
                        exampleList := &examplev1alpha1.ExampleList{}
                        err := client.List(context.Background(), exampleList)
                        if err != nil {
                                log.Info("Err, returning empty []reconcile.Request{}")
                                return []reconcile.Request{}
                        }
                        var reconcileRequests []reconcile.Request
                        for _, example := range exampleList.Items {
                                if a.Meta.GetNamespace() != example.Namespace {
                                        continue
                                }
                                log.Info("Sending reconcile request to example custom resource",
                                        "Name",
                                        example.Name)
                                reconcileRequests = append(reconcileRequests, reconcile.Request{
                                        NamespacedName: types.NamespacedName{
                                                Name:      example.Name,
                                                Namespace: example.Namespace,
                                        },
                                })
                        }
                        return reconcileRequests
                })

        return ctrl.NewControllerManagedBy(mgr).
                For(&examplev1alpha1.Example{}).
                Watches(&source.Kind{Type: &examplev1alpha1.ExampleInstance{}},
                        &handler.EnqueueRequestsFromMapFunc{ToRequests: mapFn}).
                Complete(r)
}
~~~

Now, if we test this, we can see that the reconciler is only triggered if the Namespace is correct:
~~~
[root@kind example-operator]# cat config/samples/example_v1alpha1_exampleinstance2.yaml
apiVersion: example.example.com/v1alpha1
kind: ExampleInstance
metadata:
  name: exampleinstance-sample
  namespace: test2
spec:
  # Add fields here
  foo: bar
[root@kind example-operator]# oc apply -f config/samples/example_v1alpha1_exampleinstance2.yaml
exampleinstance.example.example.com/exampleinstance-sample created
~~~

~~~
2020-12-03T04:25:59.924-0500	INFO	Detected update to resource	{"Kind": "&TypeMeta{Kind:,APIVersion:,}", "NameSpace": "test2", "Name": "exampleinstance-sample"}
2020-12-03T04:25:59.924-0500	INFO	Retrieving all Example CRs
~~~

~~~
[root@kind example-operator]# cat config/samples/example_v1alpha1_exampleinstance.yaml
apiVersion: example.example.com/v1alpha1
kind: ExampleInstance
metadata:
  name: exampleinstance-sample
  namespace: test1
spec:
  # Add fields here
  foo: bar
[root@kind example-operator]# oc apply -f config/samples/example_v1alpha1_exampleinstance.yaml
exampleinstance.example.example.com/exampleinstance-sample created
~~~

~~~
2020-12-03T04:27:01.639-0500	INFO	Detected update to resource	{"Kind": "&TypeMeta{Kind:,APIVersion:,}", "NameSpace": "test1", "Name": "exampleinstance-sample"}
2020-12-03T04:27:01.639-0500	INFO	Retrieving all Example CRs
2020-12-03T04:27:01.639-0500	INFO	Sending reconcile request to example custom resource	{"Name": "example-sample"}
2020-12-03T04:27:01.639-0500	INFO	controllers.Example	Reconciler triggered with req:	{"example": "test1/example-sample"}
2020-12-03T04:27:01.639-0500	DEBUG	controller	Successfully Reconciled	{"reconcilerGroup": "example.example.com", "reconcilerKind": "Example", "controller": "example", "name": "example-sample", "namespace": "test1"}
~~~
