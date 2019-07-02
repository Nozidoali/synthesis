#include "abc_ntk.h"


Abc_Ntk_t * Abc_NtkAlloc( Abc_NtkType_t Type, Abc_NtkFunc_t Func, int fUseMemMan )
{
    Abc_Ntk_t * pNtk;
    pNtk = ABC_ALLOC( Abc_Ntk_t, 1 );
    memset( pNtk, 0, sizeof(Abc_Ntk_t) );
    pNtk->ntkType     = Type;
    pNtk->ntkFunc     = Func;
    // start the object storage
    pNtk->vObjs       = Vec_PtrAlloc( 100 );
    pNtk->vPios       = Vec_PtrAlloc( 100 );
    pNtk->vPis        = Vec_PtrAlloc( 100 );
    pNtk->vPos        = Vec_PtrAlloc( 100 );
    pNtk->vCis        = Vec_PtrAlloc( 100 );
    pNtk->vCos        = Vec_PtrAlloc( 100 );
    pNtk->vBoxes      = Vec_PtrAlloc( 100 );
    pNtk->vLtlProperties = Vec_PtrAlloc( 100 );
    // start the memory managers
    pNtk->pMmObj      = fUseMemMan? Mem_FixedStart( sizeof(Abc_Obj_t) ) : NULL;
    pNtk->pMmStep     = fUseMemMan? Mem_StepStart( ABC_NUM_STEPS ) : NULL;
    // get ready to assign the first Obj ID
    pNtk->nTravIds    = 1;
     // start the functionality manager
     if ( !Abc_NtkIsStrash(pNtk) )
         Vec_PtrPush( pNtk->vObjs, NULL );
    if ( Abc_NtkIsStrash(pNtk) )
        pNtk->pManFunc = Abc_AigAlloc( pNtk );
    else if ( Abc_NtkHasSop(pNtk) || Abc_NtkHasBlifMv(pNtk) )
        pNtk->pManFunc = Mem_FlexStart();
// #ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        assert( 0 );
        // pNtk->pManFunc = Cudd_Init( 20, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
// #endif
    else if ( Abc_NtkHasAig(pNtk) )
        // pNtk->pManFunc = Hop_ManStart();
        assert( 0 );
    else if ( Abc_NtkHasMapping(pNtk) )
        // pNtk->pManFunc = Abc_FrameReadLibGen();
        assert( 0 );
    else if ( !Abc_NtkHasBlackbox(pNtk) )
        assert( 0 );
    // name manager
    pNtk->pManName = Nm_ManCreate( 200 );
    // attribute manager
    pNtk->vAttrs = Vec_PtrStart( VEC_ATTR_TOTAL_NUM );
    // estimated AndGateDelay
    // pNtk->AndGateDelay = 0.0;
    return pNtk;
}


void Abc_NtkDelete( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    void * pAttrMan;
    int TotalMemory, i;
//    int LargePiece = (4 << ABC_NUM_STEPS);
    if ( pNtk == NULL )
        return;
    // free EXDC Ntk
    if ( pNtk->pExdc )
        Abc_NtkDelete( pNtk->pExdc );
    if ( pNtk->pExcare )
        Abc_NtkDelete( (Abc_Ntk_t *)pNtk->pExcare );
    // dereference the BDDs
    if ( Abc_NtkHasBdd(pNtk) )
    {
#ifdef ABC_USE_CUDD
        Abc_NtkForEachNode( pNtk, pObj, i )
            Cudd_RecursiveDeref( (DdManager *)pNtk->pManFunc, (DdNode *)pObj->pData );
#endif
    }
    // make sure all the marks are clean
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        // these flags should be always zero
        // if this is not true, something is wrong somewhere
        assert( pObj->fMarkA == 0 );
        assert( pObj->fMarkB == 0 );
        assert( pObj->fMarkC == 0 );
    }
    // free the nodes
    if ( pNtk->pMmStep == NULL )
    {
        Abc_NtkForEachObj( pNtk, pObj, i )
        {
            ABC_FREE( pObj->vFanouts.pArray );
            ABC_FREE( pObj->vFanins.pArray );
        }
    }
    if ( pNtk->pMmObj == NULL )
    {
        Abc_NtkForEachObj( pNtk, pObj, i )
            ABC_FREE( pObj );
    }

    // free the arrays
    Vec_PtrFree( pNtk->vPios );
    Vec_PtrFree( pNtk->vPis );
    Vec_PtrFree( pNtk->vPos );
    Vec_PtrFree( pNtk->vCis );
    Vec_PtrFree( pNtk->vCos );
    Vec_PtrFree( pNtk->vObjs );
    Vec_PtrFree( pNtk->vBoxes );
    ABC_FREE( pNtk->vTravIds.pArray );
    if ( pNtk->vLevelsR ) Vec_IntFree( pNtk->vLevelsR );
    ABC_FREE( pNtk->pModel );
    // ABC_FREE( pNtk->pSeqModel );
    if ( pNtk->vSeqModelVec )
        Vec_PtrFreeFree( pNtk->vSeqModelVec );
    TotalMemory  = 0;
    TotalMemory += pNtk->pMmObj? Mem_FixedReadMemUsage(pNtk->pMmObj)  : 0;
    TotalMemory += pNtk->pMmStep? Mem_StepReadMemUsage(pNtk->pMmStep) : 0;
    // free the storage
    if ( pNtk->pMmObj )
        Mem_FixedStop( pNtk->pMmObj, 0 );
    if ( pNtk->pMmStep )
        Mem_StepStop ( pNtk->pMmStep, 0 );
    // name manager
    Nm_ManFree( pNtk->pManName );
    // free the timing manager
    // if ( pNtk->pManTime )
        // Abc_ManTimeStop( pNtk->pManTime );
    Vec_IntFreeP( &pNtk->vPhases );
    // start the functionality manager
    if ( Abc_NtkIsStrash(pNtk) )
        Abc_AigFree( (Abc_Aig_t *)pNtk->pManFunc );
    else if ( Abc_NtkHasSop(pNtk) || Abc_NtkHasBlifMv(pNtk) )
        Mem_FlexStop( (Mem_Flex_t *)pNtk->pManFunc, 0 );
// #ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        assert( 0 );
        // Extra_StopManager( (DdManager *)pNtk->pManFunc );
// #endif
    else if ( Abc_NtkHasAig(pNtk) )
        { if ( pNtk->pManFunc ) Hop_ManStop( (Hop_Man_t *)pNtk->pManFunc ); }
    else if ( Abc_NtkHasMapping(pNtk) )
        assert( 0 );
        // pNtk->pManFunc = NULL;
    else if ( !Abc_NtkHasBlackbox(pNtk) )
        assert( 0 );
    // free the hierarchy
    // if ( pNtk->pDesign )
    // {
    //     Abc_DesFree( pNtk->pDesign, pNtk );
    //     pNtk->pDesign = NULL;
    // }
    // free node attributes
    Vec_PtrForEachEntry( Abc_Obj_t *, pNtk->vAttrs, pAttrMan, i )
        if ( pAttrMan )
            Vec_AttFree( (Vec_Att_t *)pAttrMan, 1 );
    assert( pNtk->pSCLib == NULL );
    Vec_IntFreeP( &pNtk->vGates );
    Vec_PtrFree( pNtk->vAttrs );
    Vec_IntFreeP( &pNtk->vNameIds );
    ABC_FREE( pNtk->pWLoadUsed );
    ABC_FREE( pNtk->pName );
    ABC_FREE( pNtk->pSpec );
    ABC_FREE( pNtk->pLutTimes );
    if ( pNtk->vOnehots )
        Vec_VecFree( (Vec_Vec_t *)pNtk->vOnehots );
    Vec_PtrFreeP( &pNtk->vLtlProperties );
    Vec_IntFreeP( &pNtk->vObjPerm );
    Vec_IntFreeP( &pNtk->vTopo );
    Vec_IntFreeP( &pNtk->vFins );
    ABC_FREE( pNtk );
}


Abc_Obj_t * Abc_NtkFindNet( Abc_Ntk_t * pNtk, char * pName )
{
    Abc_Obj_t * pNet;
    int ObjId;
    assert( Abc_NtkIsNetlist(pNtk) );
    ObjId = Nm_ManFindIdByName( pNtk->pManName, pName, ABC_OBJ_NET );
    if ( ObjId == -1 )
        return NULL;
    pNet = Abc_NtkObj( pNtk, ObjId );
    return pNet;
}


Abc_Obj_t * Abc_NtkFindOrCreateNet( Abc_Ntk_t * pNtk, char * pName )
{
    Abc_Obj_t * pNet;
    assert( Abc_NtkIsNetlist(pNtk) );
    if ( pName && (pNet = Abc_NtkFindNet( pNtk, pName )) )
        return pNet;
//printf( "Creating net %s.\n", pName );
    // create a new net
    pNet = Abc_NtkCreateNet( pNtk );
    if ( pName )
        Nm_ManStoreIdName( pNtk->pManName, pNet->Id, pNet->Type, pName, NULL );
    return pNet;
}


