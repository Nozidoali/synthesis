#include "abc_mvc.h"


Mvc_Manager_t * Mvc_ManagerStart()
{
    Mvc_Manager_t * p;
    p = ABC_ALLOC( Mvc_Manager_t, 1 );
    memset( p, 0, sizeof(Mvc_Manager_t) );
    p->pMan1 = Extra_MmFixedStart( sizeof(Mvc_Cube_t)                              );
    p->pMan2 = Extra_MmFixedStart( sizeof(Mvc_Cube_t) +     sizeof(Mvc_CubeWord_t) );
    p->pMan4 = Extra_MmFixedStart( sizeof(Mvc_Cube_t) + 3 * sizeof(Mvc_CubeWord_t) );
    p->pManC = Extra_MmFixedStart( sizeof(Mvc_Cover_t) );
    return p;
}


int  Mvc_CoverContain( Mvc_Cover_t * pCover )
{
    int nCubes;
    nCubes = Mvc_CoverReadCubeNum( pCover );
    if ( nCubes < 2 )
        return 0;
    Mvc_CoverSetCubeSizes(pCover);
    Mvc_CoverSort( pCover, NULL, Mvc_CubeCompareSizeAndInt );
    Mvc_CoverRemoveDuplicates( pCover );
    if ( nCubes > 1 )
        Mvc_CoverRemoveContained( pCover );
    return (nCubes != Mvc_CoverReadCubeNum(pCover));
}


int Mvc_CoverIsEmpty( Mvc_Cover_t * pCover )
{
    return Mvc_CoverReadCubeNum(pCover) == 0;
}


int Mvc_CoverIsTautology( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int iBit, Value;

    if ( Mvc_CoverReadCubeNum(pCover) != 1 )
        return 0;

    pCube = Mvc_CoverReadCubeHead( pCover );
    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
        if ( Value == 0 )
            return 0;
    return 1;
}


void Mvc_CoverInverse( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    // complement the cubes
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubeBitNot( pCube );
}


void Mvc_CoverFree( Mvc_Cover_t * p )
{
    Mvc_Cube_t * pCube, * pCube2;
    // recycle cube list
    Mvc_CoverForEachCubeSafe( p, pCube, pCube2 )
        Mvc_CubeFree( p, pCube );
    // recycle other pointers
    Mvc_CubeFree( p, p->pMask );
    MEM_FREE( p->pMem, Mvc_Cube_t *, p->nCubesAlloc, p->pCubes );
    MEM_FREE( p->pMem, int,          p->nBits,       p->pLits  );

#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    ABC_FREE( p );
#else
    Extra_MmFixedEntryRecycle( p->pMem->pManC, (char *)p );
#endif
}


Mvc_Cover_t * Mvc_CoverAlloc( Mvc_Manager_t * pMem, int nBits )
{
    Mvc_Cover_t * p;
    int nBitsInUnsigned;

    nBitsInUnsigned  = 8 * sizeof(Mvc_CubeWord_t);
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    p                = (Mvc_Cover_t *)ABC_ALLOC( char, sizeof(Mvc_Cover_t) );
#else
    p                = (Mvc_Cover_t *)Extra_MmFixedEntryFetch( pMem->pManC );
#endif
    p->pMem          = pMem;
    p->nBits         = nBits;
    p->nWords        = nBits / nBitsInUnsigned + (int)(nBits % nBitsInUnsigned > 0);
    p->nUnused       = p->nWords * nBitsInUnsigned - p->nBits;
    p->lCubes.nItems = 0;
    p->lCubes.pHead  = NULL;
    p->lCubes.pTail  = NULL;
    p->nCubesAlloc   = 0;
    p->pCubes        = NULL;
    p->pMask         = NULL;
    p->pLits         = NULL;
    return p;
}


Mvc_Cube_t * Mvc_CubeAlloc( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;

    assert( pCover->nWords >= 0 );
    // allocate the cube
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    if ( pCover->nWords == 0 )
        pCube = (Mvc_Cube_t *)ABC_ALLOC( char, sizeof(Mvc_Cube_t) );
    else
        pCube = (Mvc_Cube_t *)ABC_ALLOC( char,  sizeof(Mvc_Cube_t) + sizeof(Mvc_CubeWord_t) * (pCover->nWords - 1) );
#else
    switch( pCover->nWords )
    {
    case 0:
    case 1:
        pCube = (Mvc_Cube_t *)Extra_MmFixedEntryFetch( pCover->pMem->pMan1 );
        break;
    case 2:
        pCube = (Mvc_Cube_t *)Extra_MmFixedEntryFetch( pCover->pMem->pMan2 );
        break;
    case 3:
    case 4:
        pCube = (Mvc_Cube_t *)Extra_MmFixedEntryFetch( pCover->pMem->pMan4 );
        break;
    default:
        pCube = (Mvc_Cube_t *)ABC_ALLOC( char, sizeof(Mvc_Cube_t) + sizeof(Mvc_CubeWord_t) * (pCover->nWords - 1) );
        break;
    }
#endif
    // set the parameters charactering this cube
    if ( pCover->nWords == 0 )
        pCube->iLast   = pCover->nWords;
    else
        pCube->iLast   = pCover->nWords - 1;
    pCube->nUnused = pCover->nUnused;
    return pCube;
}


