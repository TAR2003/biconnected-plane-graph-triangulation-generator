#include "../lib/benchmark/common.hpp"
#include "../include/triangulation/biconnected.hpp"
#include "../lib/benchmark/aggregate_driver.hpp"

int main(int argc, char *argv[])
{
    const string inputRoot = "input";
    return benchmark::runAggregateMain<biconnected, false>(
        argc, argv, inputRoot,
        "TRIANGULATION BENCHMARK - per-category, resumable per-run");
}
