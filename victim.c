#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/aes.h>

unsigned char key[16] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f
};

static __inline__ void maccess(void *p) {
  asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");
}

int main(int argc, char** argv)
{
    struct sockaddr_in server;
    struct sockaddr_in client;
    socklen_t clientlen;
    int s;
    unsigned char in[16];
    int r;
    unsigned char out[16];
    AES_KEY expanded;

    AES_set_encrypt_key(key, 128, &expanded);

    if (!inet_aton("127.0.0.1", &server.sin_addr)) return 100;

    server.sin_family = AF_INET;
    server.sin_port = htons(10000);

    s = socket(AF_INET, SOCK_DGRAM, 0);

    if (s == -1) return 111;

    if (bind(s,(struct sockaddr *) &server,sizeof server) == -1) return 111;

    for (;;) {
        clientlen = sizeof client;
        r = recvfrom(s, in, sizeof in, 0, (struct sockaddr *) &client, &clientlen);
        if (r < 16) continue;
        if (r > sizeof in) continue;

        AES_encrypt(in, out, &expanded);

        sendto(s, out, 16, 0,(struct sockaddr *) &client, clientlen);
    }
}
