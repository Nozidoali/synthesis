#ifndef ABC_NTK_H
#define ABC_NTK_H


#include "abc_memory.h"
#include "abc_vec.h"
#include "abc_name.h"
#include "abc_global.h"
#include "abc_hop.h"
#include "abc_aig.h"
#include "abc_sop.h"
#include "abc_extra.h"


// objects of the network
#define Abc_NtkForEachObj( pNtk, pObj, i )                                                         \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pObj) = Abc_NtkObj(pNtk, i)), 1); i++ )    \
        if ( (pObj) == NULL ) {} else
#define Abc_NtkForEachObjReverse( pNtk, pNode, i )                                                 \
    for ( i = Vec_PtrSize((pNtk)->vObjs) - 1; (i >= 0) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i-- ) \
        if ( (pNode) == NULL ) {} else
#define Abc_NtkForEachObjVec( vIds, pNtk, pObj, i )                                                \
    for ( i = 0; i < Vec_IntSize(vIds) && (((pObj) = Abc_NtkObj(pNtk, Vec_IntEntry(vIds,i))), 1); i++ ) \
        if ( (pObj) == NULL ) {} else
#define Abc_NtkForEachObjVecStart( vIds, pNtk, pObj, i, Start )                                    \
    for ( i = Start; i < Vec_IntSize(vIds) && (((pObj) = Abc_NtkObj(pNtk, Vec_IntEntry(vIds,i))), 1); i++ ) \
        if ( (pObj) == NULL ) {} else
#define Abc_NtkForEachNet( pNtk, pNet, i )                                                         \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNet) = Abc_NtkObj(pNtk, i)), 1); i++ )    \
        if ( (pNet) == NULL || !Abc_ObjIsNet(pNet) ) {} else
#define Abc_NtkForEachNode( pNtk, pNode, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) ) {} else
#define Abc_NtkForEachNodeNotBarBuf( pNtk, pNode, i )                                              \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) || Abc_ObjIsBarBuf(pNode) ) {} else
#define Abc_NtkForEachNode1( pNtk, pNode, i )                                                      \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) || !Abc_ObjFaninNum(pNode) ) {} else
#define Abc_NtkForEachNodeNotBarBuf1( pNtk, pNode, i )                                             \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) || !Abc_ObjFaninNum(pNode) || Abc_ObjIsBarBuf(pNode) ) {} else
#define Abc_NtkForEachNodeReverse( pNtk, pNode, i )                                                \
    for ( i = Vec_PtrSize((pNtk)->vObjs) - 1; (i >= 0) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i-- ) \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) ) {} else
#define Abc_NtkForEachNodeReverse1( pNtk, pNode, i )                                               \
    for ( i = Vec_PtrSize((pNtk)->vObjs) - 1; (i >= 0) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i-- ) \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) || !Abc_ObjFaninNum(pNode) ) {} else
#define Abc_NtkForEachBarBuf( pNtk, pNode, i )                                                     \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsBarBuf(pNode) ) {} else
#define Abc_NtkForEachGate( pNtk, pNode, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsGate(pNode) ) {} else
#define Abc_AigForEachAnd( pNtk, pNode, i )                                                        \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_AigNodeIsAnd(pNode) ) {} else
#define Abc_NtkForEachNodeCi( pNtk, pNode, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || (!Abc_ObjIsNode(pNode) && !Abc_ObjIsCi(pNode)) ) {} else
#define Abc_NtkForEachNodeCo( pNtk, pNode, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || (!Abc_ObjIsNode(pNode) && !Abc_ObjIsCo(pNode)) ) {} else
// various boxes
#define Abc_NtkForEachBox( pNtk, pObj, i )                                                         \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)) && (((pObj) = Abc_NtkBox(pNtk, i)), 1); i++ )
#define Abc_NtkForEachLatch( pNtk, pObj, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)) && (((pObj) = Abc_NtkBox(pNtk, i)), 1); i++ )   \
        if ( !Abc_ObjIsLatch(pObj) ) {} else
#define Abc_NtkForEachLatchInput( pNtk, pObj, i )                                                  \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)); i++ )                                          \
        if ( !(Abc_ObjIsLatch(Abc_NtkBox(pNtk, i)) && (((pObj) = Abc_ObjFanin0(Abc_NtkBox(pNtk, i))), 1)) ) {} else
#define Abc_NtkForEachLatchOutput( pNtk, pObj, i )                                                 \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)); i++ )                                          \
        if ( !(Abc_ObjIsLatch(Abc_NtkBox(pNtk, i)) && (((pObj) = Abc_ObjFanout0(Abc_NtkBox(pNtk, i))), 1)) ) {} else
#define Abc_NtkForEachWhitebox( pNtk, pObj, i )                                                    \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)) && (((pObj) = Abc_NtkBox(pNtk, i)), 1); i++ )   \
        if ( !Abc_ObjIsWhitebox(pObj) ) {} else
#define Abc_NtkForEachBlackbox( pNtk, pObj, i )                                                    \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)) && (((pObj) = Abc_NtkBox(pNtk, i)), 1); i++ )   \
        if ( !Abc_ObjIsBlackbox(pObj) ) {} else
// inputs and outputs
#define Abc_NtkForEachPi( pNtk, pPi, i )                                                           \
    for ( i = 0; (i < Abc_NtkPiNum(pNtk)) && (((pPi) = Abc_NtkPi(pNtk, i)), 1); i++ )
#define Abc_NtkForEachCi( pNtk, pCi, i )                                                           \
    for ( i = 0; (i < Abc_NtkCiNum(pNtk)) && (((pCi) = Abc_NtkCi(pNtk, i)), 1); i++ )
#define Abc_NtkForEachPo( pNtk, pPo, i )                                                           \
    for ( i = 0; (i < Abc_NtkPoNum(pNtk)) && (((pPo) = Abc_NtkPo(pNtk, i)), 1); i++ )
