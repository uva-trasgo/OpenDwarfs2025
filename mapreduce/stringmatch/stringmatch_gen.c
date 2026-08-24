#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/stat.h>
#include "../../include/common_args.h"
#include "word_pool.h"


#define TEST_NUM_TARGETS 5
#define TRAIN_NUM_TARGETS 10
#define REF_NUM_TARGETS 15
#define TEST_SIZE 30000000
#define TRAIN_SIZE 500000000
#define REF_SIZE 1000000000
#define TEST_SIZES 3
#define PATH_BASE "../test/mapreduce/stringmatch/stringmatch_"
#define WORDS_PER_LINE 10
#define RNG_SEED 42u
#define STRINGMATCH_NAME_MAX_LENGTH 256

static struct option long_options[] = {
    /* name, has_arg, flag, val */
	{"num-targets",   1, NULL, 'n'},
	{"size-text",     1, NULL, 's'},
    {"gen-tests",     0, NULL, 't'},
    {"help",          0, NULL, 'h'},
    {0,0,0,0}
};

static unsigned int rng_state;

static void rng_seed(unsigned int seed) {
    rng_state = seed ? seed : 1u;
}

static unsigned int rng_next(void) {
    unsigned int x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

void printHelp(){
    fprintf(stderr, "Creates input files for stringmatch : \n\
		Posible options:\n\
			-h                  : Print this help message.\n\
            -n num-targets      : Number of words to search. Pool of 2000 words. Default %d\n\
            -s text-size        : Size of the text to look into in bytes. Can get some bytes\n\
                                  over the value. Default %d\n\
            -t                  : Create test, train and ref files.\n\
		Execution example:\n\
			./stringmatch_gen -n 7 -s 2048 \n",TEST_NUM_TARGETS,TEST_SIZE);
}

void directory_structure(){
    /* Create directories if they dont exist */
    struct stat sb;//instanciación de sb para usar stat ()
    if ( stat("../test/mapreduce/stringmatch", &sb) != 0 || !S_ISDIR(sb.st_mode) )
    {
        printf("Directory no found. Creating stringmatch/ in test/mapreduce/\n");
        mkdir("../test/", 0700);
        mkdir("../test/mapreduce/", 0700);
        mkdir("../test/mapreduce/stringmatch/", 0700);
    }
}

void write_stringmatch(const char* file_keys, const char* file_text, const char* file_path)
{
	FILE* fp;
	fp = fopen(file_path,"w");
	check(fp != NULL,"stringmatch_gen.write_stringmatch() - Cannot Open File");
	fprintf(fp,"%s\n",file_keys);
	fprintf(fp,"%s",file_text);
	fclose(fp);
}

int main( int argc, char *argv[] ){
    int opt, option_index=0;
    char* endptr = NULL;

    size_t targetsSize = 0;
    size_t targets[] = {TEST_NUM_TARGETS, TEST_NUM_TARGETS, TRAIN_NUM_TARGETS, REF_NUM_TARGETS};
    size_t sizes[] = {TEST_SIZE, TEST_SIZE, TRAIN_SIZE, REF_SIZE};
    size_t genLen = 0;
    size_t genCapacity = 0;
    int wordsThisLine = 0;
    char *h_inputKeys = NULL;
    char *w = (char*) malloc(4096);
    size_t wlen;
    char file_path[STRINGMATCH_NAME_MAX_LENGTH];
    char *path_name[] = {"user", "test", "train", "ref"};
    int *pickedIdx = NULL;
    char *genBuf = NULL;
    int genTests = 0;
    int userGen = 0;
    int filesToGen = 1;
    int index = 0;
    rng_seed(RNG_SEED);


    while ((opt = getopt_long(argc, argv, ":n:s:th", long_options, &option_index)) != -1 ) {
        printf("opt=%c (%d), optarg=%s\n", opt, opt, optarg ? optarg : "NULL");
		switch(opt){
            case 'n': targets[0] = atoi(optarg); userGen = 1; break;
            case 's': sizes[0] = strtoul(optarg,&endptr,10); userGen = 1; break;
            case 't': genTests = 1; break;
			case 'h': printHelp(); exit(EXIT_FAILURE);
			case '?': fprintf(stderr, "\nError.invalid option. Exiting stringmatch_gen.\n\n"); exit(EXIT_FAILURE);
			case ':': fprintf(stderr, "\nMissing argument\n");
			default: printHelp(); exit(EXIT_FAILURE);
		}
	}

    if ( (optind < argc) || (optind == 1) || (endptr != NULL && *endptr != '\0')) {
		printHelp();
		exit(EXIT_FAILURE);
	}
    directory_structure();
    if(!userGen) index = 1;
    if(genTests) filesToGen = 4;

    /* Create test train and ref */
    for(index; index < filesToGen; index++){
        /* Pick num_targets DISTINCT words from the pool as search targets */
        if(index<=1){
            pickedIdx = (int*) malloc(sizeof(int) * targets[index]);
            for (int i = 0; i < targets[index]; i++) {
                int idx, dup;
                do {
                    idx = (int)(rng_next() % WORD_POOL_SIZE);
                    dup = 0;
                    for (int j = 0; j < i; j++) {
                        if (pickedIdx[j] == idx) { dup = 1; break; }
                    }
                } while (dup);
                pickedIdx[i] = idx;
            }
        
            targetsSize = 0;
            for (int i = 0; i < targets[index]; i++) {
                targetsSize += strlen(WORD_POOL[pickedIdx[i]]);
            }
            targetsSize += (size_t)(targets[index] - 1);
        
            h_inputKeys = (char*) malloc(targetsSize + 1);
            h_inputKeys[0] = '\0';
            for (int i = 0; i < targets[index]; i++) {
                strcat(h_inputKeys, WORD_POOL[pickedIdx[i]]);
                if (i < targets[index] - 1) strcat(h_inputKeys, ",");
            }
            free(pickedIdx);
        }
        if(index == 0){
            genBuf = (char*) malloc(sizes[index] + 4096); /* extra space for overshoot */
        } else if(index == 1){
            genCapacity = 0;
            free(genBuf);
            genBuf = (char*) malloc(REF_SIZE + 4096); /* extra space for overshoot */
        }
        genCapacity = sizes[index];
        if (wordsThisLine != 0) genBuf[genLen - 1] = ' ';
        while (genLen < genCapacity) {
            strcpy(w,WORD_POOL[rng_next() % WORD_POOL_SIZE]);
            wlen = strlen(w);
            memcpy(genBuf + genLen, w, wlen);
            genLen += wlen;
            wordsThisLine++;
            if (wordsThisLine >= WORDS_PER_LINE) {
                genBuf[genLen++] = '\n';
                wordsThisLine = 0;
            } else {
                genBuf[genLen++] = ' ';
            }
        }
        if (genBuf[genLen - 1] != '\n') genBuf[genLen - 1] = '\n';
        genBuf[genLen] = '\0';
        
        snprintf(file_path,(sizeof(char)*STRINGMATCH_NAME_MAX_LENGTH),"%s%s_N%lu_S%lu",PATH_BASE,path_name[index],targets[index],genLen);
        printf("Saving stringmatch_%s to File '%s'...\n",path_name[index],file_path);
        write_stringmatch(h_inputKeys,genBuf,file_path);
        if(!index){
            free(h_inputKeys);
            wordsThisLine = 0;
            genLen = 0;
            rng_seed(RNG_SEED);/* reset rng for test files */
        }
    }
    if(index>1) free(h_inputKeys);
    free(genBuf);
}