#ifndef ABC_AIG_H
#define ABC_AIG_H


#include "abc_memory.h"
#include "abc_ntk.h"
#include "abc_vec.h"
#include "abc_kit.h"
#include "abc_bdc.h"
#include "abc_cut.h"


#define RMAN_MAXVARS  12
#define RMAX_MAXWORD  (RMAN_MAXVARS <= 5 ? 1 : (1 << (RMAN_MAXVARS - 5)))


// iterators through the entries in the linked lists of nodes
#define Abc_AigBinForEachEntry( pBin, pEnt )                   \
    for ( pEnt = pBin;                                         \
          pEnt;                                                \
          pEnt = pEnt->pNext )
#define Abc_AigBinForEachEntrySafe( pBin, pEnt, pEnt2 )        \
    for ( pEnt = pBin,                                         \
          pEnt2 = pEnt? pEnt->pNext: NULL;                     \
          pEnt;                                                \
          pEnt = pEnt2,                                        \
          pEnt2 = pEnt? pEnt->pNext: NULL )


typedef enum {
    AIG_OBJ_NONE,                    // 0: non-existent object
    AIG_OBJ_CONST1,                  // 1: constant 1
    AIG_OBJ_CI,                      // 2: combinational input
    AIG_OBJ_CO,                      // 3: combinational output
    AIG_OBJ_BUF,                     // 4: buffer node
    AIG_OBJ_AND,                     // 5: AND node
    AIG_OBJ_EXOR,                    // 6: EXOR node
    AIG_OBJ_VOID                     // 7: unused object
} Aig_Type_t;


typedef struct Abc_Obj_t_       Abc_Obj_t;
typedef struct Abc_Ntk_t_       Abc_Ntk_t;
typedef struct Abc_Aig_t_       Abc_Aig_t;
typedef struct Aig_Man_t_            Aig_Man_t;
typedef struct Aig_Obj_t_            Aig_Obj_t;
typedef struct Aig_MmFixed_t_        Aig_MmFixed_t;
typedef struct Aig_MmFlex_t_         Aig_MmFlex_t;
typedef struct Aig_MmStep_t_         Aig_MmStep_t;
typedef struct Aig_RMan_t_ Aig_RMan_t;
typedef struct Aig_Tru_t_ Aig_Tru_t;
typedef struct Aig_VSig_t_ Aig_VSig_t;

struct Abc_Aig_t_
{
    Abc_Ntk_t *       pNtkAig;           // the AIG network
    Abc_Obj_t *       pConst1;           // the constant 1 object (not a node!)
    Abc_Obj_t **      pBins;             // the table bins
    int               nBins;             // the size of the table
    int               nEntries;          // the total number of entries in the table
    Vec_Ptr_t *       vNodes;            // the temporary array of nodes
    Vec_Ptr_t *       vStackReplaceOld;  // the nodes to be replaced
    Vec_Ptr_t *       vStackReplaceNew;  // the nodes to be used for replacement
    Vec_Vec_t *       vLevels;           // the nodes to be updated
    Vec_Vec_t *       vLevelsR;          // the nodes to be updated
    Vec_Ptr_t *       vAddedCells;       // the added nodes
    Vec_Ptr_t *       vUpdatedNets;      // the nodes whose fanouts have changed

    int               nStrash0;
    int               nStrash1;
    int               nStrash5;
    int               nStrash2;
};


struct Aig_Obj_t_  // 8 words
{
    union {
        Aig_Obj_t *  pNext;          // strashing table
        int          CioId;          // 0-based number of CI/CO
    };
    Aig_Obj_t *      pFanin0;        // fanin
    Aig_Obj_t *      pFanin1;        // fanin
    unsigned int     Type    :  3;   // object type
    unsigned int     fPhase  :  1;   // value under 000...0 pattern
    unsigned int     fMarkA  :  1;   // multipurpose mask
    unsigned int     fMarkB  :  1;   // multipurpose mask
    unsigned int     nRefs   : 26;   // reference count
    unsigned         Level   : 24;   // the level of this node
    unsigned         nCuts   :  8;   // the number of cuts
    int              TravId;         // unique ID of last traversal involving the node
    int              Id;             // unique ID of the node
    union {                          // temporary store for user's data
        void *       pData;
        int          iData;
        float        dData;
    };
};


