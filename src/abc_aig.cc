#include "abc_aig.h"


static Aig_RMan_t * s_pRMan = NULL;


static unsigned long Aig_Hash( Aig_Obj_t * pObj, int TableSize )
{
    unsigned long Key = Aig_ObjIsExor(pObj) * 1699;
    Key ^= Aig_ObjFanin0(pObj)->Id * 7937;
    Key ^= Aig_ObjFanin1(pObj)->Id * 2971;
    Key ^= Aig_ObjFaninC0(pObj) * 911;
    Key ^= Aig_ObjFaninC1(pObj) * 353;
    return Key % TableSize;
}


static unsigned Abc_HashKey2( Abc_Obj_t * p0, Abc_Obj_t * p1, int TableSize )
{
    unsigned Key = 0;
    Key ^= Abc_ObjRegular(p0)->Id * 7937;
    Key ^= Abc_ObjRegular(p1)->Id * 2971;
    Key ^= Abc_ObjIsComplement(p0) * 911;
    Key ^= Abc_ObjIsComplement(p1) * 353;
    return Key % TableSize;
}


static Aig_Obj_t ** Aig_TableFind( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t ** ppEntry;
    assert( Aig_ObjChild0(pObj) && Aig_ObjChild1(pObj) );
    assert( Aig_ObjFanin0(pObj)->Id < Aig_ObjFanin1(pObj)->Id );
    for ( ppEntry = p->pTable + Aig_Hash(pObj, p->nTableSize); *ppEntry; ppEntry = &(*ppEntry)->pNext )
        if ( *ppEntry == pObj )
            return ppEntry;
    assert( *ppEntry == NULL );
    return ppEntry;
}

static inline int   Aig_FanoutCreate( int FanId, int Num )    { assert( Num < 2 ); return (FanId << 1) | Num;   }
static inline int * Aig_FanoutObj( int * pData, int ObjId )   { return pData + 5*ObjId;                         }
static inline int * Aig_FanoutPrev( int * pData, int iFan )   { return pData + 5*(iFan >> 1) + 1 + (iFan & 1);  }
static inline int * Aig_FanoutNext( int * pData, int iFan )   { return pData + 5*(iFan >> 1) + 3 + (iFan & 1);  }


Abc_Obj_t * Abc_AigConst1( Abc_Ntk_t * pNtk )
{
    assert( Abc_NtkIsStrash(pNtk) );
    return ((Abc_Aig_t *)pNtk->pManFunc)->pConst1;
}


Abc_Obj_t * Abc_AigAnd( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 )
{
    Abc_Obj_t * pAnd;
    if ( (pAnd = Abc_AigAndLookup( pMan, p0, p1 )) )
        return pAnd;
    return Abc_AigAndCreate( pMan, p0, p1 );
}


Abc_Obj_t * Abc_AigAndLookup( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 )
{
    Abc_Obj_t * pAnd, * pConst1;
    unsigned Key;
    assert( Abc_ObjRegular(p0)->pNtk->pManFunc == pMan );
    assert( Abc_ObjRegular(p1)->pNtk->pManFunc == pMan );
    // check for trivial cases
    pConst1 = Abc_AigConst1(pMan->pNtkAig);
    if ( p0 == p1 )
        return p0;
    if ( p0 == Abc_ObjNot(p1) )
        return Abc_ObjNot(pConst1);
    if ( Abc_ObjRegular(p0) == pConst1 )
    {
        if ( p0 == pConst1 )
            return p1;
        return Abc_ObjNot(pConst1);
    }
    if ( Abc_ObjRegular(p1) == pConst1 )
    {
        if ( p1 == pConst1 )
            return p0;
        return Abc_ObjNot(pConst1);
    }
    {
        int nFans0 = Abc_ObjFanoutNum( Abc_ObjRegular(p0) );
        int nFans1 = Abc_ObjFanoutNum( Abc_ObjRegular(p1) );
        if ( nFans0 == 0 || nFans1 == 0 )
            return NULL;
    }

    // order the arguments
    if ( Abc_ObjRegular(p0)->Id > Abc_ObjRegular(p1)->Id )
        pAnd = p0, p0 = p1, p1 = pAnd;
    // get the hash key for these two nodes
    Key = Abc_HashKey2( p0, p1, pMan->nBins );
    // find the matching node in the table
    Abc_AigBinForEachEntry( pMan->pBins[Key], pAnd )
        if ( p0 == Abc_ObjChild0(pAnd) && p1 == Abc_ObjChild1(pAnd) )
        {
//            assert( Abc_ObjFanoutNum(Abc_ObjRegular(p0)) && Abc_ObjFanoutNum(p1) );
             return pAnd;
        }
    return NULL;
}


Abc_Obj_t * Abc_AigAndCreate( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 )
{
    Abc_Obj_t * pAnd;
    unsigned Key;
    // check if it is a good time for table resizing
    if ( pMan->nEntries > 2 * pMan->nBins )
        Abc_AigResize( pMan );
    // order the arguments
    if ( Abc_ObjRegular(p0)->Id > Abc_ObjRegular(p1)->Id )
        pAnd = p0, p0 = p1, p1 = pAnd;
    // create the new node
    pAnd = Abc_NtkCreateNode( pMan->pNtkAig );
    Abc_ObjAddFanin( pAnd, p0 );
    Abc_ObjAddFanin( pAnd, p1 );
    // set the level of the new node
    pAnd->Level  = 1 + Abc_MaxInt( Abc_ObjRegular(p0)->Level, Abc_ObjRegular(p1)->Level );
    pAnd->fExor  = Abc_NodeIsExorType(pAnd);
    pAnd->fPhase = (Abc_ObjIsComplement(p0) ^ Abc_ObjRegular(p0)->fPhase) & (Abc_ObjIsComplement(p1) ^ Abc_ObjRegular(p1)->fPhase);
    // add the node to the corresponding linked list in the table
    Key = Abc_HashKey2( p0, p1, pMan->nBins );
    pAnd->pNext      = pMan->pBins[Key];
    pMan->pBins[Key] = pAnd;
    pMan->nEntries++;
    // create the cuts if defined
//    if ( pAnd->pNtk->pManCut )
//        Abc_NodeGetCuts( pAnd->pNtk->pManCut, pAnd );
    pAnd->pCopy = NULL;
    // add the node to the list of updated nodes
    if ( pMan->vAddedCells )
        Vec_PtrPush( pMan->vAddedCells, pAnd );
    return pAnd;
}


void Abc_AigResize( Abc_Aig_t * pMan )
{
    Abc_Obj_t ** pBinsNew;
    Abc_Obj_t * pEnt, * pEnt2;
    int nBinsNew, Counter, i;
    // abctime clk;
    unsigned Key;

// clk = Abc_Clock();
    // get the new table size
    nBinsNew = Abc_PrimeCudd( 3 * pMan->nBins );
    // allocate a new array
    pBinsNew = ABC_ALLOC( Abc_Obj_t *, nBinsNew );
    memset( pBinsNew, 0, sizeof(Abc_Obj_t *) * nBinsNew );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < pMan->nBins; i++ )
        Abc_AigBinForEachEntrySafe( pMan->pBins[i], pEnt, pEnt2 )
        {
            Key = Abc_HashKey2( Abc_ObjChild0(pEnt), Abc_ObjChild1(pEnt), nBinsNew );
            pEnt->pNext   = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == pMan->nEntries );
//    printf( "Increasing the structural table size from %6d to %6d. ", pMan->nBins, nBinsNew );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    // replace the table and the parameters
    ABC_FREE( pMan->pBins );
    pMan->pBins = pBinsNew;
    pMan->nBins = nBinsNew;
}


int Abc_AigCleanup( Abc_Aig_t * pMan )
{
    Vec_Ptr_t * vDangles;
    Abc_Obj_t * pAnd;
    int i, nNodesOld;
//    printf( "Strash0 = %d.  Strash1 = %d.  Strash100 = %d.  StrashM = %d.\n",
//        pMan->nStrash0, pMan->nStrash1, pMan->nStrash5, pMan->nStrash2 );
    nNodesOld = pMan->nEntries;
    // collect the AND nodes that do not fanout
    vDangles = Vec_PtrAlloc( 100 );
    for ( i = 0; i < pMan->nBins; i++ )
        Abc_AigBinForEachEntry( pMan->pBins[i], pAnd )
            if ( Abc_ObjFanoutNum(pAnd) == 0 )
                Vec_PtrPush( vDangles, pAnd );
    // process the dangling nodes and their MFFCs
    Vec_PtrForEachEntry( Abc_Obj_t *, vDangles, pAnd, i )
        Abc_AigDeleteNode( pMan, pAnd );
    Vec_PtrFree( vDangles );
    return nNodesOld - pMan->nEntries;
}