#define Abc_NtkForEachCo( pNtk, pCo, i )                                                           \
    for ( i = 0; (i < Abc_NtkCoNum(pNtk)) && (((pCo) = Abc_NtkCo(pNtk, i)), 1); i++ )
#define Abc_NtkForEachLiPo( pNtk, pCo, i )                                                         \
    for ( i = 0; (i < Abc_NtkCoNum(pNtk)) && (((pCo) = Abc_NtkCo(pNtk, i < pNtk->nBarBufs ? Abc_NtkCoNum(pNtk) - pNtk->nBarBufs + i : i - pNtk->nBarBufs)), 1); i++ )
// fanin and fanouts
#define Abc_ObjForEachFanin( pObj, pFanin, i )                                                     \
    for ( i = 0; (i < Abc_ObjFaninNum(pObj)) && (((pFanin) = Abc_ObjFanin(pObj, i)), 1); i++ )
#define Abc_ObjForEachFanout( pObj, pFanout, i )                                                   \
    for ( i = 0; (i < Abc_ObjFanoutNum(pObj)) && (((pFanout) = Abc_ObjFanout(pObj, i)), 1); i++ )
#define Abc_ObjForEachFaninId( pObj, iFanin, i )                                                   \
    for ( i = 0; (i < Abc_ObjFaninNum(pObj)) && (((iFanin) = Abc_ObjFaninId(pObj, i)), 1); i++ )
#define Abc_ObjForEachFanoutId( pObj, iFanout, i )                                                 \
    for ( i = 0; (i < Abc_ObjFanoutNum(pObj)) && (((iFanout) = Abc_ObjFanoutId(pObj, i)), 1); i++ )
// cubes and literals
#define Abc_CubeForEachVar( pCube, Value, i )                                                      \
    for ( i = 0; (pCube[i] != ' ') && (Value = pCube[i]); i++ )


// network types
typedef enum {
    ABC_NTK_NONE = 0,   // 0:  unknown
    ABC_NTK_NETLIST,    // 1:  network with PIs/POs, latches, nodes, and nets
    ABC_NTK_LOGIC,      // 2:  network with PIs/POs, latches, and nodes
    ABC_NTK_STRASH,     // 3:  structurally hashed AIG (two input AND gates with c-attributes on edges)
    ABC_NTK_OTHER       // 4:  unused
} Abc_NtkType_t;


// network functionality
typedef enum {
    ABC_FUNC_NONE = 0,  // 0:  unknown
    ABC_FUNC_SOP,       // 1:  sum-of-products
    ABC_FUNC_BDD,       // 2:  binary decision diagrams
    ABC_FUNC_AIG,       // 3:  and-inverter graphs
    ABC_FUNC_MAP,       // 4:  standard cell library
    ABC_FUNC_BLIFMV,    // 5:  BLIF-MV node functions
    ABC_FUNC_BLACKBOX,  // 6:  black box about which nothing is known
    ABC_FUNC_OTHER      // 7:  unused
} Abc_NtkFunc_t;


// object types
typedef enum {
    ABC_OBJ_NONE = 0,   //  0:  unknown
    ABC_OBJ_CONST1,     //  1:  constant 1 node (AIG only)
    ABC_OBJ_PI,         //  2:  primary input terminal
    ABC_OBJ_PO,         //  3:  primary output terminal
    ABC_OBJ_BI,         //  4:  box input terminal
    ABC_OBJ_BO,         //  5:  box output terminal
    ABC_OBJ_NET,        //  6:  net
    ABC_OBJ_NODE,       //  7:  node
    ABC_OBJ_LATCH,      //  8:  latch
    ABC_OBJ_WHITEBOX,   //  9:  box with known contents
    ABC_OBJ_BLACKBOX,   // 10:  box with unknown contents
    ABC_OBJ_NUMBER      // 11:  unused
} Abc_ObjType_t;


typedef struct Abc_Obj_t_       Abc_Obj_t;

typedef struct Abc_Ntk_t_       Abc_Ntk_t;

// typedef struct Abc_Time_t_      Abc_Time_t;
// typedef struct Abc_ManTime_t_ Abc_ManTime_t;

typedef struct Abc_Aig_t_       Abc_Aig_t;



struct Abc_Obj_t_     // 48/72 bytes (32-bits/64-bits)
{
    Abc_Ntk_t *       pNtk;          // the host network
    Abc_Obj_t *       pNext;         // the next pointer in the hash table
    int               Id;            // the object ID
    unsigned          Type    :  4;  // the object type
    unsigned          fMarkA  :  1;  // the multipurpose mark
    unsigned          fMarkB  :  1;  // the multipurpose mark
    unsigned          fMarkC  :  1;  // the multipurpose mark
    unsigned          fPhase  :  1;  // the flag to mark the phase of equivalent node
    unsigned          fExor   :  1;  // marks AIG node that is a root of EXOR
    unsigned          fPersist:  1;  // marks the persistant AIG node
    unsigned          fCompl0 :  1;  // complemented attribute of the first fanin in the AIG
    unsigned          fCompl1 :  1;  // complemented attribute of the second fanin in the AIG
    unsigned          Level   : 20;  // the level of the node
    Vec_Int_t         vFanins;       // the array of fanins
    Vec_Int_t         vFanouts;      // the array of fanouts
    union { void *    pData;         // the network specific data
      int             iData; };      // (SOP, BDD, gate, equiv class, etc)
    union { void *    pTemp;         // temporary store for user's data
      Abc_Obj_t *     pCopy;         // the copy of this object
      int             iTemp;
      float           dTemp; };
};