struct Aig_Man_t_
{
    char *           pName;          // the design name
    char *           pSpec;          // the input file name
    // AIG nodes
    Vec_Ptr_t *      vCis;           // the array of PIs
    Vec_Ptr_t *      vCos;           // the array of POs
    Vec_Ptr_t *      vObjs;          // the array of all nodes (optional)
    Vec_Ptr_t *      vBufs;          // the array of buffers
    Aig_Obj_t *      pConst1;        // the constant 1 node
    Aig_Obj_t        Ghost;          // the ghost node
    int              nRegs;          // the number of registers (registers are last POs)
    int              nTruePis;       // the number of true primary inputs
    int              nTruePos;       // the number of true primary outputs
    int              nAsserts;       // the number of asserts among POs (asserts are first POs)
    int              nConstrs;       // the number of constraints (model checking only)
    int              nBarBufs;       // the number of barrier buffers
    // AIG node counters
    int              nObjs[AIG_OBJ_VOID];// the number of objects by type
    int              nDeleted;       // the number of deleted objects
    // structural hash table
    Aig_Obj_t **     pTable;         // structural hash table
    int              nTableSize;     // structural hash table size
    // representation of fanouts
    int *            pFanData;       // the database to store fanout information
    int              nFansAlloc;     // the size of fanout representation
    Vec_Vec_t *      vLevels;        // used to update timing information
    int              nBufReplaces;   // the number of times replacement led to a buffer
    int              nBufFixes;      // the number of times buffers were propagated
    int              nBufMax;        // the maximum number of buffers during computation
    // topological order
    unsigned *       pOrderData;
    int              nOrderAlloc;
    int              iPrev;
    int              iNext;
    int              nAndTotal;
    int              nAndPrev;
    // representatives
    Aig_Obj_t **     pEquivs;        // linked list of equivalent nodes (when choices are used)
    Aig_Obj_t **     pReprs;         // representatives of each node
    int              nReprsAlloc;    // the number of allocated representatives
    // various data members
    Aig_MmFixed_t *  pMemObjs;       // memory manager for objects
    Vec_Int_t *      vLevelR;        // the reverse level of the nodes
    int              nLevelMax;      // maximum number of levels
    void *           pData;          // the temporary data
    void *           pData2;         // the temporary data
    int              nTravIds;       // the current traversal ID
    int              fCatchExor;     // enables EXOR nodes
    int              fAddStrash;     // performs additional strashing
    Aig_Obj_t **     pObjCopies;     // mapping of AIG nodes into FRAIG nodes
    void (*pImpFunc) (void*, void*); // implication checking precedure
    void *           pImpData;       // implication checking data
    void *           pManTime;       // the timing manager
    void *           pManCuts;
    int *            pFastSim;
    unsigned *       pTerSimData;    // ternary simulation data
    Vec_Ptr_t *      vMapped;
    Vec_Int_t *      vFlopNums;
    Vec_Int_t *      vFlopReprs;
    // Abc_Cex_t *      pSeqModel;
    Vec_Ptr_t *      vSeqModelVec;   // vector of counter-examples (for sequential miters)
    Aig_Man_t *      pManExdc;
    Vec_Ptr_t *      vOnehots;
    int              fCreatePios;
    Vec_Int_t *      vEquPairs;
    Vec_Vec_t *      vClockDoms;
    Vec_Int_t *      vProbs;         // probability of node being 1
    Vec_Int_t *      vCiNumsOrig;    // original CI names
    int              nComplEdges;    // complemented edges
    abctime          Time2Quit;
    // timing statistics
    abctime          time1;
    abctime          time2;
  //-- jlong -- begin
  Vec_Ptr_t *      unfold2_type_I;
  Vec_Ptr_t *      unfold2_type_II;
  //-- jlong -- end
};


struct Aig_MmFixed_t_
{
    // information about individual entries
    int           nEntrySize;    // the size of one entry
    int           nEntriesAlloc; // the total number of entries allocated
    int           nEntriesUsed;  // the number of entries in use
    int           nEntriesMax;   // the max number of entries in use
    char *        pEntriesFree;  // the linked list of free entries

    // this is where the memory is stored
    int           nChunkSize;    // the size of one chunk
    int           nChunksAlloc;  // the maximum number of memory chunks
    int           nChunks;       // the current number of memory chunks
    char **       pChunks;       // the allocated memory

