DATA_PATH="/data/zzr/learned_index/data"

# =================================================================================================
# build time 
# example: ./benchmark --key_type=float64 --index=stx_btree --function=construct --num_keys=20000 --input_file=${DATA_PATH}/longitudes-200M.bin.data


./benchmark --key_type=float64 --index=aex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
# Dataset: longtitudes
# size: 200M
# batch: 65536
# 

# =================================================================================================
# construct
./benchmark --key_type=float64 --index=aex --function=construct --num_keys=2000000  --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=construct --num_keys=2000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=construct --num_keys=2000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=construct --num_keys=2000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=construct --num_keys=2000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

./benchmark --key_type=float64 --index=aex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=construct--num_keys=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=hash --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

./benchmark --key_type=uint64 --index=aex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/fb_200M_uint64

./benchmark --key_type=uint64 --index=aex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64

./benchmark --key_type=uint64 --index=aex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=alex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=pgm --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=lipp --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64

./benchmark --key_type=uint64 --index=aex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=construct --num_keys=200000000 --input_file=${DATA_PATH}/books_200M_uint64

# =================================================================================================
# lookup

# example
./benchmark --key_type=float64 --index=aex --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=lookup --query_dis=uniform --num_keys=200 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

./benchmark --key_type=uint64 --index=aex --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=alex --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=pgm --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=lipp --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/ycsb-200M.bin.data

#longtitude
./benchmark --key_type=float64 --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=hash --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

# piecewise model1 + no avx + segment num=4 

# longlat
./benchmark --key_type=float64 --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longlat-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longlat-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longlat-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longlat-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longlat-200M.bin.data
./benchmark --key_type=float64 --index=hash --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longlat-200M.bin.data

# uniform

# normal
./benchmark --key_type=float64 --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=search --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

#ycsb
./benchmark --key_type=uint64 --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=pgm --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=uint64 --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

# lognormal
./benchmark --key_type=float64 --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=float64 --index=pgm --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

# facebook
./benchmark --key_type=uint64 --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=search --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64

# wiki
./benchmark --key_type=uint64 --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64

# osm
./benchmark --key_type=uint64 --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=pgm --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=lipp --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64

# book
./benchmark --key_type=uint64 --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=10000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=10000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=10000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=10000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=10000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=lookup --query_dis=uniform --num_keys=2000000 --num_ops=10000000 --input_file=${DATA_PATH}/books_200M_uint64

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
./benchmark --key_type=float64 --index=aex --function=delta_lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=delta_lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=delta_lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=delta_lookup --query_dis=uniform --num_keys=200 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=delta_lookup --query_dis=uniform --num_keys=2000000 --num_ops=100000 --input_file=${DATA_PATH}/longitudes-200M.bin.data


./benchmark --key_type=float64 --index=aex --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=hash --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

./benchmark --key_type=float64 --index=aex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=hash --function=lookup --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

./benchmark --key_type=uint64 --index=pgm --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=10000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=10000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=10000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=pgm --function=delta_lookup --query_dis=uniform --num_keys=200000000 --num_ops=10000000 --input_file=${DATA_PATH}/books_200M_uint64

# =================================================================================================
# insert
# example
./benchmark --key_type=float64 --index=aex --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=pgm --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=insert --query_dis=uniform --num_keys=2000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data


# longtitude
./benchmark --key_type=float64 --index=aex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=pgm --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

# ycsb
./benchmark --key_type=uint64 --index=aex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=stl_map --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=stx_btree --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=alex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=lipp --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=hash --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/ycsb-200M.bin.data

# lognormal 
./benchmark --key_type=float64 --index=aex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/lognormal_200M_0_1_float.bin
./benchmark --key_type=float64 --index=stl_map --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/lognormal_200M_0_1_float.bin
./benchmark --key_type=float64 --index=stx_btree --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/lognormal_200M_0_1_float.bin
./benchmark --key_type=float64 --index=alex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/lognormal_200M_0_1_float.bin
./benchmark --key_type=float64 --index=lipp --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/lognormal_200M_0_1_float.bin
./benchmark --key_type=float64 --index=hash --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/lognormal_200M_0_1_float.bin

# facebook
./benchmark --key_type=uint64 --index=aex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=10000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=10000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=hash --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64

# wiki
./benchmark --key_type=uint64 --index=aex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=hash --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/wiki_ts_200M_uint64

# Osmc
./benchmark --key_type=uint64 --index=aex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=alex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=pgm --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=lipp --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=hash --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/osm_cellids_800M_uint64

# Amzn
./benchmark --key_type=uint64 --index=aex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=hash --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/books_200M_uint64

# =================================================================================================
# all insert
./benchmark --key_type=float64 --index=aex --function=insert --num_keys=2000000 --num_ops=2000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=insert --num_keys=2000000 --num_ops=2000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=insert --num_keys=2000000 --num_ops=2000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=insert --num_keys=2000000 --num_ops=2000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=pgm --function=insert --num_keys=2000000 --num_ops=2000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=insert --num_keys=2000000 --num_ops=2000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

# longtitude
./benchmark --key_type=float64 --index=aex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=pgm --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=hash --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data

#facebook
./benchmark --key_type=uint64 --index=aex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=hash --function=insert --query_dis=uniform --num_keys=200000000 --num_ops=200000000 --input_file=${DATA_PATH}/fb_200M_uint64


# =================================================================================================
# erase
# ./benchmark --key_type=uint64 --index=aex --function=erase --query_dis=uniform --num_keys=2000000 --num_ops=10000 --input_file=${DATA_PATH}/fb_200M_uint64

# longtitude
./benchmark --key_type=float64 --index=aex --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=pgm --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/longitudes-200M.bin.data


./benchmark --key_type=uint64 --index=aex --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=hash --function=erase --query_dis=uniform --num_keys=200000000 --num_ops=1000000 --input_file=${DATA_PATH}/fb_200M_uint64

# =================================================================================================
# range query
# example:
./benchmark --key_type=float64 --index=aex --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=pgm --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=${DATA_PATH}/longitudes-200M.bin.data

./benchmark --key_type=uint64 --index=aex --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.1 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=stl_map --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.1 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=stx_btree --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.1 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=alex --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.1 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=pgm --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.1 --input_file=${DATA_PATH}/ycsb-200M.bin.data
./benchmark --key_type=uint64 --index=lipp --function=range_query --query_dis=uniform --num_keys=2000000 --num_ops=10000 --length_ratio=0.001 --input_file=${DATA_PATH}/ycsb-200M.bin.data

# longtitude
./benchmark --key_type=float64 --index=aex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stl_map --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=stx_btree --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=alex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=pgm --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/longitudes-200M.bin.data
./benchmark --key_type=float64 --index=lipp --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/longitudes-200M.bin.data

# facebook
./benchmark --key_type=uint64 --index=aex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/fb_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=range_query --query_dis=uniform --num_keys=200000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/fb_200M_uint64

# wiki
./benchmark --key_type=uint64 --index=aex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/wiki_ts_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/wiki_ts_200M_uint64


# osm
./benchmark --key_type=uint64 --index=aex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=alex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=pgm --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/osm_cellids_800M_uint64
./benchmark --key_type=uint64 --index=lipp --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/osm_cellids_800M_uint64

# book
./benchmark --key_type=uint64 --index=aex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=stl_map --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=stx_btree --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=alex --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=pgm --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/books_200M_uint64
./benchmark --key_type=uint64 --index=lipp --function=range_query --query_dis=uniform --num_keys=200000000 --num_ops=100 --length_ratio=0.1 --input_file=${DATA_PATH}/books_200M_uint64

# =================================================================================================
# read + write mix


