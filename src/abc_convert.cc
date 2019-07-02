#include "abc_convert.h"


Abc_Ntk_t * Abc_NtkStrash( Abc_Ntk_t * pNtk, int fAllNodes, int fCleanup, int fRecord )
{
    Abc_Ntk_t * pNtkAig;
    // int nNodes;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );
    // consider the special case when the network is already structurally hashed
    if ( Abc_NtkIsStrash(pNtk) )
        return Abc_NtkRestrash( pNtk, fCleanup );
    // convert the node representation in the logic network to the AIG form
    if ( !Abc_NtkToAig(pNtk) )
    {
        printf( "Converting to AIGs has failed.\n" );
        return NULL;
    }
    // perform strashing
//    Abc_NtkCleanCopy( pNtk );
    pNtkAig = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    Abc_NtkStrashPerform( pNtk, pNtkAig, fAllNodes, fRecord );
    Abc_NtkFinalize( pNtk, pNtkAig );
    // transfer name IDs
    if ( pNtk->vNameIds )
        Abc_NtkTransferNameIds( pNtk, pNtkAig );
    // print warning about self-feed latches
//    if ( Abc_NtkCountSelfFeedLatches(pNtkAig) )
//        printf( "Warning: The network has %d self-feeding latches.\n", Abc_NtkCountSelfFeedLatches(pNtkAig) );
    // perform cleanup if requested
    // nNodes = fCleanup? Abc_AigCleanup((Abc_Aig_t *)pNtkAig->pManFunc) : 0;
    if ( fCleanup )
        Abc_AigCleanup((Abc_Aig_t *)pNtkAig->pManFunc);
//    if ( nNodes )
//        printf( "Warning: AIG cleanup removed %d nodes (this is not a bug).\n", nNodes );
    // duplicate EXDC
    if ( pNtk->pExdc )
        pNtkAig->pExdc = Abc_NtkStrash( pNtk->pExdc, fAllNodes, fCleanup, fRecord );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkAig ) )
    {
        printf( "Abc_NtkStrash: The network check has failed.\n" );
        Abc_NtkDelete( pNtkAig );
        return NULL;
    }
    return pNtkAig;
}


Abc_Ntk_t * Abc_NtkRestrash( Abc_Ntk_t * pNtk, int fCleanup )
{
//    extern int timeRetime;
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkAig;
    Abc_Obj_t * pObj;
    int i;//, nNodes;//, RetValue;
    assert( Abc_NtkIsStrash(pNtk) );
//timeRetime = Abc_Clock();
    // print warning about choice nodes
    if ( Abc_NtkGetChoiceNum( pNtk ) )
        printf( "Warning: The choice nodes in the original AIG are removed by strashing.\n" );
    // start the new network (constants and CIs of the old network will point to the their counterparts in the new network)
    pNtkAig = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // restrash the nodes (assuming a topological order of the old network)
    vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkAig->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    Vec_PtrFree( vNodes );
    // finalize the network
    Abc_NtkFinalize( pNtk, pNtkAig );
    // print warning about self-feed latches
//    if ( Abc_NtkCountSelfFeedLatches(pNtkAig) )
//        printf( "Warning: The network has %d self-feeding latches.\n", Abc_NtkCountSelfFeedLatches(pNtkAig) );
    // perform cleanup if requested
//     if ( fCleanup && (nNodes = Abc_AigCleanup((Abc_Aig_t *)pNtkAig->pManFunc)) )
//     {
// //        printf( "Abc_NtkRestrash(): AIG cleanup removed %d nodes (this is a bug).\n", nNodes );
//     }
    // duplicate EXDC
    if ( pNtk->pExdc )
        pNtkAig->pExdc = Abc_NtkDup( pNtk->pExdc );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkAig ) )
    {
        printf( "Abc_NtkStrash: The network check has failed.\n" );
        Abc_NtkDelete( pNtkAig );
        return NULL;
    }
//timeRetime = Abc_Clock() - timeRetime;
//    if ( RetValue = Abc_NtkRemoveSelfFeedLatches(pNtkAig) )
//        printf( "Modified %d self-feeding latches. The result may not verify.\n", RetValue );
    return pNtkAig;

}


int Abc_NtkGetChoiceNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, Counter;
    if ( !Abc_NtkIsStrash(pNtk) )
        return 0;
    Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        Counter += Abc_AigNodeIsChoice( pNode );
    return Counter;
}


