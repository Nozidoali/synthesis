#ifndef CKT_VISUAL_H
#define CKT_VISUAL_H


#include "abc_ntk.h"
#include "abc_convert.h"


void            WriteDotNtk           (Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, char * pFileName, int fGateNames);
char *          NtkPrintSop           (char * pSop);
int             NtkCountLogicNodes    (Vec_Ptr_t * vNodes);
void            Ckt_Visualize         (Abc_Ntk_t * pAbcNtk, std::string fileName);


#endif
