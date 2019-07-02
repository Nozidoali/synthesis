#include "abc_parser.h"


Abc_Ntk_t * Io_ReadBlif( char * pFileName, int fCheck )
{
    Io_ReadBlif_t * p;
    Abc_Ntk_t * pNtk;

    // start the file
    p = Io_ReadBlifFile( pFileName );
    if ( p == NULL )
        return NULL;

    // read the hierarchical network
    pNtk = Io_ReadBlifNetwork( p );
    if ( pNtk == NULL )
    {
        Io_ReadBlifFree( p );
        return NULL;
    }
    pNtk->pSpec = Extra_UtilStrsav( pFileName );
    // Io_ReadBlifCreateTiming( p, pNtk );
    Io_ReadBlifFree( p );

    // make sure that everything is okay with the network structure
    if ( fCheck && !Abc_NtkCheckRead( pNtk ) )
    {
        printf( "Io_ReadBlif: The network check has failed.\n" );
        Abc_NtkDelete( pNtk );
        return NULL;
    }
    return pNtk;
}


Io_ReadBlif_t * Io_ReadBlifFile( char * pFileName )
{
    Extra_FileReader_t * pReader;
    Io_ReadBlif_t * p;

    // start the reader
    pReader = Extra_FileReaderAlloc( pFileName, const_cast < char * >( "#" ), const_cast < char * >( "\n\r" ), const_cast < char * >( " \t" ) );

    if ( pReader == NULL )
        return NULL;

    // start the reading data structure
    p = ABC_ALLOC( Io_ReadBlif_t, 1 );
    memset( p, 0, sizeof(Io_ReadBlif_t) );
    p->pFileName  = pFileName;
    p->pReader    = pReader;
    p->Output     = stdout;
    p->vNewTokens = Vec_PtrAlloc( 100 );
    p->vCubes     = Vec_StrAlloc( 100 );
    p->vInArrs    = Vec_IntAlloc( 100 );
    p->vOutReqs   = Vec_IntAlloc( 100 );
    p->vInDrives  = Vec_IntAlloc( 100 );
    p->vOutLoads  = Vec_IntAlloc( 100 );
    return p;
}


Abc_Ntk_t * Io_ReadBlifNetwork( Io_ReadBlif_t * p )
{
    Abc_Ntk_t * pNtk, * pNtkMaster;

    // read the name of the master network
    p->vTokens = Io_ReadBlifGetTokens(p);
    if ( p->vTokens == NULL || strcmp( (char *)p->vTokens->pArray[0], ".model" ) )
    {
        p->LineCur = 0;
        sprintf( p->sError, "Wrong input file format." );
        Io_ReadBlifPrintErrorMessage( p );
        return NULL;
    }

    // read networks (with EXDC)
    pNtkMaster = NULL;
    while ( p->vTokens )
    {
        // read the network and its EXDC if present
        pNtk = Io_ReadBlifNetworkOne( p );
        if ( pNtk == NULL )
            break;
        if ( p->vTokens && strcmp((char *)p->vTokens->pArray[0], ".exdc") == 0 )
        {
            pNtk->pExdc = Io_ReadBlifNetworkOne( p );
            if ( pNtk->pExdc == NULL )
                break;
            Abc_NtkFinalizeRead( pNtk->pExdc );
        }
        // add this network as part of the hierarchy
        if ( pNtkMaster == NULL ) // no master network so far
        {
            p->pNtkMaster = pNtkMaster = pNtk;
            continue;
        }
    }

    if ( !p->fError )
        Abc_NtkFinalizeRead( pNtkMaster );
    // return the master network
    return pNtkMaster;
}