Abc_Ntk_t * Abc_NtkStartFrom( Abc_Ntk_t * pNtk, Abc_NtkType_t Type, Abc_NtkFunc_t Func )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj;
    int fCopyNames, i;
    if ( pNtk == NULL )
        return NULL;
    // decide whether to copy the names
    fCopyNames = ( Type != ABC_NTK_NETLIST );
    // start the network
    pNtkNew = Abc_NtkAlloc( Type, Func, 1 );
    pNtkNew->nConstrs   = pNtk->nConstrs;
    pNtkNew->nBarBufs   = pNtk->nBarBufs;
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );
    // map the constant nodes
    if ( Abc_NtkIsStrash(pNtk) && Abc_NtkIsStrash(pNtkNew) )
        Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    // clone CIs/CIs/boxes
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, fCopyNames );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, fCopyNames );
    Abc_NtkForEachBox( pNtk, pObj, i )
        Abc_NtkDupBox( pNtkNew, pObj, fCopyNames );
    // transfer logic level
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pCopy->Level = pObj->Level;
    // transfer the names
//    Abc_NtkTrasferNames( pNtk, pNtkNew );
//    Abc_ManTimeDup( pNtk, pNtkNew );
    if ( pNtk->vOnehots )
        pNtkNew->vOnehots = (Vec_Ptr_t *)Vec_VecDupInt( (Vec_Vec_t *)pNtk->vOnehots );
//     if ( pNtk->pSeqModel )
//         pNtkNew->pSeqModel = Abc_CexDup( pNtk->pSeqModel, Abc_NtkLatchNum(pNtk) );
    if ( pNtk->vObjPerm )
        pNtkNew->vObjPerm = Vec_IntDup( pNtk->vObjPerm );
    // pNtkNew->AndGateDelay = pNtk->AndGateDelay;
    // assert( pNtkNew->pManTime == NULL );
//    if ( pNtkNew->pManTime && Abc_FrameReadLibGen() && pNtkNew->AndGateDelay == 0.0 )
//        pNtkNew->AndGateDelay = Mio_LibraryReadDelayAigNode((Mio_Library_t *)Abc_FrameReadLibGen());
//     // initialize logic level of the CIs
//     if ( pNtk->AndGateDelay != 0.0 && pNtk->pManTime != NULL && pNtk->ntkType != ABC_NTK_STRASH && Type == ABC_NTK_STRASH )
//     {
//         Abc_NtkForEachCi( pNtk, pObj, i )
//             pObj->pCopy->Level = (int)(Abc_MaxFloat(0, Abc_NodeReadArrivalWorst(pObj)) / pNtk->AndGateDelay);
//     }
    // check that the CI/CO/latches are copied correctly
    assert( Abc_NtkCiNum(pNtk)    == Abc_NtkCiNum(pNtkNew) );
    assert( Abc_NtkCoNum(pNtk)    == Abc_NtkCoNum(pNtkNew) );
    assert( Abc_NtkLatchNum(pNtk) == Abc_NtkLatchNum(pNtkNew) );
    return pNtkNew;
}


Abc_Ntk_t * Abc_NtkDup( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    if ( pNtk == NULL )
        return NULL;
    // start the network
    pNtkNew = Abc_NtkStartFrom( pNtk, pNtk->ntkType, pNtk->ntkFunc );
    // copy the internal nodes
    if ( Abc_NtkIsStrash(pNtk) )
    {
        // copy the AND gates
        Abc_AigForEachAnd( pNtk, pObj, i )
            pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
        // relink the choice nodes
        Abc_AigForEachAnd( pNtk, pObj, i )
            if ( pObj->pData )
                pObj->pCopy->pData = ((Abc_Obj_t *)pObj->pData)->pCopy;
        // relink the CO nodes
        Abc_NtkForEachCo( pNtk, pObj, i )
            Abc_ObjAddFanin( pObj->pCopy, Abc_ObjChild0Copy(pObj) );
        // get the number of nodes before and after
        if ( Abc_NtkNodeNum(pNtk) != Abc_NtkNodeNum(pNtkNew) )
            printf( "Warning: Structural hashing during duplication reduced %d nodes (this is a minor bug).\n",
                Abc_NtkNodeNum(pNtk) - Abc_NtkNodeNum(pNtkNew) );
    }
    else
    {
        // duplicate the nets and nodes (CIs/COs/latches already dupped)
        Abc_NtkForEachObj( pNtk, pObj, i )
            if ( pObj->pCopy == NULL )
                Abc_NtkDupObj(pNtkNew, pObj, Abc_NtkHasBlackbox(pNtk) && Abc_ObjIsNet(pObj));
        // reconnect all objects (no need to transfer attributes on edges)
        Abc_NtkForEachObj( pNtk, pObj, i )
            if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
                Abc_ObjForEachFanin( pObj, pFanin, k )
                    Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
    }
    // duplicate the EXDC Ntk
    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    if ( pNtk->pExcare )
        pNtkNew->pExcare = Abc_NtkDup( (Abc_Ntk_t *)pNtk->pExcare );
    // duplicate timing manager
    // if ( pNtk->pManTime )
        // Abc_NtkTimeInitialize( pNtkNew, pNtk );
    if ( pNtk->vPhases )
        Abc_NtkTransferPhases( pNtkNew, pNtk );
    if ( pNtk->pWLoadUsed )
        pNtkNew->pWLoadUsed = Abc_UtilStrsav( pNtk->pWLoadUsed );
    // check correctness
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkDup(): Network check has failed.\n" );
    pNtk->pCopy = pNtkNew;
    return pNtkNew;
}


