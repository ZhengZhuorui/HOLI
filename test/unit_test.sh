# generate dataset

# (Y)
./generate_dataset --key_type=int --num_keys=1000000 --distribution=uniform --output_file=/home/zzr/data/generate_data/uniform_1M_int.bin --lower=-1000000000000 --upper=1000000000000

# (Y)
./generate_dataset --key_type=int --num_keys=1000 --distribution=uniform --output_file=/home/zzr/data/generate_data/uniform_1K_int.bin --lower=-1000000000000 --upper=100000000000

# (Y)
./generate_dataset --key_type=float --num_keys=1000 --distribution=uniform --output_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin --lower=-100 --upper=100

# (Y)
./generate_dataset --key_type=float --num_keys=1000000 --distribution=uniform --output_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin --lower=-100 --upper=100

# (Y)
./generate_dataset --key_type=float --num_keys=1000 --distribution=normal --output_file=/home/zzr/data/generate_data/normal_1K_0_1_float.bin --mean=0 --stddev=1

./generate_dataset --key_type=float --num_keys=200000000 --distribution=normal --output_file=/home/zzr/data/generate_data/normal_200M_0_1_float.bin --mean=0 --stddev=1

# (N)
./generate_dataset --key_type=float --num_keys=1000000 --distribution=lognormal --output_file=/home/zzr/data/generate_data/normal_1K_0_1_float.bin --mean=0 --stddev=1

# (Y)
./generate_dataset --key_type=int --num_keys=1000000 --distribution=id_ascend --output_file=/home/zzr/data/generate_data/id_1M_int.bin

# =================================================================================================
# test find function(STL(bineary lower bound), ALEX(exponential find), exponential find)
# (Y)
./unit_test --unit=function --key_type=int --num_keys=1000000 --function=exp_lower_bound --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin
# (Y)
./unit_test --unit=function --key_type=int --num_keys=1000000 --function=exp_lower_bound --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=function --key_type=int --num_keys=1000000 --function=search_perf --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin
# result(us):
# self exp lower bound used time=330974 us
# ALEX exp lower bound used time=775420 us
# STL lower bound used time=1301185 us
# (Y)
./unit_test --unit=function --key_type=int --num_keys=1000000 --function=search_with_error_bound_perf --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin
# result(us):
# self exp lower bound used time=180819 us
# ALEX exp lower bound used time=233474 us
# STL lower bound used time=236411 us
# search used time=205381 us

# test linear probe (with exponential probe)
# (Y)
./unit_test --unit=function --key_type=int --num_keys=1000000 --function=linear_probe --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin
# (Y)
./unit_test --unit=function --key_type=float --num_keys=1000 --function=linear_probe --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=function --key_type=float --num_keys=1000000 --function=linear_probe --input_file=/home/zzr/data/longitudes-200M.bin.data

# result(ms, NPS(number per second))
# 50, 11ms, 4.54e7
# 173, 40ms, 4.325e7
# 85, 16ms, 5.32e7

# =================================================================================================
# test fitting model(linear, logarithmic, exponential, quandratic, gap array linear)
# (Y)
./unit_test --unit=model --key_type=int --num_keys=1000 --model_type=linear --input_file=/home/zzr/data/generate_data/uniform_1K_int.bin
# (Y)
./unit_test --unit=model --key_type=float --num_keys=1000 --model_type=linear --spec=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=model --key_type=float --num_keys=1000 --model_type=log --spec=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=model --key_type=float --num_keys=1000 --model_type=exp --spec=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=model --key_type=float --num_keys=1000 --model_type=quad --spec=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# Dataset: uniform->corresponding distribution
# result(RMSE):
# linear: 5.99
# logarithmic: 91
# exponential: ~
# quandratic: 4

#(Y)
./unit_test --unit=model --key_type=float --num_keys=1000 --model_type=linear --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

#(Y)
./unit_test --unit=model --key_type=float --num_keys=1000 --model_type=log --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

#(Y)
./unit_test --unit=model --key_type=float --num_keys=1000 --model_type=exp --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