Abc_Obj_t * Abc_NtkCreateObj( Abc_Ntk_t * pNtk, Abc_ObjType_t Type )
{
    Abc_Obj_t * pObj;
    // create new object, assign ID, and add to the array
    pObj = Abc_ObjAlloc( pNtk, Type );
    pObj->Id = pNtk->vObjs->nSize;
    Vec_PtrPush( pNtk->vObjs, pObj );
    pNtk->nObjCounts[Type]++;
    pNtk->nObjs++;
    // perform specialized operations depending on the object type
    switch (Type)
    {
        case ABC_OBJ_NONE:
            assert(0);
            break;
        case ABC_OBJ_CONST1:
            assert(0);
            break;
        case ABC_OBJ_PI:
//            pObj->iTemp = Vec_PtrSize(pNtk->vCis);
            Vec_PtrPush( pNtk->vPis, pObj );
            Vec_PtrPush( pNtk->vCis, pObj );
            break;
        case ABC_OBJ_PO:
//            pObj->iTemp = Vec_PtrSize(pNtk->vCos);
            Vec_PtrPush( pNtk->vPos, pObj );
            Vec_PtrPush( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_BI:
            if ( pNtk->vCos ) Vec_PtrPush( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_BO:
            if ( pNtk->vCis ) Vec_PtrPush( pNtk->vCis, pObj );
            break;
        case ABC_OBJ_NET:
        case ABC_OBJ_NODE:
            break;
        case ABC_OBJ_LATCH:
            // pObj->pData = (void *)ABC_INIT_NONE;
            assert( 0 );
        case ABC_OBJ_WHITEBOX:
        case ABC_OBJ_BLACKBOX:
            if ( pNtk->vBoxes ) Vec_PtrPush( pNtk->vBoxes, pObj );
            break;
        default:
            assert(0);
            break;
    }
    return pObj;
}


Abc_Obj_t * Abc_NtkCreateNodeInv( Abc_Ntk_t * pNtk, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk );
    if ( pFanin ) Abc_ObjAddFanin( pNode, pFanin );
    if ( Abc_NtkHasSop(pNtk) )
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, "0 1\n" );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_Not(Cudd_bddIthVar((DdManager *)pNtk->pManFunc,0)), Cudd_Ref( (DdNode *)pNode->pData );
#endif
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_Not(Hop_IthVar((Hop_Man_t *)pNtk->pManFunc,0));
    else if ( Abc_NtkHasMapping(pNtk) )
        // pNode->pData = Mio_LibraryReadInv((Mio_Library_t *)Abc_FrameReadLibGen());
        assert( 0 );
    else
        assert( 0 );
    return pNode;
}


Abc_Obj_t * Abc_NtkCreateNodeBuf( Abc_Ntk_t * pNtk, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk );
    if ( pFanin ) Abc_ObjAddFanin( pNode, pFanin );
    if ( Abc_NtkHasSop(pNtk) )
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, "1 1\n" );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_bddIthVar((DdManager *)pNtk->pManFunc,0), Cudd_Ref( (DdNode *)pNode->pData );
#endif
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_IthVar((Hop_Man_t *)pNtk->pManFunc,0);
    else if ( Abc_NtkHasMapping(pNtk) )
        // pNode->pData = Mio_LibraryReadBuf((Mio_Library_t *)Abc_FrameReadLibGen());
        assert( 0 );
    else
        assert( 0 );
    return pNode;
}


void Abc_NtkFinalizeRead( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pBox, * pObj, * pTerm, * pNet;
    int i;
    if ( Abc_NtkHasBlackbox(pNtk) && Abc_NtkBoxNum(pNtk) == 0 )
    {
        pBox = Abc_NtkCreateBlackbox(pNtk);
        Abc_NtkForEachPi( pNtk, pObj, i )
        {
            pTerm = Abc_NtkCreateBi(pNtk);
            Abc_ObjAddFanin( pTerm, Abc_ObjFanout0(pObj) );
            Abc_ObjAddFanin( pBox, pTerm );
        }
        Abc_NtkForEachPo( pNtk, pObj, i )
        {
            pTerm = Abc_NtkCreateBo(pNtk);
            Abc_ObjAddFanin( pTerm, pBox );
            Abc_ObjAddFanin( Abc_ObjFanin0(pObj), pTerm );
        }
        return;
    }
    assert( Abc_NtkIsNetlist(pNtk) );

    // check if constant 0 net is used
    pNet = Abc_NtkFindNet( pNtk, const_cast < char * >( "1\'b0" ) );
    if ( pNet )
    {
        if ( Abc_ObjFanoutNum(pNet) == 0 )
            Abc_NtkDeleteObj(pNet);
        else if ( Abc_ObjFaninNum(pNet) == 0 )
            Abc_ObjAddFanin( pNet, Abc_NtkCreateNodeConst0(pNtk) );
    }
    // check if constant 1 net is used
    pNet = Abc_NtkFindNet( pNtk, const_cast < char * >( "1\'b1" ) );
    if ( pNet )
    {
        if ( Abc_ObjFanoutNum(pNet) == 0 )
            Abc_NtkDeleteObj(pNet);
        else if ( Abc_ObjFaninNum(pNet) == 0 )
            Abc_ObjAddFanin( pNet, Abc_NtkCreateNodeConst1(pNtk) );
    }
    // fix the net drivers
    Abc_NtkFixNonDrivenNets( pNtk );

    // reorder the CI/COs to PI/POs first
    Abc_NtkOrderCisCos( pNtk );
}


void Abc_NtkFinalize( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew )
{
    Abc_Obj_t * pObj, * pDriver, * pDriverNew;
    int i;
    // set the COs of the strashed network
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pDriver    = Abc_ObjFanin0Ntk( Abc_ObjFanin0(pObj) );
        pDriverNew = Abc_ObjNotCond(pDriver->pCopy, Abc_ObjFaninC0(pObj));
        Abc_ObjAddFanin( pObj->pCopy, pDriverNew );
    }
    // duplicate timing manager
    // if ( pNtk->pManTime )
    //     Abc_NtkTimeInitialize( pNtkNew, pNtk );
    if ( pNtk->vPhases )
        Abc_NtkTransferPhases( pNtkNew, pNtk );
    if ( pNtk->pWLoadUsed )
        pNtkNew->pWLoadUsed = Abc_UtilStrsav( pNtk->pWLoadUsed );
}


void Abc_NtkTransferPhases( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    assert( pNtk->vPhases != NULL );
    assert( Vec_IntSize(pNtk->vPhases) == Abc_NtkObjNumMax(pNtk) );
    assert( pNtkNew->vPhases == NULL );
    pNtkNew->vPhases = Vec_IntStart( Abc_NtkObjNumMax(pNtkNew) );
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( pObj->pCopy && !Abc_ObjIsNone( (Abc_Obj_t *)pObj->pCopy ) )
            Vec_IntWriteEntry( pNtkNew->vPhases, Abc_ObjId( (Abc_Obj_t *)pObj->pCopy ), Vec_IntEntry(pNtk->vPhases, i) );
}


void Abc_NtkDeleteObj( Abc_Obj_t * pObj )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
    Vec_Ptr_t * vNodes;
    int i;
    assert( !Abc_ObjIsComplement(pObj) );
    // remove from the table of names
    if ( Nm_ManFindNameById(pObj->pNtk->pManName, pObj->Id) )
        Nm_ManDeleteIdName(pObj->pNtk->pManName, pObj->Id);
    // delete fanins and fanouts
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NodeCollectFanouts( pObj, vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        Abc_ObjDeleteFanin( (Abc_Obj_t *)vNodes->pArray[i], pObj );
    Abc_NodeCollectFanins( pObj, vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        Abc_ObjDeleteFanin( pObj, (Abc_Obj_t *)vNodes->pArray[i] );
    Vec_PtrFree( vNodes );
    // remove from the list of objects
    Vec_PtrWriteEntry( pNtk->vObjs, pObj->Id, NULL );
    pObj->Id = (1<<26)-1;
    pNtk->nObjCounts[pObj->Type]--;
    pNtk->nObjs--;
    // perform specialized operations depending on the object type
    switch (pObj->Type)
    {
        case ABC_OBJ_NONE:
            assert(0);
            break;
        case ABC_OBJ_CONST1:
            assert(0);
            break;
        case ABC_OBJ_PI:
            Vec_PtrRemove( pNtk->vPis, pObj );
            Vec_PtrRemove( pNtk->vCis, pObj );
            break;
        case ABC_OBJ_PO:
            Vec_PtrRemove( pNtk->vPos, pObj );
            Vec_PtrRemove( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_BI:
            if ( pNtk->vCos ) Vec_PtrRemove( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_BO:
            if ( pNtk->vCis ) Vec_PtrRemove( pNtk->vCis, pObj );
            break;
        case ABC_OBJ_NET:
            break;
        case ABC_OBJ_NODE:
// #ifdef ABC_USE_CUDD
//             if ( Abc_NtkHasBdd(pNtk) )
//                 Cudd_RecursiveDeref( (DdManager *)pNtk->pManFunc, (DdNode *)pObj->pData );
// #endif
            pObj->pData = NULL;
            break;
        case ABC_OBJ_LATCH:
        case ABC_OBJ_WHITEBOX:
        case ABC_OBJ_BLACKBOX:
            if ( pNtk->vBoxes ) Vec_PtrRemove( pNtk->vBoxes, pObj );
            break;
        default:
            assert(0);
            break;
    }
    // recycle the object memory
    Abc_ObjRecycle( pObj );
}


Abc_Obj_t * Abc_NtkCreateNodeConst0( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk );
    if ( Abc_NtkHasSop(pNtk) || Abc_NtkHasBlifMv(pNtk) )
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, " 0\n" );
// #ifdef ABC_USE_CUDD
//     else if ( Abc_NtkHasBdd(pNtk) )
//         pNode->pData = Cudd_ReadLogicZero((DdManager *)pNtk->pManFunc), Cudd_Ref( (DdNode *)pNode->pData );
// #endif
    else if ( Abc_NtkHasAig(pNtk) )
        // pNode->pData = Hop_ManConst0((Hop_Man_t *)pNtk->pManFunc);
        assert( 0 );
    else if ( Abc_NtkHasMapping(pNtk) )
        // pNode->pData = Mio_LibraryReadConst0((Mio_Library_t *)Abc_FrameReadLibGen());
        assert( 0 );
    else if ( !Abc_NtkHasBlackbox(pNtk) )
        assert( 0 );
    return pNode;
}


Abc_Obj_t * Abc_NtkCreateNodeConst1( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk );
    if ( Abc_NtkHasSop(pNtk) || Abc_NtkHasBlifMv(pNtk) )
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, " 1\n" );
// #ifdef ABC_USE_CUDD
//     else if ( Abc_NtkHasBdd(pNtk) )
//         pNode->pData = Cudd_ReadOne((DdManager *)pNtk->pManFunc), Cudd_Ref( (DdNode *)pNode->pData );
// #endif
    else if ( Abc_NtkHasAig(pNtk) )
        // pNode->pData = Hop_ManConst1((Hop_Man_t *)pNtk->pManFunc);
        assert( 0 );
    else if ( Abc_NtkHasMapping(pNtk) )
        // pNode->pData = Mio_LibraryReadConst1((Mio_Library_t *)Abc_FrameReadLibGen());
        assert( 0 );
    else if ( !Abc_NtkHasBlackbox(pNtk) )
        assert( 0 );
    return pNode;
}



