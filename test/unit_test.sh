cmake -DCMAKE_BUILD_TYPE=Debug ..

# generate dataset

./generate_dataset --prime --key=1000000000

# (Y)
./generate_dataset --key_type=uint64  --num_keys=1000000   --distribution=uniform   --output_file=/home/zzr/data/learned_index/generate_data/uniform_1M_int.bin --lower=-1000000000000 --upper=1000000000000

# (Y)
./generate_dataset --key_type=uint64  --num_keys=1000      --distribution=uniform   --output_file=/home/zzr/data/learned_index/generate_data/uniform_1K_int.bin --lower=-1000000000000 --upper=100000000000

# (Y)
./generate_dataset --key_type=float64 --num_keys=1000      --distribution=uniform   --output_file=/home/zzr/data/learned_index/generate_data/uniform_1K_neg100to100_float.bin --lower=-100 --upper=100

# (Y)
./generate_dataset --key_type=float64 --num_keys=1000000   --distribution=uniform   --output_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin --lower=-100 --upper=100

# (Y)
./generate_dataset --key_type=float64 --num_keys=1000      --distribution=normal    --output_file=/home/zzr/data/learned_index/generate_data/normal_1K_0_1_float.bin --mean=0 --stddev=1

./generate_dataset --key_type=float64 --num_keys=200000000 --distribution=normal    --output_file=/home/zzr/data/learned_index/generate_data/normal_200M_0_1_float.bin --mean=0 --stddev=1

# (N)
./generate_dataset --key_type=float64 --num_keys=200000000 --distribution=lognormal --output_file=/home/zzr/data/learned_index/generate_data/lognormal_200M_0_1_float.bin --mean=0 --stddev=1

# (Y)
./generate_dataset --key_type=uint64  --num_keys=1000000   --distribution=id_ascend --output_file=/home/zzr/data/learned_index/generate_data/id_1M_int.bin

./generate_dataset --key_type=uint64 --num_keys=1000000 --distribution=multikey --output_file=/home/zzr/data/learned_index/generate_data/multikey_1M_int.bin
./generate_dataset --key_type=uint64 --num_keys=200000000 --distribution=multikey --output_file=/home/zzr/data/learned_index/generate_data/multikey_200M_int.bin

./unit_test --unit=sort --key_type=uint64 --num_keys=200000000 --input_file=/home/zzr/data/learned_index/genome --output_file=/home/zzr/data/learned_index/genome_200M_uint64
./unit_test --unit=sort --key_type=uint64 --num_keys=200000000 --input_file=/home/zzr/data/learned_index/covid --output_file=/home/zzr/data/learned_index/covid_200M_uint64
./unit_test --unit=sort --key_type=uint64 --num_keys=200000000 --input_file=/home/zzr/data/learned_index/planet --output_file=/home/zzr/data/learned_index/planet_200M_uint64

./unit_test --key_type=uint64 --num_keys=200000000 --input_file=/home/zzr/data/learned_index/genome
./unit_test --key_type=uint64 --num_keys=200000000 --input_file=/home/zzr/data/learned_index/genome_200M_uint64


# =================================================================================================
# test function using avx2
./unit_test --unit=avx --key_type=float64 --function=lower_bound_with_error_bound
./unit_test --unit=avx --key_type=uint64  --function=lower_bound_with_error_bound
./unit_test --unit=avx --key_type=int64   --function=lower_bound_with_error_bound
./unit_test --unit=avx --key_type=float32 --function=lower_bound_with_error_bound
./unit_test --unit=avx --key_type=uint32  --function=lower_bound_with_error_bound
./unit_test --unit=avx --key_type=int32   --function=lower_bound_with_error_bound


