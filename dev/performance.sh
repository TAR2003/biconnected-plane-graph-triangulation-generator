rm -rf ./benchmark callgrind.out.*
g++ -O3 -g -fno-omit-frame-pointer -std=c++17 ./simpleTest.cpp -o ./benchmark
# sudo apt install -y valgrind
valgrind --tool=callgrind ./benchmark
callgrind_annotate callgrind.out.* | head -n 40