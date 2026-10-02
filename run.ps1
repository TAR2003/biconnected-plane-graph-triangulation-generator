$ErrorActionPreference = "Stop"

# Helper function to locate the bench_time executable
function Get-BenchExe($name) {
    $paths = @(
        ".\build\Release\$name.exe",
        ".\build\$name.exe",
        ".\build\Debug\$name.exe"
    )
    foreach ($path in $paths) {
        if (Test-Path $path) { return $path }
    }
    throw "Executable '$name' not found in build directory. Please run build.ps1 first."
}

$benchTime = Get-BenchExe "bench_time"

# Execute active benchmark command
& $benchTime --tri_algos=Biconnected `
             --tri_input=./test-cases/input/Biconnected `
             --tri_cpu=2 `
             --tri_csv=./benchmark-results/results_time.csv `
             --tri_limits=1,10,100,1000,10000,100000,10000000,100000000,1000000000