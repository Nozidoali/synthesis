#include "abc_extra.h"


Extra_FileReader_t * Extra_FileReaderAlloc( char * pFileName,
    char * pCharsComment, char * pCharsStop, char * pCharsClean )
{
    Extra_FileReader_t * p;
    FILE * pFile;
    char * pChar;
    int nCharsToRead;
    int RetValue;
    // check if the file can be opened
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Extra_FileReaderAlloc(): Cannot open input file \"%s\".\n", pFileName );
        return NULL;
    }
    // start the file reader
    p = ABC_ALLOC( Extra_FileReader_t, 1 );
    memset( p, 0, sizeof(Extra_FileReader_t) );
    p->pFileName   = pFileName;
    p->pFile       = pFile;
    // set the character map
    memset( p->pCharMap, EXTRA_CHAR_NORMAL, 256 );
    for ( pChar = pCharsComment; *pChar; pChar++ )
        p->pCharMap[(unsigned char)*pChar] = EXTRA_CHAR_COMMENT;
    for ( pChar = pCharsStop; *pChar; pChar++ )
        p->pCharMap[(unsigned char)*pChar] = EXTRA_CHAR_STOP;
    for ( pChar = pCharsClean; *pChar; pChar++ )
        p->pCharMap[(unsigned char)*pChar] = EXTRA_CHAR_CLEAN;
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );
    p->nFileSize = ftell( pFile );
    rewind( pFile );
    // allocate the buffer
    p->pBuffer = ABC_ALLOC( char, EXTRA_BUFFER_SIZE+1 );
    p->nBufferSize = EXTRA_BUFFER_SIZE;
    p->pBufferCur  = p->pBuffer;
    // determine how many chars to read
    nCharsToRead = EXTRA_MINIMUM(p->nFileSize, EXTRA_BUFFER_SIZE);
    // load the first part into the buffer
    RetValue = fread( p->pBuffer, nCharsToRead, 1, p->pFile );
    assert( RetValue == 1 || RetValue == 0 );
    p->nFileRead = nCharsToRead;
    // set the ponters to the end and the stopping point
    p->pBufferEnd  = p->pBuffer + nCharsToRead;
    p->pBufferStop = (p->nFileRead == p->nFileSize)? p->pBufferEnd : p->pBuffer + EXTRA_BUFFER_SIZE - EXTRA_OFFSET_SIZE;
    // start the arrays
    p->vTokens = Vec_PtrAlloc( 100 );
    p->vLines = Vec_IntAlloc( 100 );
    p->nLineCounter = 1; // 1-based line counting
    return p;
}


void * Extra_FileReaderGetTokens( Extra_FileReader_t * p )
{
    Vec_Ptr_t * vTokens;
    while ( (vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens_int( p )) )
        if ( vTokens->nSize > 0 )
            break;
    return vTokens;
}


