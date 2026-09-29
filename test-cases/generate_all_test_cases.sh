python main.py
python gen_biconnected.py --out input/Biconnected --seed 42
python gen_oneconnected.py --out input/Oneconnected --seed 42
python plantri_import.py --plantri ./plantri --kind biconnected  --nmin 4 --nmax 9 --out input/Biconnected/plantri_bicon --seed 42
python plantri_import.py --plantri ./plantri --kind oneconnected --nmin 3 --nmax 7  --out input/Oneconnected/plantri_onecon --seed 42