Vec_Ptr_t * Io_ReadBlifGetTokens( Io_ReadBlif_t * p )
{
    Vec_Ptr_t * vTokens;
    char * pLastToken;
    int i;

    // get rid of the old tokens
    if ( p->vNewTokens->nSize > 0 )
    {
        for ( i = 0; i < p->vNewTokens->nSize; i++ )
            ABC_FREE( p->vNewTokens->pArray[i] );
        p->vNewTokens->nSize = 0;
    }

    // get the new tokens
    vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p->pReader);
    if ( vTokens == NULL )
        return vTokens;

    // check if there is a transfer to another line
    pLastToken = (char *)vTokens->pArray[vTokens->nSize - 1];
    if ( pLastToken[ strlen(pLastToken)-1 ] != '\\' )
        return vTokens;

    // remove the slash
    pLastToken[ strlen(pLastToken)-1 ] = 0;
    if ( pLastToken[0] == 0 )
        vTokens->nSize--;
    // load them into the new array
    for ( i = 0; i < vTokens->nSize; i++ )
        Vec_PtrPush( p->vNewTokens, Extra_UtilStrsav((char *)vTokens->pArray[i]) );

    // load as long as there is the line break
    while ( 1 )
    {
        // get the new tokens
        vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p->pReader);
        if ( vTokens->nSize == 0 )
            return p->vNewTokens;
        // check if there is a transfer to another line
        pLastToken = (char *)vTokens->pArray[vTokens->nSize - 1];
        if ( pLastToken[ strlen(pLastToken)-1 ] == '\\' )
        {
            // remove the slash
            pLastToken[ strlen(pLastToken)-1 ] = 0;
            if ( pLastToken[0] == 0 )
                vTokens->nSize--;
            // load them into the new array
            for ( i = 0; i < vTokens->nSize; i++ )
                Vec_PtrPush( p->vNewTokens, Extra_UtilStrsav((char *)vTokens->pArray[i]) );
            continue;
        }
        // otherwise, load them and break
        for ( i = 0; i < vTokens->nSize; i++ )
            Vec_PtrPush( p->vNewTokens, Extra_UtilStrsav((char *)vTokens->pArray[i]) );
        break;
    }
    return p->vNewTokens;
}

void Io_ReadBlifPrintErrorMessage( Io_ReadBlif_t * p )
{
    p->fError = 1;
    if ( p->LineCur == 0 ) // the line number is not given
        fprintf( p->Output, "%s: %s\n", p->pFileName, p->sError );
    else // print the error message with the line number
        fprintf( p->Output, "%s (line %d): %s\n", p->pFileName, p->LineCur, p->sError );
}