void * Extra_FileReaderGetTokens_int( Extra_FileReader_t * p )
{
    char * pChar;
    int fTokenStarted, MapValue;
    if ( p->fStop )
        return NULL;
    // reset the token info
    p->vTokens->nSize = 0;
    p->vLines->nSize = 0;
    fTokenStarted = 0;
    // check if the new data should to be loaded
    if ( p->pBufferCur > p->pBufferStop )
        Extra_FileReaderReload( p );

//    printf( "%d\n", p->pBufferEnd - p->pBufferCur );

    // process the string starting from the current position
    for ( pChar = p->pBufferCur; pChar < p->pBufferEnd; pChar++ )
    {
        // count the lines
        if ( *pChar == '\n' )
            p->nLineCounter++;
        // switch depending on the character
        MapValue = p->pCharMap[(int)*pChar];

//        printf( "Char value = %d. Map value = %d.\n", *pChar, MapValue );


        switch ( MapValue )
        {
            case EXTRA_CHAR_COMMENT:
                if ( *pChar != '/' || *(pChar+1) == '/' )
                { // dealing with the need to have // as a comment
                    // if the token was being written, stop it
                    if ( fTokenStarted )
                        fTokenStarted = 0;
                    // eraze the comment till the end of line
                    while ( *pChar != '\n' )
                    {
                        *pChar++ = 0;
                        if ( pChar == p->pBufferEnd )
                        {   // this failure is due to the fact the comment continued
                            // through EXTRA_OFFSET_SIZE chars till the end of the buffer
                            printf( "Extra_FileReader failed to parse the file \"%s\".\n", p->pFileName );
                            return NULL;
                        }
                    }
                    pChar--;
                    break;
                }
                // otherwise it is a normal character
            case EXTRA_CHAR_NORMAL:
                if ( !fTokenStarted )
                {
                    Vec_PtrPush( p->vTokens, pChar );
                    Vec_IntPush( p->vLines, p->nLineCounter );
                    fTokenStarted = 1;
                }
                break;
            case EXTRA_CHAR_STOP:
                if ( fTokenStarted )
                    fTokenStarted = 0;
                *pChar = 0;
                // prepare before leaving
                p->pBufferCur = pChar + 1;
                return p->vTokens;
            case EXTRA_CHAR_CLEAN:
                if ( fTokenStarted )
                    fTokenStarted = 0;
                *pChar = 0;
                break;
            default:
                assert( 0 );
        }
    }
    // the file is finished or the last part continued
    // through EXTRA_OFFSET_SIZE chars till the end of the buffer
    if ( p->pBufferStop == p->pBufferEnd ) // end of file
    {
        *pChar = 0;
        p->fStop = 1;
        return p->vTokens;
    }
    printf( "Extra_FileReader failed to parse the file \"%s\".\n", p->pFileName );
/*
    {
        int i;
        for ( i = 0; i < p->vTokens->nSize; i++ )
            printf( "%s ", p->vTokens->pArray[i] );
        printf( "\n" );
    }
*/
    return NULL;
}


void Extra_FileReaderReload( Extra_FileReader_t * p )
{
    int nCharsUsed, nCharsToRead;
    int RetValue;
    assert( !p->fStop );
    assert( p->pBufferCur > p->pBufferStop );
    assert( p->pBufferCur < p->pBufferEnd );
    // figure out how many chars are still not processed
    nCharsUsed = p->pBufferEnd - p->pBufferCur;
    // move the remaining data to the beginning of the buffer
    memmove( p->pBuffer, p->pBufferCur, (size_t)nCharsUsed );
    p->pBufferCur = p->pBuffer;
    // determine how many chars we will read
    nCharsToRead = EXTRA_MINIMUM( p->nBufferSize - nCharsUsed, p->nFileSize - p->nFileRead );
    // read the chars
    RetValue = fread( p->pBuffer + nCharsUsed, nCharsToRead, 1, p->pFile );
    assert( RetValue == 1 || RetValue == 0 );
    p->nFileRead += nCharsToRead;
    // set the ponters to the end and the stopping point
    p->pBufferEnd  = p->pBuffer + nCharsUsed + nCharsToRead;
    p->pBufferStop = (p->nFileRead == p->nFileSize)? p->pBufferEnd : p->pBuffer + EXTRA_BUFFER_SIZE - EXTRA_OFFSET_SIZE;
}


int Extra_FileReaderGetLineNumber( Extra_FileReader_t * p, int iToken )
{
    assert( iToken >= 0 && iToken < p->vTokens->nSize );
    return p->vLines->pArray[iToken];
}


void Extra_FileReaderFree( Extra_FileReader_t * p )
{
    if ( p->pFile )
        fclose( p->pFile );
    ABC_FREE( p->pBuffer );
    Vec_PtrFree( p->vTokens );
    Vec_IntFree( p->vLines );
    ABC_FREE( p );
}


char * Extra_UtilStrsav( const char *s )
{
    if(s == NULL) {  /* added 7/95, for robustness */
       return NULL;
    }
    else {
       return strcpy(ABC_ALLOC(char, strlen(s)+1), s);
    }
}


char * Extra_TimeStamp()
{
    static char Buffer[100];
    char * TimeStamp;
    time_t ltime;
    // get the current time
    time( &ltime );
    TimeStamp = asctime( localtime( &ltime ) );
    TimeStamp[ strlen(TimeStamp) - 1 ] = 0;
    strcpy( Buffer, TimeStamp );
    return Buffer;
}