    // statistics
    int           nMemoryUsed;   // memory used in the allocated entries
    int           nMemoryAlloc;  // memory allocated
};


struct Aig_MmFlex_t_
{
    // information about individual entries
    int           nEntriesUsed;  // the number of entries allocated
    char *        pCurrent;      // the current pointer to free memory
    char *        pEnd;          // the first entry outside the free memory

    // this is where the memory is stored
    int           nChunkSize;    // the size of one chunk
    int           nChunksAlloc;  // the maximum number of memory chunks
    int           nChunks;       // the current number of memory chunks
    char **       pChunks;       // the allocated memory

    // statistics
    int           nMemoryUsed;   // memory used in the allocated entries
    int           nMemoryAlloc;  // memory allocated
};


struct Aig_MmStep_t_
{
    int               nMems;    // the number of fixed memory managers employed
    Aig_MmFixed_t **  pMems;    // memory managers: 2^1 words, 2^2 words, etc
    int               nMapSize; // the size of the memory array
    Aig_MmFixed_t **  pMap;     // maps the number of bytes into its memory manager
    // additional memory chunks
    int           nChunksAlloc;  // the maximum number of memory chunks
    int           nChunks;       // the current number of memory chunks
    char **       pChunks;       // the allocated memory
};

struct Aig_VSig_t_
{
    int           nOnes;
    int           nCofOnes[RMAN_MAXVARS];
};


struct Aig_Tru_t_
{
    Aig_Tru_t *   pNext;
    int           Id;
    unsigned      nVisits : 27;
    unsigned      nVars   :  5;
    unsigned      pTruth[0];
};


struct Aig_RMan_t_
{
    int           nVars;       // the largest variable number
    Aig_Man_t *   pAig;        // recorded subgraphs
    // hash table
    int           nBins;
    Aig_Tru_t **  pBins;
    int           nEntries;
    Aig_MmFlex_t* pMemTrus;
    // bidecomposion
    Bdc_Man_t *   pBidec;
    // temporaries
    unsigned      pTruthInit[RMAX_MAXWORD]; // canonical truth table
    unsigned      pTruth[RMAX_MAXWORD];     // current truth table
    unsigned      pTruthC[RMAX_MAXWORD];    // canonical truth table
    unsigned      pTruthTemp[RMAX_MAXWORD]; // temporary truth table
    Aig_VSig_t    pMints[2*RMAN_MAXVARS];   // minterm count
    char          pPerm[RMAN_MAXVARS];      // permutation
    char          pPermR[RMAN_MAXVARS];     // reverse permutation
    // statistics
    int           nVarFuncs[RMAN_MAXVARS+1];
    int           nTotal;
    int           nTtDsd;
    int           nTtDsdPart;
    int           nTtDsdNot;
    int           nUniqueVars;
};


static inline Aig_Obj_t *  Aig_Regular( Aig_Obj_t * p )           { return (Aig_Obj_t *)((ABC_PTRUINT_T)(p) & ~01);  }
static inline Aig_Obj_t *  Aig_Not( Aig_Obj_t * p )               { return (Aig_Obj_t *)((ABC_PTRUINT_T)(p) ^  01);  }
static inline Aig_Obj_t *  Aig_NotCond( Aig_Obj_t * p, int c )    { return (Aig_Obj_t *)((ABC_PTRUINT_T)(p) ^ (c));  }
static inline int          Aig_IsComplement( Aig_Obj_t * p )      { return (int)((ABC_PTRUINT_T)(p) & 01);           }