struct Abc_Ntk_t_
{
    // general information
    Abc_NtkType_t     ntkType;       // type of the network
    Abc_NtkFunc_t     ntkFunc;       // functionality of the network
    char *            pName;         // the network name
    char *            pSpec;         // the name of the spec file if present
    Nm_Man_t *        pManName;      // name manager (stores names of objects)
    // components of the network
    Vec_Ptr_t *       vObjs;         // the array of all objects (net, nodes, latches, etc)
    Vec_Ptr_t *       vPis;          // the array of primary inputs
    Vec_Ptr_t *       vPos;          // the array of primary outputs
    Vec_Ptr_t *       vCis;          // the array of combinational inputs  (PIs, latches)
    Vec_Ptr_t *       vCos;          // the array of combinational outputs (POs, asserts, latches)
    Vec_Ptr_t *       vPios;         // the array of PIOs
    Vec_Ptr_t *       vBoxes;        // the array of boxes
    Vec_Ptr_t *       vLtlProperties;
    // the number of living objects
    int nObjCounts[ABC_OBJ_NUMBER];  // the number of objects by type
    int               nObjs;         // the number of live objs
    int               nConstrs;      // the number of constraints
    int               nBarBufs;      // the number of barrier buffers
    int               nBarBufs2;     // the number of barrier buffers
    // // the backup network and the step number
    Abc_Ntk_t *       pNetBackup;    // the pointer to the previous backup network
    int               iStep;         // the generation number for the given network
    // // hierarchy
    // Abc_Des_t *       pDesign;       // design (hierarchical networks only)
    // Abc_Ntk_t *       pAltView;      // alternative structural view of the network
    // int               fHieVisited;   // flag to mark the visited network
    // int               fHiePath;      // flag to mark the network on the path
    // int               Id;            // model ID
    // double            dTemp;         // temporary value
    // miscellaneous data members
    int               nTravIds;      // the unique traversal IDs of nodes
    Vec_Int_t         vTravIds;      // trav IDs of the objects
    Mem_Fixed_t *     pMmObj;        // memory manager for objects
    Mem_Step_t *      pMmStep;       // memory manager for arrays
    void *            pManFunc;      // functionality manager (AIG manager, BDD manager, or memory manager for SOPs)
    // Abc_ManTime_t *   pManTime;      // the timing manager (for mapped networks) stores arrival/required times for all nodes
    void *            pManCut;       // the cut manager (for AIGs) stores information about the cuts computed for the nodes
    // float             AndGateDelay;  // an average estimated delay of one AND gate
    int               LevelMax;      // maximum number of levels
    Vec_Int_t *       vLevelsR;      // level in the reverse topological order (for AIGs)
    Vec_Ptr_t *       vSupps;        // CO support information
    int *             pModel;        // counter-example (for miters)
    // Abc_Cex_t *       pSeqModel;     // counter-example (for sequential miters)
    Vec_Ptr_t *       vSeqModelVec;  // vector of counter-examples (for sequential miters)
    Abc_Ntk_t *       pExdc;         // the EXDC network (if given)
    void *            pExcare;       // the EXDC network (if given)
    void *            pData;         // misc
    Abc_Ntk_t *       pCopy;         // copy of this network
    void *            pBSMan;        // application manager
    void *            pSCLib;        // SC library
    Vec_Int_t *       vGates;        // SC library gates
    Vec_Int_t *       vPhases;       // fanins phases in the mapped netlist
    char *            pWLoadUsed;    // wire load model used
    float *           pLutTimes;     // arrivals/requireds/slacks using LUT-delay model
    Vec_Ptr_t *       vOnehots;      // names of one-hot-encoded registers
    Vec_Int_t *       vObjPerm;      // permutation saved
    Vec_Int_t *       vTopo;
    Vec_Ptr_t *       vAttrs;        // managers of various node attributes (node functionality, global BDDs, etc)
    Vec_Int_t *       vNameIds;      // name IDs
    Vec_Int_t *       vFins;         // obj/type info
};


