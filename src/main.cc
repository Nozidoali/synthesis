/*
#include "cmdline.h"
#include "abc_parser.h"
#include "abc_write.h"
#include "abc_convert.h"
#include "abc_rewrite.h"
#include "abc_frame.h"
#include "contest_limit.h"
#include "contest_show.h"
#include "contest_rewrite.h"


using namespace std;
using namespace cmdline;


parser Cmdline_Parser( int argc, char * argv[] )
{
    parser option;
    option.add < string > ( "input",  'i', "Input BLIF file (SOP style)",  true, "./benchmarks/c17.blif");
    option.add < string > ( "output", 'o', "Output BLIF file (SOP style)", false, "");
    option.add < int >    ( "limit",  'l', "Maximum fanout Constraint",    false, 2);
    option.parse_check( argc, argv );
    return option;
}


int main( int argc, char * argv[] )
{
    // command line parser
    parser option = Cmdline_Parser( argc, argv );
    string input = option.get <string> ( "input" );
    string output = option.get <string> ( "output" );
    int limit = option.get <int> ("limit");
    if ( output == "" ) {
        int npos = input.find( ".blif" );
        if ( npos == static_cast < int >( string::npos ) )
            output = "output.blif";
        else {
            output = input;
            output.replace( npos, 5, "_constraint_" );
            stringstream ss;
            string str;
            ss << limit;
            ss >> str;
            output += str;
            output += ".blif";
        }
    }
    cout << "The AIG with maximum fanout constraint is in \"" << output << "\"" << endl;

    // initialize
    Abc_Start();
    Abc_Ntk_t * pNtkNetlist = Io_ReadBlif( const_cast <char *>(input.c_str()), 1 );
    int size0 = Abc_NtkNodeNum( pNtkNetlist );
    int level0 = Abc_NtkLevel( pNtkNetlist );
    Abc_Ntk_t * pNtkLogic = Abc_NtkToLogic( pNtkNetlist );
    Contest_PrintStats( pNtkLogic, true );
    Abc_NtkDelete( pNtkNetlist );
    Abc_Ntk_t * pNtkAig = Abc_NtkStrash( pNtkLogic, 1, 1, 0 );
    Abc_NtkDelete( pNtkLogic );

    // rewrite under maximum fanout constraint
    for ( int i = 0; i < 3; ++i ) {
        Abc_NtkRewrite( pNtkAig, 1, 1, 0, 0, 0 );
        Abc_NtkRewrite( pNtkAig, 1, 0, 0, 0, 0 );
    }

    // fix nodes exceeding maximum fanout constraint
    pNtkLogic = Abc_NtkToLogic( pNtkAig );
    Abc_NtkDelete( pNtkAig );
    Contest_LimitFanout( pNtkLogic, limit );

    // output
    Abc_NtkCleanup( pNtkLogic, 0 );
    Contest_PrintStats( pNtkLogic, false );
    pNtkNetlist = Abc_NtkToNetlist( pNtkLogic );
    Abc_NtkDelete( pNtkLogic );
    assert( Contest_LimitCheck( pNtkNetlist, limit ) );
    int size1 = Abc_NtkNodeNum( pNtkNetlist );
    int level1 = Abc_NtkLevel( pNtkNetlist );
    cout << "QoR = " << size0 / static_cast < float >( size1 ) + level0 / static_cast < float >(level1) << endl;
    Io_WriteBlif( pNtkNetlist, const_cast < char * >( output.c_str() ), 0, 0, 0 );
    Abc_NtkDelete( pNtkNetlist );

    Abc_Stop();

    return 0;
}
*/