# HOLI (Updating)

HOLI, a height optimized learned range index framework base on B+ tree, reduce tree average height by height optimization strategies and hash table, aiming to achieve high lookup and range query performance in single thread, and has competive performance in concurrent scenarios.

## Running benchmark

HOLI's performance can be assess using the GRE benchmarking toool and TLI benchmarking tool. We have integrated HOLI into GRE as "[GRE_HOLI](https://github.com/ZhengZhuorui/GRE_HOLI)", which a fork of GRE_SALI. IN GRE_HOLI, you can assess the performance of HOLI comprehensively. 

## Compile & Run

The project support a demo code, a micro unit test and microbenchmark. Please use *run.sh* to run program. Here *aex* is the index name because I don't name it initally.


### Download other index
```
cd thirdparty
git clone git@github.com:microsoft/ALEX.git
git clone git@github.com:git@github.com:gvinciguerra/PGM-index.git
git clone git@github.com:Jiacheng-WU/lipp.git
git clone git@github.com:bingmann/stx-btree.git
```

### Run demo
```
bash run.sh run ./demo
bash run.sh run ./demo_con
```

### Unit test

Please see test/unit_test.sh to test all kind of components. Some unit test are deprecated.

```
#example:
bash run.sh run_debug ./unit_test --unit=index --key_type=uint64 --con --function=insert --num_keys=1000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64
```

### Micro benchmark

This differs significantly from the actual benchmark results. Please refer to ​GRE and ​TLI for precise measurements. It use to test relative performance pre-benchmark to update 


```

#example:
./benchmark --key_type=uint64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=20000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./benchmark --key_type=uint64 --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=20000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
```

## build & example
```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```
