
#include "include/export.h"
int main()
{
    ComDocIO *io = ReadComDocFile("/home/mi/project/slicePro/sliceapi/data/1.mds");
    // CommonData block=ReadFromPath(io,"/DSI0/MoticDigitalSlideImage");
    ReleaseComDocFile(io);
    return 0;
}