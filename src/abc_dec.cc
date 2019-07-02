#include "abc_dec.h"


Hop_Obj_t * Dec_GraphFactorSop( Hop_Man_t * pMan, char * pSop )
{
    Hop_Obj_t * pFunc;
    Dec_Graph_t * pFForm;
    Dec_Node_t * pNode;
    int i;
    // perform factoring
    pFForm = Dec_Factor( pSop );
    // collect the fanins
    Dec_GraphForEachLeaf( pFForm, pNode, i )
        pNode->pFunc = Hop_IthVar( pMan, i );
    // perform strashing
    pFunc = Dec_GraphToNetworkAig( pMan, pFForm );
    Dec_GraphFree( pFForm );
    return pFunc;
}


Dec_Graph_t * Dec_Factor( char * pSop )
{
    Mvc_Cover_t * pCover;
    Dec_Graph_t * pFForm;
    Dec_Edge_t eRoot;
    if ( Abc_SopIsConst0(pSop) )
        return Dec_GraphCreateConst0();
    if ( Abc_SopIsConst1(pSop) )
        return Dec_GraphCreateConst1();

    // derive the cover from the SOP representation
    pCover = Dec_ConvertSopToMvc( pSop );

    // make sure the cover is CCS free (should be done before CST)
    Mvc_CoverContain( pCover );

    // check for trivial functions
    assert( !Mvc_CoverIsEmpty(pCover) );
    assert( !Mvc_CoverIsTautology(pCover) );

    // perform CST
    Mvc_CoverInverse( pCover ); // CST
    // start the factored form
    pFForm = Dec_GraphCreate( Abc_SopGetVarNum(pSop) );
    // factor the cover
    eRoot = Dec_Factor_rec( pFForm, pCover );
    // finalize the factored form
    Dec_GraphSetRoot( pFForm, eRoot );
    // complement the factored form if SOP is complemented
    if ( Abc_SopIsComplement(pSop) )
        Dec_GraphComplement( pFForm );
    // verify the factored form
//    if ( !Dec_FactorVerify( pSop, pFForm ) )
//        printf( "Verification has failed.\n" );
//    Mvc_CoverInverse( pCover ); // undo CST
    Mvc_CoverFree( pCover );
    return pFForm;
}


Hop_Obj_t * Dec_GraphToNetworkAig( Hop_Man_t * pMan, Dec_Graph_t * pGraph )
{
    Dec_Node_t * pNode = NULL; // Suppress "might be used uninitialized"
    Hop_Obj_t * pAnd0, * pAnd1;
    int i;
    // check for constant function
    if ( Dec_GraphIsConst(pGraph) )
        return Hop_NotCond( Hop_ManConst1(pMan), Dec_GraphIsComplement(pGraph) );
    // check for a literal
    if ( Dec_GraphIsVar(pGraph) )
        return Hop_NotCond( (Hop_Obj_t *)Dec_GraphVar(pGraph)->pFunc, Dec_GraphIsComplement(pGraph) );
    // build the AIG nodes corresponding to the AND gates of the graph
    Dec_GraphForEachNode( pGraph, pNode, i )
    {
        pAnd0 = Hop_NotCond( (Hop_Obj_t *)Dec_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc, pNode->eEdge0.fCompl );
        pAnd1 = Hop_NotCond( (Hop_Obj_t *)Dec_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc, pNode->eEdge1.fCompl );
        pNode->pFunc = Hop_And( pMan, pAnd0, pAnd1 );
    }
    // complement the result if necessary
    return Hop_NotCond( (Hop_Obj_t *)pNode->pFunc, Dec_GraphIsComplement(pGraph) );
}


void Dec_GraphFree( Dec_Graph_t * pGraph )
{
    ABC_FREE( pGraph->pNodes );
    ABC_FREE( pGraph );
}