int Abc_NtkToAig( Abc_Ntk_t * pNtk )
{
    assert( !Abc_NtkIsStrash(pNtk) );
    if ( Abc_NtkHasBlackbox(pNtk) )
        return 1;
    if ( Abc_NtkHasAig(pNtk) )
        return 1;
    if ( Abc_NtkHasMapping(pNtk) )
    {
        // Abc_NtkMapToSop(pNtk);
        // return Abc_NtkSopToAig(pNtk);
        assert( 0 );
        return 0;
    }
    if ( Abc_NtkHasBdd(pNtk) )
    {
        // if ( !Abc_NtkBddToSop(pNtk, -1, ABC_INFINITY) )
        //     return 0;
        // return Abc_NtkSopToAig(pNtk);
        assert( 0 );
        return 0;
    }
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_NtkSopToAig(pNtk);
    assert( 0 );
    return 0;
}


int Abc_NtkSopToAig( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    Hop_Man_t * pMan;
    int i, Max;

    assert( Abc_NtkHasSop(pNtk) );

    // make dist1-free and SCC-free
//    Abc_NtkMakeLegit( pNtk );

    // start the functionality manager
    pMan = Hop_ManStart();
    Max = Abc_NtkGetFaninMax(pNtk);
    if ( Max ) Hop_IthVar( pMan, Max-1 );

    // convert each node from SOP to BDD
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( Abc_ObjIsBarBuf(pNode) )
            continue;
        assert( pNode->pData );
        pNode->pData = Abc_ConvertSopToAig( pMan, (char *)pNode->pData );
        if ( pNode->pData == NULL )
        {
            Hop_ManStop( pMan );
            printf( "Abc_NtkSopToAig: Error while converting SOP into AIG.\n" );
            return 0;
        }
    }
    Mem_FlexStop( (Mem_Flex_t *)pNtk->pManFunc, 0 );
    pNtk->pManFunc = pMan;

    // update the network type
    pNtk->ntkFunc = ABC_FUNC_AIG;
    return 1;
}


void Abc_NtkStrashPerform( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkNew, int fAllNodes, int fRecord )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNodeOld;
    int i; //, clk = Abc_Clock();
    assert( Abc_NtkIsLogic(pNtkOld) );
    assert( Abc_NtkIsStrash(pNtkNew) );
//    vNodes = Abc_NtkDfs( pNtkOld, fAllNodes );
    vNodes = Abc_NtkDfsIter( pNtkOld, fAllNodes );
//printf( "Nodes = %d. ", Vec_PtrSize(vNodes) );
//ABC_PRT( "Time", Abc_Clock() - clk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNodeOld, i )
    {
        if ( Abc_ObjIsBarBuf(pNodeOld) )
            pNodeOld->pCopy = Abc_ObjChild0Copy(pNodeOld);
        else
            pNodeOld->pCopy = Abc_NodeStrash( pNtkNew, pNodeOld, fRecord );
    }
    Vec_PtrFree( vNodes );
}


void Abc_NtkTransferNameIds( Abc_Ntk_t * p, Abc_Ntk_t * pNew )
{
    Abc_Obj_t * pObj, * pObjNew;
    int i;
    assert( p->vNameIds != NULL );
    assert( pNew->vNameIds == NULL );
    pNew->vNameIds = Vec_IntStart( Abc_NtkObjNumMax(pNew) );
//    Abc_NtkForEachCi( p, pObj, i )
//        printf( "%d ", Vec_IntEntry(p->vNameIds, Abc_ObjId(pObj)) );
//    printf( "\n" );
    Abc_NtkForEachObj( p, pObj, i )
        if ( pObj->pCopy && i < Vec_IntSize(p->vNameIds) && Vec_IntEntry(p->vNameIds, i) )
        {
            pObjNew = Abc_ObjRegular(pObj->pCopy);
            assert( Abc_ObjNtk(pObjNew) == pNew );
            if ( Abc_ObjIsCi(pObjNew) && !Abc_ObjIsCi(pObj) ) // do not overwrite CI name by internal node name
                continue;
            Vec_IntWriteEntry( pNew->vNameIds, Abc_ObjId(pObjNew), Vec_IntEntry(p->vNameIds, i) ^ Abc_ObjIsComplement(pObj->pCopy) );
        }
}


