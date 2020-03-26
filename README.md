# Lab 5

## OpenSSL library

```bash
git clone https://github.com/openssl/openssl.git
cd openssl
./config no-hw no-asm
make -j 4
```

## Find the offsets

```bash
readelf -a openssl/libcrypto.so > libcrypto.S
```

In `libcrypto.S` find

```text
    73: 0000000000256720  1024 OBJECT  LOCAL  DEFAULT   15 Te0
    74: 0000000000256320  1024 OBJECT  LOCAL  DEFAULT   15 Te1
    75: 0000000000255f20  1024 OBJECT  LOCAL  DEFAULT   15 Te2
    76: 0000000000255b20  1024 OBJECT  LOCAL  DEFAULT   15 Te3
```

## Attack

Fellow the instruction Modify the `attacker.c`. Modify `OPENSSL_DIR` in `Makefile`.

Then to attack Te0:

```bash
openssl_dir=<path to openssl root>
make
export LD_LIBRARY_PATH=$openssl_dir
./victim &
./attacker -t timing.bin -c cipher.bin -o 256720 -v $openssl_dir/libcrypto.so
./analysis -t timing.bin -c cipher.bin -o analysis.txt
python plot_analysis.py analysis.txt
killall victim
```
