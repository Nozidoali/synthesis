#ifndef CONTEST_LIMIT_H
#define CONTEST_LIMIT_H


#include "abc_aig.h"


Vec_Ptr_t * Contest_CollectFanoutExceedNodes( Abc_Ntk_t * pNtk, int nLimit, bool isPrint );
void Contest_PrintStats( Abc_Ntk_t * pNtk, bool isInit );
void Contest_LimitFanout( Abc_Ntk_t * pNtk, int limit );
void Contest_LimitFanout_Iter( Abc_Ntk_t * pNtk, int limit );
void Contest_NodeLimitFanout( Abc_Obj_t * pNode, int limit );
bool Contest_LimitCheck( Abc_Ntk_t * pNtk, int limit );
int Contest_SopIsAndType( char * pSop );

int Abc_SopIsBuf( char * pSop );
Abc_Obj_t * Abc_ObjInsertBetween( Abc_Obj_t * pNodeIn, Abc_Obj_t * pNodeOut, Abc_ObjType_t Type );


#endif