void Extra_Truth4VarNPN( unsigned short ** puCanons, char ** puPhases, char ** puPerms, unsigned char ** puMap )
{
    unsigned short * uCanons;
    unsigned char * uMap;
    unsigned uTruth, uPhase, uPerm;
    char ** pPerms4, * uPhases, * uPerms;
    int nFuncs, nClasses;
    int i, k;

    nFuncs  = (1 << 16);
    uCanons = ABC_ALLOC( unsigned short, nFuncs );
    uPhases = ABC_ALLOC( char, nFuncs );
    uPerms  = ABC_ALLOC( char, nFuncs );
    uMap    = ABC_ALLOC( unsigned char, nFuncs );
    memset( uCanons, 0, sizeof(unsigned short) * nFuncs );
    memset( uPhases, 0, sizeof(char) * nFuncs );
    memset( uPerms,  0, sizeof(char) * nFuncs );
    memset( uMap,    0, sizeof(unsigned char) * nFuncs );
    pPerms4 = Extra_Permutations( 4 );

    nClasses = 1;
    nFuncs = (1 << 15);
    for ( uTruth = 1; uTruth < (unsigned)nFuncs; uTruth++ )
    {
        // skip already assigned
        if ( uCanons[uTruth] )
        {
            assert( uTruth > uCanons[uTruth] );
            uMap[~uTruth & 0xFFFF] = uMap[uTruth] = uMap[uCanons[uTruth]];
            continue;
        }
        uMap[uTruth] = nClasses++;
        for ( i = 0; i < 16; i++ )
        {
            uPhase = Extra_TruthPolarize( uTruth, i, 4 );
            for ( k = 0; k < 24; k++ )
            {
                uPerm = Extra_TruthPermute( uPhase, pPerms4[k], 4, 0 );
                if ( uCanons[uPerm] == 0 )
                {
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i;
                    uPerms[uPerm]  = k;

                    uPerm = ~uPerm & 0xFFFF;
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i | 16;
                    uPerms[uPerm]  = k;
                }
                else
                    assert( uCanons[uPerm] == uTruth );
            }
            uPhase = Extra_TruthPolarize( ~uTruth & 0xFFFF, i, 4 );
            for ( k = 0; k < 24; k++ )
            {
                uPerm = Extra_TruthPermute( uPhase, pPerms4[k], 4, 0 );
                if ( uCanons[uPerm] == 0 )
                {
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i;
                    uPerms[uPerm]  = k;

                    uPerm = ~uPerm & 0xFFFF;
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i | 16;
                    uPerms[uPerm]  = k;
                }
                else
                    assert( uCanons[uPerm] == uTruth );
            }
        }
    }
    uPhases[(1<<16)-1] = 16;
    assert( nClasses == 222 );
    ABC_FREE( pPerms4 );
    if ( puCanons )
        *puCanons = uCanons;
    else
        ABC_FREE( uCanons );
    if ( puPhases )
        *puPhases = uPhases;
    else
        ABC_FREE( uPhases );
    if ( puPerms )
        *puPerms = uPerms;
    else
        ABC_FREE( uPerms );
    if ( puMap )
        *puMap = uMap;
    else
        ABC_FREE( uMap );
}


char ** Extra_Permutations( int n )
{
    char Array[50];
    char ** pRes;
    int nFact, i;
    // allocate memory
    nFact = Extra_Factorial( n );
    pRes  = (char **)Extra_ArrayAlloc( nFact, n, sizeof(char) );
    // fill in the permutations
    for ( i = 0; i < n; i++ )
        Array[i] = i;
    Extra_Permutations_rec( pRes, nFact, n, Array );
    // print the permutations
/*
    {
    int i, k;
    for ( i = 0; i < nFact; i++ )
    {
        printf( "{" );
        for ( k = 0; k < n; k++ )
            printf( " %d", pRes[i][k] );
        printf( " }\n" );
    }
    }
*/
    return pRes;
}


int Extra_Factorial( int n )
{
    int i, Res = 1;
    for ( i = 1; i <= n; i++ )
        Res *= i;
    return Res;
}


