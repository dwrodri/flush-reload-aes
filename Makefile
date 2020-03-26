CC=gcc
OPENSSL_DIR=<path to your openssl root>
LDPATH=-L$(OPENSSL_DIR)
LDLIB=-lcrypto
CFLAGS=-I$(OPENSSL_DIR)include/ -ldl
all: attacker victim analysis

attacker: attacker.c
	$(CC) attacker.c -o attacker $(LDLIB) $(LDPATH) $(CFLAGS)

victim: victim.c
	$(CC) victim.c -o victim $(LDLIB) $(LDPATH) $(CFLAGS)

analysis: analysis.c
	$(CC) analysis.c -o analysis

clean:
	rm -rf  victim attacker analysis