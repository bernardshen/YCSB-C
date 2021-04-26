#/bin/bash

trap 'kill $(jobs -p)' SIGINT

workloads="./workloads/workloada.spec ./workloads/workloadb.spec ./workloads/workloadc.spec ./workloads/workloadd.spec ./workloads/workloadf.spec"
threads = $2
table = $1

for file_name in $workloads; do
  echo "Running Redis with for $file_name"
#   export LD_LIBRARY_PATH=/home/sjc/learn/learn-kv/mykv/build/src:$LD_LIBRARY_PATH
  echo ./ycsbc -db myKV -threads $2 -P $file_name -table $1
  ./ycsbc -db myKV -threads $2 -P $file_name -table $1 -lat 2>> res/myKV_$1_t$2_lat.output &
  wait
done