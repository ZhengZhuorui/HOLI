import re
import sys
import os

DATA_PATH = "/home/zzr/data/learned_index"
bash_config_file = ["./test/unit_test_config.sh", "./benchmark/benchmark_config.sh"]
bash_file = ["./test/unit_test.sh", "./benchmark/benchmark.sh"]

for file_name, replace_file_name in zip(bash_config_file, bash_file):
    file = open(file_name, "r")
    s = file.read()
    s = re.sub("\${DATA_PATH}", DATA_PATH, s)
    file.close()
    #print(s)
    replace_file = open(replace_file_name, "w")
    replace_file.write(s)
    replace_file.close()