# =================================================================================================
# test find function(STL(bineary lower bound), ALEX(exponential find), exponential find)
# (Y)
./unit_test --unit=function --key_type=uint64 --num_keys=1000000 --function=exp_lower_bound --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_int.bin
# (Y)
./unit_test --unit=function --key_type=uint64 --num_keys=1000000 --function=exp_lower_bound --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=function --key_type=uint64 --num_keys=1000000 --function=search_perf --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_int.bin
# result(us):
# self exp lower bound used time=330974 us
# ALEX exp lower bound used time=775420 us
# STL lower bound used time=1301185 us
# (Y)
./unit_test --unit=function --key_type=uint64 --num_keys=1000000 --function=search_with_error_bound_perf --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_int.bin
./unit_test --unit=function --key_type=float64 --num_keys=1000000 --function=search_with_error_bound_perf --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_int.bin
# result(us):
# self exp lower bound used time=180819 us
# ALEX exp lower bound used time=233474 us
# STL lower bound used time=236411 us
# search used time=205381 us

# test linear probe (with exponential probe)
# (Y)
./unit_test --unit=function --key_type=uint64 --num_keys=1000000 --function=linear_probe --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_int.bin
# (Y)
./unit_test --unit=function --key_type=float64 --num_keys=1000 --function=linear_probe --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=function --key_type=float64 --num_keys=1000000 --function=linear_probe --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data



# result(ms, NPS(number per second))
# 50, 11ms, 4.54e7
# 173, 40ms, 4.325e7
# 85, 16ms, 5.32e7

# =================================================================================================
# test fitting model(linear, logarithmic, exponential, quandratic, gap array linear)
# (Y)
./unit_test --unit=model --key_type=uint64 --num_keys=1000 --model_type=linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1K_int.bin
# (Y)
./unit_test --unit=model --key_type=float64 --num_keys=1000 --model_type=linear --spec=1 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=model --key_type=float64 --num_keys=1000 --model_type=log --spec=1 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=model --key_type=float64 --num_keys=1000 --model_type=exp --spec=1 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=model --key_type=float64 --num_keys=1000 --model_type=quad --spec=1 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# Dataset: uniform->corresponding distribution
# result(RMSE):
# linear: 5.99
# logarithmic: 91
# exponential: ~
# quandratic: 4

#(Y)
./unit_test --unit=model --key_type=float64 --num_keys=1000 --model_type=linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

#(Y)
./unit_test --unit=model --key_type=float64 --num_keys=1000 --model_type=log --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

#(Y)
./unit_test --unit=model --key_type=float64 --num_keys=1000 --model_type=exp --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

#(Y)
./unit_test --unit=model --key_type=float64 --num_keys=1000 --model_type=quad --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

#(Y)
./unit_test --unit=model --key_type=float64 --num_keys=1000 --model_type=gap_linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# Dataset: uniform
# result(RMSE):
# linear: 5.99
# logarithmic: 129
# exponential: 128
# quandratic: 5
# gap linear: 10.35

# test gap array fitting model(linear, gap array linear)
#(Y)
./unit_test --unit=model --key_type=float64 --num_keys=128 --model_type=linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
#(Y)
./unit_test --unit=model --key_type=float64 --num_keys=128 --model_type=gap_linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# Dataset: uniform
# result(max error):
# linear: 3
# gap linear: 5

#(Y)
./unit_test --unit=model --key_type=float64 --num_keys=1024 --model_type=linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
#(Y)
./unit_test --unit=model --key_type=float64 --num_keys=1024 --model_type=gap_linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
#(Y)
./unit_test --unit=model --key_type=float64 --num_keys=256 --level=2 --model_type=piecewise_linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=model --key_type=float64 --num_keys=1024 --model_type=piecewise_linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=model --key_type=float64 --num_keys=4096 --level=2 --model_type=piecewise_linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=model --key_type=float64 --num_keys=65536 --level=2 --model_type=piecewise_linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

