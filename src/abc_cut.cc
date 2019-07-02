#include "abc_cut.h"


int nTotal = 0;
int nGood  = 0;
int nEqual = 0;


Cut_Man_t * Abc_NtkStartCutManForRewrite( Abc_Ntk_t * pNtk )
{
    static Cut_Params_t Params, * pParams = &Params;
    Cut_Man_t * pManCut;
    Abc_Obj_t * pObj;
    int i;
    // start the cut manager
    memset( pParams, 0, sizeof(Cut_Params_t) );
    pParams->nVarsMax  = 4;     // the max cut size ("k" of the k-feasible cuts)
    pParams->nKeepMax  = 250;   // the max number of cuts kept at a node
    pParams->fTruth    = 1;     // compute truth tables
    pParams->fFilter   = 1;     // filter dominated cuts
    pParams->fSeq      = 0;     // compute sequential cuts
    pParams->fDrop     = 0;     // drop cuts on the fly
    pParams->fVerbose  = 0;     // the verbosiness flag
    pParams->nIdsMax   = Abc_NtkObjNumMax( pNtk );
    pManCut = Cut_ManStart( pParams );
    if ( pParams->fDrop )
        Cut_ManSetFanoutCounts( pManCut, Abc_NtkFanoutCounts(pNtk) );
    // set cuts for PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
            Cut_NodeSetTriv( pManCut, pObj->Id );
    return pManCut;
}


void * Abc_NodeGetCutsRecursive( void * p, Abc_Obj_t * pObj, int fDag, int fTree )
{
    void * pList;
    if ( (pList = Abc_NodeReadCuts( p, pObj )) )
        return pList;
    Abc_NodeGetCutsRecursive( p, Abc_ObjFanin0(pObj), fDag, fTree );
    Abc_NodeGetCutsRecursive( p, Abc_ObjFanin1(pObj), fDag, fTree );
    return Abc_NodeGetCuts( p, pObj, fDag, fTree );
}


void * Abc_NodeGetCuts( void * p, Abc_Obj_t * pObj, int fDag, int fTree )
{
    Abc_Obj_t * pFanin;
    int fDagNode, fTriv, TreeCode = 0;
//    assert( Abc_NtkIsStrash(pObj->pNtk) );
    assert( Abc_ObjFaninNum(pObj) == 2 );

    // check if the node is a DAG node
    fDagNode = (Abc_ObjFanoutNum(pObj) > 1 && !Abc_NodeIsMuxControlType(pObj));
    // increment the counter of DAG nodes
    if ( fDagNode ) Cut_ManIncrementDagNodes( (Cut_Man_t *)p );
    // add the trivial cut if the node is a DAG node, or if we compute all cuts
    fTriv = fDagNode || !fDag;
    // check if fanins are DAG nodes
    if ( fTree )
    {
        pFanin = Abc_ObjFanin0(pObj);
        TreeCode |=  (Abc_ObjFanoutNum(pFanin) > 1 && !Abc_NodeIsMuxControlType(pFanin));
        pFanin = Abc_ObjFanin1(pObj);
        TreeCode |= ((Abc_ObjFanoutNum(pFanin) > 1 && !Abc_NodeIsMuxControlType(pFanin)) << 1);
    }

    // changes due to the global/local cut computation
    {
        Cut_Params_t * pParams = Cut_ManReadParams((Cut_Man_t *)p);
        if ( pParams->fLocal )
        {
            Vec_Int_t * vNodeAttrs = Cut_ManReadNodeAttrs((Cut_Man_t *)p);
            fDagNode = Vec_IntEntry( vNodeAttrs, pObj->Id );
            if ( fDagNode ) Cut_ManIncrementDagNodes( (Cut_Man_t *)p );
            fTriv = !Vec_IntEntry( vNodeAttrs, pObj->Id );
            TreeCode = 0;
            pFanin = Abc_ObjFanin0(pObj);
            TreeCode |=  Vec_IntEntry( vNodeAttrs, pFanin->Id );
            pFanin = Abc_ObjFanin1(pObj);
            TreeCode |= (Vec_IntEntry( vNodeAttrs, pFanin->Id ) << 1);
        }
    }
    return Cut_NodeComputeCuts( (Cut_Man_t *)p, pObj->Id, Abc_ObjFaninId0(pObj), Abc_ObjFaninId1(pObj),
        Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj), fTriv, TreeCode );
}


void * Abc_NodeReadCuts( void * p, Abc_Obj_t * pObj )
{
    return Cut_NodeReadCutsNew( (Cut_Man_t *)p, pObj->Id );
}


