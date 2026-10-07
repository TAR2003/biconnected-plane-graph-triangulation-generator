rm -rf input/
python main.py
python gen_biconnected.py --out input/Biconnected --sizes 10 20 30 40 50 100 200 300 400 500 600 700 800 900 1000 --count 2 --seed 1
python gen_oneconnected.py --out input/Oneconnected --sizes 10 20 30 40 50 100 200 300 400 500 600 700 800 900 1000 --count 2 --seed 1

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