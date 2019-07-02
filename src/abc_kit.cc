#include "abc_kit.h"


Kit_DsdNtk_t * Kit_DsdDecompose( unsigned * pTruth, int nVars )
{
    return Kit_DsdDecomposeInt( pTruth, nVars, 0 );
}


Kit_DsdNtk_t * Kit_DsdDecomposeInt( unsigned * pTruth, int nVars, int nDecMux )
{
    Kit_DsdNtk_t * pNtk;
    Kit_DsdObj_t * pObj;
    unsigned uSupp;
    int i, nVarsReal;
    assert( nVars <= 16 );
    pNtk = Kit_DsdNtkAlloc( nVars );
    pNtk->Root = Abc_Var2Lit( pNtk->nVars, 0 );
    // create the first node
    pObj = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, nVars );
    assert( pNtk->pNodes[0] == pObj );
    for ( i = 0; i < nVars; i++ )
       pObj->pFans[i] = Abc_Var2Lit( i, 0 );
    Kit_TruthCopy( Kit_DsdObjTruth(pObj), pTruth, nVars );
    uSupp = Kit_TruthSupport( pTruth, nVars );
    // consider special cases
    nVarsReal = Kit_WordCountOnes( uSupp );
    if ( nVarsReal == 0 )
    {
        pObj->Type = KIT_DSD_CONST1;
        pObj->nFans = 0;
        if ( pTruth[0] == 0 )
             pNtk->Root = Abc_LitNot(pNtk->Root);
        return pNtk;
    }
    if ( nVarsReal == 1 )
    {
        pObj->Type = KIT_DSD_VAR;
        pObj->nFans = 1;
        pObj->pFans[0] = Abc_Var2Lit( Kit_WordFindFirstBit(uSupp), (pTruth[0] & 1) );
        return pNtk;
    }
    Kit_DsdDecompose_rec( pNtk, pNtk->pNodes[0], uSupp, &pNtk->Root, nDecMux );
    return pNtk;
}


Kit_DsdNtk_t * Kit_DsdNtkAlloc( int nVars )
{
    Kit_DsdNtk_t * pNtk;
    pNtk = ABC_ALLOC( Kit_DsdNtk_t, 1 );
    memset( pNtk, 0, sizeof(Kit_DsdNtk_t) );
    pNtk->pNodes = ABC_ALLOC( Kit_DsdObj_t *, nVars+1 );
    pNtk->nVars = nVars;
    pNtk->nNodesAlloc = nVars+1;
    pNtk->pMem = ABC_ALLOC( unsigned, 6 * Kit_TruthWordNum(nVars) );
    return pNtk;
}


Kit_DsdObj_t * Kit_DsdObjAlloc( Kit_DsdNtk_t * pNtk, Kit_Dsd_t Type, int nFans )
{
    Kit_DsdObj_t * pObj;
    int nSize = sizeof(Kit_DsdObj_t) + sizeof(unsigned) * (Kit_DsdObjOffset(nFans) + (Type == KIT_DSD_PRIME) * Kit_TruthWordNum(nFans));
    pObj = (Kit_DsdObj_t *)ABC_ALLOC( char, nSize );
    memset( pObj, 0, (size_t)nSize );
    pObj->Id = pNtk->nVars + pNtk->nNodes;
    pObj->Type = Type;
    pObj->nFans = nFans;
    pObj->Offset = Kit_DsdObjOffset( nFans );
    // add the object
    if ( pNtk->nNodes == pNtk->nNodesAlloc )
    {
        pNtk->nNodesAlloc *= 2;
        pNtk->pNodes = ABC_REALLOC( Kit_DsdObj_t *, pNtk->pNodes, pNtk->nNodesAlloc );
    }
    assert( pNtk->nNodes < pNtk->nNodesAlloc );
    pNtk->pNodes[pNtk->nNodes++] = pObj;
    return pObj;
}


unsigned Kit_TruthSupport( unsigned * pTruth, int nVars )
{
    int i, Support = 0;
    for ( i = 0; i < nVars; i++ )
        if ( Kit_TruthVarInSupport( pTruth, nVars, i ) )
            Support |= (1 << i);
    return Support;
}