void ** Extra_ArrayAlloc( int nCols, int nRows, int Size )
{
    void ** pRes;
    char * pBuffer;
    int i;
    assert( nCols > 0 && nRows > 0 && Size > 0 );
    pBuffer = ABC_ALLOC( char, nCols * (sizeof(void *) + nRows * Size) );
    pRes = (void **)pBuffer;
    pRes[0] = pBuffer + nCols * sizeof(void *);
    for ( i = 1; i < nCols; i++ )
        pRes[i] = (void *)((char *)pRes[0] + i * nRows * Size);
    return pRes;
}


void Extra_Permutations_rec( char ** pRes, int nFact, int n, char Array[] )
{
    char ** pNext;
    int nFactNext;
    int iTemp, iCur, iLast, k;

    if ( n == 1 )
    {
        pRes[0][0] = Array[0];
        return;
    }

    // get the next factorial
    nFactNext = nFact / n;
    // get the last entry
    iLast = n - 1;

    for ( iCur = 0; iCur < n; iCur++ )
    {
        // swap Cur and Last
        iTemp        = Array[iCur];
        Array[iCur]  = Array[iLast];
        Array[iLast] = iTemp;

        // get the pointer to the current section
        pNext = pRes + (n - 1 - iCur) * nFactNext;

        // set the last entry
        for ( k = 0; k < nFactNext; k++ )
            pNext[k][iLast] = Array[iLast];

        // call recursively for this part
        Extra_Permutations_rec( pNext, nFactNext, n - 1, Array );

        // swap them back
        iTemp        = Array[iCur];
        Array[iCur]  = Array[iLast];
        Array[iLast] = iTemp;
    }
}


unsigned Extra_TruthPolarize( unsigned uTruth, int Polarity, int nVars )
{
    // elementary truth tables
    static unsigned Signs[5] = {
        0xAAAAAAAA,    // 1010 1010 1010 1010 1010 1010 1010 1010
        0xCCCCCCCC,    // 1010 1010 1010 1010 1010 1010 1010 1010
        0xF0F0F0F0,    // 1111 0000 1111 0000 1111 0000 1111 0000
        0xFF00FF00,    // 1111 1111 0000 0000 1111 1111 0000 0000
        0xFFFF0000     // 1111 1111 1111 1111 0000 0000 0000 0000
    };
    // unsigned uTruthRes, uCof0, uCof1;
    // int nMints, Shift, v;
    unsigned uCof0, uCof1;
    int Shift, v;
    assert( nVars < 6 );
    // nMints = (1 << nVars);
    // uTruthRes = uTruth;
    for ( v = 0; v < nVars; v++ )
        if ( Polarity & (1 << v) )
        {
            uCof0  = uTruth & ~Signs[v];
            uCof1  = uTruth &  Signs[v];
            Shift  = (1 << v);
            uCof0 <<= Shift;
            uCof1 >>= Shift;
            uTruth = uCof0 | uCof1;
        }
    return uTruth;
}


unsigned Extra_TruthPermute( unsigned Truth, char * pPerms, int nVars, int fReverse )
{
    unsigned Result;
    int * pMints;
    int * pMintsP;
    int nMints;
    int i, m;

    assert( nVars < 6 );
    nMints  = (1 << nVars);
    pMints  = ABC_ALLOC( int, nMints );
    pMintsP = ABC_ALLOC( int, nMints );
    for ( i = 0; i < nMints; i++ )
        pMints[i] = i;

    Extra_TruthPermute_int( pMints, nMints, pPerms, nVars, pMintsP );

    Result = 0;
    if ( fReverse )
    {
        for ( m = 0; m < nMints; m++ )
            if ( Truth & (1 << pMintsP[m]) )
                Result |= (1 << m);
    }
    else
    {
        for ( m = 0; m < nMints; m++ )
            if ( Truth & (1 << m) )
                Result |= (1 << pMintsP[m]);
    }

    ABC_FREE( pMints );
    ABC_FREE( pMintsP );

    return Result;
}


void Extra_TruthPermute_int( int * pMints, int nMints, char * pPerm, int nVars, int * pMintsP )
{
    int m, v;
    // clean the storage for minterms
    memset( pMintsP, 0, sizeof(int) * nMints );
    // go through minterms and add the variables
    for ( m = 0; m < nMints; m++ )
        for ( v = 0; v < nVars; v++ )
            if ( pMints[m] & (1 << v) )
                pMintsP[m] |= (1 << pPerm[v]);
}