./unit_test --unit=model --key_type=float64 --num_keys=4096 --level=2 --model_type=piecewise_linear_2 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=model --key_type=float64 --num_keys=4096 --level=2 --model_type=piecewise_linear_2 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=model --key_type=float64 --num_keys=4096 --level=2 --model_type=piecewise_linear_2 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=model --key_type=uint64 --num_keys=4096 --model_type=piecewise_linear --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=model --key_type=uint64 --num_keys=4096 --model_type=piecewise_linear_2 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=model --key_type=float64 --num_keys=65536 --level=2 --model_type=piecewise_linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=model --key_type=float64 --num_keys=65536 --level=2 --model_type=piecewise_linear_2 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=model --key_type=float64 --num_keys=65536 --level=1 --model_type=piecewise_linear_3 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=model --key_type=float64 --num_keys=65536 --level=2 --model_type=piecewise_linear_3 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=model --key_type=float64 --num_keys=65536 --level=2 --model_type=PDM --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

./unit_test --unit=model --key_type=float64 --num_keys=1000000 --level=1 --model_type=PDM_hash_table --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

./unit_test --unit=model --key_type=uint64 --num_keys=1000000 --level=1 --model_type=PDM_hash_table --input_file=/home/zzr/data/learned_index/genome_200M_uint64


./unit_test --unit=model --key_type=float64 --num_keys=1000000 --level=2 --model_type=PDM_hash_table --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin


./unit_test --unit=model --key_type=float64 --num_keys=1000000 --level=2 --model_type=PDM_hash_table --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

./unit_test --unit=model --key_type=float64 --num_keys=1000000 --level=3 --model_type=PDM_hash_table --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

./unit_test --unit=model --key_type=float64 --num_keys=1000000 --level=1 --model_type=PDM_hash_table_AVX --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=model --key_type=float64 --num_keys=1000000 --level=2 --model_type=PDM_hash_table_AVX --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
./unit_test --unit=model --key_type=float64 --num_keys=1000000 --level=3 --model_type=PDM_hash_table_AVX --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

./unit_test --unit=model --key_type=uint64 --num_keys=1000000 --level=1 --model_type=PDM --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=model --key_type=uint64 --num_keys=1000000 --level=2 --model_type=PDM --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=model --key_type=uint64 --num_keys=1000000 --level=1 --model_type=PDM_hash_table --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=model --key_type=uint64 --num_keys=1000000 --level=2 --model_type=PDM_hash_table --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=model --key_type=uint64 --num_keys=1000000 --level=3 --model_type=PDM_hash_table --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=model --key_type=float64 --num_keys=65536 --level=2 --model_type=linear_hash_table --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

./unit_test --unit=model --key_type=float64 --num_keys=65536 --level=3 --model_type=linear_hash_table --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=model --key_type=float64 --num_keys=10000000 --batch=10000000 --level=1 --model_type=PDM --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=model --key_type=float64 --num_keys=10000000 --batch=10000000 --level=2 --model_type=PDM --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=model --key_type=float64 --num_keys=10000000 --batch=10000000 --level=2 --model_type=PDM_avx --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=model --key_type=uint64 --num_keys=10000000 --batch=10000000 --level=2 --model_type=piecewise_linear_1_avx --input_file=/home/zzr/data/learned_index/fb_200M_uint64

# Dataset: uniform
# size: 1024
# result(max error):
# linear: 9
# gap linear: 13
# piecewise_linear: 3

# give up radix-based tree

# (Y)
./unit_test --unit=model --key_type=float64 --num_keys=2000000 --model_type=linear --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
# (Y)
./unit_test --unit=model --key_type=float64 --num_keys=1000000 --model_type=linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=model --key_type=float64 --num_keys=2000000 --model_type=piecewise_linear --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=model --key_type=float64 --num_keys=2000000 --model_type=piecewise_linear --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

# =================================================================================================
# test inner node(few) (gap array) and data node(dense array) insertion accuracy and performance
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=insert --num_keys=64 --batch=8 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=insert --num_keys=128 --batch=16 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=insert --num_keys=256 --batch=32 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=insert --num_keys=512 --batch=64 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=insert --num_keys=1024 --batch=128 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