int          Mvc_CoverReadWordNum( Mvc_Cover_t * pCover )   { return pCover->nWords;         }
int          Mvc_CoverReadBitNum( Mvc_Cover_t * pCover )    { return pCover->nBits;          }
int          Mvc_CoverReadCubeNum( Mvc_Cover_t * pCover )   { return pCover->lCubes.nItems; }
Mvc_Cube_t * Mvc_CoverReadCubeHead( Mvc_Cover_t * pCover )  { return pCover->lCubes.pHead;  }
Mvc_Cube_t * Mvc_CoverReadCubeTail( Mvc_Cover_t * pCover )  { return pCover->lCubes.pTail;  }
Mvc_List_t * Mvc_CoverReadCubeList( Mvc_Cover_t * pCover )  { return &pCover->lCubes;        }


Mvc_Cover_t * Mvc_CoverDivisor( Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pKernel;
    if ( Mvc_CoverReadCubeNum(pCover) <= 1 )
        return NULL;
    // allocate the literal array and count literals
    if ( Mvc_CoverAnyLiteral( pCover, NULL ) == -1 )
        return NULL;
    // duplicate the cover
    pKernel = Mvc_CoverDup(pCover);
    // perform the kerneling
    Mvc_CoverDivisorZeroKernel( pKernel );
    assert( Mvc_CoverReadCubeNum(pKernel) );
    return pKernel;
}


int Mvc_CoverAnyLiteral( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask )
{
    Mvc_Cube_t * pCube;
    int nWord, nBit, i;
    int nLitsCur;
    int fUseFirst = 0;

    // go through each literal
    if ( fUseFirst )
    {
        for ( i = 0; i < pCover->nBits; i++ )
            if ( !pMask || Mvc_CubeBitValue(pMask,i) )
            {
                // get the word and bit of this literal
                nWord = Mvc_CubeWhichWord(i);
                nBit  = Mvc_CubeWhichBit(i);
                // go through all the cubes
                nLitsCur = 0;
                Mvc_CoverForEachCube( pCover, pCube )
                    if ( pCube->pData[nWord] & (1<<nBit) )
                    {
                        nLitsCur++;
                        if ( nLitsCur > 1 )
                            return i;
                    }
            }
    }
    else
    {
        for ( i = pCover->nBits - 1; i >=0; i-- )
            if ( !pMask || Mvc_CubeBitValue(pMask,i) )
            {
                // get the word and bit of this literal
                nWord = Mvc_CubeWhichWord(i);
                nBit  = Mvc_CubeWhichBit(i);
                // go through all the cubes
                nLitsCur = 0;
                Mvc_CoverForEachCube( pCover, pCube )
                    if ( pCube->pData[nWord] & (1<<nBit) )
                    {
                        nLitsCur++;
                        if ( nLitsCur > 1 )
                            return i;
                    }
            }
    }
    return -1;
}


static int bit_count[256] = {
  0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};


int Mvc_CoverSetCubeSizes( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    unsigned char * pByte, * pByteStart, * pByteStop;
    int nBytes, nOnes;

    // get the number of unsigned chars in the cube's bit strings
    nBytes = pCover->nBits / (8 * sizeof(unsigned char)) + (int)(pCover->nBits % (8 * sizeof(unsigned char)) > 0);
    // iterate through the cubes
    Mvc_CoverForEachCube( pCover, pCube )
    {
        // clean the counter of ones
        nOnes = 0;
        // set the starting and stopping positions
        pByteStart = (unsigned char *)pCube->pData;
        pByteStop  = pByteStart + nBytes;
        // iterate through the positions
        for ( pByte = pByteStart; pByte < pByteStop; pByte++ )
            nOnes += bit_count[*pByte];
        // set the nOnes
        Mvc_CubeSetSize( pCube, nOnes );
    }
    return 1;
}


void Mvc_CoverSort( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask, int (* pCompareFunc)(Mvc_Cube_t *, Mvc_Cube_t *, Mvc_Cube_t *) )
{
    Mvc_Cube_t * pHead;
    int nCubes;
    // one cube does not need sorting
    nCubes = Mvc_CoverReadCubeNum(pCover);
    if ( nCubes <= 1 )
        return;
    // sort the cubes
    pHead = Mvc_CoverSort_rec( Mvc_CoverReadCubeHead(pCover), nCubes, pMask, pCompareFunc );
    // insert the sorted list into the cover
    Mvc_CoverSetCubeHead( pCover, pHead );
    Mvc_CoverSetCubeTail( pCover, Mvc_ListGetTailFromHead(pHead) );
    // make sure that the list is sorted in the increasing order
    assert( pCompareFunc( Mvc_CoverReadCubeHead(pCover), Mvc_CoverReadCubeTail(pCover), pMask ) <= 0 );
}


void Mvc_CoverRemoveDuplicates( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pPrev, * pCube, * pCube2;
    int  fEqual;

    // set the first cube of the cover
    pPrev = Mvc_CoverReadCubeHead(pCover);
    // go through all the cubes after this one
    Mvc_CoverForEachCubeStartSafe( Mvc_CubeReadNext(pPrev), pCube, pCube2 )
    {
        // compare the current cube with the prev cube
        Mvc_CubeBitEqual( fEqual, pPrev, pCube );
        if ( fEqual )
        { // they are equal - remove the current cube
            Mvc_CoverDeleteCube( pCover, pPrev, pCube );
            Mvc_CubeFree( pCover, pCube );
            // don't change the previous cube cube
        }
        else
        { // they are not equal - update the previous cube
            pPrev = pCube;
        }
    }
}