Abc_Ntk_t * Abc_NtkAlloc( Abc_NtkType_t Type, Abc_NtkFunc_t Func, int fUseMemMan );
void Abc_NtkDelete( Abc_Ntk_t * pNtk );
Abc_Obj_t * Abc_NtkFindNet( Abc_Ntk_t * pNtk, char * pName );
Abc_Obj_t * Abc_NtkFindOrCreateNet( Abc_Ntk_t * pNtk, char * pName );
Abc_Obj_t * Abc_NtkCreateObj( Abc_Ntk_t * pNtk, Abc_ObjType_t Type );
Abc_Obj_t * Abc_NtkCreateNodeInv( Abc_Ntk_t * pNtk, Abc_Obj_t * pFanin );
Abc_Obj_t * Abc_NtkCreateNodeBuf( Abc_Ntk_t * pNtk, Abc_Obj_t * pFanin );
void Abc_NtkFinalizeRead( Abc_Ntk_t * pNtk );
void Abc_NtkFinalize( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew );
void Abc_NtkTransferPhases( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk );
void Abc_NtkDeleteObj( Abc_Obj_t * pObj );
Abc_Obj_t * Abc_NtkCreateNodeConst0( Abc_Ntk_t * pNtk );
Abc_Obj_t * Abc_NtkCreateNodeConst1( Abc_Ntk_t * pNtk );
void Abc_NtkFixNonDrivenNets( Abc_Ntk_t * pNtk );
void Abc_NtkOrderCisCos( Abc_Ntk_t * pNtk );
int Abc_NtkCheckRead( Abc_Ntk_t * pNtk );
int Abc_NtkDoCheck( Abc_Ntk_t * pNtk );
int Abc_NtkCheckNames( Abc_Ntk_t * pNtk );
int Abc_NtkCheckUniqueCiNames( Abc_Ntk_t * pNtk );
int Abc_NtkNamesCompare( char ** pName1, char ** pName2 );
int Abc_NtkCheckUniqueCoNames( Abc_Ntk_t * pNtk );
int Abc_NtkCheckUniqueCioNames( Abc_Ntk_t * pNtk );
int Abc_NtkCheck( Abc_Ntk_t * pNtk );
void Abc_NtkCleanCopy( Abc_Ntk_t * pNtk );
int Abc_NtkCheckPis( Abc_Ntk_t * pNtk );
int Abc_NtkCheckPos( Abc_Ntk_t * pNtk );
int Abc_NtkCheckObj( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj );
int Abc_NtkCheckNet( Abc_Ntk_t * pNtk, Abc_Obj_t * pNet );
int Abc_NtkCheckNode( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode );
int Abc_NtkIsAcyclic( Abc_Ntk_t * pNtk );
int Abc_NtkIsAcyclic_rec( Abc_Obj_t * pNode );
Abc_Obj_t * Abc_NtkDupObj( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, int fCopyName );
Abc_Obj_t * Abc_NtkDupBox( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pBox, int fCopyName );
Vec_Ptr_t * Abc_NtkDfs( Abc_Ntk_t * pNtk, int fCollectAll );
void Abc_NtkDfs_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
Vec_Ptr_t * Abc_NtkDfsIter( Abc_Ntk_t * pNtk, int fCollectAll );
void Abc_NtkDfs_iter( Vec_Ptr_t * vStack, Abc_Obj_t * pRoot, Vec_Ptr_t * vNodes );
int Abc_NtkGetFaninMax( Abc_Ntk_t * pNtk );
int Abc_NtkGetFanoutMax( Abc_Ntk_t * pNtk );
int Abc_NtkLevel( Abc_Ntk_t * pNtk );
int Abc_NtkLevel_rec( Abc_Obj_t * pNode );
int Abc_NtkCleanup( Abc_Ntk_t * pNtk, int fVerbose );
int Abc_NtkReduceNodes( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes );
void Abc_NtkStartReverseLevels( Abc_Ntk_t * pNtk, int nMaxLevelIncrease );
Vec_Ptr_t * Abc_NtkDfsReverse( Abc_Ntk_t * pNtk );
void Abc_NtkDfsReverse_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
Vec_Int_t * Abc_NtkFanoutCounts( Abc_Ntk_t * pNtk );
void Abc_NtkReassignIds( Abc_Ntk_t * pNtk );
void Abc_NtkStopReverseLevels( Abc_Ntk_t * pNtk );

void Abc_ObjAddFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFanin );
Abc_Obj_t * Abc_ObjAlloc( Abc_Ntk_t * pNtk, Abc_ObjType_t Type );
char * Abc_ObjName( Abc_Obj_t * pObj );
void Abc_ObjDeleteFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFanin );
void Abc_ObjRecycle( Abc_Obj_t * pObj );
int Abc_ObjReverseLevel( Abc_Obj_t * pObj );
void Abc_ObjPatchFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFaninOld, Abc_Obj_t * pFaninNew );
void Abc_ObjSetReverseLevel( Abc_Obj_t * pObj, int LevelR );
int Abc_ObjReverseLevelNew( Abc_Obj_t * pObj );
int Abc_ObjRequiredLevel( Abc_Obj_t * pObj );
void Abc_ObjRemoveFanins( Abc_Obj_t * pObj );