int Kit_TruthVarInSupport( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Kit_TruthWordNum( nVars );
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


void Kit_DsdDecompose_rec( Kit_DsdNtk_t * pNtk, Kit_DsdObj_t * pObj, unsigned uSupp, unsigned short * pPar, int nDecMux )
{
    Kit_DsdObj_t * pRes, * pRes0, * pRes1;
    int nWords = Kit_TruthWordNum(pObj->nFans);
    unsigned * pTruth = Kit_DsdObjTruth(pObj);
    unsigned * pCofs2[2] = { pNtk->pMem, pNtk->pMem + nWords };
    unsigned * pCofs4[2][2] = { {pNtk->pMem + 2 * nWords, pNtk->pMem + 3 * nWords}, {pNtk->pMem + 4 * nWords, pNtk->pMem + 5 * nWords} };
    int i, iLit0, iLit1, nFans0, nFans1, nPairs;
    int fEquals[2][2], fOppos, fPairs[4][4];
    unsigned j, k, nFansNew, uSupp0, uSupp1;

    assert( pObj->nFans > 0 );
    assert( pObj->Type == KIT_DSD_PRIME );
    assert( uSupp == (uSupp0 = (unsigned)Kit_TruthSupport(pTruth, pObj->nFans)) );

    // compress the truth table
    if ( uSupp != Kit_BitMask(pObj->nFans) )
    {
        nFansNew = Kit_WordCountOnes(uSupp);
        Kit_TruthShrink( pNtk->pMem, pTruth, nFansNew, pObj->nFans, uSupp, 1 );
        for ( j = k = 0; j < pObj->nFans; j++ )
            if ( uSupp & (1 << j) )
                pObj->pFans[k++] = pObj->pFans[j];
        assert( k == nFansNew );
        pObj->nFans = k;
        uSupp = Kit_BitMask(pObj->nFans);
    }

    // consider the single variable case
    if ( pObj->nFans == 1 )
    {
        pObj->Type = KIT_DSD_NONE;
        if ( pTruth[0] == 0x55555555 )
            pObj->pFans[0] = Abc_LitNot(pObj->pFans[0]);
        else
            assert( pTruth[0] == 0xAAAAAAAA );
        // update the parent pointer
        *pPar = Abc_LitNotCond( pObj->pFans[0], Abc_LitIsCompl(*pPar) );
        return;
    }

    // decompose the output
    if ( !pObj->fMark )
    for ( i = pObj->nFans - 1; i >= 0; i-- )
    {
        // get the two-variable cofactors
        Kit_TruthCofactor0New( pCofs2[0], pTruth, pObj->nFans, i );
        Kit_TruthCofactor1New( pCofs2[1], pTruth, pObj->nFans, i );
//        assert( !Kit_TruthVarInSupport( pCofs2[0], pObj->nFans, i) );
//        assert( !Kit_TruthVarInSupport( pCofs2[1], pObj->nFans, i) );
        // get the constant cofs
        fEquals[0][0] = Kit_TruthIsConst0( pCofs2[0], pObj->nFans );
        fEquals[0][1] = Kit_TruthIsConst0( pCofs2[1], pObj->nFans );
        fEquals[1][0] = Kit_TruthIsConst1( pCofs2[0], pObj->nFans );
        fEquals[1][1] = Kit_TruthIsConst1( pCofs2[1], pObj->nFans );
        fOppos        = Kit_TruthIsOpposite( pCofs2[0], pCofs2[1], pObj->nFans );
        assert( !Kit_TruthIsEqual(pCofs2[0], pCofs2[1], pObj->nFans) );
        if ( fEquals[0][0] + fEquals[0][1] + fEquals[1][0] + fEquals[1][1] + fOppos == 0 )
        {
            // check the MUX decomposition
            uSupp0 = Kit_TruthSupport( pCofs2[0], pObj->nFans );
            uSupp1 = Kit_TruthSupport( pCofs2[1], pObj->nFans );
            assert( uSupp == (uSupp0 | uSupp1 | (1<<i)) );
            if ( uSupp0 & uSupp1 )
                continue;
            // perform MUX decomposition
            pRes0 = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, pObj->nFans );
            pRes1 = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, pObj->nFans );
            for ( k = 0; k < pObj->nFans; k++ )
            {
                pRes0->pFans[k] = (uSupp0 & (1 << k))? pObj->pFans[k] : 127;
                pRes1->pFans[k] = (uSupp1 & (1 << k))? pObj->pFans[k] : 127;
            }
            Kit_TruthCopy( Kit_DsdObjTruth(pRes0), pCofs2[0], pObj->nFans );
            Kit_TruthCopy( Kit_DsdObjTruth(pRes1), pCofs2[1], pObj->nFans );
            // update the current one
            assert( pObj->Type == KIT_DSD_PRIME );
            pTruth[0] = 0xCACACACA;
            pObj->nFans = 3;
            pObj->pFans[2] = pObj->pFans[i];
            pObj->pFans[0] = 2*pRes0->Id; pRes0->nRefs++;
            pObj->pFans[1] = 2*pRes1->Id; pRes1->nRefs++;
            // call recursively
            Kit_DsdDecompose_rec( pNtk, pRes0, uSupp0, pObj->pFans + 0, nDecMux );
            Kit_DsdDecompose_rec( pNtk, pRes1, uSupp1, pObj->pFans + 1, nDecMux );
            return;
        }

        // create the new node
        pRes = Kit_DsdObjAlloc( pNtk, KIT_DSD_AND, 2 );
        pRes->nRefs++;
        pRes->nFans = 2;
        pRes->pFans[0] = pObj->pFans[i];  pObj->pFans[i] = 127;  uSupp &= ~(1 << i);
        pRes->pFans[1] = 2*pObj->Id;
        // update the parent pointer
        *pPar = Abc_LitNotCond( 2 * pRes->Id, Abc_LitIsCompl(*pPar) );
        // consider different decompositions
        if ( fEquals[0][0] )
        {
            Kit_TruthCopy( pTruth, pCofs2[1], pObj->nFans );
        }
        else if ( fEquals[0][1] )
        {
            pRes->pFans[0] = Abc_LitNot(pRes->pFans[0]);
            Kit_TruthCopy( pTruth, pCofs2[0], pObj->nFans );
        }
        else if ( fEquals[1][0] )
        {
            *pPar = Abc_LitNot(*pPar);
            pRes->pFans[1] = Abc_LitNot(pRes->pFans[1]);
            Kit_TruthCopy( pTruth, pCofs2[1], pObj->nFans );
        }
        else if ( fEquals[1][1] )
        {
            *pPar = Abc_LitNot(*pPar);
            pRes->pFans[0] = Abc_LitNot(pRes->pFans[0]);
            pRes->pFans[1] = Abc_LitNot(pRes->pFans[1]);
            Kit_TruthCopy( pTruth, pCofs2[0], pObj->nFans );
        }
        else if ( fOppos )
        {
            pRes->Type = KIT_DSD_XOR;
            Kit_TruthCopy( pTruth, pCofs2[0], pObj->nFans );
        }
        else
            assert( 0 );
        // decompose the remainder
        assert( Kit_DsdObjTruth(pObj) == pTruth );
        Kit_DsdDecompose_rec( pNtk, pObj, uSupp, pRes->pFans + 1, nDecMux );
        return;
    }
    pObj->fMark = 1;

    // decompose the input
    for ( i = pObj->nFans - 1; i >= 0; i-- )
    {
        assert( Kit_TruthVarInSupport( pTruth, pObj->nFans, i ) );
        // get the single variale cofactors
        Kit_TruthCofactor0New( pCofs2[0], pTruth, pObj->nFans, i );
        Kit_TruthCofactor1New( pCofs2[1], pTruth, pObj->nFans, i );
        // check the existence of MUX decomposition
        uSupp0 = Kit_TruthSupport( pCofs2[0], pObj->nFans );
        uSupp1 = Kit_TruthSupport( pCofs2[1], pObj->nFans );
        assert( uSupp == (uSupp0 | uSupp1 | (1<<i)) );
        // if one of the cofs is a constant, it is time to check the output again
        if ( uSupp0 == 0 || uSupp1 == 0 )
        {
            pObj->fMark = 0;
            Kit_DsdDecompose_rec( pNtk, pObj, uSupp, pPar, nDecMux );
            return;
        }
        assert( uSupp0 && uSupp1 );
        // get the number of unique variables
        nFans0 = Kit_WordCountOnes( uSupp0 & ~uSupp1 );
        nFans1 = Kit_WordCountOnes( uSupp1 & ~uSupp0 );
        if ( nFans0 == 1 && nFans1 == 1 )
        {
            // get the cofactors w.r.t. the unique variables
            iLit0 = Kit_WordFindFirstBit( uSupp0 & ~uSupp1 );
            iLit1 = Kit_WordFindFirstBit( uSupp1 & ~uSupp0 );
            // get four cofactors
            Kit_TruthCofactor0New( pCofs4[0][0], pCofs2[0], pObj->nFans, iLit0 );
            Kit_TruthCofactor1New( pCofs4[0][1], pCofs2[0], pObj->nFans, iLit0 );
            Kit_TruthCofactor0New( pCofs4[1][0], pCofs2[1], pObj->nFans, iLit1 );
            Kit_TruthCofactor1New( pCofs4[1][1], pCofs2[1], pObj->nFans, iLit1 );
            // check existence conditions
            fEquals[0][0] = Kit_TruthIsEqual( pCofs4[0][0], pCofs4[1][0], pObj->nFans );
            fEquals[0][1] = Kit_TruthIsEqual( pCofs4[0][1], pCofs4[1][1], pObj->nFans );
            fEquals[1][0] = Kit_TruthIsEqual( pCofs4[0][0], pCofs4[1][1], pObj->nFans );
            fEquals[1][1] = Kit_TruthIsEqual( pCofs4[0][1], pCofs4[1][0], pObj->nFans );
            if ( (fEquals[0][0] && fEquals[0][1]) || (fEquals[1][0] && fEquals[1][1]) )
            {
                // construct the MUX
                pRes = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, 3 );
                Kit_DsdObjTruth(pRes)[0] = 0xCACACACA;
                pRes->nRefs++;
                pRes->nFans = 3;
                pRes->pFans[0] = pObj->pFans[iLit0]; pObj->pFans[iLit0] = 127;  uSupp &= ~(1 << iLit0);
                pRes->pFans[1] = pObj->pFans[iLit1]; pObj->pFans[iLit1] = 127;  uSupp &= ~(1 << iLit1);
                pRes->pFans[2] = pObj->pFans[i];     pObj->pFans[i] = 2 * pRes->Id; // remains in support
                // update the node
//                if ( fEquals[0][0] && fEquals[0][1] )
//                    Kit_TruthMuxVar( pTruth, pCofs4[0][0], pCofs4[0][1], pObj->nFans, i );
//                else
//                    Kit_TruthMuxVar( pTruth, pCofs4[0][1], pCofs4[0][0], pObj->nFans, i );
                Kit_TruthMuxVar( pTruth, pCofs4[1][0], pCofs4[1][1], pObj->nFans, i );
                if ( fEquals[1][0] && fEquals[1][1] )
                    pRes->pFans[0] = Abc_LitNot(pRes->pFans[0]);
                // decompose the remainder
                Kit_DsdDecompose_rec( pNtk, pObj, uSupp, pPar, nDecMux );
                return;
            }
        }

        // try other inputs
        for ( k = i+1; k < pObj->nFans; k++ )
        {
            // get four cofactors                                                ik
            Kit_TruthCofactor0New( pCofs4[0][0], pCofs2[0], pObj->nFans, k ); // 00
            Kit_TruthCofactor1New( pCofs4[0][1], pCofs2[0], pObj->nFans, k ); // 01
            Kit_TruthCofactor0New( pCofs4[1][0], pCofs2[1], pObj->nFans, k ); // 10
            Kit_TruthCofactor1New( pCofs4[1][1], pCofs2[1], pObj->nFans, k ); // 11
            // compare equal pairs
            fPairs[0][1] = fPairs[1][0] = Kit_TruthIsEqual( pCofs4[0][0], pCofs4[0][1], pObj->nFans );
            fPairs[0][2] = fPairs[2][0] = Kit_TruthIsEqual( pCofs4[0][0], pCofs4[1][0], pObj->nFans );
            fPairs[0][3] = fPairs[3][0] = Kit_TruthIsEqual( pCofs4[0][0], pCofs4[1][1], pObj->nFans );
            fPairs[1][2] = fPairs[2][1] = Kit_TruthIsEqual( pCofs4[0][1], pCofs4[1][0], pObj->nFans );
            fPairs[1][3] = fPairs[3][1] = Kit_TruthIsEqual( pCofs4[0][1], pCofs4[1][1], pObj->nFans );
            fPairs[2][3] = fPairs[3][2] = Kit_TruthIsEqual( pCofs4[1][0], pCofs4[1][1], pObj->nFans );
            nPairs = fPairs[0][1] + fPairs[0][2] + fPairs[0][3] + fPairs[1][2] + fPairs[1][3] + fPairs[2][3];
            if ( nPairs != 3 && nPairs != 2 )
                continue;

            // decomposition exists
            pRes = Kit_DsdObjAlloc( pNtk, KIT_DSD_AND, 2 );
            pRes->nRefs++;
            pRes->nFans = 2;
            pRes->pFans[0] = pObj->pFans[k]; pObj->pFans[k] = 2 * pRes->Id;  // remains in support
            pRes->pFans[1] = pObj->pFans[i]; pObj->pFans[i] = 127;       uSupp &= ~(1 << i);
            if ( !fPairs[0][1] && !fPairs[0][2] && !fPairs[0][3] ) // 00
            {
                pRes->pFans[0] = Abc_LitNot(pRes->pFans[0]);
                pRes->pFans[1] = Abc_LitNot(pRes->pFans[1]);
                Kit_TruthMuxVar( pTruth, pCofs4[1][1], pCofs4[0][0], pObj->nFans, k );
            }
            else if ( !fPairs[1][0] && !fPairs[1][2] && !fPairs[1][3] ) // 01
            {
                pRes->pFans[1] = Abc_LitNot(pRes->pFans[1]);
                Kit_TruthMuxVar( pTruth, pCofs4[0][0], pCofs4[0][1], pObj->nFans, k );
            }
            else if ( !fPairs[2][0] && !fPairs[2][1] && !fPairs[2][3] ) // 10
            {
                pRes->pFans[0] = Abc_LitNot(pRes->pFans[0]);
                Kit_TruthMuxVar( pTruth, pCofs4[0][0], pCofs4[1][0], pObj->nFans, k );
            }
            else if ( !fPairs[3][0] && !fPairs[3][1] && !fPairs[3][2] ) // 11
            {
//                unsigned uSupp0 = Kit_TruthSupport(pCofs4[0][0], pObj->nFans);
//                unsigned uSupp1 = Kit_TruthSupport(pCofs4[1][1], pObj->nFans);
//                unsigned uSupp;
//                Extra_PrintBinary( stdout, &uSupp0, pObj->nFans ); printf( "\n" );
//                Extra_PrintBinary( stdout, &uSupp1, pObj->nFans ); printf( "\n" );
                Kit_TruthMuxVar( pTruth, pCofs4[0][0], pCofs4[1][1], pObj->nFans, k );
//                uSupp = Kit_TruthSupport(pTruth, pObj->nFans);
//                Extra_PrintBinary( stdout, &uSupp, pObj->nFans ); printf( "\n" ); printf( "\n" );
            }
            else
            {
                assert( fPairs[0][3] && fPairs[1][2] );
                pRes->Type = KIT_DSD_XOR;;
                Kit_TruthMuxVar( pTruth, pCofs4[0][0], pCofs4[0][1], pObj->nFans, k );
            }
            // decompose the remainder
            Kit_DsdDecompose_rec( pNtk, pObj, uSupp, pPar, nDecMux );
            return;
        }
    }

    // if all decomposition methods failed and we are still above the limit, perform MUX-decomposition
    if ( nDecMux > 0 && (int)pObj->nFans > nDecMux )
    {
        int iBestVar = Kit_TruthBestCofVar( pTruth, pObj->nFans, pCofs2[0], pCofs2[1] );
        uSupp0 = Kit_TruthSupport( pCofs2[0], pObj->nFans );
        uSupp1 = Kit_TruthSupport( pCofs2[1], pObj->nFans );
        // perform MUX decomposition
        pRes0 = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, pObj->nFans );
        pRes1 = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, pObj->nFans );
        for ( k = 0; k < pObj->nFans; k++ )
            pRes0->pFans[k] = pRes1->pFans[k] = pObj->pFans[k];
        Kit_TruthCopy( Kit_DsdObjTruth(pRes0), pCofs2[0], pObj->nFans );
        Kit_TruthCopy( Kit_DsdObjTruth(pRes1), pCofs2[1], pObj->nFans );
        // update the current one
        assert( pObj->Type == KIT_DSD_PRIME );
        pTruth[0] = 0xCACACACA;
        pObj->nFans = 3;
        pObj->pFans[2] = pObj->pFans[iBestVar];
        pObj->pFans[0] = 2*pRes0->Id; pRes0->nRefs++;
        pObj->pFans[1] = 2*pRes1->Id; pRes1->nRefs++;
        // call recursively
        Kit_DsdDecompose_rec( pNtk, pRes0, uSupp0, pObj->pFans + 0, nDecMux );
        Kit_DsdDecompose_rec( pNtk, pRes1, uSupp1, pObj->pFans + 1, nDecMux );
    }

}


