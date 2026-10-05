
find . -type f -name "case_manifest.txt" -delete

# ./build/bench_time --tri_algos=Biconnected           --tri_input=./test-cases/input/Biconnected --tri_cpu=2 --tri_csv=t_bic.csv &
# ./build/bench_time --tri_algos=BiconnectedWithoutVGS --tri_input=./test-cases/input/Biconnected --tri_cpu=4 --tri_csv=t_vgs.csv &
# ./build/bench_time --tri_algos=Oneconnected          --tri_input=./test-cases/input/Oneconnected --tri_cpu=6 --tri_csv=t_one.csv &
# wait



# # Run with custom command-line flags defined in your setup (e.g., input directory and limits)
# ./bench_time --benchmark_counters_tabular=true \
#              --benchmark_out=results_time.csv \
#              --benchmark_out_format=csv \
#              --input_dir=/path/to/input_data \
#              --limit=100



             
# ./build/bench_time --tri_algos=Biconnected          --tri_input=./input/Biconnected --tri_cpu=2 --tri_csv=./benchmark-results/results_time.csv --tri_limits=1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000

./build/bench_time --tri_algos=Biconnected          --tri_input=./input/Biconnected --tri_cpu=2 --tri_csv=./benchmark-results/results_time.csv --tri_limits=10000000

# ./build/bench_time --tri_algos=Biconnected          --tri_input=./test-cases/input/Biconnected --tri_cpu=2 --tri_csv=./benchmark-results/results_time.csv --tri_limits=1,10,100,1000,10000,100000,1000000,10000000

# ./build/bench_time --tri_algos=BiconnectedWithoutVGS          --tri_input=./test-cases/input/Biconnected --tri_cpu=2 --tri_csv=./benchmark-results/results_time.csv --tri_limits=1,10,100,1000,10000,100000,1000000,10000000

# ./build/bench_time --tri_algos=Biconnected          --tri_input=./test-cases/input/Biconnected --tri_cpu=2 --tri_csv=./benchmark-results/results_time.csv --tri_limits=1,10,100,1000,10000,100000,1000000,10000000