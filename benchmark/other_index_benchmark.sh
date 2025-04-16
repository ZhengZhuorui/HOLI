# ALEX experiment

./build/benchmark \
--keys_file=/home/zzr/data/longitudes-200M.bin.data \
--keys_file_type=binary \
--init_num_keys=20000000 \
--total_num_keys=20000000 \
--batch_size=500000 \
--insert_frac=0 \
--lookup_distribution=uniform \
--print_batch_stats
# Result

1.571e07 lookups/sec

./build/benchmark \
--keys_file=/home/zzr/data/longitudes-200M.bin.data \
--keys_file_type=binary \
--init_num_keys=10000000 \
--total_num_keys=20000000 \
--batch_size=1000000 \
--insert_frac=0.5 \
--lookup_distribution=uniform \
--print_batch_stats