Abc_Ntk_t * Io_ReadBlifNetworkOne( Io_ReadBlif_t * p )
{
    // ProgressBar * pProgress = NULL;
    Abc_Ntk_t * pNtk;
    char * pDirective;
    int iLine, fTokensReady, fStatus;

    // make sure the tokens are present
    assert( p->vTokens != NULL );

    // create the new network
    p->pNtkCur = pNtk = Abc_NtkAlloc( ABC_NTK_NETLIST, ABC_FUNC_SOP, 1 );
    // read the model name
    if ( strcmp( (char *)p->vTokens->pArray[0], ".model" ) == 0 )
    {
        char * pToken, * pPivot;
        if ( Vec_PtrSize(p->vTokens) != 2 )
        {
            p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
            sprintf( p->sError, "The .model line does not have exactly two entries." );
            Io_ReadBlifPrintErrorMessage( p );
            return NULL;
        }
        for ( pPivot = pToken = (char *)Vec_PtrEntry(p->vTokens, 1); *pToken; pToken++ )
            if ( *pToken == '/' || *pToken == '\\' )
                pPivot = pToken+1;
        pNtk->pName = Extra_UtilStrsav( pPivot );
    }
    else if ( strcmp( (char *)p->vTokens->pArray[0], ".exdc" ) != 0 )
    {
        printf( "%s: File parsing skipped after line %d (\"%s\").\n", p->pFileName,
            Extra_FileReaderGetLineNumber(p->pReader, 0), (char*)p->vTokens->pArray[0] );
        Abc_NtkDelete(pNtk);
        p->pNtkCur = NULL;
        return NULL;
    }

    // read the inputs/outputs
    // if ( p->pNtkMaster == NULL )
    //     pProgress = Extra_ProgressBarStart( stdout, Extra_FileReaderGetFileSize(p->pReader) );
    fTokensReady = fStatus = 0;
    for ( iLine = 0; fTokensReady || (p->vTokens = Io_ReadBlifGetTokens(p)); iLine++ )
    {
//         if ( p->pNtkMaster == NULL && iLine % 1000 == 0 )
//             Extra_ProgressBarUpdate( pProgress, Extra_FileReaderGetCurPosition(p->pReader), NULL );

        // consider different line types
        fTokensReady = 0;
        pDirective = (char *)p->vTokens->pArray[0];
        if ( !strcmp( pDirective, ".names" ) )
            { fStatus = Io_ReadBlifNetworkNames( p, &p->vTokens ); fTokensReady = 1; }
        else if ( !strcmp( pDirective, ".gate" ) )
            // fStatus = Io_ReadBlifNetworkGate( p, p->vTokens );
            assert( 0 );
        else if ( !strcmp( pDirective, ".latch" ) )
            // fStatus = Io_ReadBlifNetworkLatch( p, p->vTokens );
            assert( 0 );
        else if ( !strcmp( pDirective, ".inputs" ) )
            fStatus = Io_ReadBlifNetworkInputs( p, p->vTokens );
        else if ( !strcmp( pDirective, ".outputs" ) )
            fStatus = Io_ReadBlifNetworkOutputs( p, p->vTokens );
        else if ( !strcmp( pDirective, ".input_arrival" ) )
            fStatus = Io_ReadBlifNetworkInputArrival( p, p->vTokens );
        else if ( !strcmp( pDirective, ".output_required" ) )
            fStatus = Io_ReadBlifNetworkOutputRequired( p, p->vTokens );
        else if ( !strcmp( pDirective, ".default_input_arrival" ) )
            fStatus = Io_ReadBlifNetworkDefaultInputArrival( p, p->vTokens );
        else if ( !strcmp( pDirective, ".default_output_required" ) )
            fStatus = Io_ReadBlifNetworkDefaultOutputRequired( p, p->vTokens );
        else if ( !strcmp( pDirective, ".input_drive" ) )
            fStatus = Io_ReadBlifNetworkInputDrive( p, p->vTokens );
        else if ( !strcmp( pDirective, ".output_load" ) )
            fStatus = Io_ReadBlifNetworkOutputLoad( p, p->vTokens );
        else if ( !strcmp( pDirective, ".default_input_drive" ) )
            fStatus = Io_ReadBlifNetworkDefaultInputDrive( p, p->vTokens );
        else if ( !strcmp( pDirective, ".default_output_load" ) )
            fStatus = Io_ReadBlifNetworkDefaultOutputLoad( p, p->vTokens );
        else if ( !strcmp( pDirective, ".and_gate_delay" ) )
        {}
            // fStatus = Io_ReadBlifNetworkAndGateDelay( p, p->vTokens );
//        else if ( !strcmp( pDirective, ".subckt" ) )
//            fStatus = Io_ReadBlifNetworkSubcircuit( p, p->vTokens );
        else if ( !strcmp( pDirective, ".exdc" ) )
            break;
        else if ( !strcmp( pDirective, ".end" ) )
        {
            p->vTokens = Io_ReadBlifGetTokens(p);
            break;
        }
        else if ( !strcmp( pDirective, ".blackbox" ) )
        {
            pNtk->ntkType = ABC_NTK_NETLIST;
            pNtk->ntkFunc = ABC_FUNC_BLACKBOX;
            Mem_FlexStop( (Mem_Flex_t *)pNtk->pManFunc, 0 );
            pNtk->pManFunc = NULL;
        }
        else
            printf( "%s (line %d): Skipping directive \"%s\".\n", p->pFileName,
                Extra_FileReaderGetLineNumber(p->pReader, 0), pDirective );
        if ( p->vTokens == NULL ) // some files do not have ".end" in the end
            break;
        if ( fStatus == 1 )
        {
//             Extra_ProgressBarStop( pProgress );
            Abc_NtkDelete( pNtk );
            return NULL;
        }
    }
//     if ( p->pNtkMaster == NULL )
//         Extra_ProgressBarStop( pProgress );
    return pNtk;
}