Mvc_Cover_t * Dec_ConvertSopToMvc( char * pSop )
{
    Dec_Man_t * pManDec = (Dec_Man_t *)Abc_FrameReadManDec();
    Mvc_Manager_t * pMem = (Mvc_Manager_t *)pManDec->pMvcMem;
    Mvc_Cover_t * pMvc;
    Mvc_Cube_t * pMvcCube;
    char * pCube;
    int nVars, Value, v;

    // start the cover
    nVars = Abc_SopGetVarNum(pSop);
    assert( nVars > 0 );
    pMvc = Mvc_CoverAlloc( pMem, nVars * 2 );
    // check the logic function of the node
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        // create and add the cube
        pMvcCube = Mvc_CubeAlloc( pMvc );
        Mvc_CoverAddCubeTail( pMvc, pMvcCube );
        // fill in the literals
        Mvc_CubeBitFill( pMvcCube );
        Abc_CubeForEachVar( pCube, Value, v )
        {
            if ( Value == '0' )
                Mvc_CubeBitRemove( pMvcCube, v * 2 + 1 );
            else if ( Value == '1' )
                Mvc_CubeBitRemove( pMvcCube, v * 2 );
        }
    }
    return pMvc;
}


Dec_Man_t * Dec_ManStart()
{
    Dec_Man_t * p;
    p = ABC_ALLOC( Dec_Man_t, 1 );
    p->pMvcMem = Mvc_ManagerStart();
    p->vCubes = Vec_IntAlloc( 8 );
    p->vLits = Vec_IntAlloc( 8 );
    // canonical forms, phases, perms
    Extra_Truth4VarNPN( &p->puCanons, &p->pPhases, &p->pPerms, &p->pMap );
    return p;
}


void Dec_GraphUpdateNetwork( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int fUpdateLevel, int nGain )
{
    // extern Abc_Obj_t *    Dec_GraphToNetwork( Abc_Ntk_t * pNtk, Dec_Graph_t * pGraph );
    Abc_Obj_t * pRootNew;
    Abc_Ntk_t * pNtk = pRoot->pNtk;
    // int nNodesNew, nNodesOld;
    // nNodesOld = Abc_NtkNodeNum(pNtk);
    // create the new structure of nodes
    pRootNew = Dec_GraphToNetwork( pNtk, pGraph );
    // remove the old nodes
    Abc_AigReplace( (Abc_Aig_t *)pNtk->pManFunc, pRoot, pRootNew, fUpdateLevel );
    // compare the gains
    // nNodesNew = Abc_NtkNodeNum(pNtk);
    //assert( nGain <= nNodesOld - nNodesNew );
}


void Dec_GraphPrint( FILE * pFile, Dec_Graph_t * pGraph, char * pNamesIn[], char * pNameOut )
{
    Vec_Ptr_t * vNamesIn = NULL;
    int LitSizeMax, LitSizeCur, Pos, i;

    // create the names if not given by the user
    if ( pNamesIn == NULL )
    {
        vNamesIn = Abc_NodeGetFakeNames( Dec_GraphLeaveNum(pGraph) );
        pNamesIn = (char **)vNamesIn->pArray;
    }
    if ( pNameOut == NULL )
        pNameOut = const_cast <char *>( "F" );

    // get the size of the longest literal
    LitSizeMax = 0;
    for ( i = 0; i < Dec_GraphLeaveNum(pGraph); i++ )
    {
        LitSizeCur = strlen(pNamesIn[i]);
        if ( LitSizeMax < LitSizeCur )
            LitSizeMax = LitSizeCur;
    }
    if ( LitSizeMax > 50 )
        LitSizeMax = 20;

    // write the decomposition graph (factored form)
    if ( Dec_GraphIsConst(pGraph) ) // constant
    {
        Pos = Dec_GraphPrintOutputName( pFile, pNameOut );
        fprintf( pFile, "Constant %d", !Dec_GraphIsComplement(pGraph) );
    }
    else if ( Dec_GraphIsVar(pGraph) ) // literal
    {
        Pos = Dec_GraphPrintOutputName( pFile, pNameOut );
        Dec_GraphPrintGetLeafName( pFile, Dec_GraphVarInt(pGraph), Dec_GraphIsComplement(pGraph), pNamesIn );
    }
    else
    {
        Pos = Dec_GraphPrintOutputName( pFile, pNameOut );
        Dec_GraphPrint_rec( pFile, pGraph, Dec_GraphNodeLast(pGraph), Dec_GraphIsComplement(pGraph), pNamesIn, &Pos, LitSizeMax );
    }
    fprintf( pFile, "\n" );

    if ( vNamesIn )
        Abc_NodeFreeNames( vNamesIn );
}