#(Y)
./unit_test --unit=model --key_type=float --num_keys=1000 --model_type=quad --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

#(Y)
./unit_test --unit=model --key_type=float --num_keys=1000 --model_type=gap_linear --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# Dataset: uniform
# result(RMSE):
# linear: 5.99
# logarithmic: 129
# exponential: 128
# quandratic: 5
# gap linear: 10.35

# test gap array fitting model(linear, gap array linear)
#(Y)
./unit_test --unit=model --key_type=float --num_keys=128 --model_type=linear --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
#(Y)
./unit_test --unit=model --key_type=float --num_keys=128 --model_type=gap_linear --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# Dataset: uniform
# result(max error):
# linear: 3
# gap linear: 5

#(Y)
./unit_test --unit=model --key_type=float --num_keys=1024 --model_type=linear --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
#(Y)
./unit_test --unit=model --key_type=float --num_keys=1024 --model_type=gap_linear --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
#(Y)
./unit_test --unit=model --key_type=float --num_keys=1024 --model_type=piecewise_linear --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
<<<<<<< HEAD
./unit_test --unit=model --key_type=float --num_keys=4096 --model_type=piecewise_linear --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
=======
>>>>>>> dd70831881e0ac77af93e9b01b9d8a39425d3470
# Dataset: uniform
# size: 1024
# result(max error):
# linear: 9
# gap linear: 13
# piecewise_linear: 3

# give up radix-based tree

# (Y)
./unit_test --unit=model --key_type=float --num_keys=2000000 --model_type=linear --input_file=/home/zzr/data/longitudes-200M.bin.data
# (Y)
./unit_test --unit=model --key_type=float --num_keys=1000000 --model_type=linear --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=model --key_type=float --num_keys=2000000 --model_type=piecewise_linear --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=model --key_type=float --num_keys=2000000 --model_type=piecewise_linear --input_file=/home/zzr/data/longitudes-200M.bin.data

# =================================================================================================
# test inner node(few) (gap array) and data node(dense array) insertion accuracy and performance
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=64 --batch=8 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=128 --batch=8 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=256 --batch=8 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=512 --batch=8 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=1024 --batch=8 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=128 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=256 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=512 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=1024 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=2048 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=4096 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=64 --batch=8 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=128 --batch=8 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=256 --batch=8 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=512 --batch=8 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=1024 --batch=8 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=2048 --batch=8 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=4096 --batch=8 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

# (Y)
./unit_test --unit=node --key_type=float --node_type=data_node --function=insert --num_keys=64 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=data_node --function=insert --num_keys=128 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

./unit_test --unit=node --key_type=int --node_type=data_node --function=insert --num_keys=128 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin


# Dataset uniform
# result(QPS, failed ratio):
#                              32     64+8             128              256     512
# inner node(gap array)               1.391e7, 0     1.266e7, 0.0625  
# data node(dense array)              2.2198e7        

# =================================================================================================
# test inner node(gap array) and data node(dense array) query accuracy and performance
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=8 --batch=8 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=64 --batch=64 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=128 --batch=64 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=256 --batch=64 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=512 --batch=64 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=1024 --batch=64 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (N)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=2048 --batch=64 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=4096 --batch=64 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=64 --batch=64 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=128 --batch=64 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=256 --batch=64 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=512 --batch=64 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=1024 --batch=64 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=2048 --batch=64 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=4096 --batch=64 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=64 --batch=64 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=128 --batch=64 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=256 --batch=64 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=512 --batch=64 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=1024 --batch=64 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=2048 --batch=64 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=4096 --batch=64 --level=3 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

# (Y)
./unit_test --unit=node --key_type=float --node_type=data_node --function=query --num_keys=64 --batch=64 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=data_node --function=query --num_keys=128 --batch=64  --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

# Datset: uniform
# result(us):
#                                 64         128
# inner node(gap array)           1ms  3ms, 5.33e7
# data node(dense array)          9ms  2.8e2ms, 5.71e7