int Io_ReadBlifNetworkNames( Io_ReadBlif_t * p, Vec_Ptr_t ** pvTokens )
{
    Vec_Ptr_t * vTokens = *pvTokens;
    Abc_Ntk_t * pNtk = p->pNtkCur;
    Abc_Obj_t * pNode;
    char * pToken, Char, ** ppNames;
    int nFanins, nNames;

    // create a new node and add it to the network
    if ( vTokens->nSize < 2 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "The .names line has less than two tokens." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }

    // create the node
    ppNames = (char **)vTokens->pArray + 1;
    nNames  = vTokens->nSize - 2;
    pNode   = Io_ReadCreateNode( pNtk, ppNames[nNames], ppNames, nNames );

    // derive the functionality of the node
    p->vCubes->nSize = 0;
    nFanins = vTokens->nSize - 2;
    if ( nFanins == 0 )
    {
        while ( (vTokens = Io_ReadBlifGetTokens(p)) )
        {
            pToken = (char *)vTokens->pArray[0];
            if ( pToken[0] == '.' )
                break;
            // read the cube
            if ( vTokens->nSize != 1 )
            {
                p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
                sprintf( p->sError, "The number of tokens in the constant cube is wrong." );
                Io_ReadBlifPrintErrorMessage( p );
                return 1;
            }
            // create the cube
            Char = ((char *)vTokens->pArray[0])[0];
            Vec_StrPush( p->vCubes, ' ' );
            Vec_StrPush( p->vCubes, Char );
            Vec_StrPush( p->vCubes, '\n' );
        }
    }
    else
    {
        while ( (vTokens = Io_ReadBlifGetTokens(p)) )
        {
            pToken = (char *)vTokens->pArray[0];
            if ( pToken[0] == '.' )
                break;
            // read the cube
            if ( vTokens->nSize != 2 )
            {
                p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
                sprintf( p->sError, "The number of tokens in the cube is wrong." );
                Io_ReadBlifPrintErrorMessage( p );
                return 1;
            }
            // create the cube
            Vec_StrPrintStr( p->vCubes, (char *)vTokens->pArray[0] );
            // check the char
            Char = ((char *)vTokens->pArray[1])[0];
            if ( Char != '0' && Char != '1' && Char != 'x' && Char != 'n' )
            {
                p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
                sprintf( p->sError, "The output character in the constant cube is wrong." );
                Io_ReadBlifPrintErrorMessage( p );
                return 1;
            }
            Vec_StrPush( p->vCubes, ' ' );
            Vec_StrPush( p->vCubes, Char );
            Vec_StrPush( p->vCubes, '\n' );
        }
    }
    // if there is nothing there
    if ( p->vCubes->nSize == 0 )
    {
        // create an empty cube
        Vec_StrPush( p->vCubes, ' ' );
        Vec_StrPush( p->vCubes, '0' );
        Vec_StrPush( p->vCubes, '\n' );
    }
    Vec_StrPush( p->vCubes, 0 );

    // set the pointer to the functionality of the node
    Abc_ObjSetData( pNode, Abc_SopRegister((Mem_Flex_t *)pNtk->pManFunc, p->vCubes->pArray) );

    // check the size
    if ( Abc_ObjFaninNum(pNode) != Abc_SopGetVarNum((char *)Abc_ObjData(pNode)) )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "The number of fanins (%d) of node %s is different from SOP size (%d).",
            Abc_ObjFaninNum(pNode), Abc_ObjName(Abc_ObjFanout(pNode,0)), Abc_SopGetVarNum((char *)Abc_ObjData(pNode)) );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }

    // return the last array of tokens
    *pvTokens = vTokens;
    return 0;
}


int Io_ReadBlifNetworkInputs( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    int i;
    for ( i = 1; i < vTokens->nSize; i++ )
        Io_ReadCreatePi( p->pNtkCur, (char *)vTokens->pArray[i] );
    return 0;
}


Abc_Obj_t * Io_ReadCreatePi( Abc_Ntk_t * pNtk, char * pName )
{
    Abc_Obj_t * pNet, * pTerm;
    // get the PI net
    pNet  = Abc_NtkFindNet( pNtk, pName );
    if ( pNet )
        printf( "Warning: PI \"%s\" appears twice in the list.\n", pName );
    pNet  = Abc_NtkFindOrCreateNet( pNtk, pName );
    // add the PI node
    pTerm = Abc_NtkCreatePi( pNtk );
    Abc_ObjAddFanin( pNet, pTerm );
    return pTerm;
}


Abc_Obj_t * Io_ReadCreateNode( Abc_Ntk_t * pNtk, char * pNameOut, char * pNamesIn[], int nInputs )
{
    Abc_Obj_t * pNet, * pNode;
    int i;
    // create a new node
    pNode = Abc_NtkCreateNode( pNtk );
    // add the fanin nets
    for ( i = 0; i < nInputs; i++ )
    {
        pNet = Abc_NtkFindOrCreateNet( pNtk, pNamesIn[i] );
        Abc_ObjAddFanin( pNode, pNet );
    }
    // add the fanout net
    pNet = Abc_NtkFindOrCreateNet( pNtk, pNameOut );
    Abc_ObjAddFanin( pNet, pNode );
    return pNode;
}


int Io_ReadBlifNetworkOutputs( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    int i;
    for ( i = 1; i < vTokens->nSize; i++ )
        Io_ReadCreatePo( p->pNtkCur, (char *)vTokens->pArray[i] );
    return 0;
}


