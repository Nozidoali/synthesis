#ifndef ABC_GLOBAL_H
#define ABC_GLOBAL_H


#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <iostream>

#define ABC_INFINITY    (1000000000)

#define ABC_SWAP(Type, a, b)  { Type t = a; a = b; b = t; }

#define ABC_ALLOC(type, num)     ((type *) malloc(sizeof(type) * (size_t)(num)))
#define ABC_CALLOC(type, num)    ((type *) calloc((size_t)(num), sizeof(type)))
#define ABC_FALLOC(type, num)    ((type *) memset(malloc(sizeof(type) * (size_t)(num)), 0xff, sizeof(type) * (size_t)(num)))
#define ABC_FREE(obj)            ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#define ABC_REALLOC(type, obj, num) \
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (size_t)(num))) : \
         ((type *) malloc(sizeof(type) * (size_t)(num))))

#define ABC_NUM_STEPS  10

typedef long ABC_PTRINT_T;
typedef unsigned long ABC_PTRUINT_T;
typedef unsigned long ABC_UINT64_T;
typedef long ABC_INT64_T;
typedef long ABC_PTRDIFF_T;
typedef ABC_UINT64_T word;
typedef ABC_INT64_T iword;
typedef ABC_INT64_T abctime;


// abc_global.h
static inline int      Abc_AbsInt( int a        )             { return a < 0 ? -a : a; }
static inline int      Abc_MaxInt( int a, int b )             { return a > b ?  a : b; }
static inline int      Abc_MinInt( int a, int b )             { return a < b ?  a : b; }
static inline word     Abc_MaxWord( word a, word b )          { return a > b ?  a : b; }
static inline word     Abc_MinWord( word a, word b )          { return a < b ?  a : b; }
static inline float    Abc_AbsFloat( float a          )       { return a < 0 ? -a : a; }
static inline float    Abc_MaxFloat( float a, float b )       { return a > b ?  a : b; }
static inline float    Abc_MinFloat( float a, float b )       { return a < b ?  a : b; }
static inline double   Abc_AbsDouble( double a           )    { return a < 0 ? -a : a; }
static inline double   Abc_MaxDouble( double a, double b )    { return a > b ?  a : b; }
static inline double   Abc_MinDouble( double a, double b )    { return a < b ?  a : b; }

static inline int      Abc_Float2Int( float Val )             { union { int x; float y; } v; v.y = Val; return v.x;         }
static inline float    Abc_Int2Float( int Num )               { union { int x; float y; } v; v.x = Num; return v.y;         }
static inline word     Abc_Dbl2Word( double Dbl )             { union { word x; double y; } v; v.y = Dbl; return v.x;       }
static inline double   Abc_Word2Dbl( word Num )               { union { word x; double y; } v; v.x = Num; return v.y;       }
static inline int      Abc_Base2Log( unsigned n )             { int r; if ( n < 2 ) return (int)n; for ( r = 0, n--; n; n >>= 1, r++ ) {}; return r; }
static inline int      Abc_Base10Log( unsigned n )            { int r; if ( n < 2 ) return (int)n; for ( r = 0, n--; n; n /= 10, r++ ) {}; return r; }
static inline int      Abc_Base16Log( unsigned n )            { int r; if ( n < 2 ) return (int)n; for ( r = 0, n--; n; n /= 16, r++ ) {}; return r; }
static inline char *   Abc_UtilStrsav( char * s )             { return s ? strcpy(ABC_ALLOC(char, strlen(s)+1), s) : NULL;  }
static inline int      Abc_BitWordNum( int nBits )            { return (nBits>>5) + ((nBits&31) > 0);                       }
static inline int      Abc_Bit6WordNum( int nBits )           { return (nBits>>6) + ((nBits&63) > 0);                       }
static inline int      Abc_TruthWordNum( int nVars )          { return nVars <= 5 ? 1 : (1 << (nVars - 5));                 }
static inline int      Abc_Truth6WordNum( int nVars )         { return nVars <= 6 ? 1 : (1 << (nVars - 6));                 }
static inline int      Abc_InfoHasBit( unsigned * p, int i )  { return (p[(i)>>5] & (unsigned)(1<<((i) & 31))) > 0;         }
static inline void     Abc_InfoSetBit( unsigned * p, int i )  { p[(i)>>5] |= (unsigned)(1<<((i) & 31));                     }
static inline void     Abc_InfoXorBit( unsigned * p, int i )  { p[(i)>>5] ^= (unsigned)(1<<((i) & 31));                     }
static inline unsigned Abc_InfoMask( int nVar )               { return (~(unsigned)0) >> (32-nVar);                         }

