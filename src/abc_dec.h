#ifndef ABC_DEC_H
#define ABC_DEC_H


#include "abc_global.h"
#include "abc_vec.h"
#include "abc_memory.h"
#include "abc_hop.h"
#include "abc_ntk.h"
#include "abc_extra.h"
#include "abc_mvc.h"
#include "abc_frame.h"


// interator throught the leaves
#define Dec_GraphForEachLeaf( pGraph, pLeaf, i )                                              \
    for ( i = 0; (i < (pGraph)->nLeaves) && (((pLeaf) = Dec_GraphNode(pGraph, i)), 1); i++ )
// interator throught the internal nodes
#define Dec_GraphForEachNode( pGraph, pAnd, i )                                               \
    for ( i = (pGraph)->nLeaves; (i < (pGraph)->nSize) && (((pAnd) = Dec_GraphNode(pGraph, i)), 1); i++ )


typedef struct Dec_Edge_t_ Dec_Edge_t;
typedef struct Dec_Node_t_ Dec_Node_t;
typedef struct Dec_Graph_t_ Dec_Graph_t;
typedef struct Dec_Man_t_ Dec_Man_t;


struct Dec_Edge_t_
{
    unsigned          fCompl   :  1;   // the complemented bit
    unsigned          Node     : 30;   // the decomposition node pointed by the edge
};


struct Dec_Node_t_
{
    Dec_Edge_t        eEdge0;          // the left child of the node
    Dec_Edge_t        eEdge1;          // the right child of the node
    // other info
    union { int       iFunc;           // the literal of the node (AIG)
    void *            pFunc; };        // the function of the node (BDD or AIG)
    unsigned          Level    : 14;   // the level of this node in the global AIG
    // printing info
    unsigned          fNodeOr  :  1;   // marks the original OR node
    unsigned          fCompl0  :  1;   // marks the original complemented edge
    unsigned          fCompl1  :  1;   // marks the original complemented edge
    // latch info
    unsigned          nLat0    :  5;   // the number of latches on the first edge
    unsigned          nLat1    :  5;   // the number of latches on the second edge
    unsigned          nLat2    :  5;   // the number of latches on the output edge
};


struct Dec_Graph_t_
{
    int               fConst;          // marks the constant 1 graph
    int               nLeaves;         // the number of leaves
    int               nSize;           // the number of nodes (including the leaves)
    int               nCap;            // the number of allocated nodes
    Dec_Node_t *      pNodes;          // the array of leaves and internal nodes
    Dec_Edge_t        eRoot;           // the pointer to the topmost node
};


struct Dec_Man_t_
{
    void *            pMvcMem;         // memory manager for MVC cover (used for factoring)
    Vec_Int_t *       vCubes;          // storage for cubes
    Vec_Int_t *       vLits;           // storage for literals
    // precomputation information about 4-variable functions
    unsigned short *  puCanons;        // canonical forms
    char *            pPhases;         // canonical phases
    char *            pPerms;          // canonical permutations
    unsigned char *   pMap;            // mapping of functions into class numbers
};


static inline Dec_Node_t * Dec_GraphNode( Dec_Graph_t * pGraph, int i )
{
    return pGraph->pNodes + i;
}


static inline void Dec_GraphComplement( Dec_Graph_t * pGraph )
{
    pGraph->eRoot.fCompl ^= 1;
}


static inline int Dec_GraphNodeNum( Dec_Graph_t * pGraph )
{
    return pGraph->nSize - pGraph->nLeaves;
}


static inline Dec_Graph_t * Dec_GraphCreateConst0()
{
    Dec_Graph_t * pGraph;
    pGraph = ABC_ALLOC( Dec_Graph_t, 1 );
    memset( pGraph, 0, sizeof(Dec_Graph_t) );
    pGraph->fConst = 1;
    pGraph->eRoot.fCompl = 1;
    return pGraph;
}


static inline Dec_Graph_t * Dec_GraphCreateConst1()
{
    Dec_Graph_t * pGraph;
    pGraph = ABC_ALLOC( Dec_Graph_t, 1 );
    memset( pGraph, 0, sizeof(Dec_Graph_t) );
    pGraph->fConst = 1;
    return pGraph;
}


