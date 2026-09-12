#include "../lib/benchmark/common.hpp"
#include "../include/triangulation/one_connected/biconnected.hpp"
#include "../lib/benchmark/aggregate_driver.hpp"

int main(int argc, char *argv[])
{
    const string inputRoot = "input";
    return benchmark::runAggregateMain<biconnected, true>(
        argc, argv, inputRoot,
        "ONE-CONNECTED ON BICONNECTED BENCHMARK - per-category, resumable per-run");
}