void Abc_NtkFixNonDrivenNets( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNets;
    Abc_Obj_t * pNet, * pNode;
    int i;

    if ( Abc_NtkNodeNum(pNtk) == 0 && Abc_NtkBoxNum(pNtk) == 0 )
        return;

    // special case
    pNet = Abc_NtkFindNet( pNtk, const_cast < char * >( "[_c1_]" ) );
    if ( pNet != NULL )
    {
        pNode = Abc_NtkCreateNodeConst1( pNtk );
        Abc_ObjAddFanin( pNet, pNode );
    }

    // check for non-driven nets
    vNets = Vec_PtrAlloc( 100 );
    Abc_NtkForEachNet( pNtk, pNet, i )
    {
        if ( Abc_ObjFaninNum(pNet) > 0 )
            continue;
        // add the constant 0 driver
        pNode = Abc_NtkCreateNodeConst0( pNtk );
        // add the fanout net
        Abc_ObjAddFanin( pNet, pNode );
        // add the net to those for which the warning will be printed
        Vec_PtrPush( vNets, pNet );
    }

    // print the warning
    if ( vNets->nSize > 0 )
    {
        printf( "Warning: Constant-0 drivers added to %d non-driven nets in network \"%s\":\n", Vec_PtrSize(vNets), pNtk->pName );
        Vec_PtrForEachEntry( Abc_Obj_t *, vNets, pNet, i )
        {
            printf( "%s%s", (i? ", ": ""), Abc_ObjName(pNet) );
            if ( i == 3 )
            {
                if ( Vec_PtrSize(vNets) > 3 )
                    printf( " ..." );
                break;
            }
        }
        printf( "\n" );
    }
    Vec_PtrFree( vNets );
}


void Abc_NtkOrderCisCos( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pTerm;
    int i, k;
    Vec_PtrClear( pNtk->vCis );
    Vec_PtrClear( pNtk->vCos );
    Abc_NtkForEachPi( pNtk, pObj, i )
        Vec_PtrPush( pNtk->vCis, pObj );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Vec_PtrPush( pNtk->vCos, pObj );
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( Abc_ObjIsLatch(pObj) )
            continue;
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Vec_PtrPush( pNtk->vCos, pTerm );
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Vec_PtrPush( pNtk->vCis, pTerm );
    }
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsLatch(pObj) )
            continue;
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Vec_PtrPush( pNtk->vCos, pTerm );
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Vec_PtrPush( pNtk->vCis, pTerm );
    }
}


int Abc_NtkCheckRead( Abc_Ntk_t * pNtk )
{
   // return !Abc_FrameIsFlagEnabled( "checkread" ) || Abc_NtkDoCheck( pNtk );
   return Abc_NtkDoCheck( pNtk );
}


int Abc_NtkCheck( Abc_Ntk_t * pNtk )
{
   // return !Abc_FrameIsFlagEnabled( "check" ) || Abc_NtkDoCheck( pNtk );
   return Abc_NtkDoCheck( pNtk );
}


int Abc_NtkDoCheck( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pNet, * pNode;
    int i;

    // check network types
    if ( !Abc_NtkIsNetlist(pNtk) && !Abc_NtkIsLogic(pNtk) && !Abc_NtkIsStrash(pNtk) )
    {
        fprintf( stdout, "NetworkCheck: Unknown network type.\n" );
        return 0;
    }
    if ( !Abc_NtkHasSop(pNtk) && !Abc_NtkHasBdd(pNtk) && !Abc_NtkHasAig(pNtk) && !Abc_NtkHasMapping(pNtk) && !Abc_NtkHasBlifMv(pNtk) && !Abc_NtkHasBlackbox(pNtk) )
    {
        fprintf( stdout, "NetworkCheck: Unknown functionality type.\n" );
        return 0;
    }
    if ( Abc_NtkHasMapping(pNtk) )
    {
        assert( 0 );
        // if ( pNtk->pManFunc != Abc_FrameReadLibGen() )
        // {
        //     fprintf( stdout, "NetworkCheck: The library of the mapped network is not the global library.\n" );
        //     return 0;
        // }
    }

    if ( Abc_NtkHasOnlyLatchBoxes(pNtk) )
    {
        // check CI/CO numbers
        if ( Abc_NtkPiNum(pNtk) + Abc_NtkLatchNum(pNtk) != Abc_NtkCiNum(pNtk) )
        {
            fprintf( stdout, "NetworkCheck: Number of CIs does not match number of PIs and latches.\n" );
            fprintf( stdout, "One possible reason is that latches are added twice:\n" );
            fprintf( stdout, "in procedure Abc_NtkCreateObj() and in the user's code.\n" );
            return 0;
        }
        if ( Abc_NtkPoNum(pNtk) + Abc_NtkLatchNum(pNtk) != Abc_NtkCoNum(pNtk) )
        {
            fprintf( stdout, "NetworkCheck: Number of COs does not match number of POs, asserts, and latches.\n" );
            fprintf( stdout, "One possible reason is that latches are added twice:\n" );
            fprintf( stdout, "in procedure Abc_NtkCreateObj() and in the user's code.\n" );
            return 0;
        }
    }

    // check the names
    if ( !Abc_NtkCheckNames( pNtk ) )
        return 0;

    // check PIs and POs
    Abc_NtkCleanCopy( pNtk );
    if ( !Abc_NtkCheckPis( pNtk ) )
        return 0;
    if ( !Abc_NtkCheckPos( pNtk ) )
        return 0;

    if ( Abc_NtkHasBlackbox(pNtk) )
        return 1;

    // check the connectivity of objects
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_NtkCheckObj( pNtk, pObj ) )
            return 0;

    // if it is a netlist change nets and latches
    if ( Abc_NtkIsNetlist(pNtk) )
    {
        if ( Abc_NtkNetNum(pNtk) == 0 )
            fprintf( stdout, "NetworkCheck: Warning! Netlist has no nets.\n" );
        // check the nets
        Abc_NtkForEachNet( pNtk, pNet, i )
            if ( !Abc_NtkCheckNet( pNtk, pNet ) )
                return 0;
    }
    else
    {
        if ( Abc_NtkNetNum(pNtk) != 0 )
        {
            fprintf( stdout, "NetworkCheck: A network that is not a netlist has nets.\n" );
            return 0;
        }
    }

    // check the nodes
    if ( Abc_NtkIsStrash(pNtk) )
        // assert( 0 );
        Abc_AigCheck( (Abc_Aig_t *)pNtk->pManFunc );
    else
    {
        Abc_NtkForEachNode( pNtk, pNode, i )
            if ( !Abc_NtkCheckNode( pNtk, pNode ) )
                return 0;
    }

    // check the latches
    Abc_NtkForEachLatch( pNtk, pNode, i )
        assert( 0 );
        // if ( !Abc_NtkCheckLatch( pNtk, pNode ) )
        //     return 0;

    // finally, check for combinational loops
//  clk = Abc_Clock();
    if ( !Abc_NtkIsAcyclic( pNtk ) )
    {
        fprintf( stdout, "NetworkCheck: Network contains a combinational loop.\n" );
        return 0;
    }
//  ABC_PRT( "Acyclic  ", Abc_Clock() - clk );

    // check the EXDC network if present
    if ( pNtk->pExdc )
        Abc_NtkCheck( pNtk->pExdc );
    return 1;
}


int Abc_NtkCheckNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj = NULL; // Ensure pObj isn't used uninitialized.
    Vec_Int_t * vNameIds;
    char * pName;
    int i, NameId;

    if ( Abc_NtkIsNetlist(pNtk) )
        return 1;

    // check that each CI/CO has a name
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        pObj = Abc_ObjFanout0Ntk(pObj);
        if ( Nm_ManFindNameById(pObj->pNtk->pManName, pObj->Id) == NULL )
        {
            fprintf( stdout, "NetworkCheck: CI with ID %d is in the network but not in the name table.\n", pObj->Id );
            return 0;
        }
    }
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pObj = Abc_ObjFanin0Ntk(pObj);
        if ( Nm_ManFindNameById(pObj->pNtk->pManName, pObj->Id) == NULL )
        {
            fprintf( stdout, "NetworkCheck: CO with ID %d is in the network but not in the name table.\n", pObj->Id );
            return 0;
        }
    }

    assert(pObj); // pObj should point to something here.

    // return the array of all IDs, which have names
    vNameIds = Nm_ManReturnNameIds( pNtk->pManName );
    // make sure that these IDs correspond to live objects
    Vec_IntForEachEntry( vNameIds, NameId, i )
    {
        if ( Vec_PtrEntry( pNtk->vObjs, NameId ) == NULL )
        {
            Vec_IntFree( vNameIds );
            pName = Nm_ManFindNameById(pObj->pNtk->pManName, NameId);
            fprintf( stdout, "NetworkCheck: Object with ID %d is deleted but its name \"%s\" remains in the name table.\n", NameId, pName );
            return 0;
        }
    }
    Vec_IntFree( vNameIds );

    // make sure the CI names are unique
    if ( !Abc_NtkCheckUniqueCiNames(pNtk) )
        return 0;

    // make sure the CO names are unique
    if ( !Abc_NtkCheckUniqueCoNames(pNtk) )
        return 0;

    // make sure that if a CO has the same name as a CI, they point directly
    if ( !Abc_NtkCheckUniqueCioNames(pNtk) )
        return 0;

    return 1;
}


static int Vec_PtrSortComparePtr( void ** pp1, void ** pp2 )
{
    if ( *pp1 < *pp2 )
        return -1;
    if ( *pp1 > *pp2 )
        return 1;
    return 0;
}


static void Vec_PtrSort( Vec_Ptr_t * p, int (*Vec_PtrSortCompare)() )
{
    if ( p->nSize < 2 )
        return;
    if ( Vec_PtrSortCompare == NULL )
        qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(void *),
                (int (*)(const void *, const void *)) Vec_PtrSortComparePtr );
    else
        qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(void *),
                (int (*)(const void *, const void *)) Vec_PtrSortCompare );
}