# (Y)
./unit_test --unit=node --key_type=float64 --node_type=data_node --function=insert --num_keys=64 --batch=8 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=data_node --function=insert --num_keys=128 --batch=8 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

./unit_test --unit=node --key_type=uint64 --node_type=data_node --function=insert --num_keys=128 --batch=8 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_int.bin


# Dataset uniform
# result(QPS, failed ratio):
#                              32     64+8             128              256     512
# inner node(gap array)               1.391e7, 0     1.266e7, 0.0625  
# data node(dense array)              2.2198e7        

# =================================================================================================
# test inner node(gap array) and data node(dense array) query accuracy and performance
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=query --num_keys=64 --batch=64  --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=query --num_keys=128 --batch=64  --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=query --num_keys=256 --batch=64 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=query --num_keys=512 --batch=64 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=query --num_keys=1024 --batch=64 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin


# (Y)
./unit_test --unit=node --key_type=uint64 --node_type=hash_node --function=query --num_keys=64 --batch=64  --input_file=/home/zzr/data/learned_index/fb_200M_uint64
# (Y)
./unit_test --unit=node --key_type=uint64 --node_type=hash_node --function=query --num_keys=128 --batch=64  --input_file=/home/zzr/data/learned_index/fb_200M_uint64
# (Y)
./unit_test --unit=node --key_type=uint64 --node_type=hash_node --function=query --num_keys=256 --batch=64 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
# (Y)
./unit_test --unit=node --key_type=uint64 --node_type=hash_node --function=query --num_keys=512 --batch=64 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=node --key_type=uint64 --node_type=hash_node --function=query --num_keys=1024 --batch=64 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

# (Y)
./unit_test --unit=node --key_type=float64 --node_type=data_node --function=query --num_keys=64 --batch=64 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=data_node --function=query --num_keys=128 --batch=64  --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin


# =================================================================================================
# test inner node(gap array) and data node(dense array) erase accuracy and perfornmance
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=erase --num_keys=32 --batch=4 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=erase --num_keys=64 --batch=8 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=erase --num_keys=128 --batch=16 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=erase --num_keys=256 --batch=32 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=node --key_type=float64 --node_type=hash_node --function=erase --num_keys=512 --batch=64 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin


# =================================================================================================
# test inner node mixup
# (X)
./unit_test --unit=node --key_type=float64 --node_type=inner_node --function=mixup --num_keys=64 --batch=8 --iter=8 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin

# =================================================================================================
# test index SMO

# test data split
# (Y)
./unit_test --unit=SMO --key_type=float64 --function=data_split_with_exponential_probe --num_keys=200000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
# (Y)
./unit_test --unit=SMO --key_type=float64 --function=data_split_with_linear_probe --num_keys=200000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

# (Y)
./unit_test --unit=SMO --key_type=float64 --function=data_split_with_exponential_probe --num_keys=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
# (Y)
./unit_test --unit=SMO --key_type=float64 --function=data_split_with_linear_probe --num_keys=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

# (Y)
./unit_test --unit=SMO --key_type=float64 --function=data_split_with_exponential_probe --num_keys=20000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
# (Y)
./unit_test --unit=SMO --key_type=float64 --function=data_split_with_linear_probe --num_keys=20000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

# Result: (node size, time, NPS, ml rate)
# exp_probe(2*log(n)): 1343, 589302 ms, 3.39e07
# linear_probe(2*log(n)): 7135, 986474 ms, 2.02e7
# linear_probe(4*log(n)): 2072, 964832 ms, 2.07e7
# linear_probe(8): 23035, 1.50364e6 ms, 1.33e7, 0.67

# (Y)
./unit_test --unit=SMO --key_type=uint64 --function=data_split_with_linear_probe --num_keys=1000000 --input_file=/home/zzr/data/learned_index/generate_data/id_1M_int.bin
# linear_probe(8): 1, 246014ms. 4.06e7

