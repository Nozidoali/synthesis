#ifndef ABC_VEC_H
#define ABC_VEC_H


#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include "abc_memory.h"


#define Vec_IntForEachEntry( vVec, Entry, i )                                               \
    for ( i = 0; (i < Vec_IntSize(vVec)) && (((Entry) = Vec_IntEntry(vVec, i)), 1); i++ )
#define Vec_PtrForEachEntry( Type, vVec, pEntry, i )                                               \
    for ( i = 0; (i < Vec_PtrSize(vVec)) && (((pEntry) = (Type)Vec_PtrEntry(vVec, i)), 1); i++ )
#define Vec_PtrForEachEntryStart( Type, vVec, pEntry, i, Start )                                   \
    for ( i = Start; (i < Vec_PtrSize(vVec)) && (((pEntry) = (Type)Vec_PtrEntry(vVec, i)), 1); i++ )
#define Vec_VecForEachLevelInt( vGlob, vVec, i )                                              \
    for ( i = 0; (i < Vec_VecSize(vGlob)) && (((vVec) = Vec_VecEntryInt(vGlob, i)), 1); i++ )
#define Vec_VecForEachEntry( Type, vGlob, pEntry, i, k )                                      \
    for ( i = 0; i < Vec_VecSize(vGlob); i++ )                                                \
        Vec_PtrForEachEntry( Type, Vec_VecEntry(vGlob, i), pEntry, k )
#define Vec_VecForEachLevel( vGlob, vVec, i )                                                 \
    for ( i = 0; (i < Vec_VecSize(vGlob)) && (((vVec) = Vec_VecEntry(vGlob, i)), 1); i++ )


// various attributes
typedef enum {
    VEC_ATTR_NONE = 0,     // 0
    VEC_ATTR_COPY,         // 1
    VEC_ATTR_LOCAL_AIG,    // 2
    VEC_ATTR_LOCAL_SOP,    // 3
    VEC_ATTR_LOCAL_BDD,    // 4
    VEC_ATTR_GLOBAL_AIG,   // 5
    VEC_ATTR_GLOBAL_SOP,   // 6
    VEC_ATTR_GLOBAL_BDD,   // 7
    VEC_ATTR_LEVEL,        // 8
    VEC_ATTR_LEVEL_REV,    // 9
    VEC_ATTR_RETIME_LAG,   // 10
    VEC_ATTR_FRAIG,        // 11
    VEC_ATTR_MVVAR,        // 12
    VEC_ATTR_DATA1,        // 13
    VEC_ATTR_DATA2,        // 14
    VEC_ATTR_TOTAL_NUM     // 15
} Vec_AttrType_t;


typedef struct Vec_Int_t_       Vec_Int_t;
typedef struct Vec_Ptr_t_       Vec_Ptr_t;
typedef struct Vec_Str_t_       Vec_Str_t;
typedef struct Vec_Vec_t_       Vec_Vec_t;
typedef struct Vec_Att_t_  Vec_Att_t;


struct Vec_Int_t_
{
    int              nCap;
    int              nSize;
    int *            pArray;
};


struct Vec_Ptr_t_
{
    int              nCap;
    int              nSize;
    void **          pArray;
};


struct Vec_Vec_t_
{
    int              nCap;
    int              nSize;
    void **          pArray;
};


struct Vec_Str_t_
{
    int              nCap;
    int              nSize;
    char *           pArray;
};


struct Vec_Att_t_
{
    // storage for attributes
    int              nCap;                 // the size of array allocated
    // Removed pArrayInt as it's not 64-bit safe, it generates compiler
    // warnings, and it's unused.
    void **          pArrayPtr;            // the pointer attribute array
    // attribute specific info
    void *           pMan;                 // the manager for this attribute
    void (*pFuncFreeMan) (void *);         // the procedure to free the manager
    void*(*pFuncStartObj)(void *);         // the procedure to start one attribute
    void (*pFuncFreeObj) (void *, void *); // the procedure to free one attribute
};

static inline int Vec_IntSize( Vec_Int_t * p )
{
    return p->nSize;
}


static inline int Vec_IntFind( Vec_Int_t * p, int Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return i;
    return -1;
}


static inline void Vec_IntGrow( Vec_Int_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( int, p->pArray, nCapMin );
    assert( p->pArray );
    p->nCap   = nCapMin;
}


