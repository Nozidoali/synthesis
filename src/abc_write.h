#ifndef ABC_WRITE_H
#define ABC_WRITE_H


#include "abc_memory.h"
#include "abc_vec.h"
#include "abc_ntk.h"
#include "abc_extra.h"


#define  IO_WRITE_LINE_LENGTH    78    // the output line length


void Io_WriteBlif( Abc_Ntk_t * pNtk, char * FileName, int fWriteLatches, int fBb2Wb, int fSeq );
void Io_NtkWrite( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches, int fBb2Wb, int fSeq );
void Io_NtkWriteOne( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches, int fBb2Wb, int fSeq );
void Io_NtkWritePis( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches );
void Io_NtkWritePos( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches );
int Io_NtkWriteNode( FILE * pFile, Abc_Obj_t * pNode, int Length );
void Io_NtkWriteNodeFanins( FILE * pFile, Abc_Obj_t * pNode );


#endif
