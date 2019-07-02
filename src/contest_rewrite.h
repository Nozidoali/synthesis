#ifndef CONTEST_REWRITE_H
#define CONTEST_REWRITE_H


#include "abc_rewrite.h"
#include <unordered_map>
#include <limits.h>


void Contest_PreComputeStructure( int limit );
Rwr_Man_t * Contest_Rwr_ManStart( int  fPrecompute, int limit );
void Contest_Rwr_ManPrecompute( Rwr_Man_t * p, int limit );
Rwr_Node_t * Contest_Rwr_ManTryNode( Rwr_Man_t * p, Rwr_Node_t * p0, Rwr_Node_t * p1, int fExor, int Level, int Volume, int limit );
bool Contest_CheckLimit( Rwr_Node_t * p0, Rwr_Node_t * p1, int limit );
void Contest_CheckLimit_Rec( Rwr_Node_t * pNode, std::unordered_map < int, int > & rwrNode2FO, int limit, bool & isValid );
void Contest_Rwr_ManLoadFromArray( Rwr_Man_t * p, int fVerbose );
int Contest_NtkRewrite( Abc_Ntk_t * pNtk, int fUpdateLevel, int fUseZeros, int fVerbose, int fVeryVerbose, int fPlaceEnable, int limit );
int Contest_NodeRewrite( Rwr_Man_t * p, Cut_Man_t * pManCut, Abc_Obj_t * pNode, int fUpdateLevel, int fUseZeros, int fPlaceEnable, bool isForced );
Dec_Graph_t * Contest_CutEvaluate( Rwr_Man_t * p, Abc_Obj_t * pRoot, Cut_Cut_t * pCut, Vec_Ptr_t * vFaninsCur, int nNodesSaved, int LevelMax, int * pGainBest, int fPlaceEnable, bool isForced );
int Contest_GraphToNetworkCount( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int NodeMax, int LevelMax, bool isForced );
void Contest_CheckTFILimit_rec( Abc_Obj_t * pObj, unsigned minLevel, int limit, bool & isExceed );


#endif