void Abc_NodeCollectFanouts( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
void Abc_NodeCollectFanins( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
int Abc_NodeIsConst( Abc_Obj_t * pNode );
int Abc_NodeIsConst0( Abc_Obj_t * pNode );
int Abc_NodeIsConst1( Abc_Obj_t * pNode );
int Abc_NodeIsExorType( Abc_Obj_t * pNode );
int Abc_NodeIsMuxControlType( Abc_Obj_t * pNode );
void Abc_NodeComplement( Abc_Obj_t * pNode );
void Abc_NodeComplementInput( Abc_Obj_t * pNode, Abc_Obj_t * pFanin );
Abc_Obj_t * Abc_NodeStrash( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNodeOld, int fRecord );
void Abc_NodeStrash_rec( Abc_Aig_t * pMan, Hop_Obj_t * pObj );
int Abc_NodeMffcLabelAig( Abc_Obj_t * pNode );
int Abc_NodeRefDeref( Abc_Obj_t * pNode, int fReference, int fLabel );


// checking the network type
static inline int         Abc_NtkIsNetlist( Abc_Ntk_t * pNtk )       { return pNtk->ntkType == ABC_NTK_NETLIST;     }
static inline int         Abc_NtkIsLogic( Abc_Ntk_t * pNtk )         { return pNtk->ntkType == ABC_NTK_LOGIC;       }
static inline int         Abc_NtkIsStrash( Abc_Ntk_t * pNtk )        { return pNtk->ntkType == ABC_NTK_STRASH;      }
static inline int         Abc_NtkHasSop( Abc_Ntk_t * pNtk )          { return pNtk->ntkFunc == ABC_FUNC_SOP;        }
static inline int         Abc_NtkHasBdd( Abc_Ntk_t * pNtk )          { return pNtk->ntkFunc == ABC_FUNC_BDD;        }
static inline int         Abc_NtkHasAig( Abc_Ntk_t * pNtk )          { return pNtk->ntkFunc == ABC_FUNC_AIG;        }
static inline int         Abc_NtkHasMapping( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_MAP;        }
static inline int         Abc_NtkHasBlifMv( Abc_Ntk_t * pNtk )       { return pNtk->ntkFunc == ABC_FUNC_BLIFMV;     }
static inline int         Abc_NtkHasBlackbox( Abc_Ntk_t * pNtk )     { return pNtk->ntkFunc == ABC_FUNC_BLACKBOX;   }
static inline int         Abc_NtkIsSopNetlist( Abc_Ntk_t * pNtk )    { return pNtk->ntkFunc == ABC_FUNC_SOP && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline int         Abc_NtkIsBddNetlist( Abc_Ntk_t * pNtk )    { return pNtk->ntkFunc == ABC_FUNC_BDD && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline int         Abc_NtkIsAigNetlist( Abc_Ntk_t * pNtk )    { return pNtk->ntkFunc == ABC_FUNC_AIG && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline int         Abc_NtkIsMappedNetlist( Abc_Ntk_t * pNtk ) { return pNtk->ntkFunc == ABC_FUNC_MAP && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline int         Abc_NtkIsBlifMvNetlist( Abc_Ntk_t * pNtk ) { return pNtk->ntkFunc == ABC_FUNC_BLIFMV && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline int         Abc_NtkIsSopLogic( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_SOP && pNtk->ntkType == ABC_NTK_LOGIC  ;  }
static inline int         Abc_NtkIsBddLogic( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_BDD && pNtk->ntkType == ABC_NTK_LOGIC  ;  }
static inline int         Abc_NtkIsAigLogic( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_AIG && pNtk->ntkType == ABC_NTK_LOGIC  ;  }
static inline int         Abc_NtkIsMappedLogic( Abc_Ntk_t * pNtk )   { return pNtk->ntkFunc == ABC_FUNC_MAP && pNtk->ntkType == ABC_NTK_LOGIC  ;  }

// reading objects
static inline Abc_Obj_t * Abc_NtkObj( Abc_Ntk_t * pNtk, int i )      { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vObjs, i );   }
static inline Abc_Obj_t * Abc_NtkPi( Abc_Ntk_t * pNtk, int i )       { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vPis, i );    }
static inline Abc_Obj_t * Abc_NtkPo( Abc_Ntk_t * pNtk, int i )       { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vPos, i );    }
static inline Abc_Obj_t * Abc_NtkCi( Abc_Ntk_t * pNtk, int i )       { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vCis, i );    }
static inline Abc_Obj_t * Abc_NtkCo( Abc_Ntk_t * pNtk, int i )       { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vCos, i );    }
static inline Abc_Obj_t * Abc_NtkBox( Abc_Ntk_t * pNtk, int i )      { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vBoxes, i );  }

// reading data members of the network
static inline char *      Abc_NtkName( Abc_Ntk_t * pNtk )            { return pNtk->pName;            }
static inline char *      Abc_NtkSpec( Abc_Ntk_t * pNtk )            { return pNtk->pSpec;            }
static inline Abc_Ntk_t * Abc_NtkExdc( Abc_Ntk_t * pNtk )            { return pNtk->pExdc;            }
static inline Abc_Ntk_t * Abc_NtkBackup( Abc_Ntk_t * pNtk )          { return pNtk->pNetBackup;       }
static inline int         Abc_NtkStep  ( Abc_Ntk_t * pNtk )          { return pNtk->iStep;            }

// creating simple objects
static inline Abc_Obj_t * Abc_NtkCreatePi( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_PI );         }
static inline Abc_Obj_t * Abc_NtkCreatePo( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_PO );         }
static inline Abc_Obj_t * Abc_NtkCreateBi( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_BI );         }
static inline Abc_Obj_t * Abc_NtkCreateBo( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_BO );         }
static inline Abc_Obj_t * Abc_NtkCreateNet( Abc_Ntk_t * pNtk )       { return Abc_NtkCreateObj( pNtk, ABC_OBJ_NET );        }
static inline Abc_Obj_t * Abc_NtkCreateNode( Abc_Ntk_t * pNtk )      { return Abc_NtkCreateObj( pNtk, ABC_OBJ_NODE );       }
static inline Abc_Obj_t * Abc_NtkCreateLatch( Abc_Ntk_t * pNtk )     { return Abc_NtkCreateObj( pNtk, ABC_OBJ_LATCH );      }
static inline Abc_Obj_t * Abc_NtkCreateWhitebox( Abc_Ntk_t * pNtk )  { return Abc_NtkCreateObj( pNtk, ABC_OBJ_WHITEBOX );   }
static inline Abc_Obj_t * Abc_NtkCreateBlackbox( Abc_Ntk_t * pNtk )  { return Abc_NtkCreateObj( pNtk, ABC_OBJ_BLACKBOX );   }

// getting the number of objects
static inline int         Abc_NtkObjNum( Abc_Ntk_t * pNtk )          { return pNtk->nObjs;                        }
static inline int         Abc_NtkObjNumMax( Abc_Ntk_t * pNtk )       { return Vec_PtrSize(pNtk->vObjs);           }
static inline int         Abc_NtkPiNum( Abc_Ntk_t * pNtk )           { return Vec_PtrSize(pNtk->vPis);            }
static inline int         Abc_NtkPoNum( Abc_Ntk_t * pNtk )           { return Vec_PtrSize(pNtk->vPos);            }
static inline int         Abc_NtkCiNum( Abc_Ntk_t * pNtk )           { return Vec_PtrSize(pNtk->vCis);            }
static inline int         Abc_NtkCoNum( Abc_Ntk_t * pNtk )           { return Vec_PtrSize(pNtk->vCos);            }
static inline int         Abc_NtkBoxNum( Abc_Ntk_t * pNtk )          { return Vec_PtrSize(pNtk->vBoxes);          }
static inline int         Abc_NtkBiNum( Abc_Ntk_t * pNtk )           { return pNtk->nObjCounts[ABC_OBJ_BI];       }
static inline int         Abc_NtkBoNum( Abc_Ntk_t * pNtk )           { return pNtk->nObjCounts[ABC_OBJ_BO];       }
static inline int         Abc_NtkNetNum( Abc_Ntk_t * pNtk )          { return pNtk->nObjCounts[ABC_OBJ_NET];      }
static inline int         Abc_NtkNodeNum( Abc_Ntk_t * pNtk )         { return pNtk->nObjCounts[ABC_OBJ_NODE];     }
static inline int         Abc_NtkLatchNum( Abc_Ntk_t * pNtk )        { return pNtk->nObjCounts[ABC_OBJ_LATCH];    }
static inline int         Abc_NtkWhiteboxNum( Abc_Ntk_t * pNtk )     { return pNtk->nObjCounts[ABC_OBJ_WHITEBOX]; }
static inline int         Abc_NtkBlackboxNum( Abc_Ntk_t * pNtk )     { return pNtk->nObjCounts[ABC_OBJ_BLACKBOX]; }
static inline int         Abc_NtkIsComb( Abc_Ntk_t * pNtk )          { return Abc_NtkLatchNum(pNtk) == 0;                   }
static inline int         Abc_NtkHasOnlyLatchBoxes(Abc_Ntk_t * pNtk ){ return Abc_NtkLatchNum(pNtk) == Abc_NtkBoxNum(pNtk); }
static inline int         Abc_NtkConstrNum( Abc_Ntk_t * pNtk )       { return pNtk->nConstrs;                     }

