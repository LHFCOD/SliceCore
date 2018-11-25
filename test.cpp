#include "src/export.h"
#include <iostream>
int main() {
    ComDocIO *io = ReadComDocFile("/home/mi/project/slicePro/sliceapi/data/1.mds");
    FileBlock *fileBlock = io->ReadFromPath("/DSI0/MoticDigitalSlideImage");
    for (int i = 0; i < fileBlock->length_; i++) {
        std::cout<<fileBlock->pointer_+i;
    }
    delete fileBlock;
    ReleaseComDocFile(io);
    return 0;
}