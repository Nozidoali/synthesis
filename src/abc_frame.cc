#include "abc_frame.h"


static Abc_Frame_t * s_GlobalFrame = NULL;


void * Abc_FrameReadManDec()
{
    if ( s_GlobalFrame->pManDec == NULL )
        s_GlobalFrame->pManDec = Dec_ManStart();
    return s_GlobalFrame->pManDec;
}


void Abc_Start()
{
    s_GlobalFrame = ABC_CALLOC( Abc_Frame_t, 1 );
    s_GlobalFrame->pManDec = NULL;
}


void Abc_Stop()
{
    if ( s_GlobalFrame->pManDec )
        Dec_ManStop( ( Dec_Man_t * )s_GlobalFrame->pManDec );
    ABC_FREE( s_GlobalFrame );
}
