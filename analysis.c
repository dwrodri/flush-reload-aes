#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <openssl/aes.h>
#include <unistd.h>
#include <string.h>


char *timingFileName;
char *cipherFileName;
char *resultFileName;
int threshold = 2000;

FILE *timingFP, *cipherFP, *resultFP;

uint8_t *ciphertext;
uint32_t *timing;

unsigned long long counter[16][256];
float times[16][256];

void printHelp(char* argv[]);
void parseOpt(int argc, char *argv[]);
void init();
void finish();
bool loadTrace();
void processTrace();
void saveResult();

int main(int argc, char** argv){
    parseOpt(argc, argv);
    init();
#ifdef DEBUG
    uint32_t i = 0;
#endif

	while(loadTrace() == true){
        processTrace();
#ifdef DEBUG
        i++;
#endif
	}
    saveResult();
    finish();
#ifdef DEBUG
        printf("Number samples used: %i\n", i);
#endif
}
void saveResult()
{
    int i, j;
	for(i = 0; i < 16; i++){
		for(j = 0; j < 255; j++){
			fprintf(resultFP, "%f ",times[i][j]/counter[i][j]);
		}
		fprintf(resultFP, "%f\n",times[i][j]/counter[i][j]);
	}
}
void processTrace()
{
    int i;
	for(i = 0; i < 16; i++){
        // printf("Time: %i\n", *timing);
		if(*timing < threshold){
			counter[i][ciphertext[i] & 0xff] += 1;
			times[i][ciphertext[i] & 0xff] += *timing;
        }
	}
}
bool loadTrace()
{
	return fread(timing, sizeof(uint32_t), 1, timingFP) &&
            fread(ciphertext, sizeof(uint8_t), 16, cipherFP);
}
void resetCounter()
{
    int i, j;
	for(i = 0; i < 16; i++){
		for(j = 0; j < 256; j++){
			counter[i][j] = 0;
		}
	}
}
void init()
{
    /* allocate memory for one trace */
    ciphertext = (uint8_t *) malloc(sizeof(uint8_t) * 16);
    timing = (uint32_t *) malloc(sizeof(uint32_t));

    /* open required file to read/write */
    timingFP = fopen(timingFileName, "r");
    cipherFP = fopen(cipherFileName, "r");
    resultFP = fopen(resultFileName, "w");
    resetCounter();
}
void finish()
{
   free(ciphertext);
   free(timing);

   fclose(timingFP);
   fclose(cipherFP);
   fclose(resultFP);
}

void printHelp(char* argv[])
{
    fprintf(
            stderr,
            "Usage: %s [-t timing file name] "
            "[-c cipher file name] "
            "[-o result file name] "
            "[-s threshold (default: 150)] "
            "\n",
            argv[0]
           );
    exit(EXIT_FAILURE);
}

void parseOpt(int argc, char *argv[])
{
    int opt;
    while((opt = getopt(argc, argv, "c:t:o:s:")) != -1){
        switch(opt){
        case 'c':
            cipherFileName = optarg;
            break;
        case 't':
            timingFileName = optarg;
            break;
        case 'o':
            resultFileName = optarg;
            break;
        case 's':
            threshold = atoi(optarg);
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
    if(resultFileName == NULL){
        printHelp(argv);
    }
}