int Mvc_CubeCompareSizeAndInt( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask )
{
    // compare the cubes by size
    if ( Mvc_CubeReadSize( pC1 ) < Mvc_CubeReadSize( pC2 ) )
        return 1;
    if ( Mvc_CubeReadSize( pC1 ) > Mvc_CubeReadSize( pC2 ) )
        return -1;
    // the cubes have the same size

    // compare the cubes as integers
    if ( Mvc_Cube1Words( pC1 ) )
    {
        if ( pC1->pData[0] < pC2->pData[0] )
            return -1;
        if ( pC1->pData[0] > pC2->pData[0] )
            return 1;
        return 0;
    }
    else if ( Mvc_Cube2Words( pC1 ) )
    {
        if ( pC1->pData[1] < pC2->pData[1] )
            return -1;
        if ( pC1->pData[1] > pC2->pData[1] )
            return 1;
        if ( pC1->pData[0] < pC2->pData[0] )
            return -1;
        if ( pC1->pData[0] > pC2->pData[0] )
            return 1;
        return 0;
    }
    else
    {
        int i = Mvc_CubeReadLast( pC1 );
        for(; i >= 0; i--)
        {
            if ( pC1->pData[i] < pC2->pData[i] )
                return -1;
            if ( pC1->pData[i] > pC2->pData[i] )
                return 1;
        }
        return 0;
    }
}


void Mvc_CoverRemoveContained( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCubeBeg, * pCubeEnd, * pCubeLarge;
    Mvc_Cube_t * pCube, * pCube2, * pPrev;
    unsigned sizeCur;
    int  Result;

    // since the cubes are sorted by size, it is sufficient
    // to compare each cube with other cubes that have larger sizes
    // if the given cube implies a larger cube, the larger cube is removed
    pCubeBeg = Mvc_CoverReadCubeHead(pCover);
    do
    {
        // get the current cube size
        sizeCur = Mvc_CubeReadSize(pCubeBeg);

        // initialize the end of the given size group
        pCubeEnd = pCubeBeg;
        // find the beginning of the next size group
        Mvc_CoverForEachCubeStart( Mvc_CubeReadNext(pCubeBeg), pCube )
        {
            if ( sizeCur == Mvc_CubeReadSize(pCube) )
                pCubeEnd = pCube;
            else // pCube is the first cube in the new size group
                break;
        }
        // if we could not find the next size group
        // the containment check is finished
        if ( pCube == NULL )
            break;
        // otherwise, pCubeBeg/pCubeEnd are the first/last cubes of the group

        // go through all the cubes between pCubeBeg and pCubeEnd, inclusive,
        // and for each of them, try removing cubes after pCubeEnd
        Mvc_CoverForEachCubeStart( pCubeBeg, pCubeLarge )
        {
            pPrev = pCubeEnd;
            Mvc_CoverForEachCubeStartSafe( Mvc_CubeReadNext(pCubeEnd), pCube, pCube2 )
            {
                // check containment
                Mvc_CubeBitNotImpl( Result, pCube, pCubeLarge );
                if ( !Result )
                { // pCubeLarge implies pCube - remove pCube
                    Mvc_CoverDeleteCube( pCover, pPrev, pCube );
                    Mvc_CubeFree( pCover, pCube );
                    // don't update the previous cube
                }
                else
                {   // update the previous cube
                    pPrev = pCube;
                }
            }
            // quit, if the main cube was the last one of this size
            if ( pCubeLarge == pCubeEnd )
                break;
        }

        // set the beginning of the next group
        pCubeBeg = Mvc_CubeReadNext(pCubeEnd);
    }
    while ( pCubeBeg );
}


void Mvc_CubeFree( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    if ( pCube == NULL )
        return;

    // verify the parameters charactering this cube
    assert( pCube->iLast == 0 || ((int)pCube->iLast) == pCover->nWords - 1 );
    assert( ((int)pCube->nUnused) == pCover->nUnused );

    // deallocate the cube
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    ABC_FREE( pCube );
#else
    switch( pCover->nWords )
    {
    case 0:
    case 1:
        Extra_MmFixedEntryRecycle( pCover->pMem->pMan1, (char *)pCube );
        break;
    case 2:
        Extra_MmFixedEntryRecycle( pCover->pMem->pMan2, (char *)pCube );
        break;
    case 3:
    case 4:
        Extra_MmFixedEntryRecycle( pCover->pMem->pMan4, (char *)pCube );
        break;
    default:
        ABC_FREE( pCube );
        break;
    }
#endif
}


Mvc_Cube_t * Mvc_CoverSort_rec( Mvc_Cube_t * pList, int nItems, Mvc_Cube_t * pMask, int (* pCompareFunc)(Mvc_Cube_t *, Mvc_Cube_t *, Mvc_Cube_t *) )
{
    Mvc_Cube_t * pList1, * pList2;
    int nItems1, nItems2, i;

    // trivial case
    if ( nItems == 1 )
    {
        Mvc_CubeSetNext( pList, NULL );
        return pList;
    }

    // select the new sizes
    nItems1 = nItems/2;
    nItems2 = nItems - nItems1;

    // set the new beginnings
    pList1 = pList2 = pList;
    for ( i = 0; i < nItems1; i++ )
        pList2 = Mvc_CubeReadNext( pList2 );

    // solve recursively
    pList1 = Mvc_CoverSort_rec( pList1, nItems1, pMask, pCompareFunc );
    pList2 = Mvc_CoverSort_rec( pList2, nItems2, pMask, pCompareFunc );

    // merge
    return Mvc_CoverSortMerge( pList1, pList2, pMask, pCompareFunc );
}


