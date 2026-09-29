sudo apt update && sudo apt install -y build-essential g++ make wget && \
wget https://users.cecs.anu.edu.au/~bdm/plantri/plantri52.tar.gz && \
tar -xzf plantri52.tar.gz && \
cd plantri52 && \
gcc -O3 -o plantri plantri.c && \
mv plantri .. && \
cd .. && \
rm -rf plantri52 plantri52.tar.gz 