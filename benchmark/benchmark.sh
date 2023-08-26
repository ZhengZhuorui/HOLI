# =================================================================================================
# build time 
./benchmark --key_type=float --index=aex --function=build --num_keys=200000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=build --query_dis=uniform --num_keys=200000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=build --query_dis=uniform --num_keys=200000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=build --query_dis=uniform --num_keys=200000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

# Dataset: longtitudes
# size: 200M
# batch: 65536
# 

# =================================================================================================
# lookup
./benchmark --key_type=float --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=65536 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=65536 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=65536 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=65536 --input_file=/home/zzr/data/longitudes-200M.bin.data

# Dataset: longtitudes
# size: 200M
# batch: 65536
# Result: (ms, QPS)
# aex: 135629, 4.83e6
# stl_map: 867981, 755039
# alex: 9.402e6, 6.97e6

# =================================================================================================
# insert

# =================================================================================================
# erase

# =================================================================================================
# range query

# =================================================================================================
# read + write mix