Mvc_Cube_t * Mvc_CoverSortMerge( Mvc_Cube_t * pList1, Mvc_Cube_t * pList2, Mvc_Cube_t * pMask, int (* pCompareFunc)(Mvc_Cube_t *, Mvc_Cube_t *, Mvc_Cube_t *) )
{
    Mvc_Cube_t * pList = NULL, ** ppTail = &pList;
    Mvc_Cube_t * pCube;
    while ( pList1 && pList2 )
    {
        if ( pCompareFunc( pList1, pList2, pMask ) < 0 )
        {
            pCube = pList1;
            pList1 = Mvc_CubeReadNext(pList1);
        }
        else
        {
            pCube = pList2;
            pList2 = Mvc_CubeReadNext(pList2);
        }
        *ppTail = pCube;
        ppTail = Mvc_CubeReadNextP(pCube);
    }
    *ppTail = pList1? pList1: pList2;
    return pList;
}


Mvc_Cover_t * Mvc_CoverDup( Mvc_Cover_t * p )
{
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube, * pCubeCopy;
    // clone the cover
    pCover = Mvc_CoverClone( p );
    // copy the cube list
    Mvc_CoverForEachCube( p, pCube )
    {
        pCubeCopy = Mvc_CubeDup( p, pCube );
        Mvc_CoverAddCubeTail( pCover, pCubeCopy );
    }
    return pCover;
}


Mvc_Cover_t * Mvc_CoverClone( Mvc_Cover_t * p )
{
    Mvc_Cover_t * pCover;
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    pCover                = (Mvc_Cover_t *)ABC_ALLOC( char, sizeof(Mvc_Cover_t) );
#else
    pCover                = (Mvc_Cover_t *)Extra_MmFixedEntryFetch( p->pMem->pManC );
#endif
    pCover->pMem          = p->pMem;
    pCover->nBits         = p->nBits;
    pCover->nWords        = p->nWords;
    pCover->nUnused       = p->nUnused;
    pCover->lCubes.nItems = 0;
    pCover->lCubes.pHead  = NULL;
    pCover->lCubes.pTail  = NULL;
    pCover->nCubesAlloc   = 0;
    pCover->pCubes        = NULL;
    pCover->pMask         = NULL;
    pCover->pLits         = NULL;
    return pCover;
}


Mvc_Cube_t * Mvc_CubeDup( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    Mvc_Cube_t * pCubeCopy;
    pCubeCopy = Mvc_CubeAlloc( pCover );
    Mvc_CubeBitCopy( pCubeCopy, pCube );
    return pCubeCopy;
}


void Mvc_CoverDivisorZeroKernel( Mvc_Cover_t * pCover )
{
    int iLit;
    // find any literal that occurs at least two times
//    iLit = Mvc_CoverAnyLiteral( pCover, NULL );
    iLit = Mvc_CoverWorstLiteral( pCover, NULL );
//    iLit = Mvc_CoverBestLiteral( pCover, NULL );
    if ( iLit == -1 )
        return;
    // derive the cube-free quotient
    Mvc_CoverDivideByLiteralQuo( pCover, iLit ); // the same cover
    Mvc_CoverMakeCubeFree( pCover );             // the same cover
    // call recursively
    Mvc_CoverDivisorZeroKernel( pCover );              // the same cover
}


int Mvc_CoverWorstLiteral( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask )
{
    Mvc_Cube_t * pCube;
    int nWord, nBit;
    int i, iMin, nLitsMin, nLitsCur;
    int fUseFirst = 1;

    // go through each literal
    iMin = -1;
    nLitsMin = 1000000;
    for ( i = 0; i < pCover->nBits; i++ )
        if ( !pMask || Mvc_CubeBitValue(pMask,i) )
        {
            // get the word and bit of this literal
            nWord = Mvc_CubeWhichWord(i);
            nBit  = Mvc_CubeWhichBit(i);
            // go through all the cubes
            nLitsCur = 0;
            Mvc_CoverForEachCube( pCover, pCube )
                if ( pCube->pData[nWord] & (1<<nBit) )
                    nLitsCur++;

            // skip the literal that does not occur or occurs once
            if ( nLitsCur < 2 )
                continue;

            // check if this is the best literal
            if ( fUseFirst )
            {
                if ( nLitsMin > nLitsCur )
                {
                    nLitsMin = nLitsCur;
                    iMin = i;
                }
            }
            else
            {
                if ( nLitsMin >= nLitsCur )
                {
                    nLitsMin = nLitsCur;
                    iMin = i;
                }
            }
        }

    if ( nLitsMin < 1000000 )
        return iMin;
    return -1;
}


void Mvc_CoverDivideByLiteralQuo( Mvc_Cover_t * pCover, int iLit )
{
    Mvc_Cube_t * pCube, * pCube2, * pPrev;
    // delete those cubes that do not have this literal
    // remove this literal from other cubes
    pPrev = NULL;
    Mvc_CoverForEachCubeSafe( pCover, pCube, pCube2 )
    {
        if ( Mvc_CubeBitValue( pCube, iLit ) == 0 )
        { // delete the cube from the cover
            Mvc_CoverDeleteCube( pCover, pPrev, pCube );
            Mvc_CubeFree( pCover, pCube );
            // don't update the previous cube
        }
        else
        { // delete this literal from the cube
            Mvc_CubeBitRemove( pCube, iLit );
            // update the previous cube
            pPrev = pCube;
        }
    }
}


