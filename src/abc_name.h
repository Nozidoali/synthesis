#ifndef ABC_NAME_H
#define ABC_NAME_H


#include "abc_memory.h"
#include "abc_vec.h"
#include "abc_extra.h"


typedef struct Nm_Man_t_ Nm_Man_t;
typedef struct Nm_Entry_t_ Nm_Entry_t;


struct Nm_Man_t_
{
    Nm_Entry_t **    pBinsI2N;      // mapping IDs into names
    Nm_Entry_t **    pBinsN2I;      // mapping names into IDs
    int              nBins;         // the number of bins in tables
    int              nEntries;      // the number of entries
    int              nSizeFactor;   // determined how much larger the table should be
    int              nGrowthFactor; // determined how much the table grows after resizing
    Extra_MmFlex_t * pMem;          // memory manager for entries (and names)
};


struct Nm_Entry_t_
{
    unsigned         Type;          // object type
    unsigned         ObjId;         // object ID
    Nm_Entry_t *     pNextI2N;      // the next entry in the ID hash table
    Nm_Entry_t *     pNextN2I;      // the next entry in the name hash table
    Nm_Entry_t *     pNameSake;     // the next entry with the same name
    char             Name[0];       // name of the object
};


Nm_Man_t * Nm_ManCreate( int nSize );
void Nm_ManFree( Nm_Man_t * p );
int Nm_ManFindIdByName( Nm_Man_t * p, char * pName, int Type );
Nm_Entry_t * Nm_ManTableLookupName( Nm_Man_t * p, char * pName, int Type );
char * Nm_ManStoreIdName( Nm_Man_t * p, int ObjId, int Type, char * pName, char * pSuffix );
Nm_Entry_t * Nm_ManTableLookupId( Nm_Man_t * p, int ObjId );
int Nm_ManTableAdd( Nm_Man_t * p, Nm_Entry_t * pEntry );
void Nm_ManResize( Nm_Man_t * p );
char * Nm_ManCreateUniqueName( Nm_Man_t * p, int ObjId );
char * Nm_ManFindNameById( Nm_Man_t * p, int ObjId );
void Nm_ManDeleteIdName( Nm_Man_t * p, int ObjId );
int Nm_ManTableDelete( Nm_Man_t * p, int ObjId );
Vec_Int_t * Nm_ManReturnNameIds( Nm_Man_t * p );
int Nm_ManFindIdByNameTwoTypes( Nm_Man_t * p, char * pName, int Type1, int Type2 );
Vec_Ptr_t * Abc_NodeGetFakeNames( int nNames );
void Abc_NodeFreeNames( Vec_Ptr_t * vNames );


#endif