void Extra_TruthStretch( unsigned * pOut, unsigned * pIn, int nVars, int nVarsAll, unsigned Phase )
{
    unsigned * pTemp;
    int i, k, Var = nVars - 1, Counter = 0;
    for ( i = nVarsAll - 1; i >= 0; i-- )
        if ( Phase & (1 << i) )
        {
            for ( k = Var; k < i; k++ )
            {
                Extra_TruthSwapAdjacentVars( pOut, pIn, nVarsAll, k );
                pTemp = pIn; pIn = pOut; pOut = pTemp;
                Counter++;
            }
            Var--;
        }
    assert( Var == -1 );
    // swap if it was moved an even number of times
    if ( !(Counter & 1) )
        Extra_TruthCopy( pOut, pIn, nVarsAll );
}


void Extra_TruthSwapAdjacentVars( unsigned * pOut, unsigned * pIn, int nVars, int iVar )
{
    static unsigned PMasks[4][3] = {
        { 0x99999999, 0x22222222, 0x44444444 },
        { 0xC3C3C3C3, 0x0C0C0C0C, 0x30303030 },
        { 0xF00FF00F, 0x00F000F0, 0x0F000F00 },
        { 0xFF0000FF, 0x0000FF00, 0x00FF0000 }
    };
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step, Shift;

    assert( iVar < nVars - 1 );
    if ( iVar < 4 )
    {
        Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & PMasks[iVar][0]) | ((pIn[i] & PMasks[iVar][1]) << Shift) | ((pIn[i] & PMasks[iVar][2]) >> Shift);
    }
    else if ( iVar > 4 )
    {
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 4*Step )
        {
            for ( i = 0; i < Step; i++ )
                pOut[i] = pIn[i];
            for ( i = 0; i < Step; i++ )
                pOut[Step+i] = pIn[2*Step+i];
            for ( i = 0; i < Step; i++ )
                pOut[2*Step+i] = pIn[Step+i];
            for ( i = 0; i < Step; i++ )
                pOut[3*Step+i] = pIn[3*Step+i];
            pIn  += 4*Step;
            pOut += 4*Step;
        }
    }
    else // if ( iVar == 4 )
    {
        for ( i = 0; i < nWords; i += 2 )
        {
            pOut[i]   = (pIn[i]   & 0x0000FFFF) | ((pIn[i+1] & 0x0000FFFF) << 16);
            pOut[i+1] = (pIn[i+1] & 0xFFFF0000) | ((pIn[i]   & 0xFFFF0000) >> 16);
        }
    }
}


int Extra_TruthMinCofSuppOverlap( unsigned * pTruth, int nVars, int * pVarMin )
{
    static unsigned uCofactor[16];
    int i, ValueCur, ValueMin, VarMin;
    unsigned uSupp0, uSupp1;
    int nVars0, nVars1;
    assert( nVars <= 9 );
    ValueMin = 32;
    VarMin   = -1;
    for ( i = 0; i < nVars; i++ )
    {
        // get negative cofactor
        Extra_TruthCopy( uCofactor, pTruth, nVars );
        Extra_TruthCofactor0( uCofactor, nVars, i );
        uSupp0 = Extra_TruthSupport( uCofactor, nVars );
        nVars0 = Extra_WordCountOnes( uSupp0 );
//Extra_PrintBinary( stdout, &uSupp0, 8 ); printf( "\n" );
        // get positive cofactor
        Extra_TruthCopy( uCofactor, pTruth, nVars );
        Extra_TruthCofactor1( uCofactor, nVars, i );
        uSupp1 = Extra_TruthSupport( uCofactor, nVars );
        nVars1 = Extra_WordCountOnes( uSupp1 );
//Extra_PrintBinary( stdout, &uSupp1, 8 ); printf( "\n" );
        // get the number of common vars
        ValueCur = Extra_WordCountOnes( uSupp0 & uSupp1 );
        if ( ValueMin > ValueCur && nVars0 <= 5 && nVars1 <= 5 )
        {
            ValueMin = ValueCur;
            VarMin = i;
        }
        if ( ValueMin == 0 )
            break;
    }
    if ( pVarMin )
        *pVarMin = VarMin;
    return ValueMin;
}