Abc_Ntk_t * Abc_NtkToLogic( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    // consider the case of the AIG
    if ( Abc_NtkIsStrash(pNtk) )
        return Abc_NtkAigToLogicSop( pNtk );
    assert( Abc_NtkIsNetlist(pNtk) );
    // consider simple case when there is hierarchy
//    assert( pNtk->pDesign == NULL );
    assert( Abc_NtkWhiteboxNum(pNtk) == 0 );
    assert( Abc_NtkBlackboxNum(pNtk) == 0 );
    // start the network
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, pNtk->ntkFunc );
    // duplicate the nodes
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        Abc_NtkDupObj(pNtkNew, pObj, 0);
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(Abc_ObjFanout0(pObj)), NULL );
    }
    // reconnect the internal nodes in the new network
    Abc_NtkForEachNode( pNtk, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Abc_ObjAddFanin( pObj->pCopy, Abc_ObjFanin0(pFanin)->pCopy );
    // collect the CO nodes
    Abc_NtkFinalize( pNtk, pNtkNew );
    // fix the problem with CO pointing directly to CIs
    Abc_NtkLogicMakeSimpleCos( pNtkNew, 0 );
    // duplicate EXDC
    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkToLogic( pNtk->pExdc );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkToLogic(): Network check has failed.\n" );
    return pNtkNew;
}


Abc_Ntk_t * Abc_NtkAigToLogicSop( Abc_Ntk_t * pNtk )
{
    // extern int Abc_NtkLogicMakeSimpleCos2( Abc_Ntk_t * pNtk, int fDuplicate );

    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pFanin, * pNodeNew;
    Vec_Int_t * vInts;
    int i, k, fChoices = 0;
    assert( Abc_NtkIsStrash(pNtk) );
    // start the network
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    // if the constant node is used, duplicate it
    pObj = Abc_AigConst1(pNtk);
    if ( Abc_ObjFanoutNum(pObj) > 0 )
        pObj->pCopy = Abc_NtkCreateNodeConst1(pNtkNew);
    // duplicate the nodes and create node functions
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        Abc_NtkDupObj(pNtkNew, pObj, 0);
        pObj->pCopy->pData = Abc_SopCreateAnd2( (Mem_Flex_t *)pNtkNew->pManFunc, Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj) );
    }
    // create the choice nodes
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( !Abc_AigNodeIsChoice(pObj) )
            continue;
        // create an OR gate
        pNodeNew = Abc_NtkCreateNode(pNtkNew);
        // add fanins
        vInts = Vec_IntAlloc( 10 );
        for ( pFanin = pObj; pFanin; pFanin = (Abc_Obj_t *)pFanin->pData )
        {
            Vec_IntPush( vInts, (int)(pObj->fPhase != pFanin->fPhase) );
            Abc_ObjAddFanin( pNodeNew, pFanin->pCopy );
        }
        // create the logic function
        pNodeNew->pData = Abc_SopCreateOrMultiCube( (Mem_Flex_t *)pNtkNew->pManFunc, Vec_IntSize(vInts), Vec_IntArray(vInts) );
        // set the new node
        pObj->pCopy->pCopy = pNodeNew;
        Vec_IntFree( vInts );
        fChoices = 1;
    }
    // connect the internal nodes
    Abc_NtkForEachNode( pNtk, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
            if ( pFanin->pCopy->pCopy )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy->pCopy );
            else
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
    // connect the COs
//    Abc_NtkFinalize( pNtk, pNtkNew );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pFanin = Abc_ObjFanin0(pObj);
        if ( pFanin->pCopy->pCopy )
            pNodeNew = Abc_ObjNotCond(pFanin->pCopy->pCopy, Abc_ObjFaninC0(pObj));
        else
            pNodeNew = Abc_ObjNotCond(pFanin->pCopy, Abc_ObjFaninC0(pObj));
        Abc_ObjAddFanin( pObj->pCopy, pNodeNew );
    }

    // fix the problem with complemented and duplicated CO edges
    if ( fChoices )
        Abc_NtkLogicMakeSimpleCos2( pNtkNew, 0 );
    else
        Abc_NtkLogicMakeSimpleCos( pNtkNew, 0 );
    // duplicate the EXDC Ntk
    if ( pNtk->pExdc )
    {
        if ( Abc_NtkIsStrash(pNtk->pExdc) )
            pNtkNew->pExdc = Abc_NtkAigToLogicSop( pNtk->pExdc );
        else
            pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    }
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkAigToLogicSop(): Network check has failed.\n" );
    return pNtkNew;
}