void Abc_AigDeleteNode( Abc_Aig_t * pMan, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNode0, * pNode1, * pTemp;
    int i, k;

    // make sure the node is regular and dangling
    assert( !Abc_ObjIsComplement(pNode) );
    assert( Abc_ObjIsNode(pNode) );
    assert( Abc_ObjFaninNum(pNode) == 2 );
    assert( Abc_ObjFanoutNum(pNode) == 0 );

    // when deleting an old node that is scheduled for replacement, remove it from the replacement queue
    Vec_PtrForEachEntry( Abc_Obj_t *, pMan->vStackReplaceOld, pTemp, i )
        if ( pNode == pTemp )
        {
            // remove the entry from the replacement array
            for ( k = i; k < pMan->vStackReplaceOld->nSize - 1; k++ )
            {
                pMan->vStackReplaceOld->pArray[k] = pMan->vStackReplaceOld->pArray[k+1];
                pMan->vStackReplaceNew->pArray[k] = pMan->vStackReplaceNew->pArray[k+1];
            }
            pMan->vStackReplaceOld->nSize--;
            pMan->vStackReplaceNew->nSize--;
        }

    // when deleting a new node that should replace another node, do not delete
    Vec_PtrForEachEntry( Abc_Obj_t *, pMan->vStackReplaceNew, pTemp, i )
        if ( pNode == Abc_ObjRegular(pTemp) )
            return;

    // remember the node's fanins
    pNode0 = Abc_ObjFanin0( pNode );
    pNode1 = Abc_ObjFanin1( pNode );

    // add the node to the list of updated nodes
    if ( pMan->vUpdatedNets )
    {
        Vec_PtrPushUnique( pMan->vUpdatedNets, pNode0 );
        Vec_PtrPushUnique( pMan->vUpdatedNets, pNode1 );
    }

    // remove the node from the table
    Abc_AigAndDelete( pMan, pNode );
    // if the node is in the level structure, remove it
    if ( pNode->fMarkA )
        Abc_AigRemoveFromLevelStructure( pMan->vLevels, pNode );
    if ( pNode->fMarkB )
        Abc_AigRemoveFromLevelStructureR( pMan->vLevelsR, pNode );
    // remove the node from the network
    Abc_NtkDeleteObj( pNode );

    // call recursively for the fanins
    if ( Abc_ObjIsNode(pNode0) && pNode0->vFanouts.nSize == 0 )
        Abc_AigDeleteNode( pMan, pNode0 );
    if ( Abc_ObjIsNode(pNode1) && pNode1->vFanouts.nSize == 0 )
        Abc_AigDeleteNode( pMan, pNode1 );
}


void Abc_AigAndDelete( Abc_Aig_t * pMan, Abc_Obj_t * pThis )
{
    // Abc_Obj_t * pAnd, * pAnd0, * pAnd1, ** ppPlace;
    Abc_Obj_t * pAnd, ** ppPlace;
    unsigned Key;
    assert( !Abc_ObjIsComplement(pThis) );
    assert( Abc_ObjIsNode(pThis) );
    assert( Abc_ObjFaninNum(pThis) == 2 );
    assert( pMan->pNtkAig == pThis->pNtk );
    // get the hash key for these two nodes
    // pAnd0 = Abc_ObjRegular( Abc_ObjChild0(pThis) );
    // pAnd1 = Abc_ObjRegular( Abc_ObjChild1(pThis) );
    Abc_ObjRegular( Abc_ObjChild0(pThis) );
    Abc_ObjRegular( Abc_ObjChild1(pThis) );
    Key = Abc_HashKey2( Abc_ObjChild0(pThis), Abc_ObjChild1(pThis), pMan->nBins );
    // find the matching node in the table
    ppPlace = pMan->pBins + Key;
    Abc_AigBinForEachEntry( pMan->pBins[Key], pAnd )
    {
        if ( pAnd != pThis )
        {
            ppPlace = &pAnd->pNext;
            continue;
        }
        *ppPlace = pAnd->pNext;
        break;
    }
    assert( pAnd == pThis );
    pMan->nEntries--;
    // delete the cuts if defined
    if ( pThis->pNtk->pManCut )
        Abc_NodeFreeCuts( pThis->pNtk->pManCut, pThis );
}


void Abc_AigRemoveFromLevelStructureR( Vec_Vec_t * vStruct, Abc_Obj_t * pNode )
{
    Vec_Ptr_t * vVecTemp;
    Abc_Obj_t * pTemp;
    int m;
    assert( pNode->fMarkB );
    vVecTemp = Vec_VecEntry( vStruct, Abc_ObjReverseLevel(pNode) );
    Vec_PtrForEachEntry( Abc_Obj_t *, vVecTemp, pTemp, m )
    {
        if ( pTemp != pNode )
            continue;
        Vec_PtrWriteEntry( vVecTemp, m, NULL );
        break;
    }
    assert( m < Vec_PtrSize(vVecTemp) ); // found
    pNode->fMarkB = 0;
}


void Abc_AigRemoveFromLevelStructure( Vec_Vec_t * vStruct, Abc_Obj_t * pNode )
{
    Vec_Ptr_t * vVecTemp;
    Abc_Obj_t * pTemp;
    int m;
    assert( pNode->fMarkA );
    vVecTemp = Vec_VecEntry( vStruct, pNode->Level );
    Vec_PtrForEachEntry( Abc_Obj_t *, vVecTemp, pTemp, m )
    {
        if ( pTemp != pNode )
            continue;
        Vec_PtrWriteEntry( vVecTemp, m, NULL );
        break;
    }
    assert( m < Vec_PtrSize(vVecTemp) ); // found
    pNode->fMarkA = 0;
}


Abc_Aig_t * Abc_AigAlloc( Abc_Ntk_t * pNtkAig )
{
    Abc_Aig_t * pMan;
    // start the manager
    pMan = ABC_ALLOC( Abc_Aig_t, 1 );
    memset( pMan, 0, sizeof(Abc_Aig_t) );
    // allocate the table
    pMan->nBins    = Abc_PrimeCudd( 10000 );
    pMan->pBins    = ABC_ALLOC( Abc_Obj_t *, pMan->nBins );
    memset( pMan->pBins, 0, sizeof(Abc_Obj_t *) * pMan->nBins );
    pMan->vNodes   = Vec_PtrAlloc( 100 );
    pMan->vLevels  = Vec_VecAlloc( 100 );
    pMan->vLevelsR = Vec_VecAlloc( 100 );
    pMan->vStackReplaceOld = Vec_PtrAlloc( 100 );
    pMan->vStackReplaceNew = Vec_PtrAlloc( 100 );
    // create the constant node
    assert( pNtkAig->vObjs->nSize == 0 );
    pMan->pConst1 = Abc_NtkCreateObj( pNtkAig, ABC_OBJ_NODE );
    pMan->pConst1->Type = ABC_OBJ_CONST1;
    pMan->pConst1->fPhase = 1;
    pNtkAig->nObjCounts[ABC_OBJ_NODE]--;
    // save the current network
    pMan->pNtkAig = pNtkAig;
    return pMan;
}


int Abc_AigCheck( Abc_Aig_t * pMan )
{
    Abc_Obj_t * pObj, * pAnd;
    int i, nFanins, Counter;
    Abc_NtkForEachNode( pMan->pNtkAig, pObj, i )
    {
        nFanins = Abc_ObjFaninNum(pObj);
        if ( nFanins == 0 )
        {
            if ( !Abc_AigNodeIsConst(pObj) )
            {
                printf( "Abc_AigCheck: The AIG has non-standard constant nodes.\n" );
                return 0;
            }
            continue;
        }
        if ( nFanins == 1 )
        {
            printf( "Abc_AigCheck: The AIG has single input nodes.\n" );
            return 0;
        }
        if ( nFanins > 2 )
        {
            printf( "Abc_AigCheck: The AIG has non-standard nodes.\n" );
            return 0;
        }
        if ( pObj->Level != 1 + (unsigned)Abc_MaxInt( Abc_ObjFanin0(pObj)->Level, Abc_ObjFanin1(pObj)->Level ) )
            printf( "Abc_AigCheck: Node \"%s\" has level that does not agree with the fanin levels.\n", Abc_ObjName(pObj) );
        pAnd = Abc_AigAndLookup( pMan, Abc_ObjChild0(pObj), Abc_ObjChild1(pObj) );
        if ( pAnd != pObj )
            printf( "Abc_AigCheck: Node \"%s\" is not in the structural hashing table.\n", Abc_ObjName(pObj) );
    }
    // count the number of nodes in the table
    Counter = 0;
    for ( i = 0; i < pMan->nBins; i++ )
        Abc_AigBinForEachEntry( pMan->pBins[i], pAnd )
            Counter++;
    if ( Counter != Abc_NtkNodeNum(pMan->pNtkAig) )
    {
        printf( "Abc_AigCheck: The number of nodes in the structural hashing table is wrong.\n" );
        return 0;
    }
    // if the node is a choice node, nodes in its class should not have fanouts
    Abc_NtkForEachNode( pMan->pNtkAig, pObj, i )
        if ( Abc_AigNodeIsChoice(pObj) )
            for ( pAnd = (Abc_Obj_t *)pObj->pData; pAnd; pAnd = (Abc_Obj_t *)pAnd->pData )
                if ( Abc_ObjFanoutNum(pAnd) > 0 )
                {
                    printf( "Abc_AigCheck: Representative %s", Abc_ObjName(pAnd) );
                    printf( " of choice node %s has %d fanouts.\n", Abc_ObjName(pObj), Abc_ObjFanoutNum(pAnd) );
                    return 0;
                }
    return 1;
}