unsigned Dec_GraphDeriveTruth( Dec_Graph_t * pGraph )
{
    unsigned uTruths[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    unsigned uTruth = 0; // Suppress "might be used uninitialized"
    unsigned uTruth0, uTruth1;
    Dec_Node_t * pNode;
    int i;

    // sanity checks
    assert( Dec_GraphLeaveNum(pGraph) >= 0 );
    assert( Dec_GraphLeaveNum(pGraph) <= pGraph->nSize );
    assert( Dec_GraphLeaveNum(pGraph) <= 5 );

    // check for constant function
    if ( Dec_GraphIsConst(pGraph) )
        return Dec_GraphIsComplement(pGraph)? 0 : ~((unsigned)0);
    // check for a literal
    if ( Dec_GraphIsVar(pGraph) )
        return Dec_GraphIsComplement(pGraph)? ~uTruths[Dec_GraphVarInt(pGraph)] : uTruths[Dec_GraphVarInt(pGraph)];

    // assign the elementary variables
    Dec_GraphForEachLeaf( pGraph, pNode, i )
        pNode->pFunc = (void *)(ABC_PTRUINT_T)uTruths[i];

    // compute the function for each internal node
    Dec_GraphForEachNode( pGraph, pNode, i )
    {
        uTruth0 = (unsigned)(ABC_PTRUINT_T)Dec_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc;
        uTruth1 = (unsigned)(ABC_PTRUINT_T)Dec_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc;
        uTruth0 = pNode->eEdge0.fCompl? ~uTruth0 : uTruth0;
        uTruth1 = pNode->eEdge1.fCompl? ~uTruth1 : uTruth1;
        uTruth = uTruth0 & uTruth1;
        pNode->pFunc = (void *)(ABC_PTRUINT_T)uTruth;
    }

    // complement the result if necessary
    return Dec_GraphIsComplement(pGraph)? ~uTruth : uTruth;
}


Dec_Edge_t Dec_Factor_rec( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pDiv, * pQuo, * pRem, * pCom;
    Dec_Edge_t eNodeDiv, eNodeQuo, eNodeRem;
    Dec_Edge_t eNodeAnd, eNode;

    // make sure the cover contains some cubes
    assert( Mvc_CoverReadCubeNum(pCover) );

    // get the divisor
    pDiv = Mvc_CoverDivisor( pCover );
    if ( pDiv == NULL )
        return Dec_FactorTrivial( pFForm, pCover );

    // divide the cover by the divisor
    Mvc_CoverDivideInternal( pCover, pDiv, &pQuo, &pRem );
    assert( Mvc_CoverReadCubeNum(pQuo) );

    Mvc_CoverFree( pDiv );
    Mvc_CoverFree( pRem );

    // check the trivial case
    if ( Mvc_CoverReadCubeNum(pQuo) == 1 )
    {
        eNode = Dec_FactorLF_rec( pFForm, pCover, pQuo );
        Mvc_CoverFree( pQuo );
        return eNode;
    }

    // make the quotient cube ABC_FREE
    Mvc_CoverMakeCubeFree( pQuo );

    // divide the cover by the quotient
    Mvc_CoverDivideInternal( pCover, pQuo, &pDiv, &pRem );

    // check the trivial case
    if ( Mvc_CoverIsCubeFree( pDiv ) )
    {
        eNodeDiv = Dec_Factor_rec( pFForm, pDiv );
        eNodeQuo = Dec_Factor_rec( pFForm, pQuo );
        Mvc_CoverFree( pDiv );
        Mvc_CoverFree( pQuo );
        eNodeAnd = Dec_GraphAddNodeAnd( pFForm, eNodeDiv, eNodeQuo );
        if ( Mvc_CoverReadCubeNum(pRem) == 0 )
        {
            Mvc_CoverFree( pRem );
            return eNodeAnd;
        }
        else
        {
            eNodeRem = Dec_Factor_rec( pFForm, pRem );
            Mvc_CoverFree( pRem );
            return Dec_GraphAddNodeOr( pFForm, eNodeAnd, eNodeRem );
        }
    }

    // get the common cube
    pCom = Mvc_CoverCommonCubeCover( pDiv );
    Mvc_CoverFree( pDiv );
    Mvc_CoverFree( pQuo );
    Mvc_CoverFree( pRem );

    // solve the simple problem
    eNode = Dec_FactorLF_rec( pFForm, pCover, pCom );
    Mvc_CoverFree( pCom );
    return eNode;
}


int Dec_GraphPrintOutputName( FILE * pFile, char * pNameOut )
{
    if ( pNameOut == NULL )
        return 0;
    fprintf( pFile, "%6s = ", pNameOut );
    return 10;
}


int Dec_GraphPrintGetLeafName( FILE * pFile, int iLeaf, int fCompl, char * pNamesIn[] )
{
    static char Buffer[100];
    sprintf( Buffer, "%s%s", fCompl? "!" : "", pNamesIn[iLeaf] );
    fprintf( pFile, "%s", Buffer );
    return strlen( Buffer );
}


void Dec_GraphPrint_rec( FILE * pFile, Dec_Graph_t * pGraph, Dec_Node_t * pNode, int fCompl, char * pNamesIn[], int * pPos, int LitSizeMax )
{
    Dec_Node_t * pNode0, * pNode1;
    Dec_Node_t * pNode00, * pNode01, * pNode10, * pNode11;
    pNode0 = Dec_GraphNode(pGraph, pNode->eEdge0.Node);
    pNode1 = Dec_GraphNode(pGraph, pNode->eEdge1.Node);
    if ( Dec_GraphNodeIsVar(pGraph, pNode) ) // FT_NODE_LEAF )
    {
        (*pPos) += Dec_GraphPrintGetLeafName( pFile, Dec_GraphNodeInt(pGraph,pNode), fCompl, pNamesIn );
        return;
    }
    if ( !Dec_GraphNodeIsVar(pGraph, pNode0) && !Dec_GraphNodeIsVar(pGraph, pNode1) )
    {
        pNode00 = Dec_GraphNode(pGraph, pNode0->eEdge0.Node);
        pNode01 = Dec_GraphNode(pGraph, pNode0->eEdge1.Node);
        pNode10 = Dec_GraphNode(pGraph, pNode1->eEdge0.Node);
        pNode11 = Dec_GraphNode(pGraph, pNode1->eEdge1.Node);
        if ( (pNode00 == pNode10 || pNode00 == pNode11) && (pNode01 == pNode10 || pNode01 == pNode11) )
        {
            fprintf( pFile, "(" );
            (*pPos)++;
            Dec_GraphPrint_rec( pFile, pGraph, pNode00, pNode00->fCompl0, pNamesIn, pPos, LitSizeMax );
            fprintf( pFile, " # " );
            (*pPos) += 3;
            Dec_GraphPrint_rec( pFile, pGraph, pNode01, pNode01->fCompl1, pNamesIn, pPos, LitSizeMax );
            fprintf( pFile, ")" );
            (*pPos)++;
            return;
        }
    }
    if ( fCompl )
    {
        fprintf( pFile, "(" );
        (*pPos)++;
        Dec_GraphPrint_rec( pFile, pGraph, pNode0, !pNode->eEdge0.fCompl, pNamesIn, pPos, LitSizeMax );
        fprintf( pFile, " + " );
        (*pPos) += 3;
        Dec_GraphPrint_rec( pFile, pGraph, pNode1, !pNode->eEdge1.fCompl, pNamesIn, pPos, LitSizeMax );
        fprintf( pFile, ")" );
        (*pPos)++;
    }
    else
    {
        fprintf( pFile, "(" );
        (*pPos)++;
        Dec_GraphPrint_rec( pFile, pGraph, pNode0, pNode->eEdge0.fCompl, pNamesIn, pPos, LitSizeMax );
        Dec_GraphPrint_rec( pFile, pGraph, pNode1, pNode->eEdge1.fCompl, pNamesIn, pPos, LitSizeMax );
        fprintf( pFile, ")" );
        (*pPos)++;
    }
}


Dec_Edge_t Dec_FactorTrivial( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover )
{
    Dec_Man_t * pManDec = (Dec_Man_t *)Abc_FrameReadManDec();
    Vec_Int_t * vEdgeCubes = pManDec->vCubes;
    Vec_Int_t * vEdgeLits  = pManDec->vLits;
    Dec_Edge_t eNode;
    Mvc_Cube_t * pCube;
    // create the factored form for each cube
    Vec_IntClear( vEdgeCubes );
    Mvc_CoverForEachCube( pCover, pCube )
    {
        eNode = Dec_FactorTrivialCube( pFForm, pCover, pCube, vEdgeLits );
        Vec_IntPush( vEdgeCubes, Dec_EdgeToInt_(eNode) );
    }
    // balance the factored forms
    return Dec_FactorTrivialTree_rec( pFForm, (Dec_Edge_t *)vEdgeCubes->pArray, vEdgeCubes->nSize, 1 );
}


Dec_Edge_t Dec_FactorLF_rec( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover, Mvc_Cover_t * pSimple )
{
    Dec_Man_t * pManDec = (Dec_Man_t *)Abc_FrameReadManDec();
    Vec_Int_t * vEdgeLits  = pManDec->vLits;
    Mvc_Cover_t * pDiv, * pQuo, * pRem;
    Dec_Edge_t eNodeDiv, eNodeQuo, eNodeRem;
    Dec_Edge_t eNodeAnd;

    // get the most often occurring literal
    pDiv = Mvc_CoverBestLiteralCover( pCover, pSimple );
    // divide the cover by the literal
    Mvc_CoverDivideByLiteral( pCover, pDiv, &pQuo, &pRem );
    // get the node pointer for the literal
    eNodeDiv = Dec_FactorTrivialCube( pFForm, pDiv, Mvc_CoverReadCubeHead(pDiv), vEdgeLits );
    Mvc_CoverFree( pDiv );
    // factor the quotient and remainder
    eNodeQuo = Dec_Factor_rec( pFForm, pQuo );
    Mvc_CoverFree( pQuo );
    eNodeAnd = Dec_GraphAddNodeAnd( pFForm, eNodeDiv, eNodeQuo );
    if ( Mvc_CoverReadCubeNum(pRem) == 0 )
    {
        Mvc_CoverFree( pRem );
        return eNodeAnd;
    }
    else
    {
        eNodeRem = Dec_Factor_rec( pFForm, pRem );
        Mvc_CoverFree( pRem );
        return Dec_GraphAddNodeOr( pFForm,  eNodeAnd, eNodeRem );
    }
}


Dec_Edge_t Dec_FactorTrivialCube( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover, Mvc_Cube_t * pCube, Vec_Int_t * vEdgeLits )
{
    Dec_Edge_t eNode;
    int iBit, Value;
    // create the factored form for each literal
    Vec_IntClear( vEdgeLits );
    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
        if ( Value )
        {
            eNode = Dec_EdgeCreate( iBit/2, iBit%2 ); // CST
            Vec_IntPush( vEdgeLits, Dec_EdgeToInt_(eNode) );
        }
    // balance the factored forms
    return Dec_FactorTrivialTree_rec( pFForm, (Dec_Edge_t *)vEdgeLits->pArray, vEdgeLits->nSize, 0 );
}


Dec_Edge_t Dec_FactorTrivialTree_rec( Dec_Graph_t * pFForm, Dec_Edge_t * peNodes, int nNodes, int fNodeOr )
{
    Dec_Edge_t eNode1, eNode2;
    int nNodes1, nNodes2;

    if ( nNodes == 1 )
        return peNodes[0];

    // split the nodes into two parts
    nNodes1 = nNodes/2;
    nNodes2 = nNodes - nNodes1;
//    nNodes2 = nNodes/2;
//    nNodes1 = nNodes - nNodes2;

    // recursively construct the tree for the parts
    eNode1 = Dec_FactorTrivialTree_rec( pFForm, peNodes,           nNodes1, fNodeOr );
    eNode2 = Dec_FactorTrivialTree_rec( pFForm, peNodes + nNodes1, nNodes2, fNodeOr );

    if ( fNodeOr )
        return Dec_GraphAddNodeOr( pFForm, eNode1, eNode2 );
    else
        return Dec_GraphAddNodeAnd( pFForm, eNode1, eNode2 );
}


int Dec_GraphToNetworkCount( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int NodeMax, int LevelMax )
{
    Abc_Aig_t * pMan = (Abc_Aig_t *)pRoot->pNtk->pManFunc;
    Dec_Node_t * pNode, * pNode0, * pNode1;
    Abc_Obj_t * pAnd, * pAnd0, * pAnd1;
    int i, Counter, LevelNew;//, LevelOld;
    // check for constant function or a literal
    if ( Dec_GraphIsConst(pGraph) || Dec_GraphIsVar(pGraph) )
        return 0;
    // set the levels of the leaves
    Dec_GraphForEachLeaf( pGraph, pNode, i )
        pNode->Level = Abc_ObjRegular((Abc_Obj_t *)pNode->pFunc)->Level;
    // compute the AIG size after adding the internal nodes
    Counter = 0;
    Dec_GraphForEachNode( pGraph, pNode, i )
    {
        // get the children of this node
        pNode0 = Dec_GraphNode( pGraph, pNode->eEdge0.Node );
        pNode1 = Dec_GraphNode( pGraph, pNode->eEdge1.Node );
        // get the AIG nodes corresponding to the children
        pAnd0 = (Abc_Obj_t *)pNode0->pFunc;
        pAnd1 = (Abc_Obj_t *)pNode1->pFunc;
        if ( pAnd0 && pAnd1 )
        {
            // if they are both present, find the resulting node
            pAnd0 = Abc_ObjNotCond( pAnd0, pNode->eEdge0.fCompl );
            pAnd1 = Abc_ObjNotCond( pAnd1, pNode->eEdge1.fCompl );
            pAnd  = Abc_AigAndLookup( pMan, pAnd0, pAnd1 );
            // return -1 if the node is the same as the original root
            if ( Abc_ObjRegular(pAnd) == pRoot )
                return -1;
        }
        else
            pAnd = NULL;
        // count the number of added nodes
        if ( pAnd == NULL || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pAnd)) )
        {
            if ( ++Counter > NodeMax )
                return -1;
        }
        // count the number of new levels
        LevelNew = 1 + Abc_MaxInt( pNode0->Level, pNode1->Level );
        if ( pAnd )
        {
            if ( Abc_ObjRegular(pAnd) == Abc_AigConst1(pRoot->pNtk) )
                LevelNew = 0;
            else if ( Abc_ObjRegular(pAnd) == Abc_ObjRegular(pAnd0) )
                LevelNew = (int)Abc_ObjRegular(pAnd0)->Level;
            else if ( Abc_ObjRegular(pAnd) == Abc_ObjRegular(pAnd1) )
                LevelNew = (int)Abc_ObjRegular(pAnd1)->Level;
            // LevelOld = (int)Abc_ObjRegular(pAnd)->Level;
//            assert( LevelNew == LevelOld );
        }
        if ( LevelNew > LevelMax )
            return -1;
        pNode->pFunc = pAnd;
        pNode->Level = LevelNew;
    }
    return Counter;
}