void Abc_NodeFreeCuts( void * p, Abc_Obj_t * pObj )
{
    Cut_NodeFreeCuts( (Cut_Man_t *)p, pObj->Id );
}


Cut_Man_t * Cut_ManStart( Cut_Params_t * pParams )
{
    Cut_Man_t * p;
//    extern int nTruthDsd;
//    nTruthDsd = 0;
    assert( pParams->nVarsMax >= 3 && pParams->nVarsMax <= CUT_SIZE_MAX );
    p = ABC_ALLOC( Cut_Man_t, 1 );
    memset( p, 0, sizeof(Cut_Man_t) );
    // set and correct parameters
    p->pParams = pParams;
    // prepare storage for cuts
    p->vCutsNew = Vec_PtrAlloc( pParams->nIdsMax );
    Vec_PtrFill( p->vCutsNew, pParams->nIdsMax, NULL );
    // prepare storage for sequential cuts
    if ( pParams->fSeq )
    {
        p->pParams->fFilter = 1;
        p->vCutsOld = Vec_PtrAlloc( pParams->nIdsMax );
        Vec_PtrFill( p->vCutsOld, pParams->nIdsMax, NULL );
        p->vCutsTemp = Vec_PtrAlloc( pParams->nCutSet );
        Vec_PtrFill( p->vCutsTemp, pParams->nCutSet, NULL );
        if ( pParams->fTruth && pParams->nVarsMax > 5 )
        {
            pParams->fTruth = 0;
            printf( "Skipping computation of truth tables for sequential cuts with more than 5 inputs.\n" );
        }
    }
    // entry size
    p->EntrySize = sizeof(Cut_Cut_t) + pParams->nVarsMax * sizeof(int);
    if ( pParams->fTruth )
    {
        if ( pParams->nVarsMax > 14 )
        {
            pParams->fTruth = 0;
            printf( "Skipping computation of truth table for more than %d inputs.\n", 14 );
        }
        else
        {
            p->nTruthWords = Cut_TruthWords( pParams->nVarsMax );
            p->EntrySize += p->nTruthWords * sizeof(unsigned);
        }
        p->puTemp[0] = ABC_ALLOC( unsigned, 4 * p->nTruthWords );
        p->puTemp[1] = p->puTemp[0] + p->nTruthWords;
        p->puTemp[2] = p->puTemp[1] + p->nTruthWords;
        p->puTemp[3] = p->puTemp[2] + p->nTruthWords;
    }
    // enable cut computation recording
    if ( pParams->fRecord )
    {
        p->vNodeCuts   = Vec_IntStart( pParams->nIdsMax );
        p->vNodeStarts = Vec_IntStart( pParams->nIdsMax );
        p->vCutPairs   = Vec_IntAlloc( 0 );
    }
    // allocate storage for delays
    if ( pParams->fMap && !p->pParams->fSeq )
    {
        p->vDelays = Vec_IntStart( pParams->nIdsMax );
        p->vDelays2 = Vec_IntStart( pParams->nIdsMax );
        p->vCutsMax = Vec_PtrStart( pParams->nIdsMax );
    }
    // memory for cuts
    p->pMmCuts = Extra_MmFixedStart( p->EntrySize );
    p->vTemp = Vec_PtrAlloc( 100 );
    return p;
}


void Cut_ManSetFanoutCounts( Cut_Man_t * p, Vec_Int_t * vFanCounts )
{
    p->vFanCounts = vFanCounts;
}


void Cut_NodeSetTriv( Cut_Man_t * p, int Node )
{
    assert( Cut_NodeReadCutsNew(p, Node) == NULL );
    Cut_NodeWriteCutsNew( p, Node, Cut_CutCreateTriv(p, Node) );
}


Cut_Cut_t * Cut_NodeReadCutsNew( Cut_Man_t * p, int Node )
{
    if ( Node >= p->vCutsNew->nSize )
        return NULL;
    return (Cut_Cut_t *)Vec_PtrEntry( p->vCutsNew, Node );
}


void Cut_NodeWriteCutsNew( Cut_Man_t * p, int Node, Cut_Cut_t * pList )
{
    Vec_PtrWriteEntry( p->vCutsNew, Node, pList );
}