void Abc_AigFree( Abc_Aig_t * pMan )
{
    assert( Vec_PtrSize( pMan->vStackReplaceOld ) == 0 );
    assert( Vec_PtrSize( pMan->vStackReplaceNew ) == 0 );
    // free the table
    if ( pMan->vAddedCells )
        Vec_PtrFree( pMan->vAddedCells );
    if ( pMan->vUpdatedNets )
        Vec_PtrFree( pMan->vUpdatedNets );
    Vec_VecFree( pMan->vLevels );
    Vec_VecFree( pMan->vLevelsR );
    Vec_PtrFree( pMan->vStackReplaceOld );
    Vec_PtrFree( pMan->vStackReplaceNew );
    Vec_PtrFree( pMan->vNodes );
    ABC_FREE( pMan->pBins );
    ABC_FREE( pMan );
}


void Aig_RManRecord( unsigned * pTruth, int nVarsInit )
{
    int fVerify = 1;
    Kit_DsdNtk_t * pNtk;
    Kit_DsdObj_t * pObj;
    unsigned uPhaseC;
    int i, nVars, nWords;
    int fUniqueVars;

    if ( nVarsInit > RMAN_MAXVARS )
    {
        printf( "The number of variables in too large.\n" );
        return;
    }

    if ( s_pRMan == NULL )
        s_pRMan = Aig_RManStart();
    s_pRMan->nTotal++;
    // canonicize the function
    pNtk = Kit_DsdDecompose( pTruth, nVarsInit );
    pObj = Kit_DsdNonDsdPrimeMax( pNtk );
    if ( pObj == NULL || pObj->nFans == 3 )
    {
        s_pRMan->nTtDsd++;
        Kit_DsdNtkFree( pNtk );
        return;
    }
    nVars = pObj->nFans;
    s_pRMan->nVarFuncs[nVars]++;
    if ( nVars < nVarsInit )
        s_pRMan->nTtDsdPart++;
    else
        s_pRMan->nTtDsdNot++;
    // compute the number of words
    nWords = Abc_TruthWordNum( nVars );
    // copy the function
    memcpy( s_pRMan->pTruthInit, Kit_DsdObjTruth(pObj), (size_t)(4*nWords) );
    Kit_DsdNtkFree( pNtk );
    // canonicize the output
    if ( s_pRMan->pTruthInit[0] & 1 )
        Kit_TruthNot( s_pRMan->pTruthInit, s_pRMan->pTruthInit, nVars );
    memcpy( s_pRMan->pTruth, s_pRMan->pTruthInit, 4*nWords );

    // canonize the function
    for ( i = 0; i < nVars; i++ )
        s_pRMan->pPerm[i] = i;
    uPhaseC = Aig_RManSemiCanonicize( s_pRMan->pTruthTemp, s_pRMan->pTruth, nVars, s_pRMan->pPerm, s_pRMan->pMints, 1 );
    // check unique variables
    fUniqueVars = Aig_RManVarsAreUnique( s_pRMan->pMints, nVars );
    s_pRMan->nUniqueVars += fUniqueVars;

    if ( Aig_RManTableFindOrAdd( s_pRMan, s_pRMan->pTruth, nVars ) )
        Aig_RManSaveOne( s_pRMan, s_pRMan->pTruth, nVars );

    if ( fVerify )
    {
        // derive reverse permutation
        for ( i = 0; i < nVars; i++ )
            s_pRMan->pPermR[i] = s_pRMan->pPerm[i];
        // implement permutation
        Kit_TruthPermute( s_pRMan->pTruthTemp, s_pRMan->pTruth, nVars, s_pRMan->pPermR, 1 );
        // implement polarity
        for ( i = 0; i < nVars; i++ )
            if ( uPhaseC & (1 << i) )
                Kit_TruthChangePhase( s_pRMan->pTruth, nVars, i );

        // perform verification
        if ( fUniqueVars && !Kit_TruthIsEqual( s_pRMan->pTruth, s_pRMan->pTruthInit, nVars ) )
            printf( "Verification failed.\n" );
    }
}


Aig_RMan_t * Aig_RManStart()
{
    static Bdc_Par_t Pars = {0}, * pPars = &Pars;
    Aig_RMan_t * p;
    p = ABC_ALLOC( Aig_RMan_t, 1 );
    memset( p, 0, sizeof(Aig_RMan_t) );
    p->nVars = RMAN_MAXVARS;
    p->pAig  = Aig_ManStart( 1000000 );
    Aig_IthVar( p->pAig, p->nVars-1 );
    // create hash table
    p->nBins = Abc_PrimeCudd(5000);
    p->pBins = ABC_CALLOC( Aig_Tru_t *, p->nBins );
    p->pMemTrus = Aig_MmFlexStart();
    // bi-decomposition manager
    pPars->nVarsMax = p->nVars;
    pPars->fVerbose = 0;
    p->pBidec = Bdc_ManAlloc( pPars );
    return p;
}


static inline int Aig_RManCompareSigs( Aig_VSig_t * p0, Aig_VSig_t * p1, int nVars )
{
//    return memcmp( p0, p1, sizeof(int) + sizeof(int) * nVars );
    return memcmp( p0, p1, sizeof(int) );
}


unsigned Aig_RManSemiCanonicize( unsigned * pOut, unsigned * pIn, int nVars, char * pCanonPerm, Aig_VSig_t * pSigs, int fReturnIn )
{
    Aig_VSig_t TempSig;
    int i, Temp, fChange, Counter;
    unsigned * pTemp, uCanonPhase = 0;
    // collect signatures
    Aig_RManComputeVSigs( pIn, nVars, pSigs, pOut );
    // canonicize phase
    for ( i = 0; i < nVars; i++ )
    {
//        if ( pStore[2*i+0] <= pStore[2*i+1] )
        if ( Aig_RManCompareSigs( &pSigs[2*i+0], &pSigs[2*i+1], nVars ) <= 0 )
            continue;
        uCanonPhase |= (1 << i);
        TempSig = pSigs[2*i+0];
        pSigs[2*i+0] = pSigs[2*i+1];
        pSigs[2*i+1] = TempSig;
        Kit_TruthChangePhase( pIn, nVars, i );
    }
    // permute
    Counter = 0;
    do {
        fChange = 0;
        for ( i = 0; i < nVars-1; i++ )
        {
//            if ( pStore[2*i] <= pStore[2*(i+1)] )
            if ( Aig_RManCompareSigs( &pSigs[2*i], &pSigs[2*(i+1)], nVars ) <= 0 )
                continue;
            Counter++;
            fChange = 1;

            Temp = pCanonPerm[i];
            pCanonPerm[i] = pCanonPerm[i+1];
            pCanonPerm[i+1] = Temp;

            TempSig = pSigs[2*i];
            pSigs[2*i] = pSigs[2*(i+1)];
            pSigs[2*(i+1)] = TempSig;

            TempSig = pSigs[2*i+1];
            pSigs[2*i+1] = pSigs[2*(i+1)+1];
            pSigs[2*(i+1)+1] = TempSig;

            Kit_TruthSwapAdjacentVars( pOut, pIn, nVars, i );
            pTemp = pIn; pIn = pOut; pOut = pTemp;
        }
    } while ( fChange );

    // swap if it was moved an even number of times
    if ( fReturnIn ^ !(Counter & 1) )
        Kit_TruthCopy( pOut, pIn, nVars );
    return uCanonPhase;
}


int Aig_RManVarsAreUnique( Aig_VSig_t * pMints, int nVars )
{
    int i;
    for ( i = 0; i < nVars - 1; i++ )
        if ( Aig_RManCompareSigs( &pMints[2*i], &pMints[2*(i+1)], nVars ) == 0 )
            return 0;
    return 1;
}


int Aig_RManTableFindOrAdd( Aig_RMan_t * p, unsigned * pTruth, int nVars )
{
    Aig_Tru_t ** ppSpot, * pEntry;
    int nBytes;
    ppSpot = Aig_RManTableLookup( p, pTruth, nVars );
    if ( *ppSpot )
    {
        (*ppSpot)->nVisits++;
        return 0;
    }
    nBytes = sizeof(Aig_Tru_t) + sizeof(unsigned) * Kit_TruthWordNum(nVars);
    if ( p->nEntries == 3*p->nBins )
        Aig_RManTableResize( p );
    pEntry = (Aig_Tru_t *)Aig_MmFlexEntryFetch( p->pMemTrus, nBytes );
    pEntry->Id = p->nEntries++;
    pEntry->nVars = nVars;
    pEntry->nVisits = 1;
    pEntry->pNext = NULL;
    memcpy( pEntry->pTruth, pTruth, sizeof(unsigned) * Kit_TruthWordNum(nVars) );
    *ppSpot = pEntry;
    return 1;
}


