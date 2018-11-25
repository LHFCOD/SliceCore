#pragma once
#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#include "compound_doc_io.h"

using namespace compound_doc_io;
extern "C"
{
DLL_EXPORT ComDocIO *ReadComDocFile(const char *fileName);
DLL_EXPORT void ReleaseComDocFile(ComDocIO *io);
DLL_EXPORT FileBlock *ReadFromPath(ComDocIO *io, const char *path);
DLL_EXPORT uint32_t GetBlockLength(FileBlock *block);
DLL_EXPORT char *GetBlockData(FileBlock *block);
DLL_EXPORT void ReleaseFileBlock(FileBlock *block);
}
