#/bin/bash

trap 'kill $(jobs -p)' SIGINT

workloads="./workloads/workloada.spec ./workloads/workloadb.spec ./workloads/workloadc.spec ./workloads/workloadd.spec ./workloads/workloadf.spec"

for file_name in $workloads; do
  echo "Running Redis with for $file_name"
  ./ycsbc -db redis -threads 1 -P $file_name -threads $1 -host 10.10.10.1 -port 6379 -slaves 0 -lat 2>>res/ycsbc_t$1_lat.output &
  wait
done