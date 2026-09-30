./build/bench_total --algo=biconnected --cases=biconnected

# Biconnected algorithm variant without VGS maintenance on Biconnected inputs
./build/bench_total --algo=biconnected-without-vgs --cases=biconnected

# Oneconnected algorithm on Biconnected inputs
./build/bench_total --algo=oneconnected --cases=biconnected

# Oneconnected algorithm on Oneconnected inputs
./build/bench_total --algo=oneconnected --cases=oneconnected


# Per-triangulation timing, Oneconnected algorithm, all inputs, 5k limit
./build/bench_individual --algo=oneconnected

# Per-triangulation timing, Oneconnected algorithm, all inputs, 5k limit
./build/bench_individual --algo=biconnected --cases=biconnected

# Per-triangulation timing, BiconnectedWithoutVGS algorithm, Biconnected inputs
./build/bench_individual --algo=biconnected-without-vgs --cases=biconnected 