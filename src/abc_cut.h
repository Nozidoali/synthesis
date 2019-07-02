#ifndef ABC_CUT_H
#define ABC_CUT_H


#include "abc_ntk.h"
#include "abc_extra.h"


#define CUT_SIZE_MIN    3      // the min K of the K-feasible cut computation
#define CUT_SIZE_MAX   12      // the max K of the K-feasible cut computation

#define CUT_SHIFT       8      // the number of bits for storing latch number in the cut leaves
#define CUT_MASK        0xFF   // the mask to get the stored latch number


// iterator through all the cuts of the list
#define Cut_ListForEachCut( pList, pCut )                 \
    for ( pCut = pList;                                   \
          pCut;                                           \
          pCut = pCut->pNext )
#define Cut_ListForEachCutStop( pList, pCut, pStop )      \
    for ( pCut = pList;                                   \
          pCut != pStop;                                  \
          pCut = pCut->pNext )
#define Cut_ListForEachCutSafe( pList, pCut, pCut2 )      \
    for ( pCut = pList,                                   \
          pCut2 = pCut? pCut->pNext: NULL;                \
          pCut;                                           \
          pCut = pCut2,                                   \
          pCut2 = pCut? pCut->pNext: NULL )


typedef struct Abc_Obj_t_       Abc_Obj_t;
typedef struct Abc_Ntk_t_       Abc_Ntk_t;
typedef struct Cut_ManStruct_t_         Cut_Man_t;
typedef struct Cut_CutStruct_t_         Cut_Cut_t;
typedef struct Cut_ParamsStruct_t_      Cut_Params_t;
typedef struct Cut_ListStruct_t_         Cut_List_t;


struct Cut_ManStruct_t_
{
    // user preferences
    Cut_Params_t *     pParams;          // computation parameters
    Vec_Int_t *        vFanCounts;       // the array of fanout counters
    Vec_Int_t *        vNodeAttrs;       // node attributes (1 = global; 0 = local)
    // storage for cuts
    Vec_Ptr_t *        vCutsNew;         // new cuts by node ID
    Vec_Ptr_t *        vCutsOld;         // old cuts by node ID
    Vec_Ptr_t *        vCutsTemp;        // temp cuts for cutset nodes by cutset node number
    // memory management
    Extra_MmFixed_t *  pMmCuts;
    int                EntrySize;
    int                nTruthWords;
    // temporary variables
    Cut_Cut_t *        pReady;
    Vec_Ptr_t *        vTemp;
    int                fCompl0;
    int                fCompl1;
    int                fSimul;
    int                nNodeCuts;
    Cut_Cut_t *        pStore0[2];
    Cut_Cut_t *        pStore1[2];
    Cut_Cut_t *        pCompareOld;
    Cut_Cut_t *        pCompareNew;
    unsigned *         puTemp[4];
    // record of the cut computation
    Vec_Int_t *        vNodeCuts;        // the number of cuts for each node
    Vec_Int_t *        vNodeStarts;      // the number of the starting cut of each node
    Vec_Int_t *        vCutPairs;        // the pairs of parent cuts for each cut
    // minimum delay mapping with the given cuts
    Vec_Ptr_t *        vCutsMax;
    Vec_Int_t *        vDelays;
    Vec_Int_t *        vDelays2;
    int                nDelayMin;
    // statistics
    int                nCutsCur;
    int                nCutsAlloc;
    int                nCutsDealloc;
    int                nCutsPeak;
    int                nCutsTriv;
    int                nCutsFilter;
    int                nCutsLimit;
    int                nNodes;
    int                nNodesDag;
    int                nNodesNoCuts;
    // runtime
    abctime            timeMerge;
    abctime            timeUnion;
    abctime            timeTruth;
    abctime            timeFilter;
    abctime            timeHash;
    abctime            timeMap;
};


struct Cut_CutStruct_t_
{
    unsigned           Num0       : 11;   // temporary number
    unsigned           Num1       : 11;   // temporary number
    unsigned           fSimul     :  1;   // the value of cut's output at 000.. pattern
    unsigned           fCompl     :  1;   // the cut is complemented
    unsigned           nVarsMax   :  4;   // the max number of vars [4-6]
    unsigned           nLeaves    :  4;   // the number of leaves [4-6]
    unsigned           uSign;             // the signature
    unsigned           uCanon0;           // the canonical form
    unsigned           uCanon1;           // the canonical form
    Cut_Cut_t *        pNext;             // the next cut in the list
    int                pLeaves[0];        // the array of leaves
};