int Io_ReadBlifNetworkInputArrival( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    Abc_Obj_t * pNet;
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".input_arrival", 14 ) == 0 );
    if ( vTokens->nSize != 4 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .input_arrival line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    pNet = Abc_NtkFindNet( p->pNtkCur, (char *)vTokens->pArray[1] );
    if ( pNet == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Cannot find object corresponding to %s on .input_arrival line.", (char*)vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[2], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[3], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .input_arrival line.", (char*)vTokens->pArray[2], (char*)vTokens->pArray[3] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
    //Abc_NtkTimeSetArrival( p->pNtkCur, Abc_ObjFanin0(pNet)->Id, (float)TimeRise, (float)TimeFall );
    Vec_IntPush( p->vInArrs, Abc_ObjFanin0(pNet)->Id );
    Vec_IntPush( p->vInArrs, Abc_Float2Int((float)TimeRise) );
    Vec_IntPush( p->vInArrs, Abc_Float2Int((float)TimeFall) );
    return 0;
}


int Io_ReadBlifNetworkOutputRequired( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    Abc_Obj_t * pNet;
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".output_required", 16 ) == 0 );
    if ( vTokens->nSize != 4 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .output_required line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    pNet = Abc_NtkFindNet( p->pNtkCur, (char *)vTokens->pArray[1] );
    if ( pNet == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Cannot find object corresponding to %s on .output_required line.", (char*)vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[2], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[3], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .output_required line.", (char*)vTokens->pArray[2], (char*)vTokens->pArray[3] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
//    Abc_NtkTimeSetRequired( p->pNtkCur, Abc_ObjFanout0(pNet)->Id, (float)TimeRise, (float)TimeFall );
    Vec_IntPush( p->vOutReqs, Abc_ObjFanout0(pNet)->Id );
    Vec_IntPush( p->vOutReqs, Abc_Float2Int((float)TimeRise) );
    Vec_IntPush( p->vOutReqs, Abc_Float2Int((float)TimeFall) );
    return 0;
}


int Io_ReadBlifNetworkDefaultInputArrival( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".default_input_arrival", 23 ) == 0 );
    if ( vTokens->nSize != 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .default_input_arrival line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[1], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[2], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .default_input_arrival line.", (char*)vTokens->pArray[1], (char*)vTokens->pArray[2] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
    //Abc_NtkTimeSetDefaultArrival( p->pNtkCur, (float)TimeRise, (float)TimeFall );
    p->DefInArrRise = (float)TimeRise;
    p->DefInArrFall = (float)TimeFall;
    p->fHaveDefInArr = 1;
    return 0;
}


int Io_ReadBlifNetworkDefaultOutputRequired( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".default_output_required", 25 ) == 0 );
    if ( vTokens->nSize != 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .default_output_required line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[1], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[2], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .default_output_required line.", (char*)vTokens->pArray[1], (char*)vTokens->pArray[2] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
//    Abc_NtkTimeSetDefaultRequired( p->pNtkCur, (float)TimeRise, (float)TimeFall );
    p->DefOutReqRise = (float)TimeRise;
    p->DefOutReqFall = (float)TimeFall;
    p->fHaveDefOutReq = 1;
    return 0;
}


int Io_ReadBlifNetworkInputDrive( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    Abc_Obj_t * pNet;
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".input_drive", 12 ) == 0 );
    if ( vTokens->nSize != 4 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .input_drive line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    pNet = Abc_NtkFindNet( p->pNtkCur, (char *)vTokens->pArray[1] );
    if ( pNet == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Cannot find object corresponding to %s on .input_drive line.", (char*)vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[2], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[3], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .input_drive line.", (char*)vTokens->pArray[2], (char*)vTokens->pArray[3] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
    //Abc_NtkTimeSetInputDrive( p->pNtkCur, Io_ReadFindCiId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanin0(pNet)->Id)), (float)TimeRise, (float)TimeFall );
    Vec_IntPush( p->vInDrives, Io_ReadFindCiId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanin0(pNet)->Id)) );
    Vec_IntPush( p->vInDrives, Abc_Float2Int((float)TimeRise) );
    Vec_IntPush( p->vInDrives, Abc_Float2Int((float)TimeFall) );
    return 0;
}


