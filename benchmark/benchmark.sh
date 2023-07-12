

./benchmark --key_type=float --index=alex --function=lookup --query_dis=uniform --num_keys=20000 --num_ops=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./benchmark --key_type=float --index=stl_map --function=lookup --query_dis=uniform --num_keys=20000 --num_ops=1000000--input_file=/home/zzr/data/longitudes-200M.bin.data

./benchmark --key_type=float --index=alex --function=lookup --query_dis=uniform --num_keys=20000000 --num_ops=500000 --input_file=/home/zzr/data/longitudes-200M.bin.data


# Dataset:logtitude
# size: 20M
# batch: 
# insert_frac=0
# Result: (QPS)
# STL-MAP           B-Tree          ALEX AEX
#                                   
# 
# 