void Extra_TruthCofactor0( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0x55555555) | ((pTruth[i] & 0x55555555) << 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0x33333333) | ((pTruth[i] & 0x33333333) << 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0x0F0F0F0F) | ((pTruth[i] & 0x0F0F0F0F) << 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0x00FF00FF) | ((pTruth[i] & 0x00FF00FF) << 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0x0000FFFF) | ((pTruth[i] & 0x0000FFFF) << 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
                pTruth[Step+i] = pTruth[i];
            pTruth += 2*Step;
        }
        return;
    }
}


int Extra_TruthSupport( unsigned * pTruth, int nVars )
{
    int i, Support = 0;
    for ( i = 0; i < nVars; i++ )
        if ( Extra_TruthVarInSupport( pTruth, nVars, i ) )
            Support |= (1 << i);
    return Support;
}


int Extra_TruthVarInSupport( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            if ( (pTruth[i] & 0x55555555) != ((pTruth[i] & 0xAAAAAAAA) >> 1) )
                return 1;
        return 0;
    case 1:
        for ( i = 0; i < nWords; i++ )
            if ( (pTruth[i] & 0x33333333) != ((pTruth[i] & 0xCCCCCCCC) >> 2) )
                return 1;
        return 0;
    case 2:
        for ( i = 0; i < nWords; i++ )
            if ( (pTruth[i] & 0x0F0F0F0F) != ((pTruth[i] & 0xF0F0F0F0) >> 4) )
                return 1;
        return 0;
    case 3:
        for ( i = 0; i < nWords; i++ )
            if ( (pTruth[i] & 0x00FF00FF) != ((pTruth[i] & 0xFF00FF00) >> 8) )
                return 1;
        return 0;
    case 4:
        for ( i = 0; i < nWords; i++ )
            if ( (pTruth[i] & 0x0000FFFF) != ((pTruth[i] & 0xFFFF0000) >> 16) )
                return 1;
        return 0;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
                if ( pTruth[i] != pTruth[Step+i] )
                    return 1;
            pTruth += 2*Step;
        }
        return 0;
    }
}


void Extra_TruthCofactor1( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0xAAAAAAAA) | ((pTruth[i] & 0xAAAAAAAA) >> 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0xCCCCCCCC) | ((pTruth[i] & 0xCCCCCCCC) >> 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0xF0F0F0F0) | ((pTruth[i] & 0xF0F0F0F0) >> 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0xFF00FF00) | ((pTruth[i] & 0xFF00FF00) >> 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0xFFFF0000) | ((pTruth[i] & 0xFFFF0000) >> 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
                pTruth[i] = pTruth[Step+i];
            pTruth += 2*Step;
        }
        return;
    }
}


void Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits )
{
    int Remainder, nWords;
    int w, i;

    Remainder = (nBits%(sizeof(unsigned)*8));
    nWords    = (nBits/(sizeof(unsigned)*8)) + (Remainder>0);

    for ( w = nWords-1; w >= 0; w-- )
        for ( i = ((w == nWords-1 && Remainder)? Remainder-1: 31); i >= 0; i-- )
            fprintf( pFile, "%c", '0' + (int)((Sign[w] & (1<<i)) > 0) );

//  fprintf( pFile, "\n" );
}


void Extra_PrintHex( FILE * pFile, unsigned * pTruth, int nVars )
{
    int nMints, nDigits, Digit, k;

    // write the number into the file
    fprintf( pFile, "0x" );
    nMints  = (1 << nVars);
    nDigits = nMints / 4 + ((nMints % 4) > 0);
    for ( k = nDigits - 1; k >= 0; k-- )
    {
        Digit = ((pTruth[k/8] >> (k * 4)) & 15);
        if ( Digit < 10 )
            fprintf( pFile, "%d", Digit );
        else
            fprintf( pFile, "%c", 'A' + Digit-10 );
    }
//    fprintf( pFile, "\n" );
}
