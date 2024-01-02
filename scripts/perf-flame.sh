echo $@
perf record -g $@
perf script | ./stackcollapse-perf.pl | ./flamegraph.pl > perf.svg
scp -P 30022 perf.svg zzr@10.177.31.73:/home/zzr/img