Cut_Cut_t * Cut_CutCreateTriv( Cut_Man_t * p, int Node )
{
    Cut_Cut_t * pCut;
    if ( p->pParams->fSeq )
        Node <<= CUT_SHIFT;
    pCut = Cut_CutAlloc( p );
    pCut->nLeaves    = 1;
    pCut->pLeaves[0] = Node;
    pCut->uSign      = Cut_NodeSign( Node );
    if ( p->pParams->fTruth )
    {
/*
        if ( pCut->nVarsMax == 4 )
            Cut_CutWriteTruth( pCut, p->uTruthVars[0] );
        else
            Extra_BitCopy( pCut->nLeaves, p->uTruths[0], (uint8*)Cut_CutReadTruth(pCut) );
*/
        unsigned * pTruth = Cut_CutReadTruth(pCut);
        int i;
        for ( i = 0; i < p->nTruthWords; i++ )
            pTruth[i] = 0xAAAAAAAA;
    }
    p->nCutsTriv++;
    return pCut;
}


Cut_Cut_t * Cut_CutAlloc( Cut_Man_t * p )
{
    Cut_Cut_t * pCut;
    // cut allocation
    pCut = (Cut_Cut_t *)Extra_MmFixedEntryFetch( p->pMmCuts );
    memset( pCut, 0, sizeof(Cut_Cut_t) );
    pCut->nVarsMax   = p->pParams->nVarsMax;
    pCut->fSimul     = p->fSimul;
    // statistics
    p->nCutsAlloc++;
    p->nCutsCur++;
    if ( p->nCutsPeak < p->nCutsAlloc - p->nCutsDealloc )
        p->nCutsPeak = p->nCutsAlloc - p->nCutsDealloc;
    return pCut;
}


void Cut_ManIncrementDagNodes( Cut_Man_t * p )
{
    p->nNodesDag++;
}


Cut_Params_t * Cut_ManReadParams( Cut_Man_t * p )
{
    return p->pParams;
}


Vec_Int_t * Cut_ManReadNodeAttrs( Cut_Man_t * p )
{
    return p->vNodeAttrs;
}


static inline void Cut_ListStart( Cut_List_t * p )
{
    int i;
    for ( i = 1; i <= CUT_SIZE_MAX; i++ )
    {
        p->pHead[i] = 0;
        p->ppTail[i] = &p->pHead[i];
    }
}


static inline Cut_Cut_t * Cut_ListFinish( Cut_List_t * p )
{
    Cut_Cut_t * pHead = NULL, ** ppTail = &pHead;
    int i;
    for ( i = 1; i <= CUT_SIZE_MAX; i++ )
    {
        if ( p->pHead[i] == NULL )
            continue;
        *ppTail = p->pHead[i];
        ppTail = p->ppTail[i];
    }
    *ppTail = NULL;
    return pHead;
}


Cut_Cut_t * Cut_NodeComputeCuts( Cut_Man_t * p, int Node, int Node0, int Node1, int fCompl0, int fCompl1, int fTriv, int TreeCode )
{
    Cut_List_t Super, * pSuper = &Super;
    Cut_Cut_t * pList, * pCut;
    // abctime clk;
    // start the number of cuts at the node
    p->nNodes++;
    p->nNodeCuts = 0;
    // prepare information for recording
    if ( p->pParams->fRecord )
    {
        Cut_CutNumberList( Cut_NodeReadCutsNew(p, Node0) );
        Cut_CutNumberList( Cut_NodeReadCutsNew(p, Node1) );
    }
    // compute the cuts
// clk = Abc_Clock();
    Cut_ListStart( pSuper );
    Cut_NodeDoComputeCuts( p, pSuper, Node, fCompl0, fCompl1, Cut_NodeReadCutsNew(p, Node0), Cut_NodeReadCutsNew(p, Node1), fTriv, TreeCode );
    pList = Cut_ListFinish( pSuper );
// p->timeMerge += Abc_Clock() - clk;
    // verify the result of cut computation
//    Cut_CutListVerify( pList );
    // performing the recording
    if ( p->pParams->fRecord )
    {
        Vec_IntWriteEntry( p->vNodeStarts, Node, Vec_IntSize(p->vCutPairs) );
        Cut_ListForEachCut( pList, pCut )
            Vec_IntPush( p->vCutPairs, ((pCut->Num1 << 16) | pCut->Num0) );
        Vec_IntWriteEntry( p->vNodeCuts, Node, Vec_IntSize(p->vCutPairs) - Vec_IntEntry(p->vNodeStarts, Node) );
    }
    if ( p->pParams->fRecordAig )
    {
        // extern void Aig_RManRecord( unsigned * pTruth, int nVarsInit );
        Cut_ListForEachCut( pList, pCut )
            if ( Cut_CutReadLeaveNum(pCut) > 4 )
                Aig_RManRecord( Cut_CutReadTruth(pCut), Cut_CutReadLeaveNum(pCut) );
    }
    // check if the node is over the list
    if ( p->nNodeCuts == p->pParams->nKeepMax )
        p->nCutsLimit++;
    // set the list at the node
    Vec_PtrFillExtra( p->vCutsNew, Node + 1, NULL );
    assert( Cut_NodeReadCutsNew(p, Node) == NULL );

    Cut_NodeWriteCutsNew( p, Node, pList );
    // perform mapping of this node with these cuts
// clk = Abc_Clock();
    if ( p->pParams->fMap && !p->pParams->fSeq )
    {
        Cut_NodeMapping( p, pList, Node, Node0, Node1 );
    }
// p->timeMap += Abc_Clock() - clk;
    return pList;
}