void Kit_TruthShrink( unsigned * pOut, unsigned * pIn, int nVars, int nVarsAll, unsigned Phase, int fReturnIn )
{
    unsigned * pTemp;
    int i, k, Var = 0, Counter = 0;
    for ( i = 0; i < nVarsAll; i++ )
        if ( Phase & (1 << i) )
        {
            for ( k = i-1; k >= Var; k-- )
            {
                Kit_TruthSwapAdjacentVars( pOut, pIn, nVarsAll, k );
                pTemp = pIn; pIn = pOut; pOut = pTemp;
                Counter++;
            }
            Var++;
        }
    assert( Var == nVars );
    // swap if it was moved an even number of times
    if ( fReturnIn ^ !(Counter & 1) )
        Kit_TruthCopy( pOut, pIn, nVarsAll );
}


void Kit_TruthSwapAdjacentVars( unsigned * pOut, unsigned * pIn, int nVars, int iVar )
{
    static unsigned PMasks[4][3] = {
        { 0x99999999, 0x22222222, 0x44444444 },
        { 0xC3C3C3C3, 0x0C0C0C0C, 0x30303030 },
        { 0xF00FF00F, 0x00F000F0, 0x0F000F00 },
        { 0xFF0000FF, 0x0000FF00, 0x00FF0000 }
    };
    int nWords = Kit_TruthWordNum( nVars );
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


void Kit_TruthCofactor0New( unsigned * pOut, unsigned * pIn, int nVars, int iVar )
{
    int nWords = Kit_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0x55555555) | ((pIn[i] & 0x55555555) << 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0x33333333) | ((pIn[i] & 0x33333333) << 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0x0F0F0F0F) | ((pIn[i] & 0x0F0F0F0F) << 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0x00FF00FF) | ((pIn[i] & 0x00FF00FF) << 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0x0000FFFF) | ((pIn[i] & 0x0000FFFF) << 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
                pOut[i] = pOut[Step+i] = pIn[i];
            pIn += 2*Step;
            pOut += 2*Step;
        }
        return;
    }
}


