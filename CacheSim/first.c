#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

int cacheSize = 0;
char policy[5];
int assoc = 0;
int numSets = 0;
int numLines = 0;
int blockBits = 0;
int setBits = 0;
int tagBits = 0;

int memReads = 0;
int memWrites = 0;
int cacheHits = 0;
int cacheMisses = 0;

typedef struct
{
    int validBit;
    unsigned long long usedCounter;
    unsigned long long int tag;
} cacheLine;

int main(int argc, char* argv[])
{
    if(argc != 6) {
        printf("error\n");
        return 0;
    }

    for(int i=0; argv[1][i]!='\0'; i++) {
        if(isdigit(argv[1][i]) == 0) {
            printf("error\n");
            return 0;
        }
    }
    int cacheSize = atoi(argv[1]);
    if(cacheSize <= 0 || floor(log2(cacheSize)) != ceil(log2(cacheSize))) {
        printf("error\n");
        return 0;
    }
    //cache size input

    if(strcmp(argv[2],"direct") == 0) {
        assoc = 1;
    } else if(strcmp(argv[2],"assoc") == 0) {
        assoc = 69;
    } else if(sscanf(argv[2],"assoc:%d",&assoc) == 1) {
        if(assoc <= 0 || floor(log2(assoc)) != ceil(log2(assoc))) {
            printf("error\n");
            return 0;
        }
    } else {
        printf("error\n");
        return 0;
    }
    //associativity

    if(strcmp(argv[3],"lru") == 0) {
        strcpy(policy,"lru");
    } else if(strcmp(argv[3],"fifo") == 0) {
        strcpy(policy,"fifo");
    } else {
        printf("error\n");
        return 0;
    }
    //replace policy

    for(int i=0; argv[4][i]!='\0'; i++) {
        if(isdigit(argv[4][i]) == 0) {
            printf("error\n");
            return 0;
        }
    }
    int blockSize = atoi(argv[4]);
    if(blockSize <= 0 || floor(log2(blockSize)) != ceil(log2(blockSize)) || blockSize > cacheSize) {
        printf("error\n");
        return 0;
    }
    //block size, B per line

    if(assoc == 1) {
        numSets = cacheSize / blockSize;
        numLines = 1;
    } else if(assoc == 69) {
        numSets = 1;
        numLines = cacheSize / blockSize;
    } else if(floor(log2(assoc)) == ceil(log2(assoc))) {
        numSets = cacheSize / (blockSize * assoc);
        numLines = assoc;
    }
    blockBits = log2(blockSize);
    //printf("BLOCK BITS: %d\n",blockBits);
    setBits = log2(numSets);
    //printf("SET BITS: %d\n",setBits);
    tagBits = 48 - blockBits - setBits;
    //printf("TAG BITS: %d\n",tagBits);
    //printf("NUMBER OF SETS: %d\n",numSets);
    //printf("NUMBER OF LINES PER SET: %d\n\n",numLines);
    //calculations
    

    cacheLine **cache = malloc(sizeof(cacheLine*) * numSets);
    for (int i = 0; i < numSets; i++) {
        cache[i] = malloc(sizeof(cacheLine) * numLines);
    }
    for(int i=0; i<numSets; i++) {
        for(int j=0; j<numLines; j++) {
            cache[i][j].validBit = 0;
            cache[i][j].usedCounter = 0;
        }
    }
    //create cache

    FILE *fpointer = fopen(argv[5],"r");
    if(fpointer == NULL) {
        fprintf(stderr,"error\n");
        return 0;
    }

    char option;
    unsigned long long address;
    while(fscanf(fpointer,"%*x: %c %llx\n",&option,&address) == 2) {
        //unsigned long long blockOffset = address & (2^(blockBits)-1);
        //printf("address: %lld\n",address);
        unsigned long long setIndex = 0;
        if(assoc != 69) {
            setIndex = (address>>blockBits) & ((2 << (setBits-1)) - 1);
        }
        unsigned long long tag = (address>>(blockBits+setBits));
        
        //printf("set index: %lld and tag: %lld\n",setIndex,tag);

        if(option == 'W') {
        
            int temp = cacheHits;
            for(int i=0; i<numLines; i++) {
                if(cache[setIndex][i].tag == tag && cache[setIndex][i].validBit == 1) {
                    cacheHits++;
                    memWrites++;
                    if(strcmp(policy,"lru") == 0) {
                        int full = 1;
                        for(int j=0; j<numLines; j++) {
                            if(cache[setIndex][j].validBit == 0) {
                                full = 0;
                                break;
                            }
                        }
                        if(full == 1) {
                            for(int j=0; j<numLines; j++) {
                                if(cache[setIndex][j].validBit == 1) {
                                    cache[setIndex][j].usedCounter++;
                                }
                            }
                            cache[setIndex][i].usedCounter = 0;
                        }
                    }
                    break;
                }
            }
            if(cacheHits > temp) {
                continue;
            }
            //cache hit

            cacheMisses++;
            memReads++;
            memWrites++;
            int avail = 0;
            for(int i=0; i<numLines; i++) {
                if(cache[setIndex][i].validBit == 0) {
                    avail = 1;
                    cache[setIndex][i].tag = tag;
                    for(int j=0; j<numLines; j++) {
                        if(cache[setIndex][j].validBit == 1) {
                            cache[setIndex][j].usedCounter++;
                        }
                    }
                    cache[setIndex][i].validBit = 1;
                    break;
                }
            }
            if(avail == 1) {
                continue;
            }
            //cache miss, look for next free cache line

            if(strcmp(policy,"lru") == 0) {
                int leastIndexUsed = 0;
                for(int i=0; i<numLines; i++) {
                    if(cache[setIndex][i].usedCounter > cache[setIndex][leastIndexUsed].usedCounter) {
                        leastIndexUsed = i;
                    }
                }
                for(int i=0; i<numLines; i++) {
                    cache[setIndex][i].usedCounter++;
                }
                cache[setIndex][leastIndexUsed].tag = tag;
                cache[setIndex][leastIndexUsed].usedCounter = 0;
            } else if(strcmp(policy,"fifo") == 0) {
                int firstAdded = 0;
                for(int i=0; i<numLines; i++) {
                    if(cache[setIndex][i].usedCounter > cache[setIndex][firstAdded].usedCounter) {
                        firstAdded = i;
                    }
                }
                for(int i=0; i<numLines; i++) {
                    cache[setIndex][i].usedCounter++;
                }
                cache[setIndex][firstAdded].tag = tag;
                cache[setIndex][firstAdded].usedCounter = 0;
            }
            //cache miss, replacement


        } else if(option == 'R') {
            
            int temp = cacheHits;
            for(int i=0; i<numLines; i++) {
                if(cache[setIndex][i].tag == tag && cache[setIndex][i].validBit) {
                    cacheHits++;
                    if(strcmp(policy,"lru") == 0) {
                        int full = 1;
                        for(int j=0; j<numLines; j++) {
                            if(cache[setIndex][j].validBit == 0) {
                                full = 0;
                                break;
                            }
                        }
                        if(full == 1) {
                            for(int j=0; j<numLines; j++) {
                                if(cache[setIndex][j].validBit == 1) {
                                    cache[setIndex][j].usedCounter++;
                                }
                            }
                            cache[setIndex][i].usedCounter = 0;
                        }
                    }
                    break;
                }
            }
            if(cacheHits > temp) {
                continue;
            }
            //cache hit
            
            cacheMisses++;
            memReads++;
            int avail = 0;
            for(int i=0; i<numLines; i++) {
                if(cache[setIndex][i].validBit == 0) {
                    avail = 1;
                    cache[setIndex][i].tag = tag;
                    for(int j=0; j<numLines; j++) {
                        if(cache[setIndex][j].validBit == 1) {
                            cache[setIndex][j].usedCounter++;
                        }
                    }
                    cache[setIndex][i].validBit = 1;
                    break;
                }
            }
            if(avail == 1) {
                continue;
            }
            //cache miss, look for next free cache line

            if(strcmp(policy,"lru") == 0) {
                int leastIndexUsed = 0;
                for(int i=0; i<numLines; i++) {
                    if(cache[setIndex][i].usedCounter > cache[setIndex][leastIndexUsed].usedCounter) {
                        leastIndexUsed = i;
                    }
                }
                for(int i=0; i<numLines; i++) {
                    cache[setIndex][i].usedCounter++;
                }
                cache[setIndex][leastIndexUsed].tag = tag;
                cache[setIndex][leastIndexUsed].usedCounter = 0;
            } else if(strcmp(policy,"fifo") == 0) {
                int firstAdded = 0;
                for(int i=0; i<numLines; i++) {
                    if(cache[setIndex][i].usedCounter > cache[setIndex][firstAdded].usedCounter) {
                        firstAdded = i;
                    }
                }
                for(int i=0; i<numLines; i++) {
                    cache[setIndex][i].usedCounter++;
                }
                cache[setIndex][firstAdded].tag = tag;
                cache[setIndex][firstAdded].usedCounter = 0;
            }
            //cache miss, replacement
        }
    }
    fclose(fpointer);
    //trace file loop

    printf("Memory reads: %d\n",memReads);
    printf("Memory writes: %d\n",memWrites);
    printf("Cache hits: %d\n",cacheHits);
    printf("Cache misses: %d\n",cacheMisses);

    for(int i=0; i<numSets; i++) {
        free(cache[i]);
    }
    free(cache);
    //free cache

    return 0;
}
