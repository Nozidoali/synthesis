#include "abc_name.h"


static unsigned Nm_HashNumber( int Num, int TableSize )
{
    unsigned Key = 0;
    Key ^= ( Num        & 0xFF) * 7937;
    Key ^= ((Num >>  8) & 0xFF) * 2971;
    Key ^= ((Num >> 16) & 0xFF) * 911;
    Key ^= ((Num >> 24) & 0xFF) * 353;
    return Key % TableSize;
}


static unsigned Nm_HashString( char * pName, int TableSize )
{
    static int s_Primes[10] = {
        1291, 1699, 2357, 4177, 5147,
        5647, 6343, 7103, 7873, 8147
    };
    unsigned i, Key = 0;
    for ( i = 0; pName[i] != '\0'; i++ )
        Key ^= s_Primes[i%10]*pName[i]*pName[i];
    return Key % TableSize;
}


Nm_Man_t * Nm_ManCreate( int nSize )
{
    Nm_Man_t * p;
    // allocate the table
    p = ABC_ALLOC( Nm_Man_t, 1 );
    memset( p, 0, sizeof(Nm_Man_t) );
    // set the parameters
    p->nSizeFactor   = 2; // determined the limit on the grow of data before the table resizes
    p->nGrowthFactor = 3; // determined how much the table grows after resizing
    // allocate and clean the bins
    p->nBins = Abc_PrimeCudd(nSize);
    p->pBinsI2N = ABC_ALLOC( Nm_Entry_t *, p->nBins );
    p->pBinsN2I = ABC_ALLOC( Nm_Entry_t *, p->nBins );
    memset( p->pBinsI2N, 0, sizeof(Nm_Entry_t *) * p->nBins );
    memset( p->pBinsN2I, 0, sizeof(Nm_Entry_t *) * p->nBins );
    // start the memory manager
    p->pMem = Extra_MmFlexStart();
    return p;
}


void Nm_ManFree( Nm_Man_t * p )
{
    Extra_MmFlexStop( p->pMem );
    ABC_FREE( p->pBinsI2N );
    ABC_FREE( p->pBinsN2I );
    ABC_FREE( p );
}


int Nm_ManFindIdByName( Nm_Man_t * p, char * pName, int Type )
{
    Nm_Entry_t * pEntry;
    if ( (pEntry = Nm_ManTableLookupName(p, pName, Type)) )
        return pEntry->ObjId;
    return -1;
}


Nm_Entry_t * Nm_ManTableLookupName( Nm_Man_t * p, char * pName, int Type )
{
    Nm_Entry_t * pEntry, * pTemp;
    for ( pEntry = p->pBinsN2I[ Nm_HashString(pName, p->nBins) ]; pEntry; pEntry = pEntry->pNextN2I )
    {
        // check the entry itself
        if ( !strcmp(pEntry->Name, pName) && (Type == -1 || pEntry->Type == (unsigned)Type) )
            return pEntry;
        // if there is no namesakes, continue
        if ( pEntry->pNameSake == NULL )
            continue;
        // check the list of namesakes
        for ( pTemp = pEntry->pNameSake; pTemp != pEntry; pTemp = pTemp->pNameSake )
            if ( !strcmp(pTemp->Name, pName) && (Type == -1 || pTemp->Type == (unsigned)Type) )
                return pTemp;
    }
    return NULL;
}


char * Nm_ManStoreIdName( Nm_Man_t * p, int ObjId, int Type, char * pName, char * pSuffix )
{
    Nm_Entry_t * pEntry;
    int RetValue, nEntrySize;
    // check if the object with this ID is already stored
    if ( (pEntry = Nm_ManTableLookupId(p, ObjId)) )
    {
        printf( "Nm_ManStoreIdName(): Entry with the same ID already exists.\n" );
        return NULL;
    }
    // create a new entry
    nEntrySize = sizeof(Nm_Entry_t) + strlen(pName) + (pSuffix?strlen(pSuffix):0) + 1;
//    nEntrySize = (nEntrySize / 4 + ((nEntrySize % 4) > 0)) * 4;
    nEntrySize = (nEntrySize / sizeof(char*) + ((nEntrySize % sizeof(char*)) > 0)) * sizeof(char*); // added by Saurabh on Sep 3, 2009
    pEntry = (Nm_Entry_t *)Extra_MmFlexEntryFetch( p->pMem, nEntrySize );
    pEntry->pNextI2N = pEntry->pNextN2I = pEntry->pNameSake = NULL;
    pEntry->ObjId = ObjId;
    pEntry->Type = Type;
    sprintf( pEntry->Name, "%s%s", pName, pSuffix? pSuffix : "" );
    // add the entry to the hash table
    RetValue = Nm_ManTableAdd( p, pEntry );
    assert( RetValue == 1 );
    return pEntry->Name;
}