void Kit_TruthCofactor1New( unsigned * pOut, unsigned * pIn, int nVars, int iVar )
{
    int nWords = Kit_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0xAAAAAAAA) | ((pIn[i] & 0xAAAAAAAA) >> 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0xCCCCCCCC) | ((pIn[i] & 0xCCCCCCCC) >> 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0xF0F0F0F0) | ((pIn[i] & 0xF0F0F0F0) >> 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0xFF00FF00) | ((pIn[i] & 0xFF00FF00) >> 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0xFFFF0000) | ((pIn[i] & 0xFFFF0000) >> 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
                pOut[i] = pOut[Step+i] = pIn[Step+i];
            pIn += 2*Step;
            pOut += 2*Step;
        }
        return;
    }
}


void Kit_TruthMuxVar( unsigned * pOut, unsigned * pCof0, unsigned * pCof1, int nVars, int iVar )
{
    int nWords = Kit_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pCof0[i] & 0x55555555) | (pCof1[i] & 0xAAAAAAAA);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pCof0[i] & 0x33333333) | (pCof1[i] & 0xCCCCCCCC);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pCof0[i] & 0x0F0F0F0F) | (pCof1[i] & 0xF0F0F0F0);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pCof0[i] & 0x00FF00FF) | (pCof1[i] & 0xFF00FF00);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pCof0[i] & 0x0000FFFF) | (pCof1[i] & 0xFFFF0000);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                pOut[i]      = pCof0[i];
                pOut[Step+i] = pCof1[Step+i];
            }
            pOut += 2*Step;
            pCof0 += 2*Step;
            pCof1 += 2*Step;
        }
        return;
    }
}