void Cut_CutNumberList( Cut_Cut_t * pList )
{
    Cut_Cut_t * pCut;
    int i = 0;
    Cut_ListForEachCut( pList, pCut )
        pCut->Num0 = i++;
}

static inline void Cut_ListAdd( Cut_List_t * p, Cut_Cut_t * pCut )
{
    assert( pCut->nLeaves > 0 && pCut->nLeaves <= CUT_SIZE_MAX );
    *p->ppTail[pCut->nLeaves] = pCut;
    p->ppTail[pCut->nLeaves] = &pCut->pNext;
}


static inline int Cut_CutCheckDominance( Cut_Cut_t * pDom, Cut_Cut_t * pCut )
{
    int i, k;
    for ( i = 0; i < (int)pDom->nLeaves; i++ )
    {
        for ( k = 0; k < (int)pCut->nLeaves; k++ )
            if ( pDom->pLeaves[i] == pCut->pLeaves[k] )
                break;
        if ( k == (int)pCut->nLeaves ) // node i in pDom is not contained in pCut
            return 0;
    }
    // every node in pDom is contained in pCut
    return 1;
}


static inline int Cut_CutFilterOne( Cut_Man_t * p, Cut_List_t * pSuperList, Cut_Cut_t * pCut )
{
    Cut_Cut_t * pTemp, * pTemp2, ** ppTail;
    int a;

    // check if this cut is filtered out by smaller cuts
    for ( a = 2; a <= (int)pCut->nLeaves; a++ )
    {
        Cut_ListForEachCut( pSuperList->pHead[a], pTemp )
        {
            // skip the non-contained cuts
            if ( (pTemp->uSign & pCut->uSign) != pTemp->uSign )
                continue;
            // check containment seriously
            if ( Cut_CutCheckDominance( pTemp, pCut ) )
            {
                p->nCutsFilter++;
                Cut_CutRecycle( p, pCut );
                return 1;
            }
        }
    }

    // filter out other cuts using this one
    for ( a = pCut->nLeaves + 1; a <= (int)pCut->nVarsMax; a++ )
    {
        ppTail = pSuperList->pHead + a;
        Cut_ListForEachCutSafe( pSuperList->pHead[a], pTemp, pTemp2 )
        {
            // skip the non-contained cuts
            if ( (pTemp->uSign & pCut->uSign) != pCut->uSign )
            {
                ppTail = &pTemp->pNext;
                continue;
            }
            // check containment seriously
            if ( Cut_CutCheckDominance( pCut, pTemp ) )
            {
                p->nCutsFilter++;
                p->nNodeCuts--;
                // move the head
                if ( pSuperList->pHead[a] == pTemp )
                    pSuperList->pHead[a] = pTemp->pNext;
                // move the tail
                if ( pSuperList->ppTail[a] == &pTemp->pNext )
                    pSuperList->ppTail[a] = ppTail;
                // skip the given cut in the list
                *ppTail = pTemp->pNext;
                // recycle pTemp
                Cut_CutRecycle( p, pTemp );
            }
            else
                ppTail = &pTemp->pNext;
        }
        assert( ppTail == pSuperList->ppTail[a] );
        assert( *ppTail == NULL );
    }

    return 0;
}


