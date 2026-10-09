rm -rf input/
mkdir input
mkdir input/Biconnected
mkdir input/Oneconnected
cp -r ../dev/input/Biconnected/9_general_biconnected ./input/Biconnected/
cp -r ../dev/input/Oneconnected/* ./input/Oneconnected/
python main.py
python gen_biconnected.py --out input/Biconnected --sizes 8 16 32 64 128 256 512 1024 2048 4096    --count 3 --seed 1

ython gen_oneconnected.py --out input/Oneconnected --sizes 8 16 32 64 128 256 512 1024 2048 4096   --count 3 --seed 1


# python gen_oneconnected.py --out input/Oneconnected --sizes 10 20 30 40 50 60 70 80 90 100 200 300 400 500 600 700 800 900 100 2000 3000 4000 5000 6000 7000 8000 9000 10000 20000 30000 40000 50000 60000 70000 80000 90000 100000 --count 2 --seed 1

# sudo apt update && sudo apt install -y build-essential g++ make wget && \
# wget https://users.cecs.anu.edu.au/~bdm/plantri/plantri52.tar.gz && \
# tar -xzf plantri52.tar.gz && \
# cd plantri52 && \
# gcc -O3 -o plantri plantri.c && \
# mv plantri .. && \
# cd .. && \
# rm -rf plantri52 plantri52.tar.gz 


# python plantri_import.py --plantri ./plantri --kind biconnected  --nmin 4 --nmax 7 --out input/Biconnected/Biconnected_plantri --seed 42
# python plantri_import.py --plantri ./plantri --kind oneconnected --nmin 3 --nmax 7  --out input/Oneconnected/Oneconnected_plantri --seed 42

# rm -rf plantri

#  python gen_biconnected.py --out input/Biconnected --sizes 10 20 30 40 50 100 200 300 400 500 1000 2000 3000 4000 5000 10000 20000 30000 40000 50000 100000 200000 300000 400000 500000 1000000 --count 1 --seed 1