#include "contest_limit.h"
using namespace std;


Vec_Ptr_t * Contest_CollectFanoutExceedNodes( Abc_Ntk_t * pNtk, int nLimit, bool isPrint = false )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkIsSopLogic( pNtk ) );
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NtkForEachNode( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > nLimit )
            Vec_PtrPush( vNodes, pObj );
    if ( isPrint ) {
        Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
            cout << Abc_ObjId( pObj ) << "(" << Abc_ObjFanoutNum( pObj ) << ")" << "\t";
        cout << endl;
    }
    return vNodes;
}


void Contest_PrintStats( Abc_Ntk_t * pNtk, bool isInit )
{
    assert( Abc_NtkIsSopLogic( pNtk ) || Abc_NtkIsSopNetlist( pNtk ) );
    if ( isInit )
        cout << "Original AIG: ";
    else
        cout << "Final AIG: ";
    cout << "Maximum fanout = " << Abc_NtkGetFanoutMax( pNtk ) << ", "
    << "Level = " << Abc_NtkLevel( pNtk ) << ", "
    << "Size = " << Abc_NtkNodeNum( pNtk ) << endl;
}


void Contest_LimitFanout( Abc_Ntk_t * pNtk, int limit )
{
    assert( Abc_NtkIsSopLogic( pNtk ) );
    while ( Abc_NtkGetFanoutMax( pNtk ) > limit ) {
        // cout << Abc_NtkGetFanoutMax( pNtk ) << endl;
        Contest_LimitFanout_Iter( pNtk, limit );
    }
}


void Contest_LimitFanout_Iter( Abc_Ntk_t * pNtk, int limit )
{
    Abc_Obj_t * pObj;
    int i;
    Vec_Ptr_t * vNodes;
    vNodes = Contest_CollectFanoutExceedNodes( pNtk, limit, false );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Contest_NodeLimitFanout( pObj, limit );
    Vec_PtrFree( vNodes );
}


void Contest_NodeLimitFanout( Abc_Obj_t * pNode, int n )
{
    int nGroups, i, id, cnt;
    Abc_Obj_t * pFanout;
    Abc_Ntk_t * pNtk = pNode->pNtk;
    int m = Abc_ObjFanoutNum( pNode );
    Vec_Ptr_t * vFanouts = Vec_PtrAlloc( 20 );
    Vec_Ptr_t * vBufs = Vec_PtrAlloc( n );
    Abc_NodeCollectFanouts( pNode, vFanouts );
    // fix with buffers in one level
    if ( m  <= n * n ) {
        if ( m % n == 0 || m % n == 1 )
            nGroups = m / n;
        else
            nGroups = m / n + 1;
        for ( i = 0; i < nGroups; ++i )
            Vec_PtrPush( vBufs, Abc_NtkCreateNodeBuf( pNtk, pNode ) );
        id = cnt = 0;
        Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, i ) {
            if ( cnt == n ) {
                ++id;
                if ( id == Vec_PtrSize( vBufs ) ) {
                    assert( i + 1 == Vec_PtrSize( vFanouts ) );
                    break;
                }
                cnt = 0;
            }
            Abc_ObjPatchFanin( pFanout, pNode, (Abc_Obj_t*)Vec_PtrEntry( vBufs, id ) );
            ++cnt;
        }
    }
    // fix with buffers in multi-levels
    else {
        nGroups = n;
        for ( i = 0; i < nGroups; ++i )
            Vec_PtrPush( vBufs, Abc_NtkCreateNodeBuf( pNtk, pNode ) );
        id = cnt = 0;
        int tmp = m / n;
        Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, i ) {
            if ( cnt == tmp ) {
                ++id;
                if ( id == Vec_PtrSize( vBufs ) )
                    break;
                cnt = 0;
            }
            Abc_ObjPatchFanin( pFanout, pNode, (Abc_Obj_t*)Vec_PtrEntry( vBufs, id ) );
            ++cnt;
        }
        int iStart = i;
        id = 0;
        Vec_PtrForEachEntryStart( Abc_Obj_t *, vFanouts, pFanout, i, iStart ) {
            assert( id < Vec_PtrSize( vBufs ) );
            Abc_ObjPatchFanin( pFanout, pNode, (Abc_Obj_t*)Vec_PtrEntry( vBufs, id ) );
            ++id;
        }
    }
    Vec_PtrFree( vFanouts );
    Vec_PtrFree( vBufs );
}


bool Contest_LimitCheck( Abc_Ntk_t * pNtk, int limit )
{
    assert( Abc_NtkIsSopLogic( pNtk ) || Abc_NtkIsSopNetlist( pNtk ) );
    if ( Abc_NtkGetFanoutMax( pNtk ) > limit )
        return false;
    Abc_Obj_t * pObj;
    char * pSop;
    int i;
    Abc_NtkForEachNode( pNtk, pObj, i ) {
        if ( Abc_ObjFaninNum( pObj ) > 2 )
            return false;
        pSop = (char *)pObj->pData;
        if ( !(Abc_SopIsBuf( pSop ) || Contest_SopIsAndType( pSop )) ) {
            return false;
        }
    }
    return true;
}


int Contest_SopIsAndType( char * pSop )
{
    char * pCur;
    if ( Abc_SopGetCubeNum(pSop) != 1 )
        return 0;
    for ( pCur = pSop; *pCur != ' '; pCur++ )
        if ( *pCur == '-' )
            return 0;
    if ( pCur[1] != '1' && pCur[1] != '0' )
        return 0;
    return 1;
}


int Abc_SopIsBuf( char * pSop )
{
    if ( pSop[4] != 0 )
        return 0;
    if ( (pSop[0] == '1' && pSop[2] == '1') || (pSop[0] == '0' && pSop[2] == '0') )
        return 1;
    return 0;
}


Abc_Obj_t * Abc_ObjInsertBetween( Abc_Obj_t * pNodeIn, Abc_Obj_t * pNodeOut, Abc_ObjType_t Type )
{
    Abc_Obj_t * pNodeNew;
    int iFanoutIndex, iFaninIndex;
    // find pNodeOut among the fanouts of pNodeIn
    if ( (iFanoutIndex = Vec_IntFind( &pNodeIn->vFanouts, pNodeOut->Id )) == -1 )
    {
        printf( "Node %s is not among", Abc_ObjName(pNodeOut) );
        printf( " the fanouts of node %s...\n", Abc_ObjName(pNodeIn) );
        return NULL;
    }
    // find pNodeIn among the fanins of pNodeOut
    if ( (iFaninIndex = Vec_IntFind( &pNodeOut->vFanins, pNodeIn->Id )) == -1 )
    {
        printf( "Node %s is not among", Abc_ObjName(pNodeIn) );
        printf( " the fanins of node %s...\n", Abc_ObjName(pNodeOut) );
        return NULL;
    }
    // create the new node
    pNodeNew = Abc_NtkCreateObj( pNodeIn->pNtk, Type );
    // add pNodeIn as fanin and pNodeOut as fanout
    Vec_IntPushMem( pNodeNew->pNtk->pMmStep, &pNodeNew->vFanins,  pNodeIn->Id  );
    Vec_IntPushMem( pNodeNew->pNtk->pMmStep, &pNodeNew->vFanouts, pNodeOut->Id );
    // update the fanout of pNodeIn
    Vec_IntWriteEntry( &pNodeIn->vFanouts, iFanoutIndex, pNodeNew->Id );
    // update the fanin of pNodeOut
    Vec_IntWriteEntry( &pNodeOut->vFanins, iFaninIndex, pNodeNew->Id );
    return pNodeNew;
}