static inline int      Abc_Var2Lit( int Var, int c )          { assert(Var >= 0 && !(c >> 1)); return Var + Var + c;        }
static inline int      Abc_Lit2Var( int Lit )                 { assert(Lit >= 0); return Lit >> 1;                          }
static inline int      Abc_LitIsCompl( int Lit )              { assert(Lit >= 0); return Lit & 1;                           }
static inline int      Abc_LitNot( int Lit )                  { assert(Lit >= 0); return Lit ^ 1;                           }
static inline int      Abc_LitNotCond( int Lit, int c )       { assert(Lit >= 0); return Lit ^ (int)(c > 0);                }
static inline int      Abc_LitRegular( int Lit )              { assert(Lit >= 0); return Lit & ~01;                         }
static inline int      Abc_Lit2LitV( int * pMap, int Lit )    { assert(Lit >= 0); return Abc_Var2Lit( pMap[Abc_Lit2Var(Lit)], Abc_LitIsCompl(Lit) );      }
static inline int      Abc_Lit2LitL( int * pMap, int Lit )    { assert(Lit >= 0); return Abc_LitNotCond( pMap[Abc_Lit2Var(Lit)], Abc_LitIsCompl(Lit) );   }

static inline int      Abc_Ptr2Int( void * p )                { return (int)(ABC_PTRINT_T)p;      }
static inline void *   Abc_Int2Ptr( int i )                   { return (void *)(ABC_PTRINT_T)i;   }
static inline word     Abc_Ptr2Wrd( void * p )                { return (word)(ABC_PTRUINT_T)p;    }
static inline void *   Abc_Wrd2Ptr( word i )                  { return (void *)(ABC_PTRUINT_T)i;  }

static inline int      Abc_Var2Lit2( int Var, int Att )       { assert(!(Att >> 2)); return (Var << 2) + Att; }
static inline int      Abc_Lit2Var2( int Lit )                { assert(Lit >= 0);    return Lit >> 2;         }
static inline int      Abc_Lit2Att2( int Lit )                { assert(Lit >= 0);    return Lit & 3;          }
static inline int      Abc_Var2Lit3( int Var, int Att )       { assert(!(Att >> 3)); return (Var << 3) + Att; }
static inline int      Abc_Lit2Var3( int Lit )                { assert(Lit >= 0);    return Lit >> 3;         }
static inline int      Abc_Lit2Att3( int Lit )                { assert(Lit >= 0);    return Lit & 7;          }
static inline int      Abc_Var2Lit4( int Var, int Att )       { assert(!(Att >> 4)); return (Var << 4) + Att; }
static inline int      Abc_Lit2Var4( int Lit )                { assert(Lit >= 0);    return Lit >> 4;         }
static inline int      Abc_Lit2Att4( int Lit )                { assert(Lit >= 0);    return Lit & 15;         }


// Returns the next prime >= p
static inline int Abc_PrimeCudd( unsigned int p )
{
    int i,pn;
    p--;
    do {
        p++;
        if (p&1)
        {
            pn = 1;
            i = 3;
            while ((unsigned) (i * i) <= p)
            {
                if (p % (unsigned)i == 0) {
                    pn = 0;
                    break;
                }
                i += 2;
            }
        }
        else
            pn = 0;
    } while (!pn);
    return (int)(p);
}


#endif