void Mvc_CoverMakeCubeFree( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    // get the common cube
    Mvc_CoverAllocateMask( pCover );
    Mvc_CoverCommonCube( pCover, pCover->pMask );
    // remove this cube from the cubes in the cover
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubeBitSharp( pCube, pCube, pCover->pMask );
}


void Mvc_CoverAllocateMask( Mvc_Cover_t * pCover )
{
    if ( pCover->pMask == NULL )
        pCover->pMask = Mvc_CubeAlloc( pCover );
}


void Mvc_CoverCommonCube( Mvc_Cover_t * pCover, Mvc_Cube_t * pComCube )
{
    Mvc_Cube_t * pCube;
    // clean the support
    Mvc_CubeBitFill( pComCube );
    // collect the support
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubeBitAnd( pComCube, pComCube, pCube );
}


void         Mvc_CoverSetCubeNum( Mvc_Cover_t * pCover,int nItems )           { pCover->lCubes.nItems = nItems; }
void         Mvc_CoverSetCubeHead( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube ) { pCover->lCubes.pHead = pCube;   }
void         Mvc_CoverSetCubeTail( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube ) { pCover->lCubes.pTail = pCube;   }
void         Mvc_CoverSetCubeList( Mvc_Cover_t * pCover, Mvc_List_t * pList ) { pCover->lCubes = *pList;        }


Mvc_Cube_t * Mvc_ListGetTailFromHead( Mvc_Cube_t * pHead )
{
    Mvc_Cube_t * pCube, * pTail;
    for ( pTail = pCube = pHead;
          pCube;
          pTail = pCube, pCube = Mvc_CubeReadNext(pCube) );
    return pTail;
}


void Mvc_CoverDivideInternal( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t ** ppQuo, Mvc_Cover_t ** ppRem )
{
    Mvc_Cover_t * pQuo, * pRem;
    Mvc_Cube_t * pCubeC, * pCubeD, * pCubeCopy;
    Mvc_Cube_t * pCube1, * pCube2;
    int * pGroups, nGroups;    // the cube groups
    int nCubesC, nCubesD, nMerges, iCubeC, iCubeD;
    int iMerge = -1; // Suppress "might be used uninitialized"
    int fSkipG, GroupSize, g, c, RetValue;
    int nCubes;

    // get cover sizes
    nCubesD = Mvc_CoverReadCubeNum( pDiv );
    nCubesC = Mvc_CoverReadCubeNum( pCover );

    // check trivial cases
    if ( nCubesD == 1 )
    {
        if ( Mvc_CoverIsOneLiteral( pDiv ) )
            Mvc_CoverDivideByLiteral( pCover, pDiv, ppQuo, ppRem );
        else
            Mvc_CoverDivideByCube( pCover, pDiv, ppQuo, ppRem );
        return;
    }

    // create the divisor and the remainder
    pQuo = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );
    pRem = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );

    // get the support of the divisor
    Mvc_CoverAllocateMask( pDiv );
    Mvc_CoverSupport( pDiv, pDiv->pMask );

    // sort the cubes of the divisor
    Mvc_CoverSort( pDiv, NULL, Mvc_CubeCompareInt );
    // sort the cubes of the cover
    Mvc_CoverSort( pCover, pDiv->pMask, Mvc_CubeCompareIntOutsideAndUnderMask );

    // allocate storage for cube groups
    pGroups = MEM_ALLOC( pCover->pMem, int, nCubesC + 1 );

    // mask contains variables in the support of Div
    // split the cubes into groups using the mask
    Mvc_CoverList2Array( pCover );
    Mvc_CoverList2Array( pDiv );
    pGroups[0] = 0;
    nGroups    = 1;
    for ( c = 1; c < nCubesC; c++ )
    {
        // get the cubes
        pCube1 = pCover->pCubes[c-1];
        pCube2 = pCover->pCubes[c  ];
        // compare the cubes
        Mvc_CubeBitEqualOutsideMask( RetValue, pCube1, pCube2, pDiv->pMask );
        if ( !RetValue )
            pGroups[nGroups++] = c;
    }
    // finish off the last group
    pGroups[nGroups] = nCubesC;

    // consider each group separately and decide
    // whether it can produce a quotient cube
    nCubes = 0;
    for ( g = 0; g < nGroups; g++ )
    {
        // if the group has less than nCubesD cubes,
        // there is no way it can produce the quotient cube
        // copy the cubes to the remainder
        GroupSize = pGroups[g+1] - pGroups[g];
        if ( GroupSize < nCubesD )
        {
            for ( c = pGroups[g]; c < pGroups[g+1]; c++ )
            {
                pCubeCopy = Mvc_CubeDup( pRem, pCover->pCubes[c] );
                Mvc_CoverAddCubeTail( pRem, pCubeCopy );
                nCubes++;
            }
            continue;
        }

        // mark the cubes as those that should be added to the remainder
        for ( c = pGroups[g]; c < pGroups[g+1]; c++ )
            Mvc_CubeSetSize( pCover->pCubes[c], 1 );

        // go through the cubes in the group and at the same time
        // go through the cubes in the divisor
        iCubeD  = 0;
        iCubeC  = 0;
        pCubeD  = pDiv->pCubes[iCubeD++];
        pCubeC  = pCover->pCubes[pGroups[g]+iCubeC++];
        fSkipG  = 0;
        nMerges = 0;

        while ( 1 )
        {
            // compare the topmost cubes in F and in D
            RetValue = Mvc_CubeCompareIntUnderMask( pCubeC, pCubeD, pDiv->pMask );
            // cube are ordered in increasing order of their int value
            if ( RetValue == -1 ) // pCubeC is above pCubeD
            {  // cube in C should be added to the remainder
                // check that there is enough cubes in the group
                if ( GroupSize - iCubeC < nCubesD - nMerges )
                {
                    fSkipG = 1;
                    break;
                }
                // get the next cube in the cover
                pCubeC = pCover->pCubes[pGroups[g]+iCubeC++];
                continue;
            }
            if ( RetValue == 1 ) // pCubeD is above pCubeC
            { // given cube in D does not have a corresponding cube in the cover
                fSkipG = 1;
                break;
            }
            // mark the cube as the one that should NOT be added to the remainder
            Mvc_CubeSetSize( pCubeC, 0 );
            // remember this merged cube
            iMerge = iCubeC-1;
            nMerges++;

            // stop if we considered the last cube of the group
            if ( iCubeD == nCubesD )
                break;

            // advance the cube of the divisor
            assert( iCubeD < nCubesD );
            pCubeD = pDiv->pCubes[iCubeD++];

            // advance the cube of the group
            assert( pGroups[g]+iCubeC < nCubesC );
            pCubeC = pCover->pCubes[pGroups[g]+iCubeC++];
        }

        if ( fSkipG )
        {
            // the group has failed, add all the cubes to the remainder
            for ( c = pGroups[g]; c < pGroups[g+1]; c++ )
            {
                pCubeCopy = Mvc_CubeDup( pRem, pCover->pCubes[c] );
                Mvc_CoverAddCubeTail( pRem, pCubeCopy );
                nCubes++;
            }
            continue;
        }

        // the group has worked, add left-over cubes to the remainder
        for ( c = pGroups[g]; c < pGroups[g+1]; c++ )
        {
            pCubeC = pCover->pCubes[c];
            if ( Mvc_CubeReadSize(pCubeC) )
            {
                pCubeCopy = Mvc_CubeDup( pRem, pCubeC );
                Mvc_CoverAddCubeTail( pRem, pCubeCopy );
                nCubes++;
            }
        }

        // create the quotient cube
        pCube1 = Mvc_CubeAlloc( pQuo );
        Mvc_CubeBitSharp( pCube1, pCover->pCubes[pGroups[g]+iMerge], pDiv->pMask );
        // add the cube to the quotient
        Mvc_CoverAddCubeTail( pQuo, pCube1 );
        nCubes += nCubesD;
    }
    assert( nCubes == nCubesC );

    // deallocate the memory
    MEM_FREE( pCover->pMem, int, nCubesC + 1, pGroups );

    // return the results
    *ppRem = pRem;
    *ppQuo = pQuo;
