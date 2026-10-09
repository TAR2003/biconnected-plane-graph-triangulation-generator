
find . -type f -name "case_manifest.txt" -delete

# ./build/bench_time --tri_algos=BiconnectedWithoutVGS,BiconnectedWithVGS,Oneconnected --tri_input=./input --tri_cpu=2 --tri_csv=./benchmark-results/results_time.csv --tri_limits=10000000


./build/bench_time --tri_algos=BiconnectedWithoutVGS,BiconnectedWithVGS --tri_input=./test-cases/input --tri_cpu=2 --tri_csv=./benchmark-results/results_time.csv --tri_limits=1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824