# (Y)
./unit_test --unit=SMO --key_type=float64 --function=data_split_with_exponential_probe --num_keys=1000000 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# (Y)
./unit_test --unit=SMO --key_type=float64 --function=data_split_with_linear_probe --num_keys=1000000 --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_neg100to100_float.bin
# Result: (node size, time, NPS, ml rate)
# exp_probe(2*log(n)): 102, 289789 ms, 3.45e7
# linear_probe(2*log(n)): 2862, 484237ms, 2.06e7
# linear_probe(4*log(n)): 475, 484237ms, 2.16e7
# linear_probe(8): 11095, 743938 ms, 1.334e7, 0.700

# test inner node split
# (Y)
./unit_test --unit=SMO --key_type=float64 --function=node_split --num_keys=2048 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=SMO --key_type=float64 --function=node_split --num_keys=200000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=SMO --key_type=float64 --function=node_split --num_keys=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=SMO --key_type=float64 --function=node_split --num_keys=20000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
# Result: (node size, time, NPS)
# level1: 
# level2: 64, 1.28e7, 1.55e6


./unit_test --unit=SMO --key_type=float64 --function=node_split --num_keys=200000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=SMO --key_type=float64 --function=node_split --num_keys=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=SMO --key_type=float64 --function=node_split --num_keys=20000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=SMO --key_type=float64 --function=node_rescale --num_keys=1024 --ratio=2.0 --level=2 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=SMO --key_type=float64 --function=node_rescale --num_keys=1024 --ratio=0.5 --level=2 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

# =================================================================================================
# test hash table performance
./unit_test --unit=hash_table --key_type=uint64 --con --thread_num=16 --num_keys=10000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=hash_table --key_type=uint64 --thread_num=1 --num_keys=10000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

# =================================================================================================
# test index construction accuracy and performance
# (Y)
./unit_test --unit=index --key_type=uint64 --function=bulk_load --num_keys=20000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=bulk_load --con --num_keys=20000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=bulk_load --multikey --num_keys=1000000 --input_file=/home/zzr/data/learned_index/generate_data/multikey_1M_int.bin

./unit_test --unit=index --key_type=float64 --function=bulk_load --num_keys=20000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=bulk_load --num_keys=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=bulk_load --num_keys=20000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=bulk_load --num_keys=200000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=uint64 --function=bulk_load --num_keys=200000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=bulk_load --num_keys=200000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=bulk_load --num_keys=200000000 --input_file=/home/zzr/data/learned_index/books_200M_uint64
# Result:
# linear + static data node + no balance:

# piecewise_linear + static data node + no balance:

# max segment num=8
# 2000000: 2.432e+05ms, OPS=4.1
# 20000000: 2.5727e+06ms, OPS=0.3958, 
# 200000000: 2.414e7ms, OPS=0.0414
# =================================
# max segment num=4
# 2000000: 
# 20000000: 1.478e6, OPS=0.676, height=3, inner node size=83
# 200000000: 1.292e7ms, 0.0774, inner node size=168


# piecewise_linear + dynamic data node + RW balance:
# 2000000: 
# 20000000: 
# 200000000:

# piecewise_linear_2 + static_data_node + no balance:
# 2000000: 
# 20000000: 5.240056e5ms, OPS=1.91, inner node size=93, height=3
# 200000000: 5.347e6ms, OPS=0.1870, inner node size=217
# =================================
# max segment num=8
# piecewise_linear_2 + static_data_node + no balance:
# longtitudes 200000000: 5.347e6ms, OPS=0.1870, inner node size=110, height:3
# fb: 

./unit_test --unit=index --key_type=uint64 --function=lookup --num_keys=20000 --batch=10000 --input_file=/home/zzr/data/learned_index/covid_200M_uint64


# test index lookup accuracy
./unit_test --unit=index --key_type=float64 --function=lookup --num_keys=2000000 --batch=100000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=lookup --con --num_keys=2000000 --batch=100000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=lookup --num_keys=2000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=lookup --num_keys=20000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=uint64 --function=lookup --num_keys=20000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64