int Abc_NtkCheckUniqueCiNames( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNames;
    Abc_Obj_t * pObj;
    int i, fRetValue = 1;
    assert( !Abc_NtkIsNetlist(pNtk) );
    vNames = Vec_PtrAlloc( Abc_NtkCiNum(pNtk) );
    Abc_NtkForEachCi( pNtk, pObj, i )
        Vec_PtrPush( vNames, Abc_ObjName(pObj) );
    Vec_PtrSort( vNames, (int (*)())Abc_NtkNamesCompare );
    for ( i = 1; i < Abc_NtkCiNum(pNtk); i++ )
        if ( !strcmp( (const char *)Vec_PtrEntry(vNames,i-1), (const char *)Vec_PtrEntry(vNames,i) ) )
        {
            printf( "Abc_NtkCheck: Repeated CI names: %s and %s.\n", (char*)Vec_PtrEntry(vNames,i-1), (char*)Vec_PtrEntry(vNames,i) );
            fRetValue = 0;
        }
    Vec_PtrFree( vNames );
    return fRetValue;
}


int Abc_NtkCheckUniqueCoNames( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNames;
    Abc_Obj_t * pObj;
    int i, fRetValue = 1;
    assert( !Abc_NtkIsNetlist(pNtk) );
    vNames = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_PtrPush( vNames, Abc_ObjName(pObj) );
    Vec_PtrSort( vNames, (int (*)())Abc_NtkNamesCompare );
    for ( i = 1; i < Abc_NtkCoNum(pNtk); i++ )
    {
//        printf( "%s\n", Vec_PtrEntry(vNames,i) );
        if ( !strcmp( (const char *)Vec_PtrEntry(vNames,i-1), (const char *)Vec_PtrEntry(vNames,i) ) )
        {
            printf( "Abc_NtkCheck: Repeated CO names: %s and %s.\n", (char*)Vec_PtrEntry(vNames,i-1), (char*)Vec_PtrEntry(vNames,i) );
            fRetValue = 0;
        }
    }
    Vec_PtrFree( vNames );
    return fRetValue;
}


int Abc_NtkCheckUniqueCioNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pObjCi, * pFanin;
    int i, nCiId, fRetValue = 1;
    assert( !Abc_NtkIsNetlist(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        nCiId = Nm_ManFindIdByNameTwoTypes( pNtk->pManName, Abc_ObjName(pObj), ABC_OBJ_PI, ABC_OBJ_BO );
        if ( nCiId == -1 )
            continue;
        pObjCi = Abc_NtkObj( pNtk, nCiId );
        assert( !strcmp( Abc_ObjName(pObj), Abc_ObjName(pObjCi) ) );
        pFanin = Abc_ObjFanin0(pObj);
        if ( pFanin != pObjCi )
        {
            printf( "Abc_NtkCheck: A CI/CO pair share the name (%s) but do not link directly. The name of the CO fanin is %s.\n",
                Abc_ObjName(pObj), Abc_ObjName(Abc_ObjFanin0(pObj)) );
            fRetValue = 0;
        }
    }
    return fRetValue;
}


int Abc_NtkNamesCompare( char ** pName1, char ** pName2 )
{
    return strcmp( *pName1, *pName2 );
}


void Abc_NtkCleanCopy( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->pCopy = NULL;
}


int Abc_NtkCheckPis( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;

    // check that PIs are indeed PIs
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsPi(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Object \"%s\" (id=%d) is in the PI list but is not a PI.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        if ( pObj->pData )
        {
            fprintf( stdout, "NetworkCheck: A PI \"%s\" has a logic function.\n", Abc_ObjName(pObj) );
            return 0;
        }
        if ( Abc_ObjFaninNum(pObj) > 0 )
        {
            fprintf( stdout, "NetworkCheck: A PI \"%s\" has fanins.\n", Abc_ObjName(pObj) );
            return 0;
        }
        pObj->pCopy = (Abc_Obj_t *)1;
    }
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( pObj->pCopy == NULL && Abc_ObjIsPi(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Object \"%s\" (id=%d) is a PI but is not in the PI list.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        pObj->pCopy = NULL;
    }
    return 1;
}


int Abc_NtkCheckPos( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;

    // check that POs are indeed POs
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsPo(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Net \"%s\" (id=%d) is in the PO list but is not a PO.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        if ( pObj->pData )
        {
            fprintf( stdout, "NetworkCheck: A PO \"%s\" has a logic function.\n", Abc_ObjName(pObj) );
            return 0;
        }
        if ( Abc_ObjFaninNum(pObj) != 1 )
        {
            fprintf( stdout, "NetworkCheck: A PO \"%s\" does not have one fanin (but %d).\n", Abc_ObjName(pObj), Abc_ObjFaninNum(pObj) );
            return 0;
        }
        if ( Abc_ObjFanoutNum(pObj) > 0 )
        {
            fprintf( stdout, "NetworkCheck: A PO \"%s\" has %d fanout(s).\n", Abc_ObjName(pObj), Abc_ObjFanoutNum(pObj) );
            return 0;
        }
        pObj->pCopy = (Abc_Obj_t *)1;
    }
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( pObj->pCopy == NULL && Abc_ObjIsPo(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Net \"%s\" (id=%d) is in a PO but is not in the PO list.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        pObj->pCopy = NULL;
    }
    return 1;
}


int Abc_NtkCheckObj( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin, * pFanout;
    int Value = 1;
    int i, k;

    // check the network
    if ( pObj->pNtk != pNtk )
    {
        fprintf( stdout, "NetworkCheck: Object \"%s\" does not belong to the network.\n", Abc_ObjName(pObj) );
        return 0;
    }
    // check the object ID
    if ( pObj->Id < 0 || (int)pObj->Id >= Abc_NtkObjNumMax(pNtk) )
    {
        fprintf( stdout, "NetworkCheck: Object \"%s\" has incorrect ID.\n", Abc_ObjName(pObj) );
        return 0;
    }

    // if ( !Abc_FrameIsFlagEnabled("checkfio") )
    //     return Value;

    // go through the fanins of the object and make sure fanins have this object as a fanout
    Abc_ObjForEachFanin( pObj, pFanin, i )
    {
        if ( Vec_IntFind( &pFanin->vFanouts, pObj->Id ) == -1 )
        {
            fprintf( stdout, "NodeCheck: Object \"%s\" has fanin ", Abc_ObjName(pObj) );
            fprintf( stdout, "\"%s\" but the fanin does not have it as a fanout.\n", Abc_ObjName(pFanin) );
            Value = 0;
        }
    }
    // go through the fanouts of the object and make sure fanouts have this object as a fanin
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        if ( Vec_IntFind( &pFanout->vFanins, pObj->Id ) == -1 )
        {
            fprintf( stdout, "NodeCheck: Object \"%s\" has fanout ", Abc_ObjName(pObj) );
            fprintf( stdout, "\"%s\" but the fanout does not have it as a fanin.\n", Abc_ObjName(pFanout) );
            Value = 0;
        }
    }

    // make sure fanins are not duplicated
    for ( i = 0; i < pObj->vFanins.nSize; i++ )
        for ( k = i + 1; k < pObj->vFanins.nSize; k++ )
            if ( pObj->vFanins.pArray[k] == pObj->vFanins.pArray[i] )
            {
                printf( "Warning: Node %s has", Abc_ObjName(pObj) );
                printf( " duplicated fanin %s.\n", Abc_ObjName(Abc_ObjFanin(pObj,k)) );
            }

    // save time: do not check large fanout lists
    if ( pObj->vFanouts.nSize > 100 )
        return Value;

    // make sure fanouts are not duplicated
    for ( i = 0; i < pObj->vFanouts.nSize; i++ )
        for ( k = i + 1; k < pObj->vFanouts.nSize; k++ )
            if ( pObj->vFanouts.pArray[k] == pObj->vFanouts.pArray[i] )
            {
                printf( "Warning: Node %s has", Abc_ObjName(pObj) );
                printf( " duplicated fanout %s.\n", Abc_ObjName(Abc_ObjFanout(pObj,k)) );
            }

    return Value;
}


int Abc_NtkCheckNet( Abc_Ntk_t * pNtk, Abc_Obj_t * pNet )
{
    if ( Abc_ObjFaninNum(pNet) == 0 )
    {
        fprintf( stdout, "NetworkCheck: Net \"%s\" is not driven.\n", Abc_ObjName(pNet) );
        return 0;
    }
    if ( Abc_ObjFaninNum(pNet) > 1 )
    {
        fprintf( stdout, "NetworkCheck: Net \"%s\" has more than one driver.\n", Abc_ObjName(pNet) );
        return 0;
    }
    return 1;
}


int Abc_NtkCheckNode( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode )
{
    // detect internal nodes that do not have nets
    if ( Abc_NtkIsNetlist(pNtk) && Abc_ObjFanoutNum(pNode) == 0 )
    {
        fprintf( stdout, "Node (id = %d) has no net to drive.\n", pNode->Id );
        return 0;
    }
    // the node should have a function assigned unless it is an AIG
    if ( pNode->pData == NULL )
    {
        if ( Abc_ObjIsBarBuf(pNode) )
            return 1;
        fprintf( stdout, "NodeCheck: An internal node \"%s\" does not have a logic function.\n", Abc_ObjNameNet(pNode) );
        return 0;
    }
    // the netlist and SOP logic network should have SOPs
    if ( Abc_NtkHasSop(pNtk) )
    {
        if ( !Abc_SopCheck( (char *)pNode->pData, Abc_ObjFaninNum(pNode) ) )
        {
            fprintf( stdout, "NodeCheck: SOP check for node \"%s\" has failed.\n", Abc_ObjNameNet(pNode) );
            return 0;
        }
    }
    else if ( Abc_NtkHasBdd(pNtk) )
    {
#ifdef ABC_USE_CUDD
        int nSuppSize = Cudd_SupportSize((DdManager *)pNtk->pManFunc, (DdNode *)pNode->pData);
        if ( nSuppSize > Abc_ObjFaninNum(pNode) )
        {
            fprintf( stdout, "NodeCheck: BDD of the node \"%s\" has incorrect support size.\n", Abc_ObjNameNet(pNode) );
            return 0;
        }
#endif
    }
    else if ( !Abc_NtkHasMapping(pNtk) && !Abc_NtkHasBlifMv(pNtk) && !Abc_NtkHasAig(pNtk) )
    {
        assert( 0 );
    }
    return 1;
}


int Abc_NtkIsAcyclic( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int fAcyclic;
    int i;
    // set the traversal ID for this DFS ordering
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkIncrementTravId( pNtk );
    // pNode->TravId == pNet->nTravIds      means "pNode is on the path"
    // pNode->TravId == pNet->nTravIds - 1  means "pNode is visited but is not on the path"
    // pNode->TravId <  pNet->nTravIds - 1  means "pNode is not visited"
    // traverse the network to detect cycles
    fAcyclic = 1;
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pNode = Abc_ObjFanin0Ntk(Abc_ObjFanin0(pNode));
        if ( Abc_NodeIsTravIdPrevious(pNode) )
            continue;
        // traverse the output logic cone
        if ( (fAcyclic = Abc_NtkIsAcyclic_rec(pNode)) )
            continue;
        // stop as soon as the first loop is detected
        fprintf( stdout, " CO \"%s\"\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        break;
    }
    return fAcyclic;
}


int Abc_NtkIsAcyclic_rec( Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pNtk = pNode->pNtk;
    Abc_Obj_t * pFanin;
    int fAcyclic, i;
    assert( !Abc_ObjIsNet(pNode) );
    if ( Abc_ObjIsCi(pNode) || Abc_ObjIsBox(pNode) || (Abc_NtkIsStrash(pNode->pNtk) && Abc_AigNodeIsConst(pNode)) )
        return 1;
    assert( Abc_ObjIsNode(pNode) );
    // make sure the node is not visited
    assert( !Abc_NodeIsTravIdPrevious(pNode) );
    // check if the node is part of the combinational loop
    if ( Abc_NodeIsTravIdCurrent(pNode) )
    {
        fprintf( stdout, "Network \"%s\" contains combinational loop!\n", Abc_NtkName(pNtk) );
        fprintf( stdout, "Node \"%s\" is encountered twice on the following path to the COs:\n", Abc_ObjName(pNode) );
        return 0;
    }
    // mark this node as a node on the current path
    Abc_NodeSetTravIdCurrent( pNode );
    // visit the transitive fanin
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        pFanin = Abc_ObjFanin0Ntk(pFanin);
        // make sure there is no mixing of networks
        assert( pFanin->pNtk == pNode->pNtk );
        // check if the fanin is visited
        if ( Abc_NodeIsTravIdPrevious(pFanin) )
            continue;
        // traverse the fanin's cone searching for the loop
        if ( (fAcyclic = Abc_NtkIsAcyclic_rec(pFanin)) )
            continue;
        // return as soon as the loop is detected
        fprintf( stdout, " %s ->", Abc_ObjName(pFanin) );
        return 0;
    }
    // visit choices
    if ( Abc_NtkIsStrash(pNode->pNtk) && Abc_AigNodeIsChoice(pNode) )
    {
        for ( pFanin = (Abc_Obj_t *)pNode->pData; pFanin; pFanin = (Abc_Obj_t *)pFanin->pData )
        {
            // check if the fanin is visited
            if ( Abc_NodeIsTravIdPrevious(pFanin) )
                continue;
            // traverse the fanin's cone searching for the loop
            if ( (fAcyclic = Abc_NtkIsAcyclic_rec(pFanin)) )
                continue;
            // return as soon as the loop is detected
            fprintf( stdout, " %s", Abc_ObjName(pFanin) );
            fprintf( stdout, " (choice of %s) -> ", Abc_ObjName(pNode) );
            return 0;
        }
    }
    // mark this node as a visited node
    Abc_NodeSetTravIdPrevious( pNode );
    return 1;
}