static inline int          Aig_ManCiNum( Aig_Man_t * p )          { return p->nObjs[AIG_OBJ_CI];                     }
static inline int          Aig_ManCoNum( Aig_Man_t * p )          { return p->nObjs[AIG_OBJ_CO];                     }
static inline int          Aig_ManBufNum( Aig_Man_t * p )         { return p->nObjs[AIG_OBJ_BUF];                    }
static inline int          Aig_ManAndNum( Aig_Man_t * p )         { return p->nObjs[AIG_OBJ_AND];                    }
static inline int          Aig_ManExorNum( Aig_Man_t * p )        { return p->nObjs[AIG_OBJ_EXOR];                   }
static inline int          Aig_ManNodeNum( Aig_Man_t * p )        { return p->nObjs[AIG_OBJ_AND]+p->nObjs[AIG_OBJ_EXOR];   }
static inline int          Aig_ManGetCost( Aig_Man_t * p )        { return p->nObjs[AIG_OBJ_AND]+3*p->nObjs[AIG_OBJ_EXOR]; }
static inline int          Aig_ManObjNum( Aig_Man_t * p )         { return Vec_PtrSize(p->vObjs) - p->nDeleted;      }
static inline int          Aig_ManObjNumMax( Aig_Man_t * p )      { return Vec_PtrSize(p->vObjs);                    }
static inline int          Aig_ManRegNum( Aig_Man_t * p )         { return p->nRegs;                                 }
static inline int          Aig_ManConstrNum( Aig_Man_t * p )      { return p->nConstrs;                              }

static inline Aig_Obj_t *  Aig_ManConst0( Aig_Man_t * p )         { return Aig_Not(p->pConst1);                      }
static inline Aig_Obj_t *  Aig_ManConst1( Aig_Man_t * p )         { return p->pConst1;                               }
static inline Aig_Obj_t *  Aig_ManGhost( Aig_Man_t * p )          { return &p->Ghost;                                }
static inline Aig_Obj_t *  Aig_ManCi( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vCis, i);    }
static inline Aig_Obj_t *  Aig_ManCo( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vCos, i);    }
static inline Aig_Obj_t *  Aig_ManLo( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vCis, Aig_ManCiNum(p)-Aig_ManRegNum(p)+i);   }
static inline Aig_Obj_t *  Aig_ManLi( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vCos, Aig_ManCoNum(p)-Aig_ManRegNum(p)+i);   }
static inline Aig_Obj_t *  Aig_ManObj( Aig_Man_t * p, int i )     { return p->vObjs ? (Aig_Obj_t *)Vec_PtrEntry(p->vObjs, i) : NULL;  }

static inline Aig_Type_t   Aig_ObjType( Aig_Obj_t * pObj )        { return (Aig_Type_t)pObj->Type;       }
static inline int          Aig_ObjIsNone( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_NONE;   }
static inline int          Aig_ObjIsConst1( Aig_Obj_t * pObj )    { assert(!Aig_IsComplement(pObj)); return pObj->Type == AIG_OBJ_CONST1; }
static inline int          Aig_ObjIsCi( Aig_Obj_t * pObj )        { return pObj->Type == AIG_OBJ_CI;     }
static inline int          Aig_ObjIsCo( Aig_Obj_t * pObj )        { return pObj->Type == AIG_OBJ_CO;     }
static inline int          Aig_ObjIsBuf( Aig_Obj_t * pObj )       { return pObj->Type == AIG_OBJ_BUF;    }
static inline int          Aig_ObjIsAnd( Aig_Obj_t * pObj )       { return pObj->Type == AIG_OBJ_AND;    }
static inline int          Aig_ObjIsExor( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_EXOR;   }
static inline int          Aig_ObjIsNode( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_AND || pObj->Type == AIG_OBJ_EXOR;   }
static inline int          Aig_ObjIsTerm( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_CI  || pObj->Type == AIG_OBJ_CO || pObj->Type == AIG_OBJ_CONST1;   }
static inline int          Aig_ObjIsHash( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_AND || pObj->Type == AIG_OBJ_EXOR;                                 }
static inline int          Aig_ObjIsChoice( Aig_Man_t * p, Aig_Obj_t * pObj )    { return p->pEquivs && p->pEquivs[pObj->Id] && pObj->nRefs > 0;                    }
static inline int          Aig_ObjIsCand( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_CI || pObj->Type == AIG_OBJ_AND || pObj->Type == AIG_OBJ_EXOR;     }
static inline int          Aig_ObjCioId( Aig_Obj_t * pObj )       { assert( !Aig_ObjIsNode(pObj) ); return pObj->CioId;                                            }
static inline int          Aig_ObjId( Aig_Obj_t * pObj )          { return pObj->Id;                     }

static inline int          Aig_ObjIsMarkA( Aig_Obj_t * pObj )     { return pObj->fMarkA;  }
static inline void         Aig_ObjSetMarkA( Aig_Obj_t * pObj )    { pObj->fMarkA = 1;     }
static inline void         Aig_ObjClearMarkA( Aig_Obj_t * pObj )  { pObj->fMarkA = 0;     }

