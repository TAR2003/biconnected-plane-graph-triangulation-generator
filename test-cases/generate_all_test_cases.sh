rm -rf input/
python main.py
python gen_biconnected.py --out input/Biconnected --seed 42
python gen_oneconnected.py --out input/Oneconnected --seed 42
python plantri_import.py --plantri ./plantri --kind biconnected  --nmin 4 --nmax 9 --out input/Biconnected/Biconnected_plantri --seed 42
python plantri_import.py --plantri ./plantri --kind oneconnected --nmin 3 --nmax 10  --out input/Oneconnected/Oneconnected_plantri --seed 42