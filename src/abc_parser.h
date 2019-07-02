#ifndef ABC_PARSER_H
#define ABC_PARSER_H


#include "abc_memory.h"
#include "abc_vec.h"
#include "abc_ntk.h"
#include "abc_extra.h"


typedef struct Io_ReadBlif_t_        Io_ReadBlif_t;   // all reading info


struct Io_ReadBlif_t_
{
    // general info about file
    char *               pFileName;    // the name of the file
    Extra_FileReader_t * pReader;      // the input file reader
    // current processing info
    Abc_Ntk_t *          pNtkMaster;   // the primary network
    Abc_Ntk_t *          pNtkCur;      // the primary network
    int                  LineCur;      // the line currently parsed
    // temporary storage for tokens
    Vec_Ptr_t *          vTokens;      // the current tokens
    Vec_Ptr_t *          vNewTokens;   // the temporary storage for the tokens
    Vec_Str_t *          vCubes;       // the temporary storage for the tokens
    // timing information
    Vec_Int_t *          vInArrs;      // input arrival
    Vec_Int_t *          vOutReqs;     // output required
    Vec_Int_t *          vInDrives;    // input drive
    Vec_Int_t *          vOutLoads;    // output load
    float                DefInArrRise; // input arrival default
    float                DefInArrFall; // input arrival default
    float                DefOutReqRise;// output required default
    float                DefOutReqFall;// output required default
    float                DefInDriRise; // input drive default
    float                DefInDriFall; // input drive default
    float                DefOutLoadRise;// output load default
    float                DefOutLoadFall;// output load default
    int                  fHaveDefInArr;  // provided in the file
    int                  fHaveDefOutReq; // provided in the file
    int                  fHaveDefInDri;  // provided in the file
    int                  fHaveDefOutLoad;// provided in the file
    // the error message
    FILE *               Output;       // the output stream
    char                 sError[1000]; // the error string generated during parsing
    int                  fError;       // set to 1 when error occurs
};


Abc_Ntk_t * Io_ReadBlif( char * pFileName, int fCheck );
Io_ReadBlif_t * Io_ReadBlifFile( char * pFileName );
Abc_Ntk_t * Io_ReadBlifNetwork( Io_ReadBlif_t * p );
Vec_Ptr_t * Io_ReadBlifGetTokens( Io_ReadBlif_t * p );
void Io_ReadBlifPrintErrorMessage( Io_ReadBlif_t * p );
Abc_Ntk_t * Io_ReadBlifNetworkOne( Io_ReadBlif_t * p );
int Io_ReadBlifNetworkNames( Io_ReadBlif_t * p, Vec_Ptr_t ** pvTokens );
int Io_ReadBlifNetworkInputs( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
Abc_Obj_t * Io_ReadCreatePi( Abc_Ntk_t * pNtk, char * pName );
Abc_Obj_t * Io_ReadCreateNode( Abc_Ntk_t * pNtk, char * pNameOut, char * pNamesIn[], int nInputs );
int Io_ReadBlifNetworkOutputs( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
Abc_Obj_t * Io_ReadCreatePo( Abc_Ntk_t * pNtk, char * pName );
int Io_ReadBlifNetworkInputArrival( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
int Io_ReadBlifNetworkDefaultInputArrival( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
int Io_ReadBlifNetworkOutputRequired( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
int Io_ReadBlifNetworkDefaultOutputRequired( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
int Io_ReadBlifNetworkInputDrive( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
int Io_ReadBlifNetworkOutputLoad( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
int Io_ReadBlifNetworkDefaultInputDrive( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
int Io_ReadBlifNetworkDefaultOutputLoad( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
int Io_ReadBlifNetworkAndGateDelay( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
int Io_ReadFindCiId( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj );
int Io_ReadFindCoId( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj );
void Io_ReadBlifFree( Io_ReadBlif_t * p );


#endif