static inline void Vec_IntFill( Vec_Int_t * p, int nSize, int Fill )
{
    int i;
    Vec_IntGrow( p, nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}


static inline Vec_Int_t * Vec_IntAlloc( int nCap )
{
    Vec_Int_t * p;
    p = ABC_ALLOC( Vec_Int_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( int, p->nCap ) : NULL;
    return p;
}


static inline void Vec_IntPush( Vec_Int_t * p, int Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_IntGrow( p, 16 );
        else
            Vec_IntGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}


static inline void Vec_IntFree( Vec_Int_t * p )
{
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}


static inline void Vec_IntFreeP( Vec_Int_t ** p )
{
    if ( *p == NULL )
        return;
    ABC_FREE( (*p)->pArray );
    ABC_FREE( (*p) );
}


static inline int Vec_IntEntry( Vec_Int_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}


static inline int * Vec_IntEntryP( Vec_Int_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray + i;
}


static inline void Vec_IntPushMem( Mem_Step_t * pMemMan, Vec_Int_t * p, int Entry )
{
    if ( p->nSize == p->nCap )
    {
        int * pArray;
        int i;

        if ( p->nSize == 0 )
            p->nCap = 1;
        if ( pMemMan )
            pArray = (int *)Mem_StepEntryFetch( pMemMan, p->nCap * 8 );
        else
            pArray = ABC_ALLOC( int, p->nCap * 2 );
        if ( p->pArray )
        {
            for ( i = 0; i < p->nSize; i++ )
                pArray[i] = p->pArray[i];
            if ( pMemMan )
                Mem_StepEntryRecycle( pMemMan, (char *)p->pArray, p->nCap * 4 );
            else
                ABC_FREE( p->pArray );
        }
        p->nCap *= 2;
        p->pArray = pArray;
    }
    p->pArray[p->nSize++] = Entry;
}


static inline int Vec_IntRemove( Vec_Int_t * p, int Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            break;
    if ( i == p->nSize )
        return 0;
    assert( i < p->nSize );
    for ( i++; i < p->nSize; i++ )
        p->pArray[i-1] = p->pArray[i];
    p->nSize--;
    return 1;
}


static inline void Vec_IntFillExtra( Vec_Int_t * p, int nSize, int Fill )
{
    int i;
    if ( nSize <= p->nSize )
        return;
    if ( nSize > 2 * p->nCap )
        Vec_IntGrow( p, nSize );
    else if ( nSize > p->nCap )
        Vec_IntGrow( p, 2 * p->nCap );
    for ( i = p->nSize; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}


static inline int Vec_IntGetEntry( Vec_Int_t * p, int i )
{
    Vec_IntFillExtra( p, i + 1, 0 );
    return Vec_IntEntry( p, i );
}


static inline void Vec_IntWriteEntry( Vec_Int_t * p, int i, int Entry )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] = Entry;
}


static inline void Vec_IntSetEntry( Vec_Int_t * p, int i, int Entry )
{
    Vec_IntFillExtra( p, i + 1, 0 );
    Vec_IntWriteEntry( p, i, Entry );
}


static inline Vec_Int_t * Vec_IntDup( Vec_Int_t * pVec )
{
    Vec_Int_t * p;
    p = ABC_ALLOC( Vec_Int_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nSize;
    p->pArray = p->nCap? ABC_ALLOC( int, p->nCap ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(int) * (size_t)pVec->nSize );
    return p;
}


static inline Vec_Int_t * Vec_IntStart( int nSize )
{
    Vec_Int_t * p;
    p = Vec_IntAlloc( nSize );
    p->nSize = nSize;
    if ( p->pArray ) memset( p->pArray, 0, sizeof(int) * (size_t)nSize );
    return p;
}


static inline int * Vec_IntArray( Vec_Int_t * p )
{
    return p->pArray;
}


static inline void Vec_IntClear( Vec_Int_t * p )
{
    p->nSize = 0;
}


static inline unsigned * Vec_IntFetch( Vec_Int_t * p, int nWords )
{
    if ( nWords == 0 )
        return NULL;
    assert( nWords > 0 );
    p->nSize += nWords;
    if ( p->nSize > p->nCap )
    {
//         Vec_IntGrow( p, 2 * p->nSize );
        return NULL;
    }
    return ((unsigned *)p->pArray) + p->nSize - nWords;
}


static inline void Vec_PtrClear( Vec_Ptr_t * p )
{
    p->nSize = 0;
}


static inline void Vec_PtrRemove( Vec_Ptr_t * p, void * Entry )
{
    int i;
    // delete assuming that it is closer to the end
    for ( i = p->nSize - 1; i >= 0; i-- )
        if ( p->pArray[i] == Entry )
            break;
    assert( i >= 0 );
/*
    // delete assuming that it is closer to the beginning
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            break;
    assert( i < p->nSize );
*/
    for ( i++; i < p->nSize; i++ )
        p->pArray[i-1] = p->pArray[i];
    p->nSize--;
}


static inline void Vec_PtrWriteEntry( Vec_Ptr_t * p, int i, void * Entry )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] = Entry;
}