./unit_test --unit=index --key_type=float64 --function=lookup --num_keys=200000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float64 --function=lookup --num_keys=2000000 --batch=100000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=uint64 --function=lookup --multikey --num_keys=1000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/generate_data/multikey_1M_int.bin
./unit_test --unit=index --key_type=uint64 --function=lookup --num_keys=200000000 --batch=20000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --con --key_type=uint64 --function=lookup --num_keys=200000000 --batch=20000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=index --con --key_type=uint64 --function=lookup --num_keys=30000000 --batch=30000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=index --key_type=uint64 --con --function=lookup --num_keys=300000 --batch=300000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --con --function=lookup --num_keys=200000000 --batch=20000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=lookup --num_keys=200000000 --batch=20000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=lookup --num_keys=200000000 --batch=20000000 --input_file=/home/zzr/data/learned_index/covid_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=lookup --num_keys=200000000 --batch=20000000 --input_file=/home/zzr/data/learned_index/planet_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=lookup --num_keys=200000000 --batch=20000000 --input_file=/home/zzr/data/learned_index/genome_200M_uint64

./unit_test --unit=index --key_type=uint64 --function=lookup --multikey --num_keys=200000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/generate_data/multikey_200M_int.bin

./unit_test --unit=index --key_type=float64 --function=delta_lookup --num_keys=2000000 --batch=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=delta_lookup --num_keys=20000000 --batch=20000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=delta_lookup --num_keys=200000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=uint64 --function=delta_lookup --num_keys=200000000 --batch=10000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
# Result:
# Dataset: longtitudes
# piecewise_linear + static data node + no balance:
# 2000000 + 100000:   1.198e+05ms, QPS=8.35e6
# 20000000 + 100000:  1.442e+05ms, QPS=6.933e+06
# 200000000 + 100000: 1.786e+05ms, QPS=5.6e+06

# piecewise_linear + static data node + no balance + avx:
# 2000000 + 100000:  
# 20000000 + 100000: 
# 200000000 + 100000:


# test index insert accuracy
./unit_test --unit=index --key_type=float64 --function=insert --num_keys=2000 --batch=1 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=uint64 --function=insert --multikey --num_keys=10000 --batch=10000 --input_file=/home/zzr/data/learned_index/generate_data/multikey_1M_int.bin
./unit_test --unit=index --key_type=uint64 --function=insert --multikey --num_keys=100000 --batch=100000 --input_file=/home/zzr/data/learned_index/generate_data/multikey_1M_int.bin
./unit_test --unit=index --key_type=uint64 --function=insert --multikey --num_keys=1000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/generate_data/multikey_1M_int.bin


./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=1000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/generate_data/id_1M_int.bin

./unit_test --unit=index --key_type=uint64 --function=delta_lookup --num_keys=1000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/generate_data/id_1M_int.bin


./unit_test --unit=index --key_type=float64 --function=insert --num_keys=20000 --batch=20000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=insert --num_keys=2000000 --batch=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --con --function=insert --num_keys=1000 --batch=1000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float64 --function=insert --num_keys=2000000   --batch=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=insert --num_keys=20000000  --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=insert --num_keys=200000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=insert --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=10000 --batch=10000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=index --key_type=float64 --function=insert --num_keys=105000000 --batch=105000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=100000000 --batch=5000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --con --key_type=uint64 --function=insert --num_keys=10000000 --batch=5000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --con --function=insert --num_keys=10000000 --batch=5000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=105000000 --batch=5000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=180000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --con --key_type=uint64 --function=insert --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --con --key_type=uint64 --function=insert --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --con --key_type=uint64 --function=insert --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64
./unit_test --unit=index --con --key_type=uint64 --function=insert --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64

./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=180000000 --input_file=/home/zzr/data/learned_index/planet_200M_uint64

