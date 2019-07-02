#include "abc_hop.h"


static unsigned long Hop_Hash( Hop_Obj_t * pObj, int TableSize )
{
    unsigned long Key = Hop_ObjIsExor(pObj) * 1699;
    Key ^= Hop_ObjFanin0(pObj)->Id * 7937;
    Key ^= Hop_ObjFanin1(pObj)->Id * 2971;
    Key ^= Hop_ObjFaninC0(pObj) * 911;
    Key ^= Hop_ObjFaninC1(pObj) * 353;
    return Key % TableSize;
}


static Hop_Obj_t ** Hop_TableFind( Hop_Man_t * p, Hop_Obj_t * pObj )
{
    Hop_Obj_t ** ppEntry;
    assert( Hop_ObjChild0(pObj) && Hop_ObjChild1(pObj) );
    assert( Hop_ObjFanin0(pObj)->Id < Hop_ObjFanin1(pObj)->Id );
    for ( ppEntry = p->pTable + Hop_Hash(pObj, p->nTableSize); *ppEntry; ppEntry = &(*ppEntry)->pNext )
        if ( *ppEntry == pObj )
            return ppEntry;
    assert( *ppEntry == NULL );
    return ppEntry;
}


Hop_Obj_t * Hop_Transfer( Hop_Man_t * pSour, Hop_Man_t * pDest, Hop_Obj_t * pRoot, int nVars )
{
    Hop_Obj_t * pObj;
    int i;
    // solve simple cases
    if ( pSour == pDest )
        return pRoot;
    if ( Hop_ObjIsConst1( Hop_Regular(pRoot) ) )
        return Hop_NotCond( Hop_ManConst1(pDest), Hop_IsComplement(pRoot) );
    // set the PI mapping
    Hop_ManForEachPi( pSour, pObj, i )
    {
        if ( i == nVars )
           break;
        pObj->pData = Hop_IthVar(pDest, i);
    }
    // transfer and set markings
    Hop_Transfer_rec( pDest, Hop_Regular(pRoot) );
    // clear the markings
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
    return Hop_NotCond( (Hop_Obj_t *)Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );
}


Hop_Obj_t * Hop_IthVar( Hop_Man_t * p, int i )
{
    int v;
    for ( v = Hop_ManPiNum(p); v <= i; v++ )
        Hop_ObjCreatePi( p );
    assert( i < Vec_PtrSize(p->vPis) );
    return Hop_ManPi( p, i );
}


Hop_Obj_t * Hop_ObjCreatePi( Hop_Man_t * p )
{
    Hop_Obj_t * pObj;
    pObj = Hop_ManFetchMemory( p );
    pObj->Type = AIG_PI;
    pObj->PioNum = Vec_PtrSize( p->vPis );
    Vec_PtrPush( p->vPis, pObj );
    p->nObjs[AIG_PI]++;
    return pObj;
}


void Hop_ManAddMemory( Hop_Man_t * p )
{
    char * pMemory;
    int i, nBytes;
    assert( sizeof(Hop_Obj_t) <= 64 );
    assert( p->pListFree == NULL );
//    assert( (Hop_ManObjNum(p) & IVY_PAGE_MASK) == 0 );
    // allocate new memory page
    nBytes = sizeof(Hop_Obj_t) * (1<<IVY_PAGE_SIZE) + 64;
    pMemory = ABC_ALLOC( char, nBytes );
    Vec_PtrPush( p->vChunks, pMemory );
    // align memory at the 32-byte boundary
    pMemory = pMemory + 64 - (((int)(ABC_PTRUINT_T)pMemory) & 63);
    // remember the manager in the first entry
    Vec_PtrPush( p->vPages, pMemory );
    // break the memory down into nodes
    p->pListFree = (Hop_Obj_t *)pMemory;
    for ( i = 1; i <= IVY_PAGE_MASK; i++ )
    {
        *((char **)pMemory) = pMemory + sizeof(Hop_Obj_t);
        pMemory += sizeof(Hop_Obj_t);
    }
    *((char **)pMemory) = NULL;
}


