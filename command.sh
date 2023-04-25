./generate_dataset --key_type=int --num_keys=1000000 --distribution=uniform --output_file=/home/zzr/data/generate_data/uniform_1M_int.bin --lower=-1000000000000 --upper=1000000000000

./test --unit=function --key_type=int --num_keys=1000000 --function=exp_find --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin

./benchmark --key_type=int --num_keys=1000000 --function=query --index=aex --input_file=/home/zzr/data/generate_data/uniform_1M_int.bin