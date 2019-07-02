#ifndef ABC_CONVERT_H
#define ABC_CONVERT_H


#include "abc_ntk.h"
#include "abc_vec.h"
#include "abc_extra.h"
#include "abc_aig.h"


Abc_Ntk_t * Abc_NtkStrash( Abc_Ntk_t * pNtk, int fAllNodes, int fCleanup, int fRecord );
Abc_Ntk_t * Abc_NtkRestrash( Abc_Ntk_t * pNtk, int fCleanup );
int Abc_NtkGetChoiceNum( Abc_Ntk_t * pNtk );
Abc_Ntk_t * Abc_NtkStartFrom( Abc_Ntk_t * pNtk, Abc_NtkType_t Type, Abc_NtkFunc_t Func );
Abc_Ntk_t * Abc_NtkDup( Abc_Ntk_t * pNtk );
int Abc_NtkToAig( Abc_Ntk_t * pNtk );
int Abc_NtkSopToAig( Abc_Ntk_t * pNtk );
void Abc_NtkStrashPerform( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkNew, int fAllNodes, int fRecord );
void Abc_NtkTransferNameIds( Abc_Ntk_t * p, Abc_Ntk_t * pNew );
Abc_Ntk_t * Abc_NtkToLogic( Abc_Ntk_t * pNtk );
Abc_Ntk_t * Abc_NtkAigToLogicSop( Abc_Ntk_t * pNtk );
int Abc_NtkLogicMakeSimpleCos( Abc_Ntk_t * pNtk, int fDuplicate );
int Abc_NtkLogicMakeSimpleCos2( Abc_Ntk_t * pNtk, int fDuplicate );
void Abc_NtkFixCoDriverProblem( Abc_Obj_t * pDriver, Abc_Obj_t * pNodeCo, int fDuplicate );
int Abc_NtkLogicHasSimpleCos( Abc_Ntk_t * pNtk );
Abc_Ntk_t * Abc_NtkToNetlist( Abc_Ntk_t * pNtk );
Abc_Ntk_t * Abc_NtkLogicToNetlist( Abc_Ntk_t * pNtk );

Hop_Obj_t * Abc_ConvertSopToAig( Hop_Man_t * pMan, char * pSop );
Hop_Obj_t * Abc_ConvertSopToAigInternal( Hop_Man_t * pMan, char * pSop );


#endif