int Abc_NtkLogicMakeSimpleCos( Abc_Ntk_t * pNtk, int fDuplicate )
{
    int fAddBuffers = 1;
    Vec_Ptr_t * vDrivers, * vCoTerms;
    Abc_Obj_t * pNode, * pDriver, * pDriverNew, * pFanin;
    int i, k, LevelMax, nTotal = 0;
    assert( Abc_NtkIsLogic(pNtk) );
    LevelMax = Abc_NtkLevel(pNtk);
//    Abc_NtkLogicMakeSimpleCosTest( pNtk, fDuplicate );

    // fix constant drivers
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pDriver = Abc_ObjFanin0(pNode);
        if ( !Abc_NodeIsConst(pDriver) )
            continue;
        pDriverNew = (Abc_ObjFaninC0(pNode) == Abc_NodeIsConst0(pDriver)) ? Abc_NtkCreateNodeConst1(pNtk) : Abc_NtkCreateNodeConst0(pNtk);
        if ( Abc_ObjFaninC0(pNode) )
            Abc_ObjXorFaninC( pNode, 0 );
        Abc_ObjPatchFanin( pNode, pDriver, pDriverNew );
        if ( Abc_ObjFanoutNum(pDriver) == 0 )
            Abc_NtkDeleteObj( pDriver );
    }

    // collect drivers pointed by complemented edges
    vDrivers = Vec_PtrAlloc( 100 );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        if ( !Abc_ObjFaninC0(pNode) )
            continue;
        pDriver = Abc_ObjFanin0(pNode);
        if ( Abc_NodeIsTravIdCurrent(pDriver) )
            continue;
        Abc_NodeSetTravIdCurrent(pDriver);
        Vec_PtrPush( vDrivers, pDriver );
    }
    // fix complemented drivers
    if ( Vec_PtrSize(vDrivers) > 0 )
    {
        int nDupGates = 0, nDupInvs = 0, nDupChange = 0;
        Vec_Ptr_t * vFanouts = Vec_PtrAlloc( 100 );
        Vec_PtrForEachEntry( Abc_Obj_t *, vDrivers, pDriver, i )
        {
            int fHasDir = 0, fHasInv = 0, fHasOther = 0;
            Abc_ObjForEachFanout( pDriver, pNode, k )
            {
                if ( !Abc_ObjIsCo(pNode) )
                {
                    assert( !Abc_ObjFaninC0(pNode) );
                    fHasOther = 1;
                    continue;
                }
                if ( Abc_ObjFaninC0(pNode) )
                    fHasInv = 1;
                else //if ( Abc_ObjFaninC0(pNode) )
                    fHasDir = 1;
            }
            assert( fHasInv );
            if ( Abc_ObjIsCi(pDriver) || fHasDir || (fHasOther && Abc_NtkHasMapping(pNtk)) ) // cannot change
            {
                // duplicate if critical
                if ( fDuplicate && Abc_ObjIsNode(pDriver) && Abc_ObjLevel(pDriver) == LevelMax )
                {
                    pDriverNew = Abc_NtkDupObj( pNtk, pDriver, 0 );
                    Abc_ObjForEachFanin( pDriver, pFanin, k )
                        Abc_ObjAddFanin( pDriverNew, pFanin );
                    Abc_NodeComplement( pDriverNew );
                    nDupGates++;
                }
                else // add inverter
                {
                    pDriverNew = Abc_NtkCreateNodeInv( pNtk, pDriver );
                    nDupInvs++;
                }
                // collect CO fanouts to be redirected to the new node
                Vec_PtrClear( vFanouts );
                Abc_ObjForEachFanout( pDriver, pNode, k )
                    if ( Abc_ObjIsCo(pNode) && Abc_ObjFaninC0(pNode) )
                        Vec_PtrPush( vFanouts, pNode );
                assert( Vec_PtrSize(vFanouts) > 0 );
                Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pNode, k )
                {
                    Abc_ObjXorFaninC( pNode, 0 );
                    Abc_ObjPatchFanin( pNode, pDriver, pDriverNew );
                    assert( Abc_ObjIsCi(pDriver) || Abc_ObjFanoutNum(pDriver) > 0 );
                }
            }
            else // can change
            {
                // change polarity of the driver
                assert( Abc_ObjIsNode(pDriver) );
                Abc_NodeComplement( pDriver );
                Abc_ObjForEachFanout( pDriver, pNode, k )
                {
                    if ( Abc_ObjIsCo(pNode) )
                    {
                        assert( Abc_ObjFaninC0(pNode) );
                        Abc_ObjXorFaninC( pNode, 0 );
                    }
                    else if ( Abc_ObjIsNode(pNode) )
                        Abc_NodeComplementInput( pNode, pDriver );
                    else assert( 0 );
                }
                nDupChange++;
            }
        }
        Vec_PtrFree( vFanouts );
