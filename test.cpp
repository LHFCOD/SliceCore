#include "src/export.h"
#include <iostream>

int main() {
    ComDocIO *io = ReadComDocFile("/home/mi/project/slicePro/sliceapi/data/1.mds");
    FileBlock *fileBlock = ReadFromPath(io, "/DSI0/MoticDigitalSlideImage");
    if (fileBlock != nullptr) {
        for (int i = 0; i < fileBlock->length_; i++) {
            std::cout << fileBlock->pointer_ + i;
        }
        ReleaseFileBlock(fileBlock);
    }
    ReleaseComDocFile(io);
    std::cout << "successfully end!" << std::endl;
    return 0;
}