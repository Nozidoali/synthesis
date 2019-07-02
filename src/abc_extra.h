#ifndef ABC_EXTRA_H
#define ABC_EXTRA_H


#include "abc_memory.h"
#include "abc_vec.h"


#define EXTRA_BUFFER_SIZE        4*1048576    // 1M   - size of the data chunk stored in memory
#define EXTRA_OFFSET_SIZE           4096    // 4K   - load new data when less than this is left
#define EXTRA_MINIMUM(a,b)       (((a) < (b))? (a) : (b))


typedef enum {
    EXTRA_CHAR_COMMENT,  // a character that begins the comment
    EXTRA_CHAR_NORMAL,   // a regular character
    EXTRA_CHAR_STOP,     // a character that delimits a series of tokens
    EXTRA_CHAR_CLEAN     // a character that should be cleaned
} Extra_CharType_t;


typedef struct Extra_FileReader_t_ Extra_FileReader_t;


struct Extra_FileReader_t_
{
    // the input file
    char *           pFileName;     // the input file name
    FILE *           pFile;         // the input file pointer
    int              nFileSize;     // the total number of bytes in the file
    int              nFileRead;     // the number of bytes currently read from file
    // info about processing different types of input chars
    char             pCharMap[256]; // the character map
    // temporary storage for data
    char *           pBuffer;       // the buffer
    int              nBufferSize;   // the size of the buffer
    char *           pBufferCur;    // the current reading position
    char *           pBufferEnd;    // the first position not used by currently loaded data
    char *           pBufferStop;   // the position where loading new data will be done
    // tokens given to the user
    Vec_Ptr_t *      vTokens;       // the vector of tokens returned to the user
    Vec_Int_t *      vLines;        // the vector of line numbers for each token
    int              nLineCounter;  // the counter of lines processed
    // status of the parser
    int              fStop;         // this flag goes high when the end of file is reached
};


Extra_FileReader_t * Extra_FileReaderAlloc( char * pFileName, char * pCharsComment, char * pCharsStop, char * pCharsClean );
void * Extra_FileReaderGetTokens( Extra_FileReader_t * p );
void * Extra_FileReaderGetTokens_int( Extra_FileReader_t * p );
void Extra_FileReaderReload( Extra_FileReader_t * p );
int Extra_FileReaderGetLineNumber( Extra_FileReader_t * p, int iToken );
void Extra_FileReaderFree( Extra_FileReader_t * p );
char * Extra_UtilStrsav( const char *s );
char * Extra_TimeStamp();
void Extra_Truth4VarNPN( unsigned short ** puCanons, char ** puPhases, char ** puPerms, unsigned char ** puMap );
char ** Extra_Permutations( int n );
int Extra_Factorial( int n );
void ** Extra_ArrayAlloc( int nCols, int nRows, int Size );
void Extra_Permutations_rec( char ** pRes, int nFact, int n, char Array[] );
unsigned Extra_TruthPolarize( unsigned uTruth, int Polarity, int nVars );
unsigned Extra_TruthPermute( unsigned Truth, char * pPerms, int nVars, int fReverse );
void Extra_TruthPermute_int( int * pMints, int nMints, char * pPerm, int nVars, int * pMintsP );
void Extra_TruthStretch( unsigned * pOut, unsigned * pIn, int nVars, int nVarsAll, unsigned Phase );
void Extra_TruthSwapAdjacentVars( unsigned * pOut, unsigned * pIn, int nVars, int iVar );
int Extra_TruthMinCofSuppOverlap( unsigned * pTruth, int nVars, int * pVarMin );
void Extra_TruthCofactor0( unsigned * pTruth, int nVars, int iVar );
int Extra_TruthSupport( unsigned * pTruth, int nVars );
int Extra_TruthVarInSupport( unsigned * pTruth, int nVars, int iVar );
void Extra_TruthCofactor1( unsigned * pTruth, int nVars, int iVar );
void Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits );
void Extra_PrintHex( FILE * pFile, unsigned * pTruth, int nVars );


/*=== extraUtilTruth.c ================================================================*/
static inline int   Extra_BitWordNum( int nBits )    { return nBits/(8*sizeof(unsigned)) + ((nBits%(8*sizeof(unsigned))) > 0);  }
static inline int   Extra_TruthWordNum( int nVars )  { return nVars <= 5 ? 1 : (1 << (nVars - 5)); }

static inline void  Extra_TruthSetBit( unsigned * p, int Bit )   { p[Bit>>5] |= (unsigned)(1<<(Bit & 31));               }
static inline void  Extra_TruthXorBit( unsigned * p, int Bit )   { p[Bit>>5] ^= (unsigned)(1<<(Bit & 31));               }
static inline int   Extra_TruthHasBit( unsigned * p, int Bit )   { return (p[Bit>>5] & (unsigned)(1<<(Bit & 31))) > 0;   }

static inline int Extra_WordCountOnes( unsigned uWord )
{
    uWord = (uWord & 0x55555555) + ((uWord>>1) & 0x55555555);
    uWord = (uWord & 0x33333333) + ((uWord>>2) & 0x33333333);
    uWord = (uWord & 0x0F0F0F0F) + ((uWord>>4) & 0x0F0F0F0F);
    uWord = (uWord & 0x00FF00FF) + ((uWord>>8) & 0x00FF00FF);
    return  (uWord & 0x0000FFFF) + (uWord>>16);
}
static inline int Extra_TruthCountOnes( unsigned * pIn, int nVars )
{
    int w, Counter = 0;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        Counter += Extra_WordCountOnes(pIn[w]);
    return Counter;
}
static inline int Extra_TruthIsEqual( unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn0[w] != pIn1[w] )
            return 0;
    return 1;
}
static inline int Extra_TruthIsConst0( unsigned * pIn, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn[w] )
            return 0;
    return 1;
}
static inline int Extra_TruthIsConst1( unsigned * pIn, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn[w] != ~(unsigned)0 )
            return 0;
    return 1;
}
static inline int Extra_TruthIsImply( unsigned * pIn1, unsigned * pIn2, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn1[w] & ~pIn2[w] )
            return 0;
    return 1;
}
static inline void Extra_TruthCopy( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn[w];
}
static inline void Extra_TruthClear( unsigned * pOut, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = 0;
}
static inline void Extra_TruthFill( unsigned * pOut, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~(unsigned)0;
}
static inline void Extra_TruthNot( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~pIn[w];
}
static inline void Extra_TruthAnd( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] & pIn1[w];
}
static inline void Extra_TruthOr( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] | pIn1[w];
}
static inline void Extra_TruthSharp( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] & ~pIn1[w];
}
static inline void Extra_TruthNand( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~(pIn0[w] & pIn1[w]);
}
static inline void Extra_TruthAndPhase( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars, int fCompl0, int fCompl1 )
{
    int w;
    if ( fCompl0 && fCompl1 )
    {
        for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = ~(pIn0[w] | pIn1[w]);
    }
    else if ( fCompl0 && !fCompl1 )
    {
        for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = ~pIn0[w] & pIn1[w];
    }
    else if ( !fCompl0 && fCompl1 )
    {
        for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = pIn0[w] & ~pIn1[w];
    }
    else // if ( !fCompl0 && !fCompl1 )
    {
        for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
            pOut[w] = pIn0[w] & pIn1[w];
    }
}


#endif
