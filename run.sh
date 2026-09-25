# Oneconnected algorithm on Oneconnected inputs
./build/bench_total --algo=oneconnected --cases=oneconnected

# Oneconnected algorithm on Biconnected inputs
./build/bench_total --algo=oneconnected --cases=biconnected

# Biconnected algorithm on everything
./build/bench_total --algo=biconnected --cases=biconnected

# Per-triangulation timing, Oneconnected algorithm, all inputs, 5k limit
./build/bench_individual --algo=oneconnected --limit=5000

# Per-triangulation timing, Oneconnected algorithm, all inputs, 5k limit
./build/bench_individual --algo=biconnected --cases=biconnected --limit=5000