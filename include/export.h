#pragma once
#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif
#include "ComDocIO.h"
using namespace ComDoc;
extern "C"
{
    struct CommonData
    {
        int len;
        char *p;

    };
    ComDocIO *ReadComDocFile(const char *fileName);
    void ReleaseComDocFile(ComDocIO *io);
    FileBlock* ReadFromPath(ComDocIO *io, const char *path);
    int GetBlockLength(FileBlock *block);
    unsigned char* GetBlockData(FileBlock *block);
    void ReleaseFileBlock(FileBlock *block);
}
