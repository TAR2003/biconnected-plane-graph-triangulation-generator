./build/bench_total --algo=biconnected --cases=biconnected

# Oneconnected algorithm on Biconnected inputs
./build/bench_total --algo=oneconnected --cases=biconnected

# Oneconnected algorithm on Oneconnected inputs
./build/bench_total --algo=oneconnected --cases=oneconnected


# Per-triangulation timing, Oneconnected algorithm, all inputs, 5k limit
./build/bench_individual --algo=oneconnected --limit=5000

# Per-triangulation timing, Oneconnected algorithm, all inputs, 5k limit
./build/bench_individual --algo=biconnected --cases=biconnected --limit=5000