struct Cut_ParamsStruct_t_
{
    int                nVarsMax;          // the max cut size ("k" of the k-feasible cuts)
    int                nKeepMax;          // the max number of cuts kept at a node
    int                nIdsMax;           // the max number of IDs of cut objects
    int                nBitShift;         // the number of bits used for the latch counter of an edge
    int                nCutSet;           // the number of nodes in the cut set
    int                fTruth;            // compute truth tables
    int                fFilter;           // filter dominated cuts
    int                fSeq;              // compute sequential cuts
    int                fDrop;             // drop cuts on the fly
    int                fDag;              // compute only DAG cuts
    int                fTree;             // compute only tree cuts
    int                fGlobal;           // compute only global cuts
    int                fLocal;            // compute only local cuts
    int                fRecord;           // record the cut computation flow
    int                fRecordAig;        // record the cut functions
    int                fFancy;            // perform fancy computations
    int                fMap;              // computes delay of FPGA mapping with cuts
    int                fAdjust;           // removed useless fanouts of XORs/MUXes
    int                fNpnSave;          // enables dumping 6-input truth tables
    int                fVerbose;          // the verbosiness flag
};


struct Cut_ListStruct_t_
{
    Cut_Cut_t *  pHead[CUT_SIZE_MAX+1];
    Cut_Cut_t ** ppTail[CUT_SIZE_MAX+1];
};


static inline int        Cut_CutReadLeaveNum( Cut_Cut_t * p )  {  return p->nLeaves;   }
static inline int *      Cut_CutReadLeaves( Cut_Cut_t * p )    {  return p->pLeaves;   }
static inline unsigned * Cut_CutReadTruth( Cut_Cut_t * p )     {  return (unsigned *)(p->pLeaves + p->nVarsMax); }
static inline void       Cut_CutWriteTruth( Cut_Cut_t * p, unsigned * puTruth )  {
    int i;
    for ( i = (p->nVarsMax <= 5) ? 0 : ((1 << (p->nVarsMax - 5)) - 1); i >= 0; i-- )
        p->pLeaves[p->nVarsMax + i] = (int)puTruth[i];
}
static inline unsigned Cut_NodeSign( int Node )        { return (1 << (Node % 31));                        }
static inline int      Cut_TruthWords( int nVarsMax )  { return nVarsMax <= 5 ? 1 : (1 << (nVarsMax - 5)); }


Cut_Man_t * Abc_NtkStartCutManForRewrite( Abc_Ntk_t * pNtk );
void * Abc_NodeGetCutsRecursive( void * p, Abc_Obj_t * pObj, int fDag, int fTree );
void * Abc_NodeReadCuts( void * p, Abc_Obj_t * pObj );
void * Abc_NodeGetCuts( void * p, Abc_Obj_t * pObj, int fDag, int fTree );
void Abc_NodeFreeCuts( void * p, Abc_Obj_t * pObj );

Cut_Man_t * Cut_ManStart( Cut_Params_t * pParams );
void Cut_ManSetFanoutCounts( Cut_Man_t * p, Vec_Int_t * vFanCounts );
void Cut_NodeSetTriv( Cut_Man_t * p, int Node );
Cut_Cut_t * Cut_NodeReadCutsNew( Cut_Man_t * p, int Node );
void Cut_NodeWriteCutsNew( Cut_Man_t * p, int Node, Cut_Cut_t * pList );
Cut_Cut_t * Cut_CutCreateTriv( Cut_Man_t * p, int Node );
Cut_Cut_t * Cut_CutAlloc( Cut_Man_t * p );
void Cut_ManIncrementDagNodes( Cut_Man_t * p );
Cut_Params_t * Cut_ManReadParams( Cut_Man_t * p );
Vec_Int_t * Cut_ManReadNodeAttrs( Cut_Man_t * p );
Cut_Cut_t * Cut_NodeComputeCuts( Cut_Man_t * p, int Node, int Node0, int Node1, int fCompl0, int fCompl1, int fTriv, int TreeCode );
void Cut_CutNumberList( Cut_Cut_t * pList );
void Cut_NodeDoComputeCuts( Cut_Man_t * p, Cut_List_t * pSuper, int Node, int fCompl0, int fCompl1, Cut_Cut_t * pList0, Cut_Cut_t * pList1, int fTriv, int TreeCode );
Cut_Cut_t * Cut_CutMergeTwo( Cut_Man_t * p, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1 );
void Cut_CutRecycle( Cut_Man_t * p, Cut_Cut_t * pCut );
void Cut_TruthCompute( Cut_Man_t * p, Cut_Cut_t * pCut, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1, int fCompl0, int fCompl1 );
int Cut_NodeMapping( Cut_Man_t * p, Cut_Cut_t * pCuts, int Node, int Node0, int Node1 );
void Cut_ManStop( Cut_Man_t * p );
void Cut_NodeFreeCuts( Cut_Man_t * p, int Node );


#endif