static inline int Cut_CutFilterOld( Cut_Man_t * p, Cut_Cut_t * pList, Cut_Cut_t * pCut )
{
    Cut_Cut_t * pPrev, * pTemp, * pTemp2, ** ppTail;

    // check if this cut is filtered out by smaller cuts
    pPrev = NULL;
    Cut_ListForEachCut( pList, pTemp )
    {
        if ( pTemp->nLeaves > pCut->nLeaves )
            break;
        pPrev = pTemp;
        // skip the non-contained cuts
        if ( (pTemp->uSign & pCut->uSign) != pTemp->uSign )
            continue;
        // check containment seriously
        if ( Cut_CutCheckDominance( pTemp, pCut ) )
        {
            p->nCutsFilter++;
            Cut_CutRecycle( p, pCut );
            return 1;
        }
    }
    assert( pPrev->pNext == pTemp );

    // filter out other cuts using this one
    ppTail = &pPrev->pNext;
    Cut_ListForEachCutSafe( pTemp, pTemp, pTemp2 )
    {
        // skip the non-contained cuts
        if ( (pTemp->uSign & pCut->uSign) != pCut->uSign )
        {
            ppTail = &pTemp->pNext;
            continue;
        }
        // check containment seriously
        if ( Cut_CutCheckDominance( pCut, pTemp ) )
        {
            p->nCutsFilter++;
            p->nNodeCuts--;
            // skip the given cut in the list
            *ppTail = pTemp->pNext;
            // recycle pTemp
            Cut_CutRecycle( p, pTemp );
        }
        else
            ppTail = &pTemp->pNext;
    }
    assert( *ppTail == NULL );
    return 0;
}


static inline int Cut_CutFilterGlobal( Cut_Man_t * p, Cut_Cut_t * pCut )
{
    int a;
    if ( pCut->nLeaves == 1 )
        return 0;
    for ( a = 0; a < (int)pCut->nLeaves; a++ )
        if ( Vec_IntEntry( p->vNodeAttrs, pCut->pLeaves[a] ) ) // global
            return 0;
    // there is no global nodes, the cut should be removed
    p->nCutsFilter++;
    Cut_CutRecycle( p, pCut );
    return 1;
}


static inline int Cut_CutProcessTwo( Cut_Man_t * p, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1, Cut_List_t * pSuperList )
{
    Cut_Cut_t * pCut;
    // merge the cuts
    if ( pCut0->nLeaves >= pCut1->nLeaves )
        pCut = Cut_CutMergeTwo( p, pCut0, pCut1 );
    else
        pCut = Cut_CutMergeTwo( p, pCut1, pCut0 );
    if ( pCut == NULL )
        return 0;
    assert( p->pParams->fSeq || pCut->nLeaves > 1 );
    // set the signature
    pCut->uSign = pCut0->uSign | pCut1->uSign;
    if ( p->pParams->fRecord )
        pCut->Num0 = pCut0->Num0, pCut->Num1 = pCut1->Num0;
    // check containment
    if ( p->pParams->fFilter )
    {
        if ( Cut_CutFilterOne(p, pSuperList, pCut) )
//        if ( Cut_CutFilterOneEqual(p, pSuperList, pCut) )
            return 0;
        if ( p->pParams->fSeq )
        {
            if ( p->pCompareOld && Cut_CutFilterOld(p, p->pCompareOld, pCut) )
                return 0;
            if ( p->pCompareNew && Cut_CutFilterOld(p, p->pCompareNew, pCut) )
                return 0;
        }
    }

    if ( p->pParams->fGlobal )
    {
        assert( p->vNodeAttrs != NULL );
        if ( Cut_CutFilterGlobal( p, pCut ) )
            return 0;
    }

    // compute the truth table
    if ( p->pParams->fTruth )
        Cut_TruthCompute( p, pCut, pCut0, pCut1, p->fCompl0, p->fCompl1 );
    // add to the list
    Cut_ListAdd( pSuperList, pCut );
    // return status (0 if okay; 1 if exceeded the limit)
    return ++p->nNodeCuts == p->pParams->nKeepMax;
}