//    Mvc_CoverVerifyDivision( pCover, pDiv, pQuo, pRem );
}


int Mvc_CoverIsCubeFree( Mvc_Cover_t * pCover )
{
    int Result;
    // get the common cube
    Mvc_CoverAllocateMask( pCover );
    Mvc_CoverCommonCube( pCover, pCover->pMask );
    // check whether the common cube is empty
    Mvc_CubeBitEmpty( Result, pCover->pMask );
    return Result;
}


Mvc_Cover_t * Mvc_CoverCommonCubeCover( Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pRes;
    Mvc_Cube_t * pCube;
    // create the new cover
    pRes = Mvc_CoverClone( pCover );
    // get the new cube
    pCube = Mvc_CubeAlloc( pRes );
    // get the common cube
    Mvc_CoverCommonCube( pCover, pCube );
    // add the cube to the cover
    Mvc_CoverAddCubeTail( pRes, pCube );
    return pRes;
}


Mvc_Cover_t * Mvc_CoverBestLiteralCover( Mvc_Cover_t * pCover, Mvc_Cover_t * pSimple )
{
    Mvc_Cover_t * pCoverNew;
    Mvc_Cube_t * pCubeNew;
    Mvc_Cube_t * pCubeS;
    int iLitBest;

    // create the new cover
    pCoverNew = Mvc_CoverClone( pCover );
    // get the new cube
    pCubeNew = Mvc_CubeAlloc( pCoverNew );
    // clean the cube
    Mvc_CubeBitClean( pCubeNew );

    // get the first cube of pSimple
    assert( Mvc_CoverReadCubeNum(pSimple) == 1 );
    pCubeS = Mvc_CoverReadCubeHead( pSimple );
    // find the best literal among those of pCubeS
    iLitBest = Mvc_CoverBestLiteral( pCover, pCubeS );

    // insert this literal into the cube
    Mvc_CubeBitInsert( pCubeNew, iLitBest );
    // add the cube to the cover
    Mvc_CoverAddCubeTail( pCoverNew, pCubeNew );
    return pCoverNew;
}