Kit_DsdObj_t * Kit_DsdNonDsdPrimeMax( Kit_DsdNtk_t * pNtk )
{
    Kit_DsdObj_t * pObj, * pObjMax = NULL;
    unsigned i, nSizeMax = 0;
    Kit_DsdNtkForEachObj( pNtk, pObj, i )
    {
        if ( pObj->Type != KIT_DSD_PRIME )
            continue;
        if ( nSizeMax < pObj->nFans )
        {
            nSizeMax = pObj->nFans;
            pObjMax = pObj;
        }
    }
    return pObjMax;
}


void Kit_DsdNtkFree( Kit_DsdNtk_t * pNtk )
{
    Kit_DsdObj_t * pObj;
    unsigned i;
    Kit_DsdNtkForEachObj( pNtk, pObj, i )
        ABC_FREE( pObj );
    ABC_FREE( pNtk->pSupps );
    ABC_FREE( pNtk->pNodes );
    ABC_FREE( pNtk->pMem );
    ABC_FREE( pNtk );
}


void Kit_TruthPermute( unsigned * pOut, unsigned * pIn, int nVars, char * pPerm, int fReturnIn )
{
    unsigned * pTemp;
    int i, Temp, fChange, Counter = 0;
    do {
        fChange = 0;
        for ( i = 0; i < nVars-1; i++ )
        {
            assert( pPerm[i] != pPerm[i+1] );
            if ( pPerm[i] <= pPerm[i+1] )
                continue;
            Counter++;
            fChange = 1;

            Temp = pPerm[i];
            pPerm[i] = pPerm[i+1];
            pPerm[i+1] = Temp;

            Kit_TruthSwapAdjacentVars( pOut, pIn, nVars, i );
            pTemp = pIn; pIn = pOut; pOut = pTemp;
        }
    } while ( fChange );
    if ( fReturnIn ^ !(Counter & 1) )
        Kit_TruthCopy( pOut, pIn, nVars );
}