int Io_ReadBlifNetworkOutputLoad( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    Abc_Obj_t * pNet;
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".output_load", 12 ) == 0 );
    if ( vTokens->nSize != 4 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .output_load line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    pNet = Abc_NtkFindNet( p->pNtkCur, (char *)vTokens->pArray[1] );
    if ( pNet == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Cannot find object corresponding to %s on .output_load line.", (char*)vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[2], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[3], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .output_load line.", (char*)vTokens->pArray[2], (char*)vTokens->pArray[3] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
//    Abc_NtkTimeSetOutputLoad( p->pNtkCur, Io_ReadFindCoId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanout0(pNet)->Id)), (float)TimeRise, (float)TimeFall );
    Vec_IntPush( p->vOutLoads, Io_ReadFindCoId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanout0(pNet)->Id)) );
    Vec_IntPush( p->vOutLoads, Abc_Float2Int((float)TimeRise) );
    Vec_IntPush( p->vOutLoads, Abc_Float2Int((float)TimeFall) );
    return 0;
}


int Io_ReadBlifNetworkDefaultInputDrive( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".default_input_drive", 21 ) == 0 );
    if ( vTokens->nSize != 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .default_input_drive line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[1], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[2], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .default_input_drive line.", (char*)vTokens->pArray[1], (char*)vTokens->pArray[2] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
//    Abc_NtkTimeSetDefaultInputDrive( p->pNtkCur, (float)TimeRise, (float)TimeFall );
    p->DefInDriRise = (float)TimeRise;
    p->DefInDriFall = (float)TimeFall;
    p->fHaveDefInDri = 1;
    return 0;
}


int Io_ReadBlifNetworkDefaultOutputLoad( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".default_output_load", 21 ) == 0 );
    if ( vTokens->nSize != 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .default_output_load line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[1], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[2], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .default_output_load line.", (char*)vTokens->pArray[1], (char*)vTokens->pArray[2] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
//    Abc_NtkTimeSetDefaultOutputLoad( p->pNtkCur, (float)TimeRise, (float)TimeFall );
    p->DefOutLoadRise = (float)TimeRise;
    p->DefOutLoadFall = (float)TimeFall;
    p->fHaveDefOutLoad = 1;
    return 0;
}


// int Io_ReadBlifNetworkAndGateDelay( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
// {
//     char * pFoo1;
//     double AndGateDelay;
//
//     // make sure this is indeed the .inputs line
//     assert( strncmp( (char *)vTokens->pArray[0], ".and_gate_delay", 25 ) == 0 );
//     if ( vTokens->nSize != 2 )
//     {
//         p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
//         sprintf( p->sError, "Wrong number of arguments (%d) on .and_gate_delay line (should be 1).", vTokens->nSize-1 );
//         Io_ReadBlifPrintErrorMessage( p );
//         return 1;
//     }
//     AndGateDelay = strtod( (char *)vTokens->pArray[1], &pFoo1 );
//     if ( *pFoo1 != '\0' )
//     {
//         p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
//         sprintf( p->sError, "Bad value (%s) for AND gate delay in on .and_gate_delay line line.", (char*)vTokens->pArray[1] );
//         Io_ReadBlifPrintErrorMessage( p );
//         return 1;
//     }
//     // set timing info
//     p->pNtkCur->AndGateDelay = (float)AndGateDelay;
//     return 0;
// }


int Io_ReadFindCiId( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pTemp;
    int i;
    Abc_NtkForEachCi( pNtk, pTemp, i )
        if ( pTemp == pObj )
            return i;
    return -1;
}


int Io_ReadFindCoId( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pTemp;
    int i;
    Abc_NtkForEachPo( pNtk, pTemp, i )
        if ( pTemp == pObj )
            return i;
    return -1;
}


Abc_Obj_t * Io_ReadCreatePo( Abc_Ntk_t * pNtk, char * pName )
{
    Abc_Obj_t * pNet, * pTerm;
    // get the PO net
    pNet  = Abc_NtkFindNet( pNtk, pName );
    if ( pNet && Abc_ObjFaninNum(pNet) == 0 )
        printf( "Warning: PO \"%s\" appears twice in the list.\n", pName );
    pNet  = Abc_NtkFindOrCreateNet( pNtk, pName );
    // add the PO node
    pTerm = Abc_NtkCreatePo( pNtk );
    Abc_ObjAddFanin( pTerm, pNet );
    return pTerm;
}


void Io_ReadBlifFree( Io_ReadBlif_t * p )
{
    Extra_FileReaderFree( p->pReader );
    Vec_PtrFree( p->vNewTokens );
    Vec_StrFree( p->vCubes );
    Vec_IntFree( p->vInArrs );
    Vec_IntFree( p->vOutReqs );
    Vec_IntFree( p->vInDrives );
    Vec_IntFree( p->vOutLoads );
    ABC_FREE( p );
}