Abc_Obj_t * Abc_NtkDupObj( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, int fCopyName )
{
    Abc_Obj_t * pObjNew;
    // create the new object
    pObjNew = Abc_NtkCreateObj( pNtkNew, (Abc_ObjType_t)pObj->Type );
    // transfer names of the terminal objects
    if ( fCopyName )
    {
        if ( Abc_ObjIsCi(pObj) )
        {
            if ( !Abc_NtkIsNetlist(pNtkNew) )
                Abc_ObjAssignName( pObjNew, Abc_ObjName(Abc_ObjFanout0Ntk(pObj)), NULL );
        }
        else if ( Abc_ObjIsCo(pObj) )
        {
            if ( !Abc_NtkIsNetlist(pNtkNew) )
            {
                if ( Abc_ObjIsPo(pObj) )
                    Abc_ObjAssignName( pObjNew, Abc_ObjName(Abc_ObjFanin0Ntk(pObj)), NULL );
                else
                {
                    assert( Abc_ObjIsLatch(Abc_ObjFanout0(pObj)) );
                    Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
                }
            }
        }
        else if ( Abc_ObjIsBox(pObj) || Abc_ObjIsNet(pObj) )
            Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
    }
    // copy functionality/names
    if ( Abc_ObjIsNode(pObj) ) // copy the function if functionality is compatible
    {
        if ( pNtkNew->ntkFunc == pObj->pNtk->ntkFunc )
        {
            if ( Abc_NtkIsStrash(pNtkNew) )
            {}
            else if ( Abc_NtkHasSop(pNtkNew) || Abc_NtkHasBlifMv(pNtkNew) )
                pObjNew->pData = Abc_SopRegister( (Mem_Flex_t *)pNtkNew->pManFunc, (char *)pObj->pData );
#ifdef ABC_USE_CUDD
            else if ( Abc_NtkHasBdd(pNtkNew) )
                pObjNew->pData = Cudd_bddTransfer((DdManager *)pObj->pNtk->pManFunc, (DdManager *)pNtkNew->pManFunc, (DdNode *)pObj->pData), Cudd_Ref((DdNode *)pObjNew->pData);
#endif
            else if ( Abc_NtkHasAig(pNtkNew) )
                pObjNew->pData = Hop_Transfer((Hop_Man_t *)pObj->pNtk->pManFunc, (Hop_Man_t *)pNtkNew->pManFunc, (Hop_Obj_t *)pObj->pData, Abc_ObjFaninNum(pObj));
            else if ( Abc_NtkHasMapping(pNtkNew) )
                pObjNew->pData = pObj->pData, pNtkNew->nBarBufs2 += !pObj->pData;
            else assert( 0 );
        }
    }
    else if ( Abc_ObjIsNet(pObj) ) // copy the name
    {
    }
    else if ( Abc_ObjIsLatch(pObj) ) // copy the reset value
        pObjNew->pData = pObj->pData;
    // transfer HAIG
//    pObjNew->pEquiv = pObj->pEquiv;
    // remember the new node in the old node
    pObj->pCopy = pObjNew;
    return pObjNew;
}


Abc_Obj_t * Abc_NtkDupBox( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pBox, int fCopyName )
{
    Abc_Obj_t * pTerm, * pBoxNew;
    int i;
    assert( Abc_ObjIsBox(pBox) );
    // duplicate the box
    pBoxNew = Abc_NtkDupObj( pNtkNew, pBox, fCopyName );
    // duplicate the fanins and connect them
    Abc_ObjForEachFanin( pBox, pTerm, i )
        Abc_ObjAddFanin( pBoxNew, Abc_NtkDupObj(pNtkNew, pTerm, fCopyName) );
    // duplicate the fanouts and connect them
    Abc_ObjForEachFanout( pBox, pTerm, i )
        Abc_ObjAddFanin( Abc_NtkDupObj(pNtkNew, pTerm, fCopyName), pBoxNew );
    return pBoxNew;
}


Vec_Ptr_t * Abc_NtkDfs( Abc_Ntk_t * pNtk, int fCollectAll )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i;
    // set the traversal ID
    Abc_NtkIncrementTravId( pNtk );
    // start the array of nodes
    vNodes = Vec_PtrAlloc( 100 );
    if ( pNtk->nBarBufs2 > 0 )
    {
        Abc_NtkForEachBarBuf( pNtk, pObj, i )
        {
            Abc_NodeSetTravIdCurrent( pObj );
            Abc_NtkDfs_rec( Abc_ObjFanin0Ntk(Abc_ObjFanin0(pObj)), vNodes );
            Vec_PtrPush( vNodes, pObj );
        }
    }
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        Abc_NodeSetTravIdCurrent( pObj );
        Abc_NtkDfs_rec( Abc_ObjFanin0Ntk(Abc_ObjFanin0(pObj)), vNodes );
    }
    // collect dangling nodes if asked to
    if ( fCollectAll )
    {
        Abc_NtkForEachNode( pNtk, pObj, i )
            if ( !Abc_NodeIsTravIdCurrent(pObj) )
                Abc_NtkDfs_rec( pObj, vNodes );
    }
    return vNodes;
}


void Abc_NtkDfs_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanin;
    int i;
    assert( !Abc_ObjIsNet(pNode) );
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return;
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pNode );
    // skip the CI
    if ( Abc_ObjIsCi(pNode) || (Abc_NtkIsStrash(pNode->pNtk) && Abc_AigNodeIsConst(pNode)) )
        return;
    assert( Abc_ObjIsNode( pNode ) || Abc_ObjIsBox( pNode ) );
    // visit the transitive fanin of the node
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
//        pFanin = Abc_ObjFanin( pNode, Abc_ObjFaninNum(pNode)-1-i );
        Abc_NtkDfs_rec( Abc_ObjFanin0Ntk(pFanin), vNodes );
    }
    // add the node after the fanins have been added
    Vec_PtrPush( vNodes, pNode );
}


Vec_Ptr_t * Abc_NtkDfsIter( Abc_Ntk_t * pNtk, int fCollectAll )
{
    Vec_Ptr_t * vNodes, * vStack;
    Abc_Obj_t * pObj;
    int i;
    // set the traversal ID
    Abc_NtkIncrementTravId( pNtk );
    // start the array of nodes
    vNodes = Vec_PtrAlloc( 1000 );
    vStack = Vec_PtrAlloc( 1000 );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        Abc_NodeSetTravIdCurrent( pObj );
        Abc_NtkDfs_iter( vStack, Abc_ObjFanin0Ntk(Abc_ObjFanin0(pObj)), vNodes );
    }
    // collect dangling nodes if asked to
    if ( fCollectAll )
    {
        Abc_NtkForEachNode( pNtk, pObj, i )
            if ( !Abc_NodeIsTravIdCurrent(pObj) )
                Abc_NtkDfs_iter( vStack, pObj, vNodes );
    }
    Vec_PtrFree( vStack );
    return vNodes;
}