static inline void         Aig_ObjSetTravId( Aig_Obj_t * pObj, int TravId )                { pObj->TravId = TravId;                         }
static inline void         Aig_ObjSetTravIdCurrent( Aig_Man_t * p, Aig_Obj_t * pObj )      { pObj->TravId = p->nTravIds;                    }
static inline void         Aig_ObjSetTravIdPrevious( Aig_Man_t * p, Aig_Obj_t * pObj )     { pObj->TravId = p->nTravIds - 1;                }
static inline int          Aig_ObjIsTravIdCurrent( Aig_Man_t * p, Aig_Obj_t * pObj )       { return (int)(pObj->TravId == p->nTravIds);     }
static inline int          Aig_ObjIsTravIdPrevious( Aig_Man_t * p, Aig_Obj_t * pObj )      { return (int)(pObj->TravId == p->nTravIds - 1); }

static inline int          Aig_ObjPhase( Aig_Obj_t * pObj )       { return pObj->fPhase;                           }
static inline int          Aig_ObjPhaseReal( Aig_Obj_t * pObj )   { return pObj? Aig_Regular(pObj)->fPhase ^ Aig_IsComplement(pObj) : 1;                              }
static inline int          Aig_ObjRefs( Aig_Obj_t * pObj )        { return pObj->nRefs;                            }
static inline void         Aig_ObjRef( Aig_Obj_t * pObj )         { pObj->nRefs++;                                 }
static inline void         Aig_ObjDeref( Aig_Obj_t * pObj )       { assert( pObj->nRefs > 0 ); pObj->nRefs--;      }
static inline void         Aig_ObjClearRef( Aig_Obj_t * pObj )    { pObj->nRefs = 0;                               }
static inline int          Aig_ObjFaninId0( Aig_Obj_t * pObj )    { return pObj->pFanin0? Aig_Regular(pObj->pFanin0)->Id : -1; }
static inline int          Aig_ObjFaninId1( Aig_Obj_t * pObj )    { return pObj->pFanin1? Aig_Regular(pObj->pFanin1)->Id : -1; }
static inline int          Aig_ObjFaninC0( Aig_Obj_t * pObj )     { return Aig_IsComplement(pObj->pFanin0);        }
static inline int          Aig_ObjFaninC1( Aig_Obj_t * pObj )     { return Aig_IsComplement(pObj->pFanin1);        }
static inline Aig_Obj_t *  Aig_ObjFanin0( Aig_Obj_t * pObj )      { return Aig_Regular(pObj->pFanin0);             }
static inline Aig_Obj_t *  Aig_ObjFanin1( Aig_Obj_t * pObj )      { return Aig_Regular(pObj->pFanin1);             }
static inline Aig_Obj_t *  Aig_ObjChild0( Aig_Obj_t * pObj )      { return pObj->pFanin0;                          }
static inline Aig_Obj_t *  Aig_ObjChild1( Aig_Obj_t * pObj )      { return pObj->pFanin1;                          }
static inline Aig_Obj_t *  Aig_ObjChild0Copy( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin0(pObj)? Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t *  Aig_ObjChild1Copy( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin1(pObj)->pData, Aig_ObjFaninC1(pObj)) : NULL;  }
static inline Aig_Obj_t *  Aig_ObjChild0Next( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin0(pObj)? Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObj)->pNext, Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t *  Aig_ObjChild1Next( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin1(pObj)->pNext, Aig_ObjFaninC1(pObj)) : NULL;  }
static inline void         Aig_ObjChild0Flip( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); pObj->pFanin0 = Aig_Not(pObj->pFanin0);        }
static inline void         Aig_ObjChild1Flip( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); pObj->pFanin1 = Aig_Not(pObj->pFanin1);        }
static inline Aig_Obj_t *  Aig_ObjCopy( Aig_Obj_t * pObj )        { assert( !Aig_IsComplement(pObj) ); return (Aig_Obj_t *)pObj->pData;               }
static inline void         Aig_ObjSetCopy( Aig_Obj_t * pObj, Aig_Obj_t * pCopy )     {  assert( !Aig_IsComplement(pObj) ); pObj->pData = pCopy;       }
static inline Aig_Obj_t *  Aig_ObjRealCopy( Aig_Obj_t * pObj )    { return Aig_NotCond((Aig_Obj_t *)Aig_Regular(pObj)->pData, Aig_IsComplement(pObj));}
static inline int          Aig_ObjToLit( Aig_Obj_t * pObj )       { return Abc_Var2Lit( Aig_ObjId(Aig_Regular(pObj)), Aig_IsComplement(pObj) );       }
static inline Aig_Obj_t *  Aig_ObjFromLit( Aig_Man_t * p,int iLit){ return Aig_NotCond( Aig_ManObj(p, Abc_Lit2Var(iLit)), Abc_LitIsCompl(iLit) );     }
static inline int          Aig_ObjLevel( Aig_Obj_t * pObj )       { assert( !Aig_IsComplement(pObj) ); return pObj->Level;                            }
static inline int          Aig_ObjLevelNew( Aig_Obj_t * pObj )    { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? 1 + Aig_ObjIsExor(pObj) + Abc_MaxInt(Aig_ObjFanin0(pObj)->Level, Aig_ObjFanin1(pObj)->Level) : Aig_ObjFanin0(pObj)->Level; }
static inline int          Aig_ObjSetLevel( Aig_Obj_t * pObj, int i ) { assert( !Aig_IsComplement(pObj) ); return pObj->Level = i;                    }
static inline void         Aig_ObjClean( Aig_Obj_t * pObj )       { memset( pObj, 0, sizeof(Aig_Obj_t) );                                                             }
static inline Aig_Obj_t *  Aig_ObjFanout0( Aig_Man_t * p, Aig_Obj_t * pObj )  { assert(p->pFanData && pObj->Id < p->nFansAlloc); return Aig_ManObj(p, p->pFanData[5*pObj->Id] >> 1); }
static inline Aig_Obj_t *  Aig_ObjEquiv( Aig_Man_t * p, Aig_Obj_t * pObj )    { return p->pEquivs? p->pEquivs[pObj->Id] : NULL;           }
static inline void         Aig_ObjSetEquiv( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pEqu ) { assert(p->pEquivs); p->pEquivs[pObj->Id] = pEqu;                  }
static inline Aig_Obj_t *  Aig_ObjRepr( Aig_Man_t * p, Aig_Obj_t * pObj )     { return p->pReprs? p->pReprs[pObj->Id] : NULL;             }
static inline void         Aig_ObjSetRepr( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pRepr )     { assert(p->pReprs); p->pReprs[pObj->Id] = pRepr;                                }
static inline int          Aig_ObjWhatFanin( Aig_Obj_t * pObj, Aig_Obj_t * pFanin )
{
    if ( Aig_ObjFanin0(pObj) == pFanin ) return 0;
    if ( Aig_ObjFanin1(pObj) == pFanin ) return 1;
    assert(0); return -1;
}
static inline int          Aig_ObjFanoutC( Aig_Obj_t * pObj, Aig_Obj_t * pFanout )
{
    if ( Aig_ObjFanin0(pFanout) == pObj ) return Aig_ObjFaninC0(pObj);
    if ( Aig_ObjFanin1(pFanout) == pObj ) return Aig_ObjFaninC1(pObj);
    assert(0); return -1;
}
static inline Aig_Obj_t *  Aig_ObjCreateGhost( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1, Aig_Type_t Type )
{
    Aig_Obj_t * pGhost;
    assert( Type != AIG_OBJ_AND || !Aig_ObjIsConst1(Aig_Regular(p0)) );
    assert( p1 == NULL || !Aig_ObjIsConst1(Aig_Regular(p1)) );
    assert( Type == AIG_OBJ_CI || Aig_Regular(p0) != Aig_Regular(p1) );
    pGhost = Aig_ManGhost(p);
    pGhost->Type = Type;
    if ( p1 == NULL || Aig_Regular(p0)->Id < Aig_Regular(p1)->Id )
    {
        pGhost->pFanin0 = p0;
        pGhost->pFanin1 = p1;
    }
    else
    {
        pGhost->pFanin0 = p1;
        pGhost->pFanin1 = p0;
    }
    return pGhost;
}

