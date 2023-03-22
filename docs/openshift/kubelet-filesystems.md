# How kubelet monitors filesystems

Kubelet can monitor 2 file systems, nodefs and imagefs. nodefs is auto-discovered by the presence of `/var/lib/kubelet`.
imagefs is the location where container images are stored:

~~~
The kubelet supports the following filesystem partitions:

    nodefs: The node's main filesystem, used for local disk volumes, emptyDir, log storage, and more.
            For example, nodefs contains /var/lib/kubelet/.
    imagefs: An optional filesystem that container runtimes use to store container images and
             container writable layers.

Kubelet auto-discovers these filesystems and ignores other filesystems. Kubelet does not support
other configurations.
~~~
> [https://kubernetes.io/docs/concepts/scheduling-eviction/node-pressure-eviction/#eviction-policy](https://kubernetes.io/docs/concepts/scheduling-eviction/node-pressure-eviction/#eviction-policy)

Whenever both `/var/lib/kubelet` and `/var/lib/containers` reside on the same file system, metrics for nodefs and imagefs
will be the same.

The alternative is to mount `/var/lib/containers` onto its own partition. In that case, the kubelet will produce independent
metrics for both filesystems.

## Querying nodefs and imagefs metrics on OpenShift

Here is how we can query the kubelet's metrics for nodefs and imagefs. I derived this example from
[https://gist.github.com/superseb/a4fa9640d801c54452132db8af51f2e4](https://gist.github.com/superseb/a4fa9640d801c54452132db8af51f2e4):

~~~
TOKEN=$(kubectl get secrets -n openshift-cluster-version -o jsonpath="{.items[?(@.metadata.annotat \
         ions['kubernetes\.io/service-account\.name']=='default')].data.token}" | base64 --decode)
curl -k -H "Authorization: Bearer ${TOKEN}" https://<node address>:10250/stats/summary 2>/dev/null | \
         jq '.node.fs,.node.runtime.imageFs'
~~~
> NOTE: In case of a firewall blocking port 10250, it's also possible to connect to the node itself, copy/paste the
TOKEN into the terminal and run the curl against 127.0.0.1:10250.

On a system where `/var/lib/containers` is mounted on its own partition:

~~~
$ TOKEN=$(kubectl get secrets -n openshift-cluster-version -o jsonpath="{.items[?(@.metadata.annotat \
           ions['kubernetes\.io/service-account\.name']=='default')].data.token}" | base64 --decode)
$ curl -k -H "Authorization: Bearer ${TOKEN}" \
           https://worker01.redhat-ocp1.e5gc.bos.redhat.lab:10250/stats/summary 2>/dev/null | \
           jq '.node.fs,.node.runtime.imageFs'
{
  "time": "2023-03-21T15:20:43Z",
  "availableBytes": 28379086848,
  "capacityBytes": 51880394752,
  "usedBytes": 23501307904,
  "inodesFree": 25296924,
  "inodes": 25337344,
  "inodesUsed": 40420
}
{
  "time": "2023-03-21T15:20:43Z",
  "availableBytes": 235965177856,
  "capacityBytes": 246890082304,
  "usedBytes": 26686946160,
  "inodesFree": 120490599,
  "inodes": 120610752,
  "inodesUsed": 120153
}
~~~

On a system where `/var/lib/containers` is not mounted on a separate partition. You can see that the reported values are
exactly the same:

~~~
$ curl -k -H "Authorization: Bearer ${TOKEN}" https://192.168.18.22:10250/stats/summary 2>/dev/null | \
          jq '.node.fs,.node.runtime.imageFs'
{
  "time": "2023-03-21T15:22:53Z",
  "availableBytes": 423603036160,
  "capacityBytes": 479555555328,
  "usedBytes": 55952519168,
  "inodesFree": 233893866,
  "inodes": 234163072,
  "inodesUsed": 269206
}
{
  "time": "2023-03-21T15:22:53Z",
  "availableBytes": 423603036160,
  "capacityBytes": 479555555328,
  "usedBytes": 100197083100,
  "inodesFree": 233893866,
  "inodes": 234163072,
  "inodesUsed": 269206
}
~~~

### Short code analysis

Let's have a quick look at relevant code sections in [https://github.com/kubernetes/kubernetes](https://github.com/kubernetes/kubernetes).
`kubeletServer.RootDirectory` is set to either `/var/lib/kubelet` or to the value of parameter `--root-dir`.
When reading file system statistics, eventually `cc.GetDirFsInfo(cc.rootPath)` is called which will get file system information for the file system that contains the given directory.

.../kubernetes/cmd/kubelet/app/options/options.go

~~~
(...)
   44 const defaultRootDir = "/var/lib/kubelet"
(...)
  137 // NewKubeletFlags will create a new KubeletFlags with default values
  138 func NewKubeletFlags() *KubeletFlags {
  139     return &KubeletFlags{
  140         ContainerRuntimeOptions: *NewContainerRuntimeOptions(),
  141         CertDirectory:           "/var/lib/kubelet/pki",
  142         RootDirectory:           filepath.Clean(defaultRootDir),
(...)
func (...) AddFlags (...)
(...)
  298     fs.StringVar(&f.RootDirectory, "root-dir", f.RootDirectory, "Directory path for managing kubelet files (volume mounts,etc).")
(...)
~~~

.../kubernetes/cmd/kubelet/app/server.go

~~~
(...)
   124 // NewKubeletCommand creates a *cobra.Command object with default parameters
   125 func NewKubeletCommand() *cobra.Command {
   126     cleanFlagSet := pflag.NewFlagSet(componentKubelet, pflag.ContinueOnError)
   127     cleanFlagSet.SetNormalizeFunc(cliflag.WordSepNormalizeFunc)
   128     kubeletFlags := options.NewKubeletFlags()
(...)
   237             // construct a KubeletServer from kubeletFlags and kubeletConfig
   238             kubeletServer := &options.KubeletServer{
   239                 KubeletFlags:         *kubeletFlags,
   240                 KubeletConfiguration: *kubeletConfig,
   241             }
(...)
   264             utilfeature.DefaultMutableFeatureGate.AddMetrics()
   265             // run the kubelet
   266             return Run(ctx, kubeletServer, kubeletDeps, utilfeature.DefaultFeatureGate)
   267         },
   268     }
(...)
   270     // keep cleanFlagSet separate, so Cobra doesn't pollute it with the global flags
   271     kubeletFlags.AddFlags(cleanFlagSet)
(...)
   409 // Run runs the specified KubeletServer with the given Dependencies. This should never exit.
   410 // The kubeDeps argument may be nil - if so, it is initialized from the settings on KubeletServer.
   411 // Otherwise, the caller is assumed to have set up the Dependencies object and a default one will
   412 // not be generated.
   413 func Run(ctx context.Context, s *options.KubeletServer, kubeDeps *kubelet.Dependencies, featureGate featuregate.FeatureGate) error {
   414     // To help debugging, immediately log version
   415     klog.InfoS("Kubelet version", "kubeletVersion", version.Get())
   416
   417     klog.InfoS("Golang settings", "GOGC", os.Getenv("GOGC"), "GOMAXPROCS", os.Getenv("GOMAXPROCS"), "GOTRACEBACK", os.Getenv("GOTRACEBACK"))
   418
   419     if err := initForOS(s.KubeletFlags.WindowsService, s.KubeletFlags.WindowsPriorityClass); err != nil {
   420         return fmt.Errorf("failed OS init: %w", err)
   421     }
   422     if err := run(ctx, s, kubeDeps, featureGate); err != nil {
   423         return fmt.Errorf("failed to run Kubelet: %w", err)
   424     }
   425     return nil
   426 }
(...)
   649     if kubeDeps.CAdvisorInterface == nil {
   650         imageFsInfoProvider := cadvisor.NewImageFsInfoProvider(s.ContainerRuntimeEndpoint)
   651         kubeDeps.CAdvisorInterface, err = cadvisor.New(imageFsInfoProvider, s.RootDirectory, cgroupRoots, cadvisor.UsingLegacyCadvisorStats(s.ContainerRuntimeEndpoint), s.LocalStorageCapacityIsolation)
   652         if err != nil {
   653             return err
   654         }
   655     }
(...)
~~~

.../kubernetes/pkg/kubelet/cadvisor/cadvisor_linux.go

~~~
(...)
   80 // New creates a new cAdvisor Interface for linux systems.
   81 func New(imageFsInfoProvider ImageFsInfoProvider, rootPath string, cgroupRoots []string, usingLegacyStats, localStorageCapacityIsolation bool) (Interface, error) {
(...)
  121     return &cadvisorClient{
  122         imageFsInfoProvider: imageFsInfoProvider,
  123         rootPath:            rootPath,
(...)
  169 func (cc *cadvisorClient) RootFsInfo() (cadvisorapiv2.FsInfo, error) {
  170     return cc.GetDirFsInfo(cc.rootPath)
  171 }
(...)
~~~

.../kubernetes/vendor/github.com/google/cadvisor/manager/manager.go

~~~
(...)
   119     // Get filesystem information for the filesystem that contains the given directory
   120     GetDirFsInfo(dir string) (v2.FsInfo, error)
(...)
~~~