void Abc_NtkDfs_iter( Vec_Ptr_t * vStack, Abc_Obj_t * pRoot, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pNode, * pFanin;
    int iFanin;
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pRoot ) )
        return;
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pRoot );
    // skip the CI
    if ( Abc_ObjIsCi(pRoot) || (Abc_NtkIsStrash(pRoot->pNtk) && Abc_AigNodeIsConst(pRoot)) )
        return;
    // add the CI
    Vec_PtrClear( vStack );
    Vec_PtrPush( vStack, pRoot );
    Vec_PtrPush( vStack, (void *)0 );
    while ( Vec_PtrSize(vStack) > 0 )
    {
        // get the node and its fanin
        iFanin = (int)(ABC_PTRINT_T)Vec_PtrPop(vStack);
        pNode  = (Abc_Obj_t *)Vec_PtrPop(vStack);
        assert( !Abc_ObjIsNet(pNode) );
        // add it to the array of nodes if we finished
        if ( iFanin == Abc_ObjFaninNum(pNode) )
        {
            Vec_PtrPush( vNodes, pNode );
            continue;
        }
        // explore the next fanin
        Vec_PtrPush( vStack, pNode );
        Vec_PtrPush( vStack, (void *)(ABC_PTRINT_T)(iFanin+1) );
        // get the fanin
        pFanin = Abc_ObjFanin0Ntk( Abc_ObjFanin(pNode,iFanin) );
        // if this node is already visited, skip
        if ( Abc_NodeIsTravIdCurrent( pFanin ) )
            continue;
        // mark the node as visited
        Abc_NodeSetTravIdCurrent( pFanin );
        // skip the CI
        if ( Abc_ObjIsCi(pFanin) || (Abc_NtkIsStrash(pFanin->pNtk) && Abc_AigNodeIsConst(pFanin)) )
            continue;
        Vec_PtrPush( vStack, pFanin );
        Vec_PtrPush( vStack, (void *)0 );
    }
}


int Abc_NtkGetFaninMax( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, nFaninsMax = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( nFaninsMax < Abc_ObjFaninNum(pNode) )
            nFaninsMax = Abc_ObjFaninNum(pNode);
    }
    return nFaninsMax;
}


int Abc_NtkGetFanoutMax( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, nFaninsMax = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( nFaninsMax < Abc_ObjFanoutNum(pNode) )
            nFaninsMax = Abc_ObjFanoutNum(pNode);
    }
    return nFaninsMax;
}


int Abc_NtkLevel( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, LevelsMax;
    // set the CI levels
    // if ( pNtk->pManTime == NULL || pNtk->AndGateDelay <= 0 )
        Abc_NtkForEachCi( pNtk, pNode, i )
            pNode->Level = 0;
    // else
        // Abc_NtkForEachCi( pNtk pNode, i )
        //     pNode->Level = (int)(Abc_MaxFloat(0, Abc_NodeReadArrivalWorst(pNode)) / pNtk->AndGateDelay);
    // perform the traversal
    LevelsMax = 0;
    Abc_NtkIncrementTravId( pNtk );
    if ( pNtk->nBarBufs == 0 )
    {
        Abc_NtkForEachNode( pNtk, pNode, i )
        {
            Abc_NtkLevel_rec( pNode );
            if ( LevelsMax < (int)pNode->Level )
                LevelsMax = (int)pNode->Level;
        }
    }
    else
    {
        Abc_NtkForEachLiPo( pNtk, pNode, i )
        {
            Abc_Obj_t * pDriver = Abc_ObjFanin0(pNode);
            Abc_NtkLevel_rec( pDriver );
            if ( LevelsMax < (int)pDriver->Level )
                LevelsMax = (int)pDriver->Level;
            // transfer the delay
            if ( i < pNtk->nBarBufs )
                Abc_ObjFanout0(Abc_ObjFanout0(pNode))->Level = pDriver->Level;
        }
    }
    return LevelsMax;
}


int Abc_NtkLevel_rec( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNext;
    int i, Level;
    assert( !Abc_ObjIsNet(pNode) );
    // skip the PI
    if ( Abc_ObjIsCi(pNode) )
        return pNode->Level;
    assert( Abc_ObjIsNode( pNode ) || pNode->Type == ABC_OBJ_CONST1);
    // if this node is already visited, return
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return pNode->Level;
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pNode );
    // visit the transitive fanin
    pNode->Level = 0;
    Abc_ObjForEachFanin( pNode, pNext, i )
    {
        Level = Abc_NtkLevel_rec( Abc_ObjFanin0Ntk(pNext) );
        if ( pNode->Level < (unsigned)Level )
            pNode->Level = Level;
    }
    // if ( Abc_ObjFaninNum(pNode) > 0 && !Abc_ObjIsBarBuf(pNode) )
    //     pNode->Level++;
    // modified by Chang Meng, take standalone buffers into account
    if ( Abc_ObjFaninNum(pNode) > 0 )
        pNode->Level++;
    return pNode->Level;
}


int Abc_NtkCleanup( Abc_Ntk_t * pNtk, int fVerbose )
{
    Vec_Ptr_t * vNodes;
    int Counter;
    assert( Abc_NtkIsLogic(pNtk) );
    // mark the nodes reachable from the POs
    vNodes = Abc_NtkDfs( pNtk, 0 );
    Counter = Abc_NtkReduceNodes( pNtk, vNodes );
    if ( fVerbose )
        printf( "Cleanup removed %d dangling nodes.\n", Counter );
    Vec_PtrFree( vNodes );
    return Counter;
}


int Abc_NtkReduceNodes( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pNode;
    int i, Counter;
    assert( Abc_NtkIsLogic(pNtk) );
    // mark the nodes reachable from the POs
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
        pNode->fMarkA = 1;
    // remove the non-marked nodes
    Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( pNode->fMarkA == 0 )
        {
            Abc_NtkDeleteObj( pNode );
            Counter++;
        }
    // unmark the remaining nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
        pNode->fMarkA = 0;
    // check
    if ( !Abc_NtkCheck( pNtk ) )
        printf( "Abc_NtkCleanup: The network check has failed.\n" );
    return Counter;
}


void Abc_NtkStartReverseLevels( Abc_Ntk_t * pNtk, int nMaxLevelIncrease )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i;
    // remember the maximum number of direct levels
    pNtk->LevelMax = Abc_NtkLevel(pNtk) + nMaxLevelIncrease;
    // start the reverse levels
    pNtk->vLevelsR = Vec_IntAlloc( 0 );
    Vec_IntFill( pNtk->vLevelsR, 1 + Abc_NtkObjNumMax(pNtk), 0 );
    // compute levels in reverse topological order
    vNodes = Abc_NtkDfsReverse( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Abc_ObjSetReverseLevel( pObj, Abc_ObjReverseLevelNew(pObj) );
    Vec_PtrFree( vNodes );
}


Vec_Ptr_t * Abc_NtkDfsReverse( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pFanout;
    int i, k;
    // set the traversal ID
    Abc_NtkIncrementTravId( pNtk );
    // start the array of nodes
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        Abc_NodeSetTravIdCurrent( pObj );
        pObj = Abc_ObjFanout0Ntk(pObj);
        Abc_ObjForEachFanout( pObj, pFanout, k )
            Abc_NtkDfsReverse_rec( pFanout, vNodes );
    }
    // add constant nodes in the end
    if ( !Abc_NtkIsStrash(pNtk) ) {
        Abc_NtkForEachNode( pNtk, pObj, i )
            if ( Abc_NodeIsConst(pObj) )
                Vec_PtrPush( vNodes, pObj );
    }
    return vNodes;
}


void Abc_NtkDfsReverse_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanout;
    int i;
    assert( !Abc_ObjIsNet(pNode) );
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return;
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pNode );
    // skip the CI
    if ( Abc_ObjIsCo(pNode) )
        return;
    assert( Abc_ObjIsNode( pNode ) );
    // visit the transitive fanin of the node
    pNode = Abc_ObjFanout0Ntk(pNode);
    Abc_ObjForEachFanout( pNode, pFanout, i )
        Abc_NtkDfsReverse_rec( pFanout, vNodes );
    // add the node after the fanins have been added
    Vec_PtrPush( vNodes, pNode );
}


Vec_Int_t * Abc_NtkFanoutCounts( Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vFanNums;
    Abc_Obj_t * pObj;
    int i;
    vFanNums = Vec_IntAlloc( 0 );
    Vec_IntFill( vFanNums, Abc_NtkObjNumMax(pNtk), -1 );
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( Abc_ObjIsCi(pObj) || Abc_ObjIsNode(pObj) )
            Vec_IntWriteEntry( vFanNums, i, Abc_ObjFanoutNum(pObj) );
    return vFanNums;
}