//        printf( "Resolving inverted CO drivers: Invs = %d. Dups = %d. Changes = %d.\n",
//            nDupInvs, nDupGates, nDupChange );
        nTotal += nDupInvs + nDupGates;
    }
    Vec_PtrFree( vDrivers );

    // collect COs that needs fixing by adding buffers or duplicating
    vCoTerms = Vec_PtrAlloc( 100 );
    Abc_NtkIncrementTravId( pNtk );

    // The following cases should be addressed only if the network is written
    // into a BLIF file. Otherwise, it is possible to skip them:
    // (1) if a CO points to a CI with a different name
    // (2) if an internal node drives more than one CO
    if ( fAddBuffers )
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        // if the driver is a CI and has different name, this is an error
        pDriver = Abc_ObjFanin0(pNode);
        if ( Abc_ObjIsCi(pDriver) && strcmp(Abc_ObjName(pDriver), Abc_ObjName(pNode)) )
        {
            Vec_PtrPush( vCoTerms, pNode );
            continue;
        }
        // if the driver is visited for the first time, remember the CO name
        if ( !Abc_NodeIsTravIdCurrent(pDriver) )
        {
            pDriver->pNext = (Abc_Obj_t *)Abc_ObjName(pNode);
            Abc_NodeSetTravIdCurrent(pDriver);
            continue;
        }
        // the driver has second CO - if they have different name, this is an error
        if ( strcmp((char *)pDriver->pNext, Abc_ObjName(pNode)) ) // diff names
        {
            Vec_PtrPush( vCoTerms, pNode );
            continue;
        }
    }
    // fix duplication problem
    if ( Vec_PtrSize(vCoTerms) > 0 )
    {
        int nDupBufs = 0, nDupGates = 0;
        Vec_PtrForEachEntry( Abc_Obj_t *, vCoTerms, pNode, i )
        {
            pDriver = Abc_ObjFanin0(pNode);
            // duplicate if critical
            if ( fDuplicate && Abc_ObjIsNode(pDriver) && (Abc_NtkHasMapping(pNtk) || Abc_ObjLevel(pDriver) == LevelMax) )
            {
                pDriverNew = Abc_NtkDupObj( pNtk, pDriver, 0 );
                Abc_ObjForEachFanin( pDriver, pFanin, k )
                    Abc_ObjAddFanin( pDriverNew, pFanin );
                nDupGates++;
            }
            else // add buffer
            {
                pDriverNew = Abc_NtkCreateNodeBuf( pNtk, pDriver );
                Abc_ObjAssignName( pDriverNew, Abc_ObjName(pDriver), const_cast < char * >( "_buf" ) );
                nDupBufs++;
            }
            // swing the PO
            Abc_ObjPatchFanin( pNode, pDriver, pDriverNew );
            assert( Abc_ObjIsCi(pDriver) || Abc_ObjFanoutNum(pDriver) > 0 );
        }
//        printf( "Resolving shared CO drivers: Bufs = %d. Dups = %d.\n", nDupBufs, nDupGates );
        nTotal += nDupBufs + nDupGates;
    }
    Vec_PtrFree( vCoTerms );
    return nTotal;
}


int Abc_NtkLogicMakeSimpleCos2( Abc_Ntk_t * pNtk, int fDuplicate )
{
    Abc_Obj_t * pNode, * pDriver;
    int i, nDupGates = 0;
    assert( Abc_NtkIsLogic(pNtk) );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        // if the driver is complemented, this is an error
        pDriver = Abc_ObjFanin0(pNode);
        if ( Abc_ObjFaninC0(pNode) )
        {
            Abc_NtkFixCoDriverProblem( pDriver, pNode, fDuplicate );
            nDupGates++;
            continue;
        }
        // if the driver is a CI and has different name, this is an error
        if ( Abc_ObjIsCi(pDriver) && strcmp(Abc_ObjName(pDriver), Abc_ObjName(pNode)) )
        {
            Abc_NtkFixCoDriverProblem( pDriver, pNode, fDuplicate );
            nDupGates++;
            continue;
        }
        // if the driver is visited for the first time, remember the CO name
        if ( !Abc_NodeIsTravIdCurrent(pDriver) )
        {
            pDriver->pNext = (Abc_Obj_t *)Abc_ObjName(pNode);
            Abc_NodeSetTravIdCurrent(pDriver);
            continue;
        }
        // the driver has second CO - if they have different name, this is an error
        if ( strcmp((char *)pDriver->pNext, Abc_ObjName(pNode)) ) // diff names
        {
            Abc_NtkFixCoDriverProblem( pDriver, pNode, fDuplicate );
            nDupGates++;
            continue;
        }
    }
    assert( Abc_NtkLogicHasSimpleCos(pNtk) );
    return nDupGates;
}


