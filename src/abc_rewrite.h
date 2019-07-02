#ifndef ABC_REWRITE_H
#define ABC_REWRITE_H


#include "abc_ntk.h"
#include "abc_aig.h"
#include "abc_cut.h"
#include "abc_vec.h"
#include "abc_memory.h"
#include "abc_dec.h"
#include "abc_frame.h"


#define RWR_LIMIT  1048576/4  // ((1 << 20)


typedef struct Rwr_Man_t_   Rwr_Man_t;
typedef struct Rwr_Node_t_  Rwr_Node_t;


struct Rwr_Man_t_
{
    // internal lookups
    int                nFuncs;           // number of four var functions
    unsigned short *   puCanons;         // canonical forms
    char *             pPhases;          // canonical phases
    char *             pPerms;           // canonical permutations
    unsigned char *    pMap;             // mapping of functions into class numbers
    unsigned short *   pMapInv;          // mapping of classes into functions
    char *             pPractical;       // practical NPN classes
    char **            pPerms4;          // four-var permutations
    // node space
    Vec_Ptr_t *        vForest;          // all the nodes
    Rwr_Node_t **      pTable;           // the hash table of nodes by their canonical form
    Vec_Vec_t *        vClasses;         // the nodes of the equivalence classes
    Extra_MmFixed_t *  pMmNode;          // memory for nodes and cuts
    // statistical variables
    int                nTravIds;         // the counter of traversal IDs
    int                nConsidered;      // the number of nodes considered
    int                nAdded;           // the number of nodes added to lists
    int                nClasses;         // the number of NN classes
    // the result of resynthesis
    int                fCompl;           // indicates if the output of FF should be complemented
    void *             pGraph;           // the decomposition tree (temporary)
    Vec_Ptr_t *        vFanins;          // the fanins array (temporary)
    Vec_Ptr_t *        vFaninsCur;       // the fanins array (temporary)
    Vec_Int_t *        vLevNums;         // the array of levels (temporary)
    Vec_Ptr_t *        vNodesTemp;       // the nodes in MFFC (temporary)
    // node statistics
    int                nNodesConsidered;
    int                nNodesRewritten;
    int                nNodesGained;
    int                nNodesBeg;
    int                nNodesEnd;
    int                nScores[222];
    int                nCutsGood;
    int                nCutsBad;
    int                nSubgraphs;
    // runtime statistics
    abctime            timeStart;
    abctime            timeCut;
    abctime            timeRes;
    abctime            timeEval;
    abctime            timeMffc;
    abctime            timeUpdate;
    abctime            timeTotal;
};


struct Rwr_Node_t_ // 24 bytes
{
    int                Id;               // ID
    int                TravId;           // traversal ID
    short              nScore;
    short              nGain;
    short              nAdded;
    unsigned           uTruth : 16;      // truth table
    unsigned           Volume :  8;      // volume
    unsigned           Level  :  6;      // level
    unsigned           fUsed  :  1;      // mark
    unsigned           fExor  :  1;      // mark
    Rwr_Node_t *       p0;               // first child
    Rwr_Node_t *       p1;               // second child
    Rwr_Node_t *       pNext;            // next in the table
};


// manipulation of complemented attributes
static inline int          Rwr_IsComplement( Rwr_Node_t * p )    { return (int )(((ABC_PTRUINT_T)p) & 01);           }
static inline Rwr_Node_t * Rwr_Regular( Rwr_Node_t * p )         { return (Rwr_Node_t *)((ABC_PTRUINT_T)(p) & ~01);  }
static inline Rwr_Node_t * Rwr_Not( Rwr_Node_t * p )             { return (Rwr_Node_t *)((ABC_PTRUINT_T)(p) ^  01);  }
static inline Rwr_Node_t * Rwr_NotCond( Rwr_Node_t * p, int c )  { return (Rwr_Node_t *)((ABC_PTRUINT_T)(p) ^ (c));  }


int Abc_NtkRewrite( Abc_Ntk_t * pNtk, int fUpdateLevel, int fUseZeros, int fVerbose, int fVeryVerbose, int fPlaceEnable );
Rwr_Man_t * Rwr_ManStart( int  fPrecompute );
char * Rwr_ManGetPractical( Rwr_Man_t * p );
Rwr_Node_t * Rwr_ManAddVar( Rwr_Man_t * p, unsigned uTruth, int fPrecompute );
void Rwr_ListAddToTail( Rwr_Node_t ** ppList, Rwr_Node_t * pNode );
void Rwr_ScoresClean( Rwr_Man_t * p );
int Rwr_NodeRewrite( Rwr_Man_t * p, Cut_Man_t * pManCut, Abc_Obj_t * pNode, int fUpdateLevel, int fUseZeros, int fPlaceEnable );
void * Rwr_ManReadDecs( Rwr_Man_t * p );
int Rwr_ManReadCompl( Rwr_Man_t * p );
void Rwr_ManPrintStats( Rwr_Man_t * p );
void Rwr_ScoresReport( Rwr_Man_t * p );
void Rwr_ManStop( Rwr_Man_t * p );
void Rwr_ManPrecompute( Rwr_Man_t * p );
void Rwr_ManWriteToArray( Rwr_Man_t * p );
void Rwr_ManLoadFromArray( Rwr_Man_t * p, int fVerbose );
void Rwr_ManPreprocess( Rwr_Man_t * p );
Dec_Graph_t * Rwr_CutEvaluate( Rwr_Man_t * p, Abc_Obj_t * pRoot, Cut_Cut_t * pCut, Vec_Ptr_t * vFaninsCur, int nNodesSaved, int LevelMax, int * pGainBest, int fPlaceEnable );
int Rwr_ScoresCompare( int * pNum1, int * pNum2 );
int Rwr_ManNodeVolume( Rwr_Man_t * p, Rwr_Node_t * p0, Rwr_Node_t * p1 );
Rwr_Node_t * Rwr_ManTryNode( Rwr_Man_t * p, Rwr_Node_t * p0, Rwr_Node_t * p1, int fExor, int Level, int Volume );
void Rwr_ManIncTravId( Rwr_Man_t * p );
void Rwr_MarkUsed_rec( Rwr_Man_t * p, Rwr_Node_t * pNode );
Rwr_Node_t * Rwr_ManAddNode( Rwr_Man_t * p, Rwr_Node_t * p0, Rwr_Node_t * p1, int fExor, int Level, int Volume );
Dec_Graph_t * Rwr_NodePreprocess( Rwr_Man_t * p, Rwr_Node_t * pNode );
Dec_Edge_t Rwr_TravCollect_rec( Rwr_Man_t * p, Rwr_Node_t * pNode, Dec_Graph_t * pGraph );
void Rwr_Trav_rec( Rwr_Man_t * p, Rwr_Node_t * pNode, int * pVolume );


#endif
