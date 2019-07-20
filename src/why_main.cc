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


int main(int argc, char * argv[])
{
    Abc_Start();
    Rwr_Man_t * p = Rwr_ManStart(2);
    Abc_Stop();
    return 0;
}