Nm_Entry_t * Nm_ManTableLookupId( Nm_Man_t * p, int ObjId )
{
    Nm_Entry_t * pEntry;
    for ( pEntry = p->pBinsI2N[ Nm_HashNumber(ObjId, p->nBins) ]; pEntry; pEntry = pEntry->pNextI2N )
        if ( pEntry->ObjId == (unsigned)ObjId )
            return pEntry;
    return NULL;
}


int Nm_ManTableAdd( Nm_Man_t * p, Nm_Entry_t * pEntry )
{
    Nm_Entry_t ** ppSpot, * pOther;
    // resize the tables if needed
    if ( p->nEntries > p->nBins * p->nSizeFactor )
        Nm_ManResize( p );
    // add the entry to the table Id->Name
    assert( Nm_ManTableLookupId(p, pEntry->ObjId) == NULL );
    ppSpot = p->pBinsI2N + Nm_HashNumber(pEntry->ObjId, p->nBins);
    pEntry->pNextI2N = *ppSpot;
    *ppSpot = pEntry;
    // check if an entry with the same name already exists
    if ( (pOther = Nm_ManTableLookupName(p, pEntry->Name, -1)) )
    {
        // entry with the same name already exists - add it to the ring
        pEntry->pNameSake = pOther->pNameSake? pOther->pNameSake : pOther;
        pOther->pNameSake = pEntry;
    }
    else
    {
        // entry with the same name does not exist - add it to the table
        ppSpot = p->pBinsN2I + Nm_HashString(pEntry->Name, p->nBins);
        pEntry->pNextN2I = *ppSpot;
        *ppSpot = pEntry;
    }
    // report successfully added entry
    p->nEntries++;
    return 1;
}


void Nm_ManResize( Nm_Man_t * p )
{
    Nm_Entry_t ** pBinsNewI2N, ** pBinsNewN2I, * pEntry, * pEntry2, ** ppSpot;
    int nBinsNew, Counter, e;
//     abctime clk;

// clk = Abc_Clock();
    // get the new table size
    nBinsNew = Abc_PrimeCudd( p->nGrowthFactor * p->nBins );
    // allocate a new array
    pBinsNewI2N = ABC_ALLOC( Nm_Entry_t *, nBinsNew );
    pBinsNewN2I = ABC_ALLOC( Nm_Entry_t *, nBinsNew );
    memset( pBinsNewI2N, 0, sizeof(Nm_Entry_t *) * nBinsNew );
    memset( pBinsNewN2I, 0, sizeof(Nm_Entry_t *) * nBinsNew );
    // rehash entries in Id->Name table
    Counter = 0;
    for ( e = 0; e < p->nBins; e++ )
        for ( pEntry = p->pBinsI2N[e], pEntry2 = pEntry? pEntry->pNextI2N : NULL;
              pEntry; pEntry = pEntry2, pEntry2 = pEntry? pEntry->pNextI2N : NULL )
            {
                ppSpot = pBinsNewI2N + Nm_HashNumber(pEntry->ObjId, nBinsNew);
                pEntry->pNextI2N = *ppSpot;
                *ppSpot = pEntry;
                Counter++;
            }
    // rehash entries in Name->Id table
    for ( e = 0; e < p->nBins; e++ )
        for ( pEntry = p->pBinsN2I[e], pEntry2 = pEntry? pEntry->pNextN2I : NULL;
              pEntry; pEntry = pEntry2, pEntry2 = pEntry? pEntry->pNextN2I : NULL )
            {
                ppSpot = pBinsNewN2I + Nm_HashString(pEntry->Name, nBinsNew);
                pEntry->pNextN2I = *ppSpot;
                *ppSpot = pEntry;
            }
    assert( Counter == p->nEntries );
//    printf( "Increasing the structural table size from %6d to %6d. ", p->nBins, nBinsNew );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    // replace the table and the parameters
    ABC_FREE( p->pBinsI2N );
    ABC_FREE( p->pBinsN2I );
    p->pBinsI2N = pBinsNewI2N;
    p->pBinsN2I = pBinsNewN2I;
    p->nBins = nBinsNew;
//    Nm_ManProfile( p );
}


char * Nm_ManCreateUniqueName( Nm_Man_t * p, int ObjId )
{
    static char NameStr[1000];
    Nm_Entry_t * pEntry;
    int i;
    if ( (pEntry = Nm_ManTableLookupId(p, ObjId)) )
        return pEntry->Name;
    sprintf( NameStr, "n%d", ObjId );
    for ( i = 1; Nm_ManTableLookupName(p, NameStr, -1); i++ )
        sprintf( NameStr, "n%d_%d", ObjId, i );
    return NameStr;
}


