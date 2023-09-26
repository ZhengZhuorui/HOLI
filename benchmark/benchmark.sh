# =================================================================================================
# build time 
# example: ./benchmark --key_type=float --index=stl_map --function=build --num_keys=20000 --input_file=/home/zzr/data/longitudes-200M.bin.data

./benchmark --key_type=float --index=aex --function=build --num_keys=200000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=build --query_dis=uniform --num_keys=200000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=build --query_dis=uniform --num_keys=200000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=build --query_dis=uniform --num_keys=200000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=lipp --function=build --query_dis=uniform --num_keys=200000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# Dataset: longtitudes
# size: 200M
# batch: 65536
# 

# =================================================================================================
# lookup

# example
./benchmark --key_type=float --index=aex --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=lookup --query_dis=uniform --num_keys=200 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=lipp --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data

./benchmark --key_type=int --index=aex --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=stl_map --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=stx_btree --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=alex --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=pgm --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=lipp --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/ycsb-200M.bin.data

#longtitude
./benchmark --key_type=float --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

./benchmark --key_type=float --index=hash --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

# longlat
./benchmark --key_type=float --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longlat-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longlat-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longlat-200M.bin.data
./benchmark --key_type=float --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longlat-200M.bin.data
./benchmark --key_type=float --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longlat-200M.bin.data
./benchmark --key_type=float --index=hash --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longlat-200M.bin.data

# uniform

# normal
./benchmark --key_type=float --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=search --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

#ycsb
./benchmark --key_type=int --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=pgm --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=int --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

# lognormal
./benchmark --key_type=float --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=float --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=float --index=pgm --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=lipp --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data

# Dataset: longtitudes
# size: 200M
# batch: 65536
# Result: (ms, QPS)
# aex: 135629, 4.83e6
# stl_map: 867981, 755039
# alex: 9.402e6, 6.97e6

# =================================================================================================
# delta_query
# example
./benchmark --key_type=float --index=aex --function=delta_lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=delta_lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=delta_lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=delta_lookup --query_dis=uniform --num_keys=200 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=lipp --function=delta_lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=/home/zzr/data/longitudes-200M.bin.data


./benchmark --key_type=float --index=aex --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=lipp --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=hash --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

./benchmark --key_type=float --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=hash --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data



# =================================================================================================
# insert
# example
./benchmark --key_type=float --index=aex --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=pgm --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=lipp --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data


# longtitude
./benchmark --key_type=float --index=aex --function=insert --query_dis=uniform --num_keys=100000000 --num_ops=100000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=insert --query_dis=uniform --num_keys=100000000 --num_ops=100000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=insert --query_dis=uniform --num_keys=100000000 --num_ops=100000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=insert --query_dis=uniform --num_keys=100000000 --num_ops=100000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=pgm --function=insert --query_dis=uniform --num_keys=100000000 --num_ops=100000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=lipp --function=insert --query_dis=uniform --num_keys=100000000 --num_ops=100000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

# ycsb
./benchmark --key_type=int --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=hash --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/ycsb-200M.bin.data

# lognormal 
./benchmark --key_type=float --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/lognormal_200M_0_1_float.bin
./benchmark --key_type=float --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/lognormal_200M_0_1_float.bin
./benchmark --key_type=float --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/lognormal_200M_0_1_float.bin
./benchmark --key_type=float --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/lognormal_200M_0_1_float.bin
./benchmark --key_type=float --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/lognormal_200M_0_1_float.bin
./benchmark --key_type=float --index=hash --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=/home/zzr/data/lognormal_200M_0_1_float.bin

# =================================================================================================
# erase

# =================================================================================================
# range query
# example:
./benchmark --key_type=float --index=aex --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=pgm --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=lipp --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/longitudes-200M.bin.data

./benchmark --key_type=int --index=aex --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=stl_map --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=stx_btree --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=alex --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=pgm --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/ycsb-200M.bin.data
./benchmark --key_type=int --index=lipp --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=/home/zzr/data/ycsb-200M.bin.data

./benchmark --key_type=float --index=aex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=pgm --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=lipp --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=/home/zzr/data/longitudes-200M.bin.data

# =================================================================================================
# read + write mix