static inline Vec_Ptr_t * Vec_PtrAlloc( int nCap )
{
    Vec_Ptr_t * p;
    p = ABC_ALLOC( Vec_Ptr_t, 1 );
    if ( nCap > 0 && nCap < 8 )
        nCap = 8;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( void *, p->nCap ) : NULL;
    return p;
}


static inline Vec_Ptr_t * Vec_PtrAllocArray( void ** pArray, int nSize )
{
    Vec_Ptr_t * p;
    p = ABC_ALLOC( Vec_Ptr_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = pArray;
    return p;
}


static inline Vec_Ptr_t * Vec_PtrAllocSimInfo( int nEntries, int nWords )
{
    void ** pMemory;
    unsigned * pInfo;
    int i;
    pMemory = (void **)ABC_ALLOC( char, (sizeof(void *) + sizeof(unsigned) * (size_t)nWords) * nEntries );
    pInfo = (unsigned *)(pMemory + nEntries);
    for ( i = 0; i < nEntries; i++ )
        pMemory[i] = pInfo + i * nWords;
    return Vec_PtrAllocArray( pMemory, nEntries );
}


static inline Vec_Ptr_t * Vec_PtrAllocTruthTables( int nVars )
{
    Vec_Ptr_t * p;
    unsigned Masks[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    unsigned * pTruth;
    int i, k, nWords;
    nWords = (nVars <= 5 ? 1 : (1 << (nVars - 5)));
    p = Vec_PtrAllocSimInfo( nVars, nWords );
    for ( i = 0; i < nVars; i++ )
    {
        pTruth = (unsigned *)p->pArray[i];
        if ( i < 5 )
        {
            for ( k = 0; k < nWords; k++ )
                pTruth[k] = Masks[i];
        }
        else
        {
            for ( k = 0; k < nWords; k++ )
                if ( k & (1 << (i-5)) )
                    pTruth[k] = ~(unsigned)0;
                else
                    pTruth[k] = 0;
        }
    }
    return p;
}


static inline void Vec_PtrGrow( Vec_Ptr_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( void *, p->pArray, nCapMin );
    p->nCap   = nCapMin;
}


static inline void Vec_PtrPush( Vec_Ptr_t * p, void * Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_PtrGrow( p, 16 );
        else
            Vec_PtrGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}


static inline Vec_Ptr_t * Vec_PtrStart( int nSize )
{
    Vec_Ptr_t * p;
    p = Vec_PtrAlloc( nSize );
    p->nSize = nSize;
    memset( p->pArray, 0, sizeof(void *) * (size_t)nSize );
    return p;
}


static inline int Vec_PtrSize( Vec_Ptr_t * p )
{
    return p->nSize;
}


static inline void * Vec_PtrEntry( Vec_Ptr_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}


static inline void Vec_PtrFill( Vec_Ptr_t * p, int nSize, void * Entry )
{
    int i;
    Vec_PtrGrow( p, nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = Entry;
    p->nSize = nSize;
}


static inline void Vec_PtrFillExtra( Vec_Ptr_t * p, int nSize, void * Fill )
{
    int i;
    if ( nSize <= p->nSize )
        return;
    if ( nSize > 2 * p->nCap )
        Vec_PtrGrow( p, nSize );
    else if ( nSize > p->nCap )
        Vec_PtrGrow( p, 2 * p->nCap );
    for ( i = p->nSize; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}


static inline void Vec_PtrFree( Vec_Ptr_t * p )
{
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}


static inline void Vec_PtrFreeData( Vec_Ptr_t * p )
{
    void * pTemp; int i;
    if ( p == NULL ) return;
    Vec_PtrForEachEntry( void *, p, pTemp, i )
        if ( pTemp != (void *)(ABC_PTRINT_T)1 && pTemp != (void *)(ABC_PTRINT_T)2 )
            ABC_FREE( pTemp );
}


static inline void Vec_PtrFreeFree( Vec_Ptr_t * p )
{
    if ( p == NULL ) return;
    Vec_PtrFreeData( p );
    Vec_PtrFree( p );
}


static inline void Vec_PtrFreeP( Vec_Ptr_t ** p )
{
    if ( *p == NULL )
        return;
    ABC_FREE( (*p)->pArray );
    ABC_FREE( (*p) );
}


static inline void * Vec_PtrPop( Vec_Ptr_t * p )
{
    assert( p->nSize > 0 );
    return p->pArray[--p->nSize];
}


static inline int Vec_PtrPushUnique( Vec_Ptr_t * p, void * Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Vec_PtrPush( p, Entry );
    return 0;
}


static inline int Vec_VecSize( Vec_Vec_t * p )
{
    return p->nSize;
}


static inline Vec_Ptr_t * Vec_VecEntry( Vec_Vec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return (Vec_Ptr_t *)p->pArray[i];
}


static inline Vec_Vec_t * Vec_VecAlloc( int nCap )
{
    Vec_Vec_t * p;
    p = ABC_ALLOC( Vec_Vec_t, 1 );
    if ( nCap > 0 && nCap < 8 )
        nCap = 8;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( void *, p->nCap ) : NULL;
    return p;
}

static inline void Vec_VecFree( Vec_Vec_t * p )
{
    Vec_Ptr_t * vVec;
    int i;
    Vec_VecForEachLevel( p, vVec, i )
        if ( vVec ) Vec_PtrFree( vVec );
    Vec_PtrFree( (Vec_Ptr_t *)p );
}


static inline Vec_Int_t * Vec_VecEntryInt( Vec_Vec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return (Vec_Int_t *)p->pArray[i];
}


static inline Vec_Vec_t * Vec_VecDupInt( Vec_Vec_t * p )
{
    Vec_Ptr_t * vNew;
    Vec_Int_t * vVec;
    int i;
    vNew = Vec_PtrAlloc( Vec_VecSize(p) );
    Vec_VecForEachLevelInt( p, vVec, i )
        Vec_PtrPush( vNew, Vec_IntDup(vVec) );
    return (Vec_Vec_t *)vNew;
}


static inline Vec_Vec_t * Vec_VecStart( int nSize )
{
    Vec_Vec_t * p;
    int i;
    p = Vec_VecAlloc( nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = Vec_PtrAlloc( 0 );
    p->nSize = nSize;
    return p;
}


static inline void Vec_VecPush( Vec_Vec_t * p, int Level, void * Entry )
{
    if ( p->nSize < Level + 1 )
    {
        int i;
        Vec_PtrGrow( (Vec_Ptr_t *)p, Level + 1 );
        for ( i = p->nSize; i < Level + 1; i++ )
            p->pArray[i] = Vec_PtrAlloc( 0 );
        p->nSize = Level + 1;
    }
    Vec_PtrPush( Vec_VecEntry(p, Level), Entry );
}


static inline Vec_Str_t * Vec_StrAlloc( int nCap )
{
    Vec_Str_t * p;
    p = ABC_ALLOC( Vec_Str_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( char, p->nCap ) : NULL;
    return p;
}


static inline void Vec_StrGrow( Vec_Str_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( char, p->pArray, nCapMin );
    p->nCap   = nCapMin;
}

static inline void Vec_StrPush( Vec_Str_t * p, char Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_StrGrow( p, 16 );
        else
            Vec_StrGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}


static inline void Vec_StrPrintStr( Vec_Str_t * p, const char * pStr )
{
    int i, Length = (int)strlen(pStr);
    for ( i = 0; i < Length; i++ )
        Vec_StrPush( p, pStr[i] );
}


static inline void Vec_StrFree( Vec_Str_t * p )
{
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}


static inline void * Vec_AttFree( Vec_Att_t * p, int fFreeMan )
{
    void * pMan;
    if ( p == NULL )
        return NULL;
    // free the attributes of objects
    if ( p->pFuncFreeObj )
    {
        int i;
        for ( i = 0; i < p->nCap; i++ )
            if ( p->pArrayPtr[i] )
                p->pFuncFreeObj( p->pMan, p->pArrayPtr[i] );
    }
    // free the memory manager
    pMan = fFreeMan? NULL : p->pMan;
    if ( p->pMan && fFreeMan )
        p->pFuncFreeMan( p->pMan );
    ABC_FREE( p->pArrayPtr );
    ABC_FREE( p );
    return pMan;
}


#endif