void Kit_TruthChangePhase( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Kit_TruthWordNum( nVars );
    int i, k, Step;
    unsigned Temp;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ((pTruth[i] & 0x55555555) << 1) | ((pTruth[i] & 0xAAAAAAAA) >> 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ((pTruth[i] & 0x33333333) << 2) | ((pTruth[i] & 0xCCCCCCCC) >> 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ((pTruth[i] & 0x0F0F0F0F) << 4) | ((pTruth[i] & 0xF0F0F0F0) >> 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ((pTruth[i] & 0x00FF00FF) << 8) | ((pTruth[i] & 0xFF00FF00) >> 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ((pTruth[i] & 0x0000FFFF) << 16) | ((pTruth[i] & 0xFFFF0000) >> 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                Temp = pTruth[i];
                pTruth[i] = pTruth[Step+i];
                pTruth[Step+i] = Temp;
            }
            pTruth += 2*Step;
        }
        return;
    }
}


void Kit_TruthCountOnesInCofs0( unsigned * pTruth, int nVars, int * pStore )
{
    int nWords = Kit_TruthWordNum( nVars );
    int i, k, Counter;
    memset( pStore, 0, sizeof(int) * nVars );
    if ( nVars <= 5 )
    {
        if ( nVars > 0 )
            pStore[0] = Kit_WordCountOnes( pTruth[0] & 0x55555555 );
        if ( nVars > 1 )
            pStore[1] = Kit_WordCountOnes( pTruth[0] & 0x33333333 );
        if ( nVars > 2 )
            pStore[2] = Kit_WordCountOnes( pTruth[0] & 0x0F0F0F0F );
        if ( nVars > 3 )
            pStore[3] = Kit_WordCountOnes( pTruth[0] & 0x00FF00FF );
        if ( nVars > 4 )
            pStore[4] = Kit_WordCountOnes( pTruth[0] & 0x0000FFFF );
        return;
    }
    // nVars >= 6
    // count 1's for all other variables
    for ( k = 0; k < nWords; k++ )
    {
        Counter = Kit_WordCountOnes( pTruth[k] );
        for ( i = 5; i < nVars; i++ )
            if ( (k & (1 << (i-5))) == 0 )
                pStore[i] += Counter;
    }
    // count 1's for the first five variables
    for ( k = 0; k < nWords/2; k++ )
    {
        pStore[0] += Kit_WordCountOnes( (pTruth[0] & 0x55555555) | ((pTruth[1] & 0x55555555) <<  1) );
        pStore[1] += Kit_WordCountOnes( (pTruth[0] & 0x33333333) | ((pTruth[1] & 0x33333333) <<  2) );
        pStore[2] += Kit_WordCountOnes( (pTruth[0] & 0x0F0F0F0F) | ((pTruth[1] & 0x0F0F0F0F) <<  4) );
        pStore[3] += Kit_WordCountOnes( (pTruth[0] & 0x00FF00FF) | ((pTruth[1] & 0x00FF00FF) <<  8) );
        pStore[4] += Kit_WordCountOnes( (pTruth[0] & 0x0000FFFF) | ((pTruth[1] & 0x0000FFFF) << 16) );
        pTruth += 2;
    }
}


int Kit_TruthVarIsVacuous( unsigned * pOnset, unsigned * pOffset, int nVars, int iVar )
{
    int nWords = Kit_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            if ( ((pOnset[i] & (pOffset[i] >> 1)) | (pOffset[i] & (pOnset[i] >> 1))) & 0x55555555 )
                 return 0;
        return 1;
    case 1:
        for ( i = 0; i < nWords; i++ )
            if ( ((pOnset[i] & (pOffset[i] >> 2)) | (pOffset[i] & (pOnset[i] >> 2))) & 0x33333333 )
                 return 0;
        return 1;
    case 2:
        for ( i = 0; i < nWords; i++ )
            if ( ((pOnset[i] & (pOffset[i] >> 4)) | (pOffset[i] & (pOnset[i] >> 4))) & 0x0F0F0F0F )
                 return 0;
        return 1;
    case 3:
        for ( i = 0; i < nWords; i++ )
            if ( ((pOnset[i] & (pOffset[i] >> 8)) | (pOffset[i] & (pOnset[i] >> 8))) & 0x00FF00FF )
                 return 0;
        return 1;
    case 4:
        for ( i = 0; i < nWords; i++ )
            if ( ((pOnset[i] & (pOffset[i] >> 16)) | (pOffset[i] & (pOnset[i] >> 16))) & 0x0000FFFF )
                 return 0;
        return 1;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
                if ( (pOnset[i] & pOffset[Step+i]) | (pOffset[i] & pOnset[Step+i]) )
                     return 0;
            pOnset += 2*Step;
            pOffset += 2*Step;
        }
        return 1;
    }
}