void Hop_Transfer_rec( Hop_Man_t * pDest, Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Hop_Transfer_rec( pDest, Hop_ObjFanin0(pObj) );
    Hop_Transfer_rec( pDest, Hop_ObjFanin1(pObj) );
    pObj->pData = Hop_And( pDest, Hop_ObjChild0Copy(pObj), Hop_ObjChild1Copy(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}


Hop_Obj_t * Hop_And( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1 )
{
    Hop_Obj_t * pGhost, * pResult;
//    Hop_Obj_t * pFan0, * pFan1;
    // check trivial cases
    if ( p0 == p1 )
        return p0;
    if ( p0 == Hop_Not(p1) )
        return Hop_Not(p->pConst1);
    if ( Hop_Regular(p0) == p->pConst1 )
        return p0 == p->pConst1 ? p1 : Hop_Not(p->pConst1);
    if ( Hop_Regular(p1) == p->pConst1 )
        return p1 == p->pConst1 ? p0 : Hop_Not(p->pConst1);
    // check if it can be an EXOR gate
//    if ( Hop_ObjIsExorType( p0, p1, &pFan0, &pFan1 ) )
//        return Hop_Exor( p, pFan0, pFan1 );
    // check the table
    pGhost = Hop_ObjCreateGhost( p, p0, p1, AIG_AND );
    if ( (pResult = Hop_TableLookup( p, pGhost )) )
        return pResult;
    return Hop_ObjCreate( p, pGhost );
}


Hop_Obj_t * Hop_Exor( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1 )
{
    return Hop_Or( p, Hop_And(p, p0, Hop_Not(p1)), Hop_And(p, Hop_Not(p0), p1) );
}


Hop_Obj_t * Hop_Or( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1 )
{
    return Hop_Not( Hop_And( p, Hop_Not(p0), Hop_Not(p1) ) );
}


Hop_Obj_t * Hop_TableLookup( Hop_Man_t * p, Hop_Obj_t * pGhost )
{
    Hop_Obj_t * pEntry;
    assert( !Hop_IsComplement(pGhost) );
    assert( Hop_ObjChild0(pGhost) && Hop_ObjChild1(pGhost) );
    assert( Hop_ObjFanin0(pGhost)->Id < Hop_ObjFanin1(pGhost)->Id );
    if ( p->fRefCount && (!Hop_ObjRefs(Hop_ObjFanin0(pGhost)) || !Hop_ObjRefs(Hop_ObjFanin1(pGhost))) )
        return NULL;
    for ( pEntry = p->pTable[Hop_Hash(pGhost, p->nTableSize)]; pEntry; pEntry = pEntry->pNext )
    {
        if ( Hop_ObjChild0(pEntry) == Hop_ObjChild0(pGhost) &&
             Hop_ObjChild1(pEntry) == Hop_ObjChild1(pGhost) &&
             Hop_ObjType(pEntry) == Hop_ObjType(pGhost) )
            return pEntry;
    }
    return NULL;
}


Hop_Obj_t * Hop_ObjCreate( Hop_Man_t * p, Hop_Obj_t * pGhost )
{
    Hop_Obj_t * pObj;
    assert( !Hop_IsComplement(pGhost) );
    assert( Hop_ObjIsNode(pGhost) );
    assert( pGhost == &p->Ghost );
    // get memory for the new object
    pObj = Hop_ManFetchMemory( p );
    pObj->Type = pGhost->Type;
    // add connections
    Hop_ObjConnect( p, pObj, pGhost->pFanin0, pGhost->pFanin1 );
    // update node counters of the manager
    p->nObjs[Hop_ObjType(pObj)]++;
    assert( pObj->pData == NULL );
    return pObj;
}


void Hop_ObjConnect( Hop_Man_t * p, Hop_Obj_t * pObj, Hop_Obj_t * pFan0, Hop_Obj_t * pFan1 )
{
    assert( !Hop_IsComplement(pObj) );
    assert( Hop_ObjIsNode(pObj) );
    // add the first fanin
    pObj->pFanin0 = pFan0;
    pObj->pFanin1 = pFan1;
    // increment references of the fanins and add their fanouts
    if ( p->fRefCount )
    {
        if ( pFan0 != NULL )
            Hop_ObjRef( Hop_ObjFanin0(pObj) );
        if ( pFan1 != NULL )
            Hop_ObjRef( Hop_ObjFanin1(pObj) );
    }
    else
        pObj->nRefs = Hop_ObjLevelNew( pObj );
    // set the phase
    pObj->fPhase = Hop_ObjPhaseCompl(pFan0) & Hop_ObjPhaseCompl(pFan1);
    // add the node to the structural hash table
    Hop_TableInsert( p, pObj );
}


void Hop_TableInsert( Hop_Man_t * p, Hop_Obj_t * pObj )
{
    Hop_Obj_t ** ppPlace;
    assert( !Hop_IsComplement(pObj) );
    assert( Hop_TableLookup(p, pObj) == NULL );
    if ( (pObj->Id & 0xFF) == 0 && 2 * p->nTableSize < Hop_ManNodeNum(p) )
        Hop_TableResize( p );
    ppPlace = Hop_TableFind( p, pObj );
    assert( *ppPlace == NULL );
    *ppPlace = pObj;
}


void Hop_TableResize( Hop_Man_t * p )
{
    Hop_Obj_t * pEntry, * pNext;
    Hop_Obj_t ** pTableOld, ** ppPlace;
    int nTableSizeOld, Counter, nEntries, i;
//     abctime clk;
// clk = Abc_Clock();
    // save the old table
    pTableOld = p->pTable;
    nTableSizeOld = p->nTableSize;
    // get the new table
    p->nTableSize = Abc_PrimeCudd( 2 * Hop_ManNodeNum(p) );
    p->pTable = ABC_ALLOC( Hop_Obj_t *, p->nTableSize );
    memset( p->pTable, 0, sizeof(Hop_Obj_t *) * p->nTableSize );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < nTableSizeOld; i++ )
    for ( pEntry = pTableOld[i], pNext = pEntry? pEntry->pNext : NULL; pEntry; pEntry = pNext, pNext = pEntry? pEntry->pNext : NULL )
    {
        // get the place where this entry goes in the table
        ppPlace = Hop_TableFind( p, pEntry );
        assert( *ppPlace == NULL ); // should not be there
        // add the entry to the list
        *ppPlace = pEntry;
        pEntry->pNext = NULL;
        Counter++;
    }
    nEntries = Hop_ManNodeNum(p);
    assert( Counter == nEntries );
//    printf( "Increasing the structural table size from %6d to %6d. ", nTableSizeOld, p->nTableSize );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    // replace the table and the parameters
    ABC_FREE( pTableOld );
}


void Hop_ConeUnmark_rec( Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || !Hop_ObjIsMarkA(pObj) )
        return;
    Hop_ConeUnmark_rec( Hop_ObjFanin0(pObj) );
    Hop_ConeUnmark_rec( Hop_ObjFanin1(pObj) );
    assert( Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjClearMarkA( pObj );
}


void Hop_ManStartMemory( Hop_Man_t * p )
{
    p->vChunks = Vec_PtrAlloc( 128 );
    p->vPages = Vec_PtrAlloc( 128 );
}


Hop_Man_t * Hop_ManStart()
{
    Hop_Man_t * p;
    // start the manager
    p = ABC_ALLOC( Hop_Man_t, 1 );
    memset( p, 0, sizeof(Hop_Man_t) );
    // perform initializations
    p->nTravIds = 1;
    p->fRefCount = 1;
    p->fCatchExor = 0;
    // allocate arrays for nodes
    p->vPis = Vec_PtrAlloc( 100 );
    p->vPos = Vec_PtrAlloc( 100 );
    // prepare the internal memory manager
    Hop_ManStartMemory( p );
    // create the constant node
    p->pConst1 = Hop_ManFetchMemory( p );
    p->pConst1->Type = AIG_CONST1;
    p->pConst1->fPhase = 1;
    p->nCreated = 1;
    // start the table
//    p->nTableSize = 107;
    p->nTableSize = 10007;
    p->pTable = ABC_ALLOC( Hop_Obj_t *, p->nTableSize );
    memset( p->pTable, 0, sizeof(Hop_Obj_t *) * p->nTableSize );
    return p;
}


void Hop_ManStop( Hop_Man_t * p )
{
    Hop_Obj_t * pObj;
    int i;
    // make sure the nodes have clean marks
    pObj = Hop_ManConst1(p);
    assert( !pObj->fMarkA && !pObj->fMarkB );
    Hop_ManForEachPi( p, pObj, i )
        assert( !pObj->fMarkA && !pObj->fMarkB );
    Hop_ManForEachPo( p, pObj, i )
        assert( !pObj->fMarkA && !pObj->fMarkB );
    Hop_ManForEachNode( p, pObj, i )
        assert( !pObj->fMarkA && !pObj->fMarkB );
    // print time
    // if ( p->time1 ) { ABC_PRT( "time1", p->time1 ); }
    // if ( p->time2 ) { ABC_PRT( "time2", p->time2 ); }
//    Hop_TableProfile( p );
    if ( p->vChunks )  Hop_ManStopMemory( p );
    if ( p->vPis )     Vec_PtrFree( p->vPis );
    if ( p->vPos )     Vec_PtrFree( p->vPos );
    if ( p->vObjs )    Vec_PtrFree( p->vObjs );
    ABC_FREE( p->pTable );
    ABC_FREE( p );
}


void Hop_ManStopMemory( Hop_Man_t * p )
{
    void * pMemory;
    int i;
    Vec_PtrForEachEntry( void *, p->vChunks, pMemory, i )
        ABC_FREE( pMemory );
    Vec_PtrFree( p->vChunks );
    Vec_PtrFree( p->vPages );
    p->pListFree = NULL;
}


Hop_Obj_t * Hop_Complement( Hop_Man_t * p, Hop_Obj_t * pRoot, int iVar )
{
    // quit if the PI variable is not defined
    if ( iVar >= Hop_ManPiNum(p) )
    {
        printf( "Hop_Complement(): The PI variable %d is not defined.\n", iVar );
        return NULL;
    }
    // recursively perform composition
    Hop_Complement_rec( p, Hop_Regular(pRoot), Hop_ManPi(p, iVar) );
    // clear the markings
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
    return Hop_NotCond( (Hop_Obj_t *)Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );
}


void Hop_Complement_rec( Hop_Man_t * p, Hop_Obj_t * pObj, Hop_Obj_t * pVar )
{
    assert( !Hop_IsComplement(pObj) );
    if ( Hop_ObjIsMarkA(pObj) )
        return;
    if ( Hop_ObjIsConst1(pObj) || Hop_ObjIsPi(pObj) )
    {
        pObj->pData = pObj == pVar ? Hop_Not(pObj) : pObj;
        return;
    }
    Hop_Complement_rec( p, Hop_ObjFanin0(pObj), pVar );
    Hop_Complement_rec( p, Hop_ObjFanin1(pObj), pVar );
    pObj->pData = Hop_And( p, Hop_ObjChild0Copy(pObj), Hop_ObjChild1Copy(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}
