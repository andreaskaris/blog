## How to test etcd performance

### Background 

For more background about etcd performance, see: [https://github.com/etcd-io/etcd/blob/master/Documentation/op-guide/performance.md](https://github.com/etcd-io/etcd/blob/master/Documentation/op-guide/performance.md)

Also see: [https://github.com/etcd-io/etcd/blob/master/Documentation/op-guide/performance.md](https://github.com/etcd-io/etcd/blob/master/Documentation/op-guide/performance.md)

For etcd cluster hardware recommendations, see: [https://github.com/etcd-io/etcd/blob/master/Documentation/op-guide/hardware.md#example-hardware-configurations](https://github.com/etcd-io/etcd/blob/master/Documentation/op-guide/hardware.md#example-hardware-configurations)

### etcdctl check perf - how it works

The above hardware configuration table has slightly different definitions of size compared to `etcdctl check perf`.

The `etcdctl check perf` command will simulate specific workload scenarios for small(50 clients), medium (200 clients), large (500 clients) and extra large clusters (1000 clients). It will always run for 60 seconds.

You can also check the code here:
[https://github.com/etcd-io/etcd/blob/master/etcdctl/ctlv3/command/check.go#L127](https://github.com/etcd-io/etcd/blob/master/etcdctl/ctlv3/command/check.go#L127)
~~~
The performance check's workload model. Accepted workloads: s(small), m(medium), l(large), xl(xLarge)
~~~

The tests run by `etcdctl check perf` will report PASS/FAIL depending on if they are within the following bounds:

| Measure  | Limit   |
|---|---|
| Throughput  | < 150 * 0.9 writes/s (s) ; < 1000 * 0.9 writes/s (m) ; < 8000 * 0.9 writes/s (l) ; < 15000 * 0.9 writes/s (xl)  |
| Slowest request  | > 500 ms   |
| Standard deviation | > 100 ms   |

Additionally, etcd checks if there are any errors.

[https://github.com/etcd-io/etcd/blob/6e800b9b0161ef874784fc6c679325acd67e2452/etcdctl/ctlv3/command/check.go#L51](https://github.com/etcd-io/etcd/blob/6e800b9b0161ef874784fc6c679325acd67e2452/etcdctl/ctlv3/command/check.go#L51)
~~~
(...)
var checkPerfCfgMap = map[string]checkPerfCfg{
	// TODO: support read limit
	"s": {
		limit:    150,
		clients:  50,
		duration: 60,
	},
	"m": {
		limit:    1000,
		clients:  200,
		duration: 60,
	},
	"l": {
		limit:    8000,
		clients:  500,
		duration: 60,
	},
	"xl": {
		limit:    15000,
		clients:  1000,
		duration: 60,
	},
}
(...)
~~~

[https://github.com/etcd-io/etcd/blob/6e800b9b0161ef874784fc6c679325acd67e2452/etcdctl/ctlv3/command/check.go#L232](https://github.com/etcd-io/etcd/blob/6e800b9b0161ef874784fc6c679325acd67e2452/etcdctl/ctlv3/command/check.go#L232)
~~~
(...)
	ok = true
	if len(s.ErrorDist) != 0 {
		fmt.Println("FAIL: too many errors")
		for k, v := range s.ErrorDist {
			fmt.Printf("FAIL: ERROR(%v) -> %d\n", k, v)
		}
		ok = false
	}

	if s.RPS/float64(cfg.limit) <= 0.9 {
		fmt.Printf("FAIL: Throughput too low: %d writes/s\n", int(s.RPS)+1)
		ok = false
	} else {
		fmt.Printf("PASS: Throughput is %d writes/s\n", int(s.RPS)+1)
	}
	if s.Slowest > 0.5 { // slowest request > 500ms
		fmt.Printf("Slowest request took too long: %fs\n", s.Slowest)
		ok = false
	} else {
		fmt.Printf("PASS: Slowest request took %fs\n", s.Slowest)
	}
	if s.Stddev > 0.1 { // stddev > 100ms
		fmt.Printf("Stddev too high: %fs\n", s.Stddev)
		ok = false
	} else {
		fmt.Printf("PASS: Stddev is %fs\n", s.Stddev)
	}

	if ok {
		fmt.Println("PASS")
	} else {
		fmt.Println("FAIL")
		os.Exit(ExitError)
	}
(...)
~~~

The processing of a run's metrics is done here:
[https://github.com/etcd-io/etcd/blob/6e800b9b0161ef874784fc6c679325acd67e2452/pkg/report/report.go#L185](https://github.com/etcd-io/etcd/blob/6e800b9b0161ef874784fc6c679325acd67e2452/pkg/report/report.go#L185)
~~~
(...)
func (r *report) processResults() {
	st := time.Now()
	for res := range r.results {
		r.processResult(&res)
	}
	r.stats.Total = time.Since(st)

	r.stats.RPS = float64(len(r.stats.Lats)) / r.stats.Total.Seconds()
	r.stats.Average = r.stats.AvgTotal / float64(len(r.stats.Lats))
	for i := range r.stats.Lats {
		dev := r.stats.Lats[i] - r.stats.Average
		r.stats.Stddev += dev * dev
	}
	r.stats.Stddev = math.Sqrt(r.stats.Stddev / float64(len(r.stats.Lats)))
	sort.Float64s(r.stats.Lats)
	if len(r.stats.Lats) > 0 {
		r.stats.Fastest = r.stats.Lats[0]
		r.stats.Slowest = r.stats.Lats[len(r.stats.Lats)-1]
	}
}
(...)
~~~

### Running etcdctl check perf

The following illustrated how to test etcd performance in OpenShift 4.5

Get the name of one of the etcd pods:
~~~
ETCD_POD=$(oc -n openshift-etcd get pods -l app=etcd -o name | head -1)
~~~

Run a normal test:
~~~
oc exec  -n openshift-etcd -it -c etcd $ETCD_POD -- etcdctl check perf
~~~

Run a medium test:
~~~
oc exec  -n openshift-etcd -it -c etcd $ETCD_POD -- etcdctl check perf --load='m'
~~~

Run a large test:
~~~
oc exec  -n openshift-etcd -it -c etcd $ETCD_POD -- etcdctl check perf --load='l'
~~~

Run an extra large test:
~~~
oc exec  -n openshift-etcd -it -c etcd $ETCD_POD -- etcdctl check perf --load='xl'
~~~

Here's the output of a failed run:
~~~
# oc exec  -n openshift-etcd -it -c etcd $ETCD_POD -- etcdctl check perf --load='m'
 60 / 60 Boooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo! 100.00% 1m0s
FAIL: Throughput too low: 898 writes/s
Slowest request took too long: 2.483523s
Stddev too high: 0.170017s
FAIL
command terminated with exit code 1
~~~