void Mvc_CoverDivideByLiteral( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t ** ppQuo, Mvc_Cover_t ** ppRem )
{
    Mvc_Cover_t * pQuo, * pRem;
    Mvc_Cube_t * pCubeC, * pCubeCopy;
    int iLit;

    // get the only cube of D
    assert( Mvc_CoverReadCubeNum(pDiv) == 1 );

    // start the quotient and the remainder
    pQuo = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );
    pRem = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );

    // get the first and only literal in the divisor cube
    iLit = Mvc_CoverFirstCubeFirstLit( pDiv );

    // iterate through the cubes in the cover
    Mvc_CoverForEachCube( pCover, pCubeC )
    {
        // copy the cube
        pCubeCopy = Mvc_CubeDup( pCover, pCubeC );
        // add the cube to the quotient or to the remainder depending on the literal
        if ( Mvc_CubeBitValue( pCubeCopy, iLit ) )
        {   // remove the literal
            Mvc_CubeBitRemove( pCubeCopy, iLit );
            // add the cube ot the quotient
            Mvc_CoverAddCubeTail( pQuo, pCubeCopy );
        }
        else
        {   // add the cube ot the remainder
            Mvc_CoverAddCubeTail( pRem, pCubeCopy );
        }
    }
    // return the results
    *ppRem = pRem;
    *ppQuo = pQuo;
}


void Mvc_CoverSupport( Mvc_Cover_t * pCover, Mvc_Cube_t * pSupp )
{
    Mvc_Cube_t * pCube;
    // clean the support
    Mvc_CubeBitClean( pSupp );
    // collect the support
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubeBitOr( pSupp, pSupp, pCube );
}


int Mvc_CoverIsOneLiteral( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int iBit, Counter, Value;
    if ( Mvc_CoverReadCubeNum(pCover) != 1 )
        return 0;
    pCube = Mvc_CoverReadCubeHead(pCover);
    // count literals
    Counter = 0;
    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
    {
        if ( Value )
        {
            if ( Counter++ )
                return 0;
        }
    }
    return 1;
}


void Mvc_CoverDivideByCube( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t ** ppQuo, Mvc_Cover_t ** ppRem )
{
    Mvc_Cover_t * pQuo, * pRem;
    Mvc_Cube_t * pCubeC, * pCubeD, * pCubeCopy;
    int CompResult;

    // get the only cube of D
    assert( Mvc_CoverReadCubeNum(pDiv) == 1 );

    // start the quotient and the remainder
    pQuo = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );
    pRem = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );

    // get the first and only cube of the divisor
    pCubeD = Mvc_CoverReadCubeHead( pDiv );

    // iterate through the cubes in the cover
    Mvc_CoverForEachCube( pCover, pCubeC )
    {
        // check the containment of literals from pCubeD in pCube
        Mvc_Cube2BitNotImpl( CompResult, pCubeD, pCubeC );
        if ( !CompResult )
        { // this cube belongs to the quotient
            // alloc the cube
            pCubeCopy = Mvc_CubeAlloc( pQuo );
            // clean the support of D
            Mvc_CubeBitSharp( pCubeCopy, pCubeC, pCubeD );
            // add the cube to the quotient
            Mvc_CoverAddCubeTail( pQuo, pCubeCopy );
        }
        else
        {
            // copy the cube
            pCubeCopy = Mvc_CubeDup( pRem, pCubeC );
            // add the cube to the remainder
            Mvc_CoverAddCubeTail( pRem, pCubeCopy );
        }
    }
    // return the results
    *ppRem = pRem;
    *ppQuo = pQuo;
}


int Mvc_CubeCompareInt( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask )
{
    if ( Mvc_Cube1Words(pC1) )
    {
        if ( pC1->pData[0] < pC2->pData[0] )
            return -1;
        if ( pC1->pData[0] > pC2->pData[0] )
            return 1;
        return 0;
    }
    else if ( Mvc_Cube2Words(pC1) )
    {
        if ( pC1->pData[1] < pC2->pData[1] )
            return -1;
        if ( pC1->pData[1] > pC2->pData[1] )
            return 1;
        if ( pC1->pData[0] < pC2->pData[0] )
            return -1;
        if ( pC1->pData[0] > pC2->pData[0] )
            return 1;
        return 0;
    }
    else
    {
        int i = Mvc_CubeReadLast(pC1);
        for(; i >= 0; i--)
        {
            if ( pC1->pData[i] < pC2->pData[i] )
                return -1;
            if ( pC1->pData[i] > pC2->pData[i] )
                return 1;
        }
        return 0;
    }
}