static inline int Aig_RManTableHash( unsigned * pTruth, int nVars, int nBins, int * pPrimes )
{
    int i, nWords = Kit_TruthWordNum( nVars );
    unsigned uHash = 0;
    for ( i = 0; i < nWords; i++ )
        uHash ^= pTruth[i] * pPrimes[i & 0xf];
    return (int)(uHash % nBins);
}


Aig_Tru_t ** Aig_RManTableLookup( Aig_RMan_t * p, unsigned * pTruth, int nVars )
{
    static int s_Primes[16] = {
        1291, 1699, 1999, 2357, 2953, 3313, 3907, 4177,
        4831, 5147, 5647, 6343, 6899, 7103, 7873, 8147 };
    Aig_Tru_t ** ppSpot, * pEntry;
    ppSpot = p->pBins + Aig_RManTableHash( pTruth, nVars, p->nBins, s_Primes );
    for ( pEntry = *ppSpot; pEntry; ppSpot = &pEntry->pNext, pEntry = pEntry->pNext )
        if ( Kit_TruthIsEqual( pEntry->pTruth, pTruth, nVars ) )
            return ppSpot;
    return ppSpot;
}


static inline Aig_Obj_t * Bdc_FunCopyHop( Bdc_Fun_t * pObj )
{ return Aig_NotCond( (Aig_Obj_t *)Bdc_FuncCopy(Bdc_Regular(pObj)), Bdc_IsComplement(pObj) );  }


void Aig_RManSaveOne( Aig_RMan_t * p, unsigned * pTruth, int nVars )
{
    int i, nNodes, RetValue;
    Bdc_Fun_t * pFunc;
    Aig_Obj_t * pTerm;
    // perform decomposition
    RetValue = Bdc_ManDecompose( p->pBidec, pTruth, NULL, nVars, NULL, 1000 );
    if ( RetValue < 0 )
    {
        printf( "Decomposition failed.\n" );
        return;
    }
    // convert back into HOP
    Bdc_FuncSetCopy( Bdc_ManFunc( p->pBidec, 0 ), Aig_ManConst1(p->pAig) );
    for ( i = 0; i < nVars; i++ )
        Bdc_FuncSetCopy( Bdc_ManFunc( p->pBidec, i+1 ), Aig_IthVar(p->pAig, i) );
    nNodes = Bdc_ManNodeNum(p->pBidec);
    for ( i = nVars + 1; i < nNodes; i++ )
    {
        pFunc = Bdc_ManFunc( p->pBidec, i );
        Bdc_FuncSetCopy( pFunc, Aig_And( p->pAig,
            Bdc_FunCopyHop(Bdc_FuncFanin0(pFunc)),
            Bdc_FunCopyHop(Bdc_FuncFanin1(pFunc)) ) );
    }
    pTerm = Bdc_FunCopyHop( Bdc_ManRoot(p->pBidec) );
    pTerm = Aig_ObjCreateCo( p->pAig, pTerm );
//    assert( pTerm->fPhase == 0 );
}


static inline Aig_Obj_t * Aig_ManFetchMemory( Aig_Man_t * p )
{
    // extern char * Aig_MmFixedEntryFetch( Aig_MmFixed_t * p );
    Aig_Obj_t * pTemp;
    pTemp = (Aig_Obj_t *)Aig_MmFixedEntryFetch( p->pMemObjs );
    memset( pTemp, 0, sizeof(Aig_Obj_t) );
    pTemp->Id = Vec_PtrSize(p->vObjs);
    Vec_PtrPush( p->vObjs, pTemp );
    return pTemp;
}


Aig_Man_t * Aig_ManStart( int nNodesMax )
{
    Aig_Man_t * p;
    if ( nNodesMax <= 0 )
        nNodesMax = 10007;
    // start the manager
    p = ABC_ALLOC( Aig_Man_t, 1 );
    memset( p, 0, sizeof(Aig_Man_t) );
    // perform initializations
    p->nTravIds = 1;
    p->fCatchExor = 0;
    // allocate arrays for nodes
    p->vCis  = Vec_PtrAlloc( 100 );
    p->vCos  = Vec_PtrAlloc( 100 );
    p->vObjs = Vec_PtrAlloc( 1000 );
    p->vBufs = Vec_PtrAlloc( 100 );
    //--jlong -- begin
       p->unfold2_type_I = Vec_PtrAlloc( 4);
       p->unfold2_type_II = Vec_PtrAlloc( 4);
       //--jlong -- end
    // prepare the internal memory manager
    p->pMemObjs = Aig_MmFixedStart( sizeof(Aig_Obj_t), nNodesMax );
    // create the constant node
    p->pConst1 = Aig_ManFetchMemory( p );
    p->pConst1->Type = AIG_OBJ_CONST1;
    p->pConst1->fPhase = 1;
    p->nObjs[AIG_OBJ_CONST1]++;
    // start the table
    p->nTableSize = Abc_PrimeCudd( nNodesMax );
    p->pTable = ABC_ALLOC( Aig_Obj_t *, p->nTableSize );
    memset( p->pTable, 0, sizeof(Aig_Obj_t *) * p->nTableSize );
    return p;
}


Aig_MmFixed_t * Aig_MmFixedStart( int nEntrySize, int nEntriesMax )
{
    Aig_MmFixed_t * p;

    p = ABC_ALLOC( Aig_MmFixed_t, 1 );
    memset( p, 0, sizeof(Aig_MmFixed_t) );

    p->nEntrySize    = nEntrySize;
    p->nEntriesAlloc = 0;
    p->nEntriesUsed  = 0;
    p->pEntriesFree  = NULL;

    p->nChunkSize = nEntriesMax / 8;
    if ( p->nChunkSize < 8 )
        p->nChunkSize = 8;

    p->nChunksAlloc  = 64;
    p->nChunks       = 0;
    p->pChunks       = ABC_ALLOC( char *, p->nChunksAlloc );

    p->nMemoryUsed   = 0;
    p->nMemoryAlloc  = 0;
    return p;
}


void Aig_RManComputeVSigs( unsigned * pTruth, int nVars, Aig_VSig_t * pSigs, unsigned * pAux )
{
    int v;
    for ( v = 0; v < nVars; v++ )
    {
        Kit_TruthCofactor0New( pAux, pTruth, nVars, v );
        pSigs[2*v+0].nOnes = Kit_TruthCountOnes( pAux, nVars );
        Kit_TruthCountOnesInCofs0( pAux, nVars, pSigs[2*v+0].nCofOnes );
        Aig_RManSortNums( pSigs[2*v+0].nCofOnes, nVars );

        Kit_TruthCofactor1New( pAux, pTruth, nVars, v );
        pSigs[2*v+1].nOnes = Kit_TruthCountOnes( pAux, nVars );
        Kit_TruthCountOnesInCofs0( pAux, nVars, pSigs[2*v+1].nCofOnes );
        Aig_RManSortNums( pSigs[2*v+1].nCofOnes, nVars );
    }
}


void Aig_RManSortNums( int * pArray, int nVars )
{
    int i, j, best_i, tmp;
    for ( i = 0; i < nVars-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nVars; j++ )
            if ( pArray[j] > pArray[best_i] )
                best_i = j;
        tmp = pArray[i]; pArray[i] = pArray[best_i]; pArray[best_i] = tmp;
    }
}


Aig_Obj_t * Aig_IthVar( Aig_Man_t * p, int i )
{
    int v;
    for ( v = Aig_ManCiNum(p); v <= i; v++ )
        Aig_ObjCreateCi( p );
    assert( i < Vec_PtrSize(p->vCis) );
    return Aig_ManCi( p, i );
}


Aig_Obj_t * Aig_ObjCreateCi( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    pObj = Aig_ManFetchMemory( p );
    pObj->Type = AIG_OBJ_CI;
    Vec_PtrPush( p->vCis, pObj );
    p->nObjs[AIG_OBJ_CI]++;
    return pObj;
}


Aig_MmFlex_t * Aig_MmFlexStart()
{
    Aig_MmFlex_t * p;

    p = ABC_ALLOC( Aig_MmFlex_t, 1 );
    memset( p, 0, sizeof(Aig_MmFlex_t) );

    p->nEntriesUsed  = 0;
    p->pCurrent      = NULL;
    p->pEnd          = NULL;

    p->nChunkSize    = (1 << 18);
    p->nChunksAlloc  = 64;
    p->nChunks       = 0;
    p->pChunks       = ABC_ALLOC( char *, p->nChunksAlloc );

    p->nMemoryUsed   = 0;
    p->nMemoryAlloc  = 0;
    return p;
}