static inline Dec_Graph_t * Dec_GraphCreate( int nLeaves )
{
    Dec_Graph_t * pGraph;
    pGraph = ABC_ALLOC( Dec_Graph_t, 1 );
    memset( pGraph, 0, sizeof(Dec_Graph_t) );
    pGraph->nLeaves = nLeaves;
    pGraph->nSize = nLeaves;
    pGraph->nCap = 2 * nLeaves + 50;
    pGraph->pNodes = ABC_ALLOC( Dec_Node_t, pGraph->nCap );
    memset( pGraph->pNodes, 0, sizeof(Dec_Node_t) * pGraph->nSize );
    return pGraph;
}


static inline Dec_Graph_t * Dec_GraphCreateLeaf( int iLeaf, int nLeaves, int fCompl )
{
    Dec_Graph_t * pGraph;
    assert( 0 <= iLeaf && iLeaf < nLeaves );
    pGraph = Dec_GraphCreate( nLeaves );
    pGraph->eRoot.Node   = iLeaf;
    pGraph->eRoot.fCompl = fCompl;
    return pGraph;
}



static inline void Dec_GraphSetRoot( Dec_Graph_t * pGraph, Dec_Edge_t eRoot )
{
    pGraph->eRoot = eRoot;
}


static inline Dec_Edge_t Dec_EdgeCreate( int Node, int fCompl )
{
    Dec_Edge_t eEdge = { (unsigned)fCompl, (unsigned)Node };
    return eEdge;
}


static inline Dec_Edge_t Dec_IntToEdge( unsigned Edge )
{
    return Dec_EdgeCreate( Edge >> 1, Edge & 1 );
}


static inline Dec_Node_t * Dec_GraphAppendNode( Dec_Graph_t * pGraph )
{
    Dec_Node_t * pNode;
    if ( pGraph->nSize == pGraph->nCap )
    {
        pGraph->pNodes = ABC_REALLOC( Dec_Node_t, pGraph->pNodes, 2 * pGraph->nCap );
        pGraph->nCap   = 2 * pGraph->nCap;
    }
    pNode = pGraph->pNodes + pGraph->nSize++;
    memset( pNode, 0, sizeof(Dec_Node_t) );
    return pNode;
}


static inline Dec_Edge_t Dec_GraphAddNodeAnd( Dec_Graph_t * pGraph, Dec_Edge_t eEdge0, Dec_Edge_t eEdge1 )
{
    Dec_Node_t * pNode;
    // get the new node
    pNode = Dec_GraphAppendNode( pGraph );
    // set the inputs and other info
    pNode->eEdge0 = eEdge0;
    pNode->eEdge1 = eEdge1;
    pNode->fCompl0 = eEdge0.fCompl;
    pNode->fCompl1 = eEdge1.fCompl;
    return Dec_EdgeCreate( pGraph->nSize - 1, 0 );
}


static inline Dec_Edge_t Dec_GraphAddNodeOr( Dec_Graph_t * pGraph, Dec_Edge_t eEdge0, Dec_Edge_t eEdge1 )
{
    Dec_Node_t * pNode;
    // get the new node
    pNode = Dec_GraphAppendNode( pGraph );
    // set the inputs and other info
    pNode->eEdge0 = eEdge0;
    pNode->eEdge1 = eEdge1;
    pNode->fCompl0 = eEdge0.fCompl;
    pNode->fCompl1 = eEdge1.fCompl;
    // make adjustments for the OR gate
    pNode->fNodeOr = 1;
    pNode->eEdge0.fCompl = !pNode->eEdge0.fCompl;
    pNode->eEdge1.fCompl = !pNode->eEdge1.fCompl;
    return Dec_EdgeCreate( pGraph->nSize - 1, 1 );
}


static inline Dec_Edge_t Dec_GraphAddNodeXor( Dec_Graph_t * pGraph, Dec_Edge_t eEdge0, Dec_Edge_t eEdge1, int Type )
{
    Dec_Edge_t eNode0, eNode1, eNode;
    if ( Type == 0 )
    {
        // derive the first AND
        eEdge0.fCompl ^= 1;
        eNode0 = Dec_GraphAddNodeAnd( pGraph, eEdge0, eEdge1 );
        eEdge0.fCompl ^= 1;
        // derive the second AND
        eEdge1.fCompl ^= 1;
        eNode1 = Dec_GraphAddNodeAnd( pGraph, eEdge0, eEdge1 );
        // derive the final OR
        eNode = Dec_GraphAddNodeOr( pGraph, eNode0, eNode1 );
    }
    else
    {
        // derive the first AND
        eNode0 = Dec_GraphAddNodeAnd( pGraph, eEdge0, eEdge1 );
        // derive the second AND
        eEdge0.fCompl ^= 1;
        eEdge1.fCompl ^= 1;
        eNode1 = Dec_GraphAddNodeAnd( pGraph, eEdge0, eEdge1 );
        // derive the final OR
        eNode = Dec_GraphAddNodeOr( pGraph, eNode0, eNode1 );
        eNode.fCompl ^= 1;
    }
    return eNode;
}