Abc_Obj_t * Dec_GraphToNetwork( Abc_Ntk_t * pNtk, Dec_Graph_t * pGraph )
{
    Abc_Obj_t * pAnd0, * pAnd1;
    Dec_Node_t * pNode = NULL; // Suppress "might be used uninitialized"
    int i;
    // check for constant function
    if ( Dec_GraphIsConst(pGraph) )
        return Abc_ObjNotCond( Abc_AigConst1(pNtk), Dec_GraphIsComplement(pGraph) );
    // check for a literal
    if ( Dec_GraphIsVar(pGraph) )
        return Abc_ObjNotCond( (Abc_Obj_t *)Dec_GraphVar(pGraph)->pFunc, Dec_GraphIsComplement(pGraph) );
    // build the AIG nodes corresponding to the AND gates of the graph
    Dec_GraphForEachNode( pGraph, pNode, i )
    {
        pAnd0 = Abc_ObjNotCond( (Abc_Obj_t *)Dec_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc, pNode->eEdge0.fCompl );
        pAnd1 = Abc_ObjNotCond( (Abc_Obj_t *)Dec_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc, pNode->eEdge1.fCompl );
        pNode->pFunc = Abc_AigAnd( (Abc_Aig_t *)pNtk->pManFunc, pAnd0, pAnd1 );
    }
    // complement the result if necessary
    return Abc_ObjNotCond( (Abc_Obj_t *)pNode->pFunc, Dec_GraphIsComplement(pGraph) );
}


void Dec_ManStop( Dec_Man_t * p )
{
    Mvc_ManagerFree( (Mvc_Manager_t *)p->pMvcMem );
    Vec_IntFree( p->vCubes );
    Vec_IntFree( p->vLits );
    ABC_FREE( p->puCanons );
    ABC_FREE( p->pPhases );
    ABC_FREE( p->pPerms );
    ABC_FREE( p->pMap );
    ABC_FREE( p );
}