void Cut_NodeDoComputeCuts( Cut_Man_t * p, Cut_List_t * pSuper, int Node, int fCompl0, int fCompl1, Cut_Cut_t * pList0, Cut_Cut_t * pList1, int fTriv, int TreeCode )
{
    Cut_Cut_t * pStop0, * pStop1, * pTemp0, * pTemp1;
    Cut_Cut_t * pStore0 = NULL, * pStore1 = NULL; // Suppress "might be used uninitialized"
    // int i, nCutsOld, Limit;
    int i, Limit;
    // start with the elementary cut
    if ( fTriv )
    {
//        printf( "Creating trivial cut %d.\n", Node );
        pTemp0 = Cut_CutCreateTriv( p, Node );
        Cut_ListAdd( pSuper, pTemp0 );
        p->nNodeCuts++;
    }
    // get the cut lists of children
    if ( pList0 == NULL || pList1 == NULL || (p->pParams->fLocal && TreeCode)  )
        return;

    // remember the old number of cuts
    // nCutsOld = p->nCutsCur;
    Limit = p->pParams->nVarsMax;
    // get the simultation bit of the node
    p->fSimul = (fCompl0 ^ pList0->fSimul) & (fCompl1 ^ pList1->fSimul);
    // set temporary variables
    p->fCompl0 = fCompl0;
    p->fCompl1 = fCompl1;
    // if tree cuts are computed, make sure only the unit cuts propagate over the DAG nodes
    if ( TreeCode & 1 )
    {
        assert( pList0->nLeaves == 1 );
        pStore0 = pList0->pNext;
        pList0->pNext = NULL;
    }
    if ( TreeCode & 2 )
    {
        assert( pList1->nLeaves == 1 );
        pStore1 = pList1->pNext;
        pList1->pNext = NULL;
    }
    // find the point in the list where the max-var cuts begin
    Cut_ListForEachCut( pList0, pStop0 )
        if ( pStop0->nLeaves == (unsigned)Limit )
            break;
    Cut_ListForEachCut( pList1, pStop1 )
        if ( pStop1->nLeaves == (unsigned)Limit )
            break;

    // small by small
    Cut_ListForEachCutStop( pList0, pTemp0, pStop0 )
    Cut_ListForEachCutStop( pList1, pTemp1, pStop1 )
    {
        if ( Cut_CutProcessTwo( p, pTemp0, pTemp1, pSuper ) )
            goto Quits;
    }
    // small by large
    Cut_ListForEachCutStop( pList0, pTemp0, pStop0 )
    Cut_ListForEachCut( pStop1, pTemp1 )
    {
        if ( (pTemp0->uSign & pTemp1->uSign) != pTemp0->uSign )
            continue;
        if ( Cut_CutProcessTwo( p, pTemp0, pTemp1, pSuper ) )
            goto Quits;
    }
    // small by large
    Cut_ListForEachCutStop( pList1, pTemp1, pStop1 )
    Cut_ListForEachCut( pStop0, pTemp0 )
    {
        if ( (pTemp0->uSign & pTemp1->uSign) != pTemp1->uSign )
            continue;
        if ( Cut_CutProcessTwo( p, pTemp0, pTemp1, pSuper ) )
            goto Quits;
    }
    // large by large
    Cut_ListForEachCut( pStop0, pTemp0 )
    Cut_ListForEachCut( pStop1, pTemp1 )
    {
        assert( pTemp0->nLeaves == (unsigned)Limit && pTemp1->nLeaves == (unsigned)Limit );
        if ( pTemp0->uSign != pTemp1->uSign )
            continue;
        for ( i = 0; i < Limit; i++ )
            if ( pTemp0->pLeaves[i] != pTemp1->pLeaves[i] )
                break;
        if ( i < Limit )
            continue;
        if ( Cut_CutProcessTwo( p, pTemp0, pTemp1, pSuper ) )
            goto Quits;
    }
    if ( p->nNodeCuts == 0 )
        p->nNodesNoCuts++;
Quits:
    if ( TreeCode & 1 )
        pList0->pNext = pStore0;
    if ( TreeCode & 2 )
        pList1->pNext = pStore1;
}


Cut_Cut_t * Cut_CutMergeTwo( Cut_Man_t * p, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1 )
{
    Cut_Cut_t * pRes;
    int * pLeaves;
    int Limit, nLeaves0, nLeaves1;
    int i, k, c;

    assert( pCut0->nLeaves >= pCut1->nLeaves );

    // consider two cuts
    nLeaves0 = pCut0->nLeaves;
    nLeaves1 = pCut1->nLeaves;

    // the case of the largest cut sizes
    Limit = p->pParams->nVarsMax;
    if ( nLeaves0 == Limit && nLeaves1 == Limit )
    {
        for ( i = 0; i < nLeaves0; i++ )
            if ( pCut0->pLeaves[i] != pCut1->pLeaves[i] )
                return NULL;
        pRes = Cut_CutAlloc( p );
        for ( i = 0; i < nLeaves0; i++ )
            pRes->pLeaves[i] = pCut0->pLeaves[i];
        pRes->nLeaves = pCut0->nLeaves;
        return pRes;
    }
    // the case when one of the cuts is the largest
    if ( nLeaves0 == Limit )
    {
        for ( i = 0; i < nLeaves1; i++ )
        {
            for ( k = nLeaves0 - 1; k >= 0; k-- )
                if ( pCut0->pLeaves[k] == pCut1->pLeaves[i] )
                    break;
            if ( k == -1 ) // did not find
                return NULL;
        }
        pRes = Cut_CutAlloc( p );
        for ( i = 0; i < nLeaves0; i++ )
            pRes->pLeaves[i] = pCut0->pLeaves[i];
        pRes->nLeaves = pCut0->nLeaves;
        return pRes;
    }

    // prepare the cut
    if ( p->pReady == NULL )
        p->pReady = Cut_CutAlloc( p );
    pLeaves = p->pReady->pLeaves;

    // compare two cuts with different numbers
    i = k = 0;
    for ( c = 0; c < Limit; c++ )
    {
        if ( k == nLeaves1 )
        {
            if ( i == nLeaves0 )
            {
                p->pReady->nLeaves = c;
                pRes = p->pReady;  p->pReady = NULL;
                return pRes;
            }
            pLeaves[c] = pCut0->pLeaves[i++];
            continue;
        }
        if ( i == nLeaves0 )
        {
            if ( k == nLeaves1 )
            {
                p->pReady->nLeaves = c;
                pRes = p->pReady;  p->pReady = NULL;
                return pRes;
            }
            pLeaves[c] = pCut1->pLeaves[k++];
            continue;
        }
        if ( pCut0->pLeaves[i] < pCut1->pLeaves[k] )
        {
            pLeaves[c] = pCut0->pLeaves[i++];
            continue;
        }
        if ( pCut0->pLeaves[i] > pCut1->pLeaves[k] )
        {
            pLeaves[c] = pCut1->pLeaves[k++];
            continue;
        }
        pLeaves[c] = pCut0->pLeaves[i++];
        k++;
    }
    if ( i < nLeaves0 || k < nLeaves1 )
        return NULL;
    p->pReady->nLeaves = c;
    pRes = p->pReady;  p->pReady = NULL;
    return pRes;
}