void Kit_TruthExist( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Kit_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] |=  ((pTruth[i] & 0xAAAAAAAA) >> 1) | ((pTruth[i] & 0x55555555) << 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] |=  ((pTruth[i] & 0xCCCCCCCC) >> 2) | ((pTruth[i] & 0x33333333) << 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] |=  ((pTruth[i] & 0xF0F0F0F0) >> 4) | ((pTruth[i] & 0x0F0F0F0F) << 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] |=  ((pTruth[i] & 0xFF00FF00) >> 8) | ((pTruth[i] & 0x00FF00FF) << 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] |=  ((pTruth[i] & 0xFFFF0000) >> 16) | ((pTruth[i] & 0x0000FFFF) << 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                pTruth[i]     |= pTruth[Step+i];
                pTruth[Step+i] = pTruth[i];
            }
            pTruth += 2*Step;
        }
        return;
    }
}


void Kit_TruthExistNew( unsigned * pRes, unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Kit_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pRes[i] =  pTruth[i] | ((pTruth[i] & 0xAAAAAAAA) >> 1) | ((pTruth[i] & 0x55555555) << 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pRes[i] =  pTruth[i] | ((pTruth[i] & 0xCCCCCCCC) >> 2) | ((pTruth[i] & 0x33333333) << 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pRes[i] =  pTruth[i] | ((pTruth[i] & 0xF0F0F0F0) >> 4) | ((pTruth[i] & 0x0F0F0F0F) << 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pRes[i] =  pTruth[i] | ((pTruth[i] & 0xFF00FF00) >> 8) | ((pTruth[i] & 0x00FF00FF) << 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pRes[i] =  pTruth[i] | ((pTruth[i] & 0xFFFF0000) >> 16) | ((pTruth[i] & 0x0000FFFF) << 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                pRes[i]      = pTruth[i] | pTruth[Step+i];
                pRes[Step+i] = pRes[i];
            }
            pRes += 2*Step;
            pTruth += 2*Step;
        }
        return;
    }
}


void Kit_TruthExistSet( unsigned * pRes, unsigned * pTruth, int nVars, unsigned uMask )
{
    int v;
    Kit_TruthCopy( pRes, pTruth, nVars );
    for ( v = 0; v < nVars; v++ )
        if ( uMask & (1 << v) )
            Kit_TruthExist( pRes, nVars, v );
}


void Kit_TruthForallNew( unsigned * pRes, unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Kit_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pRes[i] =  pTruth[i] & (((pTruth[i] & 0xAAAAAAAA) >> 1) | ((pTruth[i] & 0x55555555) << 1));
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pRes[i] =  pTruth[i] & (((pTruth[i] & 0xCCCCCCCC) >> 2) | ((pTruth[i] & 0x33333333) << 2));
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pRes[i] =  pTruth[i] & (((pTruth[i] & 0xF0F0F0F0) >> 4) | ((pTruth[i] & 0x0F0F0F0F) << 4));
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pRes[i] =  pTruth[i] & (((pTruth[i] & 0xFF00FF00) >> 8) | ((pTruth[i] & 0x00FF00FF) << 8));
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pRes[i] =  pTruth[i] & (((pTruth[i] & 0xFFFF0000) >> 16) | ((pTruth[i] & 0x0000FFFF) << 16));
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                pRes[i]      = pTruth[i] & pTruth[Step+i];
                pRes[Step+i] = pRes[i];
            }
            pRes += 2*Step;
            pTruth += 2*Step;
        }
        return;
    }
}