void Abc_NtkFixCoDriverProblem( Abc_Obj_t * pDriver, Abc_Obj_t * pNodeCo, int fDuplicate )
{
    Abc_Ntk_t * pNtk = pDriver->pNtk;
    Abc_Obj_t * pDriverNew, * pFanin;
    int k;
    if ( fDuplicate && !Abc_ObjIsCi(pDriver) )
    {
        pDriverNew = Abc_NtkDupObj( pNtk, pDriver, 0 );
        Abc_ObjForEachFanin( pDriver, pFanin, k )
            Abc_ObjAddFanin( pDriverNew, pFanin );
        if ( Abc_ObjFaninC0(pNodeCo) )
        {
            // change polarity of the duplicated driver
            Abc_NodeComplement( pDriverNew );
            Abc_ObjXorFaninC( pNodeCo, 0 );
        }
    }
    else
    {
        // add inverters and buffers when necessary
        if ( Abc_ObjFaninC0(pNodeCo) )
        {
            pDriverNew = Abc_NtkCreateNodeInv( pNtk, pDriver );
            Abc_ObjXorFaninC( pNodeCo, 0 );
        }
        else
            pDriverNew = Abc_NtkCreateNodeBuf( pNtk, pDriver );
    }
    // update the fanin of the PO node
    Abc_ObjPatchFanin( pNodeCo, pDriver, pDriverNew );
    assert( Abc_ObjFanoutNum(pDriverNew) == 1 );
    // remove the old driver if it dangles
    // (this happens when the duplicated driver had only one complemented fanout)
    if ( Abc_ObjFanoutNum(pDriver) == 0 )
        Abc_NtkDeleteObj( pDriver );
}


int Abc_NtkLogicHasSimpleCos( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode, * pDriver;
    int i;
    assert( Abc_NtkIsLogic(pNtk) );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        // if the driver is complemented, this is an error
        pDriver = Abc_ObjFanin0(pNode);
        if ( Abc_ObjFaninC0(pNode) )
            return 0;
        // if the driver is a CI and has different name, this is an error
        if ( Abc_ObjIsCi(pDriver) && strcmp(Abc_ObjName(pDriver), Abc_ObjName(pNode)) )
            return 0;
        // if the driver is visited for the first time, remember the CO name
        if ( !Abc_NodeIsTravIdCurrent(pDriver) )
        {
            pDriver->pNext = (Abc_Obj_t *)Abc_ObjName(pNode);
            Abc_NodeSetTravIdCurrent(pDriver);
            continue;
        }
        // the driver has second CO - if they have different name, this is an error
        if ( strcmp((char *)pDriver->pNext, Abc_ObjName(pNode)) ) // diff names
            return 0;
    }
    return 1;
}


Abc_Ntk_t * Abc_NtkToNetlist( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew, * pNtkTemp;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );
    if ( Abc_NtkIsStrash(pNtk) )
    {
        pNtkTemp = Abc_NtkAigToLogicSop(pNtk);
        pNtkNew = Abc_NtkLogicToNetlist( pNtkTemp );
        Abc_NtkDelete( pNtkTemp );
        return pNtkNew;
    }
    return Abc_NtkLogicToNetlist( pNtk );
}