void Aig_RManTableResize( Aig_RMan_t * p )
{
    Aig_Tru_t * pEntry, * pNext;
    Aig_Tru_t ** pBinsOld, ** ppPlace;
    int nBinsOld, Counter, i;
    // abctime clk;
    assert( p->pBins != NULL );
// clk = Abc_Clock();
    // save the old Bins
    pBinsOld = p->pBins;
    nBinsOld = p->nBins;
    // get the new Bins
    p->nBins = Abc_PrimeCudd( 3 * nBinsOld );
    p->pBins = ABC_CALLOC( Aig_Tru_t *, p->nBins );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < nBinsOld; i++ )
    for ( pEntry = pBinsOld[i], pNext = pEntry? pEntry->pNext : NULL;
          pEntry; pEntry = pNext, pNext = pEntry? pEntry->pNext : NULL )
    {
        // get the place where this entry goes in the Bins
        ppPlace = Aig_RManTableLookup( p, pEntry->pTruth, pEntry->nVars );
        assert( *ppPlace == NULL ); // should not be there
        // add the entry to the list
        *ppPlace = pEntry;
        pEntry->pNext = NULL;
        Counter++;
    }
    assert( Counter == p->nEntries );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    ABC_FREE( pBinsOld );
}


char * Aig_MmFlexEntryFetch( Aig_MmFlex_t * p, int nBytes )
{
    char * pTemp;
#ifdef ABC_MEMALIGN
    // extend size to max alignment
    nBytes += (ABC_MEMALIGN - nBytes % ABC_MEMALIGN) % ABC_MEMALIGN;
#endif
    // check if there are still free entries
    if ( p->pCurrent == NULL || p->pCurrent + nBytes > p->pEnd )
    { // need to allocate more entries
        if ( p->nChunks == p->nChunksAlloc )
        {
            p->nChunksAlloc *= 2;
            p->pChunks = ABC_REALLOC( char *, p->pChunks, p->nChunksAlloc );
        }
        if ( nBytes > p->nChunkSize )
        {
            // resize the chunk size if more memory is requested than it can give
            // (ideally, this should never happen)
            p->nChunkSize = 2 * nBytes;
        }
        p->pCurrent = ABC_ALLOC( char, p->nChunkSize );
        p->pEnd     = p->pCurrent + p->nChunkSize;
        p->nMemoryAlloc += p->nChunkSize;
        // add the chunk to the chunk storage
        p->pChunks[ p->nChunks++ ] = p->pCurrent;
    }
    assert( p->pCurrent + nBytes <= p->pEnd );
    // increment the counter of used entries
    p->nEntriesUsed++;
    // keep track of the memory used
    p->nMemoryUsed += nBytes;
    // return the next entry
    pTemp = p->pCurrent;
    p->pCurrent += nBytes;
    return pTemp;
}


Aig_Obj_t * Aig_ObjCreateCo( Aig_Man_t * p, Aig_Obj_t * pDriver )
{
    Aig_Obj_t * pObj;
    pObj = Aig_ManFetchMemory( p );
    pObj->Type = AIG_OBJ_CO;
    Vec_PtrPush( p->vCos, pObj );
    Aig_ObjConnect( p, pObj, pDriver, NULL );
    p->nObjs[AIG_OBJ_CO]++;
    return pObj;
}


void Aig_ObjConnect( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFan0, Aig_Obj_t * pFan1 )
{
    assert( !Aig_IsComplement(pObj) );
    assert( !Aig_ObjIsCi(pObj) );
    // add the first fanin
    pObj->pFanin0 = pFan0;
    pObj->pFanin1 = pFan1;
    // increment references of the fanins and add their fanouts
    if ( pFan0 != NULL )
    {
        assert( Aig_ObjFanin0(pObj)->Type > 0 );
        Aig_ObjRef( Aig_ObjFanin0(pObj) );
        if ( p->pFanData )
            Aig_ObjAddFanout( p, Aig_ObjFanin0(pObj), pObj );
    }
    if ( pFan1 != NULL )
    {
        assert( Aig_ObjFanin1(pObj)->Type > 0 );
        Aig_ObjRef( Aig_ObjFanin1(pObj) );
        if ( p->pFanData )
            Aig_ObjAddFanout( p, Aig_ObjFanin1(pObj), pObj );
    }
    // set level and phase
    pObj->Level = Aig_ObjLevelNew( pObj );
    pObj->fPhase = Aig_ObjPhaseReal(pFan0) & Aig_ObjPhaseReal(pFan1);
    // add the node to the structural hash table
    if ( p->pTable && Aig_ObjIsHash(pObj) )
        Aig_TableInsert( p, pObj );
    // add the node to the dynamically updated topological order
//    if ( p->pOrderData && Aig_ObjIsNode(pObj) )
//        Aig_ObjOrderInsert( p, pObj->Id );
    assert( !Aig_ObjIsNode(pObj) || pObj->Level > 0 );
}


void Aig_ObjAddFanout( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFanout )
{
    int iFan, * pFirst, * pPrevC, * pNextC, * pPrev, * pNext;
    assert( p->pFanData );
    assert( !Aig_IsComplement(pObj) && !Aig_IsComplement(pFanout) );
    assert( pFanout->Id > 0 );
    if ( pObj->Id >= p->nFansAlloc || pFanout->Id >= p->nFansAlloc )
    {
        int nFansAlloc = 2 * Abc_MaxInt( pObj->Id, pFanout->Id );
        p->pFanData = ABC_REALLOC( int, p->pFanData, 5 * nFansAlloc );
        memset( p->pFanData + 5 * p->nFansAlloc, 0, sizeof(int) * 5 * (nFansAlloc - p->nFansAlloc) );
        p->nFansAlloc = nFansAlloc;
    }
    assert( pObj->Id < p->nFansAlloc && pFanout->Id < p->nFansAlloc );
    iFan   = Aig_FanoutCreate( pFanout->Id, Aig_ObjWhatFanin(pFanout, pObj) );
    pPrevC = Aig_FanoutPrev( p->pFanData, iFan );
    pNextC = Aig_FanoutNext( p->pFanData, iFan );
    pFirst = Aig_FanoutObj( p->pFanData, pObj->Id );
    if ( *pFirst == 0 )
    {
        *pFirst = iFan;
        *pPrevC = iFan;
        *pNextC = iFan;
    }
    else
    {
        pPrev = Aig_FanoutPrev( p->pFanData, *pFirst );
        pNext = Aig_FanoutNext( p->pFanData, *pPrev );
        assert( *pNext == *pFirst );
        *pPrevC = *pPrev;
        *pNextC = *pFirst;
        *pPrev  = iFan;
        *pNext  = iFan;
    }
}


void Aig_TableInsert( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t ** ppPlace;
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_TableLookup(p, pObj) == NULL );
    if ( (pObj->Id & 0xFF) == 0 && 2 * p->nTableSize < Aig_ManNodeNum(p) )
        Aig_TableResize( p );
    ppPlace = Aig_TableFind( p, pObj );
    assert( *ppPlace == NULL );
    *ppPlace = pObj;
}


Aig_Obj_t * Aig_TableLookup( Aig_Man_t * p, Aig_Obj_t * pGhost )
{
    Aig_Obj_t * pEntry;
    assert( !Aig_IsComplement(pGhost) );
    assert( Aig_ObjIsNode(pGhost) );
    assert( Aig_ObjChild0(pGhost) && Aig_ObjChild1(pGhost) );
    assert( Aig_ObjFanin0(pGhost)->Id < Aig_ObjFanin1(pGhost)->Id );
    if ( p->pTable == NULL || !Aig_ObjRefs(Aig_ObjFanin0(pGhost)) || !Aig_ObjRefs(Aig_ObjFanin1(pGhost)) )
        return NULL;
    for ( pEntry = p->pTable[Aig_Hash(pGhost, p->nTableSize)]; pEntry; pEntry = pEntry->pNext )
    {
        if ( Aig_ObjChild0(pEntry) == Aig_ObjChild0(pGhost) &&
             Aig_ObjChild1(pEntry) == Aig_ObjChild1(pGhost) &&
             Aig_ObjType(pEntry) == Aig_ObjType(pGhost) )
            return pEntry;
    }
    return NULL;
}