char * Nm_ManFindNameById( Nm_Man_t * p, int ObjId )
{
    Nm_Entry_t * pEntry;
    if ( (pEntry = Nm_ManTableLookupId(p, ObjId)) )
        return pEntry->Name;
    return NULL;
}


void Nm_ManDeleteIdName( Nm_Man_t * p, int ObjId )
{
    Nm_Entry_t * pEntry;
    pEntry = Nm_ManTableLookupId(p, ObjId);
    if ( pEntry == NULL )
    {
        printf( "Nm_ManDeleteIdName(): This entry is not in the table.\n" );
        return;
    }
    // remove entry from the table
    Nm_ManTableDelete( p, ObjId );
}


int Nm_ManTableDelete( Nm_Man_t * p, int ObjId )
{
    Nm_Entry_t ** ppSpot, * pEntry, * pPrev;
    int fRemoved;
    p->nEntries--;
    // remove the entry from the table Id->Name
    assert( Nm_ManTableLookupId(p, ObjId) != NULL );
    ppSpot = p->pBinsI2N + Nm_HashNumber(ObjId, p->nBins);
    while ( (*ppSpot)->ObjId != (unsigned)ObjId )
        ppSpot = &(*ppSpot)->pNextI2N;
    pEntry = *ppSpot;
    *ppSpot = (*ppSpot)->pNextI2N;
    // remove the entry from the table Name->Id
    ppSpot = p->pBinsN2I + Nm_HashString(pEntry->Name, p->nBins);
    while ( *ppSpot && *ppSpot != pEntry )
        ppSpot = &(*ppSpot)->pNextN2I;
    // remember if we found this one in the list
    fRemoved = (*ppSpot != NULL);
    if ( *ppSpot )
    {
        assert( *ppSpot == pEntry );
        *ppSpot = (*ppSpot)->pNextN2I;
    }
    // quit if this entry has no namesakes
    if ( pEntry->pNameSake == NULL )
    {
        assert( fRemoved );
        return 1;
    }
    // remove entry from the ring of namesakes
    assert( pEntry->pNameSake != pEntry );
    for ( pPrev = pEntry; pPrev->pNameSake != pEntry; pPrev = pPrev->pNameSake );
    assert( !strcmp(pPrev->Name, pEntry->Name) );
    assert( pPrev->pNameSake == pEntry );
    if ( pEntry->pNameSake == pPrev ) // two entries in the ring
        pPrev->pNameSake = NULL;
    else
        pPrev->pNameSake = pEntry->pNameSake;
    // reinsert the ring back if we removed its connection with the list in the table
    if ( fRemoved )
    {
        assert( pPrev->pNextN2I == NULL );
        pPrev->pNextN2I = *ppSpot;
        *ppSpot = pPrev;
    }
    return 1;
}


Vec_Int_t * Nm_ManReturnNameIds( Nm_Man_t * p )
{
    Vec_Int_t * vNameIds;
    int i;
    vNameIds = Vec_IntAlloc( p->nEntries );
    for ( i = 0; i < p->nBins; i++ )
        if ( p->pBinsI2N[i] )
            Vec_IntPush( vNameIds, p->pBinsI2N[i]->ObjId );
    return vNameIds;
}


int Nm_ManFindIdByNameTwoTypes( Nm_Man_t * p, char * pName, int Type1, int Type2 )
{
    int iNodeId;
    iNodeId = Nm_ManFindIdByName( p, pName, Type1 );
    if ( iNodeId == -1 )
        iNodeId = Nm_ManFindIdByName( p, pName, Type2 );
    if ( iNodeId == -1 )
        return -1;
    return iNodeId;
}


Vec_Ptr_t * Abc_NodeGetFakeNames( int nNames )
{
    Vec_Ptr_t * vNames;
    char Buffer[5];
    int i;

    vNames = Vec_PtrAlloc( nNames );
    for ( i = 0; i < nNames; i++ )
    {
        if ( nNames < 26 )
        {
            Buffer[0] = 'a' + i;
            Buffer[1] = 0;
        }
        else
        {
            Buffer[0] = 'a' + i%26;
            Buffer[1] = '0' + i/26;
            Buffer[2] = 0;
        }
        Vec_PtrPush( vNames, Extra_UtilStrsav(Buffer) );
    }
    return vNames;
}


void Abc_NodeFreeNames( Vec_Ptr_t * vNames )
{
    int i;
    if ( vNames == NULL )
        return;
    for ( i = 0; i < vNames->nSize; i++ )
        ABC_FREE( vNames->pArray[i] );
    Vec_PtrFree( vNames );
}