Abc_Obj_t * Abc_AigConst1( Abc_Ntk_t * pNtk );
Abc_Obj_t * Abc_AigAnd( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
Abc_Obj_t * Abc_AigAndLookup( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
Abc_Obj_t * Abc_AigAndCreate( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
void Abc_AigResize( Abc_Aig_t * pMan );
int Abc_AigCleanup( Abc_Aig_t * pMan );
void Abc_AigDeleteNode( Abc_Aig_t * pMan, Abc_Obj_t * pNode );
void Abc_AigAndDelete( Abc_Aig_t * pMan, Abc_Obj_t * pThis );
void Abc_AigRemoveFromLevelStructureR( Vec_Vec_t * vStruct, Abc_Obj_t * pNode );
void Abc_AigRemoveFromLevelStructure( Vec_Vec_t * vStruct, Abc_Obj_t * pNode );
Abc_Aig_t * Abc_AigAlloc( Abc_Ntk_t * pNtkAig );
int Abc_AigCheck( Abc_Aig_t * pMan );
void Abc_AigFree( Abc_Aig_t * pMan );
void Aig_RManRecord( unsigned * pTruth, int nVarsInit );
Aig_RMan_t * Aig_RManStart();
unsigned Aig_RManSemiCanonicize( unsigned * pOut, unsigned * pIn, int nVars, char * pCanonPerm, Aig_VSig_t * pSigs, int fReturnIn );
int Aig_RManVarsAreUnique( Aig_VSig_t * pMints, int nVars );
int Aig_RManTableFindOrAdd( Aig_RMan_t * p, unsigned * pTruth, int nVars );
Aig_Tru_t ** Aig_RManTableLookup( Aig_RMan_t * p, unsigned * pTruth, int nVars );
void Aig_RManSaveOne( Aig_RMan_t * p, unsigned * pTruth, int nVars );
Aig_Man_t * Aig_ManStart( int nNodesMax );
Aig_MmFixed_t * Aig_MmFixedStart( int nEntrySize, int nEntriesMax );
void Aig_RManComputeVSigs( unsigned * pTruth, int nVars, Aig_VSig_t * pSigs, unsigned * pAux );
void Aig_RManSortNums( int * pArray, int nVars );
Aig_Obj_t * Aig_IthVar( Aig_Man_t * p, int i );
Aig_Obj_t * Aig_ObjCreateCi( Aig_Man_t * p );
Aig_MmFlex_t * Aig_MmFlexStart();
void Abc_AigResize( Abc_Aig_t * pMan );
void Aig_RManTableResize( Aig_RMan_t * p );
char * Aig_MmFlexEntryFetch( Aig_MmFlex_t * p, int nBytes );
Aig_Obj_t * Aig_ObjCreateCo( Aig_Man_t * p, Aig_Obj_t * pDriver );
void Aig_ObjConnect( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFan0, Aig_Obj_t * pFan1 );
void Aig_ObjAddFanout( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFanout );
void Aig_TableInsert( Aig_Man_t * p, Aig_Obj_t * pObj );
Aig_Obj_t * Aig_TableLookup( Aig_Man_t * p, Aig_Obj_t * pGhost );
void Aig_TableResize( Aig_Man_t * p );
Aig_Obj_t * Aig_And( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 );
Aig_Obj_t * Aig_Exor( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 );
Aig_Obj_t * Aig_ObjCreate( Aig_Man_t * p, Aig_Obj_t * pGhost );
Aig_Obj_t * Aig_Or( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 );
char * Aig_MmFixedEntryFetch( Aig_MmFixed_t * p );
void Abc_AigUpdateReset( Abc_Aig_t * pMan );
void Abc_AigRehash( Abc_Aig_t * pMan );
void Abc_AigReplace( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, int fUpdateLevel );
void Abc_AigReplace_int( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, int fUpdateLevel );
void Abc_AigUpdateLevel_int( Abc_Aig_t * pMan );
void Abc_AigUpdateLevelR_int( Abc_Aig_t * pMan );
Vec_Ptr_t * Abc_AigDfs( Abc_Ntk_t * pNtk, int fCollectAll, int fCollectCos );
void Abc_AigDfs_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
Abc_Obj_t * Abc_AigAndCreateFrom( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1, Abc_Obj_t * pAnd );
int Abc_AigNodeIsAcyclic( Abc_Obj_t * pNode, Abc_Obj_t * pRoot );


#endif