void Cut_CutRecycle( Cut_Man_t * p, Cut_Cut_t * pCut )
{
    p->nCutsDealloc++;
    p->nCutsCur--;
    if ( pCut->nLeaves == 1 )
        p->nCutsTriv--;
    Extra_MmFixedEntryRecycle( p->pMmCuts, (char *)pCut );
}


static inline unsigned Cut_TruthPhase( Cut_Cut_t * pCut, Cut_Cut_t * pCut1 )
{
    unsigned uPhase = 0;
    int i, k;
    for ( i = k = 0; i < (int)pCut->nLeaves; i++ )
    {
        if ( k == (int)pCut1->nLeaves )
            break;
        if ( pCut->pLeaves[i] < pCut1->pLeaves[k] )
            continue;
        assert( pCut->pLeaves[i] == pCut1->pLeaves[k] );
        uPhase |= (1 << i);
        k++;
    }
    return uPhase;
}


void Cut_TruthCompute( Cut_Man_t * p, Cut_Cut_t * pCut, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1, int fCompl0, int fCompl1 )
{
    // permute the first table
    if ( fCompl0 )
        Extra_TruthNot( p->puTemp[0], Cut_CutReadTruth(pCut0), pCut->nVarsMax );
    else
        Extra_TruthCopy( p->puTemp[0], Cut_CutReadTruth(pCut0), pCut->nVarsMax );
    Extra_TruthStretch( p->puTemp[2], p->puTemp[0], pCut0->nLeaves, pCut->nVarsMax, Cut_TruthPhase(pCut, pCut0) );
    // permute the second table
    if ( fCompl1 )
        Extra_TruthNot( p->puTemp[1], Cut_CutReadTruth(pCut1), pCut->nVarsMax );
    else
        Extra_TruthCopy( p->puTemp[1], Cut_CutReadTruth(pCut1), pCut->nVarsMax );
    Extra_TruthStretch( p->puTemp[3], p->puTemp[1], pCut1->nLeaves, pCut->nVarsMax, Cut_TruthPhase(pCut, pCut1) );
    // produce the resulting table
    if ( pCut->fCompl )
        Extra_TruthNand( Cut_CutReadTruth(pCut), p->puTemp[2], p->puTemp[3], pCut->nVarsMax );
    else
        Extra_TruthAnd( Cut_CutReadTruth(pCut), p->puTemp[2], p->puTemp[3], pCut->nVarsMax );

//    Ivy_TruthTestOne( *Cut_CutReadTruth(pCut) );

    // quit if no fancy computation is needed
    if ( !p->pParams->fFancy )
        return;

    if ( pCut->nLeaves != 7 )
        return;

    // count the total number of truth tables computed
    nTotal++;

    // MAPPING INTO ALTERA 6-2 LOGIC BLOCKS
    // call this procedure to find the minimum number of common variables in the cofactors
    // if this number is less or equal than 3, the cut can be implemented using the 6-2 logic block
    if ( Extra_TruthMinCofSuppOverlap( Cut_CutReadTruth(pCut), pCut->nVarsMax, NULL ) <= 4 )
        nGood++;

    // MAPPING INTO ACTEL 2x2 CELLS
    // call this procedure to see if a semi-canonical form can be found in the lookup table
    // (if it exists, then a two-level 3-input LUT implementation of the cut exists)
    // Before this procedure is called, cell manager should be defined by calling
    // Cut_CellLoad (make sure file "cells22_daomap_iwls.txt" is available in the working dir)
//    if ( Cut_CellIsRunning() && pCut->nVarsMax <= 9 )
//        nGood += Cut_CellTruthLookup( Cut_CutReadTruth(pCut), pCut->nVarsMax );
}