void Aig_TableResize( Aig_Man_t * p )
{
    Aig_Obj_t * pEntry, * pNext;
    Aig_Obj_t ** pTableOld, ** ppPlace;
    int nTableSizeOld, Counter, i;
    // abctime clk;
    assert( p->pTable != NULL );
// clk = Abc_Clock();
    // save the old table
    pTableOld = p->pTable;
    nTableSizeOld = p->nTableSize;
    // get the new table
    p->nTableSize = Abc_PrimeCudd( 2 * Aig_ManNodeNum(p) );
    p->pTable = ABC_ALLOC( Aig_Obj_t *, p->nTableSize );
    memset( p->pTable, 0, sizeof(Aig_Obj_t *) * p->nTableSize );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < nTableSizeOld; i++ )
    for ( pEntry = pTableOld[i], pNext = pEntry? pEntry->pNext : NULL;
          pEntry; pEntry = pNext, pNext = pEntry? pEntry->pNext : NULL )
    {
        // get the place where this entry goes in the table
        ppPlace = Aig_TableFind( p, pEntry );
        assert( *ppPlace == NULL ); // should not be there
        // add the entry to the list
        *ppPlace = pEntry;
        pEntry->pNext = NULL;
        Counter++;
    }
    assert( Counter == Aig_ManNodeNum(p) );
//    printf( "Increasing the structural table size from %6d to %6d. ", nTableSizeOld, p->nTableSize );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    // replace the table and the parameters
    ABC_FREE( pTableOld );
}


static inline int Aig_ObjIsExorType( Aig_Obj_t * p0, Aig_Obj_t * p1, Aig_Obj_t ** ppFan0, Aig_Obj_t ** ppFan1 )
{
    if ( !Aig_IsComplement(p0) || !Aig_IsComplement(p1) )
        return 0;
    p0 = Aig_Regular(p0);
    p1 = Aig_Regular(p1);
    if ( !Aig_ObjIsAnd(p0) || !Aig_ObjIsAnd(p1) )
        return 0;
    if ( Aig_ObjFanin0(p0) != Aig_ObjFanin0(p1) || Aig_ObjFanin1(p0) != Aig_ObjFanin1(p1) )
        return 0;
    if ( Aig_ObjFaninC0(p0) == Aig_ObjFaninC0(p1) || Aig_ObjFaninC1(p0) == Aig_ObjFaninC1(p1) )
        return 0;
    *ppFan0 = Aig_ObjChild0(p0);
    *ppFan1 = Aig_ObjChild1(p0);
    return 1;
}


Aig_Obj_t * Aig_And( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 )
{
    Aig_Obj_t * pGhost, * pResult;
    Aig_Obj_t * pFan0, * pFan1;
    // check trivial cases
    if ( p0 == p1 )
        return p0;
    if ( p0 == Aig_Not(p1) )
        return Aig_Not(p->pConst1);
    if ( Aig_Regular(p0) == p->pConst1 )
        return p0 == p->pConst1 ? p1 : Aig_Not(p->pConst1);
    if ( Aig_Regular(p1) == p->pConst1 )
        return p1 == p->pConst1 ? p0 : Aig_Not(p->pConst1);
    // check not so trivial cases
    if ( p->fAddStrash && (Aig_ObjIsNode(Aig_Regular(p0)) || Aig_ObjIsNode(Aig_Regular(p1))) )
    { // http://fmv.jku.at/papers/BrummayerBiere-MEMICS06.pdf
        Aig_Obj_t * pFanA, * pFanB, * pFanC, * pFanD;
        pFanA = Aig_ObjChild0(Aig_Regular(p0));
        pFanB = Aig_ObjChild1(Aig_Regular(p0));
        pFanC = Aig_ObjChild0(Aig_Regular(p1));
        pFanD = Aig_ObjChild1(Aig_Regular(p1));
        if ( Aig_IsComplement(p0) )
        {
            if ( pFanA == Aig_Not(p1) || pFanB == Aig_Not(p1) )
                return p1;
            if ( pFanB == p1 )
                return Aig_And( p, Aig_Not(pFanA), pFanB );
            if ( pFanA == p1 )
                return Aig_And( p, Aig_Not(pFanB), pFanA );
        }
        else
        {
            if ( pFanA == Aig_Not(p1) || pFanB == Aig_Not(p1) )
                return Aig_Not(p->pConst1);
            if ( pFanA == p1 || pFanB == p1 )
                return p0;
        }
        if ( Aig_IsComplement(p1) )
        {
            if ( pFanC == Aig_Not(p0) || pFanD == Aig_Not(p0) )
                return p0;
            if ( pFanD == p0 )
                return Aig_And( p, Aig_Not(pFanC), pFanD );
            if ( pFanC == p0 )
                return Aig_And( p, Aig_Not(pFanD), pFanC );
        }
        else
        {
            if ( pFanC == Aig_Not(p0) || pFanD == Aig_Not(p0) )
                return Aig_Not(p->pConst1);
            if ( pFanC == p0 || pFanD == p0 )
                return p1;
        }
        if ( !Aig_IsComplement(p0) && !Aig_IsComplement(p1) )
        {
            if ( pFanA == Aig_Not(pFanC) || pFanA == Aig_Not(pFanD) || pFanB == Aig_Not(pFanC) || pFanB == Aig_Not(pFanD) )
                return Aig_Not(p->pConst1);
            if ( pFanA == pFanC || pFanB == pFanC )
                return Aig_And( p, p0, pFanD );
            if ( pFanB == pFanC || pFanB == pFanD )
                return Aig_And( p, pFanA, p1 );
            if ( pFanA == pFanD || pFanB == pFanD )
                return Aig_And( p, p0, pFanC );
            if ( pFanA == pFanC || pFanA == pFanD )
                return Aig_And( p, pFanB, p1 );
        }
        else if ( Aig_IsComplement(p0) && !Aig_IsComplement(p1) )
        {
            if ( pFanA == Aig_Not(pFanC) || pFanA == Aig_Not(pFanD) || pFanB == Aig_Not(pFanC) || pFanB == Aig_Not(pFanD) )
                return p1;
            if ( pFanB == pFanC || pFanB == pFanD )
                return Aig_And( p, Aig_Not(pFanA), p1 );
            if ( pFanA == pFanC || pFanA == pFanD )
                return Aig_And( p, Aig_Not(pFanB), p1 );
        }
        else if ( !Aig_IsComplement(p0) && Aig_IsComplement(p1) )
        {
            if ( pFanC == Aig_Not(pFanA) || pFanC == Aig_Not(pFanB) || pFanD == Aig_Not(pFanA) || pFanD == Aig_Not(pFanB) )
                return p0;
            if ( pFanD == pFanA || pFanD == pFanB )
                return Aig_And( p, Aig_Not(pFanC), p0 );
            if ( pFanC == pFanA || pFanC == pFanB )
                return Aig_And( p, Aig_Not(pFanD), p0 );
        }
        else // if ( Aig_IsComplement(p0) && Aig_IsComplement(p1) )
        {
            if ( pFanA == pFanD && pFanB == Aig_Not(pFanC) )
                return Aig_Not(pFanA);
            if ( pFanB == pFanC && pFanA == Aig_Not(pFanD) )
                return Aig_Not(pFanB);
            if ( pFanA == pFanC && pFanB == Aig_Not(pFanD) )
                return Aig_Not(pFanA);
            if ( pFanB == pFanD && pFanA == Aig_Not(pFanC) )
                return Aig_Not(pFanB);
        }
    }
    // check if it can be an EXOR gate
    if ( p->fCatchExor && Aig_ObjIsExorType( p0, p1, &pFan0, &pFan1 ) )
        return Aig_Exor( p, pFan0, pFan1 );
    pGhost = Aig_ObjCreateGhost( p, p0, p1, AIG_OBJ_AND );
    if ( (pResult = Aig_TableLookup( p, pGhost )) )
        return pResult;
    return Aig_ObjCreate( p, pGhost );
}


Aig_Obj_t * Aig_Exor( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 )
{
    Aig_Obj_t * pGhost, * pResult;
    int fCompl;
    // check trivial cases
    if ( p0 == p1 )
        return Aig_Not(p->pConst1);
    if ( p0 == Aig_Not(p1) )
        return p->pConst1;
    if ( Aig_Regular(p0) == p->pConst1 )
        return Aig_NotCond( p1, p0 == p->pConst1 );
    if ( Aig_Regular(p1) == p->pConst1 )
        return Aig_NotCond( p0, p1 == p->pConst1 );
    // when there is no special XOR gates
    if ( !p->fCatchExor )
        return Aig_Or( p, Aig_And(p, p0, Aig_Not(p1)), Aig_And(p, Aig_Not(p0), p1) );
    // canonicize
    fCompl = Aig_IsComplement(p0) ^ Aig_IsComplement(p1);
    p0 = Aig_Regular(p0);
    p1 = Aig_Regular(p1);
    pGhost = Aig_ObjCreateGhost( p, p0, p1, AIG_OBJ_EXOR );
    // check the table
    if ( (pResult = Aig_TableLookup( p, pGhost )) )
        return Aig_NotCond( pResult, fCompl );
    pResult = Aig_ObjCreate( p, pGhost );
    return Aig_NotCond( pResult, fCompl );
}