int Mvc_CubeCompareIntOutsideAndUnderMask( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask )
{
    unsigned uBits1, uBits2;

    if ( Mvc_Cube1Words(pC1) )
    {
        // compare the cubes outside the mask
        uBits1 = pC1->pData[0] & ~(pMask->pData[0]);
        uBits2 = pC2->pData[0] & ~(pMask->pData[0]);
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;

        // compare the cubes under the mask
        uBits1 = pC1->pData[0] & pMask->pData[0];
        uBits2 = pC2->pData[0] & pMask->pData[0];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;
        // cubes are equal
        // should never happen
        assert( 0 );
        return 0;
    }
    else if ( Mvc_Cube2Words(pC1) )
    {
        // compare the cubes outside the mask
        uBits1 = pC1->pData[1] & ~(pMask->pData[1]);
        uBits2 = pC2->pData[1] & ~(pMask->pData[1]);
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;

        uBits1 = pC1->pData[0] & ~(pMask->pData[0]);
        uBits2 = pC2->pData[0] & ~(pMask->pData[0]);
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;

        // compare the cubes under the mask
        uBits1 = pC1->pData[1] & pMask->pData[1];
        uBits2 = pC2->pData[1] & pMask->pData[1];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;

        uBits1 = pC1->pData[0] & pMask->pData[0];
        uBits2 = pC2->pData[0] & pMask->pData[0];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;

        // cubes are equal
        // should never happen
        assert( 0 );
        return 0;
    }
    else
    {
        int i;

        // compare the cubes outside the mask
        for( i = Mvc_CubeReadLast(pC1); i >= 0; i-- )
        {
            uBits1 = pC1->pData[i] & ~(pMask->pData[i]);
            uBits2 = pC2->pData[i] & ~(pMask->pData[i]);
            if ( uBits1 < uBits2 )
                return -1;
            if ( uBits1 > uBits2 )
                return 1;
        }
        // compare the cubes under the mask
        for( i = Mvc_CubeReadLast(pC1); i >= 0; i-- )
        {
            uBits1 = pC1->pData[i] & pMask->pData[i];
            uBits2 = pC2->pData[i] & pMask->pData[i];
            if ( uBits1 < uBits2 )
                return -1;
            if ( uBits1 > uBits2 )
                return 1;
        }
/*
        {
            Mvc_Cover_t * pCover;
            pCover = Mvc_CoverAlloc( NULL, 96 );
            Mvc_CubePrint( pCover, pC1 );
            Mvc_CubePrint( pCover, pC2 );
            Mvc_CubePrint( pCover, pMask );
        }
*/
        // cubes are equal
        // should never happen
        assert( 0 );
        return 0;
    }
}


void Mvc_CoverList2Array( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int Counter;
    // resize storage if necessary
    Mvc_CoverAllocateArrayCubes( pCover );
    // iterate through the cubes
    Counter = 0;
    Mvc_CoverForEachCube( pCover, pCube )
        pCover->pCubes[ Counter++ ] = pCube;
    assert( Counter == Mvc_CoverReadCubeNum(pCover) );
}


int Mvc_CubeCompareIntUnderMask( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask )
{
    unsigned uBits1, uBits2;

    // compare the cubes under the mask
    if ( Mvc_Cube1Words(pC1) )
    {
        uBits1 = pC1->pData[0] & pMask->pData[0];
        uBits2 = pC2->pData[0] & pMask->pData[0];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;
        // cubes are equal
        return 0;
    }
    else if ( Mvc_Cube2Words(pC1) )
    {
        uBits1 = pC1->pData[1] & pMask->pData[1];
        uBits2 = pC2->pData[1] & pMask->pData[1];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;
        uBits1 = pC1->pData[0] & pMask->pData[0];
        uBits2 = pC2->pData[0] & pMask->pData[0];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;
        return 0;
    }
    else
    {
        int i = Mvc_CubeReadLast(pC1);
        for(; i >= 0; i--)
        {
            uBits1 = pC1->pData[i] & pMask->pData[i];
            uBits2 = pC2->pData[i] & pMask->pData[i];
            if ( uBits1 < uBits2 )
                return -1;
            if ( uBits1 > uBits2 )
                return 1;
        }
        return 0;
    }
}


int Mvc_CoverBestLiteral( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask )
{
    Mvc_Cube_t * pCube;
    int nWord, nBit;
    int i, iMax, nLitsMax, nLitsCur;
    int fUseFirst = 1;

    // go through each literal
    iMax = -1;
    nLitsMax = -1;
    for ( i = 0; i < pCover->nBits; i++ )
        if ( !pMask || Mvc_CubeBitValue(pMask,i) )
        {
            // get the word and bit of this literal
            nWord = Mvc_CubeWhichWord(i);
            nBit  = Mvc_CubeWhichBit(i);
            // go through all the cubes
            nLitsCur = 0;
            Mvc_CoverForEachCube( pCover, pCube )
                if ( pCube->pData[nWord] & (1<<nBit) )
                    nLitsCur++;

            // check if this is the best literal
            if ( fUseFirst )
            {
                if ( nLitsMax < nLitsCur )
                {
                    nLitsMax = nLitsCur;
                    iMax = i;
                }
            }
            else
            {
                if ( nLitsMax <= nLitsCur )
                {
                    nLitsMax = nLitsCur;
                    iMax = i;
                }
            }
        }

    if ( nLitsMax > 1 )
        return iMax;
    return -1;
}


int Mvc_CoverFirstCubeFirstLit( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int iBit, Value;

    // get the first cube
    pCube = Mvc_CoverReadCubeHead( pCover );
    // get the first literal
    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
        if ( Value )
            return iBit;
    return -1;
}


void Mvc_CoverAllocateArrayCubes( Mvc_Cover_t * pCover )
{
    if ( pCover->nCubesAlloc < pCover->lCubes.nItems )
    {
        if ( pCover->nCubesAlloc > 0 )
            MEM_FREE( pCover->pMem, Mvc_Cube_t *, pCover->nCubesAlloc, pCover->pCubes );
        pCover->nCubesAlloc = pCover->lCubes.nItems;
        pCover->pCubes = MEM_ALLOC( pCover->pMem, Mvc_Cube_t *, pCover->nCubesAlloc );
    }
}


void Mvc_ManagerFree( Mvc_Manager_t * p )
{
    Extra_MmFixedStop( p->pMan1 );
    Extra_MmFixedStop( p->pMan2 );
    Extra_MmFixedStop( p->pMan4 );
    Extra_MmFixedStop( p->pManC );
    ABC_FREE( p );
}