static inline unsigned Dec_EdgeToInt( Dec_Edge_t eEdge )
{
    return (eEdge.Node << 1) | eEdge.fCompl;
}


static inline int Dec_GraphIsConst( Dec_Graph_t * pGraph )
{
    return pGraph->fConst;
}


static inline int Dec_GraphIsComplement( Dec_Graph_t * pGraph )
{
    return pGraph->eRoot.fCompl;
}


static inline int Dec_GraphIsVar( Dec_Graph_t * pGraph )
{
    return pGraph->eRoot.Node < (unsigned)pGraph->nLeaves;
}


static inline Dec_Node_t * Dec_GraphVar( Dec_Graph_t * pGraph )
{
    assert( Dec_GraphIsVar( pGraph ) );
    return Dec_GraphNode( pGraph, pGraph->eRoot.Node );
}


static inline int Dec_GraphLeaveNum( Dec_Graph_t * pGraph )
{
    return pGraph->nLeaves;
}


static inline int Dec_GraphNodeInt( Dec_Graph_t * pGraph, Dec_Node_t * pNode )
{
    return pNode - pGraph->pNodes;
}


static inline int Dec_GraphVarInt( Dec_Graph_t * pGraph )
{
    assert( Dec_GraphIsVar( pGraph ) );
    return Dec_GraphNodeInt( pGraph, Dec_GraphVar(pGraph) );
}


static inline Dec_Node_t * Dec_GraphNodeLast( Dec_Graph_t * pGraph )
{
    return pGraph->pNodes + pGraph->nSize - 1;
}


static inline int Dec_GraphNodeIsVar( Dec_Graph_t * pGraph, Dec_Node_t * pNode )
{
    return Dec_GraphNodeInt(pGraph,pNode) < pGraph->nLeaves;
}


static inline unsigned    Dec_EdgeToInt_( Dec_Edge_t m )  { union { Dec_Edge_t x; unsigned y; } v; v.x = m; return v.y;  }


Hop_Obj_t * Dec_GraphFactorSop( Hop_Man_t * pMan, char * pSop );
Dec_Graph_t * Dec_Factor( char * pSop );
Hop_Obj_t * Dec_GraphToNetworkAig( Hop_Man_t * pMan, Dec_Graph_t * pGraph );
void Dec_GraphFree( Dec_Graph_t * pGraph );
Mvc_Cover_t * Dec_ConvertSopToMvc( char * pSop );
Dec_Man_t * Dec_ManStart();
void Dec_GraphUpdateNetwork( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int fUpdateLevel, int nGain );
void Dec_GraphPrint( FILE * pFile, Dec_Graph_t * pGraph, char * pNamesIn[], char * pNameOut );
unsigned Dec_GraphDeriveTruth( Dec_Graph_t * pGraph );
Dec_Edge_t Dec_Factor_rec( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover );
int Dec_GraphPrintOutputName( FILE * pFile, char * pNameOut );
int Dec_GraphPrintGetLeafName( FILE * pFile, int iLeaf, int fCompl, char * pNamesIn[] );
void Dec_GraphPrint_rec( FILE * pFile, Dec_Graph_t * pGraph, Dec_Node_t * pNode, int fCompl, char * pNamesIn[], int * pPos, int LitSizeMax );
Dec_Edge_t Dec_FactorTrivial( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover );
Dec_Edge_t Dec_FactorLF_rec( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover, Mvc_Cover_t * pSimple );
Dec_Edge_t Dec_FactorTrivialCube( Dec_Graph_t * pFForm, Mvc_Cover_t * pCover, Mvc_Cube_t * pCube, Vec_Int_t * vEdgeLits );
Dec_Edge_t Dec_FactorTrivialTree_rec( Dec_Graph_t * pFForm, Dec_Edge_t * peNodes, int nNodes, int fNodeOr );
int Dec_GraphToNetworkCount( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int NodeMax, int LevelMax );
Abc_Obj_t * Dec_GraphToNetwork( Abc_Ntk_t * pNtk, Dec_Graph_t * pGraph );
void Dec_ManStop( Dec_Man_t * p );


#endif