Aig_Obj_t * Aig_ObjCreate( Aig_Man_t * p, Aig_Obj_t * pGhost )
{
    Aig_Obj_t * pObj;
    assert( !Aig_IsComplement(pGhost) );
    assert( Aig_ObjIsHash(pGhost) );
//    assert( pGhost == &p->Ghost );
    // get memory for the new object
    pObj = Aig_ManFetchMemory( p );
    pObj->Type = pGhost->Type;
    // add connections
    Aig_ObjConnect( p, pObj, pGhost->pFanin0, pGhost->pFanin1 );
    // update node counters of the manager
    p->nObjs[Aig_ObjType(pObj)]++;
    assert( pObj->pData == NULL );
    // create the power counter
    if ( p->vProbs )
    {
        float Prob0 = Abc_Int2Float( Vec_IntEntry( p->vProbs, Aig_ObjFaninId0(pObj) ) );
        float Prob1 = Abc_Int2Float( Vec_IntEntry( p->vProbs, Aig_ObjFaninId1(pObj) ) );
        Prob0 = Aig_ObjFaninC0(pObj)? 1.0 - Prob0 : Prob0;
        Prob1 = Aig_ObjFaninC1(pObj)? 1.0 - Prob1 : Prob1;
        Vec_IntSetEntry( p->vProbs, pObj->Id, Abc_Float2Int(Prob0 * Prob1) );
    }
    return pObj;
}


Aig_Obj_t * Aig_Or( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 )
{
    return Aig_Not( Aig_And( p, Aig_Not(p0), Aig_Not(p1) ) );
}


char * Aig_MmFixedEntryFetch( Aig_MmFixed_t * p )
{
    char * pTemp;
    int i;

    // check if there are still free entries
    if ( p->nEntriesUsed == p->nEntriesAlloc )
    { // need to allocate more entries
        assert( p->pEntriesFree == NULL );
        if ( p->nChunks == p->nChunksAlloc )
        {
            p->nChunksAlloc *= 2;
            p->pChunks = ABC_REALLOC( char *, p->pChunks, p->nChunksAlloc );
        }
        p->pEntriesFree = ABC_ALLOC( char, p->nEntrySize * p->nChunkSize );
        p->nMemoryAlloc += p->nEntrySize * p->nChunkSize;
        // transform these entries into a linked list
        pTemp = p->pEntriesFree;
        for ( i = 1; i < p->nChunkSize; i++ )
        {
            *((char **)pTemp) = pTemp + p->nEntrySize;
            pTemp += p->nEntrySize;
        }
        // set the last link
        *((char **)pTemp) = NULL;
        // add the chunk to the chunk storage
        p->pChunks[ p->nChunks++ ] = p->pEntriesFree;
        // add to the number of entries allocated
        p->nEntriesAlloc += p->nChunkSize;
    }
    // incrememt the counter of used entries
    p->nEntriesUsed++;
    if ( p->nEntriesMax < p->nEntriesUsed )
        p->nEntriesMax = p->nEntriesUsed;
    // return the first entry in the free entry list
    pTemp = p->pEntriesFree;
    p->pEntriesFree = *((char **)pTemp);
    return pTemp;
}


void Abc_AigUpdateReset( Abc_Aig_t * pMan )
{
    assert( pMan->vAddedCells != NULL );
    Vec_PtrClear( pMan->vAddedCells );
    Vec_PtrClear( pMan->vUpdatedNets );
}


void Abc_AigRehash( Abc_Aig_t * pMan )
{
    Abc_Obj_t ** pBinsNew;
    Abc_Obj_t * pEnt, * pEnt2;
    int * pArray;
    unsigned Key;
    int Counter, Temp, i;

    // allocate a new array
    pBinsNew = ABC_ALLOC( Abc_Obj_t *, pMan->nBins );
    memset( pBinsNew, 0, sizeof(Abc_Obj_t *) * pMan->nBins );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < pMan->nBins; i++ )
        Abc_AigBinForEachEntrySafe( pMan->pBins[i], pEnt, pEnt2 )
        {
            // swap the fanins if needed
            pArray = pEnt->vFanins.pArray;
            if ( pArray[0] > pArray[1] )
            {
                Temp = pArray[0];
                pArray[0] = pArray[1];
                pArray[1] = Temp;
                Temp = pEnt->fCompl0;
                pEnt->fCompl0 = pEnt->fCompl1;
                pEnt->fCompl1 = Temp;
            }
            // rehash the node
            Key = Abc_HashKey2( Abc_ObjChild0(pEnt), Abc_ObjChild1(pEnt), pMan->nBins );
            pEnt->pNext   = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == pMan->nEntries );
    // replace the table and the parameters
    ABC_FREE( pMan->pBins );
    pMan->pBins = pBinsNew;
}


void Abc_AigReplace( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, int fUpdateLevel )
{
    assert( Vec_PtrSize(pMan->vStackReplaceOld) == 0 );
    assert( Vec_PtrSize(pMan->vStackReplaceNew) == 0 );
    Vec_PtrPush( pMan->vStackReplaceOld, pOld );
    Vec_PtrPush( pMan->vStackReplaceNew, pNew );
    assert( !Abc_ObjIsComplement(pOld) );
    // process the replacements
    while ( Vec_PtrSize(pMan->vStackReplaceOld) )
    {
        pOld = (Abc_Obj_t *)Vec_PtrPop( pMan->vStackReplaceOld );
        pNew = (Abc_Obj_t *)Vec_PtrPop( pMan->vStackReplaceNew );
        Abc_AigReplace_int( pMan, pOld, pNew, fUpdateLevel );
    }
    if ( fUpdateLevel )
    {
        Abc_AigUpdateLevel_int( pMan );
        if ( pMan->pNtkAig->vLevelsR )
            Abc_AigUpdateLevelR_int( pMan );
    }
}


void Abc_AigReplace_int( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, int fUpdateLevel )
{
    Abc_Obj_t * pFanin1, * pFanin2, * pFanout, * pFanoutNew, * pFanoutFanout;
    int k, v, iFanin;
    // make sure the old node is regular and has fanouts
    // (the new node can be complemented and can have fanouts)
    assert( !Abc_ObjIsComplement(pOld) );
    assert( Abc_ObjFanoutNum(pOld) > 0 );
    // look at the fanouts of old node
    Abc_NodeCollectFanouts( pOld, pMan->vNodes );
    Vec_PtrForEachEntry( Abc_Obj_t *, pMan->vNodes, pFanout, k )
    {
        if ( Abc_ObjIsCo(pFanout) )
        {
            Abc_ObjPatchFanin( pFanout, pOld, pNew );
            continue;
        }
        // find the old node as a fanin of this fanout
        iFanin = Vec_IntFind( &pFanout->vFanins, pOld->Id );
        assert( iFanin == 0 || iFanin == 1 );
        // get the new fanin
        pFanin1 = Abc_ObjNotCond( pNew, Abc_ObjFaninC(pFanout, iFanin) );
        assert( Abc_ObjRegular(pFanin1) != pFanout );
        // get another fanin
        pFanin2 = Abc_ObjChild( pFanout, iFanin ^ 1 );
        assert( Abc_ObjRegular(pFanin2) != pFanout );
        // check if the node with these fanins exists
        if ( (pFanoutNew = Abc_AigAndLookup( pMan, pFanin1, pFanin2 )) )
        { // such node exists (it may be a constant)
            // schedule replacement of the old fanout by the new fanout
            Vec_PtrPush( pMan->vStackReplaceOld, pFanout );
            Vec_PtrPush( pMan->vStackReplaceNew, pFanoutNew );
            continue;
        }
        // such node does not exist - modify the old fanout node
        // (this way the change will not propagate all the way to the COs)
        assert( Abc_ObjRegular(pFanin1) != Abc_ObjRegular(pFanin2) );

        // if the node is in the level structure, remove it
        if ( pFanout->fMarkA )
            Abc_AigRemoveFromLevelStructure( pMan->vLevels, pFanout );
        // if the node is in the level structure, remove it
        if ( pFanout->fMarkB )
            Abc_AigRemoveFromLevelStructureR( pMan->vLevelsR, pFanout );

        // remove the old fanout node from the structural hashing table
        Abc_AigAndDelete( pMan, pFanout );
        // remove the fanins of the old fanout
        Abc_ObjRemoveFanins( pFanout );
        // recreate the old fanout with new fanins and add it to the table
        Abc_AigAndCreateFrom( pMan, pFanin1, pFanin2, pFanout );
        assert( Abc_AigNodeIsAcyclic(pFanout, pFanout) );

        if ( fUpdateLevel )
        {
            // schedule the updated fanout for updating direct level
            assert( pFanout->fMarkA == 0 );
            pFanout->fMarkA = 1;
            Vec_VecPush( pMan->vLevels, pFanout->Level, pFanout );
            // schedule the updated fanout for updating reverse level
            if ( pMan->pNtkAig->vLevelsR )
            {
                assert( pFanout->fMarkB == 0 );
                pFanout->fMarkB = 1;
                Vec_VecPush( pMan->vLevelsR, Abc_ObjReverseLevel(pFanout), pFanout );
            }
        }

        // the fanout has changed, update EXOR status of its fanouts
        Abc_ObjForEachFanout( pFanout, pFanoutFanout, v )
            if ( Abc_AigNodeIsAnd(pFanoutFanout) )
                pFanoutFanout->fExor = Abc_NodeIsExorType(pFanoutFanout);
    }
    // if the node has no fanouts left, remove its MFFC
    if ( Abc_ObjFanoutNum(pOld) == 0 )
        Abc_AigDeleteNode( pMan, pOld );
}