./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=10000000 --batch=10000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=20000000 --batch=10000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64
./unit_test --unit=index --key_type=uint64 --con --function=insert --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64

./unit_test --unit=index --key_type=uint64 --con --function=insert --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/covid_200M_uint64

./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/covid_200M_uint64

./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64


./unit_test --unit=index --key_type=uint64 --con --function=insert --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64

./unit_test --unit=index --key_type=uint64 --con --function=insert --num_keys=1000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64

./unit_test --unit=index --key_type=uint64 --con --function=lookup --num_keys=1000000 --batch=10000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64

./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64


./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/books_200M_uint64

./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/wiki_ts_200M_uint64

./unit_test --unit=index --key_type=float64 --function=insert --num_keys=2000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float64 --function=insert --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float64 --function=insert --num_keys=2000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=insert --num_keys=2000000 --batch=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=uint64 --function=insert --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
# Result:
# piecewise_linear + static data node + no balance:

# test index hotspot
./unit_test --unit=index --key_type=float64 --function=insert_hotspot --num_keys=2000 --batch=1 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=uint64 --function=insert_hotspot --multikey --num_keys=10000 --batch=10000 --input_file=/home/zzr/data/learned_index/generate_data/multikey_1M_int.bin
./unit_test --unit=index --key_type=uint64 --function=insert_hotspot --multikey --num_keys=100000 --batch=100000 --input_file=/home/zzr/data/learned_index/generate_data/multikey_1M_int.bin
./unit_test --unit=index --key_type=uint64 --function=insert_hotspot --multikey --num_keys=1000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/generate_data/multikey_1M_int.bin


./unit_test --unit=index --key_type=float64 --function=insert_hotspot --num_keys=2000000 --batch=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float64 --function=insert_hotspot --num_keys=2000000   --batch=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=insert_hotspot --num_keys=20000000  --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=insert_hotspot --num_keys=200000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=insert_hotspot --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data


./unit_test --unit=index --key_type=uint64 --function=insert_hotspot --num_keys=2000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=index --key_type=uint64 --function=insert_hotspot --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64


./unit_test --unit=index --key_type=uint64 --function=insert_hotspot --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=insert_hotspot --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=insert_hotspot --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=insert_hotspot --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/books_200M_uint64

./unit_test --unit=index --key_type=uint64 --function=insert_hotspot --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/wiki_ts_200M_uint64

./unit_test --unit=index --key_type=float64 --function=insert_hotspot --num_keys=2000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float64 --function=insert_hotspot --num_keys=200000000 --batch=100000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float64 --function=insert_hotspot --num_keys=2000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=insert_hotspot --num_keys=2000000 --batch=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data


# test index erase accuracy
./unit_test --unit=index --key_type=float64 --function=erase --num_keys=2000000 --batch=1 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=erase --num_keys=200000 --batch=200000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=erase --num_keys=2000000 --batch=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float64 --function=erase --num_keys=128 --batch=96 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=erase --num_keys=2000000 --batch=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=erase --num_keys=2000000 --batch=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=erase --num_keys=20000000 --batch=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=erase --num_keys=200000000 --batch=20000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=erase --num_keys=200000000 --batch=200000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

# test index range query accuracy
./unit_test --unit=index --key_type=float64 --function=range_query --num_keys=2000000 --batch=1 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float64 --function=range_query --num_keys=20000000 --batch=10000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float64 --function=range_query --num_keys=200000000 --batch=1000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=uint64 --function=range_query --num_keys=2000000 --batch=100 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=index --con --key_type=uint64 --function=range_query --num_keys=200000000 --batch=200000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

# test index all interface
# all write:
./unit_test --unit=index --key_type=float64 --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=2000000 --erase_nums=0 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=uint64 --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=2000000 --erase_nums=0 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=index --key_type=uint64 --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=2000000 --erase_nums=0 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=index --key_type=uint64 --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=2000000 --erase_nums=0 --input_file=/home/zzr/data/learned_index/books_800M_uint64