# =================================================================================================
# test inner node(gap array) and data node(dense array) erase accuracy and perfornmance
./unit_test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=16 --batch=8 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=64 --batch=8 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=128 --batch=8 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=256 --batch=8 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=512 --batch=8 --level=1 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=64 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=128 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=256 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=512 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=1024 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=2048 --batch=8 --level=2 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

# (X)
./unit_test --unit=node --key_type=float --node_type=data_node --function=erase --num_keys=64 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=node --key_type=float --node_type=data_node --function=erase --num_keys=128 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=node --key_type=float --node_type=data_node --function=erase --num_keys=256 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=node --key_type=float --node_type=data_node --function=erase --num_keys=512 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin

# =================================================================================================
# test inner node mixup
# (X)
./unit_test --unit=node --key_type=float --node_type=inner_node --function=mixup --num_keys=64 --batch=8 --iter=8 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin


# =================================================================================================
# test index SMO

# test data split
# (Y)
./unit_test --unit=SMO --key_type=float --function=data_split_with_exponential_probe --num_keys=200000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# (Y)
./unit_test --unit=SMO --key_type=float --function=data_split_with_linear_probe --num_keys=200000 --input_file=/home/zzr/data/longitudes-200M.bin.data

# (Y)
./unit_test --unit=SMO --key_type=float --function=data_split_with_exponential_probe --num_keys=2000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# (Y)
./unit_test --unit=SMO --key_type=float --function=data_split_with_linear_probe --num_keys=2000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

# (Y)
./unit_test --unit=SMO --key_type=float --function=data_split_with_exponential_probe --num_keys=20000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# (Y)
./unit_test --unit=SMO --key_type=float --function=data_split_with_linear_probe --num_keys=20000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

# Result: (node size, time, NPS, ml rate)
# exp_probe(2*log(n)): 1343, 589302 ms, 3.39e07
# linear_probe(2*log(n)): 7135, 986474 ms, 2.02e7
# linear_probe(4*log(n)): 2072, 964832 ms, 2.07e7
# linear_probe(8): 23035, 1.50364e6 ms, 1.33e7, 0.67

# (Y)
./unit_test --unit=SMO --key_type=int --function=data_split_with_linear_probe --num_keys=1000000 --input_file=/home/zzr/data/generate_data/id_1M_int.bin
# linear_probe(8): 1, 246014ms. 4.06e7

# (Y)
./unit_test --unit=SMO --key_type=float --function=data_split_with_exponential_probe --num_keys=1000000 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=SMO --key_type=float --function=data_split_with_linear_probe --num_keys=1000000 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# Result: (node size, time, NPS, ml rate)
# exp_probe(2*log(n)): 102, 289789 ms, 3.45e7
# linear_probe(2*log(n)): 2862, 484237ms, 2.06e7
# linear_probe(4*log(n)): 475, 484237ms, 2.16e7
# linear_probe(8): 11095, 743938 ms, 1.334e7, 0.700

# test inner node split
# (Y)
./unit_test --unit=SMO --key_type=float --function=node_split --num_keys=2048 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=SMO --key_type=float --function=node_split --num_keys=200000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=SMO --key_type=float --function=node_split --num_keys=2000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=SMO --key_type=float --function=node_split --num_keys=20000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# Result: (node size, time, NPS)
# level1: 
# level2: 64, 1.28e7, 1.55e6


./unit_test --unit=SMO --key_type=int --function=node_split --num_keys=200000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=SMO --key_type=int --function=node_split --num_keys=2000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=SMO --key_type=int --function=node_split --num_keys=20000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

# =================================================================================================
# test index construction accuracy and performance
# (Y)
./unit_test --unit=index --key_type=float --function=bulk_load --num_keys=2000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=bulk_load --num_keys=20000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=bulk_load --num_keys=200000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# Result:
# linear + static data node + no balance:

# piecewise_linear + static data node + no balance:
# 2000000: 6.251e+06ms, OPS=1.6
# 20000000: 6.711e+07ms, OPS=0.15
# 200000000:

# piecewise_linear + dynamic data node + RW balance:
# 2000000: 
# 20000000: 
# 200000000:

# test index lookup accuracy
./unit_test --unit=index --key_type=float --function=lookup --num_keys=2000000 --batch=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=lookup --num_keys=20000000 --batch=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=lookup --num_keys=200000000 --batch=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# Result:
# piecewise_linear + static data node + no balance:
# 2000000 + 65536: 1.42e+05ms, QPS=4.6e6
# 20000000 + 65536: 1.611e+05ms, QPS=4.068e+06
# 200000000 + 65536: 2.119e+05ms, QPS=3.093e+06



# test index insert accuracy
./unit_test --unit=index --key_type=float --function=insert --num_keys=2000 --batch=1 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=insert --num_keys=2000000 --batch=65536 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=insert --num_keys=20000000 --batch=65536 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=insert --num_keys=200000000 --batch=65536 --input_file=/home/zzr/data/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float --function=insert --num_keys=1000000 --batch=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float --function=insert --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float --function=insert --num_keys=2000000 --batch=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=insert --num_keys=2000000 --batch=2000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# Result:
# piecewise_linear + static data node + no balance:


# test index erase accuracy
./unit_test --unit=index --key_type=float --function=erase --num_keys=2000000 --batch=1 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=erase --num_keys=2000000 --batch=200000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=erase --num_keys=2000000 --batch=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=erase --num_keys=20000000 --batch=2000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=erase --num_keys=200000000 --batch=20000000 --input_file=/home/zzr/data/longitudes-200M.bin.data

# test index range query accuracy
./unit_test --unit=index --key_type=float --function=range_query --num_keys=2000000 --batch=1 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=range_query --num_keys=2000000 --batch=1000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=range_query --num_keys=20000000 --batch=1000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=range_query --num_keys=200000000 --batch=1000 --input_file=/home/zzr/data/longitudes-200M.bin.data

# test index read/write accuracy
#./unit_test --unit=index --key_type=float --function=mix --num_keys=2000000 --batch=2 --read_ratio=0.5 --input_file=/home/zzr/data/longitudes-200M.bin.data
#./unit_test --unit=index --key_type=float --function=mix --num_keys=2000000 --batch=20000 --read_ratio=0.5 --input_file=/home/zzr/data/longitudes-200M.bin.data
#./unit_test --unit=index --key_type=float --function=mix --num_keys=20000000 --batch=200000 --read_ratio=0.5 --input_file=/home/zzr/data/longitudes-200M.bin.data
#./unit_test --unit=index --key_type=float --function=mix --num_keys=200000000 --batch=2000000 --read_ratio=0.5 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=1000000 --erase_nums=1000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=1000000 --erase_nums=0 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=tot --num_keys=20000000 --read_nums=20000000 --write_nums=10000000 --erase_nums=10000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float --function=tot --num_keys=200000000 --read_nums=200000000 --write_nums=100000000 --erase_nums=100000000 --input_file=/home/zzr/data/longitudes-200M.bin.data


# test index balance accuracy
./unit_test --unit=index --insert_balance=1 --key_type=float --function=lookup --num_keys=2000000 --batch=65536 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --insert_balance=1 --key_type=float --function=lookup --num_keys=20000000 --batch=65536 --input_file=/home/zzr/data/longitudes-200M.bin.data
./unit_test --unit=index --insert_balance=1 --key_type=float --function=lookup --num_keys=200000000 --batch=65536 --input_file=/home/zzr/data/longitudes-200M.bin.data

# test index concurrancy
./unit_test --unit=index_con --key_type=float --function=rw --num_keys=2000000 --batch=200000 --read_ratio=0.5 --input_file=/home/zzr/data/longitudes-200M.bin.data

# test index find performance
./benchmark --key_type=int --num_keys=1000000 --function=query --index=aex --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin

# test index insert performance

# test index erase performance

# test index RW(0.5:0.5) performance

# test index memory performace

# test index 

# =================================================================================================

# test tree balance




# =================================================================================================
# test all (need much time)
# bash test/test.sh