// working with complemented attributes of objects
static inline int         Abc_ObjIsComplement( Abc_Obj_t * p )       { return (int )((ABC_PTRUINT_T)p & (ABC_PTRUINT_T)01);             }
static inline Abc_Obj_t * Abc_ObjRegular( Abc_Obj_t * p )            { return (Abc_Obj_t *)((ABC_PTRUINT_T)p & ~(ABC_PTRUINT_T)01);     }
static inline Abc_Obj_t * Abc_ObjNot( Abc_Obj_t * p )                { return (Abc_Obj_t *)((ABC_PTRUINT_T)p ^  (ABC_PTRUINT_T)01);     }
static inline Abc_Obj_t * Abc_ObjNotCond( Abc_Obj_t * p, int c )     { return (Abc_Obj_t *)((ABC_PTRUINT_T)p ^  (ABC_PTRUINT_T)(c!=0)); }

// checking the object type
static inline int         Abc_ObjIsNone( Abc_Obj_t * pObj )          { return pObj->Type == ABC_OBJ_NONE;    }
static inline int         Abc_ObjIsPi( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PI;      }
static inline int         Abc_ObjIsPo( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PO;      }
static inline int         Abc_ObjIsBi( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_BI;      }
static inline int         Abc_ObjIsBo( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_BO;      }
static inline int         Abc_ObjIsCi( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PI || pObj->Type == ABC_OBJ_BO; }
static inline int         Abc_ObjIsCo( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PO || pObj->Type == ABC_OBJ_BI; }
static inline int         Abc_ObjIsTerm( Abc_Obj_t * pObj )          { return Abc_ObjIsCi(pObj) || Abc_ObjIsCo(pObj); }
static inline int         Abc_ObjIsNet( Abc_Obj_t * pObj )           { return pObj->Type == ABC_OBJ_NET;     }
static inline int         Abc_ObjIsNode( Abc_Obj_t * pObj )          { return pObj->Type == ABC_OBJ_NODE;    }
static inline int         Abc_ObjIsLatch( Abc_Obj_t * pObj )         { return pObj->Type == ABC_OBJ_LATCH;   }
static inline int         Abc_ObjIsBox( Abc_Obj_t * pObj )           { return pObj->Type == ABC_OBJ_LATCH || pObj->Type == ABC_OBJ_WHITEBOX || pObj->Type == ABC_OBJ_BLACKBOX; }
static inline int         Abc_ObjIsWhitebox( Abc_Obj_t * pObj )      { return pObj->Type == ABC_OBJ_WHITEBOX;}
static inline int         Abc_ObjIsBlackbox( Abc_Obj_t * pObj )      { return pObj->Type == ABC_OBJ_BLACKBOX;}
static inline int         Abc_ObjIsBarBuf( Abc_Obj_t * pObj )        { return Abc_NtkHasMapping(pObj->pNtk) && Abc_ObjIsNode(pObj) && Vec_IntSize(&pObj->vFanins) == 1 && pObj->pData == NULL;  }
static inline void        Abc_ObjBlackboxToWhitebox( Abc_Obj_t * pObj ) { assert( Abc_ObjIsBlackbox(pObj) ); pObj->Type = ABC_OBJ_WHITEBOX; pObj->pNtk->nObjCounts[ABC_OBJ_BLACKBOX]--; pObj->pNtk->nObjCounts[ABC_OBJ_WHITEBOX]++; }

// reading data members of the object
static inline unsigned    Abc_ObjType( Abc_Obj_t * pObj )            { return pObj->Type;               }
static inline unsigned    Abc_ObjId( Abc_Obj_t * pObj )              { return pObj->Id;                 }
static inline int         Abc_ObjLevel( Abc_Obj_t * pObj )           { return pObj->Level;              }
static inline Vec_Int_t * Abc_ObjFaninVec( Abc_Obj_t * pObj )        { return &pObj->vFanins;           }
static inline Vec_Int_t * Abc_ObjFanoutVec( Abc_Obj_t * pObj )       { return &pObj->vFanouts;          }
static inline Abc_Obj_t * Abc_ObjCopy( Abc_Obj_t * pObj )            { return pObj->pCopy;              }
static inline Abc_Ntk_t * Abc_ObjNtk( Abc_Obj_t * pObj )             { return pObj->pNtk;               }
static inline Abc_Ntk_t * Abc_ObjModel( Abc_Obj_t * pObj )           { assert( pObj->Type == ABC_OBJ_WHITEBOX ); return (Abc_Ntk_t *)pObj->pData;   }
static inline void *      Abc_ObjData( Abc_Obj_t * pObj )            { return pObj->pData;              }
static inline Abc_Obj_t * Abc_ObjEquiv( Abc_Obj_t * pObj )           { return (Abc_Obj_t *)pObj->pData; }
static inline Abc_Obj_t * Abc_ObjCopyCond( Abc_Obj_t * pObj )        { return Abc_ObjRegular(pObj)->pCopy? Abc_ObjNotCond(Abc_ObjRegular(pObj)->pCopy, Abc_ObjIsComplement(pObj)) : NULL;  }

