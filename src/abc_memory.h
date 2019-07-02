#ifndef ABC_MEMORY_H
#define ABC_MEMORY_H


#include "abc_global.h"


typedef struct Extra_MmFixed_t_    Extra_MmFixed_t;
typedef struct Extra_MmFlex_t_     Extra_MmFlex_t;


typedef struct Mem_Fixed_t_    Mem_Fixed_t;
typedef struct Mem_Flex_t_     Mem_Flex_t;
typedef struct Mem_Step_t_     Mem_Step_t;


struct Extra_MmFixed_t_
{
    // information about individual entries
    int           nEntrySize;    // the size of one entry
    int           nEntriesAlloc; // the total number of entries allocated
    int           nEntriesUsed;  // the number of entries in use
    int           nEntriesMax;   // the max number of entries in use
    char *        pEntriesFree;  // the linked list of free entries

    // this is where the memory is stored
    int           nChunkSize;    // the size of one chunk
    int           nChunksAlloc;  // the maximum number of memory chunks
    int           nChunks;       // the current number of memory chunks
    char **       pChunks;       // the allocated memory

    // statistics
    int           nMemoryUsed;   // memory used in the allocated entries
    int           nMemoryAlloc;  // memory allocated
};


struct Extra_MmFlex_t_
{
    // information about individual entries
    int           nEntriesUsed;  // the number of entries allocated
    char *        pCurrent;      // the current pointer to free memory
    char *        pEnd;          // the first entry outside the free memory

    // this is where the memory is stored
    int           nChunkSize;    // the size of one chunk
    int           nChunksAlloc;  // the maximum number of memory chunks
    int           nChunks;       // the current number of memory chunks
    char **       pChunks;       // the allocated memory

    // statistics
    int           nMemoryUsed;   // memory used in the allocated entries
    int           nMemoryAlloc;  // memory allocated
};


struct Mem_Fixed_t_
{
    // information about individual entries
    int           nEntrySize;    // the size of one entry
    int           nEntriesAlloc; // the total number of entries allocated
    int           nEntriesUsed;  // the number of entries in use
    int           nEntriesMax;   // the max number of entries in use
    char *        pEntriesFree;  // the linked list of free entries

    // this is where the memory is stored
    int           nChunkSize;    // the size of one chunk
    int           nChunksAlloc;  // the maximum number of memory chunks
    int           nChunks;       // the current number of memory chunks
    char **       pChunks;       // the allocated memory

    // statistics
    int           nMemoryUsed;   // memory used in the allocated entries
    int           nMemoryAlloc;  // memory allocated
};


struct Mem_Flex_t_
{
    // information about individual entries
    int           nEntriesUsed;  // the number of entries allocated
    char *        pCurrent;      // the current pointer to free memory
    char *        pEnd;          // the first entry outside the free memory

    // this is where the memory is stored
    int           nChunkSize;    // the size of one chunk
    int           nChunksAlloc;  // the maximum number of memory chunks
    int           nChunks;       // the current number of memory chunks
    char **       pChunks;       // the allocated memory

    // statistics
    int           nMemoryUsed;   // memory used in the allocated entries
    int           nMemoryAlloc;  // memory allocated
};


struct Mem_Step_t_
{
    int             nMems;              // the number of fixed memory managers employed
    Mem_Fixed_t **  pMems;              // memory managers: 2^1 words, 2^2 words, etc
    int             nMapSize;           // the size of the memory array
    Mem_Fixed_t **  pMap;               // maps the number of bytes into its memory manager
    int             nLargeChunksAlloc;  // the maximum number of large memory chunks
    int             nLargeChunks;       // the current number of large memory chunks
    void **         pLargeChunks;       // the allocated large memory chunks
};


Mem_Fixed_t * Mem_FixedStart( int nEntrySize );
int Mem_FixedReadMemUsage( Mem_Fixed_t * p );
void Mem_FixedStop( Mem_Fixed_t * p, int fVerbose );
char * Mem_FixedEntryFetch( Mem_Fixed_t * p );
void Mem_FixedEntryRecycle( Mem_Fixed_t * p, char * pEntry );

Mem_Flex_t * Mem_FlexStart();
void Mem_FlexStop( Mem_Flex_t * p, int fVerbose );
char * Mem_FlexEntryFetch( Mem_Flex_t * p, int nBytes );

Mem_Step_t * Mem_StepStart( int nSteps );
int Mem_StepReadMemUsage( Mem_Step_t * p );
void Mem_StepStop( Mem_Step_t * p, int fVerbose );
char * Mem_StepEntryFetch( Mem_Step_t * p, int nBytes );
void Mem_StepEntryRecycle( Mem_Step_t * p, char * pEntry, int nBytes );

Extra_MmFixed_t * Extra_MmFixedStart( int nEntrySize );
char * Extra_MmFixedEntryFetch( Extra_MmFixed_t * p );
void Extra_MmFixedEntryRecycle( Extra_MmFixed_t * p, char * pEntry );
void Extra_MmFixedStop( Extra_MmFixed_t * p );

Extra_MmFlex_t * Extra_MmFlexStart();
void Extra_MmFlexStop( Extra_MmFlex_t * p );
char * Extra_MmFlexEntryFetch( Extra_MmFlex_t * p, int nBytes );


#endif
