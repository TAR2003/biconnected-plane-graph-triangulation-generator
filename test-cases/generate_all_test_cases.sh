rm -rf input/
python main.py
python gen_biconnected.py --out input/Biconnected --seed 42
python gen_oneconnected.py --out input/Oneconnected --seed 42

# sudo apt update && sudo apt install -y build-essential g++ make wget && \
wget https://users.cecs.anu.edu.au/~bdm/plantri/plantri52.tar.gz && \
tar -xzf plantri52.tar.gz && \
cd plantri52 && \
gcc -O3 -o plantri plantri.c && \
mv plantri .. && \
cd .. && \
rm -rf plantri52 plantri52.tar.gz 


python plantri_import.py --plantri ./plantri --kind biconnected  --nmin 4 --nmax 8 --out input/Biconnected/Biconnected_plantri --seed 42
python plantri_import.py --plantri ./plantri --kind oneconnected --nmin 3 --nmax 8  --out input/Oneconnected/Oneconnected_plantri --seed 42

rm -rf plantri