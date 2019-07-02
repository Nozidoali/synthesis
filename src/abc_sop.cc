#include "abc_sop.h"


char * Abc_SopRegister( Mem_Flex_t * pMan, const char * pName )
{
    char * pRegName;
    if ( pName == NULL ) return NULL;
    pRegName = Mem_FlexEntryFetch( pMan, strlen(pName) + 1 );
    strcpy( pRegName, pName );
    return pRegName;
}


int Abc_SopGetVarNum( char * pSop )
{
    char * pCur;
    for ( pCur = pSop; *pCur != '\n'; pCur++ )
        if ( *pCur == 0 )
            return -1;
    return pCur - pSop - 2;
}


int Abc_SopCheck( char * pSop, int nFanins )
{
    char * pCubes, * pCubesOld;
    int fFound0 = 0, fFound1 = 0;

    // check the logic function of the node
    for ( pCubes = pSop; *pCubes; pCubes++ )
    {
        // get the end of the next cube
        for ( pCubesOld = pCubes; *pCubes != ' '; pCubes++ );
        // compare the distance
        if ( pCubes - pCubesOld != nFanins )
        {
            fprintf( stdout, "Abc_SopCheck: SOP has a mismatch between its cover size (%d) and its fanin number (%d).\n",
                (int)(ABC_PTRDIFF_T)(pCubes - pCubesOld), nFanins );
            return 0;
        }
        // check the output values for this cube
        pCubes++;
        if ( *pCubes == '0' )
            fFound0 = 1;
        else if ( *pCubes == '1' )
            fFound1 = 1;
        else if ( *pCubes != 'x' && *pCubes != 'n' )
        {
            fprintf( stdout, "Abc_SopCheck: SOP has a strange character (%c) in the output part of its cube.\n", *pCubes );
            return 0;
        }
        // check the last symbol (new line)
        pCubes++;
        if ( *pCubes != '\n' )
        {
            fprintf( stdout, "Abc_SopCheck: SOP has a cube without new line in the end.\n" );
            return 0;
        }
    }
    if ( fFound0 && fFound1 )
    {
        fprintf( stdout, "Abc_SopCheck: SOP has cubes in both phases.\n" );
        return 0;
    }
    return 1;
}


int Abc_SopIsConst0( char * pSop )
{
    return pSop[0] == ' ' && pSop[1] == '0';
}


int Abc_SopIsConst1( char * pSop )
{
    return pSop[0] == ' ' && pSop[1] == '1';
}


int Abc_SopGetCubeNum( char * pSop )
{
    char * pCur;
    int nCubes = 0;
    if ( pSop == NULL )
        return 0;
    for ( pCur = pSop; *pCur; pCur++ )
        nCubes += (*pCur == '\n');
    return nCubes;
}


int Abc_SopIsExorType( char * pSop )
{
    char * pCur;
    for ( pCur = pSop; *pCur; pCur++ )
        if ( *pCur == '\n' )
            return (int)(*(pCur - 1) == 'x' || *(pCur - 1) == 'n');
    assert( 0 );
    return 0;
}


int Abc_SopIsComplement( char * pSop )
{
    char * pCur;
    for ( pCur = pSop; *pCur; pCur++ )
        if ( *pCur == '\n' )
            return (int)(*(pCur - 1) == '0' || *(pCur - 1) == 'n');
    assert( 0 );
    return 0;
}


char * Abc_SopCreateAnd2( Mem_Flex_t * pMan, int fCompl0, int fCompl1 )
{
    char Buffer[6];
    Buffer[0] = '1' - fCompl0;
    Buffer[1] = '1' - fCompl1;
    Buffer[2] = ' ';
    Buffer[3] = '1';
    Buffer[4] = '\n';
    Buffer[5] = 0;
    return Abc_SopRegister( pMan, Buffer );
}


char * Abc_SopCreateOrMultiCube( Mem_Flex_t * pMan, int nVars, int * pfCompl )
{
    char * pSop, * pCube;
    int i;
    pSop = Abc_SopStart( pMan, nVars, nVars );
    i = 0;
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        pCube[i] = '1' - (pfCompl? pfCompl[i] : 0);
        i++;
    }
    return pSop;
}


char * Abc_SopStart( Mem_Flex_t * pMan, int nCubes, int nVars )
{
    char * pSopCover, * pCube;
    int i, Length;

    Length = nCubes * (nVars + 3);
    pSopCover = Mem_FlexEntryFetch( pMan, Length + 1 );
    memset( pSopCover, '-', (size_t)Length );
    pSopCover[Length] = 0;

    for ( i = 0; i < nCubes; i++ )
    {
        pCube = pSopCover + i * (nVars + 3);
        pCube[nVars + 0] = ' ';
        pCube[nVars + 1] = '1';
        pCube[nVars + 2] = '\n';
    }
    return pSopCover;
}


void Abc_SopComplementVar( char * pSop, int iVar )
{
    char * pCube;
    int nVars = Abc_SopGetVarNum(pSop);
    assert( iVar < nVars );
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        if ( pCube[iVar] == '0' )
            pCube[iVar] = '1';
        else if ( pCube[iVar] == '1' )
            pCube[iVar] = '0';
    }
}


void Abc_SopComplement( char * pSop )
{
    char * pCur;
    for ( pCur = pSop; *pCur; pCur++ )
        if ( *pCur == '\n' )
        {
            if ( *(pCur - 1) == '0' )
                *(pCur - 1) = '1';
            else if ( *(pCur - 1) == '1' )
                *(pCur - 1) = '0';
            else if ( *(pCur - 1) == 'x' )
                *(pCur - 1) = 'n';
            else if ( *(pCur - 1) == 'n' )
                *(pCur - 1) = 'x';
            else
                assert( 0 );
        }
}