void Abc_NtkReassignIds( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Vec_Ptr_t * vObjsNew;
    Abc_Obj_t * pNode, * pTemp, * pConst1;
    int i, k;
    assert( Abc_NtkIsStrash(pNtk) );
//printf( "Total = %d. Current = %d.\n", Abc_NtkObjNumMax(pNtk), Abc_NtkObjNum(pNtk) );
    // start the array of objects with new IDs
    vObjsNew = Vec_PtrAlloc( pNtk->nObjs );
    // put constant node first
    pConst1 = Abc_AigConst1(pNtk);
    assert( pConst1->Id == 0 );
    Vec_PtrPush( vObjsNew, pConst1 );
    // put PI nodes next
    Abc_NtkForEachPi( pNtk, pNode, i )
    {
        pNode->Id = Vec_PtrSize( vObjsNew );
        Vec_PtrPush( vObjsNew, pNode );
    }
    // put PO nodes next
    Abc_NtkForEachPo( pNtk, pNode, i )
    {
        pNode->Id = Vec_PtrSize( vObjsNew );
        Vec_PtrPush( vObjsNew, pNode );
    }
    // put latches and their inputs/outputs next
    Abc_NtkForEachBox( pNtk, pNode, i )
    {
        pNode->Id = Vec_PtrSize( vObjsNew );
        Vec_PtrPush( vObjsNew, pNode );
        Abc_ObjForEachFanin( pNode, pTemp, k )
        {
            pTemp->Id = Vec_PtrSize( vObjsNew );
            Vec_PtrPush( vObjsNew, pTemp );
        }
        Abc_ObjForEachFanout( pNode, pTemp, k )
        {
            pTemp->Id = Vec_PtrSize( vObjsNew );
            Vec_PtrPush( vObjsNew, pTemp );
        }
    }
    // finally, internal nodes in the DFS order
    vNodes = Abc_AigDfs( pNtk, 1, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( pNode == pConst1 )
            continue;
        pNode->Id = Vec_PtrSize( vObjsNew );
        Vec_PtrPush( vObjsNew, pNode );
    }
    Vec_PtrFree( vNodes );
    assert( Vec_PtrSize(vObjsNew) == pNtk->nObjs );

    // update the fanin/fanout arrays
    Abc_NtkForEachObj( pNtk, pNode, i )
    {
        Abc_ObjForEachFanin( pNode, pTemp, k )
            pNode->vFanins.pArray[k] = pTemp->Id;
        Abc_ObjForEachFanout( pNode, pTemp, k )
            pNode->vFanouts.pArray[k] = pTemp->Id;
    }

    // replace the array of objs
    Vec_PtrFree( pNtk->vObjs );
    pNtk->vObjs = vObjsNew;

    // rehash the AIG
    Abc_AigRehash( (Abc_Aig_t *)pNtk->pManFunc );

    // update the name manager!!!
}


void Abc_NtkStopReverseLevels( Abc_Ntk_t * pNtk )
{
    assert( pNtk->vLevelsR );
    Vec_IntFree( pNtk->vLevelsR );
    pNtk->vLevelsR = NULL;
    pNtk->LevelMax = 0;

}


void Abc_ObjAddFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pFaninR = Abc_ObjRegular(pFanin);
    assert( !Abc_ObjIsComplement(pObj) );
    assert( pObj->pNtk == pFaninR->pNtk );
    assert( pObj->Id >= 0 && pFaninR->Id >= 0 );
    assert( !Abc_ObjIsPi(pObj) && !Abc_ObjIsPo(pFaninR) );    // fanin of PI or fanout of PO
    assert( !Abc_ObjIsCo(pObj) || !Abc_ObjFaninNum(pObj) );  // CO with two fanins
    assert( !Abc_ObjIsNet(pObj) || !Abc_ObjFaninNum(pObj) ); // net with two fanins
    Vec_IntPushMem( pObj->pNtk->pMmStep, &pObj->vFanins,     pFaninR->Id );
    Vec_IntPushMem( pObj->pNtk->pMmStep, &pFaninR->vFanouts, pObj->Id    );
    if ( Abc_ObjIsComplement(pFanin) )
        Abc_ObjSetFaninC( pObj, Abc_ObjFaninNum(pObj)-1 );
}


void Abc_ObjRecycle( Abc_Obj_t * pObj )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
//    int LargePiece = (4 << ABC_NUM_STEPS);
    // free large fanout arrays
//    if ( pNtk->pMmStep && pObj->vFanouts.nCap * 4 > LargePiece )
//        free( pObj->vFanouts.pArray );
    if ( pNtk->pMmStep == NULL )
    {
        ABC_FREE( pObj->vFanouts.pArray );
        ABC_FREE( pObj->vFanins.pArray );
    }
    // clean the memory to make deleted object distinct from the live one
    memset( pObj, 0, sizeof(Abc_Obj_t) );
    // recycle the object
    if ( pNtk->pMmObj )
        Mem_FixedEntryRecycle( pNtk->pMmObj, (char *)pObj );
    else
        ABC_FREE( pObj );
}


Abc_Obj_t * Abc_ObjAlloc( Abc_Ntk_t * pNtk, Abc_ObjType_t Type )
{
    Abc_Obj_t * pObj;
    if ( pNtk->pMmObj )
        pObj = (Abc_Obj_t *)Mem_FixedEntryFetch( pNtk->pMmObj );
    else
        pObj = (Abc_Obj_t *)ABC_ALLOC( Abc_Obj_t, 1 );
    memset( pObj, 0, sizeof(Abc_Obj_t) );
    pObj->pNtk = pNtk;
    pObj->Type = Type;
    pObj->Id   = -1;
    return pObj;
}


char * Abc_ObjName( Abc_Obj_t * pObj )
{
    return Nm_ManCreateUniqueName( pObj->pNtk->pManName, pObj->Id );
}


void Abc_ObjDeleteFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFanin )
{
    assert( !Abc_ObjIsComplement(pObj) );
    assert( !Abc_ObjIsComplement(pFanin) );
    assert( pObj->pNtk == pFanin->pNtk );
    assert( pObj->Id >= 0 && pFanin->Id >= 0 );
    if ( !Vec_IntRemove( &pObj->vFanins, pFanin->Id ) )
    {
        printf( "The obj %d is not found among the fanins of obj %d ...\n", pFanin->Id, pObj->Id );
        return;
    }
    if ( !Vec_IntRemove( &pFanin->vFanouts, pObj->Id ) )
    {
        printf( "The obj %d is not found among the fanouts of obj %d ...\n", pObj->Id, pFanin->Id );
        return;
    }
}


char * Abc_ObjAssignName( Abc_Obj_t * pObj, char * pName, char * pSuffix )
{
    assert( pName != NULL );
    return Nm_ManStoreIdName( pObj->pNtk->pManName, pObj->Id, pObj->Type, pName, pSuffix );
}


int Abc_ObjReverseLevel( Abc_Obj_t * pObj )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
    assert( pNtk->vLevelsR );
    Vec_IntFillExtra( pNtk->vLevelsR, pObj->Id + 1, 0 );
    return Vec_IntEntry(pNtk->vLevelsR, pObj->Id);
}


void Abc_ObjPatchFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFaninOld, Abc_Obj_t * pFaninNew )
{
    Abc_Obj_t * pFaninNewR = Abc_ObjRegular(pFaninNew);
    int iFanin;//, nLats;//, fCompl;
    assert( !Abc_ObjIsComplement(pObj) );
    assert( !Abc_ObjIsComplement(pFaninOld) );
    assert( pFaninOld != pFaninNewR );
//    assert( pObj != pFaninOld );
//    assert( pObj != pFaninNewR );
    assert( pObj->pNtk == pFaninOld->pNtk );
    assert( pObj->pNtk == pFaninNewR->pNtk );
    if ( (iFanin = Vec_IntFind( &pObj->vFanins, pFaninOld->Id )) == -1 )
    {
        printf( "Node %s is not among", Abc_ObjName(pFaninOld) );
        printf( " the fanins of node %s...\n", Abc_ObjName(pObj) );
        return;
    }

    // remember the attributes of the old fanin
//    fCompl = Abc_ObjFaninC(pObj, iFanin);
    // replace the old fanin entry by the new fanin entry (removes attributes)
    Vec_IntWriteEntry( &pObj->vFanins, iFanin, pFaninNewR->Id );
    // set the attributes of the new fanin
//    if ( fCompl ^ Abc_ObjIsComplement(pFaninNew) )
//        Abc_ObjSetFaninC( pObj, iFanin );
    if ( Abc_ObjIsComplement(pFaninNew) )
        Abc_ObjXorFaninC( pObj, iFanin );

//    if ( Abc_NtkIsSeq(pObj->pNtk) && (nLats = Seq_ObjFaninL(pObj, iFanin)) )
//        Seq_ObjSetFaninL( pObj, iFanin, nLats );
    // update the fanout of the fanin
    if ( !Vec_IntRemove( &pFaninOld->vFanouts, pObj->Id ) )
    {
        printf( "Node %s is not among", Abc_ObjName(pObj) );
        printf( " the fanouts of its old fanin %s...\n", Abc_ObjName(pFaninOld) );
//        return;
    }
    Vec_IntPushMem( pObj->pNtk->pMmStep, &pFaninNewR->vFanouts, pObj->Id );
}


void Abc_ObjSetReverseLevel( Abc_Obj_t * pObj, int LevelR )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
    assert( pNtk->vLevelsR );
    Vec_IntFillExtra( pNtk->vLevelsR, pObj->Id + 1, 0 );
    Vec_IntWriteEntry( pNtk->vLevelsR, pObj->Id, LevelR );
}


int Abc_ObjReverseLevelNew( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int i, LevelCur, Level = 0;
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        LevelCur = Abc_ObjReverseLevel( pFanout );
        Level = Abc_MaxFloat( Level, LevelCur );
    }
    return Level + 1;
}


int Abc_ObjRequiredLevel( Abc_Obj_t * pObj )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
    assert( pNtk->vLevelsR );
    return pNtk->LevelMax + 1 - Abc_ObjReverseLevel(pObj);
}


void Abc_ObjRemoveFanins( Abc_Obj_t * pObj )
{
    Vec_Int_t * vFaninsOld;
    Abc_Obj_t * pFanin;
    int k;
    // remove old fanins
    vFaninsOld = &pObj->vFanins;
    for ( k = vFaninsOld->nSize - 1; k >= 0; k-- )
    {
        pFanin = Abc_NtkObj( pObj->pNtk, vFaninsOld->pArray[k] );
        Abc_ObjDeleteFanin( pObj, pFanin );
    }
    pObj->fCompl0 = 0;
    pObj->fCompl1 = 0;
    assert( vFaninsOld->nSize == 0 );
}


void Abc_NodeCollectFanouts( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanout;
    int i;
    Vec_PtrClear(vNodes);
    Abc_ObjForEachFanout( pNode, pFanout, i )
        Vec_PtrPush( vNodes, pFanout );
}


void Abc_NodeCollectFanins( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanin;
    int i;
    Vec_PtrClear(vNodes);
    Abc_ObjForEachFanin( pNode, pFanin, i )
        Vec_PtrPush( vNodes, pFanin );
}


