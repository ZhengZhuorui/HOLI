# index


生成数据集
```
./geenrate_dataset --dataset=uniform_int \
                   --num_keys=10000000 \
                   --output_file=/home/zzr/data/uniform_int_10M.bin
```

基准测试
```
./benchmark --dataset=uniform_int \
            --num_keys=10000000 \
            --
```

测试
```
./test --
```