// working with fanin/fanout edges
static inline int         Abc_ObjFaninNum( Abc_Obj_t * pObj )        { return pObj->vFanins.nSize;     }
static inline int         Abc_ObjFanoutNum( Abc_Obj_t * pObj )       { return pObj->vFanouts.nSize;    }
static inline int         Abc_ObjFaninId( Abc_Obj_t * pObj, int i)   { return pObj->vFanins.pArray[i]; }
static inline int         Abc_ObjFaninId0( Abc_Obj_t * pObj )        { return pObj->vFanins.pArray[0]; }
static inline int         Abc_ObjFaninId1( Abc_Obj_t * pObj )        { return pObj->vFanins.pArray[1]; }
static inline int         Abc_ObjFanoutEdgeNum( Abc_Obj_t * pObj, Abc_Obj_t * pFanout )  { assert( Abc_NtkHasAig(pObj->pNtk) );  if ( Abc_ObjFaninId0(pFanout) == pObj->Id ) return 0; if ( Abc_ObjFaninId1(pFanout) == pObj->Id ) return 1; assert( 0 ); return -1;  }
static inline Abc_Obj_t * Abc_ObjFanout( Abc_Obj_t * pObj, int i )   { return (Abc_Obj_t *)pObj->pNtk->vObjs->pArray[ pObj->vFanouts.pArray[i] ];  }
static inline Abc_Obj_t * Abc_ObjFanout0( Abc_Obj_t * pObj )         { return (Abc_Obj_t *)pObj->pNtk->vObjs->pArray[ pObj->vFanouts.pArray[0] ];  }
static inline Abc_Obj_t * Abc_ObjFanin( Abc_Obj_t * pObj, int i )    { return (Abc_Obj_t *)pObj->pNtk->vObjs->pArray[ pObj->vFanins.pArray[i] ];   }
static inline Abc_Obj_t * Abc_ObjFanin0( Abc_Obj_t * pObj )          { return (Abc_Obj_t *)pObj->pNtk->vObjs->pArray[ pObj->vFanins.pArray[0] ];   }
static inline Abc_Obj_t * Abc_ObjFanin1( Abc_Obj_t * pObj )          { return (Abc_Obj_t *)pObj->pNtk->vObjs->pArray[ pObj->vFanins.pArray[1] ];   }
static inline Abc_Obj_t * Abc_ObjFanin0Ntk( Abc_Obj_t * pObj )       { return (Abc_NtkIsNetlist(pObj->pNtk)? Abc_ObjFanin0(pObj)  : pObj);  }
static inline Abc_Obj_t * Abc_ObjFanout0Ntk( Abc_Obj_t * pObj )      { return (Abc_NtkIsNetlist(pObj->pNtk)? Abc_ObjFanout0(pObj) : pObj);  }
static inline int         Abc_ObjFaninC0( Abc_Obj_t * pObj )         { return pObj->fCompl0;                                                }
static inline int         Abc_ObjFaninC1( Abc_Obj_t * pObj )         { return pObj->fCompl1;                                                }
static inline int         Abc_ObjFaninC( Abc_Obj_t * pObj, int i )   { assert( i >=0 && i < 2 ); return i? pObj->fCompl1 : pObj->fCompl0;   }
static inline void        Abc_ObjSetFaninC( Abc_Obj_t * pObj, int i ){ assert( i >=0 && i < 2 ); if ( i ) pObj->fCompl1 = 1; else pObj->fCompl0 = 1; }
static inline void        Abc_ObjXorFaninC( Abc_Obj_t * pObj, int i ){ assert( i >=0 && i < 2 ); if ( i ) pObj->fCompl1^= 1; else pObj->fCompl0^= 1; }
static inline Abc_Obj_t * Abc_ObjChild( Abc_Obj_t * pObj, int i )    { return Abc_ObjNotCond( Abc_ObjFanin(pObj,i), Abc_ObjFaninC(pObj,i) );}
static inline Abc_Obj_t * Abc_ObjChild0( Abc_Obj_t * pObj )          { return Abc_ObjNotCond( Abc_ObjFanin0(pObj), Abc_ObjFaninC0(pObj) );  }
static inline Abc_Obj_t * Abc_ObjChild1( Abc_Obj_t * pObj )          { return Abc_ObjNotCond( Abc_ObjFanin1(pObj), Abc_ObjFaninC1(pObj) );  }
static inline Abc_Obj_t * Abc_ObjChildCopy( Abc_Obj_t * pObj, int i ){ return Abc_ObjNotCond( Abc_ObjFanin(pObj,i)->pCopy, Abc_ObjFaninC(pObj,i) );  }
static inline Abc_Obj_t * Abc_ObjChild0Copy( Abc_Obj_t * pObj )      { return Abc_ObjNotCond( Abc_ObjFanin0(pObj)->pCopy, Abc_ObjFaninC0(pObj) );    }
static inline Abc_Obj_t * Abc_ObjChild1Copy( Abc_Obj_t * pObj )      { return Abc_ObjNotCond( Abc_ObjFanin1(pObj)->pCopy, Abc_ObjFaninC1(pObj) );    }
static inline Abc_Obj_t * Abc_ObjChild0Data( Abc_Obj_t * pObj )      { return Abc_ObjNotCond( (Abc_Obj_t *)Abc_ObjFanin0(pObj)->pData, Abc_ObjFaninC0(pObj) );    }
static inline Abc_Obj_t * Abc_ObjChild1Data( Abc_Obj_t * pObj )      { return Abc_ObjNotCond( (Abc_Obj_t *)Abc_ObjFanin1(pObj)->pData, Abc_ObjFaninC1(pObj) );    }
static inline Abc_Obj_t * Abc_ObjFromLit( Abc_Ntk_t * p, int iLit )  { return Abc_ObjNotCond( Abc_NtkObj(p, Abc_Lit2Var(iLit)), Abc_LitIsCompl(iLit) );           }
static inline int         Abc_ObjToLit( Abc_Obj_t * p )              { return Abc_Var2Lit( Abc_ObjId(Abc_ObjRegular(p)), Abc_ObjIsComplement(p) );                }
static inline int         Abc_ObjFaninPhase( Abc_Obj_t * p, int i )  { assert(p->pNtk->vPhases); assert( i >= 0 && i < Abc_ObjFaninNum(p) ); return (Vec_IntEntry(p->pNtk->vPhases, Abc_ObjId(p)) >> i) & 1;  }
static inline void        Abc_ObjFaninFlipPhase( Abc_Obj_t * p,int i){ assert(p->pNtk->vPhases); assert( i >= 0 && i < Abc_ObjFaninNum(p) ); *Vec_IntEntryP(p->pNtk->vPhases, Abc_ObjId(p)) ^= (1 << i);      }