int Kit_TruthBestCofVar( unsigned * pTruth, int nVars, unsigned * pCof0, unsigned * pCof1 )
{
    int i, iBestVar, nSuppSizeCur0, nSuppSizeCur1, nSuppSizeCur, nSuppSizeMin;
    if ( Kit_TruthIsConst0(pTruth, nVars) || Kit_TruthIsConst1(pTruth, nVars) )
        return -1;
    // iterate through variables
    iBestVar = -1;
    nSuppSizeMin = KIT_INFINITY;
    for ( i = 0; i < nVars; i++ )
    {
        // cofactor the functiona and get support sizes
        Kit_TruthCofactor0New( pCof0, pTruth, nVars, i );
        Kit_TruthCofactor1New( pCof1, pTruth, nVars, i );
        nSuppSizeCur0 = Kit_TruthSupportSize( pCof0, nVars );
        nSuppSizeCur1 = Kit_TruthSupportSize( pCof1, nVars );
        nSuppSizeCur  = nSuppSizeCur0 + nSuppSizeCur1;
        // compare this variable with other variables
        if ( nSuppSizeMin > nSuppSizeCur )
        {
            nSuppSizeMin = nSuppSizeCur;
            iBestVar = i;
        }
    }
    assert( iBestVar != -1 );
    // cofactor w.r.t. this variable
    Kit_TruthCofactor0New( pCof0, pTruth, nVars, iBestVar );
    Kit_TruthCofactor1New( pCof1, pTruth, nVars, iBestVar );
    return iBestVar;
}


int Kit_TruthSupportSize( unsigned * pTruth, int nVars )
{
    int i, Counter = 0;
    for ( i = 0; i < nVars; i++ )
        Counter += Kit_TruthVarInSupport( pTruth, nVars, i );
    return Counter;
}
