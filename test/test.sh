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

# =================================================================================================
# test find function(STL(bineary lower bound), ALEX(exponential find), exponential find)
# (Y)
./test --unit=function --key_type=int --num_keys=1000000 --function=exp_lower_bound --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin
# (Y)
./test --unit=function --key_type=int --num_keys=1000000 --function=exp_lower_bound_perf --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin
# result(us):
# self exp lower bound used time=330974 us
# ALEX exp lower bound used time=775420 us
# STL lower bound used time=1301185 us

# test linear probe (with exponential probe)
# (Y)
./test --unit=function --key_type=int --num_keys=1000000 --function=linear_probe --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin
# (Y)
./test --unit=function --key_type=float --num_keys=1000 --function=linear_probe --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
# (Y)
./test --unit=function --key_type=float --num_keys=1000000 --function=linear_probe --input_file=/home/zzr/data/longitudes-200M.bin.data

# result(ms, NPS(number per second))
# 50, 11ms, 4.54e7
# 173, 40ms, 4.325e7
# 85, 16ms, 5.32e7

# =================================================================================================
# test fitting model(linear, logarithmic, exponential, quandratic, gap array linear)
# (Y)
./test --unit=model --key_type=int --num_keys=1000 --model_type=linear --input_file=/home/zzr/data/generate_data/uniform_1K_int.bin
# (Y)
./test --unit=model --key_type=float --num_keys=1000 --model_type=linear --spec=1 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
# (Y)
./test --unit=model --key_type=float --num_keys=1000 --model_type=log --spec=1 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
# (Y)
./test --unit=model --key_type=float --num_keys=1000 --model_type=exp --spec=1 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
# (Y)
./test --unit=model --key_type=float --num_keys=1000 --model_type=quad --spec=1 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
# Dataset: uniform->corresponding distribution
# result(RMSE):
# linear: 5.99
# logarithmic: 91
# exponential: ~
# quandratic: 4

#(Y)
./test --unit=model --key_type=float --num_keys=1000 --model_type=linear --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin

#(Y)
./test --unit=model --key_type=float --num_keys=1000 --model_type=log --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin

#(Y)
./test --unit=model --key_type=float --num_keys=1000 --model_type=exp --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin

#(Y)
./test --unit=model --key_type=float --num_keys=1000 --model_type=quad --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin

#(Y)
./test --unit=model --key_type=float --num_keys=1000 --model_type=gap_linear --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
# Dataset: uniform
# result(RMSE):
# linear: 5.99
# logarithmic: 129
# exponential: 128
# quandratic: 5
# gap linear: 10.35

# test gap array fitting model(linear, gap array linear)
#(Y)
./test --unit=model --key_type=float --num_keys=128 --model_type=linear --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
#(Y)
./test --unit=model --key_type=float --num_keys=128 --model_type=gap_linear --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
# Dataset: uniform
# result(max error):
# linear: 5
# gap linear: 3

#(Y)
./test --unit=model --key_type=float --num_keys=112 --model_type=linear --input_file=/home/zzr/data/generate_data/normal_1K_0_1_float.bin
#(Y)
./test --unit=model --key_type=float --num_keys=112 --model_type=gap_linear --input_file=/home/zzr/data/generate_data/normal_1K_0_1_float.bin
# Dataset: normal
# result(max error):
# linear: 10
# gap linear: 22
# give up radix-based tree

# (Y)
./test --unit=model --key_type=float --num_keys=2000000 --model_type=linear --input_file=/home/zzr/data/longitudes-200M.bin.data
# (Y)
./test --unit=model --key_type=float --num_keys=1000000 --model_type=linear --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin


# =================================================================================================
# test inner node(few) (gap array) and data node(dense array) insertion accuracy and performance
# (Y)
./test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=72 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
# (Y)
./test --unit=node --key_type=float --node_type=data_node --function=insert --num_keys=72 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
#(Y)
./test --unit=node --key_type=float --node_type=inner_node --function=insert --num_keys=144 --batch=16 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin

# Dataset uniform
# result(QPS, failed ratio):
#                              32     64             128              256     512
# inner node(gap array)               1.391e7, 0     1.266e7, 0.0625  
# data node(dense array)              2.2198e7        

# =================================================================================================
# test inner node(gap array) and data node(dense array) query accuracy and performance
# (Y)
./test --unit=node --key_type=float --node_type=inner_node --function=query --num_keys=64 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
# (Y)
./test --unit=node --key_type=float --node_type=data_node --function=query --num_keys=64 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
./test --unit=node --key_type=float --node_type=data_node --function=query --num_keys=128 --batch=16 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin

# Datset: uniform
# result(us):
#                                 64         128
# inner node(gap array)           too small
# data node(dense array)          too small

# =================================================================================================
# test inner node(gap array) and data node(dense array) erase accuracy and perfornmance
# (Y)
./test --unit=node --key_type=float --node_type=inner_node --function=erase --num_keys=64 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin
# (X)
./test --unit=node --key_type=float --node_type=data_node --function=erase --num_keys=64 --batch=8 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin

# =================================================================================================
# test inner node mixup
# (X)
./test --unit=node --key_type=float --node_type=inner_node --function=mixup --num_keys=64 --batch=8 --iter=8 --input_file=/home/zzr/data/generate_data/uniform_1K_neg100to100_float.bin


# =================================================================================================
# test index SMO

# test data split
# (Y)
./test --unit=SMO --key_type=float --function=data_split --num_keys=2000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# (Y)
./test --unit=SMO --key_type=float --function=data_split_with_linear_probe --num_keys=2000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# Result: (node size, time, NPS)
# exp_probe(2*log(n)): 1343, 589302 ms, 3.39e07
# linear_probe(2*log(n)): 7135, 986474 ms, 2.02e7
# linear_probe(4*log(n)): 2072, 964832 ms, 2.07e7

# (Y)
./test --unit=SMO --key_type=float --function=data_split --num_keys=1000000 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./test --unit=SMO --key_type=float --function=data_split_with_linear_probe --num_keys=1000000 --input_file=/home/zzr/data/generate_data/uniform_1M_neg100to100_float.bin
# Result: (node size, time, NPS)
# exp_probe(2*log(n)): 102, 289789 ms, 3.45e7
# linear_probe(2*log(n)): 2862, 484237ms, 2.06e7
# linear_probe(4*log(n)): 475, 484237ms, 2.16e7

# test inner node split
# (Y)
./test --unit=SMO --key_type=float --function=node_split --num_keys=2000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# Result: (node size, time, NPS)
# 645, 1.56e6 ms, 1.28e7

# =================================================================================================
# test index construction accuracy and performance
# ()
./test --unit=index --key_type=float --function=bulk_load --num_keys=2000000 --input_file=/home/zzr/data/longitudes-200M.bin.data
# Result:
# 

# test index insert accuracy

# test index erase accuracy

# test index bulk load accuracy

# test index read/write accuracy

# test index find performance
./benchmark --key_type=int --num_keys=1000000 --function=query --index=aex --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin

# test index insert performance

# test index erase performance

# test index RW(0.5:0.5) performance

# test index memory performace

# test index 

# =================================================================================================
# test all (need much time)
# bash test/test.sh