void Abc_AigUpdateLevel_int( Abc_Aig_t * pMan )
{
    Abc_Obj_t * pNode, * pFanout;
    Vec_Ptr_t * vVec;
    int LevelNew, i, k, v;

    // go through the nodes and update the level of their fanouts
    Vec_VecForEachLevel( pMan->vLevels, vVec, i )
    {
        if ( Vec_PtrSize(vVec) == 0 )
            continue;
        Vec_PtrForEachEntry( Abc_Obj_t *, vVec, pNode, k )
        {
            if ( pNode == NULL )
                continue;
            assert( Abc_ObjIsNode(pNode) );
            assert( (int)pNode->Level == i );
            // clean the mark
            assert( pNode->fMarkA == 1 );
            pNode->fMarkA = 0;
            // iterate through the fanouts
            Abc_ObjForEachFanout( pNode, pFanout, v )
            {
                if ( Abc_ObjIsCo(pFanout) )
                    continue;
                // get the new level of this fanout
                LevelNew = 1 + Abc_MaxInt( Abc_ObjFanin0(pFanout)->Level, Abc_ObjFanin1(pFanout)->Level );
                assert( LevelNew > i );
                if ( (int)pFanout->Level == LevelNew ) // no change
                    continue;
                // if the fanout is present in the data structure, pull it out
                if ( pFanout->fMarkA )
                    Abc_AigRemoveFromLevelStructure( pMan->vLevels, pFanout );
                // update the fanout level
                pFanout->Level = LevelNew;
                // add the fanout to the data structure to update its fanouts
                assert( pFanout->fMarkA == 0 );
                pFanout->fMarkA = 1;
                Vec_VecPush( pMan->vLevels, pFanout->Level, pFanout );
            }
        }
        Vec_PtrClear( vVec );
    }
}


void Abc_AigUpdateLevelR_int( Abc_Aig_t * pMan )
{
    Abc_Obj_t * pNode, * pFanin, * pFanout;
    Vec_Ptr_t * vVec;
    int LevelNew, i, k, v, j;

    // go through the nodes and update the level of their fanouts
    Vec_VecForEachLevel( pMan->vLevelsR, vVec, i )
    {
        if ( Vec_PtrSize(vVec) == 0 )
            continue;
        Vec_PtrForEachEntry( Abc_Obj_t *, vVec, pNode, k )
        {
            if ( pNode == NULL )
                continue;
            assert( Abc_ObjIsNode(pNode) );
            assert( Abc_ObjReverseLevel(pNode) == i );
            // clean the mark
            assert( pNode->fMarkB == 1 );
            pNode->fMarkB = 0;
            // iterate through the fanins
            Abc_ObjForEachFanin( pNode, pFanin, v )
            {
                if ( Abc_ObjIsCi(pFanin) )
                    continue;
                // get the new reverse level of this fanin
                LevelNew = 0;
                Abc_ObjForEachFanout( pFanin, pFanout, j )
                    if ( LevelNew < Abc_ObjReverseLevel(pFanout) )
                        LevelNew = Abc_ObjReverseLevel(pFanout);
                LevelNew += 1;
                assert( LevelNew > i );
                if ( Abc_ObjReverseLevel(pFanin) == LevelNew ) // no change
                    continue;
                // if the fanin is present in the data structure, pull it out
                if ( pFanin->fMarkB )
                    Abc_AigRemoveFromLevelStructureR( pMan->vLevelsR, pFanin );
                // update the reverse level
                Abc_ObjSetReverseLevel( pFanin, LevelNew );
                // add the fanin to the data structure to update its fanins
                assert( pFanin->fMarkB == 0 );
                pFanin->fMarkB = 1;
                Vec_VecPush( pMan->vLevelsR, LevelNew, pFanin );
            }
        }
        Vec_PtrClear( vVec );
    }
}


Vec_Ptr_t * Abc_AigDfs( Abc_Ntk_t * pNtk, int fCollectAll, int fCollectCos )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    // set the traversal ID
    Abc_NtkIncrementTravId( pNtk );
    // start the array of nodes
    vNodes = Vec_PtrAlloc( 100 );
    // go through the PO nodes and call for each of them
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        Abc_AigDfs_rec( Abc_ObjFanin0(pNode), vNodes );
        Abc_NodeSetTravIdCurrent( pNode );
        if ( fCollectCos )
            Vec_PtrPush( vNodes, pNode );
    }
    // collect dangling nodes if asked to
    if ( fCollectAll )
    {
        Abc_NtkForEachNode( pNtk, pNode, i )
            if ( !Abc_NodeIsTravIdCurrent(pNode) )
                Abc_AigDfs_rec( pNode, vNodes );
    }
    return vNodes;
}


void Abc_AigDfs_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanin;
    int i;
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return;
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pNode );
    // skip the PI
    if ( Abc_ObjIsCi(pNode) || Abc_AigNodeIsConst(pNode) )
        return;
    assert( Abc_ObjIsNode( pNode ) );
    // visit the transitive fanin of the node
    Abc_ObjForEachFanin( pNode, pFanin, i )
        Abc_AigDfs_rec( pFanin, vNodes );
    // visit the equivalent nodes
    if ( Abc_AigNodeIsChoice( pNode ) )
        for ( pFanin = (Abc_Obj_t *)pNode->pData; pFanin; pFanin = (Abc_Obj_t *)pFanin->pData )
            Abc_AigDfs_rec( pFanin, vNodes );
    // add the node after the fanins have been added
    Vec_PtrPush( vNodes, pNode );
}


Abc_Obj_t * Abc_AigAndCreateFrom( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1, Abc_Obj_t * pAnd )
{
    Abc_Obj_t * pTemp;
    unsigned Key;
    assert( !Abc_ObjIsComplement(pAnd) );
    // order the arguments
    if ( Abc_ObjRegular(p0)->Id > Abc_ObjRegular(p1)->Id )
        pTemp = p0, p0 = p1, p1 = pTemp;
    // create the new node
    Abc_ObjAddFanin( pAnd, p0 );
    Abc_ObjAddFanin( pAnd, p1 );
    // set the level of the new node
    pAnd->Level      = 1 + Abc_MaxInt( Abc_ObjRegular(p0)->Level, Abc_ObjRegular(p1)->Level );
    pAnd->fExor      = Abc_NodeIsExorType(pAnd);
    // add the node to the corresponding linked list in the table
    Key = Abc_HashKey2( p0, p1, pMan->nBins );
    pAnd->pNext      = pMan->pBins[Key];
    pMan->pBins[Key] = pAnd;
    pMan->nEntries++;
    // create the cuts if defined
//    if ( pAnd->pNtk->pManCut )
//        Abc_NodeGetCuts( pAnd->pNtk->pManCut, pAnd );
    pAnd->pCopy = NULL;
    // add the node to the list of updated nodes
//    if ( pMan->vAddedCells )
//        Vec_PtrPush( pMan->vAddedCells, pAnd );
    return pAnd;
}


int Abc_AigNodeIsAcyclic( Abc_Obj_t * pNode, Abc_Obj_t * pRoot )
{
    Abc_Obj_t * pFanin0, * pFanin1;
    Abc_Obj_t * pChild00, * pChild01;
    Abc_Obj_t * pChild10, * pChild11;
    if ( !Abc_AigNodeIsAnd(pNode) )
        return 1;
    pFanin0 = Abc_ObjFanin0(pNode);
    pFanin1 = Abc_ObjFanin1(pNode);
    if ( pRoot == pFanin0 || pRoot == pFanin1 )
        return 0;
    if ( Abc_ObjIsCi(pFanin0) )
    {
        pChild00 = NULL;
        pChild01 = NULL;
    }
    else
    {
        pChild00 = Abc_ObjFanin0(pFanin0);
        pChild01 = Abc_ObjFanin1(pFanin0);
        if ( pRoot == pChild00 || pRoot == pChild01 )
            return 0;
    }
    if ( Abc_ObjIsCi(pFanin1) )
    {
        pChild10 = NULL;
        pChild11 = NULL;
    }
    else
    {
        pChild10 = Abc_ObjFanin0(pFanin1);
        pChild11 = Abc_ObjFanin1(pFanin1);
        if ( pRoot == pChild10 || pRoot == pChild11 )
            return 0;
    }
    return 1;
}