int Abc_NodeIsConst( Abc_Obj_t * pNode )
{
    assert( Abc_NtkIsLogic(pNode->pNtk) || Abc_NtkIsNetlist(pNode->pNtk) );
    return Abc_ObjIsNode(pNode) && Abc_ObjFaninNum(pNode) == 0;
}


int Abc_NodeIsConst0( Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) );
    if ( !Abc_NodeIsConst(pNode) )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_SopIsConst0((char *)pNode->pData);
#ifdef ABC_USE_CUDD
    if ( Abc_NtkHasBdd(pNtk) )
        return Cudd_IsComplement(pNode->pData);
#endif
    if ( Abc_NtkHasAig(pNtk) )
        return Hop_IsComplement((Hop_Obj_t *)pNode->pData)? 1:0;
    if ( Abc_NtkHasMapping(pNtk) )
        // return pNode->pData == Mio_LibraryReadConst0((Mio_Library_t *)Abc_FrameReadLibGen());
        assert( 0 );
    assert( 0 );
    return 0;
}


int Abc_NodeIsConst1( Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) );
    if ( !Abc_NodeIsConst(pNode) )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_SopIsConst1((char *)pNode->pData);
#ifdef ABC_USE_CUDD
    if ( Abc_NtkHasBdd(pNtk) )
        return !Cudd_IsComplement(pNode->pData);
#endif
    if ( Abc_NtkHasAig(pNtk) )
        return !Hop_IsComplement((Hop_Obj_t *)pNode->pData);
    if ( Abc_NtkHasMapping(pNtk) )
        // return pNode->pData == Mio_LibraryReadConst1((Mio_Library_t *)Abc_FrameReadLibGen());
        assert( 0 );
    assert( 0 );
    return 0;
}


int Abc_NodeIsExorType( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNode0, * pNode1;
    // check that the node is regular
    assert( !Abc_ObjIsComplement(pNode) );
    // if the node is not AND, this is not EXOR
    if ( !Abc_AigNodeIsAnd(pNode) )
        return 0;
    // if the children are not complemented, this is not EXOR
    if ( !Abc_ObjFaninC0(pNode) || !Abc_ObjFaninC1(pNode) )
        return 0;
    // get children
    pNode0 = Abc_ObjFanin0(pNode);
    pNode1 = Abc_ObjFanin1(pNode);
    // if the children are not ANDs, this is not EXOR
    if ( Abc_ObjFaninNum(pNode0) != 2 || Abc_ObjFaninNum(pNode1) != 2 )
        return 0;
    // this is AIG, which means the fanins should be ordered
    assert( Abc_ObjFaninId0(pNode0) != Abc_ObjFaninId1(pNode1) ||
            Abc_ObjFaninId0(pNode1) != Abc_ObjFaninId1(pNode0) );
    // if grand children are not the same, this is not EXOR
    if ( Abc_ObjFaninId0(pNode0) != Abc_ObjFaninId0(pNode1) ||
         Abc_ObjFaninId1(pNode0) != Abc_ObjFaninId1(pNode1) )
         return 0;
    // finally, if the complemented edges are matched, this is not EXOR
    if ( Abc_ObjFaninC0(pNode0) == Abc_ObjFaninC0(pNode1) ||
         Abc_ObjFaninC1(pNode0) == Abc_ObjFaninC1(pNode1) )
         return 0;
    return 1;
}


int Abc_NodeIsMuxControlType( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNode0, * pNode1;
    // check that the node is regular
    assert( !Abc_ObjIsComplement(pNode) );
    // skip the node that do not have two fanouts
    if ( Abc_ObjFanoutNum(pNode) != 2 )
        return 0;
    // get the fanouts
    pNode0 = Abc_ObjFanout( pNode, 0 );
    pNode1 = Abc_ObjFanout( pNode, 1 );
    // if they have more than one fanout, we are not interested
    if ( Abc_ObjFanoutNum(pNode0) != 1 ||  Abc_ObjFanoutNum(pNode1) != 1 )
        return 0;
    // if the fanouts have the same fanout, this is MUX or EXOR (or a redundant gate (CA)(CB))
    return Abc_ObjFanout0(pNode0) == Abc_ObjFanout0(pNode1);
}


void Abc_NodeComplement( Abc_Obj_t * pNode )
{
    assert( Abc_NtkIsLogic(pNode->pNtk) || Abc_NtkIsNetlist(pNode->pNtk) );
    assert( Abc_ObjIsNode(pNode) );
    if ( Abc_NtkHasSop(pNode->pNtk) )
        Abc_SopComplement( (char *)pNode->pData );
    else if ( Abc_NtkHasAig(pNode->pNtk) )
        pNode->pData = Hop_Not( (Hop_Obj_t *)pNode->pData );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNode->pNtk) )
        pNode->pData = Cudd_Not( pNode->pData );
#endif
    else
        assert( 0 );
}


void Abc_NodeComplementInput( Abc_Obj_t * pNode, Abc_Obj_t * pFanin )
{
    int iFanin;
    if ( (iFanin = Vec_IntFind( &pNode->vFanins, pFanin->Id )) == -1 )
    {
        printf( "Node %s should be among", Abc_ObjName(pFanin) );
        printf( " the fanins of node %s...\n", Abc_ObjName(pNode) );
        return;
    }
    if ( Abc_NtkHasSop(pNode->pNtk) )
        Abc_SopComplementVar( (char *)pNode->pData, iFanin );
    else if ( Abc_NtkHasAig(pNode->pNtk) )
        pNode->pData = Hop_Complement( (Hop_Man_t *)pNode->pNtk->pManFunc, (Hop_Obj_t *)pNode->pData, iFanin );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNode->pNtk) )
    {
        DdManager * dd = (DdManager *)pNode->pNtk->pManFunc;
        DdNode * bVar, * bCof0, * bCof1;
        bVar = Cudd_bddIthVar( dd, iFanin );
        bCof0 = Cudd_Cofactor( dd, (DdNode *)pNode->pData, Cudd_Not(bVar) );   Cudd_Ref( bCof0 );
        bCof1 = Cudd_Cofactor( dd, (DdNode *)pNode->pData, bVar );             Cudd_Ref( bCof1 );
        Cudd_RecursiveDeref( dd, (DdNode *)pNode->pData );
        pNode->pData = Cudd_bddIte( dd, bVar, bCof0, bCof1 );        Cudd_Ref( (DdNode *)pNode->pData );
        Cudd_RecursiveDeref( dd, bCof0 );
        Cudd_RecursiveDeref( dd, bCof1 );
    }
#endif
    else
        assert( 0 );
}


Abc_Obj_t * Abc_NodeStrash( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNodeOld, int fRecord )
{
    Hop_Man_t * pMan;
    Hop_Obj_t * pRoot;
    Abc_Obj_t * pFanin;
    int i;
    assert( Abc_ObjIsNode(pNodeOld) );
    assert( Abc_NtkHasAig(pNodeOld->pNtk) && !Abc_NtkIsStrash(pNodeOld->pNtk) );
    // get the local AIG manager and the local root node
    pMan = (Hop_Man_t *)pNodeOld->pNtk->pManFunc;
    pRoot = (Hop_Obj_t *)pNodeOld->pData;
    // check the constant case
    if ( Abc_NodeIsConst(pNodeOld) || Hop_Regular(pRoot) == Hop_ManConst1(pMan) )
        return Abc_ObjNotCond( Abc_AigConst1(pNtkNew), Hop_IsComplement(pRoot) );
    // set elementary variables
    Abc_ObjForEachFanin( pNodeOld, pFanin, i )
        Hop_IthVar(pMan, i)->pData = pFanin->pCopy;
    // strash the AIG of this node
    Abc_NodeStrash_rec( (Abc_Aig_t *)pNtkNew->pManFunc, Hop_Regular(pRoot) );
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
    // return the final node
    return Abc_ObjNotCond( (Abc_Obj_t *)Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );
}


void Abc_NodeStrash_rec( Abc_Aig_t * pMan, Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Abc_NodeStrash_rec( pMan, Hop_ObjFanin0(pObj) );
    Abc_NodeStrash_rec( pMan, Hop_ObjFanin1(pObj) );
    pObj->pData = Abc_AigAnd( pMan, (Abc_Obj_t *)Hop_ObjChild0Copy(pObj), (Abc_Obj_t *)Hop_ObjChild1Copy(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}


int Abc_NodeMffcLabelAig( Abc_Obj_t * pNode )
{
    int nConeSize1, nConeSize2;
    assert( Abc_NtkIsStrash(pNode->pNtk) );
    assert( !Abc_ObjIsComplement( pNode ) );
    assert( Abc_ObjIsNode( pNode ) );
    if ( Abc_ObjFaninNum(pNode) == 0 )
        return 0;
    nConeSize1 = Abc_NodeRefDeref( pNode, 0, 1 ); // dereference
    nConeSize2 = Abc_NodeRefDeref( pNode, 1, 0 ); // reference
    assert( nConeSize1 == nConeSize2 );
    assert( nConeSize1 > 0 );
    return nConeSize1;
}


int Abc_NodeRefDeref( Abc_Obj_t * pNode, int fReference, int fLabel )
{
    Abc_Obj_t * pNode0, * pNode1;
    int Counter;
    // label visited nodes
    if ( fLabel )
        Abc_NodeSetTravIdCurrent( pNode );
    // skip the CI
    if ( Abc_ObjIsCi(pNode) )
        return 0;
    // process the internal node
    pNode0 = Abc_ObjFanin0(pNode);
    pNode1 = Abc_ObjFanin1(pNode);
    Counter = 1;
    if ( fReference )
    {
        if ( pNode0->vFanouts.nSize++ == 0 )
            Counter += Abc_NodeRefDeref( pNode0, fReference, fLabel );
        if ( pNode1->vFanouts.nSize++ == 0 )
            Counter += Abc_NodeRefDeref( pNode1, fReference, fLabel );
    }
    else
    {
        assert( pNode0->vFanouts.nSize > 0 );
        assert( pNode1->vFanouts.nSize > 0 );
        if ( --pNode0->vFanouts.nSize == 0 )
            Counter += Abc_NodeRefDeref( pNode0, fReference, fLabel );
        if ( --pNode1->vFanouts.nSize == 0 )
            Counter += Abc_NodeRefDeref( pNode1, fReference, fLabel );
    }
    return Counter;
}