int Cut_NodeMapping( Cut_Man_t * p, Cut_Cut_t * pCuts, int Node, int Node0, int Node1 )
{
    Cut_Cut_t * pCut0, * pCut1, * pCut;
    int Delay0, Delay1, Delay;
    // get the fanin cuts
    Delay0 = Vec_IntEntry( p->vDelays2, Node0 );
    Delay1 = Vec_IntEntry( p->vDelays2, Node1 );
    pCut0 = (Delay0 == 0) ? (Cut_Cut_t *)Vec_PtrEntry( p->vCutsNew, Node0 ) : (Cut_Cut_t *)Vec_PtrEntry( p->vCutsMax, Node0 );
    pCut1 = (Delay1 == 0) ? (Cut_Cut_t *)Vec_PtrEntry( p->vCutsNew, Node1 ) : (Cut_Cut_t *)Vec_PtrEntry( p->vCutsMax, Node1 );
    if ( Delay0 == Delay1 )
        Delay = (Delay0 == 0) ? Delay0 + 1: Delay0;
    else if ( Delay0 > Delay1 )
    {
        Delay = Delay0;
        pCut1 = (Cut_Cut_t *)Vec_PtrEntry( p->vCutsNew, Node1 );
        assert( pCut1->nLeaves == 1 );
    }
    else // if ( Delay0 < Delay1 )
    {
        Delay = Delay1;
        pCut0 = (Cut_Cut_t *)Vec_PtrEntry( p->vCutsNew, Node0 );
        assert( pCut0->nLeaves == 1 );
    }
    // merge the cuts
    if ( pCut0->nLeaves < pCut1->nLeaves )
        pCut  = Cut_CutMergeTwo( p, pCut1, pCut0 );
    else
        pCut  = Cut_CutMergeTwo( p, pCut0, pCut1 );
    if ( pCut == NULL )
    {
        Delay++;
        pCut = Cut_CutAlloc( p );
        pCut->nLeaves = 2;
        pCut->pLeaves[0] = Node0 < Node1 ? Node0 : Node1;
        pCut->pLeaves[1] = Node0 < Node1 ? Node1 : Node0;
    }
    assert( Delay > 0 );
    Vec_IntWriteEntry( p->vDelays2, Node, Delay );
    Vec_PtrWriteEntry( p->vCutsMax, Node, pCut );
    if ( p->nDelayMin < Delay )
        p->nDelayMin = Delay;
    return Delay;
}


void Cut_ManStop( Cut_Man_t * p )
{
    if ( p->vCutsNew )    Vec_PtrFree( p->vCutsNew );
    if ( p->vCutsOld )    Vec_PtrFree( p->vCutsOld );
    if ( p->vCutsTemp )   Vec_PtrFree( p->vCutsTemp );
    if ( p->vFanCounts )  Vec_IntFree( p->vFanCounts );
    if ( p->vTemp )       Vec_PtrFree( p->vTemp );

    if ( p->vCutsMax )    Vec_PtrFree( p->vCutsMax );
    if ( p->vDelays )     Vec_IntFree( p->vDelays );
    if ( p->vDelays2 )    Vec_IntFree( p->vDelays2 );
    if ( p->vNodeCuts )   Vec_IntFree( p->vNodeCuts );
    if ( p->vNodeStarts ) Vec_IntFree( p->vNodeStarts );
    if ( p->vCutPairs )   Vec_IntFree( p->vCutPairs );
    if ( p->puTemp[0] )   ABC_FREE( p->puTemp[0] );

    Extra_MmFixedStop( p->pMmCuts );
    ABC_FREE( p );
}


void Cut_NodeFreeCuts( Cut_Man_t * p, int Node )
{
    Cut_Cut_t * pList, * pCut, * pCut2;
    pList = Cut_NodeReadCutsNew( p, Node );
    if ( pList == NULL )
        return;
    Cut_ListForEachCutSafe( pList, pCut, pCut2 )
        Cut_CutRecycle( p, pCut );
    Cut_NodeWriteCutsNew( p, Node, NULL );
}