// setting data members of the network
static inline void        Abc_ObjSetLevel( Abc_Obj_t * pObj, int Level )         { pObj->Level =  Level;    }
static inline void        Abc_ObjSetCopy( Abc_Obj_t * pObj, Abc_Obj_t * pCopy )  { pObj->pCopy =  pCopy;    }
static inline void        Abc_ObjSetData( Abc_Obj_t * pObj, void * pData )       { pObj->pData =  pData;    }

// working with the traversal ID
static inline void        Abc_NtkIncrementTravId( Abc_Ntk_t * p )           { if (!p->vTravIds.pArray) Vec_IntFill(&p->vTravIds, Abc_NtkObjNumMax(p)+500, 0); p->nTravIds++; assert(p->nTravIds < (1<<30));  }
static inline int         Abc_NodeTravId( Abc_Obj_t * p )                   { return Vec_IntGetEntry(&Abc_ObjNtk(p)->vTravIds, Abc_ObjId(p));                       }
static inline void        Abc_NodeSetTravId( Abc_Obj_t * p, int TravId )    { Vec_IntSetEntry(&Abc_ObjNtk(p)->vTravIds, Abc_ObjId(p), TravId );                     }
static inline void        Abc_NodeSetTravIdCurrent( Abc_Obj_t * p )         { Abc_NodeSetTravId( p, Abc_ObjNtk(p)->nTravIds );                                      }
static inline void        Abc_NodeSetTravIdPrevious( Abc_Obj_t * p )        { Abc_NodeSetTravId( p, Abc_ObjNtk(p)->nTravIds-1 );                                    }
static inline int         Abc_NodeIsTravIdCurrent( Abc_Obj_t * p )          { return (Abc_NodeTravId(p) == Abc_ObjNtk(p)->nTravIds);                                }
static inline int         Abc_NodeIsTravIdPrevious( Abc_Obj_t * p )         { return (Abc_NodeTravId(p) == Abc_ObjNtk(p)->nTravIds-1);                              }
static inline void        Abc_NodeSetTravIdCurrentId( Abc_Ntk_t * p, int i) { Vec_IntSetEntry(&p->vTravIds, i, p->nTravIds );                                       }
static inline int         Abc_NodeIsTravIdCurrentId( Abc_Ntk_t * p, int i)  { return (Vec_IntGetEntry(&p->vTravIds, i) == p->nTravIds);                             }

// checking the AIG node types
static inline int         Abc_AigNodeIsConst( Abc_Obj_t * pNode )    { assert(Abc_NtkIsStrash(Abc_ObjRegular(pNode)->pNtk));  return Abc_ObjRegular(pNode)->Type == ABC_OBJ_CONST1;       }
static inline int         Abc_AigNodeIsAnd( Abc_Obj_t * pNode )      { assert(!Abc_ObjIsComplement(pNode)); assert(Abc_NtkIsStrash(pNode->pNtk)); return Abc_ObjFaninNum(pNode) == 2;                         }
static inline int         Abc_AigNodeIsChoice( Abc_Obj_t * pNode )   { assert(!Abc_ObjIsComplement(pNode)); assert(Abc_NtkIsStrash(pNode->pNtk)); return pNode->pData != NULL && Abc_ObjFanoutNum(pNode) > 0; }

// handling persistent nodes
static inline int         Abc_NodeIsPersistant( Abc_Obj_t * pNode )    { assert( Abc_AigNodeIsAnd(pNode) ); return pNode->fPersist; }
static inline void        Abc_NodeSetPersistant( Abc_Obj_t * pNode )   { assert( Abc_AigNodeIsAnd(pNode) ); pNode->fPersist = 1;    }
static inline void        Abc_NodeClearPersistant( Abc_Obj_t * pNode ) { assert( Abc_AigNodeIsAnd(pNode) ); pNode->fPersist = 0;    }

static inline char * Abc_ObjNameNet( Abc_Obj_t * pObj ) { return (Abc_ObjIsNode(pObj) && Abc_NtkIsNetlist(pObj->pNtk)) ? Abc_ObjName(Abc_ObjFanout0(pObj)) : Abc_ObjName(pObj); }
char * Abc_ObjAssignName( Abc_Obj_t * pObj, char * pName, char * pSuffix );


#endif
