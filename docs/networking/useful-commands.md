# Useful commands

## Tracking the evolution of conntrack with lnstat

lnstat is a neat tool to check the evolution of entries (and other counters) in the conntrack table:
~~~
# lnstat -f nf_conntrack
nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|nf_connt|
 entries|clashres|   found|     new| invalid|  ignore|  delete|delete_l|  insert|insert_f|    drop|early_dr|icmp_err|expect_n|expect_c|expect_d|search_r|
        |        |        |        |        |        |        |     ist|        |   ailed|        |      op|      or|      ew|   reate|   elete|  estart|
    1202|     134|       0|       0| 3113180|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|    2659|
    1216|       0|       0|       0|       2|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|
    1173|       0|       0|       0|       1|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|
    1169|       0|       0|       0|       1|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|
    1168|       0|       0|       0|       1|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|       0|
~~~

For more details, see: https://linux.die.net/man/8/lnstat
