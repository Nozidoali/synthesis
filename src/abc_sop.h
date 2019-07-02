#ifndef ABC_SOP_H
#define ABC_SOP_H


#include "abc_memory.h"


// cubes and literals
#define Abc_CubeForEachVar( pCube, Value, i )                                                      \
    for ( i = 0; (pCube[i] != ' ') && (Value = pCube[i]); i++ )
#define Abc_SopForEachCube( pSop, nFanins, pCube )                                                 \
    for ( pCube = (pSop); *pCube; pCube += (nFanins) + 3 )
#define Abc_SopForEachCubePair( pSop, nFanins, pCube, pCube2 )                                     \
    Abc_SopForEachCube( pSop, nFanins, pCube )                                                     \
    Abc_SopForEachCube( pCube + (nFanins) + 3, nFanins, pCube2 )


char * Abc_SopRegister( Mem_Flex_t * pMan, const char * pName );
int Abc_SopGetVarNum( char * pSop );
int Abc_SopCheck( char * pSop, int nFanins );
int Abc_SopIsConst0( char * pSop );
int Abc_SopIsConst1( char * pSop );
int Abc_SopGetCubeNum( char * pSop );
int Abc_SopIsExorType( char * pSop );
int Abc_SopIsComplement( char * pSop );
char * Abc_SopCreateAnd2( Mem_Flex_t * pMan, int fCompl0, int fCompl1 );
char * Abc_SopCreateOrMultiCube( Mem_Flex_t * pMan, int nVars, int * pfCompl );
char * Abc_SopStart( Mem_Flex_t * pMan, int nCubes, int nVars );
void Abc_SopComplementVar( char * pSop, int iVar );
void Abc_SopComplement( char * pSop );


#endif