./unit_test --unit=index --key_type=uint64 --function=tot --num_keys=200000 --read_nums=200000 --write_nums=200000 --erase_nums=0 --input_file=/home/zzr/data/learned_index/books_800M_uint64

./unit_test --unit=index --key_type=uint64 --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=2000000 --erase_nums=0 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64
./unit_test --unit=index --key_type=uint64 --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=2000000 --erase_nums=0 --input_file=/home/zzr/data/learned_index/wiki_ts_200M_uint64


./unit_test --unit=index --key_type=float64 --function=tot --num_keys=200000000 --read_nums=2000000 --write_nums=200000000 --erase_nums=0 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=uint64 --function=tot --num_keys=200000000 --read_nums=0 --write_nums=200000000 --erase_nums=0 --input_file=/home/zzr/data/learned_index/fb_200M_uint64
# half write:
./unit_test --unit=index --key_type=float64 --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=1000000 --erase_nums=0 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
# all read:
./unit_test --unit=index --key_type=float64 --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=0 --erase_nums=0 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
# all erase:
./unit_test --unit=index --key_type=float64 --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=0 --erase_nums=2000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

./unit_test --unit=index --key_type=float64 --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=1000000 --erase_nums=1000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=tot --num_keys=2000000 --read_nums=2000000 --write_nums=1000000 --erase_nums=0 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=tot --num_keys=20000000 --read_nums=20000000 --write_nums=10000000 --erase_nums=10000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --key_type=float64 --function=tot --num_keys=200000000 --read_nums=200000000 --write_nums=100000000 --erase_nums=100000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data


# test index balance accuracy
./unit_test --unit=index --insert_balance=1 --key_type=float64 --function=lookup --num_keys=2000000 --batch=65536 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --insert_balance=1 --key_type=float64 --function=lookup --num_keys=20000000 --batch=65536 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data
./unit_test --unit=index --insert_balance=1 --key_type=float64 --function=lookup --num_keys=200000000 --batch=20000000 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

# test index concurrancy
./unit_test --unit=index_con --key_type=float64 --function=rw --num_keys=2000000 --batch=200000 --read_ratio=0.5 --input_file=/home/zzr/data/learned_index/longitudes-200M.bin.data

# test index find performance
./benchmark --key_type=uint64 --num_keys=1000000 --function=query --index=aex --input_file=/home/zzr/data/learned_index/generate_data/uniform_1M_int.bin

# test index insert performance

# test index erase performance

# test index RW(0.5:0.5) performance

# test index memory performace

# test index 

# =================================================================================================

# test tree balance


# =================================================================================================
# test index concurrency
./unit_test --unit=index_con --con --key_type=float64 --function=tot --num_keys=20000000 --thread_num=16 --batch=2000000 --read_nums=2000000 --write_nums=1000000 --input_file=/home/zzr/data/learned_index/covid_200M_uint64

./unit_test --unit=index_con --con --key_type=uint64 --function=tot --num_keys=200000000 --thread_num=16 --batch=200000000 --read_nums=100000000 --write_nums=0 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64

./unit_test --unit=index_con --con --key_type=uint64 --function=tot --num_keys=200000000 --thread_num=16 --batch=200000000 --read_nums=100000000 --write_nums=100000000 --input_file=/home/zzr/data/learned_index/osm_cellids_200M_uint64

./unit_test --unit=index_con --con --key_type=uint64 --function=tot --num_keys=200000000 --thread_num=16 --batch=200000000 --read_nums=100000000 --write_nums=100000000 --input_file=/home/zzr/data/learned_index/fb_200M_uint64

./unit_test --unit=index_con --con --key_type=uint64 --function=tot --num_keys=200000000 --thread_num=16 --batch=200000000 --read_nums=100000000 --write_nums=100000000 --input_file=/home/zzr/data/learned_index/covid_200M_uint64

# =================================================================================================
# test all (need much time)
# bash test/test.sh