Abc_Ntk_t * Abc_NtkLogicToNetlist( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pNet, * pDriver, * pFanin;
    int i, k;

    assert( Abc_NtkIsLogic(pNtk) );

    // remove dangling nodes
    Abc_NtkCleanup( pNtk, 0 );

    // make sure the CO names are unique
    Abc_NtkCheckUniqueCiNames( pNtk );
    Abc_NtkCheckUniqueCoNames( pNtk );
    Abc_NtkCheckUniqueCioNames( pNtk );

//    assert( Abc_NtkLogicHasSimpleCos(pNtk) );
    if ( !Abc_NtkLogicHasSimpleCos(pNtk) )
    {
        // if ( !Abc_FrameReadFlag("silentmode") )
        //     printf( "Abc_NtkLogicToNetlist() warning: The network is converted to have simple COs.\n" );
        Abc_NtkLogicMakeSimpleCos( pNtk, 0 );
    }

    // start the netlist by creating PI/PO/Latch objects
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_NETLIST, pNtk->ntkFunc );
    // create the CI nets and remember them in the new CI nodes
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        pNet = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pObj) );
        Abc_ObjAddFanin( pNet, pObj->pCopy );
        pObj->pCopy->pCopy = pNet;
    }
    // duplicate all nodes
    Abc_NtkForEachNode( pNtk, pObj, i )
        Abc_NtkDupObj(pNtkNew, pObj, 0);
    // first add the nets to the CO drivers
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pDriver = Abc_ObjFanin0(pObj);
        if ( Abc_ObjIsCi(pDriver) )
        {
            assert( !strcmp( Abc_ObjName(pDriver), Abc_ObjName(pObj) ) );
            Abc_ObjAddFanin( pObj->pCopy, pDriver->pCopy->pCopy );
            continue;
        }
        assert( Abc_ObjIsNode(pDriver) );
        // if the CO driver has no net, create it
        if ( pDriver->pCopy->pCopy == NULL )
        {
            // create the CO net and connect it to CO
            //if ( Abc_NtkFindNet(pNtkNew, Abc_ObjName(pDriver)) == NULL )
            //    pNet = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pDriver) );
            //else
            pNet = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pObj) );
            Abc_ObjAddFanin( pObj->pCopy, pNet );
            // connect the CO net to the new driver and remember it in the new driver
            Abc_ObjAddFanin( pNet, pDriver->pCopy );
            pDriver->pCopy->pCopy = pNet;
        }
        else
        {
            assert( !strcmp( Abc_ObjName(pDriver->pCopy->pCopy), Abc_ObjName(pObj) ) );
            Abc_ObjAddFanin( pObj->pCopy, pDriver->pCopy->pCopy );
        }
    }
    // create the missing nets
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        char Buffer[1000];
        if ( pObj->pCopy->pCopy ) // the net of the new object is already created
            continue;
        // create the new net
        sprintf( Buffer, "new_%s_", Abc_ObjName(pObj) );
        //pNet = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pObj) ); // here we create net names such as "n48", where 48 is the ID of the node
        pNet = Abc_NtkFindOrCreateNet( pNtkNew, Buffer );
        Abc_ObjAddFanin( pNet, pObj->pCopy );
        pObj->pCopy->pCopy = pNet;
    }
    // connect nodes to the fanins nets
    Abc_NtkForEachNode( pNtk, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy->pCopy );
    // duplicate EXDC
    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkToNetlist( pNtk->pExdc );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkLogicToNetlist(): Network check has failed.\n" );
    return pNtkNew;
}


Hop_Obj_t * Abc_ConvertSopToAig( Hop_Man_t * pMan, char * pSop )
{
    extern Hop_Obj_t * Dec_GraphFactorSop( Hop_Man_t * pMan, char * pSop );
    int fUseFactor = 1;
    // consider the constant node
    if ( Abc_SopGetVarNum(pSop) == 0 )
        return Hop_NotCond( Hop_ManConst1(pMan), Abc_SopIsConst0(pSop) );
    // decide when to use factoring
    if ( fUseFactor && Abc_SopGetVarNum(pSop) > 2 && Abc_SopGetCubeNum(pSop) > 1 && !Abc_SopIsExorType(pSop) )
        return Dec_GraphFactorSop( pMan, pSop );
    return Abc_ConvertSopToAigInternal( pMan, pSop );
}


Hop_Obj_t * Abc_ConvertSopToAigInternal( Hop_Man_t * pMan, char * pSop )
{
    Hop_Obj_t * pAnd, * pSum;
    int i, Value, nFanins;
    char * pCube;
    // get the number of variables
    nFanins = Abc_SopGetVarNum(pSop);
    if ( Abc_SopIsExorType(pSop) )
    {
        pSum = Hop_ManConst0(pMan);
        for ( i = 0; i < nFanins; i++ )
            pSum = Hop_Exor( pMan, pSum, Hop_IthVar(pMan,i) );
    }
    else
    {
        // go through the cubes of the node's SOP
        pSum = Hop_ManConst0(pMan);
        Abc_SopForEachCube( pSop, nFanins, pCube )
        {
            // create the AND of literals
            pAnd = Hop_ManConst1(pMan);
            Abc_CubeForEachVar( pCube, Value, i )
            {
                if ( Value == '1' )
                    pAnd = Hop_And( pMan, pAnd, Hop_IthVar(pMan,i) );
                else if ( Value == '0' )
                    pAnd = Hop_And( pMan, pAnd, Hop_Not(Hop_IthVar(pMan,i)) );
            }
            // add to the sum of cubes
            pSum = Hop_Or( pMan, pSum, pAnd );
        }
    }
    // decide whether to complement the result
    if ( Abc_SopIsComplement(pSop) )
        pSum = Hop_Not(pSum);
    return pSum;
}
